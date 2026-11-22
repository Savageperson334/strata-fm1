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

## The other buttons

Three buttons do something besides PLAY and the octave buttons; the rest do nothing in STRATA.

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

Once a recipe sounds right, press **SAVE** to keep it.

---

## MIDI

STRATA takes MIDI in over USB and the TRS jack, on any channel.

- **Notes** play voices while held, like a normal synth. The sustain pedal (CC 64) holds them.
- **CC 123** (all notes off) releases the MIDI voices.
- Keys tapped on the FM-1 go out over USB as notes, so a computer can record them.
- **CC 20 to 33** set the fourteen controls:

| CC | Control | CC | Control |
| --- | --- | --- | --- |
| 20 | TONE | 27 | DEPTH |
| 21 | RATIO | 28 | BEAT |
| 22 | MOTION | 29 | FADE |
| 23 | SPACE | 30 | DECAY |
| 24 | FILTER | 31 | RESO |
| 25 | DRIFT | 32 | AIR |
| 26 | GRIT | 33 | SUB |

---

## Troubleshooting

| Problem | Fix |
| --- | --- |
| No sound | Turn MASTER up and tap a key. If FADE is long, voices take that long to come in: hold SEL and turn K3 down. |
| Sound is muffled | FILTER is low: turn SELECT up. |
| The installer can't find the FM-1 | Use Chrome or Edge, try another USB cable, and close other apps or pages that use MIDI. |
| The knobs do M-VAVE things | STRATA isn't installed yet: the screen should say STRATA at the top. |
| "OVERLOAD" on the screen | The sound took too long to make, so the oldest voice was let go. Use fewer voices or less AIR. |
| Want the original sound back | Hold HOME for a second. |
| Want M-VAVE's firmware back | Use the installer's "Back to the stock firmware" link. |

M-VAVE and FM-1 are trademarks of their owners; STRATA isn't affiliated with M-VAVE.
