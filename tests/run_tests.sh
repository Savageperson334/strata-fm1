#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# STRATA's tests:
#   dsp        the sound engine alone: silence, fades, clicks, every control swept, an hour of drone (tests/dsp_render.c)
#   app-logic  app.c on a stand-in platform: keys latch, PLAY, the SEL layer, SAVE across a restart, MIDI
#   host-*     the whole firmware in fm1-base's simulator (app/tests/basic.txt), and its screenshots
#   update     the update path against STRATA's own package (after ./build.sh)
#   tests/run_tests.sh
set -u
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
PY="${PYTHON:-python3}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
B=app/build
OUT=$B/tests
FAIL=0
mkdir -p "$OUT" $B/scenarios/basic
run() {
    name="$1"; shift
    if "$@" > "$OUT/$name.log" 2>&1; then
        echo "  ok   $name"
    else
        echo "  FAIL $name (see $OUT/$name.log)"
        tail -20 "$OUT/$name.log" | sed 's/^/       /'
        FAIL=1
    fi
}
run dsp sh -c "$CC -O2 -ffp-contract=off -std=c99 -DPLAT_HOST -Ibase/include -Iapp/dsp -o $OUT/dsp_render \
    tests/dsp_render.c app/dsp/strata_dsp.c -lm && $OUT/dsp_render $OUT"
run app-logic sh -c "$CC -std=gnu99 -w -DPLAT_HOST -Iapp -Iapp/dsp -Ibase/include -o $OUT/app_logic \
    tests/app_logic.c app/dsp/strata_dsp.c -lm && $OUT/app_logic"
if [ -f $B/strata.fwsc ]; then
    run ota-entry sh -c "$CC -w -o $OUT/ota_test base/tests/ota_test.c && $OUT/ota_test $B/strata.fwsc"
    run update-loader sh -c "head -c 100000 $B/strata.bin > $OUT/old_app.bin && \
        $PY base/tools/fm1pkg_make.py $OUT/old_app.bin $B/loader/ota.bin $OUT/old.fwsc >/dev/null && \
        $CC -w -o $OUT/ldr_test base/tests/ldr_test.c && $OUT/ldr_test $OUT/old.fwsc $B/strata.fwsc"
else
    echo "  skip update path (no $B/strata.fwsc: ./build.sh)"
fi
run host-build sh build.sh --host
run host-basic $B/host/strata_host app/tests/basic.txt $B/scenarios/basic
[ $FAIL -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
exit $FAIL
