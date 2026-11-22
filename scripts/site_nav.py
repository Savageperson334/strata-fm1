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

The emulator also gets a small switch over the panel, [ Names | What they do ]: the second writes on each button
what it does in this firmware in place of the name printed on the FM-1 (a button that does nothing is dimmed)."""
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


# what each button does, in a line or two that fit on it (None: nothing in this firmware)
BUTTONS = {
    "strata": {"FX": None, "SEL": ["HOLD:", "MORE"], "ENV": None, "LFO": None, "EDIT": ["LANES", "HOLD: SET"],
               "GLO": None, "HOME": ["HOLD:", "RESET"], "SAVE": ["SAVE", "SOUND"], "ARP": ["ARP", "HOLD: SET"],
               "SEQ": ["BEAT", "HOLD: SET"], "PLAY": ["FADE OUT", "/ BACK"], "REC": ["MUTATE"],
               "OCT-": ["KEYS DOWN"], "OCT+": ["KEYS UP"]},
    "palindrome": {"FX": ["MORPH", "KNOBS"], "SEL": ["HOLD:", "MIX"], "ENV": None, "LFO": None,
                   "EDIT": ["SILT", "HOLD: MORE"], "GLO": None, "HOME": ["HOLD:", "CLEAR"], "SAVE": None, "ARP": None,
                   "SEQ": None, "PLAY": ["GRAINS", "ON / OFF"], "REC": ["REC MODE", "HOLD: NOISE"],
                   "OCT-": ["KEYS DOWN"], "OCT+": ["KEYS UP"]},
}
LABELS_STYLE = """<style id="fm1labels-style">
#fm1labels{display:inline-flex;margin:0 0 .7rem;border:1.5px solid currentColor;border-radius:999px;overflow:hidden;
font:600 12px/1.3 -apple-system,BlinkMacSystemFont,"Helvetica Neue",Helvetica,Arial,sans-serif;letter-spacing:.07em}
#fm1labels button{all:unset;cursor:pointer;padding:6px 14px;opacity:.72}
#fm1labels button:hover{opacity:1}
#fm1labels button:focus-visible{outline:2px solid currentColor;outline-offset:-4px;border-radius:999px}
#fm1labels button[aria-pressed="true"]{opacity:1;background:currentColor;background:color-mix(in srgb,currentColor 22%,transparent)}
#panel text.fnl{display:none}
#panel.fn text.fnl{display:inline}
#panel.fn g.hasfn>text.nml{display:none}
#panel.fn g.nofn{opacity:.4}
</style>"""
LABELS_HTML = ('<div id="fm1labels" role="group" aria-label="What is written on the buttons">'
               '<button type="button" data-fn="0" aria-pressed="true">Names</button>'
               '<button type="button" data-fn="1" aria-pressed="false">What they do</button></div>')
# (the panel is drawn by the page's own script, a module, after this one: wait for its buttons)
LABELS_JS = """<script id="fm1labels-js">
(function(){
  var FN=__FN__,panel=document.getElementById("panel"),pill=document.getElementById("fm1labels"),tries=0;
  if(!panel||!pill)return;
  function build(){
    var faces=panel.querySelectorAll("rect.btnface");
    if(!faces.length){if(++tries<200)setTimeout(build,50);return;}
    faces.forEach(function(face){
      var g=face.parentNode,texts=g.querySelectorAll("text"),name=texts.length?texts[0].textContent:"";
      if(!(name in FN))return;
      var lines=FN[name],x=+face.getAttribute("x"),y=+face.getAttribute("y"),w=+face.getAttribute("width"),h=+face.getAttribute("height");
      if(!lines){g.classList.add("nofn");return;}
      g.classList.add("hasfn");
      texts.forEach(function(t){t.classList.add("nml");});
      lines.forEach(function(s,i){
        var t=texts[0].cloneNode(false);
        t.classList.remove("nml");t.classList.add("fnl");
        t.setAttribute("font-size",lines.length>1?10.5:12);
        t.setAttribute("letter-spacing",".02em");
        t.setAttribute("x",x+w/2);
        t.setAttribute("y",lines.length>1?y+h/2-1+i*12:y+h/2+4);
        t.textContent=s;
        t.dataset.max=w-5;
        g.appendChild(t);
      });
    });
    set(saved());
  }
  function fit(){                               /* a line wider than its button: squeezed to it */
    panel.querySelectorAll("text.fnl").forEach(function(t){
      try{if(t.getComputedTextLength()>+t.dataset.max){t.setAttribute("textLength",t.dataset.max);t.setAttribute("lengthAdjust","spacingAndGlyphs");}}catch(e){}
    });
  }
  function saved(){try{return localStorage.getItem("fm1labels")==="1";}catch(e){return false;}}
  function set(on){
    panel.classList.toggle("fn",on);
    pill.querySelectorAll("button").forEach(function(b){b.setAttribute("aria-pressed",String((b.dataset.fn==="1")===on));});
    if(on)fit();
    try{localStorage.setItem("fm1labels",on?"1":"0");}catch(e){}
  }
  pill.addEventListener("click",function(e){var b=e.target.closest("button");if(b)set(b.dataset.fn==="1");});
  build();
})();
</script>"""


def emu_labels(html, instrument):
    """the emulator's switch between the buttons' names and what they do"""
    import json
    html = re.sub(r'<style id="fm1labels-style">.*?</style>\s*', "", html, flags=re.S)   # (run again: replaced)
    html = re.sub(r'<div id="fm1labels".*?</div>\s*', "", html, flags=re.S)
    html = re.sub(r'<script id="fm1labels-js">.*?</script>\s*', "", html, flags=re.S)
    html, n = re.subn(r'(<div class="devicebox">)', lambda m: LABELS_HTML + "\n" + m.group(1), html, count=1)
    if not n:
        raise SystemExit("site_nav: no panel in the emulator's page")
    js = LABELS_JS.replace("__FN__", json.dumps(BUTTONS[instrument]))
    html = html.replace("</head>", LABELS_STYLE + "\n</head>", 1)
    return html.replace("</body>", js + "\n</body>", 1) if "</body>" in html else html + js


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
                html = emu_labels(html, instrument)
            if section == "manual" and instrument == "palindrome":
                html = palindrome_manual(html)
            f.write_text(html, encoding="utf-8")
            done.append(rel)
    print(f"site_nav: {instrument}: {', '.join(done) or 'no pages'}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2])
