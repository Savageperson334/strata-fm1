# FM-1 instrument family — scope, identity and boundaries

**Status:** Living product understanding / orientation for contributors, not an implementation ticket.
**Basis:** Repository `main` and its manuals on 22 November 2026. The code and current manuals win if this narrative becomes stale. **Planned** and **possible** ideas below are not assertions that they already work, nor are they automatic authorizations to implement them.

## 1. The organizing idea

This repository contains **three different custom firmwares for the M-VAVE FM-1**, rather than three pages of one firmware. **Only one firmware runs on the FM-1 at a time.** They may share `fm1-base`, build/test support, the web emulator, documentation structure, and site navigation; they should retain distinct synthesis engines, performance identities, versioning and releases.

| Firmware | Creative object | What the player does | System's central memory | Short identity |
| --- | --- | --- | --- | --- |
| **STRATA** | An ongoing live sound and its rhythm | Latch voices; steer timbre, pulses, evolving patterns | Saved sound/chord/settings, plus live musical state | **Grow and steer sound** |
| **PALINDROME** | A recorded piece of sound | Inject a source; record a ring; play and alter grains; record a new generation | Separate **audio rings**, each with its own sound and SILT parameters | **Transform and inherit sound** |
| **BabbleBeat** | A phrase with grammatical roles | Generate a structured sentence; mutate it; remember a word; play/speak it | **Linguistic state**: current tokens, theme drift and a remembered noun | **Transform and perform language** |

There is a family resemblance: each combines something recognizable with controllable mutation and can become stranger over time. **That does not mean each should accumulate the other two's engines.** A unified sound editor, general-purpose workstation, or cross-firmware patch format is not the design goal.

### "Mode" means different things in different contexts

- **STRATA / PALINDROME / BabbleBeat** are **firmwares / instruments**, not modes inside one flashed program.
- **STRATA's ARP, SEQ and EDIT** are cooperating *performance subsystems* of one continuous instrument; they can run together.
- **PALINDROME's SILT, MORPH, RINGS, SHAPE, MOTION, MIX and REC FX** are persistent *control pages* around a continuously running synthesis/capture/granular engine. A page switch is not necessarily an audio switch.
- **BabbleBeat's WORDS and VOICE** are two *control pages* of one language-to-audio process, not two competing sound engines.
- Website navigation switches *which firmware's information/emulator you are viewing*. Its colors signal instrument identity; the browser switch does **not** magically change firmware installed on the physical device.

## 2. STRATA — the living sound

### Identity

STRATA should feel like a playable patch that **keeps breathing after you take your hands off the keys**. The player starts an evolving drone or chord and can push it into rhythmic, harmonic, noisy or ambient territory without first programming a song.

The essential character is **continuous synthesis + musical time + generative control**. A low barrier to making sound matters even as deeper sequencing and modulation grow.

### Implemented / documented today

- Six latched FM drone voices. Sound uses detuned FM pairs/feedback, sub oscillator and tuned noise, a resonant low-pass and reverb. Piano keys latch/release notes; PLAY fades and recalls the chord.
- The primary sound controls cover **TONE, RATIO, MOTION, SPACE, FILTER, DRIFT and GRIT**, with a held-SEL layer for **DEPTH, SHIMMER, FADE, DECAY, RESO, AIR and SUB**.
- **ARP** is an arpeggiator of the latched voices, with rate/shape/order/floor.
- **SEQ** is a synthesized drum-groove engine with eight grooves, tempo/swing, kit, room, level and ducking.
- **EDIT** operates four Euclidean lanes. Independent STEPS, PULSES, ROTATE, CHANCE, EVERY and PLAYS send triggers to drums or ARP. The lanes use a shared clock with beat/arp integration.
- **REC** performs a small random mutation of the main sound parameters; it is not an audio recording button in STRATA.
- SAVE stores patch, chord and relevant sequencer settings; MIDI notes/CCs are supported. The 240×240 display mirrors the physical controls and shows sound/transport state.

These are *interacting features*, not separate installed instruments. For example, the drone can breathe under a tempo-locked arpeggiator while a Euclidean lane replaces selected drum hits.

### Intended direction / proposal, not presently a shipped promise

The repository's [Generative Modulation & Euclidean Rhythm Design](GENERATIVE_MODULATION_DESIGN.md) describes an expanded instrument that could add **multiple complex LFOs, performance macros, richer Euclidean routing, clock interactions and a modulation matrix**. Only its initial shared-clock / Euclidean subset is documented as implemented.

My understanding of the desired expansion is that it should make the existing drone/beat instrument *more expressive and interconnected*, not turn STRATA into a different synthesizer with nested DAW menus. A modulation destination should remain anchored in the sound the player already understands. Preserve the ability to latch a few notes and immediately hear something interesting.

**Scope guardrails:** Do not mistake the proposed four-LFO matrix, additive trigger modes, lane-to-LFO routing or advanced per-step behavior for shipped capability. Do not sacrifice live knob response, understandable default controls, or stable audio simply to add more modulation destinations.

## 3. PALINDROME — sound records itself, then becomes something else

### Identity

**SILT is the instrument; MORPH is the injector.** The defining gesture is **make a recording, manipulate it, capture its output into another distinct recording**, and return to any earlier generation without destroying it.

**The key idea (0.8): a sound, looped, and made again from itself.** A key strikes MORPH (an envelope-shaped, complex seed, never noise); the note is caught as a ring; SILT loops it; **DRIFT** makes the next generation from the last by itself, each evened to the same loudness. DRIFT off, the loop is stable; up, it drifts. All of it follows from the first touch.

SILT is not just an effect hanging off MORPH. The selected ring and its granular manipulation are the primary surface. The system has a visual and audible history: **concentric circles for separate audio generations**.

### Implemented / documented today

**MORPH — source page (FX).** An independent four-corner XY synth blends **FM, modal, additive and wavefolding** sounds. Keys strike source notes that die away (DECAY; at its top they hold). X/Y, complexity, brightness, orbit/motion and excitation shape the source. MORPH may continue sounding on other pages.

**SILT — home/primary page (EDIT or HOME).** A granular playback engine operates the **selected recorded ring**. POSITION, SIZE, DENSITY, SPRAY, REVERSE and PITCH make it an instrument in its own right. Up to 16 grains are active globally; up to four latched keyboard pitches can play grains, including when automatic playback is off.

**Recording (REC).** One press starts a finite capture into a **new ring**. The current design has **three 1.49-second rings**. The first capture comes from MORPH if there is no ring; on MORPH's page it takes the synth source, while SILT-focused playing captures processed grains of the selected ring. Capture completes automatically. With nothing sounding REC is armed and starts with the next sound. A capture always makes a new ring; when all three are in use the **oldest ring (never the selected one) makes room**, and the screen names it beforehand. MORPH is monitored on its own page and during its capture, otherwise the grains are heard alone, so a sound and its recording do not stack up. The interface shows source/progress/armed/drops state and changes to the new ring when capture finishes. Returning to an older ring and recording it again can create a *branch*, not a forced linear chain.

**Ring management (SEL).** SELECT chooses the ring. **CARRY** can deliberately mix a prior ring into a future capture; deletions must be deliberately armed and confirmed. Rings hold independent playback settings and their own PCM.

**Additional pages:**
- **ENV — SILT SHAPE:** grain envelope, length variation, scattered pitch, detune, cluster.
- **LFO — SILT MOTION:** automatic scan/position movement and timing irregularity.
- **ARP — MIX** and **SEQ — REC FX:** balance, fade, stereo/spread/level; and record-only transformations including noise/color/crush/gate/drop/damp. REC FX changes the *next recording*, not previously recorded PCM.

**UI philosophy:** Ordinary functions need **one click/press**, with persistent pages and legible source/ring feedback. No important hidden hold gesture should be necessary in the browser. MASTER stays volume; SELECT is the ring selector. SAVE is currently unused, rather than secretly retaining stock-firmware behavior; ARP opens MIX and SEQ opens REC FX; GLO, held, is the on-screen guide.

### The key conceptual boundary

**A ring is not automatically an additive live loop layer.** Multiple circles primarily indicate preserved generations or branches. The selected ring is the one you edit and granularly play. Mixing or carrying prior material must be **intentional**; adding a ring is not permission to sum every ring and feed it back indefinitely.

Likewise, a ring is **recorded audio**, while MORPH is a continuously playing **source**. The ring's current knob settings should not permanently alter its original captured PCM until a *new* generation is recorded.

### Direction / non-goals

A useful continuation includes clearer provenance (which ring was recorded from which), expressive ways to compare and revisit generations, safer source and capture decisions, more interesting modulation of granular playback, and maintaining the intuitive visual link between sound and concentric rings.

But **not** by replacing SILT with a full multi-track DAW, overwriting the ring being played, treating recording as instant when it actually takes time, or merging everything into a single circular buffer. Hardware RAM and CPU budgets constrain ring count, duration and density. The full prior [SILT-first redesign plan](PALINDROME_SILT_FIRST_REDESIGN.md) contains earlier architectural decisions and cautions; it must be read with the **current** implementation in mind, because some of that plan has already shipped.

**Current limitations:** Ring audio/settings are volatile (no ring storage across power-off), no MIDI input in the PALINDROME app, and physical-device performance remains unverified. A published experimental firmware package is not equivalent to hardware qualification.

## 4. BabbleBeat — language is the oscillator's score

### Identity

BabbleBeat is a **grammar-driven generative speech instrument** that treats a sentence as something you can play, mutate, deform and remember. It is not supposed to be only random word salad, but neither does it need sophisticated general-purpose TTS before it becomes musically interesting.

Its central loop is:

`GRAMMAR TEMPLATE → ROLE-TAGGED WORD CHOICE → SENTENCE → MUTATION / MEMORY → SIMPLE ROBOTIC VOICE → REPEAT`

The original Python WordNet/Dracula gibberish application supplied the *idea* of themed vocabulary, sentence structures, associative drift, repetition and remembered phrases. CHIMERA supplied only **partial conceptual inspiration** for an articulatory/visual speech synth. Neither is being transplanted wholesale.

### Implemented / documented today

**WORDS — EDIT page.** A compact C grammar generator picks from **18 sentence templates**, including transitive and intransitive statements, adjectives, questions, negative statements, future/past constructions, conditionals, relative clauses and compound clauses. Word-role tags distinguish nouns, verbs, adjectives, punctuation and connecting words; the lexicon includes **Machine, Dream, Body and Gothic** noun worlds.

The main controls are **GRAMMAR, WANDER, RECALL, DAMAGE, WORLD, LENGTH and STYLE**:

- **GRAMMAR** favors intact structures or allows destructive token repetition/omission at low settings.
- **WANDER** moves nouns across themed worlds and a drifting vocabulary context.
- **RECALL** permits a remembered noun to return in later phrases; **REC** chooses that noun from the current sentence.
- **DAMAGE** sets how often a new sentence is mutated as it is generated; GRAMMAR decides whether those mutations keep the grammar or break it.
- **WORLD** chooses the primary noun pool.
- **LENGTH** can add a coordinated clause.
- **STYLE** weights statements, mixed constructions or questions.

The mutation engine can replace/swap word-role slots, change an adjective, or **rebuild the sentence using a different template while preserving the first noun**. Destructive operations can duplicate/delete tokens or interrupt a sentence with “again”. It is a **small deterministic sentence machine**, not full syntactic or semantic understanding. Some grammatically unusual combinations are acceptable as part of its character.

**VOICE — FX page.** Hand-authored, approximate phoneme spellings drive a small **formant synthesizer** (a glottal pulse and noise through four cascaded formants that glide between sounds, shaped noise for hisses and bursts). **PITCH, RATE, TONE, BREATH, LEVEL, STUTTER and INFLECT** shape the voice. The spoken output is intentionally crude and should be judged first as an *instrument* rather than a voice assistant.

**Performance:**
- **SEQ:** generate and immediately speak a new sentence.
- **ARP:** mutate and speak the current sentence.
- **REC:** remember a noun for future use.
- **PLAY:** toggle automatic, sequential babbling.
- **White keys:** sixteen **words**, one a key (the WORLD's nouns in order); a key says a phrase about its word and keeps it, lower keys in a lower voice. The word can move, one step at a time (WANDER), never erratically; the phrase is made again now and then (DAMAGE).
- **Black keys:** the word of the white key on their left, in another **kind of phrase**, a kind a key (why?, not, was, if, will, is it?, did not, used to, because, may, when). OCT shifts the voice's register.
- **GRAMMAR** sets how every phrase on the unit is built: each kind of phrase (the white keys' statement and the eleven black-key kinds) has a form at five levels, BARE, SIMPLE, PLAIN, JOINED, NESTED. Breaking a phrase (doubled, dropped words) belongs to DAMAGE.
- **SEL** steps through the knob sets (VOICE, MOUTH, WORDS); **RECALL** past 70 fixes on the remembered word and loops the phrases back on themselves.
- **HOME:** stop speech/autoplay; **SAVE** currently shows unavailable.

WORDS and VOICE are two **single-press knob pages**, not two separate firmware modes. The screen shows the sentence, active grammatical template, memory and an audio-related playhead display.

### Intended direction, deliberately bounded

The near-term design priority is **better grammar and musical mutation**, not an immense speech physics engine. Potential next steps include:

- More deliberately constructed sentence structures (tense, subject/object agreement, substitution constraints, connective logic), plus a clear choice between *preserve syntax* and *destroy syntax*.
- Phrase-to-phrase memory that does more than one remembered noun: recurring subjects, rhythmic refrains, thematic obsessions, or gradual shifts from one semantic field to another.
- **Timing and sequencing** of words/phonemes with rests, accents, repeats and musically useful triggers. Despite the name **BabbleBeat**, the current app does **not** contain a STRATA-style drum machine or a mature beat sequencer.
- Better, but still practical, pronunciation quality and expressive vowel/formant behavior; a legible, truthful speech visualization if helpful.
- A larger **offline-compiled** compact vocabulary with curated WordNet-style semantic relationships, subject to flash/RAM limits, rather than running NLTK or the Python interface on the FM-1.

**Out of scope for the initial stages:** Porting CHIMERA's full physical vocal tract/FDTD engine, a general English TTS system, neural voices, downloading arbitrary dictionaries at runtime, loading all of Dracula into device memory, recording and editing full spoken audio as SILT rings, or treating a simple word-stutter control as a complete glitch language model. These ideas can be reconsidered explicitly later, but should not block work on grammar.

**Current limitations:** Small fixed lexicon and pronunciation table; no arbitrary user text input, recording, saved patches or app MIDI; the BabbleBeat package (0.4 on) has not run on hardware. The website intentionally offers the **browser emulator** and documents that hardware installation is unavailable.

## 5. Shared design principles

### A. The device is an instrument, not a menu demonstration

The physical FM-1 has fixed button labels, a small screen, seven encoders, note keys, limited RAM and limited CPU. **What a control does must be clear on the current page.** The tutorial should explain *the real app*, not paper over two incompatible button models.

- STRATA is the established performance instrument with live controls and **some held editing layers**. Those are its current documented semantics, not a template for the others.
- PALINDROME intentionally rejected mandatory holds: persistent pages and **one press per action** are central to its redesign.
- BabbleBeat likewise uses **single-press WORDS/VOICE pages** and direct generate/mutate/remember actions.
- Do not inherit an original M-VAVE button meaning unless it genuinely helps the new firmware.

### A2. The screen explains the buttons

The FM-1's printed button names fit none of these instruments, so **in all three, holding GLO shows a guide on the device's own screen**: the twelve buttons drawn where they are, each with what it does in that firmware, and a line or two for the keys. It is the one deliberate hold in PALINDROME and BabbleBeat, and it changes nothing: let go and the instrument is as it was. GLO has no other job in any of them.

### B. Mutation means three different things

| Firmware | What gets mutated | What should survive |
| --- | --- | --- |
| STRATA | Synth parameters, modulation relationships, musical triggers | The ongoing playable drone / note set and the possibility of steering back |
| PALINDROME | Rendered audio through grain processing and **new** captures | Earlier recorded rings, their identity, and independent playback settings |
| BabbleBeat | Words, syntactic structure, memory context and phoneme playback | The sentence's roles/coherence when chosen, or a recognizable anchor across rewrites |

This distinction is worth preserving in code names, docs and tests. “REC” and “MUTATE” need not mean the same thing between firmware images.

### C. Common infrastructure, separate creative identity

Keep separate:
- Engines/app state and release identities/`VERSION`s.
- Independent instrument docs, tests, emulator previews and firmware safety checks.
- Thematic page colors: **STRATA teal**, **PALINDROME violet**, **BabbleBeat charcoal/copper**.
- Install state: a published browser emulator is not evidence that the corresponding package is safe to flash.

Share where appropriate:
- `fm1-base` platform/display/audio contracts, host simulation and hardware build tooling.
- Site navigation and tutorial infrastructure; switcher links should keep users on equivalent home/manual/emu/install pages.
- Safety disciplines: no unbounded allocations in the audio callback, no blind feedback loops, no unsafe flash operations, predictable save semantics and explicit behavior when resource limits are reached.
- Tests that exercise the **same C engine used for the web and firmware target**. Avoid a feature-rich web mockup that cannot run on hardware.

### D. Visual design should reflect the actual sound/data model

- STRATA: activity of voices, timbre and clocked triggers; show the playable state.
- PALINDROME: each **ring** represents real, separate audio memory; grains and capture progress should reflect it.
- BabbleBeat: the **sentence and its structure** are first-class, with visibility of mutation/memory and the currently spoken word or rhythm.
- Pages follow each firmware's palette without recoloring the M-VAVE screen art so much that it no longer represents the embedded device.

## 6. Readiness / what “done” means

| Area | STRATA | PALINDROME | BabbleBeat |
| --- | --- | --- | --- |
| Core implementation | Drone + ARP + beat + Euclidean lanes | MORPH source + generational SILT rings | Grammar + mutation + small formant-synthesis voice |
| Browser / host | Available; simulator-tested | Available; simulator-tested | Available; simulator-tested |
| Device release status | Firmware releases exist | Experimental prerelease exists | Experimental prerelease exists (0.4 on) |
| Real FM-1 validation | **Not established** in repo docs | **Not established** in repo docs | **Not established** |
| Persistence | Saved patch/chord/settings | Rings/settings are RAM-only | No patch save; a remembered noun is volatile |
| Main next design frontier | Deep generative modulation while keeping live simplicity | Better expressive recording lineage and safe manipulation | Better grammar, memories and performance sequencing |

**A passing emulator/host test is a development milestone, not hardware certification.** Device testing should include audio load, clipping, RAM headroom, controls, USB/MIDI behavior where supported, reset/recovery and installer/stock-firmware restoration. See [hardware check](HARDWARE_CHECK.md).

## 7. Working priorities and decision rules

This is my interpretation of a practical development order, not an imposed roadmap:

1. **Do not blur the instrument identities.** New work should be explainable in one sentence using each firmware's central object: evolving sound, recorded ring, or structured phrase.
2. **Fix obvious usability and structural faults before widening scope.** A confusing capture source, invisible button layer or grammatically incoherent default mode is a larger issue than a missing fancy algorithm.
3. **Extend the object, not the feature list.** STRATA: how modulation/rhythm change a sound. PALINDROME: how generations inherit/change a recording. BabbleBeat: how grammar/memory change a phrase.
4. **Prefer controllable variation.** Interesting randomness should be anchored: note/chord in STRATA, source ring in PALINDROME, grammatical role/theme in BabbleBeat.
5. **Keep future proposals visibly separate from shipped features.** A doc can propose four LFOs, additional audio rings or a WordNet compiler without claiming they are implemented or even approved.
6. **Measure on real hardware before increasing DSP/RAM requirements or flashing new builds.** Keep preview publication distinct from firmware release.

## 8. Questions intentionally left open

These are **design choices**, not missing requirements that must be answered before ordinary development:

- **STRATA:** How much deeper should patch routing and automation become without spoiling the immediate seven-knob live workflow?
- **PALINDROME:** Should future versions permit intentional *simultaneous* ring mixing, or primarily continue soloing a ring and recording a new generation? The current default should not be changed implicitly.
- **BabbleBeat:** Should rhythmic word/per-phoneme performance become a full sequencer, or stay a minimal gestural instrument around a generative sentence stream? Neither should be treated as implemented today.
- **Across all three:** How much consistent visual/navigation language helps users without making three unrelated instruments feel artificially identical?

## 9. Source-of-truth pointers

- [Repository README](../README.md): firmware family, locations, release/build notes.
- [STRATA manual](../app/docs/MANUAL.md) and [generative modulation design](GENERATIVE_MODULATION_DESIGN.md): shipped sound/beat/lanes versus staged modulation proposal.
- [PALINDROME manual](../instruments/palindrome/docs/MANUAL.md), [PALINDROME app](../instruments/palindrome/app.c), and [SILT-first plan](PALINDROME_SILT_FIRST_REDESIGN.md): current single-press generational ring model versus original redesign proposal.
- [BabbleBeat manual](../instruments/babblebeat/docs/MANUAL.md), [language engine](../instruments/babblebeat/lang/babble_lang.c), and [voice engine](../instruments/babblebeat/dsp/babble_voice.c): actual words, templates and basic sound.
- [Shared Pages navigation](../scripts/site_nav.py), [publish script](../scripts/publish-instrument-site.sh), and [hardware checklist](HARDWARE_CHECK.md).

---

**One-line project compass:** **STRATA evolves a sound. PALINDROME evolves a recording. BabbleBeat evolves a sentence.** The FM-1 should let the player intervene in each evolution without taking away the ability to play.
