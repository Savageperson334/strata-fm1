#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""One navigation on every page of both firmwares' sites, so STRATA's and PALINDROME's mirror each other.

    scripts/site_nav.py SITE_DIR strata|palindrome

SITE_DIR is one firmware's site as fm1-base's tools/make_site.py makes it (index.html, emu/, manual/, install/).
Each of those four pages gets the same block at its top right:

    Emulator   Manual   Install   Source        this firmware's four places, the one you are on marked
              [ STRATA | PALINDROME ]           the other firmware's page of the same kind, one tap away

fm1-base's own header on the front page and the manual (the firmware's name and its links) is replaced by it; the
installer and the emulator had none. Run again on a page that has it: the block is replaced, not doubled.

The emulator also gets a switch over the panel, [ Play | Tutorial ]: Tutorial draws a blueprint over the panel, a line
from each control to a box that says what it does in this firmware; a tap there reads about a control instead of
playing it (scripts/tutorial/)."""
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


TUTORIAL = Path(__file__).resolve().parent / "tutorial"
MODE_HTML = ('<div id="fm1mode" role="group" aria-label="Play the panel, or read what each control does">'
             '<button type="button" data-mode="play" aria-pressed="true">Play</button>'
             '<button type="button" data-mode="tutorial" aria-pressed="false">Tutorial</button></div>\n'
             '<div id="fm1tut-card" aria-live="polite" hidden></div>')


def emu_tutorial(html, instrument):
    """the emulator's Play / Tutorial switch: scripts/tutorial/ (tutorial.js, tutorial.css, INSTRUMENT.json)"""
    import json
    for old in (r'<style id="fm1labels-style">.*?</style>\s*', r'<div id="fm1labels".*?</div>\s*',       # (0.5's switch)
                r'<script id="fm1labels-js">.*?</script>\s*', r'<style id="fm1tut-style">.*?</style>\s*',
                r'<div id="fm1mode".*?</div>\s*<div id="fm1tut-card"[^>]*></div>\s*', r'<script id="fm1tut-js">.*?</script>\s*'):
        html = re.sub(old, "", html, flags=re.S)                     # (run again: replaced)
    html, n = re.subn(r'(<div class="devicebox">)', lambda m: MODE_HTML + "\n" + m.group(1), html, count=1)
    if not n:
        raise SystemExit("site_nav: no panel in the emulator's page")
    data = json.loads((TUTORIAL / f"{instrument}.json").read_text(encoding="utf-8"))
    js = (TUTORIAL / "tutorial.js").read_text(encoding="utf-8").replace("__TUTORIAL__", json.dumps(data).replace("</", "<\\/"))
    css = (TUTORIAL / "tutorial.css").read_text(encoding="utf-8")
    html = html.replace("</head>", f'<style id="fm1tut-style">\n{css}</style>\n</head>', 1)
    block = f'<script id="fm1tut-js">\n{js}</script>'
    return html.replace("</body>", block + "\n</body>", 1) if "</body>" in html else html + block


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



# PALINDROME's manual is written as ordinary Markdown for GitHub. On Pages,
# group its two independent modes into a responsive two-column quick reference.
MANUAL_STYLE = """<style id="palindrome-manual-style">
body:has(.palindrome-modes){font-size:16px;line-height:1.5}
body:has(.palindrome-modes) main,body:has(.palindrome-modes) #fm1nav{max-width:72rem}
.palindrome-modes{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:clamp(16px,3vw,36px);align-items:start;margin:1.6rem 0 2rem}
.palindrome-mode{min-width:0;border-top:4px solid var(--mode);padding-top:12px}
.palindrome-mode.morph{--mode:#226b91}
.palindrome-mode.silt{--mode:#79549c}
.palindrome-mode h2{color:var(--mode);border:0;padding:0;margin:0 0 .35rem;font-size:1.55rem;letter-spacing:.07em}
.palindrome-mode h3{margin:1.2rem 0 .55rem}
.palindrome-mode .figs{margin:.7rem 0 1rem}
.palindrome-mode figure.shot img{width:min(100%,240px)}
.palindrome-mode figure.shot figcaption{font-size:.83rem}
.palindrome-mode .table{margin-bottom:1rem}
body:has(.palindrome-modes) table{font-size:.91rem}
body:has(.palindrome-modes) td,body:has(.palindrome-modes) th{padding:.32rem .45rem .32rem 0}
body:has(.palindrome-modes) main>h2{margin-top:2rem}
body:has(.palindrome-modes) main>.figs figure.shot img{width:220px}
@media(max-width:850px){.palindrome-modes{grid-template-columns:1fr;gap:24px}.palindrome-mode figure.shot img{width:min(100%,240px)}}
</style>"""


def palindrome_manual(html):
    """Group the rendered MORPH and SILT sections without duplicating the Markdown.
    A missing heading leaves the original manual intact and readable."""
    html = re.sub(r'<style id="palindrome-manual-style">.*?</style>\s*', "", html, flags=re.S)
    # Idempotent: undo our own wrappers before grouping again.
    html = re.sub(r'<div class="palindrome-modes">\s*<section class="palindrome-mode morph"[^>]*>', "", html)
    html = re.sub(r'</section>\s*<section class="palindrome-mode silt"[^>]*>', "", html)
    html = re.sub(r'</section>\s*</div>(?=\s*<h2\b)', "", html)
    morph = re.search(r'<h2 id="morph">', html)
    silt = re.search(r'<h2 id="silt">', html)
    if not morph or not silt or morph.start() >= silt.start():
        return html
    end = re.search(r'<h2\b', html[silt.end():])
    if not end:
        return html
    a, b, c = morph.start(), silt.start(), silt.end() + end.start()
    html = (html[:a] + '<div class="palindrome-modes">' +
            '<section class="palindrome-mode morph" aria-labelledby="morph">' + html[a:b] + '</section>' +
            '<section class="palindrome-mode silt" aria-labelledby="silt">' + html[b:c] + '</section></div>' + html[c:])
    return html.replace("</head>", MANUAL_STYLE + "\n</head>", 1)


def main(site, instrument):
    if instrument not in SOURCE:
        raise SystemExit("site_nav: strata or palindrome")
    done = []
    for section, rel in PAGES:
        f = Path(site) / rel
        if f.is_file():
            html = apply(f.read_text(encoding="utf-8"), instrument, section)
            if section == "emu":
                html = emu_tutorial(html, instrument)
            if section == "manual" and instrument == "palindrome":
                html = palindrome_manual(html)
            f.write_text(html, encoding="utf-8")
            done.append(rel)
    print(f"site_nav: {instrument}: {', '.join(done) or 'no pages'}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2])
