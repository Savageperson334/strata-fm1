# STRABBLEDROME: memory and symbol audit

Milestone 2 of [the plan](README.md). Measured 23 November 2026 on STRATA 0.4.2, PALINDROME 0.9 and BabbleBeat 0.4.

**Result: all three fit in one image.** No loader and no second flash region are needed. One buffer has to be shared, and the three apps have to be compiled apart.

## Where the numbers come from

- **Device totals** are the FM-1 builds in CI (`tests.yml`, job `device`): the linker's own report.
- **The platform's share** is the launcher's own device build: it has no instrument in it, so what it reports is the platform (and a few hundred bytes of menu).
- **Each instrument's own share** is its device total less the launcher's.
- As a cross-check, each instrument's sources were also compiled alone as 32-bit objects and read with `size` (the script is at the end). The two methods agree within a few hundred bytes of RAM.

## RAM pool (344,064 B; the builder keeps 8,192 B free)

| | Pool |
| --- | ---: |
| The platform itself (BabbleBeat's total: it puts nothing in the pool) | 69,124 |
| STRATA's own buffers (143,112 less the platform) | 73,988 |
| PALINDROME's three rings (265,732 less the platform; 3 x 65,536) | 196,608 |
| BabbleBeat | 0 |
| **All three side by side** | **339,720** |
| Room in the pool, keeping 8,192 free | 335,872 |

Side by side they miss by **3,848 B**. The 478 KB in the first plan counted the platform's 69 KB three times.

**So one overlay is enough:** STRATA's buffers and PALINDROME's rings in one workspace of 196,608 B, since only one of the two runs at a time. That makes the pool **265,732 B, with 78 KB to spare**. The launcher clears the workspace on every change of instrument, so nothing of one is ever heard in the other.

## Other RAM (.data + .bss; 98,304 B)

| | Device total | Its own share |
| --- | ---: | ---: |
| The launcher alone (the platform) | 24,948 | |
| STRATA | 29,876 | 4,928 |
| PALINDROME | 33,332 | 8,384 |
| BabbleBeat | 35,220 | 10,272 |
| **Combined** | | **48,532 of 98,304** |

## Flash (the app's slot is about 581 KiB)

| | Image | Its own share |
| --- | ---: | ---: |
| The launcher alone (the platform) | 80,816 | |
| STRATA | 107,836 | 27,020 |
| PALINDROME | 103,960 | 23,144 |
| BabbleBeat | 102,960 | 22,144 |
| **Combined** | | **about 153,000**, a quarter of the slot |

The instruments' own code will come out somewhat larger in the combined build, because their `app.c` moves from the platform's size-optimised unit to units of their own. There is room for that many times over.

## Symbols

- The platform compiles an app by `#include "app.c"` inside its own single unit, and its drawing calls (`cv_begin`, `cv_text`, the fonts, the palette) are `static` there. Three `app.c` files in that one unit collide: STRATA and PALINDROME share 71 file-scope names, STRATA and BabbleBeat 45, PALINDROME and BabbleBeat 63.
- The sound engines' own global names (`sd_`, `pm_`, `bb_`) do not collide at all.
- The platform settings the three ask for agree: the same block size (256) and the same palette.
- STRATA's store map contains the other two's, so STRATA's saved sound can stay where standalone STRATA keeps it.

**So: each instrument's `app.c` becomes its own unit**, listed with the engines (`o2` in `fm1.toml`), with its seven `app_` calls renamed by macro (`strata_init` and so on) and the dozen drawing calls it needs handed to it by the launcher as ordinary functions. The instruments' sources are not edited for this, and their standalone builds are untouched. Only the two pool buffers need a line each, behind an `#ifdef`.

## What is not settled by this audit

- Whether each instrument starts clean when it is started a second time. Anything it sets only by a static initialiser, and not in its `app_init`, will be stale. To be checked instrument by instrument as it goes in.
- How Help shows an instrument's own button guide. The plan: the launcher presents GLO to the instrument for as long as Help is open.
- The real total. These are sums of separate builds until the combined image links; the device job prints the numbers on every push.

## The measurement

```sh
W="-w -std=c99 -Os -m32 -ffreestanding -Ibase/include"
cc $W -Iapp -Iapp/dsp -c stub_strata.c -o st_app.o          # a stub that declares the platform, then #include "app.c"
cc $W -O2 -Iapp/dsp -c app/dsp/strata_dsp.c -o st_dsp.o     # likewise the other two
size *.o
nm st_app.o | awk '$2 ~ /[tTbBdDrR]/ {print $3}' | sort -u  # file-scope names, compared with comm -12
```
