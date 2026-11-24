/* SPDX-License-Identifier: MPL-2.0
 *
 * What the launcher hands an instrument. The platform compiles the app (here: the launcher) inside its own single
 * unit, and its drawing, its fonts, its colours and its announcer are private to that unit. An instrument in a
 * firmware of several is a unit of its own, so the launcher passes those on as ordinary functions; engine.h, on
 * the instrument's side, gives them back their platform names. The panel goes through here too, so that the menu
 * can keep it from the instrument. */
#pragma once
#include <stdint.h>
typedef struct { uint8_t which; } sdr_font;       /* stands for a platform font: SDR_FONT[0] small, [1] bold */
extern const sdr_font SDR_FONT[2];
void sdr_cv_begin(uint32_t w, uint32_t h, uint16_t bg);
void sdr_cv_blit(uint32_t x, uint32_t y);
void sdr_cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c);
void sdr_cv_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c);
int32_t sdr_cv_text(int32_t x, int32_t y, const sdr_font *f, const char *s, uint16_t c);
int32_t sdr_text_w(const sdr_font *f, const char *s);
void sdr_lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c);
uint16_t sdr_color(int k);                        /* the palette's five: 0 rules .. 4 values */
int sdr_ann_on(void);
int sdr_ann_where(void);
void sdr_ann_say(const char *a, const char *b, uint32_t flags);
void sdr_ann_tick(uint32_t now);
uint32_t sdr_buttons(void);                       /* the panel as the instrument is to see it: never GLO (the */
uint32_t sdr_keys(void);                          /* .. menu's), nothing while the menu is open, GLO alone while */
int32_t sdr_enc(int role);                        /* .. the launcher shows the instrument's own guide */
/* the memory STRATA's buffers and PALINDROME's rings share: one of the two runs at a time, and the launcher
 * clears it on every change (in 32-bit words; 196,608 bytes) */
#define SDR_WORKSPACE_WORDS 49152u
extern uint32_t sdr_workspace[SDR_WORKSPACE_WORDS];
/* an instrument's calls, under its own name */
#define SDR_ENGINE(p)                                                                   \
    void p##_init(void); void p##_ui_init(void); void p##_frame(void);                 \
    void p##_input_only(void); void p##_panic(void);                                   \
    void p##_render(int32_t *lr, uint32_t n, uint32_t master_q12);                     \
    void p##_say(const char *a, const char *b); int p##_expect(const char *what, int *got);
SDR_ENGINE(strata) SDR_ENGINE(palindrome) SDR_ENGINE(babblebeat)
void strata_overload(void);
