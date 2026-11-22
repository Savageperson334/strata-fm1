# PALINDROME — SILT-first navigation and generational recording plan

**Audience:** Claude Code implementing PALINDROME in the FM-1 firmware repository  
**Status:** Design specification / staged implementation plan. **Not implemented by this document.**  
**Baseline inspected:** `main` at `acf5ee3`, PALINDROME v0.5 (22 November 2026). Re-read HEAD before editing; development may continue.  
**Target:** The PALINDROME alternative firmware, its host simulator, its browser emulator/tutorial and its manual. STRATA is a separate firmware.

> **Product sentence:** **SILT is the instrument. MORPH is the injector. Each recording becomes a new ring that can be revisited, played, modified and used to create the next generation.**

## 0. Implementation handoff / non-negotiables

1. Work **only** within PALINDROME and its shared site/tutorial support where required. **Do not modify STRATA DSP, STRATA controls, `VERSION`, release identity, or installer behavior.** The existing `/palindrome/` and `/palindrome/emu/` URLs must remain valid.
2. Treat this as a **navigation + audio-memory architecture redesign**, not a request for additional unrelated synth algorithms.
3. **No mandatory button holds** for any normal function. A single click/tap must be sufficient in the browser and on hardware. Existing press/hold overloads should be retired from PALINDROME, not merely hidden by a better tutorial.
4. Make **SILT the default/home page** and primary musical workflow. MORPH remains an independently running four-corner source and its controls are still available when needed.
5. A new capture must **not destructively overwrite** an older ring. Selecting an earlier ring must restore its audio and its per-ring playing controls. Rendering/resampling produces a **new** ring.
6. **No accidental feedback**: never read and write the same sample buffer for generational capture; no implicitly mixed old layers; bound all feedback and output gain.
7. **Hardware and WebAssembly must share the actual C DSP and app logic.** Don't implement a nice browser-only simulation that diverges from hardware. The browser tutorial is an interface on the real emulator.
8. **Measure the FM-1 RAM and render budget before choosing the ring count.** The first target is three rings only if the hardware linker/build has comfortable headroom. Preserve a lower-count fallback if necessary.
9. Use a feature branch/PR if another developer is actively modifying these areas; inspect current changes and reconcile first. Do not clobber work, force-push, or bump/releases versions until tests pass and a release is intentionally requested.

### Existing code worth preserving

- `../instruments/palindrome/dsp/palindrome.c` and `.h`: four-vector synthesis, one 32,768-sample mono int16 ring at 22.05 kHz, sixteen grains, note/pitch slots, recording degradation (NOISE/COLOR/CRUSH/GATE/DROP/DAMP), and LIVE/FREEZE/OVERDUB/RESAMPLE.
- `../instruments/palindrome/app.c`: page selection based on `mode` plus `page_of(buttons)` held masks; `pidx` parameter mapping; visual circle/square; tap-vs-hold REC semantics; tapped grain pitch keys.
- `../scripts/tutorial/tutorial.js`, `palindrome.json`, `tutorial.css`: current **Play / Tutorial** switch. Tutorial is a **clickable reference blueprint**; it makes real controls inert instead of guiding live playing.
- `../scripts/site_nav.py`, `../scripts/publish-instrument-site.sh`, `../.github/workflows/palindrome.yml`: build/publish the independent PALINDROME site and emulator.
- `../instruments/palindrome/docs/MANUAL.md`: concise reference, currently organized by MORPH/SILT halves; update to match the new workflow only after control semantics are real.
- `../instruments/palindrome/tests/basic.txt`, `tests/dsp.c`: existing host/sound smoke tests. Extend them; don't remove coverage.

As of the baseline, the default page is MORPH; FX enters MORPH; EDIT enters SILT and held EDIT selects extra SILT parameters; held REC edits recording texture but a tap cycles record mode; held SEL edits shared controls. This architecture is **the specific thing to replace**.

## 1. Mental model and language

Use consistent vocabulary everywhere: device screen, emulator controls, tutorial and manual.

| Term | Meaning |
| --- | --- |
| **SILT** | Primary granular instrument. Plays and transforms the **selected ring**. |
| **MORPH** | Auxiliary live synthesis source/injector. It is not a recording layer. |
| **Ring / layer** | One independent recording, rendered audio plus its own editable playback settings. Visually a concentric circle. Prefer **RING 01**, **RING 02**, etc. on screen. |
| **Selected ring** | Which recording the SILT knobs edit and the grain engine plays. Selection alone never mutates recorded PCM. |
| **Source** | Explicit audio source for a new capture: MORPH or the processed output of the selected SILT ring. |
| **Record / capture** | A finite acquisition of the source into a **new** destination ring. |
| **Generation** | A new ring made by manipulating an older ring. Ring 03 might have Ring 02 as parent. Selecting an old ring and recapturing it can branch the history. |
| **Playback** | Grains from the selected ring. PLAY controls whether automatic grains spawn; the keys can still trigger them manually. |
| **Transform** | Non-destructive grain settings, pitch/texture/movement, and optionally the recording effects when creating a new generation. |
| **Mix** | Optional deliberate combination of rings/source. The default is **solo selected ring**, not all rings mixed. |

The user's normal loop must be immediately understandable:

```text
SILT home: no rings yet
  → FX: MORPH generates the source
  → REC: capture MORPH into RING 01
  → SILT automatically becomes active, RING 01 selected
  → PLAY and turn grain knobs
  → REC: capture SILT's processed playback into RING 02
  → SELECT: switch between preserved rings
  → edit any ring / capture another generation
```

A page switch is **not** a source signal switch or mute. Show a visible SOURCE label when record source differs from the selected editing page. Source choice must never be implicit in the audio engine, even if the UI sets sensible defaults.

## 2. Navigation rules

**One press = one action / persistent page.** No "tap to toggle, hold to edit" and no "release after turning a knob does something different" for normal use. Pressed pages remain active until another page is selected. All normal functions must be usable by mouse clicks on PC; no virtual chord or held button is required.

**Boot:** show SILT. If no ring exists, display an inviting empty-ring state: **"NO RINGS · FX = SOUND · REC = CAPTURE"**. Do not autoplay random audio or leave users on a confusing empty MORPH page.

**HOME:** one tap returns to SILT (safe, non-destructive). Do not require a one-second hold to return home. If an all-clear/reset function remains, place it behind an explicit confirmation in a visible settings/utility screen; no press-and-hold as an essential operation.

**SELECT encoder:** a **stable, predictable ring selector** across SILT-centric pages. It can be unused on MORPH if necessary, but it must not silently become PITCH, DROP, SPREAD, etc. on different SILT pages. Show its target prominently: `SELECT: RING 02/03`. If no rings exist, encoder changes nothing and screen explains why. Preserve old parameters by relocating them to other pages.

**Keys:** default to SILT grain pitches, with 4 latched pitch slots and immediate note strikes. On the MORPH page the keys latch up to four source synth notes. The screen must clearly show which role keys currently have; changing pages preserves each side's note state. OCT− and OCT+ continue shifting keyboard range (their physical names fit the job).

### Proposed one-press page map

This is the preferred layout; adjust assignments only if a more intuitive same-name mapping can be proved, with a documented decision. Every used button is a single press.

| FM-1 label | New meaning | Surface |
| --- | --- | --- |
| **EDIT** | **SILT** | Main grains: position, size, density, spray, pitch, reverse, etc. |
| **FX** | **MORPH SOURCE** | XY synth injector, 4 synth corners |
| **SEL** | **LAYERS** | Ring browser, source/parent metadata, delete/replace confirmation, visibility |
| **REC** | **CAPTURE NEW RING** | One press starts finite recording from the visible source; second tap while busy must never create another ring |
| **PLAY** | **GRAINS PLAY / STOP** | Automatic grain spawning; hand-played keys remain usable |
| **ENV** | **SHAPE** | Grain envelope and variation: SHAPE, SIZE variation, DETUNE, SCATTER, CLUSTER |
| **LFO** | **MOTION** | SCAN, timing irregularity, position movement, modulation behavior |
| **GLO** | **MIX / PROCESS** | Output/mixing controls and discoverable **RECORD FX** subpage |
| **HOME** | **SILT HOME** | One tap, no clearing |
| **SAVE** | **SAVE / PROJECT** | Until real persistence exists, display "NOT AVAILABLE" and never pretend to save |
| **ARP** | **PITCH ORDER** (future/optional) | Dedicated melodic behavior for held SILT keys, rather than a hidden modifier |
| **SEQ** | **RHYTHM** (future/optional) | Regular/irregular, eventually Euclidean grain spawning; don't invent a function just to occupy the label |

The text on the physical FM-1 face cannot change. Compensation must come from the on-screen page title, illuminated active button, the emulator's software legend, a succinct cheat sheet and possibly an optional printable overlay. **Don't retain default firmware behavior just because the original button once had that meaning.** Unused buttons should be visibly inactive—not mysterious.

### Page and parameter ownership

- Default SILT page: KNOB1 POSITION, KNOB2 SIZE, KNOB3 DENSITY, KNOB4 SPRAY, **SELECT RING**, ALGORITHM REVERSE, PRESETS PITCH.
- MORPH page: preserve XY, complexity, tone, spread, movement and excite. Relocate whatever currently occupies SELECT if SELECT is kept global in MORPH. Alternative: explicitly say SELECT is an exception only on MORPH; the primary SILT controls must remain consistent.
- ENV page: SHAPE, VARY, SCATTER, DETUNE, CLUSTER and related grain-envelope settings.
- LFO page: SCAN and TIMING, with any real modulation controls; leave other slots empty rather than adding placeholders.
- GLO page: MIX, FADE, LEVEL, layer level/solo, and a clearly visible **MIX / RECORD FX** sub-tab. The sub-tab may be selected using an on-screen menu plus an encoder, not by holding GLO. Keep NOISE, COLOR, CRUSH, GATE, DROP, DAMP and FEEDBACK reachable by clicks/encoders, with an obvious **RECORD FX** heading.
- SEL page: layer index, parent, source, play/mute indication, room remaining, and a guarded management action. No automatic deletion.
- On every page, keep the physical knob name and the current parameter name/value visible. Avoid "page changed because a button is down" logic.

For a subpage (e.g. GLO → RECORD FX), use a **persistent, visible selection**, not an implicit temporary modifier. The UI may offer a tiny page picker operated by K4/PRESETS or the on-screen control. Preserve **SELECT = ring** when possible. For a first milestone, implement the four central pages EDIT, FX, SEL and REC fully; migrate the other pages after testing navigation.

## 3. Recording model: concentric generations

### Audio representation and hardware budget

The current buffer `PM_BUFFER = 32768` at half the 44.1 kHz engine rate holds **~1.486 s of mono 16-bit PCM**, i.e. **65,536 bytes per full ring**. More rings cost real hardware RAM:

| Full-size rings | PCM only |
| --- | ---: |
| 1 | 64 KiB |
| 2 | 128 KiB |
| 3 | 192 KiB |
| 4 | 256 KiB |

These figures omit grain state, engine state, mixer, UI, platform, scratch buffers, and safety headroom. The fm1-base default memory linker uses a bounded pool; the physical device has not yet been profiled for this extension. **Do not allocate three/four buffers on assumptions alone.**

Preflight: record actual simulator/native link output and map, `pool_spare`, static RAM/BSS, and worst-case render time. Try 3 rings if viable. If not, deliver **2 preserved rings** at current quality first or choose a measured reduced sample count/quality with an honest display of duration. Host/WebAssembly may offer an optional larger capacity, but the shipped emulator defaults to hardware-compatible limits. Keep max count compile-time and test it.

### Suggested data model

```c
/* Sketch only — actual fields/layout determined from memory budget. */
typedef struct {
    uint32_t id;                  /* stable until deleted */
    uint32_t parent_id;           /* 0 = created from MORPH */
    uint32_t valid_samples;       /* <= samples per ring */
    uint8_t  source_kind;         /* MORPH / SILT */
    uint8_t  ready;
    /* Per-ring normalized SILT parameters and key/algorithm preferences. */
    /* PCM storage owned by an indexed, fixed ring slot; not embedded in a giant copied struct. */
} PalRingMeta;

typedef enum {
    CAP_IDLE, CAP_RECORDING, CAP_FINISHING, CAP_ERROR
} CaptureState;

typedef struct {
    uint8_t selected_slot;
    uint8_t source_kind;
    uint8_t source_slot;          /* valid for SILT source */
    uint8_t destination_slot;     /* reserved, not exposed until complete */
    uint32_t samples_written;
    CaptureState capture;
} PalRingTransport;
```

Store metadata and PCM in fixed, bounded slots; do not allocate in the audio callback. Use `parent_id` rather than assuming each ring always follows the preceding index: creating from old ring 01 must be allowed even after ring 03 exists. IDs and slot indexes must remain distinct so deletion/reuse does not connect to a wrong parent.

### Capture semantics

**REC is a single-press, finite operation.**

- With no rings, default capture source to MORPH; the UI prompts if the source is silent rather than presenting unexplained emptiness.
- On MORPH, **REC** captures only the live MORPH output. On SILT, **REC** captures the **processed SILT output of the selected ring**. In other utility pages, retain a clearly displayed explicit capture source, last chosen from FX/EDIT or a visible source selector.
- Capture is **not an instantaneous snapshot** unless a full previous-history buffer is intentionally retained. Prefer **start recording on REC and automatically complete after the ring duration**, with clear progress, to avoid hidden memory costs.
- Commit the newly captured ring **only after the entire PCM is written**. Until then it is marked busy/unavailable. Old rings remain playable and immutable. On success select the new ring and return to SILT automatically.
- The captured signal defaults to the processed SILT **wet-only** grain output when SILT is the source, so unchanged MORPH cannot leak into a subsequent generation. An explicit mix/capture-source option can be added later.
- Never capture the write destination into itself; never spawn grains reading incomplete destination PCM. Continue reading the previously selected source slot during capture.
- Pressing REC while CAP_RECORDING: no second capture is created. Show progress. Decide whether a clearly labeled CANCEL action is needed in the layer manager, not an ambiguous hidden hold.
- If no slot is free, show **"RINGS FULL"** with a clear non-destructive path to choose and explicitly replace or delete a ring. **Never silently destroy the oldest.**
- On no usable signal, error, short capture, power interruption or panic: keep all committed rings intact; discard the unfinished destination. A ring should not appear as valid until finished.
- Treat the input/output of capture effects (noise, crush, gate, drop, damp) as a distinct stage. They affect new PCM only, not existing PCM. FREEZE can remain an internal buffer safety concept but should not be a required user navigation mode.
- On completed capture, ensure selected ring's new playback controls have sensible defaults, preferably inherited from source if SILT-derived but never applied destructively to its PCM.

### Playback and continuity

- Playback selects **one ring by default**. No cumulative layer playback or doubled gain simply because there are more rings.
- Layer selection preserves per-ring grain settings, note pitches (define whether global or per-layer), playback options and sample content. At minimum persist POSITION/SIZE/DENSITY/SPRAY/PITCH/REVERSE/SHAPE per ring while it exists.
- Ring switching while existing grains sound: let existing grains finish against their original immutable ring or crossfade them out, but do not suddenly reinterpret old grain sample indices against the newly selected ring. Use a ring ID/slot in each grain state.
- Keep the **sixteen-grain global cap** initially to bound CPU, not sixteen per ring. If several rings are intentionally mixed later, allocate/grant voices explicitly.
- Ring routing and gains should be audibly click-free with per-block smoothing.
- Allow selecting an older ring, modifying its controls non-destructively, then creating a **new child** ring: branching, not overwriting its previous child.
- Define new ring active settings and key state on capture. Avoid silently turning MORPH notes off unless the user explicitly requests this.

### Memory/race safety

- UI queries ring data while the audio interrupt writes capture buffers. Use safe telemetry snapshots / narrowly volatile fields as appropriate, and avoid blocking the render.
- Ring preview downsampling must never scan beyond valid written samples, assume contiguous chronological data when indices differ, or read an in-progress slot as if ready.
- Reset/panic should stop capture cleanly and release grains. Avoid corrupting metadata on the next boot; memory/presets are **RAM-only unless persistence is separately implemented**.
- Do not add SD/flash persistence in this milestone without a separate storage design; repeatedly writing live PCM to flash is inappropriate.

## 4. Screen layout: SILT owns the visual space

The present 240×240 display puts a prominent MORPH vector square inside a larger ring. The new visual hierarchy should give **rings** primary emphasis.

### SILT home

```text
┌──────────────────────────────┐
│ SILT    RING 02 / 03    ▶    │
│                              │
│  ◎ ◎ ◎  concentric rings     │
│  selected ring is brightest  │
│  waveform + grain playheads  │
│  MORPH = small source center │
│                              │
│ SOURCE: SILT 02   REC: READY │
│ POS  SIZE  DENS  SPRAY      │
│  ... remaining knob cells    │
└──────────────────────────────┘
```

- One persistent indicator for **selected ring index / total**.
- Each ring corresponds to independent audio; rings are drawn outward as generations are added (01 inner, 02 next, 03 outer). The selected ring is brighter or thicker, not just different colored.
- Memory waveform belongs to its own ring; show a compact sampled/decimated envelope, not a decorative identical waveform copied across all rings.
- The center MORPH indicator is deliberately small on SILT pages.
- Capture state unmistakable: SOURCE, REC/READY, progress and free slots.
- If zero rings: blank outlined ring and the simple prompt to inject/capture.
- Avoid UI geometry that illegibly squeezes text into unused knob boxes. Prioritize selected ring, source, major parameter names and current values.

### MORPH page

The center enlarges to a usable vector XY square and 4 corner labels; the outer SILT rings remain visible but subdued so the user sees where the audio goes. Page switch must not stop SILT playback. Show **MORPH · SOURCE** and a direct REC hint.

### Layer browser / layer mixer

One press SEL opens a clear list such as `01 ← MORPH`, `02 ← 01`, `03 ← 02` with active/selected and source. SELECT changes current ring with detents. Provide a deliberately visible action for delete/replace (confirmation). A mixer/mute/solo view may follow, but default should be solo selected ring. Do not imply that scrolling between rings immediately performs a capture.

### Layout engineering

Reuse existing `cv_*` screen primitives and `pd_ring` geometry as practical; avoid 60 Hz expensive per-sample operations or re-rendering full screens in the DSP loop. Render a small fixed-number waveform summary per layer; keep 240 px legibility in both host screenshots and the browser. The same visuals must display correctly on the physical LCD.

## 5. Tutorial/emulator: learn by doing

The newly deployed **Play / Tutorial** switch is a good foundation, but tutorial.js currently intercepts clicks, makes `#controls` inert, and only shows text explanations. Preserve its reference-blueprint utility, **plus** a short **interactive guided Play** path. The first-session tutorial should teach the actual workflow rather than explain a historical modifier architecture.

### Desired structure

- **Play:** controls operate the real WebAssembly instrument normally.
- **Guide:** overlays a small, contextual step and highlights the real live control; user taps/turns the actual instrument and the guide detects resulting state. No holding required. Can skip/back/exit; don't trap focus.
- **Reference:** current blueprint/help behavior may remain as a secondary, non-playing explainer, generated from the same page/button schema. Don't let reference mode's simulated "hold" state contradict hardware.

### First-run guide (short, skippable)

1. **SILT HOME**: "Nothing captured yet. Press FX to make a source."
2. **MORPH**: "Tap a note, turn X/Y and hear four synthesis corners."
3. **CAPTURE**: "Press REC once. Watch ring 01 fill and finish."
4. **PLAY SILT**: "The new ring is selected. Press PLAY, adjust SIZE and POSITION."
5. **SECOND GENERATION**: "Change grain processing and press REC. Watch ring 02 appear, preserving ring 01."
6. **LAYERS**: "Press SEL, turn SELECT to go back to ring 01. Its sound and controls are still there."

Advancement checks must follow actual app state, not a timer or a click on an inert blueprint. Expose minimal stable telemetry from the host/WebAssembly app (page, ring_count, selected_ring, capture_state, capture_progress, active_grains, source) through existing emulator facilities if available. Do not add fragile DOM inspection of button text as the primary progress source. Handle failure or a missed event gracefully (manual Next/Back/Skip). Keep keyboard, pointer, and accessible labels in mind.

### Labeling strategy

Software labels should say **the job**, not an old firmware name: e.g. `FX · MORPH SOURCE`, `EDIT · SILT`, `SEL · RINGS`, `REC · NEW LAYER`, `PLAY · GRAINS`. The visible device screen, manual, emulator help overlay, and guided steps must use the **same exact vocabulary**. No "EDIT held", "REC held", or "SEL held" text should remain after migration. On PC, a normal click must act as a full tap.

## 6. Proposed implementation sequence (complete, testable stages)

**Stage A — Navigation first, no DSP semantic changes.**

1. Introduce an explicit persistent page enum and a single dispatcher for button-down edges.
2. Default to SILT and implement EDIT/FX/SEL/HOME/PLAY pages without holds or implicit on-release actions.
3. Reassign main knobs with SELECT reserved for ring selection, even if only one ring exists for now. Add screen headings and inactive controls; keep existing pitch/voice behavior.
4. Give REC a single clear action stub or the existing one-ring finite capture behind a feature flag. No unsignaled capture mode cycle.
5. Update host interaction tests and initial emulator hints to match. At this stage, no capability silently disappears; if advanced params are not remapped yet, keep a visible temporary menu.

**Stage B — Non-destructive ring store.**

1. Capture the real native memory report before allocating extra PCM.
2. Implement a fixed-capacity ring-store abstraction, metadata, ready/in-progress state, stable IDs and per-ring params.
3. Separate input source, selected playback ring and destination. Have a deterministic path for MORPH → RING 01 and SILT 01 → RING 02.
4. Define and test finite capture progress / auto-commit; no aliasing.
5. Implement ring selection and smooth grain playback transitions. Handle full-capacity gracefully.
6. Keep original synthesized FM/vector engine and grain algorithms unchanged as much as possible.

**Stage C — Complete knob/page migration.**

1. Move recording effects, ENV, LFO/MOTION, GLO/MIX and pitch/rhythm controls to discoverable persistent pages.
2. Preserve all useful controls currently in PALINDROME 0.5; do not delete DSP algorithms just because the old modifiers went away.
3. Make the on-screen ring browser and 240×240 SILT-first visualization usable on host screenshots and emulator.

**Stage D — Guided emulator tutorial and documentation.**

1. Evolve the tutorial schema; remove legacy modifier instructions.
2. Add guided live steps and telemetry-based completion, retaining blueprint as optional reference.
3. Rewrite `MANUAL.md` with SILT first, MORPH as injector, and a side-by-side control comparison/quick reference; follow with layered capture examples, recording effects and button map.
4. Rebuild the real site using existing PALINDROME workflow; ensure the STRATA homepage/manual/installer/emulator remain untouched and the independent PALINDROME URLs work.

**Stage E — Release readiness.**

1. Host simulator, unit DSP, button and integration tests pass.
2. Build WebAssembly, listen through actual emulator, inspect browser console, confirm single mouse click interactions.
3. Build native firmware and inspect linker memory totals plus worst-case audio render time; test on physical FM-1 if available. **Do not call hardware verified solely from host tests.**
4. Only after success, intentionally bump PALINDROME's independent VERSION and release the updated manual/site/emulator/installer through the existing pipeline.

## 7. Minimum acceptance scenarios

Test these as user-level scripts and low-level DSP tests. A stage is not done until its scenario works.

| Scenario | Expected behavior |
| --- | --- |
| Boot | SILT home shows zero rings, an easy instruction, no ghost audio |
| Press FX once | MORPH page, keys play synth notes; no hold |
| Press REC once from MORPH | Progress appears, one new PCM ring, auto-finish, old layers untouched |
| Press PLAY in SILT | Grains audible from selected ring; K1/K2 change their sound |
| Press REC from SILT 01 | Ring 02 records **processed SILT only**, not MORPH/source double-feed |
| Select 01 after making 02 | 01's original PCM and edited settings are still there |
| Select 02 then 01 while grains play | No click, invalid sample read, unexpected stuck grain, or silent crash |
| Modify 01 and record again | New child ring, 02 remains unchanged; parent IDs correct |
| Reach ring capacity | "RINGS FULL"; no silent deletion or overwrite |
| All 16 grains active | Bounded CPU; new grains skipped or deliberately voiced according to design |
| Press REC while recording | No accidental second destination or ring count inflation |
| Navigate through all pages | One click, no modifier or button hold; active mode always indicated |
| Browser guided tutorial | Can actually play sound and complete the six steps using clickable controls |
| Save button while unsupported | Correct, explicit "NOT AVAILABLE"; no fake saving |
| STRATA smoke | Existing STRATA functionality, URLs, releases, and tests unaffected |

Additional DSP checks: zero/uninitialized ring, max-length read boundary, half-speed writer, reverse reads, abrupt source switch, wet-only resampling feedback/gain, per-ring parameter isolation, capture cancellation/error/panic, parent relationship consistency and per-grain source pinning.

### Explicit UX anti-goals

- Never make the user learn "tap vs hold" for normal access.
- Never have a control perform two unrelated functions based on whether another control remains pressed.
- Never hide destructive recording/clear behind a gesture whose name implies something else.
- Never blur MORPH and SILT into equally prominent pages; SILT is the destination and home.
- Never let a second-generation recording erase the first.
- Never add circular artwork that only **looks** like separate rings while all rings still reference the same PCM.
- Never make the tutorial imply that a feature works when the underlying DSP has not shipped.

## 8. Decisions to settle with a quick prototype, not a debate

- **Exactly how many full-length rings fit** with native safety headroom; start with 3 if safe, otherwise 2 or a documented duration/quality compromise.
- **Subpage selection** for recording effects under GLO, with a one-click, visible path and no required hold.
- **SELECT behavior on MORPH**: universal ring selector or clearly labeled limited exception; SILT pages must be consistent.
- **Default layer key state**: global pitch latches or saved per-ring; choose whichever produces more predictable switching, then test it.
- **Recording while playing:** select SILT wet-only as a safe default; provide explicit source/audition routing rather than accidentally feeding live MORPH into a child layer.
- **What happens when recording is aborted**: discard incomplete destination and preserve existing rings. UI must say so.

Do not block progress on ambitious multitrack mixing, SD storage, four-or-more ring capacity, custom knob labels, or a full modular modulation matrix. Those are later iterations after the SILT-first workflow feels right.

## 9. Deliverable summary for Claude

Implement the **smallest complete vertical slice** first: SILT home → one-click MORPH → one-click REC → new ring → granular playback → one-click REC → second preserved ring → SELECT back to first ring. Then expand controls and the tutorial around it.

When reporting completion, include the actual commit, native and host/WebAssembly build outcomes, RAM usage, ring capacity, render-load observation, screenshots of SILT with 0/1/2 rings, emulator link, changed controls, tests and remaining hardware limitations. Do not claim a feature is complete because the manual says it exists.

## 10. Implementation status (branch `feature/silt-first`)

Stages A to D are implemented on the branch and playable at `/palindrome/next/`; nothing is released and
PALINDROME's `VERSION` is unchanged (stage E waits for a deliberate release and for hardware).

**Measured** (the device build, `.github/workflows/palindrome-next.yml`, job `memory`):

| | 0.5 (one ring) | this branch (three rings) | of |
| --- | ---: | ---: | ---: |
| Pool | 134,660 B | 265,732 B | 344,064 B |
| `.data` + `.bss` | 30,380 B | 33,052 B | 98,304 B |
| Image | 93,144 B | 97,868 B | app slot |

Three rings leave 78,332 B of the pool (the build insists on 8,192). A fourth would leave 12,796 B: it fits, but
not comfortably, so `PM_RINGS` is 3. Render time on the FM-1 is **not measured**: no hardware. The grain loop is
0.5's (16 grains); a capture adds one write every second sample and, for 16 blocks at its end, 2,048 samples of
level-evening a block.

**Decisions** (section 8):

- **Ring count:** three full-length rings (above).
- **Subpage under GLO:** GLO opens MIX; pressed again, REC FX; again, MIX. The top line names both and lights the
  open one. One press each, nothing held.
- **SELECT on MORPH:** the ring selector there too. SPREAD moved to MIX, DROP to ALGORITHM on REC FX, and CARRY
  (below) to RINGS, so SELECT is never anything else.
- **Key state:** the four grain pitches are global, not per ring: selecting a ring changes the sound under the
  same held keys. MORPH's notes are separate and survive page changes.
- **Recording while playing:** SILT's source is the grains alone (wet only); MORPH's is MORPH alone. With the
  grains stopped, a SILT capture has nothing to record and is dropped.
- **An aborted or empty capture** is discarded and the rings are untouched. A capture is finite, so there is no
  cancel: REC pressed meanwhile says it ends by itself.
- **FEEDBACK became CARRY:** with no overdub there is nothing to feed back. CARRY copies the selected ring under
  the next capture (0: the source alone), read from the parent, never from the ring being written.
- **A new ring's level is evened** (peak to about 0.7, never more than four times up), so generations do not fade;
  its two ends are eased over 256 samples so the loop closes without a click.
- **A new ring's settings** are the ones in use; made from SILT, its PITCH, REVERSE, SCATTER and DETUNE start at
  rest, so what the grains already did to it is not done twice.
- **Ring numbers** are ids that are never reused (`RING 04` after a delete), so `FROM RING 01` stays true.
- **Deleting:** on RINGS, KNOB4 arms (one step: the selected ring; two: everything, notes included) and SEL
  confirms; any other button calls it off. HOME is one tap to SILT and clears nothing.
- **Guide telemetry:** the panel's lights (the emulator's own frames) and `pm_web_state` (WebAssembly build only),
  sent by `web/emu/app_worklet.js`. The guide reads no button text.

**Not done:** ARP (pitch order) and SEQ (rhythm) are inactive and say so; no ring mixing (the selected ring is
solo); no persistence; no test on an FM-1.

