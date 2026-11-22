# STRATA, PALINDROME and BABBLEBEAT

**[Project scope and instrument identities](docs/FIRMWARE_SCOPE.md)** — what STRATA, PALINDROME and BabbleBeat are, their internal modes/pages, current features, future directions, and boundaries.

Three custom firmwares for the M-VAVE FM-1 live in this repository. They are separate instruments with separate
source trees and independent versioning; the FM-1 holds one at a time.
STRATA and PALINDROME provide firmware installers. BabbleBeat is a browser-first prototype; no hardware
installer is available yet.

| Firmware | What it is | Status | Site | Manual | Install |
| --- | --- | --- | --- | --- | --- |
| **STRATA** | A drone and texture synth with an arpeggiator, a drum groove and Euclidean lanes | Released (`vX.Y`) | [site](https://savageperson334.github.io/strata-fm1/) | [manual](https://savageperson334.github.io/strata-fm1/manual/) | [install](https://savageperson334.github.io/strata-fm1/install/) |
| **PALINDROME** | SILT plays grains from recorded rings; MORPH, a four-corner synth, is the source; each press of REC captures a new ring (three; then the oldest makes room) | Experimental prerelease (`palindrome-vX.Y`) | [site](https://savageperson334.github.io/strata-fm1/palindrome/) | [manual](https://savageperson334.github.io/strata-fm1/palindrome/manual/) | [install](https://savageperson334.github.io/strata-fm1/palindrome/install/) |
| **BABBLEBEAT** | Grammar-driven sentences, remembered nouns, structural mutations and a small formant-synthesis voice | Browser prototype (`0.1`) | [site](https://savageperson334.github.io/strata-fm1/babblebeat/) | [manual](https://savageperson334.github.io/strata-fm1/babblebeat/manual/) | [emulator](https://savageperson334.github.io/strata-fm1/babblebeat/emu/) (no hardware installer) |


PALINDROME's source, manual and build notes are in [instruments/palindrome](instruments/palindrome) (it was
first published as PALIMPSEST; the old web address forwards to the new one), and BabbleBeat's in
[instruments/babblebeat](instruments/babblebeat). No custom firmware here has been tested on a real FM-1 yet:
[docs/HARDWARE_CHECK.md](docs/HARDWARE_CHECK.md) is a ten-minute check for the first person who does. The next
section is about STRATA; the layout and building sections cover all three.

## STRATA

A drone and texture synth firmware for the M-VAVE FM-1. No menus: the seven knobs are always live, and SEL, ARP,
SEQ and EDIT each give them a second set while held.

**Install:** open <https://savageperson334.github.io/strata-fm1/install/> in Chrome or Edge, plug in the FM-1 by
USB, click Connect, then Install. **Manual:** <https://savageperson334.github.io/strata-fm1/manual/>
(source: [app/docs/MANUAL.md](app/docs/MANUAL.md)).

- Tap keys to start up to six long voices; tap again to let them fade. PLAY fades all out and brings the chord back.
- ARP: a slow arpeggiator through the latched voices (tap to switch; hold ARP for RATE, SHAPE, ORDER, FLOOR).
- SEQ: a drum groove under the drone, 8 patterns on a synthesized kit with sidechain ducking; the arp locks to its
  tempo (tap to switch; hold SEQ for TEMPO, GROOVE, SWING, LEVEL, KIT, ROOM, DUCK).
- KNOB1–4, SELECT, ALGORITHM, PRESETS: TONE, RATIO, MOTION, SPACE, FILTER, DRIFT, GRIT. Hold SEL for DEPTH, SHIMMER, FADE,
  DECAY, RESO, AIR, SUB.
- Each voice: two detuned FM pairs with feedback, a sub sine, and noise band-passed at its pitch. Then a resonant
  TPT low-pass swept by a slow LFO, and a 4-line FDN reverb up to near-endless decay.
- EDIT: four Euclidean lanes, E(pulses, steps) up to 32 steps, each with its own length, rotation, chance and step
  length; a lane plays a drum in place of the groove's part or steps the arp (tap to switch; hold EDIT to set).
- The screen's eight boxes sit as the knobs do on the panel: MASTER, SELECT, PRESETS, ALGORITHM left; KNOB1–4 right.
- MIDI in over USB and TRS: notes, sustain, CC 20–31 and 102–114 for the twenty-five controls.

## Layout

| Path | What |
| --- | --- |
| `app/app.c`, `app/app.h` | The app: keys, knobs, screen, saving, MIDI (fm1-base's contract) |
| `app/dsp/strata_dsp.c` | The sound engine, self-contained, compiled at -O2 |
| `app/fm1.toml` | Build and site settings |
| `app/tests/` | Scenarios for fm1-base's host simulator |
| `tests/` | The engine rendered to WAV and checked; the app logic on a stand-in platform; `run_tests.sh` |
| `instruments/palindrome/` | PALINDROME: its own app, engine (`dsp/`), tests, manual and `VERSION` |
| `instruments/babblebeat/` | BabbleBeat: its own app, grammar (`lang/`), voice (`dsp/`), tests, manual and `VERSION` |
| `docs/` | [What each firmware is and is not](docs/FIRMWARE_SCOPE.md), the hardware checklist, and the design plans (STRATA's modulation, PALINDROME's SILT-first redesign) |
| `scripts/publish-instrument-site.sh` | Publishes one firmware's site to GitHub Pages without touching the others' |
| `scripts/site_nav.py` | The same navigation and each firmware's colours on every page of all three sites, and the emulators' Play / Guide / Tutorial switch |
| `scripts/tutorial/` | That switch: its script and style, and each firmware's text for it (`strata.json`, `palindrome.json`, `babblebeat.json`) |
| `base/` | [fm1-base](https://github.com/charlesvestal/fm1-base) (git submodule), with felucca-bsp |

## Building

You don't need to build it yourself. Changing `VERSION` on `main` makes GitHub build the firmware from source with the shared
FM-1 builder, run the tests, publish a signed release, and put the installer site on GitHub Pages
(`.github/workflows/release.yml`). Every push to `main` runs the tests (`.github/workflows/tests.yml`).
PALINDROME builds, releases and publishes on its own when `instruments/palindrome/` changes
(`.github/workflows/palindrome.yml`), from `instruments/palindrome/VERSION`. BabbleBeat has no firmware release:
`.github/workflows/babblebeat.yml` publishes its browser emulator and manual. A change to the shared navigation,
colours or tutorial text is applied to the published pages by `.github/workflows/refresh-pages.yml`, with no
firmware rebuilt. `tests.yml` also builds all three for the FM-1 itself, releasing nothing, and reports each
one's image and memory use.

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
M-VAVE and FM-1 are trademarks of their owners; STRATA, PALINDROME and BabbleBeat are not affiliated with M-VAVE.
