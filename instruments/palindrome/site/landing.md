# PALINDROME

PALINDROME is an experimental instrument for the M-VAVE FM-1 in two halves. MORPH makes sound from four methods you
blend with two knobs; SILT records that sound into a loop and plays it back in grains, forward and backward.

![PALINDROME's screen: MORPH's square in the middle of SILT's memory ring, the buttons named in the corners, the knobs named at the foot](manual/img/silt.png "PALINDROME on the FM-1's screen")

## Start in a minute

1. Turn MASTER up and tap a few keys. Up to four notes latch; KNOB1 and KNOB2 move the sound around the square.
2. Wait two seconds for the ring to fill, then tap EDIT. The knobs are now SILT's.
3. Tap REC to freeze the loop and PLAY to start the grains.
4. Turn KNOB1 to choose where the grains start, KNOB2 for their length, KNOB3 for how many.
5. Hold REC and turn KNOB1 to record hiss into the loop, KNOB3 to crush it, KNOB4 to gate quiet sound out.

## Get it

- **[Install PALINDROME](install/)** from Chrome or Edge on a computer, with the FM-1 plugged in by USB. It takes
  about a minute.
- [Play it in the browser](emu/) first if you like. It's the same code, with sound, so you don't need an FM-1.
- The [manual](manual/) is one page: the screen, the knobs and the buttons.
- [Download {{PKG}}](firmware/{{PKG}}) for the command-line installer.
- The [source](https://github.com/Savageperson334/strata-fm1/tree/main/instruments/palindrome) is under MPL-2.0.

Version {{VERSION}}, experimental. It's custom firmware, installed at your own risk; it has been tested in a simulator,
not yet on a real FM-1. The installer can put M-VAVE's own firmware back. PALINDROME's sibling is [STRATA](../), a drone
and texture synth; installing one leaves the other's saved sound alone.

Built on fm1-base by Charles Vestal and felucca-bsp, from Felucca by Leo Kuroshita (Hügelton Instruments). M-VAVE and
FM-1 are trademarks of their owners; PALINDROME is not affiliated with M-VAVE.
