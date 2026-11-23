# PALINDROME

**MORPH makes the seed. SILT is the instrument.** SILT preserves three recordings as concentric rings, lets them play independently, and captures their transformations as new generations.

[Play the emulator](../emu/) · [Install](../install/) · [Source](https://github.com/Savageperson334/strata-fm1/tree/main/instruments/palindrome)

## Start here

1. Turn up **MASTER**. Tap a key on SILT's empty home screen. MORPH strikes a note, automatically records **RING 01**, and SILT starts playing it.
2. Turn **KNOB1** (position) or **KNOB2** (grain size). Change **ALGORITHM** to choose a playback mode.
3. Press **REC** to record that processed sound into **RING 02**.
4. Raise **KNOB4 / WEAVE** to combine the selected ring with older material. Turn **SELECT** to change the ring you are editing.
5. Open **FX** to play MORPH directly or inject another sound. DRIFT is optional on **LFO**; it starts off.

One press opens a page. Holding a button is **not** required to reach any musical control (GLO held only opens an optional on-device guide).

## Two halves, different jobs

| | **SILT — EDIT / HOME** | **MORPH — FX** |
| --- | --- | --- |
| Job | Play, manipulate, blend and resample rings | Generate fresh source audio |
| Keys | Pitch-latch grains (up to four pitches) | Strike up to four synth notes |
| KNOB1 | **POSITION** — where to read | **X** — vector left/right |
| KNOB2 | **SIZE** — 20–500 ms grains | **Y** — vector down/up |
| KNOB3 | **DENSITY** — starts/sec | **COMPLEX** — harmonic richness |
| KNOB4 | **WEAVE** — introduce other rings | **TONE** — brightness |
| SELECT | **RING** — choose recording | **RING** — choose recording |
| ALGORITHM | **MODE** — Loop/Cloud/Pulse/Fracture | **MOVE** — automatic XY orbit |
| PRESETS | **PITCH** — transpose grains | **EXCITE** — modal resonance decay |

![SILT's three concentric rings](img/silt.png "SILT")

![MORPH's vector synth](img/morph.png "MORPH")

### What the four SILT modes mean

| Mode | Behavior |
| --- | --- |
| **LOOP** | Smooth, coherent reading along the loop; limited random scatter |
| **CLOUD** | Overlapping granular texture (closest to the original SILT behavior) |
| **PULSE** | Short, separated, regularly timed grain events |
| **FRACTURE** | Short broken fragments with position jumps, pitch wander and additional reversals |

Each ring keeps its own **mode, grain settings and playhead motion**. WEAVE is the selected ring's blend control: at 0, only that ring starts grains; turning it up lets the other recordings spawn their own grains too. All three still share **sixteen total grain voices**, so WEAVE doesn't triple the processing budget. The currently selected ring is the one the knobs edit; rings you aren't editing continue advancing their playheads.

## The three rings

Each ring holds **1.49 seconds of mono audio**. The oldest recording appears toward the center; later generations appear farther out. A selected ring is brighter. Moving dots show the grains and their rings.

**REC** captures one ring's duration from the current source, then automatically selects the new ring. On the FX page, the source is MORPH. Elsewhere, it is SILT's processed audio—including WEAVE contributions. A capture runs to completion; tapping REC again while it is running doesn't create another ring. If there's no sound, REC arms until the next audible event.

**SEL** opens the ring browser. Turn SELECT to browse, and use these controls:

| Control | Rings page action |
| --- | --- |
| **KNOB1 / CARRY** | Keep some selected-ring audio under the next capture |
| **KNOB2 / LOCK** | Turn right to lock a ring; left to unlock |
| **KNOB4 / DELETE** | Arm a confirmed delete or clear-all; SEL confirms |

**A locked ring cannot be automatically replaced.** If all three are full, manual REC can recycle only the oldest **unlocked, unselected** recording, which is named on screen in advance. If none qualifies, REC refuses without destroying anything. Deliberate delete still requires confirmation. DRIFT never recycles a full ring.

## SILT detail pages

Every page is selected with **one press**; SELECT continues to choose the ring.

| Knob | **ENV — SILT SHAPE** | **LFO — SILT MOTION** |
| --- | --- | --- |
| KNOB1 | SHAPE — struck to swell envelope | SCAN — automatic position motion |
| KNOB2 | VARY — random grain duration | TIMING — regular to irregular starts |
| KNOB3 | SCATTER — jumps in intervals/octaves | LOOP — playhead speed: off to four times |
| KNOB4 | DETUNE — individual grain tuning | SPRAY — grain position randomness |
| ALGORITHM | CLUSTER — one to four simultaneous grains | DRIFT — automatic new-ring generation |
| PRESETS | REVERSE — reverse probability | — |

**DRIFT starts OFF.** Increase it on LFO to create successive generations during unattended SILT playback. The engine waits while you interact or browse RINGS, and pauses when all three ring slots are occupied. Unlike a manual REC, DRIFT will never delete an existing recording to make room.

## Shared sound and recording effects

**ARP** opens MIX (the button's firmware-specific role); **SEQ** opens REC FX. Screen labels indicate the active page.

| Knob | **ARP — MIX** | **SEQ — REC FX** |
| --- | --- | --- |
| KNOB1 | MIX — MORPH ↔ SILT | NOISE — inject hiss into next capture |
| KNOB2 | FADE — MORPH attack/release | COLOR — noise tone |
| KNOB3 | SPREAD — MORPH stereo width | CRUSH — recording bit-depth effect |
| KNOB4 | STEREO — grain panning | GATE — suppress weak recorded audio |
| ALGORITHM | DECAY — MORPH strike length / HOLD | DROP — capture gaps |
| PRESETS | LEVEL — overall level | DAMP — recording lowpass |

REC FX affects **new recordings**, not the PCM of an older ring.

Other controls: **PLAY** toggles automatic grains (the keys can trigger grains even with PLAY off); **OCT−/+** shifts note range; **HOME** returns to SILT without erasing anything; **GLO** held shows an optional button reference; **SAVE** reports unavailable. A key on the empty SILT screen injects a MORPH note and captures the first ring automatically.

## Patch ideas

| Patch | Try this |
| --- | --- |
| **Tape loop** | MODE LOOP, WEAVE 0; MOTION: LOOP 50, SPRAY 0 |
| **Ambient cloud** | MODE CLOUD, SIZE 70, DENSITY 55, ENV SHAPE 60 |
| **Beat fragments** | MODE PULSE, SIZE 20, DENSITY 45; tap several pitch keys |
| **Broken machine** | MODE FRACTURE, LFO SPRAY 80, ENV SCATTER 55 |
| **Two generations** | Record RING 01, change its size/pitch, REC RING 02, then WEAVE 70 |
| **Preserved source** | Lock RING 01 in SEL; use 02 and 03 for destructive experiments |
| **Autonomous evolution** | With room free, LFO DRIFT 30, then leave the device alone |

**Limits:** Three rings, 1.49 seconds each, sixteen total grains. Audio and settings remain in RAM and disappear on power-off. No MIDI recording or persistent projects. This is experimental firmware; native builds and device audio performance need verification before flashing. The installer can restore stock M-VAVE firmware.
