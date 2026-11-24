/* SPDX-License-Identifier: MPL-2.0 */
/* PALINDROME's engine (1.0). MORPH makes a sound; REC catches it in one of three rings; the rings are scanned.
 *
 *   MORPH   a four-corner synth (FM, MODAL, ADD, FOLD). A key strikes a note that lasts exactly one ring (1.49 s):
 *           its loudness follows SHAPE (a pluck .. a swell) and is gone by the ring's end, and behind it hidden
 *           envelopes carry X and Y toward their mirror image and back, the complexity from bright to plain and the
 *           tone darker, as far as MOVE says, from where the knobs are now. So a note recorded is a ring that loops
 *           clean, and the same key sounds the same way
 *   RINGS   three recordings of PM_BUFFER samples at half the engine's rate (1.49 s each), always there, empty or
 *           not. Each has a head going round it (SPEED: backward, still, as recorded, faster) and grains read from
 *           the head (POSITION, SIZE, DENSITY, PITCH, SPREAD), as in a granular looper: DENSITY low, a grain now and
 *           then; at about 2 they overlap into the plain recording, time-stretched by SPEED and transposed by PITCH
 *   CAPTURE one ring's length of MORPH (from the note it starts with), or of everything heard but the ring written
 *           (the others resampled into it), then its level evened and its ends eased so it loops without a click
 * Single-precision floats, no library, no allocation in the render. */
#pragma once
#include <stdint.h>
#define PM_RATE 44100u
#define PM_VOICES 4
#define PM_BUFFER 32768u                 /* a ring, at half rate: 65,536 B of the pool each */
#define PM_NOTE (2u * PM_BUFFER)         /* a MORPH note's life in samples: one ring */
#define PM_RINGS 3
#define PM_GRAINS 18
#define PM_RING_GRAINS 6                 /* the most grains one ring sounds at once */
/* the controls everyone shares */
enum { M_X, M_Y, M_COMPLEX, M_TONE, M_MOVE, M_SHAPE,     /* MORPH */
       G_RING1, G_RING2, G_RING3, G_MORPH, G_DAMP, G_WIDTH, /* MIX: each ring's level, MORPH's, the rings' tone, width */
       PM_PARAMS };
/* the controls each ring keeps for itself */
enum { R_POS, R_SIZE, R_DENSITY, R_PITCH, R_SPEED, R_SPREAD, PM_RPARAMS };
void pm_init(void);
void pm_panic(void);
void pm_set(int id, float v);            /* 0..1 */
float pm_get(int id);
void pm_rset(int ring, int id, float v);
float pm_rget(int ring, int id);
float pm_size_s(float v);                /* SIZE in seconds */
float pm_overlap(float v);               /* DENSITY as grains overlapping */
int pm_semis(float v);                   /* PITCH in semitones, -24..24 */
#define PM_SPEEDS 10
float pm_speed(float v);                 /* SPEED in steps: -2, -1, -.5, -.25, 0 (still), .25, .5, 1 (as recorded), 1.5, 2 */
float pm_speed_value(int step);          /* the knob's value for a step, 0..PM_SPEEDS-1 */
int pm_speed_step(float v);

void pm_note(int note);                  /* a MORPH note struck (MIDI), for one ring's length */
int pm_notes(void);                      /* notes sounding */
float pm_note_age(void);                 /* the newest note: how far through its life, 0..1; -1 none */
float pm_morph_x(void);                  /* where the newest note's hidden envelopes have MORPH now (else the knobs) */
float pm_morph_y(void);

void pm_play(int on);                    /* every ring with a recording sounds (as its level says) */
void pm_gate(int ring, int on);          /* a key holds a ring open: it sounds whether PLAY runs or not */
void pm_jump(int ring, float pos);       /* its head to pos (0..1): a slice */
void pm_key_pitch(int ring, int semis);  /* added to PITCH, -48..48 */
int pm_key_semis(int ring);

int pm_ring_ready(int ring);             /* a finished recording there */
void pm_ring_clear(int ring);            /* emptied (its grains fade out); not while it is being captured */
int pm_ring_count(void);
void pm_ring_wave(int ring, float *lo, float *hi, uint32_t n);   /* n arcs: lowest and highest sample; empty: lo > hi */
float pm_head(int ring);                 /* where the grains start now, 0..1 round it */
float pm_ring_level(int ring);           /* how loud it is playing now, 0..1 (for the screen) */
int pm_grain(int i, float *at, float *env, int *ring);   /* 0: silent; else where it reads (0..1), its window, its ring */
int pm_active_grains(void);
#ifdef PLAT_HOST
const int16_t *pm_ring_pcm(int ring);    /* (the tests': its samples) */
#endif

enum { PM_SRC_MORPH, PM_SRC_MIX };
int pm_capture(int ring, int source);    /* 0: started; -1: one is running */
int pm_capture_ring(void);               /* the ring being written, -1 none */
int pm_capture_source(void);
float pm_capture_progress(void);         /* 0..1 */
#define PM_EV_SILENT (-2)
int pm_service(void);                    /* every frame: a ring just finished (its number), PM_EV_SILENT (nothing to keep:
                                          * the ring left empty), or -1 */
void pm_render(float *left, float *right, uint32_t n);
void pm_scope(float *out, uint32_t n);
