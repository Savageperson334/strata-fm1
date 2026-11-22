/* SPDX-License-Identifier: MPL-2.0
 * STRATA's app logic on a computer, without fm1-base: a stand-in platform (keys, buttons, encoders, a store, MIDI,
 * a screen that draws nothing) drives app.c and checks what it does. fm1-base's own simulator runs the real thing in
 * CI (tests/run_tests.sh); this one needs only a C compiler:
 *   cc -std=gnu99 -DPLAT_HOST -Iapp -Iapp/dsp -Ibase/include tests/app_logic.c app/dsp/strata_dsp.c -lm */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"

/* ---- the stand-in platform ---- */
typedef struct { uint8_t h; } felucca_font_t;
const felucca_font_t FONT_XS, FONT_B;
#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define C_BLACK 0x0000u
#define C_WHITE 0xFFFFu
static uint16_t pal[5];
#define C_LINE pal[0]
#define C_DIM pal[1]
#define C_GRAY pal[2]
#define C_AMB pal[3]
#define C_HI pal[4]
static uint32_t draws;
static void cv_begin(uint32_t w, uint32_t h, uint16_t bg) { (void)w; (void)h; (void)bg; draws++; }
static void cv_blit(uint32_t x, uint32_t y) { (void)x; (void)y; }
static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
static void cv_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c) { (void)x0; (void)y0; (void)x1; (void)y1; (void)c; }
static char last_text[64];
static int32_t cv_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    (void)y; (void)f; (void)c;
    snprintf(last_text, sizeof last_text, "%s", s);
    return x + 7 * (int32_t)strlen(s);
}
static int32_t text_w(const felucca_font_t *f, const char *s) { (void)f; return 7 * (int32_t)strlen(s); }
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
#define ANN_NOW 1u
static int ann_on(void) { return 0; }
static int ann_where(void) { return 0; }
static void ann_say(const char *a, const char *b, uint32_t f) { (void)a; (void)b; (void)f; }
static void ann_tick(uint32_t now) { (void)now; }

static uint32_t now_ms = 1000, keys_now, btn_now, lights_btn, lights_keys;
static int32_t enc_pending[NE];
static uint8_t store[256];
static int store_len = -1, store_writes;
static uint32_t min_q[64], min_w, min_r, midi_out_n;
static uint32_t master_now = 2048;
uint32_t plat_master(void) { return master_now; }
uint32_t plat_ms(void) { return now_ms; }
uint32_t plat_keys(void) { return keys_now; }
uint32_t plat_buttons(void) { return btn_now; }
int32_t plat_enc(int role) { int32_t e = enc_pending[role]; enc_pending[role] = 0; return e; }
int plat_store_load(uint32_t obj, void *dst, uint32_t max)
{
    if (obj != OBJ_SETTINGS || store_len < 0 || (uint32_t)store_len > max) return -1;
    memcpy(dst, store, (size_t)store_len);
    return store_len;
}
int plat_store_save(uint32_t obj, const void *src, uint32_t len)
{
    if (obj != OBJ_SETTINGS || len > sizeof store) return -1;
    memcpy(store, src, len);
    store_len = (int)len;
    store_writes++;
    return 0;
}
int plat_midi_in(uint32_t *pkt, int *port)
{
    if (min_r == min_w) return 0;
    *pkt = min_q[min_r++ & 63u];
    *port = PORT_USB;
    return 1;
}
void plat_midi_send(uint32_t pkt, uint32_t to) { (void)pkt; (void)to; midi_out_n++; }
void plat_lights(uint32_t b, uint32_t k, uint32_t db, uint32_t dk, int glow) { (void)db; (void)dk; (void)glow; lights_btn = b; lights_keys = k; }

#include "app.c"

/* ---- the checks ---- */
static int fails;
static void expect(const char *what, int want)
{
    int got = -999;
    app_expect(what, &got);
    if (got != want) {
        printf("  FAIL %s: %d, wanted %d\n", what, got, want);
        fails++;
    } else {
        printf("  ok   %s = %d\n", what, want);
    }
}
static void run_ms(uint32_t ms)          /* frames at 60 Hz, audio between them */
{
    int32_t lr[2 * APP_BLOCK];
    uint32_t t;
    for (t = 0; t < ms; t += 16) {
        int b;
        for (b = 0; b < 3; b++)
            app_render(lr, APP_BLOCK, 4096);
        now_ms += 16;
        app_frame();
    }
}
static void key_tap(int k) { keys_now |= 1u << k; run_ms(32); keys_now &= ~(1u << k); run_ms(32); }
static void btn_tap(int b) { btn_now |= 1u << b; run_ms(32); btn_now &= ~(1u << b); run_ms(32); }
static void midi(uint32_t st, uint32_t d1, uint32_t d2) { min_q[min_w++ & 63u] = (st >> 4) | st << 8 | d1 << 16 | d2 << 24; }

int main(void)
{
    app_init();
    app_ui_init();
    run_ms(100);
    printf("boot\n");
    expect("voices", 0);
    expect("octave", -1);
    expect("dirty", 0);
    expect("p0", 35);

    printf("keys latch\n");
    key_tap(0);                          /* F2 at octave -1 */
    expect("voices", 1);
    key_tap(7);                          /* C3 */
    expect("voices", 2);
    if (!(lights_keys & 1u) || !(lights_keys & (1u << 7))) { printf("  FAIL key lights\n"); fails++; }
    else printf("  ok   the sounding keys are lit\n");
    if (!(lights_btn & LED_PLAY_GREEN)) { printf("  FAIL PLAY green\n"); fails++; } else printf("  ok   PLAY is green\n");
    key_tap(0);                          /* F2 again: out */
    expect("voices", 1);

    printf("PLAY out and back\n");
    key_tap(4);
    expect("voices", 2);
    btn_tap(B_PLAY);
    expect("voices", 0);
    expect("nchord", 2);
    btn_tap(B_PLAY);
    expect("voices", 2);

    printf("seven voices: the oldest gives way\n");
    {
        int k;
        for (k = 10; k < 16; k++)
            key_tap(k);
    }
    expect("voices", 6);

    printf("encoders and the SEL layer\n");
    {
        int i;
        for (i = 0; i < 4; i++) {        /* four slow detents: 1/64 each */
            enc_pending[EN_K1] = 1;
            run_ms(100);
        }
    }
    expect("p0", 41);                    /* 0.35 + 4/64 */
    enc_pending[EN_K1] = 3;              /* a fast spin: three times as far */
    run_ms(100);
    expect("p0", 55);
    enc_pending[EN_K1] = -3;
    run_ms(100);
    expect("p0", 41);
    expect("dirty", 1);
    btn_now |= 1u << B_SEL;
    run_ms(50);
    expect("layer", 1);
    enc_pending[EN_K1] = -1;
    run_ms(100);
    enc_pending[EN_K1] = -1;
    run_ms(100);
    expect("p7", 37);                    /* DEPTH 0.40 - 2/64 */
    expect("p0", 41);
    btn_now &= ~(1u << B_SEL);
    run_ms(50);
    expect("layer", 0);
    enc_pending[EN_PRESET] = -100;       /* past the end: held at 0 */
    run_ms(100);
    expect("p6", 0);

    printf("octave\n");
    btn_tap(B_OCTUP);
    btn_tap(B_OCTUP);
    btn_tap(B_OCTUP);
    btn_tap(B_OCTUP);
    expect("octave", 2);

    printf("SAVE, a power cycle, PLAY brings the chord back\n");
    btn_tap(B_SAVE);
    expect("dirty", 0);
    if (store_writes != 1) { printf("  FAIL store writes %d\n", store_writes); fails++; } else printf("  ok   saved once\n");
    app_panic();
    memset(slot, 0, sizeof slot);
    memset(&set, 0, sizeof set);
    app_init();
    run_ms(100);
    expect("voices", 0);
    expect("octave", 2);
    expect("p0", 41);
    expect("nchord", 6);
    btn_tap(B_PLAY);
    expect("voices", 6);
    btn_tap(B_PLAY);

    printf("HOME held resets the sound, a tap does not\n");
    btn_tap(B_HOME);
    expect("p0", 41);
    btn_now |= 1u << B_HOME;
    run_ms(1100);
    btn_now &= ~(1u << B_HOME);
    run_ms(50);
    expect("p0", 35);

    printf("REC mutates the main controls only\n");
    {
        int before = 0, after = 0, p;
        for (p = 0; p < 7; p++) before += (int)(set.param[p] * 1000);
        btn_tap(B_REC);
        for (p = 0; p < 7; p++) after += (int)(set.param[p] * 1000);
        if (before == after) { printf("  FAIL mutate changed nothing\n"); fails++; } else printf("  ok   mutate moved the sound\n");
        expect("p7", 40);
    }

    printf("MIDI\n");
    run_ms(4000);
    midi(0x90, 60, 100);
    midi(0x90, 67, 100);
    run_ms(50);
    expect("voices", 2);
    midi(0x80, 60, 0);
    run_ms(50);
    expect("voices", 1);
    midi(0xB0, 64, 127);                 /* sustain */
    midi(0x80, 67, 0);
    run_ms(50);
    expect("voices", 1);
    midi(0xB0, 64, 0);
    run_ms(50);
    expect("voices", 0);
    midi(0xB0, 20, 127);                 /* CC 20: TONE */
    run_ms(50);
    expect("p0", 100);

    printf("ARP: a tap switches it, a hold and a turn sets it\n");
    int filter_before = (int)(set.param[P_FILTER] * 100.0f + 0.5f);
    expect("arp", 0);
    btn_tap(B_ARP);
    expect("arp", 1);
    if (!(lights_btn & (1u << B_ARP))) { printf("  FAIL ARP light\n"); fails++; } else printf("  ok   ARP is lit\n");
    btn_now |= 1u << B_ARP;
    run_ms(50);
    expect("layer", 2);
    enc_pending[EN_K1] = 1;              /* RATE */
    run_ms(100);
    enc_pending[EN_SELECT] = 1;          /* nothing on this layer */
    run_ms(100);
    btn_now &= ~(1u << B_ARP);
    run_ms(50);
    expect("arp", 1);                    /* a turn while held: no switch */
    expect("layer", 0);
    expect("p14", 47);                   /* 0.45 + 1/64 */
    expect("p4", filter_before);         /* FILTER untouched: SELECT does nothing on the arp's layer */
    {
        int k, steps = 0, last = -2;
        for (k = 0; k < 4; k++) key_tap(k * 4);
        for (k = 0; k < 200; k++) {      /* the step moves among the voices */
            run_ms(32);
            if (sd_arp_slot() != last) { steps++; last = sd_arp_slot(); }
        }
        if (steps < 4) { printf("  FAIL arp steps %d\n", steps); fails++; } else printf("  ok   the arp steps (%d changes)\n", steps);
    }
    btn_tap(B_SAVE);
    app_panic();
    memset(&set, 0, sizeof set);
    app_init();
    run_ms(50);
    expect("arp", 1);
    expect("p14", 47);
    btn_tap(B_ARP);
    expect("arp", 0);

    printf("SEQ: a tap starts the beat, a hold and a turn sets it\n");
    expect("beat", 0);
    btn_tap(B_SEQ);
    expect("beat", 1);
    {
        int k, last = -9, changes = 0;
        for (k = 0; k < 60; k++) { run_ms(32); if (sd_beat_step() != last) { changes++; last = sd_beat_step(); } }
        if (changes < 8) { printf("  FAIL beat steps %d\n", changes); fails++; } else printf("  ok   the beat steps (%d)\n", changes);
    }
    btn_now |= 1u << B_SEQ;
    run_ms(50);
    expect("layer", 3);
    enc_pending[EN_K1] = 1;              /* TEMPO */
    run_ms(100);
    enc_pending[EN_PRESET] = 1;          /* DUCK */
    run_ms(100);
    btn_now &= ~(1u << B_SEQ);
    run_ms(50);
    expect("beat", 1);
    expect("layer", 0);
    expect("p18", 32);                   /* TEMPO 0.30 + 1/64 */
    expect("p24", 37);                   /* DUCK 0.35 + 1/64 */
    btn_tap(B_SAVE);
    memset(&set, 0, sizeof set);
    app_init();
    run_ms(50);
    expect("beat", 1);
    expect("p18", 32);
    btn_tap(B_SEQ);
    expect("beat", 0);

    printf("EDIT: a tap switches the lanes, a hold and a turn sets the picked lane\n");
    expect("euc", 0);
    expect("L0s", 16);
    expect("L0p", 4);
    expect("L0t", ET_KICK);
    expect("L1s", 12);
    btn_tap(B_EDIT);
    expect("euc", 1);
    if (!(lights_btn & (1u << B_EDIT))) { printf("  FAIL EDIT light\n"); fails++; } else printf("  ok   EDIT is lit\n");
    run_ms(600);
    if (!sd_clock_on()) { printf("  FAIL the clock should run\n"); fails++; } else printf("  ok   the lanes start the clock, the beat off\n");
    expect("beat", 0);
    btn_now |= 1u << B_EDIT;
    run_ms(50);
    expect("layer", 4);
    enc_pending[EN_K2] = 1;              /* lane 1: PULSES 4 -> 5 */
    run_ms(100);
    enc_pending[EN_K1] = -12;            /* STEPS 16 -> 4: the pulses kept to what fits */
    run_ms(100);
    expect("L0s", 4);
    expect("L0p", 4);
    enc_pending[EN_K3] = -1;             /* ROTATE wraps: 0 -> 3 */
    run_ms(100);
    expect("L0r", 3);
    enc_pending[EN_K4] = -3;             /* CHANCE 100 -> 85 */
    run_ms(100);
    expect("L0c", 85);
    enc_pending[EN_SELECT] = 1;          /* lane 2 */
    run_ms(100);
    expect("lane", 1);
    enc_pending[EN_PRESET] = 2;          /* PLAYS: one choice a turn, HAT -> OPEN */
    run_ms(100);
    expect("L1t", ET_OPEN);
    enc_pending[EN_ALGO] = -1;           /* EVERY 1/16 -> 1/8 */
    run_ms(100);
    expect("L1d", 3);
    expect("L0s", 4);                    /* lane 1 untouched by lane 2's turns */
    enc_pending[EN_SELECT] = 9;          /* past the last lane: held */
    run_ms(100);
    enc_pending[EN_SELECT] = 1;
    run_ms(100);
    enc_pending[EN_SELECT] = 1;
    run_ms(100);
    expect("lane", 3);
    btn_now &= ~(1u << B_EDIT);
    run_ms(50);
    expect("euc", 1);                    /* turns while held: no switch */
    expect("layer", 0);
    btn_tap(B_SAVE);
    memset(&set, 0, sizeof set);
    app_init();
    run_ms(50);
    expect("euc", 1);
    expect("L0s", 4);
    expect("L0c", 85);
    expect("L1t", ET_OPEN);
    btn_tap(B_EDIT);
    expect("euc", 0);
    run_ms(100);
    if (sd_clock_on()) { printf("  FAIL the clock should stop\n"); fails++; } else printf("  ok   lanes off: the clock stops\n");

    printf("0.3's saved sound carries over\n");
    {
        settings_v3_t v3;
        int i;
        memset(&v3, 0, sizeof v3);
        v3.magic = SET_MAGIC_V3;
        for (i = 0; i < 25; i++) v3.param[i] = 0.5f;
        v3.param[18] = 0.8f;
        v3.octave = 1;
        v3.nchord = 1;
        v3.chord[0] = 50;
        v3.beat = 1;
        memcpy(store, &v3, sizeof v3);
        store_len = (int)sizeof v3;
        memset(&set, 0, sizeof set);
        app_init();
        run_ms(50);
        expect("p18", 80);
        expect("beat", 1);
        expect("octave", 1);
        expect("euc", 0);
        expect("L0s", 16);               /* the lanes at their defaults */
        expect("L2t", ET_ARP);
        btn_tap(B_SEQ);
        expect("beat", 0);
    }

    printf("0.2's saved sound carries over\n");
    {
        settings_v2_t v2;
        int i;
        memset(&v2, 0, sizeof v2);
        v2.magic = SET_MAGIC_V2;
        for (i = 0; i < 18; i++) v2.param[i] = 0.5f;
        v2.param[14] = 0.66f;
        v2.octave = -2;
        v2.nchord = 2;
        v2.chord[0] = 45; v2.chord[1] = 52;
        v2.arp = 1;
        memcpy(store, &v2, sizeof v2);
        store_len = (int)sizeof v2;
        memset(&set, 0, sizeof set);
        app_init();
        run_ms(50);
        expect("p14", 66);
        expect("arp", 1);
        expect("octave", -2);
        expect("beat", 0);
        expect("p18", 30);
        btn_tap(B_ARP);
    }

    printf("0.1's saved sound carries over\n");
    {
        settings_v1_t v1;
        int i;
        memset(&v1, 0, sizeof v1);
        v1.magic = SET_MAGIC_V1;
        for (i = 0; i < 14; i++) v1.param[i] = 0.5f;
        v1.param[0] = 0.77f;
        v1.octave = 1;
        v1.nchord = 3;
        v1.chord[0] = 40; v1.chord[1] = 47; v1.chord[2] = 52;
        memcpy(store, &v1, sizeof v1);
        store_len = (int)sizeof v1;
        memset(&set, 0, sizeof set);
        app_init();
        run_ms(50);
        expect("p0", 77);
        expect("octave", 1);
        expect("nchord", 3);
        expect("arp", 0);
        expect("p14", 45);               /* the arp's controls at their defaults */
    }

    printf("an hour of frames and audio with six voices\n");
    {
        int k;
        for (k = 0; k < 6; k++) key_tap(k * 2);
    }
    run_ms(3600u * 1000u / 60u);         /* a minute of frames (the DSP's hour runs in dsp_render) */
    expect("voices", 6);

    printf(fails ? "%d FAILED\n" : "all passed\n", fails);
    return fails != 0;
}
