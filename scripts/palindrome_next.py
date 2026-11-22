#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""PALINDROME's unreleased preview at /palindrome/next/ (.github/workflows/palindrome-next.yml).

    scripts/palindrome_next.py DIR COMMIT

DIR holds emu/ (tools/build.py --web), img/ (the simulator's screens) and manual.md. The emulator's page gets the
Play / Guide / Reference switch as the released site's does (site_nav.emu_tutorial) and a line saying it is a
preview; DIR/index.html is a small front page for it."""
import html
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import site_nav                                      # noqa: E402

NOTE = ('<p id="pal-next-note" style="margin:0 0 .8rem;padding:.5rem .8rem;border:1.5px dashed currentColor;'
        'border-radius:6px;font-size:.95rem">Preview of PALINDROME\'s SILT-first redesign, build {commit}: '
        'not released, not on any FM-1. <a href="../">About this preview</a> · '
        '<a href="../../emu/">The released emulator</a></p>')


def main(out, commit):
    out = Path(out)
    emu = out / "emu" / "index.html"
    page = site_nav.emu_tutorial(emu.read_text(encoding="utf-8"), "palindrome")
    page = page.replace('<div id="fm1mode"', NOTE.format(commit=html.escape(commit[:7])) + '\n<div id="fm1mode"', 1)
    emu.write_text(page, encoding="utf-8")
    shots = "".join(f'<figure><img src="img/{n}.png" alt="{html.escape(a)}" width="240" height="240"><figcaption>{html.escape(a)}'
                    "</figcaption></figure>"
                    for n, a in (("silt0", "SILT, nothing captured yet"), ("morph", "MORPH, the source"),
                                 ("capture", "REC: ring 01 filling"), ("silt1", "Ring 01"), ("silt", "Rings 01 and 02"),
                                 ("silt3", "Three rings"), ("rings", "The rings and where they came from"),
                                 ("delete", "A delete, armed"), ("shape", "SILT SHAPE"), ("motion", "SILT MOTION"),
                                 ("mix", "MIX"), ("recfx", "REC FX")) if (out / "img" / f"{n}.png").exists())
    (out / "index.html").write_text(f"""<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><meta name="robots" content="noindex">
<title>PALINDROME next (preview)</title>
<style>body{{background:#151420;color:#eadff7;font:17px/1.5 system-ui,sans-serif;max-width:46rem;margin:2.5rem auto;padding:0 1.2rem}}
a{{color:#c7a6fa}}h1{{letter-spacing:.08em;margin:0 0 .3rem}}figure{{display:inline-block;margin:.4rem;vertical-align:top;width:240px;max-width:45%}}
img{{width:100%;height:auto;image-rendering:pixelated;border:1px solid #3a3550}}figcaption{{font-size:.85rem;color:#c4b6d0}}
.go{{display:inline-block;margin:.6rem 0;padding:.6rem 1.2rem;border-radius:999px;background:#c7a6fa;color:#151420;font-weight:700;text-decoration:none}}</style>
<main><h1>PALINDROME next</h1>
<p><strong>SILT is the instrument. MORPH is the injector. Each recording becomes a new ring.</strong></p>
<p>This is a preview of the redesign, build {html.escape(commit[:7])}. It is not released: there is no installer for
it, and it has never run on an FM-1. <a href="../">The released PALINDROME</a> is unchanged.</p>
<p><a class="go" href="emu/">Play the preview in the browser</a></p>
<p>In the emulator, <strong>Guide</strong> walks through the first two rings on the live panel;
<strong>Reference</strong> says what every control does. One press does everything: nothing is held.
<a href="https://github.com/Savageperson334/strata-fm1/blob/feature/silt-first/instruments/palindrome/docs/MANUAL.md">The
manual for this build</a> is on its branch.</p>
<h2>The screens, from the simulator</h2>{shots}</main></html>
""", encoding="utf-8")
    print(f"palindrome_next: {out}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2])
