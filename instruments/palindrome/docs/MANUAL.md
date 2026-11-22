# PALINDROME

An experimental FM-1 instrument in two halves: **MORPH makes sound; SILT records it and takes it apart.** Both
keep running whichever one the knobs are on. It's custom firmware, installed at your own risk, and it has not been
tested on a real FM-1 yet. Try the browser version first.

![The screen: MORPH's square in the middle of SILT's memory ring](img/morph.png)

## Quick start

1. Turn MASTER up. Tap a few keys (up to four notes latch; tap one again to let it go).
2. Turn KNOB1 and KNOB2 to move the white spot around the square. Each corner is a different way of making sound.
3. Wait two seconds for the ring to fill, then tap EDIT. The knobs are now SILT's.
4. Tap REC once to FREEZE what's recorded. Tap PLAY to start the grains.
5. Turn KNOB1 (where the grains start), KNOB2 (how long they are) and KNOB3 (how many).
6. Tap FX to go back to MORPH's knobs. The grains carry on.

## The screen

The screen is a picture of what the instrument is doing, with the buttons and knobs named around it.

- **The ring** is SILT's memory, about a second and a half of sound, drawn as the loop it is. The recorded wave
  stays where it was written. The **white tick** is the write head going round and writing over it.
- **The mark outside the ring** is where grains start (POSITION). The **dotted arc** beside it is how far they
  scatter (SPRAY).
- **Each grain is a spot riding the ring** with a short tail. Grains playing forward ride the outside; grains
  playing backward ride the inside.
- **The square** in the middle is MORPH. Its four corners are four ways of making sound: FM, MODAL (struck,
  bell-like tones), ADD (added harmonics) and FOLD (wavefolding). Each name lights up as much as you hear it. The
  **white spot** is where the sound is now, and the **loop around it** is the sound's own wave.
- **The dotted circle** in the square is the path MOVE carries the spot on. The **four pips** under the square
  are the voices, as far apart as SPREAD has them.
- **The four corners of the screen** name the buttons and show how they stand. Top: **FX: MORPH** and
  **EDIT: SILT**, with the one whose knobs are in use lit (holding SEL shows SEL: SHARED). Bottom left: **REC:** and
  how the memory is being written. Bottom right: **PLAY: GRAINS**, lit while the grains run.
- **The eight boxes at the foot** name what each knob sets right now, with a bar for its value. They sit as the
  knobs do on the FM-1: MASTER and SELECT over PRESETS and ALGORITHM on the left, KNOB1 and KNOB2 over KNOB3 and
  KNOB4 on the right. Turn a knob and its box turns white and shows the number.
- **A hint** appears across the middle until you've done the thing once: tap keys, then PLAY, then REC.

Whichever half the knobs are on is drawn bright; the other is dim but still moving.

How the write head looks:

| REC mode | Head | What happens |
| --- | --- | --- |
| LIVE | One white tick, moving | MORPH is recorded over the oldest sound |
| FREEZE | Two short marks, still | Nothing is written; the memory is safe |
| OVERDUB | Two ticks, moving | New sound is mixed onto what's there |
| RESAMPLE | A tick with a block, moving | The grains themselves are recorded back in |

![SILT: the memory frozen, grains riding the ring](img/silt.png)

## MORPH — tap FX

| Knob | Control | What it does |
| --- | --- | --- |
| KNOB1 | X | The spot left and right |
| KNOB2 | Y | The spot up and down |
| KNOB3 | COMPLEX | How rich each corner's sound is |
| KNOB4 | TONE | Low-pass: dark to bright |
| SELECT | SPREAD | How far apart the voices sit in stereo |
| ALGORITHM | MOVE | Carries the spot round a circle by itself |
| PRESETS | EXCITE | How long the struck corner rings |

Keys latch notes and unlatch them one by one. OCT− and OCT+ move the keys; notes already playing keep their pitch.

## SILT — tap EDIT

| Knob | Control | What it does |
| --- | --- | --- |
| KNOB1 | POSITION | Where grains start, from the newest sound to the oldest |
| KNOB2 | SIZE | Grain length, 20–300 ms |
| KNOB3 | DENSITY | 1–40 grains a second |
| KNOB4 | SPRAY | How far grains scatter from POSITION |
| SELECT | PITCH | Two octaves down to two octaves up |
| ALGORITHM | REVERSE | The chance a grain plays backward |
| PRESETS | FEEDBACK | How much of the old sound survives in OVERDUB and RESAMPLE |

- **PLAY** starts and stops the stream of grains. PLAY is lit while it runs.
- **REC** steps through LIVE, FREEZE, OVERDUB, RESAMPLE.
- **In SILT, a key** jumps POSITION to that point of the memory and fires a burst of grains.

## Shared — hold SEL

| Knob | Control | What it does |
| --- | --- | --- |
| KNOB1 | MIX | MORPH against the grains |
| KNOB2 | FADE | How slowly notes come and go |
| KNOB3 | SCAN | Moves POSITION back and forth by itself |
| KNOB4 | WINDOW | The grains' edges: soft to softer |
| SELECT | STEREO | How wide the grains are scattered |
| ALGORITHM | DAMP | Dulls what is recorded |
| PRESETS | LEVEL | Overall level |

## The other buttons

- **HOME, held for a second,** lets every note go and stops the grains. A tap only shows the version, so you can't
  clear everything by accident.
- **SAVE** does nothing yet: the sound and the memory are lost at power-off.

## Limits

No saved patches, no MIDI in, and the memory is short. Installing PALINDROME leaves STRATA's saved sound alone, and
either installer can put the other firmware (or M-VAVE's own) back.
