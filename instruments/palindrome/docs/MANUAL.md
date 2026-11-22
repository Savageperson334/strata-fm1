# PALINDROME

**A sound, looped, and made again from itself.** MORPH strikes the seed, SILT loops it, DRIFT makes each new generation from the last. One press does everything; hold GLO and the screen shows what every button does.

[Play in the browser](../emu/) · [Install firmware](../install/)

## Quick start

1. The FM-1 starts on **SILT** with no rings. Raise MASTER and **tap a key**. MORPH strikes a note, it is caught as RING 01, and the grains loop it.
2. **Leave it.** Every few seconds DRIFT makes the loop again from itself: a new ring around the last. The screen shows the three newest generations.
3. Turn **DRIFT** (ALGORITHM) down to OFF and the loop stays as it is. Turn it up and generations come faster and keep less.
4. **Tap keys.** The key you seeded with plays the ring as recorded; the others transpose it.
5. Turn POSITION, SIZE and DENSITY. A knob you are turning is heard first; the next generation waits for your hands.
6. **FX** opens MORPH to play by itself. Press **REC** there, then a key, for a new seed.

## Buttons

Every button is one press. A page stays open until you open another.

| Button | Does |
| --- | --- |
| EDIT | Opens **SILT** |
| HOME | Back to **SILT** from anywhere; clears nothing |
| FX | Opens **MORPH SOURCE**; the keys become its notes |
| SEL | Opens **RINGS**; confirms a delete armed there |
| ENV | Opens **SILT SHAPE** |
| LFO | Opens **SILT MOTION** |
| ARP | Opens **MIX** |
| SEQ | Opens **REC FX** |
| GLO (hold) | The **guide**: what every button does, on the screen |
| REC | Captures the SOURCE into a **new ring** |
| PLAY | Grains on / off |
| OCT− / OCT+ | Shift the keys an octave; sounding notes and pitches stay |
| SAVE | Not used; the screen says so |

**SELECT** chooses the ring on every page. **MASTER** is always volume.

![GLO held: the guide](img/guide.png "The guide · hold GLO")

## SILT

**EDIT or HOME — granular playback of the selected ring**

![SILT: two rings, the selected one bright, grains riding it](img/silt.png "SILT · EDIT")

Plays up to 16 grains from the selected ring. Each grain has a position, length, pitch and direction, and stays on the ring it started on.

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **POSITION** | Where grains start, round the ring |
| KNOB2 | **SIZE** | 20–500 ms |
| KNOB3 | **DENSITY** | 1–40 starts/sec |
| KNOB4 | **SPRAY** | Random start position |
| SELECT | **RING** | Which ring plays and is edited |
| ALGORITHM | **DRIFT** | The loop made again by itself: OFF, now and then, back to back |
| PRESETS | **PITCH** | −24 to +24 semitones |

### Grain keys

Everywhere except MORPH's page the keys are grain pitches. Tap up to four to latch them; tap again to release. A new key triggers grains at once, even with PLAY off. The key the seed was struck with plays the ring as recorded (C until there is one). CLUSTER (SILT SHAPE) sets whether pitches play in turn or together. The top line shows them: `KEYS: 0 +7`.

### DRIFT

A generation is SILT's grains recorded over a share of the ring they play, as a new ring. It is evened to the same loudness every time, so generation after generation neither swells nor dies away.

| DRIFT | What happens |
| --- | --- |
| **OFF** | Nothing changes by itself. The loop is stable |
| Low | A generation every ten seconds or so; most of the ring is kept, so it changes slowly |
| High | One generation after another; little is kept, so it soon becomes something else |

- The screen counts down: `DRIFT: NEXT 4s`, then a percentage while the generation is made.
- DRIFT waits while you play: a key, a knob or a button starts the count again. It also waits on MORPH's and RINGS' pages and while no grains sound.
- What the grains do is what the next generation becomes: SPRAY blurs it, PITCH moves it, REVERSE turns it round, REC FX wear it.
- The three newest generations stay as rings. **SELECT** an older one and DRIFT carries on from there.

### The screen

Rings are concentric circles, the oldest innermost, each drawn from its own recording. The selected ring is bright and dense. The tick outside it is POSITION and the dotted arc is SPRAY; moving dots are grains. The small square in the middle is MORPH. Under the rings: **SOURCE** (what REC would capture) and **REC**: READY, ARMED, a percentage while capturing, DROPS 01 (the ring the next capture replaces), or DRIFT's countdown.

## MORPH

**FX — the sound source**

![MORPH: the vector square, the rings dim behind it](img/morph.png "MORPH SOURCE · FX")

Four synthesis methods blend across the XY square. A key **strikes** a note, up to four at once; each dies away by itself, as long as DECAY (on MIX) says, richest and brightest at the strike and plainer as it fades. With DECAY at the top (HOLD) notes stay on until their key is tapped again.

MORPH is an instrument by itself here, and whenever there are no rings. It is heard on this page and while it is being captured. Everywhere else you hear the grains alone, so a sound and its recording do not pile up; MIX brings MORPH back in.

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **X** | Left ↔ right |
| KNOB2 | **Y** | Bottom ↔ top |
| KNOB3 | **COMPLEX** | Harmonic richness |
| KNOB4 | **TONE** | Dark ↔ bright low-pass |
| SELECT | **RING** | Which ring plays and is edited |
| ALGORITHM | **MOVE** | Automatic XY orbit |
| PRESETS | **EXCITE** | Modal ringing time |

### Vector square

| Corner | Sound |
| --- | --- |
| Top left · FM | Frequency modulation |
| Top right · MODAL | Struck, decaying bells |
| Bottom left · ADD | Added harmonics |
| Bottom right · FOLD | Wavefolding |

The white dot is the current mix; the waveform surrounds it. The dotted orbit shows MOVE. Four pips below show the voices. SPREAD (their stereo width) is on MIX.

## Rings and capture

**REC — one press, one new ring**

![A capture: RING 01 filling as the next circle](img/capture.png "REC")

A ring is 1.49 seconds of sound. There is room for three. A capture always makes a new ring; with three, the oldest gives up its place.

| SOURCE | When | What is recorded |
| --- | --- | --- |
| **MORPH** | On MORPH's page, and whenever there are no rings | MORPH alone |
| **SILT 02** | Everywhere else, once a ring exists | SILT's grains of the selected ring alone |

- **No rings yet**: a key on any page but MORPH's strikes a note and captures it at once.
- The capture runs for one ring's length and **ends by itself**. Pressing REC meanwhile does nothing.
- The new ring appears only when it is complete, evened in level, and is then selected. Its number never changes.
- A ring made from SILT starts with PITCH, REVERSE, SCATTER and DETUNE at rest, so they are not applied twice.
- **Nothing sounding** (no notes, or no grains): REC is **armed** and the capture starts with the next sound, so a struck note is caught from its start. REC again takes the arming off.
- **Three rings already**: the oldest ring makes room for the new one, but never the selected ring. The screen names it first: `REC: DROPS 01`.
- Go back to an older ring, change it and press REC: that is a new branch.

### RINGS

**SEL — the rings and where they came from**

![RINGS: three rings, the second selected](img/rings.png "RINGS · SEL")

| Knob | Control | Function |
| --- | --- | --- |
| SELECT | **RING** | Choose a ring |
| KNOB1 | **CARRY** | How much of the selected ring is copied under the next capture |
| KNOB4 | **DELETE** | One step right: arm deleting the selected ring. Two: arm clearing everything |

A delete happens only when **SEL** is pressed while it is armed. Any other button calls it off.

## SILT SHAPE and MOTION

**ENV — how grains differ**

![SILT SHAPE](img/shape.png "SILT SHAPE · ENV")

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **SHAPE** | Struck → bell → swell |
| KNOB2 | **VARY** | Random length, up to ¼–4× SIZE |
| KNOB3 | **SCATTER** | Random octave and interval jumps |
| KNOB4 | **DETUNE** | Up to ±50 cents per grain |
| ALGORITHM | **CLUSTER** | 1–4 simultaneous grains |
| PRESETS | **REVERSE** | Reverse probability |

**LFO — movement**

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **SCAN** | Automatic POSITION movement |
| KNOB2 | **TIMING** | Regular → irregular starts |
| KNOB3 | **LOOP** | The play point going round the ring: OFF still, 50 as recorded, 100 four times as fast |

Every SILT setting on SILT, SHAPE and MOTION, and STEREO on MIX, belongs to the selected ring.

## MIX and REC FX

**ARP — MIX. SEQ — REC FX.**

| Knob | MIX | Function |
| --- | --- | --- |
| KNOB1 | **MIX** | MORPH ↔ SILT balance; starts on SILT alone |
| KNOB2 | **FADE** | MORPH note attack/release |
| KNOB3 | **SPREAD** | MORPH voice stereo width |
| KNOB4 | **STEREO** | Grain stereo placement |
| ALGORITHM | **DECAY** | How long a struck MORPH note rings, 0.1–10 s; at the top, **HOLD** |
| PRESETS | **LEVEL** | Overall output level |

![REC FX](img/recfx.png "REC FX · SEQ")

REC FX shapes what the **next capture** records. It never changes a ring that exists.

| Knob | REC FX | Function |
| --- | --- | --- |
| KNOB1 | **NOISE** | Add hiss (captures noise alone with no notes) |
| KNOB2 | **COLOR** | Low rumble → bright hiss |
| KNOB3 | **CRUSH** | Reduce amplitude resolution |
| KNOB4 | **GATE** | Silence below a threshold |
| ALGORITHM | **DROP** | Random gaps |
| PRESETS | **DAMP** | Darken the capture |

## Patch ideas

Capture one or two MORPH notes as RING 01 first (FX, REC, play them). Values are displayed knob percentages.

| Patch | Settings |
| --- | --- |
| Stable loop | DRIFT OFF, LOOP 50, SPRAY 0: the seed, round and round |
| Slow erosion | DRIFT 20, SPRAY 20: the same loop, a little further off every time |
| Frozen cloud | MOTION: LOOP OFF; SPRAY 60: grains from one place, as before 0.8 |
| Chord cloud | Keys C/G/C; CLUSTER 100, SHAPE 50, SIZE 70, DENSITY 40 |
| Plucked arp | 3–4 keys; CLUSTER 0, TIMING 0, SHAPE 0, SIZE 30, DENSITY 60 |
| Shimmer | SCATTER 40, DETUNE 60, REVERSE 50, SIZE 70, STEREO 100 |
| Rain | TIMING 100, VARY 80, SIZE 20, SPRAY 100, DENSITY 80 |
| Generations | DRIFT 100, then turn PITCH between generations: each ring a step further |
| Worn copy | REC FX: NOISE 30, CRUSH 40, DROP 30; REC from SILT; repeat for more wear |
| Layered | RINGS: CARRY 60; FX, REC, play new notes: the new ring is the old one with MORPH on top |
| Drone | MIX: DECAY to HOLD, MIX 50; FX, latch notes: MORPH stays under the grains |

## Notes

Experimental firmware. Simulator-tested, not yet validated on physical FM-1 hardware. Try the browser before installing. Three rings of 1.49 s; they and all settings live in memory only and are lost at power-off (SAVE is not available). No MIDI input. New grains are skipped when all 16 are in use. The installer can restore stock firmware or switch instruments without overwriting STRATA's saved sound.
