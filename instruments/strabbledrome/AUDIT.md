# STRABBLEDROME: memory and symbol audit

Milestone 2 of [the plan](README.md). Measured 23 November 2026 on STRATA 0.4.2, PALINDROME 0.9 and BabbleBeat 0.4.

**Result: all three fit in one image.** No loader and no second flash region are needed. One buffer has to be shared, and the three apps have to be compiled apart.

## Where the numbers come from

- **Device totals** are the FM-1 builds in CI (`tests.yml`, job `device`): the linker's own report.
- **Each instrument's own share** is from compiling its sources alone as 32-bit objects and reading `size` (the script is at the end). Code size on the FM-1's processor will differ by some percent; data sizes will not.
- The platform's share is a device total less that instrument's own share. The three subtractions agree with each other, which is the check.

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
| STRATA | 29,876 | about 5,500 |
| PALINDROME | 33,180 | about 8,900 |
| BabbleBeat | 35,220 | about 12,100 |
| The platform | | about 24,000 |
| **Combined** | | **about 51,000 of 98,304** |

## Flash (the app's slot is about 581 KiB)

Each image is about 102 to 108 KB, most of it the platform. Each instrument's own code and tables come to roughly 30 to 55 KB. Combined: **about 180 to 200 KB**, a third of the slot.

## Symbols

- The platform compiles an app by `#include "app.c"` inside its own single unit, and its drawing calls (`cv_begin`, `cv_text`, the fonts, the palette) are `static` there. Three `app.c` files in that one unit collide: STRATA and PALINDROME share 71 file-scope names, STRATA and BabbleBeat 45, PALINDROME and BabbleBeat 63.
- The sound engines' own global names (`sd_`, `pm_`, `bb_`) do not collide at all.
- The platform settings the three ask for agree: the same block size (256) and the same palette.
- STRATA's store map contains the other two's, so STRATA's saved sound can stay where standalone STRATA keeps it.

**So: each instrument's `app.c` becomes its own unit**, listed with the engines (`o2` in `fm1.toml`), with its seven `app_` calls renamed by macro (`strata_init` and so on) and the dozen drawing calls it needs handed to it by the launcher as ordinary functions. The instruments' sources are not edited for this, and their standalone builds are untouched. Only the two pool buffers need a line each, behind an `#ifdef`.

## What is not settled by this audit

- Whether each instrument starts clean when it is started a second time. Anything it sets only by a static initialiser, and not in its `app_init`, will be stale. To be checked instrument by instrument as it goes in.
- How Help shows an instrument's own button guide. The plan: the launcher presents GLO to the instrument for as long as Help is open.
- The real numbers. These are estimates until the combined image links; the device job prints them on every push.

## The measurement

```sh
W="-w -std=c99 -Os -m32 -ffreestanding -Ibase/include"
cc $W -Iapp -Iapp/dsp -c stub_strata.c -o st_app.o          # a stub that declares the platform, then #include "app.c"
cc $W -O2 -Iapp/dsp -c app/dsp/strata_dsp.c -o st_dsp.o     # likewise the other two
size *.o
nm st_app.o | awk '$2 ~ /[tTbBdDrR]/ {print $3}' | sort -u  # file-scope names, compared with comm -12
```
