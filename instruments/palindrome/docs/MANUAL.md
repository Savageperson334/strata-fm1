# PALINDROME

**MORPH makes a sound. REC catches it in one of three rings. The rings are scanned.**

PALINDROME 1.0 is three circular recordings, 1.49 seconds each, and a synth to fill them. Every ring has a head going
round it and grains read from the head, as in a granular looper; the keys chop and scrub a ring as a pocket sampler's
do. A ring can also be made from the others, so sounds pass from circle to circle.

Nothing here has run on a real FM-1 yet.

![The three rings](img/rings.png "SCAN: three rings, the selected one bright")
![MORPH](img/morph.png "MORPH: the square, the path of a note, its loudness over a ring")

## Play it

1. **Tap a key.** With no ring yet, MORPH strikes a note one ring long, ring 1 catches it from its first sample, and it
   starts going round.
2. **Scan it.** KNOB1 POSITION scrubs through it; ENV freezes it, LFO reverses it; KNOB3 DENSITY thins it into grains
   or thickens it into a cloud.
3. **Hold keys.** Each white key is one of sixteen places round the ring, the black keys the places between. A held key
   sounds the ring from there, with PLAY on or off.
4. **Another ring.** Turn SELECT to ring 2 and tap a key: an empty ring catches the note. (Over a full ring: hold REC
   and tap a key, from any page.)
5. **From each other.** Press EDIT, turn SELECT to ring 3, press REC: what you hear of rings 1 and 2 (and MORPH) is
   resampled into ring 3.

## Buttons

| Button | Does |
| --- | --- |
| **EDIT**, **HOME** | SCAN page. HOME held a second empties the selected ring |
| **FX** | MORPH page: the keys strike its notes |
| **ARP** | MIX page |
| **SEL** | The keys on SCAN and MIX: SLICE (places round the ring) or PITCH (the ring transposed; the C as recorded). Lit: PITCH |
| **PLAY** | The rings going round, on and off |
| **REC** + a key | Held, with a key: that MORPH note is caught in the selected ring at once, from any page |
| **REC** (tap) | On MORPH: armed (blinking); the next note is caught in the selected ring. On SCAN or MIX: everything heard but the selected ring is resampled into it, now |
| **ENV** | FREEZE: the selected ring's head stops (lit); again, it goes on as it was |
| **LFO** | REVERSE: the selected ring runs the other way (lit while backward) |
| **OCT− / OCT+** | The keys an octave down or up |
| **GLO** (hold) | The guide on the screen |
| SAVE, SEQ | Nothing |

**SELECT** chooses the ring on every page: the one the SCAN knobs set, the keys play and REC fills. RING 1 is the
innermost circle. On SCAN and MIX, while the selected ring is empty, a key strikes MORPH and the ring catches the note.
Recording into a ring replaces what was in it; nothing is saved at power-off. A fast turn of a knob goes four times
further.

## SCAN — EDIT

Each ring keeps its own settings.

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **POSITION** | Where the grains start, round and round: turn it to scrub |
| KNOB2 | **SIZE** | Each grain, 20 ms to 1 s |
| KNOB3 | **DENSITY** | Grains overlapping: x0.25 (a grain now and then) to x5 (a cloud). About x2 is the plain recording |
| KNOB4 | **PITCH** | Two octaves down to two up, in semitones |
| ALGORITHM | **SPEED** | The head, a step a detent: -2x, -1x, -0.5x, -1/4, STILL, 1/4, 0.5x, 1.0x (as recorded), 1.5x, 2x |
| PRESETS | **SPREAD** | Grains scattered round the head and in time |

## MORPH — FX

A note lasts exactly one ring. It plays MORPH as it is set now, and behind it hidden envelopes move it: X and Y travel
toward their mirror image and back (a palindrome), the sound starts brighter and ends plainer and darker. MOVE says how
far. Because a note is silent again by the ring's end, a caught note loops without a seam, and the same key on the same
settings sounds the same way.

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **X** | FM and ADD on the left, MODAL and FOLD on the right |
| KNOB2 | **Y** | ADD and FOLD at the bottom, FM and MODAL at the top |
| KNOB3 | **COMPLEX** | How rich each way of making sound is |
| KNOB4 | **TONE** | Dark to bright |
| ALGORITHM | **MOVE** | How far the hidden envelopes carry a note. 0: it stays where the knobs are |
| PRESETS | **SHAPE** | A note's loudness over its ring: a pluck, a bell, a swell |

The screen draws the path a note will take (from the knobs' place toward the mirror point) and, underneath, its
loudness over one ring with the newest note's place in it.

## MIX — ARP

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1..3 | **RING 1..3** | Each ring's level |
| KNOB4 | **MORPH** | MORPH's own level as you play it |
| ALGORITHM | **DAMP** | The rings darker |
| PRESETS | **WIDTH** | The rings spread across the stereo field |

## Limits

Three rings of 1.49 s at 22 kHz, up to six grains each, four MORPH notes. Rings live in RAM only. A capture's level is
evened (its loudest sample to about 70%); a capture with nothing in it leaves the ring empty.
