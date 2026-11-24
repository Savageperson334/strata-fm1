#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Shared website navigation across three released instruments and one planned combined project.

    scripts/site_nav.py SITE_DIR strata|palindrome|babblebeat

SITE_DIR is one firmware's site as fm1-base's tools/make_site.py makes it (index.html, emu/, manual/, install/).
Single shared navigation across three firmwares, plus a per-firmware colour
system injected into each generated GitHub Pages page. STRATA is teal,
PALINDROME violet, and BABBLEBEAT copper on charcoal. Switching instruments
preserves Emulator / Manual / Install context.

Each of those four pages gets the same block at its top right:

    Emulator   Manual   Install   Source        this firmware's four places, the one you are on marked
              [ STRATA | PALINDROME | BABBLEBEAT ]           the other firmware's page of the same kind, one tap away

fm1-base's own header on the front page and the manual (the firmware's name and its links) is replaced by it; the
installer and the emulator had none. Run again on a page that has it: the block is replaced, not doubled.

The emulator also gets a switch over the panel, [ Play | Tutorial ]: Tutorial draws a blueprint over the panel, a line
from each control to a box that says what it does in this firmware; a tap there reads about a control instead of
playing it. PALINDROME's has a third, Guide, which leads through the first rings on the live panel (scripts/tutorial/)."""
import re
import sys
from pathlib import Path

REPO = "https://github.com/Savageperson334/strata-fm1"
SOURCE = {"strata": REPO, "palindrome": REPO + "/tree/main/instruments/palindrome", "babblebeat": REPO + "/tree/main/instruments/babblebeat",
          "strabbledrome": REPO + "/tree/main/instruments/strabbledrome"}
STRABBLEDROME_SOURCE = SOURCE["strabbledrome"]
# every firmware has an installer now (STRABBLEDROME's: all three in one install)
NO_INSTALL = set()
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
#fm1nav .fm1nav-swap a[data-project="strabbledrome"]{border-left:1px solid currentColor;letter-spacing:0}
#fm1nav .fm1nav-swap a[aria-current]{opacity:1;background:currentColor;background:color-mix(in srgb,currentColor 22%,transparent)}
@media (max-width:48rem){#fm1nav{padding-inline:16px}body>.page #fm1nav{padding-inline:0}}
</style>"""


# The base site's generated CSS supplies the layout. These semantic variables
# recolour the page, emulator surround and manuals without recolouring the
# simulated FM-1 hardware itself. Use the same palette per firmware/section.
THEMES = {
    "strata": {
        "paper": "#eaf7f7", "sheet": "#d5ebed", "rule": "#b7d3da",
        "ink": "#143847", "muted": "#486675", "hot": "#007d90",
        "hot-t": "#d1ebef", "hot-ink": "#086879", "cool": "#338c78",
        "cool-t": "#d8eee7", "ground": "#e7f5f6", "card": "#faffff",
        "accent": "#086f88", "teal": "#096f71", "bad": "#ba4757",
        "scheme": "light"
    },
    "palindrome": {
        "paper": "#f2edfa", "sheet": "#e5dcf2", "rule": "#cfc0e2",
        "ink": "#34264e", "muted": "#6e6087", "hot": "#8358b1",
        "hot-t": "#e8ddf4", "hot-ink": "#683e96", "cool": "#5d75a4",
        "cool-t": "#dbe4f4", "ground": "#eee8f8", "card": "#fcfaff",
        "accent": "#7b52a8", "teal": "#5b5fa4", "bad": "#a73e63",
        "scheme": "light"
    },
    "babblebeat": {
        "paper": "#191b23", "sheet": "#292d37", "rule": "#4c4c5a",
        "ink": "#f1e9de", "muted": "#bcb4a9", "hot": "#f6a873",
        "hot-t": "#43332f", "hot-ink": "#ffc19b", "cool": "#9dc7ad",
        "cool-t": "#303d37", "ground": "#191b23", "card": "#272b35",
        "accent": "#eda06b", "teal": "#a8d7bd", "bad": "#ff9696",
        "scheme": "dark"
    },
    "strabbledrome": {
        "paper": "#f1f1ee", "sheet": "#e2e2dd", "rule": "#c6c6bf",
        "ink": "#22252b", "muted": "#5b606b", "hot": "#3f5873",
        "hot-t": "#dde3ea", "hot-ink": "#2f4660", "cool": "#5c6f58",
        "cool-t": "#e0e6dd", "ground": "#eeeeea", "card": "#fbfbf9",
        "accent": "#3a536e", "teal": "#4b6a67", "bad": "#a8484e",
        "scheme": "light"
    }
}

THEME_RULES = """
html { color-scheme: var(--fm1-scheme); }
body { background: var(--paper) !important; color: var(--ink) !important; }
body:has(.devicebox) { background: var(--ground) !important; }
a { text-decoration-color: var(--hot); }
a:focus-visible, button:focus-visible { outline-color: var(--hot-ink); }
#fm1nav {
  max-width: 60rem; color: var(--ink); border-bottom: 2px solid var(--rule);
  padding: 10px 0 14px; margin-bottom: 1.6rem;
}
#fm1nav a { color: var(--ink); }
#fm1nav .fm1nav-swap { border-color: var(--accent); }
#fm1nav .fm1nav-swap a { color: var(--ink); padding: 7px 11px; }
#fm1nav .fm1nav-swap a:hover { background: var(--hot-t); }
#fm1nav .fm1nav-swap a[aria-current] {
  background: var(--accent); color: var(--card); opacity: 1;
  font-weight: 750;
}
body:has(.devicebox) > header h1 { color: var(--accent); }
@media(max-width:540px) {
  #fm1nav { align-items: stretch; }
  #fm1nav .fm1nav-links { justify-content: space-between; gap: 8px; }
  #fm1nav .fm1nav-swap { display: flex; flex-wrap: wrap; width: 100%; border-radius:12px; }
  #fm1nav .fm1nav-swap a { flex: 1 1 42%; text-align: center; padding: 8px 5px; font-size: 11px; }
  #fm1nav .fm1nav-swap a[data-project="strabbledrome"] { border-left:0; border-top:1px solid var(--rule); }
}
"""

def theme_style(instrument):
    colors = THEMES[instrument]
    pairs = " ".join(
        f'--{key}: {value};' for key, value in colors.items() if key != "scheme"
    )
    pairs += f' --fm1-scheme: {colors["scheme"]};'
    return ('<meta id="fm1theme-meta" name="theme-color" content="' +
            colors["ground"] + '">\n<style id="fm1theme-style">\n' +
            ':root { ' + pairs + ' }\n' + THEME_RULES + '\n</style>')


TUTORIAL = Path(__file__).resolve().parent / "tutorial"


def mode_html(data):
    """the switch over the panel: Play, Guide (a firmware whose tutorial has one) and Tutorial / Reference, and the card"""
    names = [("play", "Play")] + ([("guide", data["guide"].get("label", "Guide"))] if data.get("guide") else [])
    names.append(("tutorial", data.get("tutorial_label", "Tutorial")))
    return ('<div id="fm1mode" role="group" aria-label="Play the panel, be guided on it, or read what each control does">'
            + "".join(f'<button type="button" data-mode="{m}" aria-pressed="{"true" if m == "play" else "false"}">{t}</button>'
                      for m, t in names)
            + '</div>\n<div id="fm1tut-card" aria-live="polite" hidden></div>')


def emu_tutorial(html, instrument):
    """the emulator's Play / Tutorial switch: scripts/tutorial/ (tutorial.js, tutorial.css, INSTRUMENT.json)"""
    import json
    for old in (r'<style id="fm1labels-style">.*?</style>\s*', r'<div id="fm1labels".*?</div>\s*',       # (0.5's switch)
                r'<script id="fm1labels-js">.*?</script>\s*', r'<style id="fm1tut-style">.*?</style>\s*',
                r'<div id="fm1mode".*?</div>\s*<div id="fm1tut-card"[^>]*></div>\s*', r'<script id="fm1tut-js">.*?</script>\s*'):
        html = re.sub(old, "", html, flags=re.S)                     # (run again: replaced)
    data = json.loads((TUTORIAL / f"{instrument}.json").read_text(encoding="utf-8"))
    html, n = re.subn(r'(<div class="devicebox">)', lambda m: mode_html(data) + "\n" + m.group(1), html, count=1)
    if not n:
        raise SystemExit("site_nav: no panel in the emulator's page")
    js = (TUTORIAL / "tutorial.js").read_text(encoding="utf-8").replace("__TUTORIAL__", json.dumps(data).replace("</", "<\\/"))
    css = (TUTORIAL / "tutorial.css").read_text(encoding="utf-8")
    html = html.replace("</head>", f'<style id="fm1tut-style">\n{css}</style>\n</head>', 1)
    block = f'<script id="fm1tut-js">\n{js}</script>'
    return html.replace("</body>", block + "\n</body>", 1) if "</body>" in html else html + block


def nav(instrument, section):
    """Cross-links among STRATA, PALINDROME, and BABBLEBEAT, in any section."""
    import posixpath
    roots = {"strata": "", "palindrome": "palindrome", "babblebeat": "babblebeat", "strabbledrome": "strabbledrome"}
    current_dir = posixpath.join(roots[instrument], section) if section else roots[instrument]
    def relative(target_instrument, target_section):
        if target_section == "install" and target_instrument in NO_INSTALL:
            target_section = ""                     # (no installer there: its front page)
        target = posixpath.join(roots[target_instrument], target_section)
        rel = posixpath.relpath(target or ".", start=current_dir or ".")
        return "./" if rel == "." else rel + "/"
    links = "".join(
        f'<a href="{relative(instrument, s)}"{" aria-current=" + chr(34) + "page" + chr(34) if s == section else ""}>{label}</a>'
        for s, label in LINKS if not (s == "install" and instrument in NO_INSTALL)
    )
    links += f'<a href="{SOURCE[instrument]}">Source</a>'
    swap = "".join(
        f'<a href="{relative(name, section)}"{" aria-current=" + chr(34) + "true" + chr(34) if name == instrument else ""}>{name.upper()}</a>'
        for name in ("strata", "palindrome", "babblebeat")
    )
    # STRABBLEDROME: all three behind one menu, in one install.
    swap += (f'<a href="{relative("strabbledrome", section)}" data-project="strabbledrome"'
             f'{" aria-current=" + chr(34) + "true" + chr(34) if instrument == "strabbledrome" else ""} '
             f'title="All three in one firmware, one install">STRABBLEDROME</a>')
    return (f'<nav id="fm1nav" aria-label="{instrument.upper()}"><div class="fm1nav-links">{links}</div>'
            f'<div class="fm1nav-swap" role="group" aria-label="Firmware projects">{swap}</div></nav>')


def apply(html, instrument, section):
    html = re.sub(r'<style id="fm1nav-style">.*?</style>\s*', "", html, flags=re.S)       # (run again: replaced)
    html = re.sub(r'<style id="fm1theme-style">.*?</style>\s*', "", html, flags=re.S)
    html = re.sub(r'<meta id="fm1theme-meta"[^>]*>\s*', "", html, flags=re.S)
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
    additions = STYLE + "\n" + theme_style(instrument)
    if "</head>" in html:
        return html.replace("</head>", additions + "\n</head>", 1)
    return re.sub(r"(<style\b)", lambda m: additions + "\n" + m.group(1), html, count=1) if "<style" in html else additions + html



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
    html = re.sub(r'<div class="palindrome-modes">\s*<section class="palindrome-mode (?:morph|silt)"[^>]*>', "", html)
    html = re.sub(r'</section>\s*<section class="palindrome-mode (?:morph|silt)"[^>]*>', "", html)
    html = re.sub(r'</section>\s*</div>(?=\s*<h2\b)', "", html)
    found = sorted((m.start(), name, m) for name in ("morph", "silt")
                   for m in [re.search(rf'<h2 id="{name}">', html)] if m)
    if len(found) != 2:
        return html
    (a, first, _), (b, second, m2) = found           # (whichever comes first in the manual: SILT, since the redesign)
    end = re.search(r'<h2\b', html[m2.end():])
    if not end:
        return html
    c = m2.end() + end.start()
    html = (html[:a] + '<div class="palindrome-modes">' +
            f'<section class="palindrome-mode {first}" aria-labelledby="{first}">' + html[a:b] + '</section>' +
            f'<section class="palindrome-mode {second}" aria-labelledby="{second}">' + html[b:c] + '</section></div>' + html[c:])
    return html.replace("</head>", MANUAL_STYLE + "\n</head>", 1)


def main(site, instrument):
    if instrument not in SOURCE:
        raise SystemExit("site_nav: strata, palindrome, babblebeat or strabbledrome")
    done = []
    for section, rel in PAGES:
        f = Path(site) / rel
        if f.is_file():
            html = apply(f.read_text(encoding="utf-8"), instrument, section)
            if section == "emu" and (TUTORIAL / f"{instrument}.json").is_file():   # (STRABBLEDROME has none)
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
