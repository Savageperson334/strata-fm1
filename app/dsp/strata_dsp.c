/* SPDX-License-Identifier: MPL-2.0
 * STRATA's sound engine: up to six drone voices, a resonant low-pass, a slow LFO and a long stereo reverb.
 *
 * Each voice:  two FM pairs (carrier + self-feedback modulator), detuned apart and panned apart,
 *              a sub sine an octave down, and white noise band-passed around the voice's pitch (AIR).
 * Then:        stereo TPT state-variable low-pass (FILTER, RESO), swept by the LFO (MOTION, DEPTH)
 *              a 4-line feedback-delay-network reverb with input diffusion and two modulated lines (SPACE, DECAY)
 *              a soft clip and a DC blocker.
 * Single-precision float throughout (the AC79's FPU); no double literals. */
#include "strata_dsp.h"
#include "fastmath.h"
#include "plat.h"                        /* PLAT_POOL only: big buffers go in the FM-1's RAM pool */
#define SD_POOL PLAT_POOL

/* ------------------------------------------------------------------ params --- */
static volatile float p_target[SD_NPARAM];
static float p[SD_NPARAM];               /* smoothed, once a block */

static const float P_DEFAULT[SD_NPARAM] = {
    0.35f, /* TONE */   0.10f, /* RATIO: 1.0 */ 0.25f, /* MOTION */ 0.45f, /* SPACE */
    0.55f, /* FILTER */ 0.30f, /* DRIFT */      0.08f, /* GRIT */   0.40f, /* DEPTH */
    0.15f, /* BEAT */   0.45f, /* FADE */       0.60f, /* DECAY */  0.20f, /* RESO */
    0.10f, /* AIR */    0.25f, /* SUB */
};

static float clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

void sd_param(int id, float v)
{
    if (id >= 0 && id < SD_NPARAM)
        p_target[id] = clamp01(v);
}
float sd_param_get(int id) { return id >= 0 && id < SD_NPARAM ? p_target[id] : 0.0f; }

/* readable curves */
static const float RATIOS[] = {0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 5.0f, 6.0f, 7.0f};
#define NRATIOS 11
float sd_ratio_of(float v)
{
    /* the whole and half ratios hold for a stretch of the knob; between them the ratio slides (bells, clangs) */
    float x = clamp01(v) * (float)(NRATIOS - 1), f;
    int i = (int)x;
    if (i >= NRATIOS - 1)
        return RATIOS[NRATIOS - 1];
    f = x - (float)i;
    f = f < 0.3f ? 0.0f : f > 0.7f ? 1.0f : (f - 0.3f) * 2.5f;
    return RATIOS[i] + (RATIOS[i + 1] - RATIOS[i]) * f;
}
float sd_motion_hz(float v) { return 0.01f * fm_exp2f(clamp01(v) * 8.2288f); }      /* 0.01 .. 3 Hz */
float sd_cutoff_hz(float v) { return 40.0f * fm_exp2f(clamp01(v) * 8.4525f); }      /* 40 Hz .. 14 kHz */
float sd_fade_s(float v) { return 0.1f * fm_exp2f(clamp01(v) * 7.6439f); }          /* 0.1 .. 20 s */
float sd_decay_s(float v) { return 1.5f * fm_exp2f(clamp01(v) * 5.3219f); }         /* 1.5 .. 60 s */
float sd_beat_hz(float v) { float x = clamp01(v); return 3.0f * x * x; }            /* 0 .. 3 Hz */

/* ------------------------------------------------------------------- noise --- */
static uint32_t rng = 0x2545F491u;
static inline float rnd(void)            /* -1..1 */
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (float)(int32_t)rng * (1.0f / 2147483648.0f);
}

/* ------------------------------------------------------------------ voices --- */
typedef struct {
    volatile int req_note;               /* the main loop's ask: note and gate */
    volatile int req_gate;
    int note;                            /* sounding */
    float env;                           /* 0..1 */
    float pcA, pmA, pcB, pmB, ps;        /* phases, turns */
    float yA1, yA2, yB1, yB2;            /* the modulators' last outputs (feedback) */
    float wand[3], wand_t[3];            /* slow random wander: pitch A, pitch B, level */
    float bp1, bp2;                      /* AIR's band-pass state */
    float idx_now;                       /* the FM index at the end of the last block (ramped) */
    float level;                         /* for the screen */
} voice_t;
static voice_t V[SD_VOICES];

void sd_voice_on(int slot, int note)
{
    if (slot < 0 || slot >= SD_VOICES)
        return;
    V[slot].req_note = note;
    V[slot].req_gate = 1;
}
void sd_voice_off(int slot)
{
    if (slot >= 0 && slot < SD_VOICES)
        V[slot].req_gate = 0;
}
int sd_voice_busy(int slot)
{
    return slot >= 0 && slot < SD_VOICES && (V[slot].req_gate || V[slot].env > 0.0005f);
}
float sd_voice_level(int slot) { return slot >= 0 && slot < SD_VOICES ? V[slot].level : 0.0f; }

/* ------------------------------------------------------------------ filter --- */
typedef struct { float ic1, ic2; } svf_t;
static svf_t flt[2];

/* ------------------------------------------------------------------ reverb --- */
#define RV_LEN 4096u                     /* each line's buffer (a power of two) */
#define RV_MASK (RV_LEN - 1u)
static SD_POOL float rv_buf[4][RV_LEN];
static const uint32_t RV_DLY[4] = {1777u, 2137u, 2593u, 3083u};
static float rv_lp[4];
static uint32_t rv_w;
static float rv_mod_ph;
#define AP_LEN 512u
#define AP_MASK (AP_LEN - 1u)
static SD_POOL float ap_buf[4][AP_LEN];
static const uint32_t AP_DLY[4] = {142u, 379u, 107u, 277u};   /* L: 0, 1; R: 2, 3 */
static uint32_t ap_w;

static inline float allpass(int i, float x)
{
    float d = ap_buf[i][(ap_w - AP_DLY[i]) & AP_MASK];
    float y = d - 0.6f * x;
    ap_buf[i][ap_w & AP_MASK] = x + 0.6f * y;
    return y;
}

static inline float rv_read_frac(int i, float dly)   /* dly > 1: a delay of a fraction of a sample, interpolated */
{
    uint32_t di = (uint32_t)dly;         /* (integer arithmetic on the write index: exact for hours of drone) */
    float f = dly - (float)di;
    float a = rv_buf[i][(rv_w - di) & RV_MASK], b = rv_buf[i][(rv_w - di - 1u) & RV_MASK];
    return a + (b - a) * f;
}

/* -------------------------------------------------------------- the rest --- */
static float lfo_ph;
static float lfo_val;
static float dc_x[2], dc_y[2];
#define SCOPE_N 256u
static float scope_ring[SCOPE_N];
static uint32_t scope_w, scope_dec;
static float denorm = 1e-18f;
static float norm_now = 1.0f;            /* the voices' level, eased down as more of them sound */

void sd_init(void)
{
    int i;
    for (i = 0; i < SD_NPARAM; i++)
        p_target[i] = p[i] = P_DEFAULT[i];
    for (i = 0; i < SD_VOICES; i++) {
        voice_t *v = &V[i];
        v->req_note = v->note = 48;
        v->req_gate = 0;
        v->env = 0.0f;
        v->pcA = 0.0f;
        v->pcB = 0.37f;                  /* the pairs start apart: no phasey attack */
        v->pmA = 0.11f;
        v->pmB = 0.63f;
        v->idx_now = 0.0f;
    }
}

void sd_panic(void)
{
    int i;
    for (i = 0; i < SD_VOICES; i++) {
        V[i].req_gate = 0;
        V[i].env = 0.0f;
        V[i].level = 0.0f;
    }
}

float sd_lfo(void) { return lfo_val; }

void sd_scope(float *dst)
{
    /* 64 points from the latest quarter-rate output, starting where it rises through zero: a steady picture */
    uint32_t start = (scope_w - 64u - 120u) & (SCOPE_N - 1u), k, i;
    for (k = 0; k < 120u; k++) {
        uint32_t a = (start + k) & (SCOPE_N - 1u), b = (a + 1u) & (SCOPE_N - 1u);
        if (scope_ring[a] <= 0.0f && scope_ring[b] > 0.0f) {
            start = b;
            break;
        }
    }
    for (i = 0; i < 64u; i++)
        dst[i] = scope_ring[(start + i) & (SCOPE_N - 1u)];
}

static inline float mtof(float n) { return 440.0f * fm_exp2f((n - 69.0f) * (1.0f / 12.0f)); }

/* a soft clip: linear to about 0.5, flat at 1 */
static inline float soft(float x)
{
    if (x > 1.5f)
        return 1.0f;
    if (x < -1.5f)
        return -1.0f;
    return x - x * x * x * (1.0f / 6.75f);
}

void sd_render(float *outl, float *outr, uint32_t n)
{
    uint32_t i;
    int s;
    float inv_n = 1.0f / (float)n;
    float ratio, tone_i, depth, fb, beat, fade_c, drift, sub_g, air_g;
    float fc, g, k, a1, a2, a3;
    float rt60, rv_g[4], damp, wet, dry, mod_d0, mod_d1, rv_in, norm0, dnorm, sum_e2 = 0.0f;

    /* the controls, smoothed once a block (~50 ms) */
    for (s = 0; s < SD_NPARAM; s++)
        p[s] += (p_target[s] - p[s]) * 0.12f;

    ratio = sd_ratio_of(p[P_RATIO]);
    tone_i = 6.0f * p[P_TONE] * p[P_TONE] * (1.0f / 6.2831853f);   /* index, in turns */
    depth = p[P_DEPTH];
    fb = 0.32f * p[P_GRIT];
    beat = sd_beat_hz(p[P_BEAT]);
    drift = p[P_DRIFT];
    sub_g = p[P_SUB] * 0.55f;
    air_g = p[P_AIR] * p[P_AIR] * 0.9f;
    {
        float tau = sd_fade_s(p[P_FADE]) * 0.25f;       /* about 98% of the way in FADE seconds */
        fade_c = 1.0f - fm_expf(-1.0f / (tau * SD_RATE));
    }

    /* the LFO, once a block */
    lfo_ph += sd_motion_hz(p[P_MOTION]) * (float)n * (1.0f / SD_RATE);
    if (lfo_ph >= 1.0f)
        lfo_ph -= 1.0f;
    lfo_val = fm_sin_turns(lfo_ph);

    /* the filter's coefficients, swept by the LFO up to 1.5 octaves */
    fc = sd_cutoff_hz(p[P_FILTER]) * fm_exp2f(depth * 1.5f * lfo_val);
    if (fc > 18000.0f)
        fc = 18000.0f;
    g = fm_tanf(3.14159265f * fc * (1.0f / SD_RATE));
    k = 2.0f - 1.88f * p[P_RESO];
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;

    /* the reverb's feedback per line from DECAY (RT60) */
    rt60 = sd_decay_s(p[P_DECAY]);
    for (s = 0; s < 4; s++)
        rv_g[s] = fm_exp2f(-9.965784f * (float)RV_DLY[s] / (rt60 * SD_RATE));   /* 10^(-3 d / (rt60 sr)) */
    damp = 0.62f - 0.3f * p[P_DECAY];    /* longer tails are darker */
    /* a long tail builds up: feed it less, so long and short decays sit at about the same level */
    rv_in = 1.4f * fm_sqrtf(1.0f - 0.25f * (rv_g[0] * rv_g[0] + rv_g[1] * rv_g[1] + rv_g[2] * rv_g[2] + rv_g[3] * rv_g[3]));
    wet = p[P_SPACE] * 1.1f;
    dry = 1.0f - 0.55f * p[P_SPACE];

    /* clear the block: voices add into it */
    for (i = 0; i < n; i++)
        outl[i] = outr[i] = 0.0f;

    for (s = 0; s < SD_VOICES; s++) {
        voice_t *v = &V[s];
        int gate = v->req_gate, want = v->req_note;
        float tgt, c, f, detA, detB, incA, incB, incmA, incmB, incS, lfo_v, idx0, idx1, didx;
        float panA_l, panA_r, panB_l, panB_r, lvl, bpg, bpk, bpa1, bpa2, bpa3, peak = 0.0f;
        int w;

        if (!gate && v->env < 0.00002f) {           /* silent: nothing to do */
            v->env = 0.0f;
            v->level = 0.0f;
            continue;
        }
        /* a new note on a sounding voice: dip fast, then change */
        if (gate && want != v->note) {
            if (v->env < 0.002f)
                v->note = want;
            else
                gate = 0;
        }
        tgt = gate ? 1.0f : 0.0f;
        c = (v->req_gate && want != v->note) ? 0.0015f : fade_c;   /* ~15 ms dip for a change of note */

        /* the slow wander: new targets now and then, glided to */
        for (w = 0; w < 3; w++) {
            if ((rng & 63u) == (uint32_t)(w * 7 + s))
                v->wand_t[w] = rnd();
            v->wand[w] += (v->wand_t[w] - v->wand[w]) * 0.004f;
            (void)rnd();
        }
        f = mtof((float)v->note);
        detA = fm_exp2f((-drift * 14.0f + v->wand[0] * drift * 9.0f) * (1.0f / 1200.0f));
        detB = fm_exp2f((drift * 14.0f + v->wand[1] * drift * 9.0f) * (1.0f / 1200.0f));
        incA = f * detA * (1.0f / SD_RATE);
        incB = f * detB * (1.0f / SD_RATE);
        incmA = (f * detA * ratio + beat) * (1.0f / SD_RATE);
        incmB = (f * detB * ratio - beat) * (1.0f / SD_RATE);
        incS = f * 0.5f * (1.0f / SD_RATE);

        /* each voice breathes at its own point of the LFO */
        lfo_v = fm_sin_turns(lfo_ph + (float)s * 0.1667f);
        idx1 = tone_i * (1.0f + depth * 0.9f * lfo_v);
        if (idx1 < 0.0f)
            idx1 = 0.0f;
        idx1 *= 1.0f / (1.0f + f * (1.0f / 2500.0f));   /* high notes less bright: no fizz */
        idx0 = v->idx_now;
        didx = (idx1 - idx0) * inv_n;
        v->idx_now = idx1;

        /* the pairs panned apart, more with DRIFT; voices spread across the field */
        {
            float spread = 0.25f + 0.45f * drift, centre = ((float)s - 2.5f) * 0.06f;
            float pa = clamp01(0.5f - spread * 0.5f + centre), pb = clamp01(0.5f + spread * 0.5f + centre);
            panA_l = 1.0f - pa; panA_r = pa;
            panB_l = 1.0f - pb; panB_r = pb;
        }
        lvl = 0.21f * (1.0f + v->wand[2] * drift * 0.35f);

        /* AIR's band-pass around three times the pitch */
        {
            float bc = f * 3.0f;
            if (bc < 200.0f) bc = 200.0f;
            if (bc > 9000.0f) bc = 9000.0f;
            bpg = fm_tanf(3.14159265f * bc * (1.0f / SD_RATE));
            bpk = 0.35f;
            bpa1 = 1.0f / (1.0f + bpg * (bpg + bpk));
            bpa2 = bpg * bpa1;
            bpa3 = bpg * bpa2;
        }

        for (i = 0; i < n; i++) {
            float mA, mB, cA, cB, idx, e, sub, out_l, out_r;
            v->env += (tgt - v->env) * c;
            e = v->env * v->env * lvl;                   /* squared: fades sound even */
            idx = idx0 + didx * (float)i;
            mA = fm_sin_turns(v->pmA + fb * (v->yA1 + v->yA2));
            v->yA2 = v->yA1;
            v->yA1 = mA;
            mB = fm_sin_turns(v->pmB + fb * (v->yB1 + v->yB2));
            v->yB2 = v->yB1;
            v->yB1 = mB;
            cA = fm_sin_turns(v->pcA + idx * mA);
            cB = fm_sin_turns(v->pcB + idx * mB);
            out_l = cA * panA_l + cB * panB_l;
            out_r = cA * panA_r + cB * panB_r;
            if (sub_g > 0.001f) {
                sub = fm_sin_turns(v->ps) * sub_g;
                v->ps += incS;
                if (v->ps >= 1.0f) v->ps -= 1.0f;
                out_l += sub;
                out_r += sub;
            }
            if (air_g > 0.0005f) {                       /* TPT band-pass on white noise */
                float x = rnd(), v3, v1, v2, bp;
                v3 = x - v->bp2;
                v1 = bpa1 * v->bp1 + bpa2 * v3;
                v2 = v->bp2 + bpa2 * v->bp1 + bpa3 * v3;
                v->bp1 = 2.0f * v1 - v->bp1;
                v->bp2 = 2.0f * v2 - v->bp2;
                bp = v1 * bpk * air_g * 2.2f;
                out_l += bp;
                out_r += bp;
            }
            outl[i] += out_l * e;
            outr[i] += out_r * e;
            v->pcA += incA; if (v->pcA >= 1.0f) v->pcA -= 1.0f;
            v->pcB += incB; if (v->pcB >= 1.0f) v->pcB -= 1.0f;
            v->pmA += incmA; if (v->pmA >= 1.0f) v->pmA -= 1.0f; else if (v->pmA < 0.0f) v->pmA += 1.0f;
            v->pmB += incmB; if (v->pmB >= 1.0f) v->pmB -= 1.0f; else if (v->pmB < 0.0f) v->pmB += 1.0f;
        }
        v->bp1 = fm_flush(v->bp1);
        v->bp2 = fm_flush(v->bp2);
        peak = v->env;
        v->level = peak;
        sum_e2 += peak * peak;
    }

    /* more voices, each a little quieter: two sound at full level, six at about 0.58 */
    {
        float want = sum_e2 > 2.0f ? 1.0f / fm_sqrtf(sum_e2 * 0.5f) : 1.0f;
        norm0 = norm_now;
        norm_now += (want - norm_now) * 0.05f;
        dnorm = (norm_now - norm0) * inv_n;
    }

    /* the filter, the reverb, the finish */
    mod_d0 = 7.0f * fm_sin_turns(rv_mod_ph);
    mod_d1 = 7.0f * fm_sin_turns(rv_mod_ph + 0.31f);
    rv_mod_ph += 0.23f * (float)n * (1.0f / SD_RATE);
    if (rv_mod_ph >= 1.0f)
        rv_mod_ph -= 1.0f;
    for (i = 0; i < n; i++) {
        float x[2], y[2], d[4], h[4], il, ir;
        int c;
        float nm = norm0 + dnorm * (float)i;
        x[0] = outl[i] * nm;
        x[1] = outr[i] * nm;
        for (c = 0; c < 2; c++) {                        /* TPT SVF low-pass */
            svf_t *q = &flt[c];
            float v3 = x[c] - q->ic2;
            float v1 = a1 * q->ic1 + a2 * v3;
            float v2 = q->ic2 + a2 * q->ic1 + a3 * v3;
            q->ic1 = 2.0f * v1 - q->ic1;
            q->ic2 = 2.0f * v2 - q->ic2;
            y[c] = v2;
        }
        /* reverb: diffuse each side, then into the network */
        denorm = -denorm;
        il = allpass(1, allpass(0, y[0] * rv_in + denorm));
        ir = allpass(3, allpass(2, y[1] * rv_in + denorm));
        ap_w++;
        d[0] = rv_read_frac(0, (float)RV_DLY[0] + mod_d0);
        d[1] = rv_buf[1][(rv_w - RV_DLY[1]) & RV_MASK];
        d[2] = rv_read_frac(2, (float)RV_DLY[2] + mod_d1);
        d[3] = rv_buf[3][(rv_w - RV_DLY[3]) & RV_MASK];
        for (c = 0; c < 4; c++) {                        /* damping in the loop */
            rv_lp[c] += (d[c] - rv_lp[c]) * damp;
            d[c] = rv_lp[c] * rv_g[c];
        }
        /* Hadamard: the lines feed each other evenly */
        h[0] = 0.5f * (d[0] + d[1] + d[2] + d[3]);
        h[1] = 0.5f * (d[0] - d[1] + d[2] - d[3]);
        h[2] = 0.5f * (d[0] + d[1] - d[2] - d[3]);
        h[3] = 0.5f * (d[0] - d[1] - d[2] + d[3]);
        rv_buf[0][rv_w & RV_MASK] = h[0] + il * 0.5f;
        rv_buf[1][rv_w & RV_MASK] = h[1] + ir * 0.5f;
        rv_buf[2][rv_w & RV_MASK] = h[2] + il * 0.5f;
        rv_buf[3][rv_w & RV_MASK] = h[3] + ir * 0.5f;
        rv_w++;
        y[0] = y[0] * dry + (rv_lp[0] + rv_lp[2]) * 0.5f * wet;
        y[1] = y[1] * dry + (rv_lp[1] + rv_lp[3]) * 0.5f * wet;
        for (c = 0; c < 2; c++) {                        /* DC blocker, soft clip */
            float o = y[c] - dc_x[c] + 0.9995f * dc_y[c];
            dc_x[c] = y[c];
            dc_y[c] = o;
            y[c] = soft(o);
        }
        outl[i] = y[0];
        outr[i] = y[1];
        if (++scope_dec >= 16u) {         /* 64 points: about 23 ms, two cycles of a low drone */
            scope_dec = 0;
            scope_ring[scope_w++ & (SCOPE_N - 1u)] = 0.5f * (y[0] + y[1]);
        }
    }
    for (s = 0; s < 4; s++)
        rv_lp[s] = fm_flush(rv_lp[s]);
    for (s = 0; s < 2; s++) {
        flt[s].ic1 = fm_flush(flt[s].ic1);
        flt[s].ic2 = fm_flush(flt[s].ic2);
        dc_y[s] = fm_flush(dc_y[s]);
    }
}
