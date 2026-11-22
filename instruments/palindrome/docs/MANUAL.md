# PALINDROME

An experimental FM-1 instrument in two halves: **MORPH makes sound; SILT records it and plays it back in grains.**
Both keep running whichever one the knobs are on. It's custom firmware, installed at your own risk, and it has not
been tested on a real FM-1 yet. Try the browser version first.

![The screen: MORPH's square in the middle of SILT's memory ring](img/morph.png)

## Quick start

1. Turn MASTER up. Tap a few keys (up to four notes latch; tap one again to let it go).
2. Turn KNOB1 and KNOB2 to move the white spot around the square. Each corner is a different way of making sound.
3. Wait two seconds for the ring to fill, then tap EDIT. The knobs and keys are now SILT's.
4. Tap REC once to FREEZE what's recorded. Tap PLAY to start the grains.
5. Tap two or three keys. The grains now play those pitches in turn.
6. Turn KNOB1 (where the grains start), KNOB2 (how long they are) and KNOB3 (how many).
7. Tap FX to go back to MORPH. The grains carry on.

## Finding your way

Two buttons choose which half the knobs and keys belong to. Three more give the knobs another set while held.

| Button | Knobs | Keys |
| --- | --- | --- |
| Tap **FX** | MORPH's | Latch MORPH's notes |
| Tap **EDIT** | SILT's | Latch the grains' pitches |
| Hold **EDIT** | The rest of SILT's | |
| Hold **REC** | The recording's | |
| Hold **SEL** | MIX, FADE and LEVEL, for both halves | |

The screen always says which set you're on: the corners name the buttons, and the eight boxes at the foot name what
each knob sets right now. The boxes sit as the knobs do on the FM-1: MASTER and SELECT over PRESETS and ALGORITHM on
the left, KNOB1 and KNOB2 over KNOB3 and KNOB4 on the right. Turn a knob and its box turns white and shows the
number. Every value runs 0 to 100.

A hint appears across the middle of the screen until you've done the thing once.

---

## Part 1: MORPH, which makes the sound

MORPH is the half that makes sound. It blends four ways of making a tone, and whatever it plays is what SILT
records.

### MORPH on the screen

- **The square** in the middle is MORPH. Its four corners are four ways of making sound, each named and lit as much
  as you hear it.
- **The white spot** is where the sound is now, and the **loop around it** is the sound's own wave.
- **The dotted circle** is the path MOVE carries the spot on.
- **The four pips** under the square are the voices, as far apart as SPREAD has them.
- **FX: MORPH** in the top left corner is lit while the knobs are MORPH's.

| Corner | Name | Sound |
| --- | --- | --- |
| Top left | FM | FM |
| Top right | MODAL | Struck, bell-like tones that decay |
| Bottom left | ADD | Added harmonics |
| Bottom right | FOLD | Wavefolding |

### MORPH's keys

Keys latch notes, up to four, and unlatch them one by one. OCT− and OCT+ move the keys; notes already playing keep
their pitch.

### MORPH's knobs — tap FX

| Knob | Control | What it does |
| --- | --- | --- |
| KNOB1 | X | The spot left and right |
| KNOB2 | Y | The spot up and down |
| KNOB3 | COMPLEX | How rich each corner's sound is |
| KNOB4 | TONE | Low-pass: dark to bright |
| SELECT | SPREAD | How far apart the voices sit in stereo |
| ALGORITHM | MOVE | Carries the spot round a circle by itself |
| PRESETS | EXCITE | How long the struck corner rings |

---

## Part 2: SILT, which records it and plays grains

SILT is the half that records. It keeps the last second and a half of MORPH in a loop, and plays that loop back as
grains: short pieces of it, up to 16 at once, each with its own place, length, pitch and direction.

![SILT: the memory frozen, grains riding the ring](img/silt.png)

### SILT on the screen

- **The ring** is the memory, drawn as the loop it is. The recorded wave stays where it was written.
- **The tick across the ring** is the write head going round and writing over it.
- **The mark outside the ring** is where grains start (POSITION). The **dotted arc** beside it is how far they
  scatter (SPRAY).
- **Each grain is a spot riding the ring** with a short tail. Grains playing forward ride the outside; grains
  playing backward ride the inside.
- **The numbers under the square** are the keys held for the grains, in semitones from the recorded pitch: `0 +7
  +12` is the recorded pitch, a fifth above it and an octave above it.
- **EDIT: SILT** in the top right corner is lit while the knobs are SILT's. **REC:** in the bottom left says how the
  memory is being written, and **PLAY: GRAINS** in the bottom right is lit while the grains run.

### Recording — tap REC

REC steps through four ways of writing the memory. The head on the ring shows which one you're in.

| REC mode | Head | What happens |
| --- | --- | --- |
| LIVE | One white tick, moving | MORPH is recorded over the oldest sound |
| FREEZE | Two short marks, still | Nothing is written; the memory is safe |
| OVERDUB | Two ticks, moving | New sound is mixed onto what's there |
| RESAMPLE | A tick with a block, moving | The grains themselves are recorded back in |

The head's tick turns **dim** whenever GATE or DROP has lifted it off the memory, and **specks trail it** while
NOISE is going in.

### Playing grains — PLAY and the keys

- **PLAY** starts and stops a steady stream of grains. PLAY is lit while it runs.
- **A key** holds a pitch for the grains. The fifth white key from the left (C) is the recorded pitch, and each key
  from there is a semitone. Tap a held key again to let it go. OCT− and OCT+ move the keys an octave.
- **Up to four keys** can be held. The grains take them in turn, so three held keys become an arpeggio. Turn CLUSTER
  up (hold EDIT) and they start together as a chord.
- **Tapping a key plays its grains at once,** whether PLAY is running or not. With PLAY stopped you play the grains
  by hand.
- With no key held, the grains play at the recorded pitch, moved by the PITCH knob.

### SILT's knobs — tap EDIT

| Knob | Control | What it does |
| --- | --- | --- |
| KNOB1 | POSITION | Where grains start, from the newest sound to the oldest |
| KNOB2 | SIZE | Grain length, 20–500 ms |
| KNOB3 | DENSITY | 1–40 starts a second |
| KNOB4 | SPRAY | How far grains scatter from POSITION |
| SELECT | PITCH | Two octaves down to two octaves up, on top of the keys |
| ALGORITHM | REVERSE | The chance a grain plays backward |
| PRESETS | SHAPE | Each grain's loudness over its length: 0 struck (in fast, out slow), 50 a bell, 100 a swell |

### More of SILT's knobs — hold EDIT

These set how the grains differ from one another. The top right corner says EDIT: SILT MORE while EDIT is held.

| Knob | Control | What it does |
| --- | --- | --- |
| KNOB1 | TIMING | When grains start: 0 an even pulse, 100 scattered, with short gaps and long ones |
| KNOB2 | VARY | Each grain's length: all the same at 0, anywhere from a quarter to four times SIZE at 100 |
| KNOB3 | SCATTER | The chance a grain jumps an octave up or down; higher adds fifths and two octaves |
| KNOB4 | DETUNE | Each grain a little off pitch, up to half a semitone either way: thickens the sound |
| SELECT | STEREO | How wide the grains are scattered left to right |
| ALGORITHM | CLUSTER | Grains started together: 1, 2, 3 or 4 (a quarter of the knob each), each on the next held key |
| PRESETS | SCAN | Moves POSITION back and forth by itself |

![Holding EDIT: the rest of SILT's knobs, two keys held](img/more.png)

### The recording's knobs — hold REC

Hold REC and the knobs set what goes into the memory. Letting go after a turn doesn't change the REC mode.

| Knob | Control | What it does |
| --- | --- | --- |
| KNOB1 | NOISE | Hiss recorded along with the sound. With no notes held it records noise alone |
| KNOB2 | COLOR | The hiss's tone: a low rumble, through even, to a bright fizz |
| KNOB3 | CRUSH | Fewer and fewer levels: grit, then a few hard steps |
| KNOB4 | GATE | Only records while the sound is louder than this, so quiet hiss and tails stay out |
| SELECT | DROP | The head lifts off at random, leaving holes where the old sound shows through |
| ALGORITHM | DAMP | Dulls what is recorded |
| PRESETS | FEEDBACK | How much of the old sound survives in OVERDUB and RESAMPLE |

- All of this is printed into the memory as it is written. FREEZE writes nothing, so it changes nothing there.
- In OVERDUB and RESAMPLE the noise and crush pile up with every pass. FEEDBACK sets how fast.
- For a clean loop: NOISE, CRUSH and DROP at zero, GATE a little up.

![Holding REC: the recording's knobs](img/rec.png)

### Things to try in SILT

Start each one with a note or two recorded and REC on FREEZE.

| Sound | Keys | Knobs |
| --- | --- | --- |
| **Chord cloud** | C, the G above it, the next C | CLUSTER 100, SHAPE 50, SIZE 70, DENSITY 40 |
| **Plucked arpeggio** | Three or four keys | CLUSTER 0, TIMING 0, SHAPE 0, SIZE 30, DENSITY 60 |
| **Shimmer** | None, or one | SCATTER 40, DETUNE 60, REVERSE 50, SIZE 70, STEREO 100 |
| **Rain** | None | TIMING 100, VARY 80, SIZE 20, SPRAY 100, DENSITY 80 |
| **Played by hand** | PLAY stopped; tap keys | SHAPE 0, SIZE 60, CLUSTER 50 |

---

## Both halves

### MIX, FADE and LEVEL — hold SEL

| Knob | Control | What it does |
| --- | --- | --- |
| KNOB1 | MIX | MORPH against the grains |
| KNOB2 | FADE | How slowly MORPH's notes come and go |
| PRESETS | LEVEL | Overall level |

The other knobs set nothing while SEL is held, and their boxes are empty.

### The other buttons

- **HOME, held for a second,** lets every note and every grain key go and stops the grains. A tap only shows the
  version, so you can't clear everything by accident.
- **SAVE** does nothing yet: the sound and the memory are lost at power-off.

## Limits

No saved patches, no MIDI in, and the memory is short. When 16 grains are already playing, new ones are skipped
until there's room. Installing PALINDROME leaves STRATA's saved sound alone, and either installer can put the other
firmware (or M-VAVE's own) back.
