/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>
#define PM_RATE 44100u
#define PM_VOICES 4
#define PM_BUFFER 32768u
#define PM_GRAINS 12
enum { M_X, M_Y, M_COMPLEX, M_TONE, M_SPREAD, M_MOVE, M_EXCITE,
       S_POS, S_SIZE, S_DENSITY, S_SPRAY, S_PITCH, S_REVERSE, S_FEEDBACK,
       G_MIX, G_FADE, G_SCAN, G_WINDOW, G_STEREO, G_DAMP, G_LEVEL, PM_PARAMS };
enum { PM_LIVE, PM_FREEZE, PM_OVERDUB, PM_RESAMPLE };
void pm_init(void);
void pm_panic(void);
void pm_set(int p, float v);
float pm_get(int p);
void pm_note(int slot, int note);
void pm_record(int mode);
void pm_grains(int on);
void pm_burst(void);
void pm_render(float *left, float *right, uint32_t n);
void pm_scope(float *out, uint32_t n);
void pm_memory(float *lo, float *hi, uint32_t n);
float pm_fill(void);
float pm_morph_x(void);
float pm_morph_y(void);
int pm_active_grains(void);
/* for the screen: the memory as the ring it is */
float pm_head(void);                     /* the write head, 0..1 round the memory */
void pm_ring(float *lo, float *hi, uint32_t n);   /* the memory by where it lies, in n arcs: each one's lowest and
                                          * highest sample; an arc not yet written has lo > hi */
float pm_position(void);                 /* where grains start, 0 newest .. 1 oldest (SCAN's motion in it) */
int pm_grain(int i, float *at, float *env);   /* grain i: 0 silent, 1 forward, -1 reverse; where it reads (0..1 round
                                          * the memory) and how loud its window is now (0..1) */
