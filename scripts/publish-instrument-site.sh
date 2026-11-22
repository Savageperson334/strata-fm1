#!/bin/bash
# Preserve independent instrument subtrees across STRATA, PALINDROME and BABBLEBEAT publications.
# (PALINDROME was first published as PALIMPSEST at /palimpsest/: that address now forwards to /palindrome/.)
set -euo pipefail
src="$(realpath "$1")"
instrument="$2"
remote="https://x-access-token:${GH_TOKEN}@github.com/${GITHUB_REPOSITORY}.git"
case "$instrument" in strata|palindrome|babblebeat) ;; *) exit 2;; esac
# The same 3-way navigation on every page (Emulator, Manual, Install, Source).
python3 "$(dirname "$(realpath "$0")")/site_nav.py" "$src" "$instrument"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for attempt in 1 2 3 4 5; do
  rm -rf "$tmp/pages"
  git clone -q --depth 1 --branch gh-pages "$remote" "$tmp/pages"
  cd "$tmp/pages"
  git config user.name "github-actions[bot]"
  git config user.email "41898282+github-actions[bot]@users.noreply.github.com"
  # (--checksum: a page whose only change is the version number has the same size as the one just cloned, and
  # often the same second on its clock, which rsync would otherwise take for unchanged)
  if [ "$instrument" = strata ]; then
    rsync -a --checksum --delete --exclude=.git --exclude=/palindrome/ --exclude=/palimpsest/ --exclude=/babblebeat/ "$src/" ./
  elif [ "$instrument" = palindrome ]; then
    # /palindrome/next is an independently staged experimental emulator.
    # The stable PALINDROME site publication must not delete it.
    mkdir -p palindrome
    rsync -a --checksum --delete --exclude=/next/ "$src/" palindrome/
  else
    mkdir -p "$instrument"
    rsync -a --checksum --delete "$src/" "$instrument/"
  fi
  # The old address: one small page that forwards to the new one.
  if [ -f palindrome/index.html ]; then
    rm -rf palimpsest
    mkdir -p palimpsest
    cat > palimpsest/index.html <<'HTML'
<!doctype html><html lang="en"><meta charset="utf-8"><meta http-equiv="refresh" content="0; url=../palindrome/"><link rel="canonical" href="../palindrome/"><title>PALINDROME</title><p>PALIMPSEST is now called PALINDROME: <a href="../palindrome/">go to PALINDROME</a>.</p></html>
HTML
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
