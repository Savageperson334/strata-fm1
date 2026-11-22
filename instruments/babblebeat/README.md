# BabbleBeat

Third standalone M-VAVE FM-1 firmware in the STRATA / PALINDROME repository.

The first version is deliberately focused on a **sentence grammar generator
and mutator**, with a primitive synthetic voice. It is a new instrument,
not a port of the old Python WordNet/Dracula program. The tiny thematic
lexicons echo that program's imagery, while the grammar and interface are
written for the FM-1. No source or DSP from CHIMERA has been copied.

The language engine is deterministic, heap-free C: templates assemble
sentences from nouns, inflected verbs, adjectives, and function words.
Tokens carry their grammatical role so mutations can operate on nouns
and adjectives without destroying the pattern, or can repeat/delete
tokens when GRAMMAR is turned down. REC remembers one noun and RECALL
controls its future appearance.

The rudimentary voice plays compact hand-written phoneme codes through
a glottal pulse / noise source and three resonators. It will sound
mechanical; it is not full general-purpose TTS.

## Build / simulate

After fetching the fm1-base submodule and its prerequisites:

~~~sh
cc -O2 -Iinstruments/babblebeat/lang \
  instruments/babblebeat/tests/lang.c instruments/babblebeat/lang/babble_lang.c -o /tmp/babble-lang
/tmp/babble-lang

cc -O2 -Ibase/include -Iinstruments/babblebeat/lang -Iinstruments/babblebeat/dsp \
  instruments/babblebeat/tests/voice.c instruments/babblebeat/lang/babble_lang.c \
  instruments/babblebeat/dsp/babble_voice.c -lm -o /tmp/babble-voice
/tmp/babble-voice

python3 base/tools/build.py instruments/babblebeat --host
mkdir -p instruments/babblebeat/build/shots
instruments/babblebeat/build/host/babblebeat_host \
  instruments/babblebeat/tests/basic.txt instruments/babblebeat/build/shots
python3 base/tools/build.py instruments/babblebeat --web
~~~

The hardware build also needs the JieLi SDK/toolchain. Firmware package ID 60
is provisional; the hardware has not been validated and this prototype
should not be published as an installable firmware yet.

[Manual](docs/MANUAL.md) · [Tests](tests/)

## Preview

When merged to `main`, the BabbleBeat GitHub Pages job publishes an independent browser preview under `/babblebeat/` (no firmware package). The navigation switches among STRATA (teal), PALINDROME (violet), and BabbleBeat (copper/charcoal), preserving Emulator/Manual/Install context.
