# PALINDROME

A separate firmware from STRATA. See [manual](docs/MANUAL.md). Site, emulator and installer:
<https://savageperson334.github.io/strata-fm1/palindrome/>.

The FM-1's toolchain has no C library headers: keep `<string.h>` and friends out of `app.c` and `dsp/` (the
simulator-only `app_expect` is inside `#ifdef PLAT_HOST`), and note that `plat.h` already defines an integer `clamp`.

From the repository root, after initializing the base submodule and installing its prerequisites:

    python3 base/tools/build.py instruments/palindrome --host
    python3 base/tools/build.py instruments/palindrome --web
    cc -O2 -DPLAT_HOST -Ibase/include -Iinstruments/palindrome/dsp instruments/palindrome/tests/dsp.c instruments/palindrome/dsp/palindrome.c -lm -o /tmp/palindrome-test
    /tmp/palindrome-test

The build workflow generates actual host screenshots before the release build. The separate version file controls PALINDROME releases; STRATA's VERSION, app and releases are independent. Package ID prefix 59 is project-local; verify availability before distribution to a firmware hub.
