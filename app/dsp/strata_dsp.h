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
    /* the arpeggiator's (the panel's ARP-held layer) */
    P_ARATE,                             /* steps a second, 0.1 .. 8 */
    P_ASHAPE,                            /* each step's swell: sharp (a pluck) .. smooth (a slow crossfade) */
    P_AORDER,                            /* UP, DOWN, UP-DOWN, RANDOM (four quarters of the range) */
    P_AFLOOR,                            /* how loud the voices off the step stay: silent .. nearly full */
    /* the beat's (the panel's SEQ-held layer) */
    P_BTEMPO,                            /* 60 .. 160 BPM */
    P_BGROOVE,                           /* one of SD_GROOVES patterns */
    P_BSWING,                            /* 50 .. 75 % */
    P_BLEVEL,                            /* the drums' level */
    P_BKIT,                              /* the kit: dark and low .. bright and snappy */
    P_BROOM,                             /* how much of the drums goes into the reverb */
    P_BDUCK,                             /* the drone ducks under the kick: none .. deep pumping */
    /* the bitcrusher's (the panel's PLAY-held layer): after the filter, before the reverb */
    P_CBITS,                             /* 16 bits .. 2 */
    P_CRATE,                             /* the sample rate it holds to, 44.1 kHz .. about 400 Hz */
    P_CDRIVE,                            /* how hard it is pushed in: clean .. folded and clipped */
    P_CMIX,                              /* how much of it: none (off) .. all */
    SD_NPARAM
};

void sd_init(void);
void sd_param(int id, float v);          /* any time, from any context: glides there over ~50 ms */
float sd_param_get(int id);
int sd_crush_bits(float v);              /* BITS as bits, 16 .. 2 */
float sd_crush_hz(float v);              /* SAMPLE as Hz */
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

/* the arpeggiator: the sounding voices one at a time, lowest to highest (or as ORDER says), each swelling up while the
 * rest sink to FLOOR; one voice alone pulses on and off */
void sd_arp(int on);
int sd_arp_on(void);
int sd_arp_slot(void);                   /* the voice on the step now, -1 none */

/* the beat: a synthesized kit (kick, snare, closed and open hat, a percussion voice tuned an octave above the lowest
 * voice) playing one of SD_GROOVES 16-step patterns under the drone. While it runs, the arp steps on its grid
 * (RATE picks a whole note .. a sixteenth) */
#define SD_GROOVES 8
void sd_beat(int on);                    /* on: from the first step */
int sd_beat_on(void);
int sd_beat_step(void);                  /* 0..15, the step playing now; -1 off */
const char *sd_groove_name(float v);
float sd_tempo_bpm(float v);
float sd_swing_pct(float v);
const char *sd_arp_div_name(float v);    /* RATE as a note length, while the clock runs */

/* the Euclidean lanes: four generators on the beat's clock, each spreading `pulses` hits as evenly as it can over
 * `steps` steps, E(pulses, steps), with its own length, rotation, chance and step length, so lanes of different
 * lengths drift against each other. A lane plays one of the kit's drums IN PLACE OF the groove's part for that drum
 * (with the beat off, the lanes alone are the drums), or steps the arpeggiator on its hits. The clock runs while the
 * beat or a lane needs it; it starts from step one when the first of them starts.
 * Rotation 0 puts a hit on a lane's first step; each rotation step moves the pattern one step later. The hit on a
 * lane's first step is accented. */
#define SD_LANES 4
enum { ET_OFF, ET_KICK, ET_SNARE, ET_HAT, ET_OPEN, ET_PERC, ET_ARP, ET_NTARGET };
#define SD_LANE_DIVS 5                   /* div 0 .. 4: a step is 1/1, 1/2, 1/4, 1/8, 1/16 */
typedef struct {
    uint8_t steps;                       /* 1 .. 32 */
    uint8_t pulses;                      /* 0 .. steps */
    uint8_t rot;                         /* 0 .. steps - 1 */
    uint8_t prob;                        /* 0 .. 100: the chance each hit plays */
    uint8_t div;                         /* 0 .. SD_LANE_DIVS - 1 */
    uint8_t target;                      /* ET_* */
} sd_lane_t;
uint32_t sd_euclid_mask(int steps, int pulses, int rot);   /* bit i: a hit on step i */
void sd_lane_set(int lane, const sd_lane_t *l);            /* main loop; out-of-range fields are clamped */
void sd_euclid(int on);                  /* all four lanes on / off */
int sd_euclid_on(void);
uint32_t sd_lane_mask(int lane);         /* the lane's pattern as it plays */
int sd_lane_pos(int lane);               /* the step it played last, -1 not running */
int sd_arp_lane(void);                   /* the lane stepping the arp, -1 none */
int sd_clock_on(void);                   /* the clock runs (the beat, or a lane) */
const char *sd_div_name(int div);
const char *sd_target_name(int target);

/* readable values for the screen */
float sd_ratio_of(float v);
float sd_motion_hz(float v);
float sd_cutoff_hz(float v);
float sd_fade_s(float v);
float sd_decay_s(float v);
float sd_beat_hz(float v);
float sd_arp_hz(float v);
int sd_arp_order(float v);               /* 0 UP, 1 DOWN, 2 UP-DOWN, 3 RANDOM */
