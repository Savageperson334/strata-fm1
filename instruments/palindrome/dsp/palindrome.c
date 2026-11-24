/* SPDX-License-Identifier: MPL-2.0 */
/* PALINDROME's engine (dsp/palindrome.h says what it is). Shared by the FM-1, the simulator and the browser.
 *
 * Who touches what: the panel (pm_set, pm_note, pm_capture, pm_jump ...) only leaves requests and targets; the render
 * (the audio interrupt on the FM-1) acts on them at the start of its next block, and alone writes the rings, the
 * voices and the grains. A ring being captured is not ready, so nothing reads it while it is written. */
#include "palindrome.h"
#include "fastmath.h"
#include "plat.h"                        /* PLAT_POOL */
#define INV_RATE (1.0f/44100.0f)
#define SEAM 256u                        /* a capture's ends eased over this many samples: the loop closes clean */
#ifdef FM1_WORKSPACE                     /* in a firmware of several instruments (STRABBLEDROME): the rings in the
                                          * memory they share, of which only the instrument running has the use */
extern uint32_t FM1_WORKSPACE[];
#define memory ((int16_t (*)[PM_BUFFER])FM1_WORKSPACE)
#else
static int16_t memory[PM_RINGS][PM_BUFFER] PLAT_POOL;
#endif

typedef struct {
    uint32_t age;                        /* samples since it was struck; PM_NOTE: over */
    float phase, m1, m2, modal, filt, inc, amp, x, y;
    int note;
} Voice;
typedef struct {
    float pos, inc, age, step, amp, gain;
    int ring, active, dying;
} Grain;

static Voice voice[PM_VOICES];
static Grain grain[PM_GRAINS];
static volatile float target[PM_PARAMS], rtarget[PM_RINGS][PM_RPARAMS];
static float p[PM_PARAMS], rp[PM_RINGS][PM_RPARAMS];
static const float DEFAULTS[PM_PARAMS] = {
    .30f, .65f, .40f, .55f, .45f, .42f,  /* X Y COMPLEX TONE MOVE SHAPE: modal-ish FM, moving, a bell */
    .80f, .80f, .80f, .70f, .25f, .55f,  /* the rings' levels, MORPH's, DAMP, WIDTH */
};
static const float RDEFAULTS[PM_RPARAMS] = {
    0.f, .55f, .70f, .5f, 7.f / 9.f, 0.f,   /* POSITION, SIZE 0.17 s, DENSITY 2 (the plain recording), PITCH 0,
                                          * SPEED as recorded, SPREAD none */
};
/* requests from the panel: each a count the render follows */
#define NQ 8u
static volatile int note_q[NQ];
static volatile uint32_t note_w;
static uint32_t note_r;
static volatile int playing, gate[PM_RINGS], key_semis[PM_RINGS];
static volatile float jump_to[PM_RINGS];
static volatile uint32_t jump_req[PM_RINGS];
static uint32_t jump_seen[PM_RINGS];
static volatile int cap_req_ring, cap_req_src;
static volatile uint32_t cap_req;
static uint32_t cap_seen;
/* the rings */
static volatile int ready[PM_RINGS];
static float head[PM_RINGS], pos_now[PM_RINGS], ring_amp[PM_RINGS], spawn_at[PM_RINGS], spawn_next[PM_RINGS];
static float ring_lp[PM_RINGS];
static volatile float head_now[PM_RINGS], level_now[PM_RINGS];
/* a capture */
enum { CAP_IDLE, CAP_REC, CAP_FINISH };
static volatile int cap_state, cap_ring = -1, cap_src;
static volatile uint32_t cap_i;
static volatile int cap_event = -1;      /* for pm_service: the ring finished, PM_EV_SILENT, or -1 */
static uint32_t norm_i;
static float cap_peak, cap_gain, cap_dc, cap_lp, half_sample;
static int half;
/* the rest */
static uint32_t rng = 0x9E3779B9u;
static float dc_l, dc_r, prev_l, prev_r, scope[64];
static volatile uint32_t scope_wr;
static volatile float morph_x, morph_y, newest_age = -1.f;
static int newest = -1;

static float fclamp(float x, float a, float b) { return x < a ? a : x > b ? b : x; }
static float wrap(float x) { return x - fm_floorf(x); }
static float rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float)(rng >> 8) * (1.0f / 16777216.0f); }
static float soft(float x) { return x / (1.0f + (x < 0 ? -x : x)); }

/* ------------------------------------------------------------------ the panel's side --- */
void pm_set(int id, float v) { if (id >= 0 && id < PM_PARAMS) target[id] = fclamp(v, 0, 1); }
float pm_get(int id) { return id >= 0 && id < PM_PARAMS ? target[id] : 0; }
void pm_rset(int r, int id, float v) { if (r >= 0 && r < PM_RINGS && id >= 0 && id < PM_RPARAMS) rtarget[r][id] = fclamp(v, 0, 1); }
float pm_rget(int r, int id) { return r >= 0 && r < PM_RINGS && id >= 0 && id < PM_RPARAMS ? rtarget[r][id] : 0; }
float pm_size_s(float v) { return .02f * fm_exp2f(5.64f * fclamp(v, 0, 1)); }          /* 20 ms .. 1 s */
float pm_overlap(float v) { return .25f * fm_exp2f(4.32f * fclamp(v, 0, 1)); }         /* .25 .. 5 */
int pm_semis(float v) { float s = (fclamp(v, 0, 1) - .5f) * 48.f; return (int)(s < 0 ? s - .5f : s + .5f); }
static const float SPEED[PM_SPEEDS] = {-2.f, -1.f, -.5f, -.25f, 0.f, .25f, .5f, 1.f, 1.5f, 2.f};
int pm_speed_step(float v) { int i = (int)(fclamp(v, 0, 1) * (float)(PM_SPEEDS - 1) + .5f); return i; }
float pm_speed(float v) { return SPEED[pm_speed_step(v)]; }
float pm_speed_value(int i) { return (float)(i < 0 ? 0 : i >= PM_SPEEDS ? PM_SPEEDS - 1 : i) * (1.f / (float)(PM_SPEEDS - 1)); }
void pm_note(int note)
{
    if (note < 0 || note > 127) return;
    note_q[note_w % NQ] = note;
    note_w++;
}
int pm_notes(void) { int n = 0; for (int j = 0; j < PM_VOICES; j++) n += voice[j].note >= 0 && voice[j].age < PM_NOTE; return n; }
float pm_note_age(void) { return newest_age; }
float pm_morph_x(void) { return morph_x; }
float pm_morph_y(void) { return morph_y; }
void pm_play(int on) { playing = on != 0; }
void pm_gate(int r, int on) { if (r >= 0 && r < PM_RINGS) gate[r] = on != 0; }
void pm_jump(int r, float pos) { if (r >= 0 && r < PM_RINGS) { jump_to[r] = wrap(pos); jump_req[r]++; } }
void pm_key_pitch(int r, int semis) { if (r >= 0 && r < PM_RINGS) key_semis[r] = semis < -48 ? -48 : semis > 48 ? 48 : semis; }
int pm_key_semis(int r) { return r >= 0 && r < PM_RINGS ? key_semis[r] : 0; }
int pm_ring_ready(int r) { return r >= 0 && r < PM_RINGS && ready[r]; }
void pm_ring_clear(int r)
{
    if (r < 0 || r >= PM_RINGS || pm_capture_ring() == r) return;
    ready[r] = 0;
    for (int i = 0; i < PM_GRAINS; i++) if (grain[i].active && grain[i].ring == r) grain[i].dying = 1;
}
int pm_ring_count(void) { int n = 0; for (int r = 0; r < PM_RINGS; r++) n += ready[r]; return n; }
float pm_head(int r) { return r >= 0 && r < PM_RINGS ? head_now[r] : 0; }
float pm_ring_level(int r) { return r >= 0 && r < PM_RINGS ? level_now[r] : 0; }
#ifdef PLAT_HOST
const int16_t *pm_ring_pcm(int r) { return pm_ring_ready(r) ? memory[r] : 0; }
#endif
void pm_ring_wave(int r, float *lo, float *hi, uint32_t n)
{
    uint32_t written = 0, step;
    if (r < 0 || r >= PM_RINGS || !n) return;
    if (ready[r]) written = PM_BUFFER;
    else if (cap_state == CAP_REC && cap_ring == r) written = cap_i;
    step = PM_BUFFER / n;
    for (uint32_t k = 0; k < n; k++) {
        uint32_t a = k * step, b = a + step;
        float l = 1, h = -1;
        if (b <= written)
            for (uint32_t i = a; i < b; i += 4) {
                float v = (float)memory[r][i] * (1.f / 32768.f);
                if (v < l) l = v;
                if (v > h) h = v;
            }
        lo[k] = l; hi[k] = h;
    }
}
int pm_grain(int i, float *at, float *env, int *r)
{
    Grain *g;
    if (i < 0 || i >= PM_GRAINS) return 0;
    g = &grain[i];
    if (!g->active) return 0;
    *at = g->pos * (1.f / (float)PM_BUFFER);
    *env = fm_sin_turns(g->age * .5f);
    *r = g->ring;
    return 1;
}
int pm_active_grains(void) { int n = 0; for (int i = 0; i < PM_GRAINS; i++) n += grain[i].active; return n; }
int pm_capture(int r, int src)
{
    if (r < 0 || r >= PM_RINGS || cap_state != CAP_IDLE || cap_req != cap_seen) return -1;
    cap_req_ring = r; cap_req_src = src;
    cap_req++;
    return 0;
}
int pm_capture_ring(void) { return cap_state != CAP_IDLE ? cap_ring : cap_req != cap_seen ? cap_req_ring : -1; }
int pm_capture_source(void) { return cap_req != cap_seen ? cap_req_src : cap_src; }
float pm_capture_progress(void)
{
    if (cap_state == CAP_REC) return (float)cap_i * (1.f / (float)PM_BUFFER);
    return cap_state == CAP_FINISH ? 1.f : 0.f;
}
int pm_service(void)
{
    int e = cap_event;
    if (e == -1) return -1;
    if (e >= 0) ready[e] = 1;                                  /* (its samples final: from now it may be read) */
    cap_event = -1;
    cap_state = CAP_IDLE;
    return e;
}
#ifdef PLAT_WEB
/* For the browser emulator's guide (web/emu/app_worklet.js reads it, scripts/tutorial/tutorial.js follows it): the
 * rings recorded (bits 0..3), the ring being captured plus one (4..7), a capture running (8), how far it is in percent
 * (9..15), grains sounding (16..20). Only in the WebAssembly build. */
__attribute__((used, visibility("default"))) uint32_t pm_web_state(void)
{
    int c = pm_capture_ring();
    return (uint32_t)pm_ring_count() | ((uint32_t)(c + 1) << 4) | ((uint32_t)(c >= 0) << 8) |
           ((uint32_t)(pm_capture_progress() * 100.f) << 9) | ((uint32_t)(pm_active_grains() & 31) << 16);
}
#endif
void pm_scope(float *out, uint32_t n)
{
    uint32_t w = scope_wr;
    for (uint32_t i = 0; i < n; i++) out[i] = scope[(w - n + i) & 63u];
}
void pm_panic(void)
{
    for (int j = 0; j < PM_VOICES; j++) { voice[j].note = -1; voice[j].age = PM_NOTE; voice[j].amp = 0; }
    for (int i = 0; i < PM_GRAINS; i++) grain[i].active = 0;
    note_r = note_w;
    playing = 0;
    for (int r = 0; r < PM_RINGS; r++) { gate[r] = 0; ring_amp[r] = 0; }
    if (cap_state == CAP_REC) { cap_state = CAP_IDLE; ready[cap_ring] = 0; cap_ring = -1; }  /* (a half capture: dropped) */
    cap_seen = cap_req;
    dc_l = dc_r = prev_l = prev_r = 0;
}
void pm_init(void)
{
    for (int i = 0; i < PM_PARAMS; i++) target[i] = p[i] = DEFAULTS[i];
    for (int r = 0; r < PM_RINGS; r++) {
        for (int i = 0; i < PM_RPARAMS; i++) rtarget[r][i] = rp[r][i] = RDEFAULTS[i];
        ready[r] = 0; head[r] = pos_now[r] = 0; ring_amp[r] = 0; spawn_at[r] = 0; spawn_next[r] = 1;
        ring_lp[r] = 0; key_semis[r] = 0; gate[r] = 0; jump_seen[r] = jump_req[r];
    }
    cap_state = CAP_IDLE; cap_ring = -1; cap_event = -1; cap_seen = cap_req;
    newest = -1; newest_age = -1;
    morph_x = DEFAULTS[M_X]; morph_y = DEFAULTS[M_Y];
    pm_panic();
}

/* --------------------------------------------------------------------- the render --- */
/* a note's loudness at u (0..1 through its life). SHAPE: a pluck (0) .. a bell .. a swell (1); always back to nothing
 * at the ring's end, so a note recorded loops without a seam */
static float shape_amp(float u, float s)
{
    float a = .004f + .55f * s * s, k = .3f + 5.f * (1.f - s), v;
    if (u >= 1.f) return 0;
    v = u < a ? u / a : fm_expf(-k * (u - a) / (1.f - a));
    if (u > .9f) v *= (1.f - u) * 10.f;
    return v;
}
static float read_ring(const int16_t *m, float pos)
{
    uint32_t i = (uint32_t)pos;
    float f = pos - (float)i, a = m[i & (PM_BUFFER - 1u)], b = m[(i + 1u) & (PM_BUFFER - 1u)];
    return (a + (b - a) * f) * (1.f / 32768.f);
}
static void spawn(int r)
{
    int n = 0, free = -1;
    float size = pm_size_s(rp[r][R_SIZE]), ov = pm_overlap(rp[r][R_DENSITY]), sp = rp[r][R_SPREAD];
    float semis = (float)pm_semis(rp[r][R_PITCH]) + (float)key_semis[r];
    for (int i = 0; i < PM_GRAINS; i++) {
        if (grain[i].active && grain[i].ring == r) n++;
        else if (!grain[i].active && free < 0) free = i;
    }
    if (n >= PM_RING_GRAINS || free < 0) return;
    Grain *g = &grain[free];
    g->ring = r;
    g->pos = wrap(head[r] + pos_now[r] + sp * (rnd() - .5f) * .3f) * (float)PM_BUFFER;
    g->inc = .5f * fm_exp2f(semis * (1.f / 12.f));            /* (the ring is at half rate) */
    g->age = 0;
    g->step = 1.f / (size * 44100.f);
    g->amp = 1.f / fm_sqrtf(ov < 2.f ? 1.f : ov * .5f);
    g->gain = 1; g->dying = 0; g->active = 1;
}
void pm_render(float *left, float *right, uint32_t n)
{
    float vw[PM_VOICES][4], vc[PM_VOICES], vt[PM_VOICES], va0[PM_VOICES], dva[PM_VOICES];
    float step[PM_RINGS], interval[PM_RINGS], rlev[PM_RINGS], pan[PM_RINGS], damp, inv_n;
    int capturing, sounding = 0;
    if (!n) return;
    inv_n = 1.f / (float)n;
    for (int j = 0; j < PM_PARAMS; j++) p[j] += (target[j] - p[j]) * .15f;
    for (int r = 0; r < PM_RINGS; r++)
        for (int j = 0; j < PM_RPARAMS; j++) rp[r][j] += (rtarget[r][j] - rp[r][j]) * (j == R_POS ? .3f : .15f);

    /* --- requests --- */
    if (cap_req != cap_seen) {
        cap_seen = cap_req;
        if (cap_state == CAP_IDLE) {
            int r = cap_req_ring;
            cap_ring = r; cap_src = cap_req_src; ready[r] = 0;
            for (int i = 0; i < PM_GRAINS; i++) if (grain[i].active && grain[i].ring == r) grain[i].dying = 1;
            cap_i = 0; cap_peak = 0; cap_dc = 0; cap_lp = 0; half = 0; norm_i = 0;
            cap_state = CAP_REC;
        }
    }
    while (note_r != note_w) {                                 /* a note struck: a free voice, or the oldest */
        int note = note_q[note_r % NQ], v = 0;
        note_r++;
        for (int j = 1; j < PM_VOICES; j++) if (voice[j].age > voice[v].age) v = j;
        voice[v].note = note; voice[v].age = 0;
        voice[v].inc = 440.f * fm_exp2f(((float)note - 69.f) * (1.f / 12.f)) * INV_RATE;
        voice[v].phase = voice[v].m1 = voice[v].m2 = voice[v].filt = 0; voice[v].modal = 1; voice[v].amp = 0;
        newest = v;
    }
    for (int r = 0; r < PM_RINGS; r++) {
        uint32_t k = jump_req[r];
        if (k != jump_seen[r]) {                               /* a slice: the head there (the old grains fade out) */
            jump_seen[r] = k;
            head[r] = wrap(jump_to[r] - pos_now[r]);
            for (int i = 0; i < PM_GRAINS; i++) if (grain[i].active && grain[i].ring == r) grain[i].dying = 1;
            spawn_at[r] = spawn_next[r];                       /* (a grain from there at once) */
        }
    }

    /* --- MORPH: each note's place in its life, and the hidden envelopes from the knobs as they are now --- */
    for (int j = 0; j < PM_VOICES; j++) {
        Voice *v = &voice[j];
        float u = (float)v->age * (1.f / (float)PM_NOTE), u1 = (float)(v->age + n) * (1.f / (float)PM_NOTE);
        float c = fm_sin_turns(fclamp(u, 0, 1) * .5f), mv = p[M_MOVE], x, y;
        va0[j] = v->amp;
        dva[j] = 0;
        if (v->note < 0 || v->age >= PM_NOTE) { vw[j][0] = vw[j][1] = vw[j][2] = vw[j][3] = 0; vc[j] = vt[j] = 0; v->amp = 0; continue; }
        sounding++;
        /* X and Y toward their mirror image and back (a palindrome); brighter early, plainer late; darker on */
        x = fclamp(p[M_X] + mv * (1.f - 2.f * p[M_X]) * c, 0, 1);
        y = fclamp(p[M_Y] + mv * (1.f - 2.f * p[M_Y]) * c, 0, 1);
        v->x = x; v->y = y;
        vw[j][0] = (1 - x) * y; vw[j][1] = x * y; vw[j][2] = (1 - x) * (1 - y); vw[j][3] = x * (1 - y);
        vc[j] = fclamp(p[M_COMPLEX] * (1.f + mv * (.9f * (1.f - u) - .45f)), 0, 1);
        {
            float t = p[M_TONE] * (1.f - .55f * mv * u);
            vt[j] = .02f + .62f * t * t;
        }
        dva[j] = (shape_amp(u1, p[M_SHAPE]) - va0[j]) * inv_n;
    }
    if (newest >= 0 && voice[newest].note >= 0 && voice[newest].age < PM_NOTE) {
        morph_x = voice[newest].x; morph_y = voice[newest].y;
        newest_age = (float)voice[newest].age * (1.f / (float)PM_NOTE);
    } else { morph_x = p[M_X]; morph_y = p[M_Y]; newest_age = -1.f; }

    /* --- the rings: heads, how often grains start, levels --- */
    for (int r = 0; r < PM_RINGS; r++) {
        int on = ready[r] && (playing || gate[r]);
        float size = pm_size_s(rp[r][R_SIZE]), ov = pm_overlap(rp[r][R_DENSITY]);
        step[r] = pm_speed(rp[r][R_SPEED]) * (1.f / (float)PM_NOTE);
        interval[r] = size * 44100.f / ov;
        rlev[r] = on ? p[G_RING1 + r] * p[G_RING1 + r] * 1.2f : 0.f;
        pan[r] = .5f + ((float)r - 1.f) * .4f * p[G_WIDTH];
        pos_now[r] = rp[r][R_POS];
        if (!on && ring_amp[r] < .001f) ring_amp[r] = 0;
    }
    damp = .08f + .9f * (1.f - p[G_DAMP]) * (1.f - p[G_DAMP]);
    capturing = cap_state == CAP_REC;

    for (uint32_t i = 0; i < n; i++) {
        float dl = 0, dr = 0, wl = 0, wr = 0, w_ring[PM_RINGS] = {0, 0, 0}, morph, l, rr;
        /* MORPH's notes */
        for (int j = 0; j < PM_VOICES; j++) {
            Voice *v = &voice[j];
            float a, base, sig = 0, cx, val, pn;
            if (v->note < 0 || v->age >= PM_NOTE) continue;
            v->amp = va0[j] + dva[j] * (float)i;
            a = v->phase; base = fm_sin_turns(a); cx = vc[j];
            if (vw[j][0] > .0001f) sig += vw[j][0] * fm_sin_turns(a + (.06f + .65f * cx) * fm_sin_turns(a * 2.f));
            if (vw[j][1] > .0001f) sig += vw[j][1] * (.55f * base + (.28f * fm_sin_turns(v->m1) + .17f * fm_sin_turns(v->m2)) * (.3f + .7f * cx) * v->modal);
            if (vw[j][2] > .0001f) sig += vw[j][2] * (base + cx * (.55f * fm_sin_turns(a * 2.f) + .32f * fm_sin_turns(a * 3.f) + .13f * fm_sin_turns(a * 5.f))) / (1.f + cx);
            if (vw[j][3] > .0001f) {
                float q = wrap(base * (1.f + 3.f * cx) * .25f + .25f), folded = 1.f - 4.f * (q > .5f ? q - .5f : .5f - q);
                sig += vw[j][3] * ((1 - cx) * base + cx * folded);
            }
            v->modal *= .99997f;
            v->phase = wrap(a + v->inc);
            v->m1 = wrap(v->m1 + v->inc * 2.756f);
            v->m2 = wrap(v->m2 + v->inc * 5.404f);
            v->filt += (sig - v->filt) * vt[j];
            val = v->filt * v->amp * .34f / (sounding > 1 ? .7f + .3f * (float)sounding : 1.f);
            pn = .5f + ((float)j - 1.5f) * .12f;
            dl += val * (1.f - pn); dr += val * pn;
            v->age++;
        }
        /* the rings: heads round, grains started, read and windowed */
        for (int r = 0; r < PM_RINGS; r++) {
            float want = rlev[r] > 0 ? 1.f : 0.f;
            ring_amp[r] += (want - ring_amp[r]) * .002f;
            if (!ready[r] && ring_amp[r] < .001f) continue;
            head[r] = wrap(head[r] + step[r]);
            if (want > 0) {
                spawn_at[r] += 1.f;
                if (spawn_at[r] >= spawn_next[r]) {
                    float sp = rp[r][R_SPREAD];
                    spawn_at[r] = 0;
                    spawn_next[r] = interval[r] * (1.f + sp * .9f * (rnd() * 2.f - 1.f));
                    if (spawn_next[r] < 64.f) spawn_next[r] = 64.f;
                    spawn(r);
                }
            }
        }
        for (int k = 0; k < PM_GRAINS; k++) {
            Grain *g = &grain[k];
            float w, s;
            int r;
            if (!g->active) continue;
            r = g->ring;
            w = fm_sin_turns(g->age * .5f);
            s = read_ring(memory[r], g->pos) * w * w * g->amp * g->gain;
            w_ring[r] += s;
            g->pos += g->inc;
            if (g->pos >= (float)PM_BUFFER) g->pos -= (float)PM_BUFFER;
            g->age += g->step;
            if (g->age >= 1.f) g->active = 0;
            if (g->dying) { g->gain -= 1.f / 96.f; if (g->gain <= 0) g->active = 0; }
        }
        for (int r = 0; r < PM_RINGS; r++) {
            float s;
            ring_lp[r] += (w_ring[r] - ring_lp[r]) * damp;
            s = ring_lp[r] * ring_amp[r] * (rlev[r] > 0 ? rlev[r] : p[G_RING1 + r] * p[G_RING1 + r] * 1.2f);
            w_ring[r] = s;
            wl += s * (1.f - pan[r]); wr += s * pan[r];
        }
        morph = p[G_MORPH] * p[G_MORPH] * 1.3f;
        /* a capture: at half rate, MORPH alone, or all that is heard but the ring it writes */
        if (capturing) {
            float src = cap_src == PM_SRC_MORPH ? (dl + dr) : (dl + dr) * morph;
            if (cap_src != PM_SRC_MORPH) for (int r = 0; r < PM_RINGS; r++) if (r != cap_ring) src += w_ring[r];
            cap_lp += (src - cap_lp) * .55f;
            if (!half) { half_sample = cap_lp; half = 1; }
            else {
                uint32_t ci = cap_i;
                float in = (half_sample + cap_lp) * .5f;
                half = 0;
                cap_dc += (in - cap_dc) * .006f; in -= cap_dc;          /* nothing under about 20 Hz */
                if (ci < SEAM) in *= (float)ci * (1.f / SEAM);
                else if (ci >= PM_BUFFER - SEAM) in *= (float)(PM_BUFFER - 1u - ci) * (1.f / SEAM);
                in = fclamp(in, -.999f, .999f);
                if (in > cap_peak) cap_peak = in;
                if (-in > cap_peak) cap_peak = -in;
                memory[cap_ring][ci] = (int16_t)(in * 32767.f);
                cap_i = ++ci;
                if (ci >= PM_BUFFER) { capturing = 0; cap_state = CAP_FINISH; norm_i = 0; }
            }
        }
        l = dl * morph + wl;
        rr = dr * morph + wr;
        {
            float ol = l - prev_l + .9995f * dc_l, orr = rr - prev_r + .9995f * dc_r;
            prev_l = l; prev_r = rr; dc_l = ol; dc_r = orr;
            left[i] = soft(ol * 1.3f); right[i] = soft(orr * 1.3f);
        }
        if ((i & 15u) == 0) scope[scope_wr++ & 63u] = (left[i] + right[i]) * .5f;
    }
    for (int r = 0; r < PM_RINGS; r++) {
        head_now[r] = wrap(head[r] + pos_now[r]);
        level_now[r] = ring_amp[r] * (ready[r] ? 1.f : 0.f);
        ring_lp[r] = fm_flush(ring_lp[r]);
    }
    /* a finished capture: its level evened (the loudest sample to about .7, at most four times up), a stretch a
     * block, then handed to pm_service; nothing in it: left empty */
    if (cap_state == CAP_FINISH && cap_event == -1) {
        if (!norm_i) cap_gain = cap_peak < .004f ? 0.f : fclamp(.7f / cap_peak, .5f, 4.f);
        if (cap_gain > 0) {
            uint32_t end = norm_i + 2048u;
            for (uint32_t i = norm_i; i < end; i++) memory[cap_ring][i] = (int16_t)fclamp((float)memory[cap_ring][i] * cap_gain, -32767.f, 32767.f);
            norm_i = end;
            if (norm_i >= PM_BUFFER) cap_event = cap_ring;
        } else cap_event = PM_EV_SILENT;
    }
}
