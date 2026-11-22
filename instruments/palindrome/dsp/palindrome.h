/* SPDX-License-Identifier: MPL-2.0 */
/* PALINDROME's engine. SILT is the instrument: grains played from the selected ring. MORPH is the injector: a
 * four-corner synth whose sound is captured into rings. A ring is one finished recording (PM_BUFFER samples at half
 * the engine's rate, 1.49 s) that is never written again: a capture always makes a new ring, from MORPH or from
 * SILT's own grains (docs/PALINDROME_SILT_FIRST_REDESIGN.md). */
#pragma once
#include <stdint.h>
#define PM_RATE 44100u
#define PM_VOICES 4
#define PM_BUFFER 32768u
/* how many rings are kept. Each is 65,536 B of the pool: PALINDROME 0.5's device build used 134,660 B of the pool's
 * 344,064 with one, so three leave about 78 KB spare and four about 12 KB (the build insists on 8 KB): three. */
#define PM_RINGS 3
#define PM_GRAINS 16
#define PM_PITCHES 4                     /* keys held for the grains: the pitches they take */
enum { M_X, M_Y, M_COMPLEX, M_TONE, M_SPREAD, M_MOVE, M_EXCITE,
       S_POS, S_SIZE, S_DENSITY, S_SPRAY, S_PITCH, S_REVERSE,
       R_CARRY,                          /* how much of the selected ring is copied under the next capture (0: none;
                                          * 0.5's FEEDBACK, which kept old sound through an overdub) */
       G_MIX, G_FADE, G_SCAN,
       S_SHAPE,                          /* a grain's loudness over its length: struck .. a bell .. a swell */
       G_STEREO, G_DAMP, G_LEVEL,
       /* what a capture does to the sound it records: noise put in, and noise kept out */
       R_NOISE,                          /* hiss recorded along with the sound: none .. loud */
       R_COLOR,                          /* that hiss's tone: a low rumble .. a bright hiss */
       R_CRUSH,                          /* the recording's resolution: clean .. a few coarse steps */
       R_GATE,                           /* only sound louder than this is recorded; under it, silence */
       R_DROP,                           /* dropouts: how often a short stretch is left out */
       /* how the grains are played */
       S_TIMING,                         /* when grains start: an even pulse .. scattered in time */
       S_VARY,                           /* each grain's length: all SIZE .. anywhere from a quarter to four times it */
       S_SCATTER,                        /* the chance a grain jumps an octave (then also a fifth, two octaves) */
       S_DETUNE,                         /* each grain a little off pitch: none .. half a semitone either way */
       S_CLUSTER,                        /* grains started together: 1 .. 4, each on the next held key */
       M_SUSTAIN,                        /* a MORPH note's life: struck and gone in a moment .. dying away over seconds
                                          * .. (the top of the knob) held until let go */
       G_DRIFT,                          /* the loop made again by itself: never (it stays as it is) .. now and then,
                                          * most of it kept .. one generation after another, each all new. The
                                          * engine only holds the value: the panel starts the generations */
       S_LOOP,                           /* the play point going round the ring by itself: still (POSITION alone) ..
                                          * at the speed it was recorded (the middle: the ring heard as a loop) ..
                                          * four times that */
       PM_PARAMS };
void pm_init(void);
void pm_panic(void);
void pm_set(int p, float v);
float pm_get(int p);
void pm_note(int slot, int note);        /* a MORPH note on a voice (0..PM_VOICES-1), or -1: let go. Each call with a
                                          * note strikes it: under DECAY's top it then dies away by itself */
int pm_voice_sounding(int slot);         /* that voice still has a note (held, or not yet died away) */
void pm_monitor(int on);                 /* MORPH to be heard whatever MIX says (its own page; a capture of it) */
void pm_grains(int on);
void pm_burst(void);
/* the keys held for the grains: slot 0..PM_PITCHES-1 at semis semitones from the recorded pitch (-48..48), or
 * PM_NO_PITCH to let it go. The grains take the held pitches in turn; none held, the recorded pitch */
#define PM_NO_PITCH 127
void pm_grain_pitch(int slot, int semis);
void pm_strike(int slot);                /* grains now at that slot's pitch (a key played), whether PLAY runs or not */
void pm_render(float *left, float *right, uint32_t n);
void pm_scope(float *out, uint32_t n);
float pm_morph_x(void);
float pm_morph_y(void);
int pm_active_grains(void);

/* --- the rings. Called from the UI (never from the render); a slot is 0..PM_RINGS-1, a ring's id counts up from 1
 * and is never used twice, so a ring's parent stays the same ring's whatever is deleted --- */
enum { PM_SRC_MORPH, PM_SRC_SILT };
int pm_ring_count(void);                 /* the finished rings */
int pm_ring_selected(void);              /* the slot the grains play and the SILT controls set; -1: none */
int pm_ring_select(int slot);            /* 0, or -1 (no finished ring there, or a capture is running). The ring left
                                          * keeps its settings; the one chosen brings its own back */
int pm_ring_info(int slot, int *id, int *parent, int *source);   /* 1: a finished ring (parent 0: from MORPH) */
int pm_ring_delete(int slot);            /* 0, or -1. Its grains fade out; another ring (the newest) is selected */
void pm_ring_clear(void);                /* every ring gone, a running capture dropped */
void pm_ring_wave(int slot, float *lo, float *hi, uint32_t n);   /* the ring in n arcs: each one's lowest and highest
                                          * sample; an arc with nothing written (yet) has lo > hi */
/* --- a capture: the source recorded into a new ring, for one ring's length, then finished (its level evened) and
 * handed over by pm_service. The rings there already are not touched --- */
enum { PM_CAP_IDLE, PM_CAP_RECORDING, PM_CAP_FINISHING };
enum { PM_ERR_BUSY = -1, PM_ERR_FULL = -2, PM_ERR_NOSRC = -3 };
/* a generation: a capture of SILT's grains with the ring they play kept under them. keep 0..1: how much of the new
 * ring is the old one as it was (in place, so what stays, stays put), the rest the grains. Returns as pm_capture */
int pm_capture_keep(int source,float keep);
#ifdef PLAT_HOST
const int16_t *pm_ring_pcm(int slot);    /* (the tests': a ring's samples, PM_BUFFER of them) */
#endif
int pm_capture(int source);              /* 0: started. PM_ERR_BUSY: one is running (nothing new is made);
                                          * PM_ERR_NOSRC: SILT with no ring. With every slot taken the oldest ring makes
                                          * room (never the selected one, which a capture reads): pm_capture_dropped */
int pm_capture_dropped(void);            /* the number of the ring the last pm_capture gave up for room, 0: none */
int pm_capture_state(void);
float pm_capture_progress(void);         /* 0..1 */
int pm_capture_slot(void);               /* where it is being written; -1: no capture */
int pm_capture_source(void);
enum { PM_EV_NONE, PM_EV_COMMITTED, PM_EV_SILENT };
int pm_service(int *slot);               /* call every frame. PM_EV_COMMITTED: the capture is a ring now, *slot,
                                          * selected; PM_EV_SILENT: there was nothing to record, it was dropped */
float pm_position(void);                 /* where grains start, 0..1 round the ring (SCAN's motion in it) */
int pm_grain(int i, float *at, float *env, int *slot);   /* grain i: 0 silent, 1 forward, -1 reverse; where it reads
                                          * (0..1 round its ring), how loud its window is now, which ring */
