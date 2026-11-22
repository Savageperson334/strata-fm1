# PALIMPSEST

A separate firmware from STRATA. See [manual](docs/MANUAL.md).

From the repository root, after initializing the base submodule and installing its prerequisites:

    python3 base/tools/build.py instruments/palimpsest --host
    python3 base/tools/build.py instruments/palimpsest --web
    cc -O2 -DPLAT_HOST -Ibase/include -Iinstruments/palimpsest/dsp instruments/palimpsest/tests/dsp.c instruments/palimpsest/dsp/palimpsest.c -lm -o /tmp/palimpsest-test
    /tmp/palimpsest-test

The build workflow generates actual host screenshots before the release build. The separate version file controls PALIMPSEST releases; STRATA's VERSION, app and releases are independent. Package ID prefix 59 is project-local; verify availability before distribution to a firmware hub.
