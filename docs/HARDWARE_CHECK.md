# Hardware check: ten minutes on a real FM-1

Neither STRATA nor PALINDROME has run on a real FM-1 yet. Everything so far was checked in a simulator, which can't
tell us whether the FM-1's chip keeps up or how the knobs feel. This is a short check for the first person who
installs one. Please note what happens at each step, even if it's "fine".

Have ready: the FM-1, a USB data cable, a computer with Chrome or Edge, and headphones or a speaker.

## 1. Install (2 minutes)

- [ ] Open the installer (STRATA: <https://savageperson334.github.io/strata-fm1/install/>), click Connect, then
      Install. **Note:** did it finish, and about how long did it take?
- [ ] The FM-1 restarts and shows **VERSION** and a number for a moment. **Note the number.**
- [ ] Switch the FM-1 off and on again. **Note:** does it start normally?

If the install fails or the FM-1 won't start, stop here and use the installer's "Back to the stock firmware", or
M-VAVE's M-UPGRADE app.

## 2. Basic sound (2 minutes)

- [ ] Turn MASTER up and tap one low key. **Note:** does a sound fade in within a couple of seconds? Any click,
      crackle or hiss?
- [ ] Tap the same key again. **Note:** does it fade out smoothly?
- [ ] Turn each of the seven knobs a little. **Note:** does the right box on the screen light up for each knob, and
      does turning clockwise raise the value?
- [ ] Turn MASTER. **Note:** does the VOLUME box follow it?

## 3. The heavy test (3 minutes)

This is the one that matters most: it loads the chip as hard as STRATA can.

- [ ] Tap six keys so six voices play.
- [ ] Hold SEL and turn ALGORITHM (AIR) and PRESETS (SUB) all the way up.
- [ ] Turn KNOB4 (SPACE) up. Hold SEL and turn KNOB4 (DECAY) up.
- [ ] Tap ARP, tap SEQ, tap EDIT, so the arp, the beat and the lanes all run.
- [ ] Listen for a full minute. **Note any of these:**
  - the word **OVERLOAD** on the screen (and how often),
  - crackling, stuttering or dropouts,
  - the screen freezing or the knobs feeling slow,
  - the FM-1 getting warm or the fan running hard.
- [ ] If there was trouble, tap keys off one at a time. **Note:** with how many voices does it clear up?

## 4. Saving and the rest (2 minutes)

- [ ] Press SAVE, switch off and on, press PLAY. **Note:** do the same notes and sound come back?
- [ ] Hold HOME for a second. **Note:** does the screen say SOUND RESET?
- [ ] Leave a quiet drone running for five minutes. **Note:** anything odd, such as drifting volume or noise?
- [ ] If you have a MIDI keyboard or a computer handy, play a note into the FM-1 over USB. **Note:** does it sound?

## 5. PALINDROME (optional, 3 minutes)

- [ ] Install it from <https://savageperson334.github.io/strata-fm1/palindrome/install/>.
- [ ] Tap three keys, wait two seconds, tap EDIT, tap REC, tap PLAY. **Note:** do you hear grains, and does the ring
      on the screen move smoothly or in jerks?
- [ ] Hold SEL and turn KNOB1 (MIX) up, then tap FX and EDIT a few times. **Note:** any clicks when switching?
- [ ] Tap REC until the corner says LIVE, then hold REC and turn KNOB1 (NOISE) up. **Note:** do the grains turn to
      hiss, and did letting go of REC leave the mode on LIVE?
- [ ] The heavy test: tap EDIT, tap three keys, then hold EDIT and turn ALGORITHM (CLUSTER) all the way up; let go
      and turn KNOB3 (DENSITY) and KNOB2 (SIZE) all the way up. **Note:** any crackle or stutter with 16 grains
      playing, and does the screen keep moving?
- [ ] Install STRATA again and press PLAY. **Note:** did STRATA's saved sound survive?

## What to send back

The version number, a line per section, and if anything went wrong, roughly what you were doing at the time. A
phone video of the screen and sound during the heavy test is the most useful thing of all.
