/* SPDX-License-Identifier: MPL-2.0 */
/* PALINDROME 1.0's engine: a note lasts one ring; a capture of it loops clean; the rings play, still or moving; the
 * others resample into one; nothing to record leaves a ring empty; MOVE carries MORPH out and back. */
#include "palindrome.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static float l[256], r[256];
static double run(int blocks)
{
    double sq = 0;
    for (int b = 0; b < blocks; b++) {
        pm_render(l, r, 256);
        for (int i = 0; i < 256; i++) {
            assert(isfinite(l[i]) && isfinite(r[i]));
            assert(fabsf(l[i]) <= 1 && fabsf(r[i]) <= 1);
            sq += l[i] * l[i] + r[i] * r[i];
        }
    }
    return sqrt(sq / (blocks * 512.0));
}
#define NOTE_BLOCKS ((int)(PM_NOTE / 256u))
static int until_done(void)
{
    int ev, guard = 0;
    while ((ev = pm_service()) == -1) { run(1); assert(++guard < 2 * NOTE_BLOCKS); }
    return ev;
}
int main(void)
{
    pm_init();
    /* a note: it sounds, and is over after one ring's length */
    pm_note(48);
    double a = run(40);
    assert(a > .01 && pm_notes() == 1);
    run(NOTE_BLOCKS - 40 + 2);
    assert(pm_notes() == 0);
    assert(run(20) < 1e-3);
    printf("  ok   a note lasts one ring (%.3f rms)\n", a);

    /* MOVE: X goes toward its mirror and comes back */
    pm_set(M_X, .2f); pm_set(M_MOVE, 1.f); run(10);
    pm_note(48); run(NOTE_BLOCKS / 2);
    float mid = pm_morph_x();
    run(NOTE_BLOCKS / 2 - 8);
    float late = pm_morph_x();
    assert(mid > .7f && late < .35f);
    run(20);
    printf("  ok   MOVE: X .20 -> %.2f -> %.2f\n", mid, late);
    pm_set(M_MOVE, .45f);

    /* a capture of a note: a ring, its ends quiet, its body loud */
    assert(pm_capture(0, PM_SRC_MORPH) == 0);
    pm_note(45);
    assert(pm_capture(1, PM_SRC_MORPH) == -1);                 /* (one at a time) */
    assert(until_done() == 0 && pm_ring_ready(0) && pm_ring_count() == 1);
    {
        const int16_t *m = pm_ring_pcm(0);
        int peak = 0;
        for (unsigned i = 0; i < PM_BUFFER; i++) if (abs(m[i]) > peak) peak = abs(m[i]);
        assert(abs(m[0]) < 200 && abs(m[PM_BUFFER - 1]) < 200 && peak > 15000);
        printf("  ok   a note caught as ring 1 (peak %d, ends %d %d)\n", peak, m[0], m[PM_BUFFER - 1]);
    }
    run(NOTE_BLOCKS);

    /* playing: grains sound; SPEED still holds the head, as recorded moves it a ring a ring's length */
    pm_play(1);
    pm_rset(0, R_SPEED, pm_speed_value(4));
    run(20);
    float h0 = pm_head(0);
    a = run(40);
    assert(pm_active_grains() > 0 && a > .01 && fabsf(pm_head(0) - h0) < 1e-4);
    pm_rset(0, R_SPEED, pm_speed_value(7));
    run(4); h0 = pm_head(0);
    run(NOTE_BLOCKS / 4);
    float moved = pm_head(0) - h0;
    if (moved < 0) moved += 1;
    assert(moved > .2f && moved < .3f);
    printf("  ok   ring 1 plays (%.3f rms); still holds; as recorded moves %.2f in a quarter ring\n", a, moved);

    /* a slice, and the extremes stay in range */
    pm_jump(0, .5f); run(2);
    assert(fabsf(pm_head(0) - .5f) < .02f);
    for (int k = 0; k < PM_RPARAMS; k++) pm_rset(0, k, 1.f);
    run(200);
    for (int k = 0; k < PM_RPARAMS; k++) pm_rset(0, k, 0.f);
    run(200);
    pm_rset(0, R_SIZE, .55f); pm_rset(0, R_DENSITY, .7f); pm_rset(0, R_PITCH, .5f); pm_rset(0, R_SPEED, pm_speed_value(7));
    printf("  ok   slices, and every SCAN control at its ends\n");

    /* resampling: ring 1 into ring 2 */
    assert(pm_capture(1, PM_SRC_MIX) == 0);
    assert(until_done() == 1 && pm_ring_ready(1));
    printf("  ok   ring 1 resampled into ring 2\n");

    /* nothing heard: the ring stays empty */
    pm_play(0); run(200);
    assert(pm_capture(2, PM_SRC_MORPH) == 0);
    assert(until_done() == PM_EV_SILENT && !pm_ring_ready(2) && pm_ring_count() == 2);
    printf("  ok   nothing heard: ring 3 stays empty\n");

    /* a ring re-recorded while it plays: not read meanwhile, then back */
    pm_play(1);
    pm_note(50);
    assert(pm_capture(0, PM_SRC_MORPH) == 0);
    run(4);
    assert(!pm_ring_ready(0));
    assert(until_done() == 0 && pm_ring_ready(0));
    run(100);
    pm_panic();
    assert(run(50) < .01);
    puts("PALINDROME engine tests passed");
    return 0;
}
