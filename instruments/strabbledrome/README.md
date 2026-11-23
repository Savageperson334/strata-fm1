# STRABBLEDROME

**A fourth M-VAVE FM-1 firmware project: STRATA + PALINDROME + BabbleBeat, selectable from one instrument.**

**Status: planned / integration scaffold. No STRABBLEDROME firmware binary, emulator or installer exists yet.** The three standalone releases are playable and remain independent.

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

The individual November 2026 device builds report approximately:

| Instrument | App binary | RAM pool |
| --- | ---: | ---: |
| STRATA 0.4.2 | 108 KB | 143 KB |
| PALINDROME 0.9 | 104 KB | 266 KB |
| BabbleBeat 0.4 | 103 KB | 69 KB |
| **Sum** | **315 KB** | **478 KB** |

The platform provides an application flash slot of about **581 KiB** and a RAM pool of about **336 KiB**. Binary totals are not a combined build or reliable flash-size proof; they count separate platform overhead. More importantly, the sum of the current static pool allocations **does not fit**. A fourth independent firmware can't be produced by merely concatenating the three existing binaries.

### Preferred architecture to prototype

Build one application dispatcher with a **shared, exclusive RAM workspace** whose maximum footprint is sized for the largest selected engine. Keep each engine's source separate and independently buildable; explicitly namespace their callbacks and symbols. Rework their large static allocations to use the workspace or another provably exclusive overlay. At most one audio engine owns the memory at a time. The linker map, host build, device build, and actual FM-1 memory report must verify the result.

A flash-resident module/loader is a fallback option **only if the combined image cannot fit**. Do not alter bootloader or update/recovery regions on the basis of guessed flash capacity.

## Milestones for Claude / contributors

1. **Launcher proof:** Create the real app shell and a consistent GLO menu on host + browser + device, even before integrating sound engines.
2. **Memory and symbol audit:** Prove that app callbacks, pool buffers, persistence and linker sections can be shared or namespaced. Record exact code/flash/RAM totals.
3. **Integrate STRATA:** Drone, six voices, arpeggiator, drums, Euclidean lanes and saved patches still work. Reserve FX/ENV/LFO for its future modulation expansion.
4. **Integrate PALINDROME:** Three 1.49-second SILT rings, MORPH injector, SCAN, FLOW, WEAVE, REC and safe switching out of the PCM buffer.
5. **Integrate BabbleBeat:** Grammar, voice, key mappings, mutation and remembered words.
6. **Full system tests:** Change instruments many times without crashes, stale audio, stuck buttons, memory overlap or patch corruption. Exercise the same GLO menu in the physical UI and browser emulator.
7. **Separate release:** Only after a native build, device checks and recovery validation should STRABBLEDROME receive its own package identity, VERSION, installer and fourth playable site.

### Boundaries

- No simultaneous triple audio engine or unexplained blended instruments in v1.
- No promises that recordings survive a mode switch.
- No replacement of the three standalone releases.
- No fake installation links or packaged firmware before an actual successful build.
- No LFO, FX or ENV button permanently stolen from STRATA.

This directory is the **canonical GitHub entrypoint** for STRABBLEDROME until executable source and build instructions are added.
