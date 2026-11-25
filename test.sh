#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p bin
# shellcheck disable=SC2086
"${CC:-cc}" -std=c11 -Wall -Wextra -Wpedantic -Werror ${CFLAGS:--O2} \
    -Isrc tests/synth_test.c src/keyboard_layout.c src/synth.c -o bin/synth-test ${LDFLAGS:-} -lm
./bin/synth-test
