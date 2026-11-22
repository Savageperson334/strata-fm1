/* SPDX-License-Identifier: MPL-2.0 */
/* BabbleBeat's voice: a small formant synthesizer in the manner of Klatt's (a glottal source and breath noise
 * through a cascade of formant resonators for the voiced sounds, shaped noise beside it for hisses and bursts),
 * driven by the phoneme spellings of lang/babble_lang.c. Not general text-to-speech: it says what the lexicon
 * spells. */
#pragma once
#include <stdint.h>
#include "babble_lang.h"
#define BB_QUEUE 256
#define BB_LEVELS 44                     /* the level display: this many steps of about 23 ms */
typedef struct {float n,f1,f2,f3,av,ah,af,kf,ka,nz;} bb_seg;   /* a stretch of a phone: its length in control steps and
                                          * what the voice moves toward meanwhile */
typedef struct {
 /* what to say next, handed over from the panel: written there into the slot the voice is not reading, then
  * request counts up; the voice fades what it is saying, takes the slot, and says so in seen */
 char slot[2][BB_QUEUE];
 uint16_t slot_used[2];
 int slot_note[2];
 volatile uint32_t request,seen;
 /* what it is saying */
 char queue[BB_QUEUE];
 uint16_t used,pos;
 int note,question,fading,talking;
 bb_seg seg[3];
 int nseg,iseg;
 float left;                             /* control steps left of the stretch */
 /* where the voice is now (moving toward the stretch's targets) */
 float f1,f2,f3,av,ah,af,f0,accent,nz;
 float ff,fbw;                           /* the hiss's centre and width */
 /* the controls */
 float pitch,rate,tone,breath,volume,stutter,inflect;
 /* the sound */
 uint32_t noise,ctl;
 float phase,inc,closing,inv_closing,peak,flutter;
 float ra[5],rb[5],rc[5],y1[5],y2[5];    /* resonators: F1..F4 in cascade, then the hiss's */
 float hw1,hw2;
 float out_gain;
 /* for the screen */
 uint8_t level[BB_LEVELS];
 volatile uint32_t level_at;
 float level_peak;
 uint32_t level_n;
} bb_voice;
void bb_voice_init(bb_voice*);
void bb_voice_stop(bb_voice*);
void bb_voice_controls(bb_voice*,const float knobs[7]);   /* PITCH RATE TONE BREATH LEVEL STUTTER INFLECT, 0..1 */
void bb_voice_say(bb_voice*,const bb_mind*,int note);     /* the whole sentence; note -1: the PITCH knob's, spoken */
void bb_voice_word(bb_voice*,const bb_token*,int note);   /* one word at a MIDI note's pitch, held level */
int bb_voice_active(const bb_voice*);    /* something is being said, or is about to be */
float bb_voice_sample(bb_voice*);        /* the next sample, at 44.1 kHz, within -1..1 */
float bb_voice_progress(const bb_voice*);                 /* how far through what it is saying, 0..1 */
void bb_voice_levels(const bb_voice*,float *out,int n);   /* the last n (<= BB_LEVELS) levels it made, oldest first, 0..1 */
