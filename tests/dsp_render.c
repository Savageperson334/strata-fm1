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

    /* 6. the arpeggiator: three voices take turns; the level moves step to step, nothing clicks, even plucked */
    scene(dir, "arp");
    sd_panic();
    for (i = 0; i < SD_NPARAM; i++)
        sd_param(i, 0.35f);
    sd_param(P_FADE, 0.1f);
    sd_param(P_AFLOOR, 0.0f);
    sd_param(P_ARATE, 0.55f);            /* ~1.6 steps/s */
    sd_voice_on(0, 48);
    sd_voice_on(1, 55);
    sd_voice_on(2, 60);
    sd_arp(1);
    render_s(1.0);
    {
        int seen[SD_VOICES] = {0}, k, kinds = 0, b;
        float l[BLOCK], r[BLOCK];
        reset_stats();
        for (b = 0; b < (int)(6.0 * 44100.0 / BLOCK); b++) {
            int a = sd_arp_slot();
            if (a >= 0) seen[a] = 1;
            sd_render(l, r, BLOCK);
            for (k = 0; k < (int)BLOCK; k++) {
                double j = fabs(l[k] - st.last_l);
                if (j > st.maxjump) st.maxjump = j;
                st.last_l = l[k];
                if (wav) { int16_t s2[2] = {(int16_t)lrintf(l[k] * 32000.0f), (int16_t)lrintf(r[k] * 32000.0f)}; fwrite(s2, 2, 2, wav); wav_frames++; }
            }
        }
        for (k = 0; k < SD_VOICES; k++) kinds += seen[k];
        printf("    arp: %d voices took a step, max step %.4f\n", kinds, st.maxjump);
        check(kinds == 3, "every voice gets its step");
        check(st.maxjump < 0.25, "the arp does not click (smooth)");
    }
    sd_param(P_ASHAPE, 0.0f);            /* plucked */
    sd_param(P_AORDER, 0.9f);            /* random */
    reset_stats();
    render_s(4.0);
    printf("    arp plucked: max step %.4f\n", st.maxjump);
    check(st.bad == 0 && st.maxjump < 0.35, "the arp does not click (plucked, random)");
    sd_voice_off(1);
    sd_voice_off(2);
    reset_stats();
    render_s(3.0);
    check(st.peak > 0.01, "one voice left pulses on");
    sd_arp(0);
    render_s(1.0);
    check(sd_arp_slot() == -1, "arp off");
    wav_close();

    /* 7. the beat: steps at the tempo, every groove plays clean, the drone ducks, the arp follows the grid */
    scene(dir, "beat");
    sd_panic();
    for (i = 0; i < SD_NPARAM; i++)
        sd_param(i, 0.35f);
    sd_param(P_FADE, 0.1f);
    sd_param(P_BTEMPO, 0.6f);            /* 120 BPM: 8 sixteenths a second */
    sd_param(P_BSWING, 0.0f);
    sd_param(P_BGROOVE, 0.0f);
    sd_param(P_BDUCK, 0.8f);
    sd_param(P_BLEVEL, 0.7f);
    sd_voice_on(0, 45);
    sd_voice_on(1, 52);
    render_s(1.0);
    sd_beat(1);
    {
        float l[BLOCK], r[BLOCK];
        int b, k, last = -1, steps = 0, arp_changes = 0, arp_last = -2;
        reset_stats();
        for (b = 0; b < (int)(4.0 * 44100.0 / BLOCK); b++) {
            sd_render(l, r, BLOCK);
            if (sd_beat_step() != last) { steps++; last = sd_beat_step(); }
            for (k = 0; k < (int)BLOCK; k++) {
                if (fabs(l[k]) > st.peak) st.peak = fabs(l[k]);
                if (!isfinite(l[k])) st.bad++;
                if (wav) { int16_t s2[2] = {(int16_t)lrintf(l[k] * 32000.0f), (int16_t)lrintf(r[k] * 32000.0f)}; fwrite(s2, 2, 2, wav); wav_frames++; }
            }
        }
        printf("    beat: %d steps in 4 s at 120 BPM, peak %.3f\n", steps, st.peak);
        check(steps >= 30 && steps <= 34, "the beat keeps tempo (32 sixteenths in 4 s)");
        check(st.bad == 0 && st.peak <= 1.0, "drums and drone together stay in range");
        /* the arp on the grid: quarter notes at 120 BPM = 2 steps a second */
        sd_param(P_ARATE, 0.5f);         /* 1/4 */
        sd_arp(1);
        for (b = 0; b < (int)(4.0 * 44100.0 / BLOCK); b++) {
            sd_render(l, r, BLOCK);
            if (sd_arp_slot() != arp_last) { arp_changes++; arp_last = sd_arp_slot(); }
            for (k = 0; k < (int)BLOCK; k++)
                if (wav) { int16_t s2[2] = {(int16_t)lrintf(l[k] * 32000.0f), (int16_t)lrintf(r[k] * 32000.0f)}; fwrite(s2, 2, 2, wav); wav_frames++; }
        }
        printf("    arp on the beat: %d changes in 4 s (quarters: about 8)\n", arp_changes);
        check(arp_changes >= 7 && arp_changes <= 10, "the arp steps on the beat's quarter notes");
        sd_arp(0);
    }
    for (i = 0; i < SD_GROOVES; i++) {   /* every groove, swung, bright kit */
        sd_param(P_BGROOVE, ((float)i + 0.5f) / (float)SD_GROOVES);
        sd_param(P_BSWING, 0.7f);
        sd_param(P_BKIT, (float)i / 7.0f);
        reset_stats();
        render_s(2.0);
        if (st.bad || st.peak > 1.0) { printf("  FAIL groove %s\n", sd_groove_name(((float)i + 0.5f) / 8.0f)); fails++; }
    }
    check(1, "all eight grooves render in range");
    sd_beat(0);
    render_s(1.0);
    check(sd_beat_step() == -1, "beat off");
    wav_close();

    /* 8. the Euclidean lanes: the patterns themselves, then lanes as the drums, on their own lengths, and the arp */
    printf("euclid\n");
    {
        static const struct { int p, n; const char *want; } gold[] = {
            {3, 8, "X..X..X."}, {4, 16, "X...X...X...X..."}, {5, 12, "X..X.X..X.X."}, {5, 16, "X...X..X..X..X.."},
            {7, 16, "X..X.X.X..X.X.X."}, {3, 9, "X..X..X.."}, {0, 8, "........"}, {8, 8, "XXXXXXXX"}, {1, 1, "X"},
        };
        int n, pl, k, bad = 0;
        for (k = 0; k < (int)(sizeof gold / sizeof gold[0]); k++) {
            char got[40];
            uint32_t m = sd_euclid_mask(gold[k].n, gold[k].p, 0);
            for (n = 0; n < gold[k].n; n++) got[n] = (m >> n & 1u) ? 'X' : '.';
            got[n] = 0;
            if (strcmp(got, gold[k].want)) { printf("  FAIL E(%d,%d) = %s, wanted %s\n", gold[k].p, gold[k].n, got, gold[k].want); fails++; bad++; }
        }
        check(!bad, "the known patterns: E(3,8), E(4,16), E(5,12), E(5,16), E(7,16), E(3,9), none, all");
        bad = 0;
        for (n = 1; n <= 32; n++)        /* every length and count: the count exact, the gaps within a step of each other */
            for (pl = 0; pl <= n; pl++) {
                uint32_t m = sd_euclid_mask(n, pl, 0), r;
                int cnt = 0, first = -1, last = -1, gmin = 99, gmax = 0, rot;
                for (k = 0; k < n; k++)
                    if (m >> k & 1u) {
                        cnt++;
                        if (first < 0) first = k;
                        if (last >= 0) { int g2 = k - last; if (g2 < gmin) gmin = g2; if (g2 > gmax) gmax = g2; }
                        last = k;
                    }
                if (cnt > 1) { int g2 = first + n - last; if (g2 < gmin) gmin = g2; if (g2 > gmax) gmax = g2; }
                if (cnt != pl || (cnt > 1 && gmax - gmin > 1) || (pl && !(m & 1u)) || (n < 32 && m >> n)) bad++;
                for (rot = 0; rot <= n; rot++) {   /* a rotation moves it later, and wraps */
                    r = sd_euclid_mask(n, pl, rot);
                    for (k = 0; k < n; k++)
                        if ((m >> k & 1u) != (r >> ((k + rot) % n) & 1u)) bad++;
                }
            }
        check(!bad, "every E(p,n) to 32 steps: exact count, even gaps, a hit on step one, rotation wraps");
    }
    sd_panic();
    for (i = 0; i < SD_NPARAM; i++)
        sd_param(i, 0.35f);
    sd_param(P_BTEMPO, 0.6f);            /* 120 BPM: 8 sixteenths a second */
    sd_param(P_BSWING, 0.0f);
    sd_param(P_BLEVEL, 0.7f);
    sd_param(P_SPACE, 0.0f);
    sd_arp(0);
    sd_beat(0);
    render_s(3.0);
    scene(dir, "euclid");
    {
        sd_lane_t kick = {16, 4, 0, 100, 4, ET_KICK}, hat = {12, 5, 0, 100, 4, ET_HAT}, off = {16, 0, 0, 100, 4, ET_OFF};
        sd_lane_t arp = {16, 7, 0, 100, 4, ET_ARP};
        float l[BLOCK], r[BLOCK];
        int b, k, seen12 = 0, seen16 = 0, changes = 0, last = -2, p0, p1;
        sd_lane_set(0, &kick);
        sd_lane_set(1, &hat);
        sd_lane_set(2, &off);
        sd_lane_set(3, &off);
        reset_stats();
        render_s(1.0);
        check(st.peak < 1e-4 && !sd_clock_on(), "lanes set but switched off: silence, the clock stopped");
        sd_euclid(1);                    /* the beat off: the lanes alone are the drums */
        reset_stats();
        for (b = 0; b < (int)(4.0 * 44100.0 / BLOCK); b++) {
            sd_render(l, r, BLOCK);
            if (sd_lane_pos(0) == 15) seen16 = 1;
            if (sd_lane_pos(1) == 11) seen12 = 1;
            if (sd_lane_pos(1) > 11) fails++;
            for (k = 0; k < (int)BLOCK; k++) {
                if (fabs(l[k]) > st.peak) st.peak = fabs(l[k]);
                if (!isfinite(l[k])) st.bad++;
                if (wav) { int16_t s2[2] = {(int16_t)lrintf(l[k] * 32000.0f), (int16_t)lrintf(r[k] * 32000.0f)}; fwrite(s2, 2, 2, wav); wav_frames++; }
            }
        }
        printf("    lanes as drums, the beat off: peak %.3f\n", st.peak);
        check(sd_clock_on() && st.peak > 0.1 && st.bad == 0, "the lanes play the drums with the beat off");
        check(seen16 && seen12, "a 16-step and a 12-step lane each run their own length");
        /* 32 sixteenths in: the 16-step lane is back at its start, the 12-step lane is 8 steps into its third turn */
        p0 = sd_lane_pos(0);
        p1 = sd_lane_pos(1);
        printf("    after 4 s: lane 1 at step %d of 16, lane 2 at step %d of 12\n", p0 + 1, p1 + 1);
        check((p0 - p1 + 48) % 4 == 0 && p0 != p1, "the two lanes have drifted apart (polymeter)");
        sd_beat(1);                      /* the beat joins without restarting the clock */
        sd_render(l, r, BLOCK);
        check(sd_lane_pos(0) == p0 || sd_lane_pos(0) == (p0 + 1) % 16, "the beat joins the running clock, no restart");
        render_s(2.0);
        sd_beat(0);
        kick.prob = 0;                   /* no chance: no hits */
        hat.prob = 0;
        sd_lane_set(0, &kick);
        sd_lane_set(1, &hat);
        render_s(6.0);                   /* (the last kick's tail dies away) */
        reset_stats();
        render_s(2.0);
        check(st.peak < 1e-3 && sd_clock_on(), "chance 0: the lanes run and nothing plays");
        /* a lane steps the arp: E(7,16) at 120 BPM is 3.5 hits a second */
        sd_lane_set(0, &off);
        sd_lane_set(1, &off);
        sd_lane_set(2, &arp);
        sd_voice_on(0, 45);
        sd_voice_on(1, 52);
        sd_voice_on(2, 57);
        sd_arp(1);
        check(sd_arp_lane() == 2, "lane 3 steps the arp");
        render_s(1.0);
        for (b = 0; b < (int)(4.0 * 44100.0 / BLOCK); b++) {
            sd_render(l, r, BLOCK);
            if (sd_arp_slot() != last) { changes++; last = sd_arp_slot(); }
            for (k = 0; k < (int)BLOCK; k++)
                if (wav) { int16_t s2[2] = {(int16_t)lrintf(l[k] * 32000.0f), (int16_t)lrintf(r[k] * 32000.0f)}; fwrite(s2, 2, 2, wav); wav_frames++; }
        }
        printf("    arp on E(7,16): %d steps in 4 s (14 hits)\n", changes);
        check(changes >= 13 && changes <= 15, "the arp steps once on each of the lane's hits");
        sd_voice_off(1);
        sd_voice_off(2);
        render_s(1.0);
        changes = 0;
        last = -2;
        for (b = 0; b < (int)(4.0 * 44100.0 / BLOCK); b++) {
            sd_render(l, r, BLOCK);
            if (sd_arp_slot() != last) { changes++; last = sd_arp_slot(); }
        }
        printf("    one voice on E(7,16): %d on/off changes in 4 s (14 pulses)\n", changes);
        check(changes >= 26 && changes <= 30, "one voice alone pulses on each hit");
        sd_arp(0);
        sd_euclid(0);
        sd_lane_set(2, &off);
        render_s(0.5);
        check(!sd_clock_on() && sd_lane_pos(0) == -1, "lanes off: the clock stops");
        sd_voice_off(0);
    }
    wav_close();

    /* 9. an hour of drone at the longest decay, resonance high: stable, no denormal slow-down, no drift to DC */
    printf("hour\n");
    sd_voice_on(0, 45);
    sd_voice_on(1, 52);
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
