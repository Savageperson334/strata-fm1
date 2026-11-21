# STRATA

STRATA turns the M-VAVE FM-1 into a drone and texture synth. There's one mode and no menus. Tap a key and a voice
fades in and keeps going; tap it again and it fades away. Every knob changes the sound while it plays.

![STRATA playing three voices: the wave on top, the notes F2 C3 E3 lit below, and the seven controls with their values](manual/img/screen-playing.png "STRATA on the FM-1's screen")

This is version {{VERSION}}. You install it at your own risk, and the installer can put M-VAVE's own firmware back.

- **[Install it](install/)** from Chrome or Edge on a computer, with the FM-1 plugged in by USB. It takes about a
  minute.
- [Try it in the browser](emu/) first if you like. It's the same code, with sound, so you don't need an FM-1.
- The [Manual](manual/) is one page: what each knob and button does.
- [Download {{PKG}}](firmware/{{PKG}}) for the command-line installer: `python3 tools/fm1_install.py {{PKG}}`.
- The [Source](https://github.com/Savageperson334/strata-fm1) is under MPL-2.0. It's built on fm1-base by Charles
  Vestal and felucca-bsp, which come from Felucca by Leo Kuroshita (Hügelton Instruments).

M-VAVE and FM-1 are trademarks of their owners. STRATA is not affiliated with M-VAVE.
