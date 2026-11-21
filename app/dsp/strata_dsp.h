/* SPDX-License-Identifier: MPL-2.0
 * STRATA: a drone and texture synth for the M-VAVE FM-1.
 * The sound engine. Self-contained (no platform calls): the FM-1 build compiles it at -O2 (fm1.toml o2),
 * and tests/dsp_render.c renders it to WAV on a computer. */
#pragma once
#include <stdint.h>

#define SD_RATE 44100.0f
#define SD_VOICES 6                      /* drone voices; each is two detuned FM pairs, a sub and a breath of noise */

/* the 14 controls, 0..1 each. The first 7 are the panel's main layer, the next 7 its SEL-held layer */
enum {
    P_TONE,                              /* FM index: pure sine .. bright, glassy */
    P_RATIO,                             /* modulator ratio, swept through 0.5 .. 7 (whole ratios harmonic, between them bells) */
    P_MOTION,                            /* the slow LFO's rate, 0.01 .. 3 Hz */
    P_SPACE,                             /* reverb mix */
    P_FILTER,                            /* low-pass cutoff, 40 Hz .. 14 kHz */
    P_DRIFT,                             /* detune spread and slow random wander of pitch and level */
    P_GRIT,                              /* modulator feedback: smooth .. noisy */
    P_DEPTH,                             /* how far MOTION moves the tone and the filter */
    P_BEAT,                              /* modulator offset, 0 .. 3 Hz: slow beating shimmer */
    P_FADE,                              /* voices fade in and out over 0.1 .. 20 s */
    P_DECAY,                             /* reverb length, about 1.5 s .. near-endless */
    P_RESO,                              /* filter resonance */
    P_AIR,                               /* band-passed noise tuned to each voice */
    P_SUB,                               /* a sine an octave below each voice */
    SD_NPARAM
};

void sd_init(void);
void sd_param(int id, float v);          /* any time, from any context: glides there over ~50 ms */
float sd_param_get(int id);
void sd_voice_on(int slot, int note);    /* fade a voice in at a MIDI note (a sounding voice crossfades to it) */
void sd_voice_off(int slot);             /* fade it out */
int sd_voice_busy(int slot);             /* still audible (or fading) */
float sd_voice_level(int slot);          /* 0..1, for the screen */
void sd_panic(void);                     /* silence at once */
/* n stereo frames, float, about -1..1, before the master gain */
void sd_render(float *l, float *r, uint32_t n);
/* the latest output, 64 points at 1/16 rate (about 23 ms), for the screen */
void sd_scope(float *dst64);
float sd_lfo(void);                      /* the MOTION LFO, -1..1 */

/* readable values for the screen */
float sd_ratio_of(float v);
float sd_motion_hz(float v);
float sd_cutoff_hz(float v);
float sd_fade_s(float v);
float sd_decay_s(float v);
float sd_beat_hz(float v);
