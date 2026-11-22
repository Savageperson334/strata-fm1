/* SPDX-License-Identifier: MPL-2.0
 * STRATA: a drone and texture synth for the M-VAVE FM-1. One mode, every knob live.
 *
 *   keys            tap to start a drone voice at that note, tap again to let it fade (up to six at once)
 *   K1..K4, SELECT, ALGO, PRESET   the seven main controls; hold SEL for the seven more beneath them
 *   ARP             tap: the arpeggiator on / off; hold ARP and turn K1..K4: RATE, SHAPE, ORDER, FLOOR
 *   PLAY            all voices out (the chord is kept); again, the chord back in
 *   OCT- / OCT+     the keys' octave
 *   REC             MUTATE: nudge the sound somewhere nearby
 *   HOME (hold 1 s) the sound back to its defaults
 *   SAVE            keep the sound, the octave and the chord (PLAY brings the chord back after a power-on)
 *   MIDI in         notes play voices while held (CC 64 holds them); CC 20..37 the eighteen controls
 *
 * Part of fm1-base's unity build (device/unity.c, host/plat_host.c), after the platform: gfx.c and announce.c are in
 * scope. The sound engine is dsp/strata_dsp.c, a unit of its own at -O2. */
#include "fastmath.h"
#include "strata_dsp.h"

/* --------------------------------------------------------------- settings --- */
#define SET_MAGIC 0x53545232u            /* "STR2" (0.2, with the arp): another magic loads the defaults */
#define SET_MAGIC_V1 0x53545231u         /* "STR1" (0.1): read and carried over (settings_v1_t) */
typedef struct {
    uint32_t magic;
    float param[SD_NPARAM];
    int8_t octave;                       /* -3 .. +2: the keys start at F3 + 12 x octave */
    uint8_t nchord;                      /* the chord PLAY brings back */
    uint8_t chord[SD_VOICES];
    uint8_t arp;                         /* the arpeggiator on */
    uint8_t rsv[3];
} settings_t;
typedef struct {                         /* 0.1's record: 14 controls, no arp */
    uint32_t magic;
    float param[14];
    int8_t octave;
    uint8_t nchord;
    uint8_t chord[SD_VOICES];
    uint8_t rsv[4];
} settings_v1_t;
static settings_t set;
static uint8_t set_dirty;                /* changed since the last save: SAVE is lit */

static const float DEFAULTS[SD_NPARAM] = {
    0.35f, 0.10f, 0.25f, 0.45f, 0.55f, 0.30f, 0.08f,   /* TONE RATIO MOTION SPACE FILTER DRIFT GRIT */
    0.40f, 0.15f, 0.45f, 0.60f, 0.20f, 0.10f, 0.25f,   /* DEPTH BEAT FADE DECAY RESO AIR SUB */
    0.45f, 0.55f, 0.00f, 0.15f,                        /* RATE SHAPE ORDER FLOOR (the arp) */
};

static const char *const P_NAME[SD_NPARAM] = {"TONE", "RATIO", "MOTION", "SPACE", "FILTER", "DRIFT", "GRIT",
                                              "DEPTH", "BEAT", "FADE", "DECAY", "RESO", "AIR", "SUB",
                                              "RATE", "SHAPE", "ORDER", "FLOOR"};
static const char *const ORDER_NAME[4] = {"UP", "DOWN", "UP-DN", "RND"};
/* the encoders, in the order the screen shows them: K1 K2 K3 K4 on top, SELECT ALGO PRESET below */
static const uint8_t ENC_OF_CELL[7] = {EN_K1, EN_K2, EN_K3, EN_K4, EN_SELECT, EN_ALGO, EN_PRESET};
static const char *const ENC_LABEL[7] = {"K1", "K2", "K3", "K4", "SELECT", "ALGO", "PRESET"};

static void settings_defaults(void)
{
    int i;
    set.magic = SET_MAGIC;
    for (i = 0; i < SD_NPARAM; i++)
        set.param[i] = DEFAULTS[i];
    set.octave = -1;
    set.nchord = 2;                      /* F2 and C3: a fifth to start with */
    set.chord[0] = 41;
    set.chord[1] = 48;
    for (i = 2; i < SD_VOICES; i++)
        set.chord[i] = 0;
    set.arp = 0;
    for (i = 0; i < 3; i++)
        set.rsv[i] = 0;
}

static int settings_ok(void)
{
    int i;
    if (set.magic != SET_MAGIC || set.octave < -3 || set.octave > 2 || set.nchord > SD_VOICES || set.arp > 1u)
        return 0;
    for (i = 0; i < SD_NPARAM; i++)
        if (!(set.param[i] >= 0.0f && set.param[i] <= 1.0f))   /* (NaN fails too) */
            return 0;
    for (i = 0; i < set.nchord; i++)
        if (set.chord[i] > 127u)
            return 0;
    return 1;
}

static void settings_load(void)
{
    int got = plat_store_load(OBJ_SETTINGS, &set, sizeof set);
    if (got == (int)sizeof(settings_v1_t) && set.magic == SET_MAGIC_V1) {   /* 0.1's: kept, the arp's added */
        settings_v1_t v1;
        uint8_t *d = (uint8_t *)&v1;
        const uint8_t *src = (const uint8_t *)&set;
        uint32_t k;
        int i;
        for (k = 0; k < sizeof v1; k++)
            d[k] = src[k];
        settings_defaults();
        for (i = 0; i < 14; i++)
            set.param[i] = v1.param[i];
        set.octave = v1.octave;
        set.nchord = v1.nchord;
        for (i = 0; i < SD_VOICES; i++)
            set.chord[i] = v1.chord[i];
        got = (int)sizeof set;
    }
    if (got != (int)sizeof set || !settings_ok())
        settings_defaults();
}

static void params_to_engine(void)
{
    int i;
    for (i = 0; i < SD_NPARAM; i++)
        sd_param(i, set.param[i]);
    sd_arp(set.arp);
}

/* ----------------------------------------------------------------- voices --- */
/* which note each engine voice plays, and why: the main loop decides, the engine fades */
enum { SRC_NONE, SRC_KEY, SRC_MIDI };
static struct {
    int8_t note;                         /* -1: none */
    uint8_t gate;                        /* asked to sound */
    uint8_t src;
    uint32_t t;                          /* when it started: the oldest goes first */
} slot[SD_VOICES];
static uint32_t slot_clock;

static int voices_on(void)
{
    int s, n = 0;
    for (s = 0; s < SD_VOICES; s++)
        n += slot[s].gate != 0;
    return n;
}

static int slot_find(int note)           /* the sounding slot at note, or -1 */
{
    int s;
    for (s = 0; s < SD_VOICES; s++)
        if (slot[s].gate && slot[s].note == note)
            return s;
    return -1;
}

static void slot_off(int s)
{
    if (s < 0 || !slot[s].gate)
        return;
    slot[s].gate = 0;
    sd_voice_off(s);
    plat_midi_send(0x08u | 0x80u << 8 | (uint32_t)slot[s].note << 16, TO_USB);
}

/* a voice for note: a silent one, else the quietest fading one, else the oldest sounding one (it crossfades) */
static int slot_start(int note, int src)
{
    int s, best = -1;
    float lo = 2.0f;
    uint32_t oldest = 0xFFFFFFFFu;
    if (note < 0 || note > 127)
        return -1;
    for (s = 0; s < SD_VOICES && best < 0; s++)
        if (!slot[s].gate && !sd_voice_busy(s))
            best = s;
    for (s = 0; s < SD_VOICES && best < 0; s++)
        if (!slot[s].gate && sd_voice_level(s) < lo)
            lo = sd_voice_level(s);
    for (s = 0; s < SD_VOICES && best < 0 && lo < 2.0f; s++)
        if (!slot[s].gate && sd_voice_level(s) == lo)
            best = s;
    if (best < 0)
        for (s = 0; s < SD_VOICES; s++)
            if (slot[s].t < oldest) {
                oldest = slot[s].t;
                best = s;
            }
    if (slot[best].gate)
        plat_midi_send(0x08u | 0x80u << 8 | (uint32_t)slot[best].note << 16, TO_USB);
    slot[best].note = (int8_t)note;
    slot[best].gate = 1;
    slot[best].src = (uint8_t)src;
    slot[best].t = ++slot_clock;
    sd_voice_on(best, note);
    plat_midi_send(0x09u | 0x90u << 8 | (uint32_t)note << 16 | 100u << 24, TO_USB);
    return best;
}

static void all_off(void)
{
    int s;
    for (s = 0; s < SD_VOICES; s++)
        slot_off(s);
}

/* PLAY: the sounding notes become the chord and fade out; with none sounding, the chord fades back in */
static void play_button(void)
{
    int s, n = 0;
    if (voices_on()) {
        for (s = 0; s < SD_VOICES; s++)
            if (slot[s].gate)
                set.chord[n++] = (uint8_t)slot[s].note;
        set.nchord = (uint8_t)n;
        all_off();
        app_say("FADE", "OUT");
    } else if (set.nchord) {
        for (s = 0; s < set.nchord; s++)
            slot_start(set.chord[s], SRC_KEY);
        app_say("FADE", "IN");
    } else {
        app_say("PLAY", "TAP A KEY");
    }
}

/* ------------------------------------------------------------------- MIDI --- */
/* the render reads MIDI in (as the platform asks) and passes notes here through a small ring; controllers it sets
 * at once */
#define MQ 64u
static volatile uint32_t mq[MQ];
static volatile uint32_t mq_w, mq_r;
static volatile uint8_t cc_moved;        /* a controller changed a control: the screen and SAVE's light */
static uint8_t sustain;
static uint32_t midi_held[4];            /* notes held on MIDI, a bit each */

static void midi_in(void)                /* audio interrupt */
{
    uint32_t pkt;
    int port;
    while (plat_midi_in(&pkt, &port)) {
        uint32_t st = (pkt >> 8) & 0xF0u, d1 = (pkt >> 16) & 0x7Fu, d2 = (pkt >> 24) & 0x7Fu;
        if (st == 0xB0u && d1 >= 20u && d1 < 20u + SD_NPARAM) {   /* CC 20..37 */
            sd_param((int)(d1 - 20u), (float)d2 * (1.0f / 127.0f));
            cc_moved = 1;
        } else if (st == 0x90u || st == 0x80u || (st == 0xB0u && (d1 == 64u || d1 == 123u))) {
            if (mq_w - mq_r < MQ) {
                mq[mq_w & (MQ - 1u)] = pkt;
                mq_w++;
            }
        }
    }
}

static void midi_notes(void)             /* main loop: what the render queued */
{
    while (mq_r != mq_w) {
        uint32_t pkt = mq[mq_r & (MQ - 1u)];
        uint32_t st = (pkt >> 8) & 0xF0u, d1 = (pkt >> 16) & 0x7Fu, d2 = (pkt >> 24) & 0x7Fu;
        int s;
        mq_r++;
        if (st == 0x90u && d2) {
            midi_held[d1 >> 5] |= 1u << (d1 & 31u);
            if (slot_find((int)d1) < 0)
                slot_start((int)d1, SRC_MIDI);
        } else if (st == 0x80u || st == 0x90u) {
            midi_held[d1 >> 5] &= ~(1u << (d1 & 31u));
            s = slot_find((int)d1);
            if (s >= 0 && slot[s].src == SRC_MIDI && !sustain)
                slot_off(s);
        } else if (d1 == 64u) {
            sustain = d2 >= 64u;
            if (!sustain)                /* pedal up: the MIDI notes no longer held let go */
                for (s = 0; s < SD_VOICES; s++)
                    if (slot[s].gate && slot[s].src == SRC_MIDI &&
                        !(midi_held[slot[s].note >> 5] >> (slot[s].note & 31) & 1u))
                        slot_off(s);
        } else if (d1 == 123u) {         /* all notes off */
            for (s = 0; s < SD_VOICES; s++)
                if (slot[s].src == SRC_MIDI)
                    slot_off(s);
        }
    }
}

/* ----------------------------------------------------------------- render --- */
static float buf_l[APP_BLOCK], buf_r[APP_BLOCK];
static float gain_now;
static volatile uint8_t overloaded;

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i;
    float gain = (float)master_q12 * (1.0f / 4096.0f) * 8300000.0f;   /* MASTER, read once a block */
    midi_in();
    if (n > APP_BLOCK)
        n = APP_BLOCK;
    sd_render(buf_l, buf_r, n);
    for (i = 0; i < n; i++) {
        gain_now += (gain - gain_now) * 0.004f;   /* turning MASTER does not click */
        lr[2 * i] = (int32_t)(buf_l[i] * gain_now);
        lr[2 * i + 1] = (int32_t)(buf_r[i] * gain_now);
    }
}

void app_panic(void)
{
    int s;
    sd_panic();
    for (s = 0; s < SD_VOICES; s++)
        slot[s].gate = 0;
}

void app_overload(void) { overloaded = 1; }

void app_init(void)
{
    int s;
    sd_init();
    settings_load();
    params_to_engine();
    for (s = 0; s < SD_VOICES; s++) {    /* (a restart in the simulator keeps RAM: start clean) */
        slot[s].note = -1;
        slot[s].gate = 0;
        slot[s].src = SRC_NONE;
        slot[s].t = 0;
    }
    mq_r = mq_w;
    sustain = 0;
}

/* --------------------------------------------------------------------- text --- */
static void text_cat(char *d, const char *s)
{
    while (*d)
        d++;
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static void int_text(int32_t v, char *b)     /* signed, no padding */
{
    char t[12];
    int n = 0;
    uint32_t u = (uint32_t)(v < 0 ? -v : v);
    do
        t[n++] = (char)('0' + u % 10u);
    while ((u /= 10u) && n < 11);
    if (v < 0)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}

static void fixed_text(float v, int dec, char *b)   /* v with dec decimals (0..2), rounded */
{
    int32_t scale = dec == 2 ? 100 : dec == 1 ? 10 : 1, x = (int32_t)(v * (float)scale + 0.5f);
    char t[12];
    int_text(x / scale, b);
    if (dec) {
        int32_t f = x % scale;
        text_cat(b, ".");
        if (dec == 2 && f < 10)
            text_cat(b, "0");
        int_text(f, t);
        text_cat(b, t);
    }
}

static void hz_text(float hz, char *b)       /* 0.12 / 4.5 / 420 / 1.2k */
{
    if (hz < 1.0f)
        fixed_text(hz, 2, b);
    else if (hz < 10.0f)
        fixed_text(hz, 1, b);
    else if (hz < 1000.0f)
        fixed_text(hz, 0, b);
    else {
        fixed_text(hz * 0.001f, hz < 10000.0f ? 1 : 0, b);
        text_cat(b, "k");
    }
}

/* a control's value as the screen shows it, and its unit */
static const char *value_text(int p, char *b)
{
    float v = set.param[p];
    switch (p) {
    case P_RATIO: fixed_text(sd_ratio_of(v), 2, b); return "";
    case P_MOTION: hz_text(sd_motion_hz(v), b); return "Hz";
    case P_FILTER: hz_text(sd_cutoff_hz(v), b); return "Hz";
    case P_BEAT: fixed_text(sd_beat_hz(v), 2, b); return "Hz";
    case P_FADE: { float s = sd_fade_s(v); fixed_text(s, s < 10.0f ? 1 : 0, b); return "s"; }
    case P_DECAY: { float s = sd_decay_s(v); fixed_text(s, s < 10.0f ? 1 : 0, b); return "s"; }
    case P_ARATE: hz_text(sd_arp_hz(v), b); return "/s";
    case P_AORDER: b[0] = 0; text_cat(b, ORDER_NAME[sd_arp_order(v)]); return "";
    default: int_text((int32_t)(v * 100.0f + 0.5f), b); return "";
    }
}

static void note_name(int note, char *b)
{
    static const char *const N[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    char o[8];
    b[0] = 0;
    text_cat(b, N[note % 12]);
    int_text(note / 12 - 1, o);
    text_cat(b, o);
}

/* ----------------------------------------------------------------------- UI --- */
static struct {
    char msg[2][24];
    uint32_t msg_until;
    uint32_t keys, btn;
    uint8_t layer;                       /* 0 main, 1 SEL held, 2 ARP held */
    uint8_t arp_turned;                  /* a knob turned while ARP was held: its release does not switch the arp */
    int8_t touched;                      /* the cell turned last (lit a moment), -1 none */
    uint32_t touched_at;
    uint32_t enc_at[NE];                 /* when each encoder last moved: fast turns go further */
    uint32_t home_t;                     /* HOME went down (0: not held, or acted) */
    uint8_t redraw_head, redraw_params;
    uint32_t frame;
} ui;

/* the control on screen cell c (0..6) in the layer showing, -1 none */
static int cell_param(int c)
{
    if (ui.layer == 2)
        return c < 4 ? P_ARATE + c : -1;
    return c + 7 * ui.layer;
}

void app_say(const char *a, const char *b)
{
    uint32_t i;
    char t[48];
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1300u;
    ui.redraw_head = 1;
    if (ann_on()) {                      /* and to a screen reader (announce.c, fm1-reader) */
        t[0] = 0;
        text_cat(t, ui.msg[0]);
        if (ui.msg[1][0]) {
            text_cat(t, " ");
            text_cat(t, ui.msg[1]);
        }
        ann_say(t, 0, ANN_NOW);
    }
}

static void octave_text(char *t)
{
    t[0] = set.octave < 0 ? '-' : '+';
    t[1] = (char)('0' + (set.octave < 0 ? -set.octave : set.octave));
    t[2] = 0;
}

static void speak(void)                  /* a screen reader that has just started listening hears the whole state */
{
    char t[96], o[8];
    if (ann_on() && ann_where()) {
        t[0] = 0;
        text_cat(t, APP_NAME ". OCTAVE ");
        octave_text(o);
        text_cat(t, o);
        text_cat(t, ". VOICES ");
        int_text(voices_on(), o);
        text_cat(t, o);
        ann_say(t, 0, ANN_NOW);
    }
    ann_tick(plat_ms());
}

static void param_change(int p, int32_t detents, int fast)
{
    float step = (fast ? 3.0f : 1.0f) / 64.0f;
    float v = fm_clampf(set.param[p] + step * (float)detents, 0.0f, 1.0f);
    char t[16], u[24];
    const char *unit;
    if (v == set.param[p])
        return;
    set.param[p] = v;
    sd_param(p, v);
    set_dirty = 1;
    ui.redraw_params = 1;
    if (ann_on()) {
        unit = value_text(p, t);
        u[0] = 0;
        text_cat(u, t);
        text_cat(u, unit);
        ann_say(u, 0, ANN_NOW);
    }
}

static void mutate(void)                 /* REC: every main control nudged, up to an eighth of its range */
{
    static uint32_t r = 0x9E3779B9u;
    int p;
    r ^= plat_ms() * 2654435761u;
    for (p = 0; p < 7; p++) {
        float d;
        r ^= r << 13;
        r ^= r >> 17;
        r ^= r << 5;
        d = ((float)(r & 0xFFFFu) * (1.0f / 32768.0f) - 1.0f) * 0.125f;
        set.param[p] = fm_clampf(set.param[p] + d, 0.0f, 1.0f);
        sd_param(p, set.param[p]);
    }
    set_dirty = 1;
    ui.redraw_params = 1;
    app_say("MUTATE", "");
}

static void reset_sound(void)
{
    int p;
    for (p = 0; p < SD_NPARAM; p++)
        set.param[p] = DEFAULTS[p];
    params_to_engine();
    set_dirty = 1;
    ui.redraw_params = 1;
    app_say("SOUND", "RESET");
}

static void arp_toggle(void)
{
    set.arp = (uint8_t)!set.arp;
    sd_arp(set.arp);
    set_dirty = 1;
    ui.redraw_head = ui.redraw_params = 1;
    app_say("ARP", set.arp ? "ON" : "OFF");
}

static void octave_step(int d)
{
    int o = clamp(set.octave + d, -3, 2);
    char t[4];
    if (o == set.octave)
        return;
    set.octave = (int8_t)o;
    set_dirty = 1;
    octave_text(t);
    app_say("OCTAVE", t);
}

static void save(void)
{
    int s, n = 0;
    if (voices_on()) {                   /* the chord sounding now is the one PLAY brings back */
        for (s = 0; s < SD_VOICES; s++)
            if (slot[s].gate)
                set.chord[n++] = (uint8_t)slot[s].note;
        set.nchord = (uint8_t)n;
    }
    if (plat_store_save(OBJ_SETTINGS, &set, sizeof set) == 0) {
        set_dirty = 0;
        app_say("SAVED", "");
    } else {
        app_say("SAVE", "FAILED");
    }
}

static int key_note(uint32_t k) { return 53 + (int)k + 12 * set.octave; }   /* key 0 is F3 */

static void lights(void)
{
    uint32_t lit = 0, dim, keys = 0, dkeys = 0, k;
    int s;
    for (s = 0; s < SD_VOICES; s++) {    /* the keys of the notes sounding (bright) and fading (dim) */
        int rel = slot[s].note - key_note(0);
        if (slot[s].note < 0 || rel < 0 || rel >= NKEYS)
            continue;
        if (slot[s].gate)
            keys |= 1u << rel;
        else if (sd_voice_busy(s))
            dkeys |= 1u << rel;
    }
    if (ui.layer == 1)
        lit |= 1u << B_SEL;
    if (set.arp)
        lit |= 1u << B_ARP;
    if (voices_on())
        lit |= LED_PLAY_GREEN;
    if (set_dirty)
        lit |= 1u << B_SAVE;
    for (k = 0; k < NB; k++)             /* a held button lights while held */
        if (ui.btn >> k & 1u)
            lit |= 1u << k;
    dim = 1u << B_SEL | 1u << B_ARP | 1u << B_PLAY | 1u << B_OCTDN | 1u << B_OCTUP | 1u << B_REC | 1u << B_HOME | 1u << B_SAVE;
    plat_lights(lit, keys, dim, dkeys, 1);
}

static void input(void)
{
    uint32_t keys = plat_keys(), btn = plat_buttons(), k, now = plat_ms();
    int c;
    midi_notes();
    if (overloaded) {                    /* the render ran late: the oldest voice goes */
        uint32_t oldest = 0xFFFFFFFFu;
        int s, o = -1;
        overloaded = 0;
        for (s = 0; s < SD_VOICES; s++)
            if (slot[s].gate && slot[s].t < oldest) {
                oldest = slot[s].t;
                o = s;
            }
        if (o >= 0 && voices_on() > 2) {
            slot_off(o);
            app_say("OVERLOAD", "VOICE OFF");
        }
    }
    for (k = 0; k < NKEYS; k++) {        /* a key tapped: its note in or out */
        uint32_t bit = 1u << k;
        if ((keys & bit) && !(ui.keys & bit)) {
            int note = key_note(k), s = slot_find(note);
            char t[8];
            note_name(note, t);
            if (s >= 0) {
                slot_off(s);
                app_say(t, "OUT");
            } else {
                slot_start(note, SRC_KEY);
                app_say(t, "IN");
            }
        }
    }
    for (k = 0; k < NB; k++) {
        uint32_t bit = 1u << k;
        if (!(btn & bit) || (ui.btn & bit))
            continue;
        switch (k) {
        case B_OCTDN: octave_step(-1); break;
        case B_OCTUP: octave_step(1); break;
        case B_PLAY: play_button(); break;
        case B_REC: mutate(); break;
        case B_SAVE: save(); break;
        case B_HOME: ui.home_t = now ? now : 1u; break;
        case B_ARP: ui.arp_turned = 0; break;
        default: break;
        }
    }
    if ((ui.btn >> B_ARP & 1u) && !(btn >> B_ARP & 1u) && !ui.arp_turned)
        arp_toggle();                    /* ARP let go without a turn: the arp on / off */
    if (ui.home_t) {                     /* HOME held a second: the sound back to its defaults */
        if (!(btn >> B_HOME & 1u)) {
            ui.home_t = 0;
            app_say("HOLD HOME", "TO RESET");
        } else if (now - ui.home_t >= 1000u) {
            ui.home_t = 0;
            reset_sound();
        }
    }
    c = (btn >> B_ARP & 1u) ? 2 : (int)((btn >> B_SEL) & 1u);   /* ARP held: the arp's; SEL held: the layer beneath */
    if (c != ui.layer) {
        ui.layer = (uint8_t)c;
        ui.redraw_params = 1;
        ui.redraw_head = 1;
    }
    for (c = 0; c < 7; c++) {
        int en = ENC_OF_CELL[c];
        int32_t e = plat_enc(en);
        int fast;
        if (!e)
            continue;
        fast = e > 1 || e < -1 || now - ui.enc_at[en] < 45u;
        ui.enc_at[en] = now;
        if (ui.layer == 2)
            ui.arp_turned = 1;
        if (cell_param(c) < 0)
            continue;
        ui.touched = (int8_t)c;
        ui.touched_at = now;
        param_change(cell_param(c), e, fast);
    }
    if (cc_moved) {                      /* MIDI moved a control: show it, light SAVE */
        int p;
        cc_moved = 0;
        for (p = 0; p < SD_NPARAM; p++)
            set.param[p] = sd_param_get(p);
        set_dirty = 1;
        ui.redraw_params = 1;
    }
    ui.keys = keys;
    ui.btn = btn;
    lights();
}

void app_input_only(void) { input(); }

/* ----------------------------------------------------------------- drawing --- */
static void draw_head(void)
{
    char t[24];
    uint32_t now = plat_ms();
    int32_t x;
    cv_begin(240, 28, C_BLACK);
    if (now < ui.msg_until) {
        x = cv_text(8, 4, &FONT_B, ui.msg[0], C_WHITE);
        cv_text(x + 8, 4, &FONT_B, ui.msg[1], C_HI);
    } else {
        cv_text(8, 4, &FONT_B, APP_NAME, C_WHITE);
        if (ui.layer)
            cv_text(98, 9, &FONT_XS, ui.layer == 2 ? "ARP SETTINGS" : "MORE", C_HI);
        else if (set.arp)
            cv_text(98, 9, &FONT_XS, "ARP", C_AMB);
        octave_text(t + 4);
        t[0] = 'O'; t[1] = 'C'; t[2] = 'T'; t[3] = ' ';
        cv_text(232 - text_w(&FONT_XS, t), 9, &FONT_XS, t, C_GRAY);
    }
    cv_rect(0, 27, 240, 1, C_LINE);
    cv_blit(0, 0);
}

/* the sound as it is: its wave on top, the six voices below, each lit as loud as it is */
static void draw_scope(void)
{
    float sc[64];
    int32_t i, y0 = 34, amp = 30, px = 0, py = 0;
    cv_begin(240, 92, C_BLACK);
    sd_scope(sc);
    cv_rect(0, y0, 240, 1, C_LINE);
    for (i = 0; i < 64; i++) {
        int32_t x = (i * 239) / 63, y = y0 - (int32_t)(fm_clampf(sc[i] * 1.6f, -1.0f, 1.0f) * (float)amp);
        if (i)
            cv_line(px, py, x, y, C_HI);
        px = x;
        py = y;
    }
    {                                    /* the LFO: a dot moving across the bottom of the wave */
        int32_t lx = 120 + (int32_t)(sd_lfo() * 112.0f);
        cv_rect(lx - 2, 66, 5, 3, C_AMB);
    }
    for (i = 0; i < SD_VOICES; i++) {
        int32_t x = 4 + i * 39, h;
        float lv = sd_voice_level(i);
        char t[8];
        cv_rect(x, 72, 36, 18, C_LINE);
        h = (int32_t)(lv * 18.0f + 0.5f);
        if (h > 0)
            cv_rect(x, 90 - h, 36, h, slot[i].gate ? C_AMB : C_DIM);
        if (sd_arp_slot() == i) {        /* the arp's step: a white frame */
            cv_rect(x, 71, 36, 1, C_WHITE);
            cv_rect(x, 90, 36, 1, C_WHITE);
            cv_rect(x - 1, 71, 1, 20, C_WHITE);
            cv_rect(x + 36, 71, 1, 20, C_WHITE);
        }
        if (slot[i].note >= 0 && (slot[i].gate || lv > 0.01f)) {
            note_name(slot[i].note, t);
            cv_text(x + (36 - text_w(&FONT_XS, t)) / 2, 75, &FONT_XS, t, slot[i].gate ? C_WHITE : C_GRAY);
        }
    }
    cv_blit(0, 28);
}

static void draw_params(void)
{
    int c;
    uint32_t now = plat_ms();
    cv_begin(240, 120, C_BLACK);
    for (c = 0; c < 8; c++) {
        int32_t x = (c % 4) * 60, y = (c / 4) * 60, w;
        if (c < 7 && cell_param(c) < 0) {   /* the arp's layer uses K1..K4 only */
            cv_text(x + 4, y + 20, &FONT_B, "-", C_LINE);
            cv_text(x + 4, y + 40, &FONT_XS, ENC_LABEL[c], C_LINE);
        } else if (c < 7) {
            int p = cell_param(c), lit = ui.touched == c && now - ui.touched_at < 600u;
            char t[16];
            const char *unit = value_text(p, t);
            int32_t vx;
            cv_text(x + 4, y + 4, &FONT_XS, P_NAME[p], ui.layer ? C_HI : C_GRAY);   /* (layers 1, 2: lit) */
            vx = cv_text(x + 4, y + 20, &FONT_B, t, lit ? C_WHITE : C_HI);
            if (unit[0])
                cv_text(vx + 2, y + 26, &FONT_XS, unit, C_GRAY);
            cv_text(x + 4, y + 40, &FONT_XS, ENC_LABEL[c], C_DIM);
            w = (int32_t)(set.param[p] * 52.0f + 0.5f);   /* the value as a bar */
            cv_rect(x + 4, y + 53, 52, 3, C_LINE);
            if (w > 0)
                cv_rect(x + 4, y + 53, w, 3, lit ? C_WHITE : C_AMB);
        } else {                         /* the eighth cell: what SEL does */
            if (ui.layer == 2) {         /* ARP held: whether it's on, and how to switch it */
                cv_text(x + 4, y + 4, &FONT_XS, "ARP", C_WHITE);
                cv_text(x + 4, y + 20, &FONT_B, set.arp ? "ON" : "OFF", set.arp ? C_HI : C_DIM);
                cv_text(x + 4, y + 40, &FONT_XS, "TAP ARP", C_DIM);
            } else {
                cv_text(x + 4, y + 4, &FONT_XS, "HOLD SEL", ui.layer ? C_WHITE : C_GRAY);
                cv_text(x + 4, y + 16, &FONT_XS, ui.layer ? "LAYER 2" : "FOR MORE", ui.layer ? C_HI : C_DIM);
                cv_text(x + 4, y + 32, &FONT_XS, "HOLD ARP", C_GRAY);
                cv_text(x + 4, y + 44, &FONT_XS, set.arp ? "ARP IS ON" : "FOR ARP", set.arp ? C_AMB : C_DIM);
            }
        }
        if (c % 4)
            cv_rect(x, y + 4, 1, 52, C_LINE);
    }
    cv_rect(0, 59, 240, 1, C_LINE);
    cv_blit(0, 120);
}

void app_ui_init(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.touched = -1;
    ui.redraw_head = ui.redraw_params = 1;
}

void app_frame(void)
{
    static uint32_t lit_until;
    uint32_t now = plat_ms();
    input();
    speak();
    if (ui.msg_until && now >= ui.msg_until) {   /* a message goes */
        ui.msg_until = 0;
        ui.redraw_head = 1;
    }
    if (ui.touched >= 0 && now - ui.touched_at < 700u)
        lit_until = ui.touched_at + 600u;
    if (lit_until && now >= lit_until) {         /* a turned control's highlight goes */
        lit_until = 0;
        ui.redraw_params = 1;
    }
    if (ui.redraw_head) {
        ui.redraw_head = 0;
        draw_head();
    }
    if ((++ui.frame & 1u) == 0u)                 /* the wave at about 30 frames a second */
        draw_scope();
    if (ui.redraw_params) {
        ui.redraw_params = 0;
        draw_params();
    }
}

#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "voices"))
        *got = voices_on();
    else if (!strcmp(what, "octave"))
        *got = set.octave;
    else if (!strcmp(what, "layer"))
        *got = ui.layer;
    else if (!strcmp(what, "dirty"))
        *got = set_dirty;
    else if (!strcmp(what, "nchord"))
        *got = set.nchord;
    else if (!strcmp(what, "arp"))
        *got = set.arp;
    else if (what[0] == 'p' && what[1] >= '0' && what[1] <= '9')   /* pN: control N, x 100 */
        *got = (int)(set.param[atoi(what + 1) % SD_NPARAM] * 100.0f + 0.5f);
    else
        return 0;
    return 1;
}
#endif
