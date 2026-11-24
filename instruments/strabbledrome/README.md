# STRABBLEDROME

**A fourth M-VAVE FM-1 firmware project: STRATA + PALINDROME + BabbleBeat, selectable from one instrument.**

**Status: milestones 1 to 5 done; not released.** All three instruments are built in and switch from one menu (`app.c`). It passes its simulator test and builds for the FM-1 (174 KB image, 49 of 96 KB RAM, 266 of 336 KB pool) and for the browser on every push. It has not run on a real FM-1, and there is no installer, published emulator or package identity yet: that is milestone 7. [Manual: switching between the three](docs/MANUAL.md) · [Audit](AUDIT.md)

[STRATA source](../../app/) · [PALINDROME source](../palindrome/) · [BabbleBeat source](../babblebeat/) · [Repository](../../README.md)

## Intended experience

A single installed firmware contains the three instruments. Press **GLO** once to open a common menu, turn the SELECT encoder, press SEL to confirm, and use GLO or HOME to go back. There are no required button holds or multi-button chords.

    STRABBLEDROME — SYSTEM
    > STRATA       drone / rhythm
      PALINDROME   granular rings
      BABBLEBEAT   robotic speech
      Help / Controls
      System Settings

**One engine runs at a time**. On switching, stop audio safely, clear/release the outgoing instrument's volatile voice state, initialize the incoming engine, and restore whatever persistent configuration is safely supported. The active instrument retains its own pages, sound and identity. Switching back to PALINDROME will not magically restore its RAM-only recorded rings.

### Shared button behavior

| Input | System menu | Playing an instrument |
| --- | --- | --- |
| GLO tap | Back / close | Open STRABBLEDROME menu (one press) |
| SELECT encoder | Highlight a destination | Active instrument's normal encoder |
| SEL button | Confirm | Active instrument's normal button |
| HOME | Back / close | Active instrument's normal HOME |
| Other knobs/keys | No accidental synth actions | Owned by the active instrument |

The menu is the owner of GLO input. **Help / Controls** offers the active instrument's button guide without hiding anything behind a hold. STRATA should keep LFO free for complex oscillators, FX for the modulation matrix, ENV for envelopes, and SAVE for patches. The standalone builds need not adopt the combined wrapper's control convention.

## The actual engineering problem

The [audit](AUDIT.md) measured each instrument's own share apart from the platform's:

| | Flash | RAM pool | Other RAM |
| --- | ---: | ---: | ---: |
| The platform (the launcher's own build) | 81 KB | 69 KB | 25 KB |
| STRATA 0.4.2, its own share | 27 KB | 74 KB | 5 KB |
| PALINDROME 0.9, its own share | 23 KB | 197 KB | 8 KB |
| BabbleBeat 0.4, its own share | 22 KB | 0 | 10 KB |
| **Combined** | **~153 of 581 KB** | **340 KB side by side; 266 KB with one overlay** | **49 of 96 KB** |
| Limit | 581 KiB | 336 KiB, 8 KB kept free | 96 KiB |

Flash and ordinary RAM fit with room. The pool misses by 4 KB if STRATA's buffers and PALINDROME's rings sit side by side, and fits with 78 KB spare if they share one workspace. (The earlier 478 KB sum counted the platform's 69 KB three times.)

The harder part is names, not bytes: the platform compiles an app inside its own single unit, where three `app.c` files collide on dozens of file-scope names.

### Preferred architecture to prototype

Build one application dispatcher with a **shared, exclusive RAM workspace** whose maximum footprint is sized for the largest selected engine. Keep each engine's source separate and independently buildable; explicitly namespace their callbacks and symbols. Rework their large static allocations to use the workspace or another provably exclusive overlay. At most one audio engine owns the memory at a time. The linker map, host build, device build, and actual FM-1 memory report must verify the result.

**Decided by the audit:** each instrument's `app.c` is compiled as its own unit with its seven `app_` calls renamed, and the launcher hands it the platform's drawing calls, which are private to the platform's unit. The instruments' sources stay as they are. STRATA's buffers and PALINDROME's rings share one workspace, cleared on every change.

A flash-resident module/loader is a fallback option **only if the combined image cannot fit**. The audit says it fits. Do not alter bootloader or update/recovery regions on the basis of guessed flash capacity.

## Milestones for Claude / contributors

1. **Launcher proof (done):** Create the real app shell and a consistent GLO menu on host + browser + device, even before integrating sound engines.
2. **Memory and symbol audit (done, [AUDIT.md](AUDIT.md)):** Prove that app callbacks, pool buffers, persistence and linker sections can be shared or namespaced. Record exact code/flash/RAM totals.
3. **Integrate STRATA (done; its own tests still run on standalone STRATA, the combined test covers starting it, playing it and its guide):** Drone, six voices, arpeggiator, drums, Euclidean lanes and saved patches still work. Reserve FX/ENV/LFO for its future modulation expansion.
4. **Integrate PALINDROME (done, likewise):** Three 1.49-second SILT rings, MORPH injector, SCAN, FLOW, WEAVE, REC and safe switching out of the PCM buffer.
5. **Integrate BabbleBeat (done, likewise):** Grammar, voice, key mappings, mutation and remembered words.
6. **Full system tests (begun: `tests/basic.txt` changes instruments repeatedly and in a hurry; not yet on hardware):** Change instruments many times without crashes, stale audio, stuck buttons, memory overlap or patch corruption. Exercise the same GLO menu in the physical UI and browser emulator.
7. **Separate release:** Only after a native build, device checks and recovery validation should STRABBLEDROME receive its own package identity, VERSION, installer and fourth playable site.

### Boundaries

- No simultaneous triple audio engine or unexplained blended instruments in v1.
- No promises that recordings survive a mode switch.
- No replacement of the three standalone releases.
- No fake installation links or packaged firmware before an actual successful build.
- No LFO, FX or ENV button permanently stolen from STRATA.

## How it is put together

- `app.c` is the launcher: the menu, the change of instrument, and the platform's drawing handed on (`shim.h`).
- `engines/strata.c`, `palindrome.c`, `babblebeat.c` each include that instrument's own `app.c`, unchanged, as a unit of its own, with its `app_` calls renamed (`engine.h`). The sound engines are included the same way.
- STRATA's reverb buffers and PALINDROME's rings share one workspace (`FM1_WORKSPACE`, an `#ifdef` in each engine). The launcher clears it on every change.
- GLO never reaches an instrument. HELP in the menu presents GLO to the running instrument so that it shows its own guide.
- Not done: remembering the last instrument across power-off, and keeping an instrument's state when you leave it.

## Build and run

~~~sh
python3 base/tools/build.py instruments/strabbledrome --host
instruments/strabbledrome/build/host/strabbledrome_host instruments/strabbledrome/tests/basic.txt /tmp/shots
~~~

`tests.yml` runs that on every push, and builds the launcher for the FM-1 and for the browser. Nothing is published.
