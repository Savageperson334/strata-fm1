/* SPDX-License-Identifier: MPL-2.0
 * STRATA's engine on a computer: renders scenes to 16-bit WAV and checks them (no NaN, no clipping run-away,
 * the voices fade as asked, nothing jumps). Built and run by tests/run_tests.sh:
 *   cc -O2 -ffp-contract=off -DPLAT_HOST -Ibase/include -Iapp/dsp tests/dsp_render.c app/dsp/strata_dsp.c -lm
 *   ./dsp_render OUT_DIR */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "strata_dsp.h"

#define BLOCK 256u
static FILE *wav;
static uint32_t wav_frames;
static int fails;

static void wav_open(const char *path)
{
    static const uint8_t hdr[44] = {0};
    wav = fopen(path, "wb");
    if (!wav) { perror(path); exit(2); }
    fwrite(hdr, 1, 44, wav);
    wav_frames = 0;
}
static void put32(uint8_t *b, uint32_t v) { b[0] = v; b[1] = v >> 8; b[2] = v >> 16; b[3] = v >> 24; }
static void wav_close(void)
{
    uint8_t h[44];
    uint32_t bytes = wav_frames * 4u;
    memcpy(h, "RIFF", 4); put32(h + 4, 36u + bytes); memcpy(h + 8, "WAVEfmt ", 8);
    put32(h + 16, 16); h[20] = 1; h[21] = 0; h[22] = 2; h[23] = 0; put32(h + 24, 44100); put32(h + 28, 44100 * 4);
    h[32] = 4; h[33] = 0; h[34] = 16; h[35] = 0; memcpy(h + 36, "data", 4); put32(h + 40, bytes);
    fseek(wav, 0, SEEK_SET);
    fwrite(h, 1, 44, wav);
    fclose(wav);
    wav = 0;
}

typedef struct { double peak, sumsq, maxjump; long n, bad; float last_l; } stats_t;
static stats_t st;

static void render_s(double seconds)
{
    float l[BLOCK], r[BLOCK];
    long blocks = (long)(seconds * 44100.0 / BLOCK), b;
    uint32_t i;
    for (b = 0; b < blocks; b++) {
        sd_render(l, r, BLOCK);
        for (i = 0; i < BLOCK; i++) {
            float a = l[i], c = r[i];
            double j;
            if (!isfinite(a) || !isfinite(c)) { st.bad++; a = c = 0; }
            if (fabs(a) > st.peak) st.peak = fabs(a);
            if (fabs(c) > st.peak) st.peak = fabs(c);
            st.sumsq += (double)a * a + (double)c * c;
            j = fabs(a - st.last_l);
            if (j > st.maxjump) st.maxjump = j;
            st.last_l = a;
            st.n += 2;
            if (wav) {
                int16_t s[2] = {(int16_t)lrintf(a * 32000.0f), (int16_t)lrintf(c * 32000.0f)};
                fwrite(s, 2, 2, wav);
                wav_frames++;
            }
        }
    }
}
static void reset_stats(void) { memset(&st, 0, sizeof st); }
static double rms_db(void) { return st.n ? 10.0 * log10(st.sumsq / st.n + 1e-30) : -300.0; }

static void check(int ok, const char *what)
{
    printf("  %s %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) fails++;
}

static void scene(const char *dir, const char *name)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.wav", dir, name);
    wav_open(path);
    printf("%s\n", name);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    int i;
    clock_t t0;
    double secs;

    sd_init();

    /* 1. silence at boot */
    reset_stats();
    render_s(1.0);
    check(st.peak < 1e-6, "silent with no voices");

    /* 2. a fifth fades in over FADE, holds, fades out */
    scene(dir, "fifth");
    sd_param(P_FADE, 0.35f);             /* ~1.3 s */
    sd_voice_on(0, 41);                  /* F2 */
    sd_voice_on(1, 48);                  /* C3 */
    reset_stats();
    render_s(0.05);
    check(rms_db() < -30.0, "starts quiet (fades in)");
    reset_stats();
    render_s(4.0);
    printf("    held: peak %.3f, rms %.1f dB, max step %.4f\n", st.peak, rms_db(), st.maxjump);
    check(st.bad == 0, "no NaN or Inf");
    check(st.peak > 0.05 && st.peak < 1.0, "audible, not clipped hard");
    check(st.maxjump < 0.25, "no clicks while holding");
    sd_voice_off(0);
    sd_voice_off(1);
    render_s(3.0);
    check(!sd_voice_busy(0) && !sd_voice_busy(1), "voices free after fading out");
    sd_param(P_SPACE, 0.0f);             /* the tail, then nothing */
    render_s(6.0);
    reset_stats();
    render_s(0.5);
    check(st.peak < 0.01, "quiet after the tail");
    sd_param(P_SPACE, 0.45f);
    wav_close();

    /* 3. a six-voice chord with everything up: the worst case for level and CPU */
    scene(dir, "full");
    for (i = 0; i < SD_NPARAM; i++)
        sd_param(i, 0.9f);
    sd_param(P_FADE, 0.1f);
    {
        static const int chord[6] = {36, 43, 48, 52, 55, 62};
        for (i = 0; i < 6; i++)
            sd_voice_on(i, chord[i]);
    }
    reset_stats();
    t0 = clock();
    render_s(20.0);
    secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("    full: peak %.3f, rms %.1f dB; %.1f%% of real time on this computer\n", st.peak, rms_db(),
           100.0 * secs / 20.0);
    check(st.bad == 0, "no NaN or Inf with everything up");
    check(st.peak <= 1.0, "soft clip holds the peak");
    wav_close();

    /* 4. a voice changing note while it sounds dips and comes back, without a click */
    scene(dir, "retune");
    sd_panic();
    for (i = 0; i < SD_NPARAM; i++)
        sd_param(i, 0.3f);
    sd_param(P_FADE, 0.2f);
    sd_voice_on(0, 45);
    render_s(2.0);
    reset_stats();
    sd_voice_on(0, 52);
    render_s(1.0);
    printf("    retune: max step %.4f\n", st.maxjump);
    check(st.maxjump < 0.2, "a change of note does not click");
    wav_close();

    /* 5. sweeps of every control while a drone plays: no blow-ups */
    scene(dir, "sweeps");
    sd_voice_on(1, 40);
    sd_voice_on(2, 47);
    reset_stats();
    for (i = 0; i < SD_NPARAM; i++) {
        int step;
        for (step = 0; step <= 40; step++) {
            sd_param(i, (float)step / 40.0f);
            render_s(0.04);
        }
        sd_param(i, 0.4f);
    }
    printf("    sweeps: peak %.3f, max step %.4f\n", st.peak, st.maxjump);
    check(st.bad == 0, "no NaN or Inf through every sweep");
    check(st.maxjump < 0.5, "sweeps stay smooth");
    wav_close();

    /* 6. an hour of drone at the longest decay, resonance high: stable, no denormal slow-down, no drift to DC */
    printf("hour\n");
    sd_param(P_DECAY, 1.0f);
    sd_param(P_RESO, 0.95f);
    sd_param(P_SPACE, 1.0f);
    reset_stats();
    t0 = clock();
    render_s(3600.0);
    secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("    hour: peak %.3f, rms %.1f dB, %.1f s to render\n", st.peak, rms_db(), secs);
    check(st.bad == 0, "an hour without NaN");
    check(st.peak <= 1.0 && rms_db() > -40.0, "an hour later it still sounds");
    sd_panic();
    sd_param(P_SPACE, 0.0f);
    render_s(2.0);
    reset_stats();
    render_s(0.5);
    check(st.peak < 0.01, "panic silences");

    printf(fails ? "%d FAILED\n" : "all passed\n", fails);
    return fails != 0;
}
