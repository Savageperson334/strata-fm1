# STRATA and PALIMPSEST

Two custom firmwares for the M-VAVE FM-1 live in this repository. They are separate instruments with separate
versions, releases and installers; the FM-1 holds one at a time, and either installer can put the other (or
M-VAVE's own firmware) back.

| Firmware | What it is | Status | Site |
| --- | --- | --- | --- |
| **STRATA** | A drone and texture synth with an arpeggiator, a drum groove and Euclidean lanes | Released (`vX.Y`) | <https://savageperson334.github.io/strata-fm1/> |
| **PALIMPSEST** | An alternate instrument: MORPH blends four synthesis methods, SILT captures and granulates the sound | Experimental prerelease (`palimpsest-vX.Y`) | <https://savageperson334.github.io/strata-fm1/palimpsest/> |

PALIMPSEST's source, manual and build notes are in [instruments/palimpsest](instruments/palimpsest). The rest of
this page is about STRATA.

## STRATA

A drone and texture synth firmware for the M-VAVE FM-1. One mode, no menus, every knob live.

**Install:** open <https://savageperson334.github.io/strata-fm1/install/> in Chrome or Edge, plug in the FM-1 by
USB, click Connect, then Install. **Manual:** <https://savageperson334.github.io/strata-fm1/manual/>
(source: [app/docs/MANUAL.md](app/docs/MANUAL.md)).

- Tap keys to start up to six long voices; tap again to let them fade. PLAY fades all out and brings the chord back.
- ARP: a slow arpeggiator through the latched voices (tap to switch; hold ARP for RATE, SHAPE, ORDER, FLOOR).
- SEQ: a drum groove under the drone, 8 patterns on a synthesized kit with sidechain ducking; the arp locks to its
  tempo (tap to switch; hold SEQ for TEMPO, GROOVE, SWING, LEVEL, KIT, ROOM, DUCK).
- KNOB1–4, SELECT, ALGORITHM, PRESETS: TONE, RATIO, MOTION, SPACE, FILTER, DRIFT, GRIT. Hold SEL for DEPTH, BEAT, FADE,
  DECAY, RESO, AIR, SUB.
- Each voice: two detuned FM pairs with feedback, a sub sine, and noise band-passed at its pitch. Then a resonant
  TPT low-pass swept by a slow LFO, and a 4-line FDN reverb up to near-endless decay.
- EDIT: four Euclidean lanes, E(pulses, steps) up to 32 steps, each with its own length, rotation, chance and step
  length; a lane plays a drum in place of the groove's part or steps the arp (tap to switch; hold EDIT to set).
- The screen's eight boxes sit as the knobs do on the panel: MASTER, SELECT, PRESETS, ALGORITHM left; KNOB1–4 right.
- MIDI in over USB and TRS: notes, sustain, CC 20–44 for the twenty-five controls.

## Layout

| Path | What |
| --- | --- |
| `app/app.c`, `app/app.h` | The app: keys, knobs, screen, saving, MIDI (fm1-base's contract) |
| `app/dsp/strata_dsp.c` | The sound engine, self-contained, compiled at -O2 |
| `app/fm1.toml` | Build and site settings |
| `app/tests/` | Scenarios for fm1-base's host simulator |
| `tests/` | The engine rendered to WAV and checked; the app logic on a stand-in platform; `run_tests.sh` |
| `instruments/palimpsest/` | PALIMPSEST, the alternate firmware: its own app, engine, tests, manual and `VERSION` |
| `scripts/publish-instrument-site.sh` | Publishes one firmware's site to GitHub Pages without touching the other's |
| `base/` | [fm1-base](https://github.com/charlesvestal/fm1-base) (git submodule), with felucca-bsp |

## Building

You don't need to build it yourself. Changing `VERSION` on `main` makes GitHub build the firmware from source with the shared
FM-1 builder, run the tests, publish a signed release, and put the installer site on GitHub Pages
(`.github/workflows/release.yml`). Every push to `main` runs the tests (`.github/workflows/tests.yml`).
PALIMPSEST builds, releases and publishes on its own when `instruments/palimpsest/` changes
(`.github/workflows/palimpsest.yml`), from `instruments/palimpsest/VERSION`.

To build locally on Linux x86-64 (or macOS with Docker), see fm1-base's README for the JieLi toolchain and the AC79
SDK, then:

    git clone --recursive https://github.com/Savageperson334/strata-fm1
    ./build.sh                     # app/build/strata.fwsc
    ./build.sh --host              # the host simulator
    sh tests/run_tests.sh

The engine and the app logic test with nothing but a C compiler:

    cc -O2 -DPLAT_HOST -Ibase/include -Iapp/dsp tests/dsp_render.c app/dsp/strata_dsp.c -lm -o dsp_render && ./dsp_render .

## Licence

MPL-2.0. Built on fm1-base (MPL-2.0) by Charles Vestal and felucca-bsp, from Felucca by Leo Kuroshita (Hügelton
Instruments). Installing custom firmware is at your own risk. The installer can put M-VAVE's firmware back.
M-VAVE and FM-1 are trademarks of their owners; STRATA is not affiliated with M-VAVE.
