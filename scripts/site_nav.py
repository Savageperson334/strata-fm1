#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""One navigation on every page of both firmwares' sites, so STRATA's and PALINDROME's mirror each other.

    scripts/site_nav.py SITE_DIR strata|palindrome

SITE_DIR is one firmware's site as fm1-base's tools/make_site.py makes it (index.html, emu/, manual/, install/).
Each of those four pages gets the same block at its top right:

    Emulator   Manual   Install   Source        this firmware's four places, the one you are on marked
              [ STRATA | PALINDROME ]           the other firmware's page of the same kind, one tap away

fm1-base's own header on the front page and the manual (the firmware's name and its links) is replaced by it; the
installer and the emulator had none. Run again on a page that has it: the block is replaced, not doubled."""
import re
import sys
from pathlib import Path

REPO = "https://github.com/Savageperson334/strata-fm1"
SOURCE = {"strata": REPO, "palindrome": REPO + "/tree/main/instruments/palindrome"}
PAGES = (("", "index.html"), ("emu", "emu/index.html"), ("manual", "manual/index.html"),
         ("install", "install/index.html"))
LINKS = (("emu", "Emulator"), ("manual", "Manual"), ("install", "Install"))

# colours come from the page it is on (currentColor), so it sits right on the light pages and the dark ones
STYLE = """<style id="fm1nav-style">
#fm1nav{max-width:46rem;margin:0 auto 1.4rem;padding:12px 0 10px;display:flex;flex-direction:column;align-items:flex-end;
gap:8px;font:600 14px/1.3 -apple-system,BlinkMacSystemFont,"Helvetica Neue",Helvetica,Arial,sans-serif;
border-bottom:2px solid currentColor;box-sizing:border-box}
#fm1nav a{color:inherit;text-decoration:none;background:none}
#fm1nav .fm1nav-links{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:4px 18px}
#fm1nav .fm1nav-links a{opacity:.72;padding:2px 0;border-bottom:2px solid transparent}
#fm1nav .fm1nav-links a:hover,#fm1nav .fm1nav-links a[aria-current]{opacity:1;border-bottom-color:currentColor}
#fm1nav .fm1nav-swap{display:inline-flex;border:1.5px solid currentColor;border-radius:999px;overflow:hidden;
font-size:12px;letter-spacing:.07em}
#fm1nav .fm1nav-swap a{padding:5px 14px;opacity:.72}
#fm1nav .fm1nav-swap a:hover{opacity:1}
#fm1nav .fm1nav-swap a[aria-current]{opacity:1;background:currentColor;background:color-mix(in srgb,currentColor 22%,transparent)}
@media (max-width:48rem){#fm1nav{padding-inline:16px}body>.page #fm1nav{padding-inline:0}}
</style>"""


def nav(instrument, section):
    """the block for one page: section "" (the front page), "emu", "manual" or "install" """
    up = "../" if section else ""                                    # to this firmware's front page
    other = "palindrome" if instrument == "strata" else "strata"
    other_root = up + ("palindrome/" if instrument == "strata" else "../")
    here = lambda s: up + (s + "/" if s else "") or "./"             # noqa: E731
    there = other_root + (section + "/" if section else "")
    links = "".join(f'<a href="{here(s)}"{" aria-current=" + chr(34) + "page" + chr(34) if s == section else ""}>{label}</a>'
                    for s, label in LINKS)
    links += f'<a href="{SOURCE[instrument]}">Source</a>'
    swap = {instrument: f'<a href="{here("")}" aria-current="true">{instrument.upper()}</a>',
            other: f'<a href="{there}">{other.upper()}</a>'}
    return (f'<nav id="fm1nav" aria-label="{instrument.upper()}"><div class="fm1nav-links">{links}</div>'
            f'<div class="fm1nav-swap" role="group" aria-label="Firmware">{swap["strata"]}{swap["palindrome"]}</div></nav>')


def apply(html, instrument, section):
    html = re.sub(r'<style id="fm1nav-style">.*?</style>\s*', "", html, flags=re.S)       # (run again: replaced)
    html = re.sub(r'<nav id="fm1nav".*?</nav>\s*', "", html, flags=re.S)
    html = re.sub(r'<p id="pali(?:mpsest|ndrome)-link">.*?</p>', "", html, flags=re.S)    # (the old one-line pointer)
    block = nav(instrument, section)
    html, n = re.subn(r'<header class="site">.*?</header>', lambda m: block, html, count=1, flags=re.S)   # fm1-base's own
    if not n:
        html, n = re.subn(r"(<body[^>]*>)", lambda m: m.group(1) + "\n" + block, html, count=1)
    if not n:                                                        # (a page with no <body> tag: before its <main>)
        html, n = re.subn(r"(<main\b)", lambda m: block + m.group(1), html, count=1)
    if not n:
        raise SystemExit("site_nav: no place for the navigation")
    if "</head>" in html:
        return html.replace("</head>", STYLE + "\n</head>", 1)
    return re.sub(r"(<style\b)", lambda m: STYLE + m.group(1), html, count=1) if "<style" in html else STYLE + html


def main(site, instrument):
    if instrument not in SOURCE:
        raise SystemExit("site_nav: strata or palindrome")
    done = []
    for section, rel in PAGES:
        f = Path(site) / rel
        if f.is_file():
            f.write_text(apply(f.read_text(encoding="utf-8"), instrument, section), encoding="utf-8")
            done.append(rel)
    print(f"site_nav: {instrument}: {', '.join(done) or 'no pages'}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2])
