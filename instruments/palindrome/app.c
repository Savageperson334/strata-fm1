/* SPDX-License-Identifier: MPL-2.0 */
/* PALINDROME 1.0's panel and screen. MORPH makes a sound; REC catches it in one of three rings; the rings are
 * scanned, as a granular looper does and as a pocket sampler's keys chop and scrub a sample.
 *
 *   EDIT, HOME  SCAN: the selected ring's POSITION SIZE DENSITY PITCH, SPEED (ALGORITHM), SPREAD (PRESETS)
 *   FX          MORPH: X Y COMPLEX TONE, MOVE (ALGORITHM), SHAPE (PRESETS); the keys strike its notes
 *   ARP         MIX: RING 1, RING 2, RING 3, MORPH, DAMP (ALGORITHM), WIDTH (PRESETS)
 *   SELECT      the ring, on every page
 *   SEL         the keys on SCAN and MIX: SLICE (the white keys sixteen places round the ring, the black ones between)
 *               or PITCH (the ring transposed, the C as recorded)
 *   PLAY        the rings going round, on and off. A key held on SCAN or MIX sounds its ring either way
 *   REC         held, with a key: that MORPH note caught in the selected ring, at once, from any page. Tapped: on
 *               MORPH's page armed for the next note; elsewhere all that is heard but the selected ring caught in it
 *               now (the others resampled)
 *   ENV         FREEZE the selected ring (its head still); again, it goes on as it was
 *   LFO         REVERSE the selected ring
 *   HOME        SCAN; held a second, the selected ring emptied
 *   OCT- OCT+   the keys an octave down, up
 *   GLO (held)  the guide on the screen
 * On SCAN and MIX, while the selected ring is empty, a key strikes MORPH and catches the note in it.
 *
 * The screen: the three rings as circles, RING 1 innermost, each its recording's wave; the selected one bright. A
 * white notch is a ring's head, dots its grains. MORPH is the square in the middle (large on its own page, with the
 * path a note's hidden envelopes take it on, and its loudness over the ring). Top: the page, the keys' job, PLAY,
 * the ring. Foot: the knobs by what they set, placed as on the panel. */
#include "palindrome.h"
#include "fastmath.h"

enum { PG_SCAN, PG_MORPH, PG_MIX, NPAGES };
#define RING_PAR 0x80u                   /* (in PAGE: a control of the selected ring's own) */
#define PG_NONE 255
static const uint8_t PAGE[NPAGES][7] = {   /* KNOB1..4, SELECT (the ring: not here), ALGORITHM, PRESETS */
    {RING_PAR | R_POS, RING_PAR | R_SIZE, RING_PAR | R_DENSITY, RING_PAR | R_PITCH, PG_NONE, RING_PAR | R_SPEED, RING_PAR | R_SPREAD},
    {M_X, M_Y, M_COMPLEX, M_TONE, PG_NONE, M_MOVE, M_SHAPE},
    {G_RING1, G_RING2, G_RING3, G_MORPH, PG_NONE, G_DAMP, G_WIDTH}};
static const char *const NAME[NPAGES][7] = {
    {"POSITION", "SIZE", "DENSITY", "PITCH", "RING", "SPEED", "SPREAD"},
    {"X", "Y", "COMPLEX", "TONE", "RING", "MOVE", "SHAPE"},
    {"RING 1", "RING 2", "RING 3", "MORPH", "RING", "DAMP", "WIDTH"}};
static const uint8_t PAGE_LED[NPAGES] = {B_EDIT, B_FX, B_ARP};
static const char *const TITLE[NPAGES] = {"SCAN", "MORPH", "MIX"};
static const int enc[7] = {EN_K1, EN_K2, EN_K3, EN_K4, EN_SELECT, EN_ALGO, EN_PRESET};

static int page, sel, octave, playing, pitch_keys, armed, guide;
static int gate_ring = -1;               /* the ring the keys held are sounding, -1 none */
static int rec_used;                     /* a key was played while REC was held: its release does nothing more */
static int frozen[PM_RINGS];             /* FREEZE: the SPEED step it had, +1 (0: not frozen) */
static uint32_t home_t;                  /* HOME went down (0: not held, or acted) */
static uint32_t enc_at[7];               /* when each knob last moved: fast turns go further */
static uint32_t old_keys, old_btn, frames, message_until;
static int touched = -1;                 /* the knob turned last: its box white, its value in it, for a moment */
static uint32_t touched_at;
static char message[40];
static float gain_now, audio_l[APP_BLOCK], audio_r[APP_BLOCK];

/* ------------------------------------------------------------------ text --- */
static void number(int v, char *s)
{
    char t[12];
    int n = 0;
    unsigned u;
    if (v < 0) { *s++ = '-'; u = (unsigned)(-v); } else u = (unsigned)v;
    do { t[n++] = (char)('0' + u % 10u); u /= 10u; } while (u);
    while (n) *s++ = t[--n];
    *s = 0;
}
static char *put(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; return d; }
static char *end(char *s) { while (*s) s++; return s; }
static void fixed1(float v, char *s)     /* one decimal */
{
    int t = (int)(v * 10.f + (v < 0 ? -.5f : .5f));
    if (t < 0) { *s++ = '-'; t = -t; }
    number(t / 10, s); s = end(s); *s++ = '.'; *s++ = (char)('0' + t % 10); *s = 0;
}
void app_say(const char *a, const char *b)
{
    uint32_t i = 0;
    for (; a && a[i] && i < 38; i++) message[i] = a[i];
    if (b && *b && i < 37) { message[i++] = ' '; while (*b && i < 38) message[i++] = *b++; }
    message[i] = 0;
    message_until = plat_ms() + 1600u;
}
static void say_ring(const char *a, int r, const char *b)
{
    char t[40], *e = put(t, a);
    *e++ = (char)('1' + r); *e = 0;
    put(e, b);
    app_say(t, "");
}

/* ------------------------------------------------------------- the controls --- */
static float value(int pg, int j) { int id = PAGE[pg][j]; return id & RING_PAR ? pm_rget(sel, id & 0x7f) : pm_get(id); }
static void set_value(int pg, int j, float v)
{
    int id = PAGE[pg][j];
    if (id & RING_PAR) {
        if ((id & 0x7f) == R_POS) v -= fm_floorf(v);           /* POSITION goes round */
        pm_rset(sel, id & 0x7f, v);
    } else pm_set(id, v);
}
/* a box's value as text, 0: the plain 0..100 */
static const char *value_text(int pg, int j, char *t)
{
    int id = PAGE[pg][j];
    float v = value(pg, j);
    if (!(id & RING_PAR)) return 0;
    switch (id & 0x7f) {
    case R_SIZE: {
        float s = pm_size_s(v);
        if (s < 1.f) { number((int)(s * 1000.f + .5f), t); put(end(t), "ms"); }
        else { fixed1(s, t); put(end(t), "s"); }
        return t;
    }
    case R_DENSITY: put(t, "x"); fixed1(pm_overlap(v), t + 1); return t;
    case R_PITCH: { int s = pm_semis(v); t[0] = s > 0 ? '+' : 0; t[1] = 0; number(s, s > 0 ? t + 1 : t); return t; }
    case R_SPEED: {
        float s = pm_speed(v);
        if (s == 0) put(t, "STILL");
        else if (s == .25f || s == -.25f) put(t, s > 0 ? "x1/4" : "-x1/4");
        else { fixed1(s, t); put(end(t), "x"); }
        return t;
    }
    default: return 0;
    }
}

/* ------------------------------------------------------------------- the keys --- */
static int black(int k) { int o = k % 12; return o == 1 || o == 3 || o == 5 || o == 8 || o == 10; }   /* (key 0 is F) */
static int white_index(int k) { int n = 0; for (int i = 0; i < k; i++) n += !black(i); return n; }
static float slice_of(int k) { return ((float)white_index(k) - (black(k) ? .5f : 0.f)) * (1.f / 16.f); }
static int any_ring(void) { return pm_ring_count() > 0; }
/* a MORPH note; armed (or no ring yet), caught in the selected ring from its first sample */
static void strike(int k, int catch_it)
{
    if ((armed || catch_it) && pm_capture_ring() < 0) {
        if (!pm_capture(sel, PM_SRC_MORPH)) say_ring("CATCHING IN RING ", sel, "");
        armed = 0;
    }
    pm_note(41 + k + 12 * octave);
}
static void freeze(void)                 /* ENV: the selected ring's head still, or going on again */
{
    if (!pm_ring_ready(sel)) { say_ring("RING ", sel, " IS EMPTY"); return; }
    if (frozen[sel]) { pm_rset(sel, R_SPEED, pm_speed_value(frozen[sel] - 1)); frozen[sel] = 0; say_ring("RING ", sel, " GOES ON"); }
    else {
        int s = pm_speed_step(pm_rget(sel, R_SPEED));
        if (s == 4) { app_say("ALREADY STILL", ""); return; }
        frozen[sel] = s + 1;
        pm_rset(sel, R_SPEED, pm_speed_value(4));
        say_ring("RING ", sel, " FROZEN");
    }
}
static void reverse(void)                /* LFO: the selected ring's head the other way */
{
    static const uint8_t MIRROR[PM_SPEEDS] = {9, 7, 6, 5, 4, 3, 2, 1, 1, 0};   /* -2 <-> 2, -1 <-> 1 ... (1.5: -1) */
    int s = MIRROR[frozen[sel] ? frozen[sel] - 1 : pm_speed_step(pm_rget(sel, R_SPEED))];
    if (frozen[sel]) frozen[sel] = s + 1;
    else pm_rset(sel, R_SPEED, pm_speed_value(s));
    say_ring("RING ", sel, pm_speed(pm_speed_value(s)) < 0 ? " BACKWARD" : " FORWARD");
}

/* ------------------------------------------------------------------ the frame --- */
void app_init(void)
{
    pm_init();
    page = PG_SCAN; sel = 0; octave = 0; playing = 0; pitch_keys = 0; armed = 0; guide = 0; gate_ring = -1;
    rec_used = 0; home_t = 0;
    for (int r = 0; r < PM_RINGS; r++) frozen[r] = 0;
    for (int j = 0; j < 7; j++) enc_at[j] = 0;
    old_keys = old_btn = frames = message_until = 0; gain_now = 0; touched = -1;
}
void app_panic(void) { pm_panic(); playing = 0; armed = 0; gate_ring = -1; }
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    float gain = (float)master_q12 * (8300000.f / 4096.f);
    while (n) {
        uint32_t k = n > APP_BLOCK ? APP_BLOCK : n;
        pm_render(audio_l, audio_r, k);
        for (uint32_t i = 0; i < k; i++) {
            gain_now += (gain - gain_now) * .004f;
            *lr++ = (int32_t)(audio_l[i] * gain_now); *lr++ = (int32_t)(audio_r[i] * gain_now);
        }
        n -= k;
    }
}
static int heard_besides(int r)          /* something to resample into ring r */
{
    if (pm_notes()) return 1;
    for (int i = 0; i < PM_RINGS; i++) if (i != r && pm_ring_ready(i) && (playing || i == gate_ring) && pm_get(G_RING1 + i) > .02f) return 1;
    return 0;
}
static void rec(void)
{
    if (pm_capture_ring() >= 0) { app_say("RECORDING:", "IT ENDS BY ITSELF"); return; }
    if (armed) { armed = 0; app_say("REC", "NOT ARMED"); return; }
    if (page == PG_MORPH) { armed = 1; say_ring("ARMED: THE NEXT NOTE INTO RING ", sel, ""); return; }
    if (!heard_besides(sel)) { app_say("NOTHING HEARD:", "FX, REC, A KEY"); return; }
    if (!pm_capture(sel, PM_SRC_MIX)) say_ring("RESAMPLING INTO RING ", sel, "");
}
void app_input_only(void)
{
    uint32_t keys = plat_keys(), btn = plat_buttons(), newb = btn & ~old_btn, newk = keys & ~old_keys, now = plat_ms();
    int ev = pm_service();
    if (ev >= 0) {                       /* a capture finished: the ring plays */
        say_ring("RING ", ev, " CAUGHT");
        if (!playing) { playing = 1; pm_play(1); }
    } else if (ev == PM_EV_SILENT) app_say("NOTHING HEARD:", "THE RING STAYS EMPTY");
    guide = (int)((btn >> B_GLO) & 1u);
    if (guide) { newb &= 0; newk = 0; }
    if (newb & ((1u << B_EDIT) | (1u << B_HOME))) page = PG_SCAN;
    if (newb & (1u << B_HOME)) home_t = now ? now : 1u;
    if (home_t) {                        /* HOME held a second: the selected ring emptied */
        if (!(btn >> B_HOME & 1u)) home_t = 0;
        else if (now - home_t >= 1000u) {
            home_t = 0;
            if (pm_ring_ready(sel)) { pm_ring_clear(sel); frozen[sel] = 0; say_ring("RING ", sel, " EMPTIED"); }
        }
    }
    if (newb & (1u << B_ENV)) freeze();
    if (newb & (1u << B_LFO)) reverse();
    if (newb & (1u << B_FX)) page = PG_MORPH;
    if (newb & (1u << B_ARP)) page = PG_MIX;
    if (newb & (1u << B_SEL)) { pitch_keys = !pitch_keys; app_say("KEYS:", pitch_keys ? "PITCH" : "SLICE"); }
    if (newb & (1u << B_PLAY)) { playing = !playing; pm_play(playing); }
    if (newb & (1u << B_REC)) rec_used = 0;
    if ((old_btn >> B_REC & 1u) && !(btn >> B_REC & 1u) && !rec_used && !guide) rec();   /* REC let go, no key: tapped */
    if (newb & (1u << B_SAVE)) app_say("SAVE:", "NOTHING TO SAVE");
    if ((newb & (1u << B_OCTDN)) && octave > -3) octave--;
    if ((newb & (1u << B_OCTUP)) && octave < 2) octave++;
    /* the keys: MORPH's notes on its page (and while there is no ring); on SCAN and MIX the selected ring's slices
     * or pitches, sounding while held */
    for (int k = 0; k < NKEYS; k++) {
        if (!(newk & (1u << k))) continue;
        if (btn >> B_REC & 1u) { rec_used = 1; strike(k, 1); continue; }   /* REC held: this note, caught now */
        if (page == PG_MORPH) { strike(k, 0); continue; }
        if (!pm_ring_ready(sel)) { strike(k, pm_capture_ring() < 0); continue; }   /* an empty ring catches it */
        if (pitch_keys) pm_key_pitch(sel, k - 7 + 12 * octave);
        else pm_jump(sel, slice_of(k));
        if (gate_ring >= 0 && gate_ring != sel) pm_gate(gate_ring, 0);
        gate_ring = sel;
        pm_gate(sel, 1);
    }
    if (gate_ring >= 0 && (!(keys & ((1u << NKEYS) - 1u)) || page == PG_MORPH)) { pm_gate(gate_ring, 0); gate_ring = -1; }
    for (int j = 0; j < 7; j++) {
        int d = plat_enc(enc[j]);
        if (!d || guide) continue;
        touched = j; touched_at = now;
        if (j != 4 && now - enc_at[j] < 45u && PAGE[page][j] != (RING_PAR | R_SPEED) && PAGE[page][j] != (RING_PAR | R_PITCH))
            d *= 4;                      /* (a fast turn goes further) */
        enc_at[j] = now;
        if (PAGE[page][j] == (RING_PAR | R_SPEED)) {   /* SPEED: one step a detent */
            int s = pm_speed_step(pm_rget(sel, R_SPEED)) + (d > 0 ? 1 : -1);
            pm_rset(sel, R_SPEED, pm_speed_value(s));
            frozen[sel] = 0;
            continue;
        }
        if (j == 4) {                    /* SELECT: the ring, on every page */
            int s = sel + (d > 0 ? 1 : -1);
            sel = s < 0 ? 0 : s >= PM_RINGS ? PM_RINGS - 1 : s;
            continue;
        }
        set_value(page, j, value(page, j) + (float)d * (PAGE[page][j] == (RING_PAR | R_PITCH) ? 1.f / 48.f : 1.f / 64.f));
    }
    old_keys = keys; old_btn = btn;
    {
        /* the lights: the page's button; PLAY green while the rings go round; REC blinking armed, lit recording;
         * the keys: MORPH's sounding notes' ... no: on SCAN the key under the selected ring's head (SLICE) or its
         * pitch (PITCH) */
        uint32_t lit = (1u << PAGE_LED[page]) | (playing ? LED_PLAY_GREEN : 0), kl = 0;
        if (pm_capture_ring() >= 0 || (armed && ((now >> 8) & 1u))) lit |= 1u << B_REC;
        if (guide) lit |= 1u << B_GLO;
        if (pitch_keys) lit |= 1u << B_SEL;
        if (frozen[sel]) lit |= 1u << B_ENV;
        if (pm_speed(pm_rget(sel, R_SPEED)) < 0) lit |= 1u << B_LFO;
        if (page != PG_MORPH && pm_ring_ready(sel)) {
            if (pitch_keys) {
                int k = pm_key_semis(sel) + 7 - 12 * octave;
                if (k >= 0 && k < NKEYS) kl = 1u << k;
            } else if (pm_ring_level(sel) > .05f) {
                int w = (int)(pm_head(sel) * 16.f), k = 0;
                for (int i = 0; i < NKEYS; i++) if (!black(i) && white_index(i) == w) k = i;
                kl = 1u << k;
            }
        }
        plat_lights(lit, kl, (1u << B_FX) | (1u << B_SEL) | (1u << B_EDIT) | (1u << B_ENV) | (1u << B_LFO) | (1u << B_GLO) | (1u << B_HOME) |
                             (1u << B_ARP) | (1u << B_PLAY) | (1u << B_REC) | (1u << B_OCTDN) | (1u << B_OCTUP), 0, 1);
    }
}

/* ------------------------------------------------------------- the picture --- */
#define PD_CX 120
#define PD_CY 96
#define PD_AMP 6                         /* how far a ring's wave reaches in and out of its circle */
static const int32_t PD_R[PM_RINGS] = {42, 59, 76};   /* RING 1 innermost */
static int8_t pd_sin[256], pd_cos[256];
/* the screen is drawn as two strips of 240 x 120 (the canvas holds no more): the whole picture twice, each time the
 * part in the strip kept. No line of text crosses from one strip to the other */
static int32_t pd_y0;
static void pd_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    if ((y0 < pd_y0 && y1 < pd_y0) || (y0 >= pd_y0 + 120 && y1 >= pd_y0 + 120)) return;
    cv_line(x0, y0 - pd_y0, x1, y1 - pd_y0, c);
}
static void pd_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { cv_rect(x, y - pd_y0, w, h, c); }
static int32_t pd_text(int32_t x, int32_t y, const char *s, uint16_t c)
{
    if (y < pd_y0 || y + 14 > pd_y0 + 120) return x + text_w(&FONT_XS, s);
    return cv_text(x, y - pd_y0, &FONT_XS, s, c);
}
static void pd_frame(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    pd_box(x, y, w, 1, c); pd_box(x, y + h - 1, w, 1, c); pd_box(x, y, 1, h, c); pd_box(x + w - 1, y, 1, h, c);
}
static void pd_polar(int32_t r, uint32_t a, int32_t *x, int32_t *y)
{
    a &= 255u; *x = PD_CX + r * pd_sin[a] / 127; *y = PD_CY - r * pd_cos[a] / 127;   /* 0 at the top, clockwise */
}
static uint32_t pd_turn(float t) { return (uint32_t)(int32_t)((t - fm_floorf(t)) * 256.f) & 255u; }
/* a knob's box, 60 x 20 at column col, row row of the foot: its name and a bar; turned, in white with its value */
static void pd_knob(int32_t col, int32_t row, const char *name, float v, int lit, uint16_t fill, const char *val)
{
    int32_t x = col * 60, y = 200 + row * 20, w = (int32_t)(v * 52.f + .5f);
    if (lit || val) {
        char t[12], nm[12];
        int32_t tw, n = 0;
        if (val) { while (val[n] && n < 11) { t[n] = val[n]; n++; } t[n] = 0; n = 0; }
        else number((int)(v * 100 + .5f), t);
        tw = text_w(&FONT_XS, t);
        while (name[n] && n < 11) { nm[n] = name[n]; n++; }
        nm[n] = 0;
        while (n > 1 && text_w(&FONT_XS, nm) > 52 - tw - 4) nm[--n] = 0;
        pd_text(x + 4, y + 1, nm, lit ? C_WHITE : C_GRAY);
        pd_text(x + 56 - tw, y + 1, t, lit ? C_WHITE : C_HI);
    } else pd_text(x + 4, y + 1, name, C_GRAY);
    pd_box(x + 4, y + 15, 52, 3, C_LINE);
    if (w > 0) pd_box(x + 4, y + 15, w, 3, lit ? C_WHITE : fill);
}
static float ring_lo[PM_RINGS][128], ring_hi[PM_RINGS][128], pd_scope[32], g_at[PM_GRAINS], g_env[PM_GRAINS];
static int g_on[PM_GRAINS], g_ring[PM_GRAINS];

static void pd_ring(int r, uint16_t c, int dense)
{
    int32_t x0, y0, x1, y1, rad = PD_R[r];
    for (int i = 0; i < 128; i++) {
        uint32_t a = (uint32_t)i * 2u;
        int32_t a0, a1;
        if (ring_hi[r][i] < ring_lo[r][i]) {                      /* nothing there (yet): dots */
            if (!(i & 1)) { pd_polar(rad, a, &x0, &y0); pd_box(x0, y0, 1, 1, C_LINE); }
            continue;
        }
        a0 = (int32_t)(ring_lo[r][i] * 9.f); a1 = (int32_t)(ring_hi[r][i] * 9.f);
        if (a0 < -PD_AMP) a0 = -PD_AMP;
        if (a1 > PD_AMP) a1 = PD_AMP;
        pd_polar(rad + a0, a, &x0, &y0); pd_polar(rad + a1, a, &x1, &y1);
        pd_line(x0, y0, x1, y1, c);
        if (dense) { pd_polar(rad + a0, a + 1u, &x0, &y0); pd_polar(rad + a1, a + 1u, &x1, &y1); pd_line(x0, y0, x1, y1, c); }
    }
}
/* MORPH's square, half its side h: where the sound is among the four corners and the sound itself round it; large
 * (its own page): the corners named, the path a note's hidden envelopes take it on (to the mirror image and back) */
static void pd_square(int32_t h, int big)
{
    static const char *const CORNER[4] = {"FM", "MODAL", "ADD", "FOLD"};   /* top left, top right, bottom left, right */
    float mx = pm_morph_x(), my = pm_morph_y(), w[4];
    int32_t dx = PD_CX - h + (int32_t)(mx * (float)(2 * h)), dy = PD_CY + h - (int32_t)(my * (float)(2 * h));
    int32_t x0 = 0, y0 = 0, x1, y1, k, loop = big ? 16 : 7;
    uint16_t on = pm_notes() ? C_HI : C_DIM;
    w[0] = (1 - mx) * my; w[1] = mx * my; w[2] = (1 - mx) * (1 - my); w[3] = mx * (1 - my);
    pd_box(PD_CX - h, PD_CY - h, 2 * h + 1, 2 * h + 1, C_BLACK);
    pd_frame(PD_CX - h, PD_CY - h, 2 * h + 1, 2 * h + 1, big ? C_GRAY : C_LINE);
    if (big) {
        float bx = pm_get(M_X), by = pm_get(M_Y), mv = pm_get(M_MOVE);
        int32_t ax = PD_CX - h + (int32_t)(bx * (float)(2 * h)), ay = PD_CY + h - (int32_t)(by * (float)(2 * h));
        int32_t ex = PD_CX - h + (int32_t)((bx + mv * (1.f - 2.f * bx)) * (float)(2 * h));
        int32_t ey = PD_CY + h - (int32_t)((by + mv * (1.f - 2.f * by)) * (float)(2 * h));
        for (k = 0; k < 4; k++)
            pd_text((k & 1) ? PD_CX + h - 3 - text_w(&FONT_XS, CORNER[k]) : PD_CX - h + 4, (k & 2) ? PD_CY + h - 14 : PD_CY - h + 1,
                    CORNER[k], w[k] > .4f ? C_HI : w[k] > .12f ? C_GRAY : C_DIM);
        for (k = 0; k <= 12; k++) {                               /* the path: the knobs' place to its mirror */
            int32_t px = ax + (ex - ax) * k / 12, py = ay + (ey - ay) * k / 12;
            pd_box(px, py, 1, 1, C_GRAY);
        }
        pd_box(ax - 1, ay - 1, 3, 3, C_GRAY);
    }
    for (k = 0; k <= 32; k++) {                                   /* the sound, as a loop round its place */
        int32_t r = loop * 3 / 8 + (int32_t)(pd_scope[k & 31] * (float)loop);
        if (r < 1) r = 1;
        if (r > loop - 1) r = loop - 1;
        x1 = dx + r * pd_sin[(k * 8) & 255] / 127; y1 = dy - r * pd_cos[(k * 8) & 255] / 127;
        if (x1 < PD_CX - h + 1) x1 = PD_CX - h + 1;
        if (x1 > PD_CX + h - 1) x1 = PD_CX + h - 1;
        if (y1 < PD_CY - h + 1) y1 = PD_CY - h + 1;
        if (y1 > PD_CY + h - 1) y1 = PD_CY + h - 1;
        if (k) pd_line(x0, y0, x1, y1, on);
        x0 = x1; y0 = y1;
    }
    pd_box(dx - 1, dy - 1, 3, 3, pm_notes() ? C_WHITE : C_GRAY);
}
/* MORPH's page: a note's loudness over one ring (SHAPE), where the newest note is in it */
static void pd_envelope(void)
{
    int32_t x0 = 20, y0 = 196, w = 200, h = 12, px = 0, py = 0;
    float s = pm_get(M_SHAPE), age = pm_note_age();
    for (int32_t i = 0; i <= w; i += 4) {
        float u = (float)i / (float)w, a = .004f + .55f * s * s, k = .3f + 5.f * (1.f - s), v;
        v = u < a ? u / a : fm_expf(-k * (u - a) / (1.f - a));
        if (u > .9f) v *= (1.f - u) * 10.f;
        int32_t x = x0 + i, y = y0 - (int32_t)(v * (float)h);
        if (i) pd_line(px, py, x, y, C_GRAY);
        px = x; py = y;
    }
    pd_box(x0, y0 + 1, w + 1, 1, C_LINE);
    if (age >= 0) { int32_t x = x0 + (int32_t)(age * (float)w); pd_line(x, y0 - h - 2, x, y0 + 1, C_WHITE); }
}
/* GLO held: what each button does */
static void pd_guide(void)
{
    static const char *const G[12][3] = {
        {"FX", "MORPH", "SYNTH"}, {"SEL", "KEYS:", "SL/PI"}, {"ENV", "FREE-", "ZE"}, {"LFO", "REV-", "ERSE"}, {"EDIT", "SCAN", "PAGE"}, {"GLO", "THIS", "GUIDE"},
        {"HOME", "SCAN", "PAGE"}, {"SAVE", "-", ""}, {"ARP", "MIX", "PAGE"}, {"SEQ", "-", ""}, {"PLAY", "RINGS", "ON OFF"}, {"REC", "+KEY:", "CATCH"}};
    pd_text(4, 2, "PALINDROME GUIDE", C_HI);
    pd_text(236 - text_w(&FONT_XS, "LET GO OF GLO"), 2, "LET GO OF GLO", C_GRAY);
    for (int i = 0; i < 12; i++) {
        int32_t x = (i % 6) * 40, y = 17 + (i / 6) * 42;
        int used = G[i][1][0] != '-', here = (i == 0 && page == PG_MORPH) || (i == 4 && page == PG_SCAN) || (i == 8 && page == PG_MIX) ||
                                             (i == 2 && frozen[sel]);
        pd_frame(x + 1, y, 38, 41, here ? C_HI : C_LINE);
        pd_text(x + 3, y + 1, G[i][0], used ? C_GRAY : C_DIM);
        pd_text(x + 3, y + 13, G[i][1], used ? C_WHITE : C_DIM);
        pd_text(x + 3, y + 25, G[i][2], C_HI);
    }
    pd_text(4, 104, "SELECT: THE RING, ON EVERY PAGE", C_WHITE);
    pd_text(4, 122, "FX: KEYS STRIKE MORPH, ONE RING LONG", C_WHITE);
    pd_text(4, 136, "REC HELD + A KEY: CAUGHT IN THE RING", C_GRAY);
    pd_text(4, 150, "SCAN: KEYS SLICE THE RING (OR PITCH)", C_WHITE);
    pd_text(4, 164, "REC TAP THERE: THE OTHERS RESAMPLED", C_GRAY);
    pd_text(4, 180, "HOME HELD: EMPTY THE RING", C_GRAY);
}
static void pd_scene(void)
{
    int morph = page == PG_MORPH, cap = pm_capture_ring(), i;
    int32_t x0, y0, x1, y1, tx;
    char t[40], *e;
    if (guide) pd_guide();
    else {
        /* --- the rings --- */
        for (i = 0; i < PM_RINGS; i++) {
            uint16_t c = i == cap ? C_WHITE : i == sel ? (morph ? C_GRAY : C_HI) : pm_ring_level(i) > .05f ? C_AMB : C_DIM;
            pd_ring(i, c, i == sel && !morph);
        }
        if (cap >= 0) {                                           /* the capture's head */
            uint32_t a = pd_turn(pm_capture_progress());
            pd_polar(PD_R[cap] - PD_AMP - 2, a, &x0, &y0); pd_polar(PD_R[cap] + PD_AMP + 2, a, &x1, &y1);
            pd_line(x0, y0, x1, y1, C_WHITE);
        }
        for (i = 0; i < PM_RINGS; i++) if (pm_ring_ready(i) && i != cap) {   /* each ring's head */
            uint32_t a = pd_turn(pm_head(i));
            int32_t r = PD_R[i];
            pd_polar(r + PD_AMP + 1, a, &x0, &y0); pd_polar(r + PD_AMP + 5, a, &x1, &y1);
            pd_line(x0, y0, x1, y1, pm_ring_level(i) > .05f ? C_WHITE : C_DIM);
        }
        for (i = 0; i < PM_GRAINS; i++) if (g_on[i]) {             /* the grains, on their rings */
            int32_t s = g_env[i] > .6f ? 1 : 0;
            pd_polar(PD_R[g_ring[i]], pd_turn(g_at[i]), &x0, &y0);
            pd_box(x0 - s, y0 - s, 2 * s + 1, 2 * s + 1, g_env[i] > .4f ? (morph ? C_GRAY : C_WHITE) : C_AMB);
        }
        pd_square(morph ? 30 : 18, morph);
        if (morph) pd_envelope();

        /* --- top: the page, the keys' job, PLAY, the ring --- */
        tx = pd_text(4, 1, TITLE[page], C_HI);
        e = put(t, "RING "); *e++ = (char)('1' + sel); *e = 0;
        x1 = 236 - text_w(&FONT_XS, t);
        pd_text(x1, 1, t, pm_ring_ready(sel) ? C_WHITE : C_GRAY);
        x1 -= 13;                                                /* PLAY: a triangle going round, a square stopped */
        if (playing) for (i = 0; i < 5; i++) pd_box(x1 + i, 3 + i, 1, 9 - 2 * i, C_WHITE);
        else pd_box(x1, 4, 7, 7, C_DIM);
        if (morph && cap >= 0) { e = put(t, "CATCHING "); number((int)(pm_capture_progress() * 100.f), e); put(end(e), "%"); }
        else if (morph && armed) put(t, "ARMED: NEXT NOTE");
        else if (morph || !any_ring()) put(t, "KEYS: NOTES");
        else if (pitch_keys) {
            int s = pm_key_semis(sel);
            e = put(t, "KEYS: PITCH ");
            if (s > 0) *e++ = '+';
            number(s, e);
        } else put(t, "KEYS: SLICE");
        {
            int32_t tw = text_w(&FONT_XS, t), mid = (tx + x1) / 2;
            if (tw <= x1 - tx - 8) pd_text(mid - tw / 2, 1, t, morph ? (cap >= 0 || armed ? C_WHITE : C_GRAY) : C_AMB);
        }
        /* --- REC: what it would catch, and how it stands --- */
        if (!morph) {
            if (cap >= 0) {
                e = put(t, "RING "); *e++ = (char)('1' + cap); *e = 0;
                e = put(e, pm_capture_source() == PM_SRC_MORPH ? " FROM MORPH " : " FROM THE OTHERS ");
                number((int)(pm_capture_progress() * 100.f), e); put(end(e), "%");
            } else if (armed) { e = put(t, "ARMED: NEXT NOTE INTO RING "); *e++ = (char)('1' + sel); *e = 0; }
            else { e = put(t, "REC: THE OTHERS INTO RING "); *e++ = (char)('1' + sel); *e = 0; }
            pd_text(4, 184, t, cap >= 0 ? C_WHITE : armed ? ((plat_ms() >> 8) & 1u ? C_WHITE : C_AMB) : C_GRAY);
        }
        /* --- a message, or what to do first --- */
        {
            const char *first = message_until && (int32_t)(message_until - plat_ms()) > 0 ? message : 0, *second = 0;
            uint16_t c = C_WHITE;
            if (!first && !any_ring() && cap < 0) { first = "TAP A KEY"; second = "IT IS CAUGHT IN RING 1"; c = C_GRAY; }
            if (first) {
                int32_t tw = text_w(&FONT_XS, first), tw2 = second ? text_w(&FONT_XS, second) : 0, w = (tw > tw2 ? tw : tw2) + 12;
                int32_t y = second ? 140 : 150, h = second ? 32 : 18;
                pd_box(PD_CX - w / 2, y, w, h, C_LINE);
                pd_box(PD_CX - w / 2 + 1, y + 1, w - 2, h - 2, C_BLACK);
                pd_text(PD_CX - tw / 2, y + 2, first, c);
                if (second) pd_text(PD_CX - tw2 / 2, y + 16, second, c);
            }
        }
    }
    /* --- the knobs by what they set, as on the panel: MASTER SELECT | KNOB1 KNOB2 over PRESETS ALGORITHM | KNOB3 KNOB4 */
    {
        static const uint8_t COL[7] = {2, 3, 2, 3, 1, 1, 0}, ROW[7] = {0, 0, 1, 1, 0, 1, 1};
        int lit = touched >= 0 && plat_ms() - touched_at < 900u ? touched : -1;
        uint16_t fill = morph ? C_HI : C_AMB;
        pd_knob(0, 0, "VOLUME", (float)plat_master() * (1.f / 4096.f), 0, C_GRAY, 0);
        for (i = 0; i < 7; i++) {
            if (i == 4) { t[0] = (char)('1' + sel); t[1] = 0; pd_knob(COL[i], ROW[i], "RING", (float)(sel + 1) / 3.f, lit == i, C_WHITE, t); continue; }
            {
                char v[16];
                const char *vt = value_text(page, i, v);
                pd_knob(COL[i], ROW[i], NAME[page][i], value(page, i), lit == i, fill, vt);
            }
        }
        pd_box(119, 201, 2, 38, C_DIM);
    }
}
static void draw(void)
{
    for (int i = 0; i < PM_RINGS; i++) pm_ring_wave(i, ring_lo[i], ring_hi[i], 128);
    pm_scope(pd_scope, 32);
    for (int i = 0; i < PM_GRAINS; i++) g_on[i] = pm_grain(i, &g_at[i], &g_env[i], &g_ring[i]);
    for (int i = 0; i < 2; i++) {
        pd_y0 = i * 120;
        cv_begin(240, 120, C_BLACK);
        pd_scene();
        cv_blit(0, (uint32_t)pd_y0);
    }
}
void app_ui_init(void)
{
    for (int i = 0; i < 256; i++) {
        pd_sin[i] = (int8_t)(fm_sin_turns((float)i * (1.f / 256.f)) * 127.f);
        pd_cos[i] = (int8_t)(fm_sin_turns((float)i * (1.f / 256.f) + .25f) * 127.f);
    }
    lcd_fill(0, 0, 240, 240, C_BLACK);
    app_say(APP_NAME, APP_VERSION);
    message_until = plat_ms() + 2500u;
    draw();
}
void app_frame(void) { app_input_only(); if ((++frames & 3u) == 0) draw(); }
#ifdef PLAT_HOST                         /* the simulator's checks (strcmp: the FM-1's toolchain has no <string.h>) */
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "page")) *got = page;
    else if (!strcmp(what, "rings")) *got = pm_ring_count();
    else if (!strcmp(what, "ring")) *got = sel + 1;
    else if (!strcmp(what, "ready")) *got = pm_ring_ready(sel);
    else if (!strcmp(what, "capturing")) *got = pm_capture_ring() >= 0;
    else if (!strcmp(what, "playing")) *got = playing;
    else if (!strcmp(what, "armed")) *got = armed;
    else if (!strcmp(what, "pitchkeys")) *got = pitch_keys;
    else if (!strcmp(what, "notes")) *got = pm_notes();
    else if (!strcmp(what, "grains")) *got = pm_active_grains();
    else if (!strcmp(what, "guide")) *got = guide;
    else if (!strcmp(what, "gate")) *got = gate_ring + 1;
    else if (!strcmp(what, "semis")) *got = pm_key_semis(sel);
    else if (!strcmp(what, "head")) *got = (int)(pm_head(sel) * 100.f);
    else if (!strcmp(what, "pos")) *got = (int)(pm_rget(sel, R_POS) * 100.f + .5f);
    else if (!strcmp(what, "speed")) *got = (int)(pm_speed(pm_rget(sel, R_SPEED)) * 100.f + (pm_speed(pm_rget(sel, R_SPEED)) < 0 ? -.5f : .5f));
    else if (!strcmp(what, "pitch")) *got = pm_semis(pm_rget(sel, R_PITCH));
    else if (!strcmp(what, "move")) *got = (int)(pm_get(M_MOVE) * 100.f + .5f);
    else if (!strcmp(what, "mx")) *got = (int)(pm_morph_x() * 100.f + .5f);
    else return 0;
    return 1;
}
#endif
