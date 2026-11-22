# STRATA

STRATA turns the M-VAVE FM-1 into a drone and texture synth with no menus. Tap a key and a voice fades
in and keeps going; tap it again and it fades away. Every knob changes the sound while it plays.

![STRATA playing three voices: the wave on top, the notes F2 C3 E3 lit below, and the eight boxes placed as the knobs are](manual/img/screen-playing.png "STRATA on the FM-1's screen")

## Start in a minute

1. Turn MASTER up and tap a low key, then one seven keys above it. Two voices fade in and stay.
2. Turn KNOB4 for more reverb and KNOB1 to make it brighter.
3. Tap ARP for a slow ripple through the notes, SEQ for a drum groove, EDIT for four drifting Euclidean lanes.
4. Press PLAY to fade everything out, and again to bring the same notes back.

## Get it

- **[Install STRATA](install/)** from Chrome or Edge on a computer, with the FM-1 plugged in by USB. It takes about a
  minute.
- [Play it in the browser](emu/) first if you like. It's the same code, with sound, so you don't need an FM-1. Its Tutorial switch draws a
  blueprint over the panel that says what every control does.
- The [manual](manual/) is one page: what each knob and button does.
- [Download {{PKG}}](firmware/{{PKG}}) for the command-line installer.
- The [source](https://github.com/Savageperson334/strata-fm1) is under MPL-2.0.

Version {{VERSION}}. It's custom firmware, installed at your own risk; it has been tested in a simulator, not yet on a
real FM-1. The installer can put M-VAVE's own firmware back. STRATA's sibling is [PALINDROME](palindrome/), an
experimental instrument that records its own sound and plays it back in grains.

Built on fm1-base by Charles Vestal and felucca-bsp, from Felucca by Leo Kuroshita (Hügelton Instruments). M-VAVE and
FM-1 are trademarks of their owners; STRATA is not affiliated with M-VAVE.
