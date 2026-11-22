# BabbleBeat — grammar-driven robotic speech

A third independent FM-1 firmware, alongside STRATA and PALINDROME. A sentence
is assembled by grammar rules first, then optionally rewritten or damaged.
The voice is a simple formant robot, not general text-to-speech.

## Quick start

1. Press a **white key**. It has a word, and says a phrase about it. Press it again: the same phrase, now and then a new one.
2. Press the **black key** to its right: the same word in another kind of phrase (a question, a "not", a "was").
3. Keep playing keys. A word stays on its key; now and then it moves to the next word of its world.
4. **SEQ** puts a new word on the key. **REC** remembers the key's word; RECALL brings it back into other phrases.
5. **PLAY** babbles about the key's word; **HOME** stops.
6. **SEL** steps through the three sets of knobs: VOICE, MOUTH, WORDS. Lost? Hold **GLO**.

## Buttons

| Button | Action |
| --- | --- |
| PLAY | Babble: one phrase after another about the key's word |
| SEQ | A new word on the key, and a phrase about it |
| ARP | The key's phrase made again |
| REC | Remember the key's word |
| SEL | The next set of knobs: VOICE → MOUTH → WORDS |
| FX | VOICE knobs |
| EDIT | WORDS knobs |
| HOME | Stop speaking and turn off the babble |
| OCT- / OCT+ | The whole voice down or up an octave |
| GLO (hold) | The guide: what every button does, on the screen |
| SAVE, ENV, LFO | Not used; the screen says so |

## Keys

**A white key is a word.** The sixteen take the nouns of the WORLD in order, so neighbouring keys are neighbouring words. A key says a plain statement about its word and keeps it, so you can come back to it. Lower keys speak in a lower voice. The screen shows `WORD:` and a row of sixteen marks, the tall one the key in use.

**A black key is a kind of phrase.** It uses the word of the white key on its left:

| Black key, left to right | Kind | Example with *mirror* |
| --- | --- | --- |
| 1 | **WHY?** | why does the mirror remember the static? |
| 2 | **NOT** | the mirror does not corrupt the filter. |
| 3 | **WAS** | the mirror was ancient. |
| 4 | **IF** | if the mirror shivers, the antenna haunts the signal. |
| 5 | **WILL** | the mirror will return. |
| 6 | **IS IT?** | is the mirror crooked? |
| 7 | **DID NOT** | the mirror did not forget the engine. |
| 8 | **USED TO** | the mirror used to vanish. |
| 9 | **BECAUSE** | the mirror opens the static because the robot waits. |
| 10 | **MAY** | the mirror may swallow the engine. |
| 11 | **WHEN** | when the mirror dreams, the signal will return. |

**What changes, and how fast**

| Knob (WORDS) | Sets |
| --- | --- |
| **WANDER** | How often a key's word moves when you play it. It moves one step, to the next word of its world, never a jump. At 0 it never moves |
| **DAMAGE** | How often a key's phrase is made again when you play it. At 0 a phrase stays until you change it |
| **GRAMMAR** | Low: a changed phrase may be broken instead (a word doubled, dropped, an "again") |

When a white key's word moves, the black key beside it follows the next time it is played.

## WORDS — EDIT

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **GRAMMAR** | High: valid structures retained; low: duplicates and omissions permitted |
| KNOB2 | **WANDER** | How often a key's word moves a step; also, other nouns borrowed from outside the world |
| KNOB3 | **RECALL** | How often the remembered word returns. Past 70 the phrases fix on it and loop back on themselves: "the mirror was, the mirror was pale." |
| KNOB4 | **DAMAGE** | How often a key's phrase is made again as it is played (GRAMMAR decides how gently) |
| SELECT | **WORLD** | Machine, Dream, Body or Gothic/Dracula vocabulary, for keys that have no word yet |
| ALGORITHM | **LENGTH** | High values can append a coordinated clause |
| PRESETS | **STYLE** | Statements at low values; mixed in middle; questions at high values |

## VOICE — FX

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **PITCH** | Base pitch for automatic speech |
| KNOB2 | **RATE** | Phoneme length / speaking speed |
| KNOB3 | **TONE** | A darker, larger voice to a brighter, smaller one |
| KNOB4 | **BREATH** | Breath noise in the voiced sounds |
| SELECT | **LEVEL** | Synthesizer voice volume |
| ALGORITHM | **STUTTER** | Repeat individual pronunciation codes |
| PRESETS | **INFLECT** | How much the pitch moves: a lift on each word, a fall at the end, a rise for a question |

## MOUTH — SEL

SEL steps VOICE → MOUTH → WORDS. The middle of each MOUTH knob is the voice as it is made.

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **SIZE** | A small mouth (every resonance higher) to a large one |
| KNOB2 | **SLUR** | Crisp, to sliding lazily from sound to sound |
| KNOB3 | **GAP** | The quiet between words: run together, to spaced out |
| KNOB4 | **WAVER** | How much the pitch trembles |

MASTER always controls volume.

## How the sentence engine works

A compact lexicon tags parts of speech. Seven classes of grammatical pattern
use third-person verbs for statements and base verbs after "does" or "do".
Intransitive verbs are only used without objects; other templates use
transitive verbs. Thus the generator can make:

> the silent machine remembers the ghost.

> inside the coffin the silver shadow whispers.

> why does the mirror follow the moon?

At high GRAMMAR, mutation prefers role-preserving substitutions and swaps.
At low GRAMMAR, tokens can repeat, disappear or be interrupted by "again".
REC keeps a noun in a small memory slot; RECALL governs whether a later
sentence uses it. WORLD determines the noun pool; WANDER can cross worlds.

### More grammar in the second prototype

The generator now includes 18 grammatical templates, including compound
sentences, *if/then* and *when* clauses, *because* and *but* connectors,
relative clauses with *that*, negative past tense and future constructions.
At high GRAMMAR, some mutations **rebuild** a sentence in a different
structure while retaining its original first noun. The LENGTH parameter
can append another coordinated clause, sometimes two at very high settings.

Examples of supported structures (the vocabulary and verb are random):

> if the ghost whispers, the circuit remembers the moon.

> the machine that watches the ghost shivers.

> the castle did not remember the robot.

> when the shadow vanishes, the machine will return.

WANDER retains a drifting thematic pool between sentences, with remembered
words attracting that drift toward their own world. Lowering GRAMMAR permits
token repeats, deletions and intrusive words, while the underlying templates
remain valid before mutation.

The four themed banks loosely echo the previous Python WordNet/Dracula
experiment. BabbleBeat does not yet include WordNet, a full novel, the
desktop program's advanced oracle, or live dictionary downloads.

## Voice limitations

The voice is a small formant synthesizer with a hand-phonemized word list:
a glottal pulse and breath noise through four gliding formants, with shaped
noise for hisses and stops. It is robotic on purpose, and it is
not a general English TTS engine or CHIMERA's modeled vocal tract.
There is no text input, no recording, no save/load of patches, no MIDI
support in the app yet, and no tested hardware release. The browser
emulator and first real-device trials must establish intelligibility.
