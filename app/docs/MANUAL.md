# STRATA

STRATA turns the M-VAVE FM-1 into a drone and texture synth with one mode and no menus. Tap a key and a voice fades
in and keeps going; tap it again and it fades away. Every knob changes the sound while it plays.

It's custom firmware, installed at your own risk. The installer can always put M-VAVE's own firmware back.

![STRATA playing three voices: the wave on top, the notes F2 C3 E3 lit below, and the seven controls with their values](img/screen-playing.png "STRATA playing")

---

## Installing

It takes about a minute from a computer, with nothing to download.

1. On a computer, open the [installer](../install/) in **Chrome or Edge**. Safari and Firefox can't talk to the FM-1.
2. Plug the FM-1 in with a USB cable that carries data (some charging cables don't), and switch it on.
3. Click **Connect** and allow MIDI access when the browser asks.
4. Click **Install**. Keep the cable in and the FM-1 on until the page says it's done.
5. The FM-1 restarts and the screen says **STRATA** at the top.

**To go back to M-VAVE's firmware:** use the installer's "Back to the stock firmware" link, or M-VAVE's M-UPGRADE
app from m-vave.com.

You can also [try STRATA in the browser](../emu/) first, with sound. It's the same code, so you don't need an FM-1.

---

## Quick start

You'll have a drone going in under 30 seconds.

1. Turn **MASTER** (volume) up.
2. Tap a low key, then a key seven notes above it. Two voices fade in and keep going.
3. Turn **K4** for more reverb, and **K1** to make it brighter.
4. Press **PLAY** to fade everything out. Press it again to bring the same notes back.

---

## Playing

Keys start and stop voices, and you don't hold them down.

- **Tap a key** to start a voice at that note. It fades in and keeps sounding after you let go.
- **Tap the same key again** to let that voice fade out.
- Up to **six voices** play at once. A seventh replaces the oldest one, with a short crossfade.
- **PLAY** fades every voice out and remembers the chord. Press it again to bring that chord back.
- **OCT− / OCT+** move the keys down or up an octave. Voices already playing stay where they are.
- **MASTER** is the volume.

The keys that are sounding light up. A key that's still fading out glows dimly, and PLAY glows green while anything
is sounding.

---

## The screen

The screen has three parts, top to bottom.

- **Top:** STRATA and the keys' octave (OCT −1 to start). Messages show here for a moment, like "C3 IN" or "SAVED".
  While SEL is held it says **MORE**.
- **Middle:** the sound's wave, with a small dot sliding underneath that follows the slow LFO (MOTION). Below that
  are six boxes, one per voice, showing the note. A box fills as loud as that voice is.
- **Bottom:** the seven controls in two rows, laid out like the knobs: K1, K2, K3, K4 on top, then SELECT, ALGO,
  PRESET. Each box shows the control's name, its value, which knob it's on, and a bar. The box you're turning
  lights up.

![The layer under SEL: DEPTH, BEAT, FADE, DECAY, RESO, AIR and SUB](img/screen-layer2.png "SEL held")

---

## The knobs

All seven knobs change the sound live, and holding **SEL** gives you seven more on the same knobs. Turn slowly for
fine steps, or quickly to cover the whole range.

**Main controls** (just turn the knob):

| Knob | Control | What it does | Range |
| --- | --- | --- | --- |
| K1 | **TONE** | Brightness of the FM: pure sine to glassy and buzzy | 0–100 |
| K2 | **RATIO** | The FM ratio. Whole numbers sound harmonic; in between turns to bells and clangs | 0.5–7 |
| K3 | **MOTION** | Speed of the slow LFO that makes the sound breathe | 0.01–3 Hz |
| K4 | **SPACE** | How much reverb | 0–100 |
| SELECT | **FILTER** | Low-pass cutoff: dark and muffled to fully open | 40 Hz–14 kHz |
| ALGO | **DRIFT** | Detunes each voice, lets pitch and level wander, widens the stereo | 0–100 |
| PRESET | **GRIT** | Feedback in the FM: smooth at 0, rough and noisy near the top | 0–100 |

**Under SEL** (hold SEL and turn; let go to get back):

| Knob | Control | What it does | Range |
| --- | --- | --- | --- |
| K1 | **DEPTH** | How far MOTION moves the tone and the filter | 0–100 |
| K2 | **BEAT** | A slow beating shimmer between the voice's two halves | 0–3 Hz |
| K3 | **FADE** | How long voices take to fade in and out | 0.1–20 s |
| K4 | **DECAY** | How long the reverb rings, up to nearly endless | 1.5–60 s |
| SELECT | **RESO** | Filter resonance: a singing peak at the cutoff | 0–100 |
| ALGO | **AIR** | Breathy noise, tuned to each voice's pitch | 0–100 |
| PRESET | **SUB** | A sine one octave below each voice | 0–100 |

If you're not sure which physical knob is which, turn one and watch which box lights up on the screen.

---

## The arpeggiator

The arpeggiator plays the voices you've started one at a time, each swelling up while the others sink back. With
the slow default it sounds like a ripple through the chord, not a fast pattern.

- **Tap ARP** to switch it on or off. ARP lights up while it's on, and the screen says ARP at the top.
- **Hold ARP and turn K1 to K4** to set it up. The screen shows ARP SETTINGS. Letting go of ARP after a turn doesn't
  switch it off.
- The voice on the step has a white frame around its box on the screen.
- With only one voice playing, the arp pulses it on and off.
- Tap keys in and out while it runs; the arp picks up the change on the next step.

![ARP held: RATE, SHAPE, ORDER and FLOOR on K1 to K4, the C3 voice framed as the step](img/screen-arp.png "ARP settings")

| Knob (ARP held) | Control | What it does | Range |
| --- | --- | --- | --- |
| K1 | **RATE** | How fast it steps | 0.1–8 steps a second |
| K2 | **SHAPE** | Sharp steps at 0, slow swells that blend into each other near 100 | 0–100 |
| K3 | **ORDER** | UP (low to high), DOWN, UP-DN (up and back), RND (random) | 4 choices |
| K4 | **FLOOR** | How loud the voices off the step stay: silent at 0, nearly full at 100 | 0–100 |

SAVE keeps whether the arp is on and its settings.

---

## The beat

The beat plays a drum groove under the drone: a synthesized kick, snare, hats, and a percussion voice tuned an octave
above your lowest note. The drone can duck under the kick, so the whole sound pumps in time.

- **Tap SEQ** to start or stop the beat. It always starts from the first step. SEQ flashes on the quarter notes
  while it runs, and sixteen small steps across the screen show where it is.
- **Hold SEQ and turn any of the seven knobs** to set it up. The screen shows BEAT SETTINGS. Letting go of SEQ after
  a turn doesn't stop it.
- While the beat runs, **the arp locks to its tempo**: RATE becomes a note length (1/1, 1/2, 1/4, 1/8, 1/16)
  instead of steps a second.

![SEQ held: TEMPO, GROOVE, SWING, LEVEL, KIT, ROOM and DUCK, the steps across the screen](img/screen-beat.png "Beat settings")

| Knob (SEQ held) | Control | What it does | Range |
| --- | --- | --- | --- |
| K1 | **TEMPO** | Beats per minute | 60–160 bpm |
| K2 | **GROOVE** | The pattern, one per click: PULSE, DUB, BREAK, HALF, TOMS, TICK, HEART, DRIVE | 8 grooves |
| K3 | **SWING** | Straight at 50 %, heavy shuffle at 75 % | 50–75 % |
| K4 | **LEVEL** | How loud the drums are | 0–100 |
| SELECT | **KIT** | Dark and low with long tails at 0, bright and snappy at 100 | 0–100 |
| ALGO | **ROOM** | How much of the drums goes into the reverb (heard as much as SPACE lets it) | 0–100 |
| PRESET | **DUCK** | How far the drone dips on each kick: none at 0, deep pumping at 100 | 0–100 |

The grooves:

| Groove | What it is |
| --- | --- |
| PULSE | Four on the floor, hats off the beat |
| DUB | One drop: kick and snare together on beat three |
| BREAK | A broken breakbeat |
| HALF | Half time, snare on three |
| TOMS | Kick and tuned toms rolling |
| TICK | Minimal: kick and small ticks |
| HEART | A slow lub-dub |
| DRIVE | Driving: kick on every beat, busy hats |

SAVE keeps whether the beat is on and its settings.

---

## The other buttons

Three more buttons do something besides PLAY, ARP, SEQ and the octave buttons; the rest do nothing in STRATA.

- **REC = MUTATE.** Nudges the seven main controls a little in random directions. Press it a few times to wander
  somewhere new.
- **HOME, held for one second,** resets the sound to how it started. A quick tap does nothing, so you can't hit it
  by accident. It doesn't stop the voices.
- **SAVE** keeps the sound, the octave and the chord that's playing. They come back after you switch off, and PLAY
  starts the saved chord. SAVE stays lit while there's something new to save.

---

## Recipes

These are starting points; set the values you see on the screen, then play the notes listed. Anything not listed
can stay where it is.

| Sound | Main controls | Under SEL | Play |
| --- | --- | --- | --- |
| **Deep organ** | TONE 20, RATIO 1.00, FILTER 800, SPACE 40, DRIFT 20 | SUB 60, FADE 2 s | A low note and the note seven keys up |
| **Glass bells** | TONE 60, RATIO between 2.50 and 3.00, SPACE 70 | BEAT 0.5 Hz, DECAY 30 s, FADE 3 s | Three or four notes spread across the keys |
| **Dark storm** | GRIT 60, FILTER 300, MOTION 0.05 Hz, SPACE 80 | RESO 60, AIR 50, DEPTH 80, DECAY 60 s | One or two low notes, OCT− pressed once |
| **Shimmer pad** | TONE 40, RATIO 2.00, DRIFT 60, MOTION 0.20 Hz, SPACE 60 | BEAT 1.0 Hz | A chord of three notes |
| **Music box ripple** | TONE 45, RATIO 3.00, SPACE 70 | DECAY 20 s; ARP on: RATE 3/s, SHAPE 10, ORDER UP-DN, FLOOR 0 | Four notes going up the keys |
| **Night drive** | TONE 35, RATIO 2.00, FILTER 1.5k, SPACE 50 | ARP on: RATE 1/8, SHAPE 20; SEQ on: TEMPO 118, DRIVE, DUCK 60 | A minor chord of three notes |
| **Dub space** | TONE 25, RATIO 1.00, SPACE 80 | DECAY 30 s; SEQ on: TEMPO 72, DUB, SWING 60 %, ROOM 70, DUCK 40 | One low note and its fifth |
| **Slow tide** | TONE 25, RATIO 1.00, FILTER 1.0k, SPACE 60 | SUB 50; ARP on: RATE 0.3/s, SHAPE 90, FLOOR 30 | A low chord of three notes |

Once a recipe sounds right, press **SAVE** to keep it.

---

## MIDI

STRATA takes MIDI in over USB and the TRS jack, on any channel.

- **Notes** play voices while held, like a normal synth. The sustain pedal (CC 64) holds them.
- **CC 123** (all notes off) releases the MIDI voices.
- Keys tapped on the FM-1 go out over USB as notes, so a computer can record them.
- **CC 20 to 44** set the twenty-five controls:

| CC | Control | CC | Control |
| --- | --- | --- | --- |
| 20 | TONE | 27 | DEPTH |
| 21 | RATIO | 28 | BEAT |
| 22 | MOTION | 29 | FADE |
| 23 | SPACE | 30 | DECAY |
| 24 | FILTER | 31 | RESO |
| 25 | DRIFT | 32 | AIR |
| 26 | GRIT | 33 | SUB |
| 34 | RATE (arp) | 36 | ORDER (arp) |
| 35 | SHAPE (arp) | 37 | FLOOR (arp) |
| 38 | TEMPO (beat) | 42 | KIT (beat) |
| 39 | GROOVE (beat) | 43 | ROOM (beat) |
| 40 | SWING (beat) | 44 | DUCK (beat) |
| 41 | LEVEL (beat) | | |

---

## Troubleshooting

| Problem | Fix |
| --- | --- |
| No sound | Turn MASTER up and tap a key. If FADE is long, voices take that long to come in: hold SEL and turn K3 down. |
| Sound is muffled | FILTER is low: turn SELECT up. |
| The installer can't find the FM-1 | Use Chrome or Edge, try another USB cable, and close other apps or pages that use MIDI. |
| The knobs do M-VAVE things | STRATA isn't installed yet: the screen should say STRATA at the top. |
| "OVERLOAD" on the screen | The sound took too long to make, so the oldest voice was let go. Use fewer voices or less AIR. |
| The voices pulse or take turns | The arp is on: tap ARP to switch it off. |
| Drums are playing | The beat is on: tap SEQ to stop it. |
| The drone pumps or dips | That's DUCK: hold SEQ and turn PRESET down. |
| Want the original sound back | Hold HOME for a second. |
| Want M-VAVE's firmware back | Use the installer's "Back to the stock firmware" link. |

M-VAVE and FM-1 are trademarks of their owners; STRATA isn't affiliated with M-VAVE.
