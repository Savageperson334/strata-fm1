# PALIMPSEST 0.1

An independent experimental FM-1 instrument: **MORPH makes sounds; SILT takes them apart.** Both engines continue running when the panel changes modes.

## Quick start

1. Turn MASTER up. Tap FX to enter MORPH, and tap a few keys (up to four latched notes).
2. K1 and K2 navigate the four-engine vector space. K3 changes harmonic complexity.
3. Wait two seconds for audio memory to fill. Tap EDIT for SILT.
4. Tap REC once to FREEZE the recording. Tap PLAY to start the grain cloud.
5. Turn grain size, density, position and pitch. Tap a key in SILT to jump to a buffer position and burst grains.
6. Tap FX to return to synthesis; SILT continues playing. Tap HOME to release all notes and stop new grains.

## MORPH — FX

| Knob | Control |
| --- | --- |
| K1 | X vector position |
| K2 | Y vector position |
| K3 | Harmonic complexity |
| K4 | Tone / lowpass |
| SELECT | Voice stereo spread |
| ALGO | Autonomous vector orbit |
| PRESET | Modal decay / excitation |

Top left FM; top right decaying inharmonic modal oscillators; bottom left additive; bottom right wavefolding. The modal corner is struck on note-on. The keys latch and can be unlatched individually. OCT buttons move the keyboard range; existing voices retain their pitch.

## SILT — EDIT

| Knob | Control |
| --- | --- |
| K1 | Position (newest to oldest) |
| K2 | Grain length, 20–300 ms |
| K3 | Density, 1–40 grains/sec |
| K4 | Position spray |
| SELECT | Pitch, −24 to +24 semitones |
| ALGO | Reverse probability |
| PRESET | Feedback retention (overdub/resample) |

The 16-bit mono ring stores about 1.49 seconds at 22.05 kHz. Grains are stereo-panned and linearly interpolated. Up to twelve overlapping grains. A lit PLAY indicates continuous grain generation; keys trigger additional bursts.

REC cycles LIVE → FREEZE → OVERDUB → RESAMPLE → LIVE. LIVE records MORPH. FREEZE protects the captured audio. OVERDUB mixes new sound into existing memory. RESAMPLE writes a bounded mix of processed grains and MORPH. Repeated processing deliberately changes the sound.

## SEL — shared controls

MIX, FADE, SCAN (automatic buffer movement), WINDOW (grain envelope), STEREO (grain pan spread), DAMP (recording lowpass), LEVEL.

## Prototype limits

RAM-only audio and patch state; SAVE is intentionally not writing settings, avoiding overwriting STRATA's patch data. MIDI integration, persistent patches, richer modulation, longer buffers and more sophisticated physical modeling are future work. The screen shows the output waveform and selected position, not a literal full memory waveform. Hardware timing and package installation require validation on a physical FM-1. Use the browser first.

![MORPH screen](img/morph.png)

![SILT screen](img/silt.png)
