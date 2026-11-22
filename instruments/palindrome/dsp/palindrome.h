/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>
#define PM_RATE 44100u
#define PM_VOICES 4
#define PM_BUFFER 32768u
#define PM_GRAINS 16
#define PM_PITCHES 4                     /* keys held for the grains: the pitches they take */
enum { M_X, M_Y, M_COMPLEX, M_TONE, M_SPREAD, M_MOVE, M_EXCITE,
       S_POS, S_SIZE, S_DENSITY, S_SPRAY, S_PITCH, S_REVERSE, S_FEEDBACK,
       G_MIX, G_FADE, G_SCAN, S_SHAPE, G_STEREO, G_DAMP, G_LEVEL,
       /* how the memory is written (the panel's REC-held page): noise put in, and noise kept out */
       R_NOISE,                          /* hiss recorded along with the sound: none .. loud */
       R_COLOR,                          /* that hiss's tone: a low rumble .. a bright hiss */
       R_CRUSH,                          /* the recording's resolution: clean .. a few coarse steps */
       R_GATE,                           /* only sound louder than this is recorded; under it the head lifts and what
                                          * was there stays (0: everything is recorded) */
       R_DROP,                           /* dropouts: how often the head skips a short stretch, leaving older sound */
       /* how the grains are played (the panel's EDIT-held page) */
       S_TIMING,                         /* when grains start: an even pulse .. scattered in time */
       S_VARY,                           /* each grain's length: all SIZE .. anywhere from a quarter to four times it */
       S_SCATTER,                        /* the chance a grain jumps an octave (then also a fifth, two octaves) */
       S_DETUNE,                         /* each grain a little off pitch: none .. half a semitone either way */
       S_CLUSTER,                        /* grains started together: 1 .. 4, each on the next held key */
       PM_PARAMS };
/* S_SHAPE (where G_WINDOW was): a grain's loudness over its length: struck (in fast, out slow) .. a bell .. a swell
 * (in slow, out fast) */
enum { PM_LIVE, PM_FREEZE, PM_OVERDUB, PM_RESAMPLE };
void pm_init(void);
void pm_panic(void);
void pm_set(int p, float v);
float pm_get(int p);
void pm_note(int slot, int note);
void pm_record(int mode);
void pm_grains(int on);
void pm_burst(void);
/* the keys held for the grains (SILT's keys): slot 0..PM_PITCHES-1 at semis semitones from the recorded pitch
 * (-48..48), or PM_NO_PITCH to let it go. The grains take the held pitches in turn; none held, the recorded pitch */
#define PM_NO_PITCH 127
void pm_grain_pitch(int slot, int semis);
void pm_strike(int slot);                /* grains now at that slot's pitch (a key played), whether PLAY runs or not */
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
float pm_writing(void);                  /* how much the head is writing now, 0 (lifted, or frozen) .. 1 */
int pm_grain(int i, float *at, float *env);   /* grain i: 0 silent, 1 forward, -1 reverse; where it reads (0..1 round
                                          * the memory) and how loud its window is now (0..1) */
