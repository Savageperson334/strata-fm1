#!/bin/bash
# Preserve independent instrument subtrees across STRATA and PALIMPSEST publications.
set -euo pipefail
src="$(realpath "$1")"
instrument="$2"
remote="https://x-access-token:${GH_TOKEN}@github.com/${GITHUB_REPOSITORY}.git"
case "$instrument" in strata|palimpsest) ;; *) exit 2;; esac
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for attempt in 1 2 3 4 5; do
  rm -rf "$tmp/pages"
  git clone -q --depth 1 --branch gh-pages "$remote" "$tmp/pages"
  cd "$tmp/pages"
  git config user.name "github-actions[bot]"
  git config user.email "41898282+github-actions[bot]@users.noreply.github.com"
  if [ "$instrument" = strata ]; then
    rsync -a --delete --exclude=.git --exclude=/palimpsest/ "$src/" ./
  else
    mkdir -p palimpsest
    rsync -a --delete "$src/" palimpsest/
  fi
  # Keep the existing STRATA homepage and URL. Add a visible instrument selector.
  if [ -f palimpsest/index.html ] && [ -f index.html ]; then
    python3 - <<'PY'
from pathlib import Path
p = Path("index.html")
s = p.read_text()
if 'id="palimpsest-link"' not in s:
    banner = '<p id="palimpsest-link"><strong>Alternate instrument:</strong> <a href="palimpsest/">PALIMPSEST — vector synthesis &amp; granular manipulation</a> · <a href="palimpsest/emu/">Play emulator</a></p>'
    pos = s.find("</h1>")
    if pos != -1:
        s = s[:pos+5] + banner + s[pos+5:]
        p.write_text(s)
PY
  fi
  touch .nojekyll
  git add -A
  if git diff --cached --quiet; then exit 0; fi
  git commit -q -m "Publish $instrument from $GITHUB_SHA"
  if git push -q "$remote" HEAD:gh-pages; then exit 0; fi
  cd "$tmp"
  sleep "$attempt"
done
echo "Pages advanced repeatedly; refusing to overwrite another instrument." >&2
exit 1
