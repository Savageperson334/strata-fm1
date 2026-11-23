"""Smoke checks for all three firmware themes and paths.
Run: python3 tests/test_site_nav.py
"""
from pathlib import Path
import importlib.util
import re

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("site_nav", root / "scripts" / "site_nav.py")
site_nav = importlib.util.module_from_spec(spec)
spec.loader.exec_module(site_nav)

FIRMWARES = ("strata", "palindrome", "babblebeat")
SECTIONS = ("", "emu", "manual", "install")
COLORS = {"strata": "#eaf7f7", "palindrome": "#f2edfa", "babblebeat": "#191b23"}

# Nav links are relative to the selected firmware and preserve section.
for current in FIRMWARES:
    for section in SECTIONS:
        n = site_nav.nav(current, section)
        assert n.count('<a href=') == 8, (current, section, n)
        assert 'STRABBLEDROME · WIP' in n
        assert f'href="{site_nav.STRABBLEDROME_SOURCE}"' in n
        assert n.count('data-project="strabbledrome"') == 1
        for target in FIRMWARES:
            assert target.upper() in n
        import posixpath
        a = {"strata": "", "palindrome": "palindrome", "babblebeat": "babblebeat"}
        here = posixpath.join(a[current], section) or "."
        for target in FIRMWARES:
            there = posixpath.join(a[target], section) or "."
            expected = posixpath.relpath(there, start=here)
            expected = "./" if expected == "." else expected + "/"
            assert f'href="{expected}"' in n, (current, target, section, n)
        assert n.count('aria-current="true"') == 1

# Synthetic generated page: theming must not duplicate on repeated publishing.
html = '<!doctype html><html><head><title>Test</title></head><body><header class="site">old</header><main>Hello</main></body></html>'
for current in FIRMWARES:
    out = site_nav.apply(html, current, "")
    assert "fm1theme-style" in out and COLORS[current] in out
    assert out.count("fm1theme-style") == 1
    assert out.count("fm1theme-meta") == 1
    assert "fm1nav" in out and "Hello" in out
    again = site_nav.apply(out, current, "")
    assert again.count("fm1theme-style") == 1
    assert again.count("fm1theme-meta") == 1
    assert again.count('<nav id="fm1nav"') == 1
print("3 firmware themes, shared navigation, and STRABBLEDROME project link passed")
