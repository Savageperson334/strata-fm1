# BabbleBeat — grammar-driven robotic speech

A third independent FM-1 firmware, alongside STRATA and PALINDROME. A sentence
is assembled by grammar rules first, then optionally rewritten or damaged.
The voice is a simple formant robot, not general text-to-speech.

## Quick start

1. Press a few **white keys**. Each one makes its own phrase and says it; press one again to hear it again.
2. Press **black keys** to mutate the phrase you played last: negate it, ask it, swap its nouns.
3. Press **REC** to remember a noun; later phrases may return to it.
4. Press **PLAY** to start or stop continuous, autonomous babbling; **SEQ** for a new phrase on the key.
5. Press **EDIT** for WORDS knobs, **FX** for VOICE knobs. Both select pages with a single press.
6. Lost? Hold **GLO**: the screen shows what every button and key does.

## Buttons

| Button | Action |
| --- | --- |
| PLAY | Toggle automatic phrase generation and playback |
| SEQ | New grammatically structured sentence, spoken immediately |
| ARP | Mutate the current sentence and speak the result |
| REC | Remember one noun from this sentence for later reuse |
| EDIT | WORDS page |
| FX | VOICE page |
| HOME | Stop speaking and turn off automatic speech |
| OCT- / OCT+ | The whole voice down or up an octave |
| GLO (hold) | The guide: what every button does, on the screen |
| SAVE, SEL, ENV, LFO | Not used; the screen says so |
| White keys | Sixteen phrases, one a key |
| Black keys | Eleven mutators of the phrase last played |

## Keys

**White keys are phrases.** Each of the sixteen holds its own. Press one and it says its phrase; the first press
makes it. Lower keys speak in a lower voice. The key last played is the phrase SEQ, ARP, REC and the black keys
work on, and its mark is the tall one in the row on the screen.

**Black keys are mutators.** Each changes the phrase last played in its own way and says the result:

| Black key, left to right | Mutation |
| --- | --- |
| 1 | **NEW NOUN** — one noun replaced |
| 2 | **SWAP NOUNS** — two nouns change places |
| 3 | **NEW ADJECTIVE** — the adjective replaced (or the phrase rebuilt with one) |
| 4 | **REBUILD** — another sentence pattern around the same first noun |
| 5 | **NEGATE** — "does not" / "did not" |
| 6 | **TENSE** — will / was / used to |
| 7 | **ASK** — turned into a question |
| 8 | **ADD CLAUSE** — ", and the … " |
| 9 | **RECALL** — the remembered noun put in (remembers one if none is) |
| 10 | **STUTTER** — a word doubled |
| 11 | **DROP** — a word dropped |

Every other key's phrase stays as you left it, so you can go back to it.

## WORDS — EDIT

| Knob | Control | Function |
| --- | --- | --- |
| KNOB1 | **GRAMMAR** | High: valid structures retained; low: duplicates and omissions permitted |
| KNOB2 | **WANDER** | Chance of borrowing nouns from outside the selected world |
| KNOB3 | **RECALL** | Chance of using the remembered noun |
| KNOB4 | **DAMAGE** | How often a new sentence is mutated as it is made (GRAMMAR decides how gently) |
| SELECT | **WORLD** | Machine, Dream, Body or Gothic/Dracula vocabulary |
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
