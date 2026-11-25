#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p bin
# shellcheck disable=SC2086
"${CC:-cc}" -std=c11 -Wall -Wextra -Wpedantic -Werror ${CFLAGS:--O2} \
    -DALT_SYNTH_HEADLESS -Isrc tests/synth_test.c src/keyboard.c src/synth.c -o bin/synth-test ${LDFLAGS:-} -lm
./bin/synth-test
