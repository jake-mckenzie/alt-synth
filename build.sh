#!/bin/sh
set -eu
cd "$(dirname "$0")"
: "${CC:=cc}"
: "${CFLAGS:=-O2}"
export PKG_CONFIG_PATH="$PWD/.deps/raylib/lib/pkgconfig:$PWD/.deps/raylib/lib64/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
# Overrides allow Raylib installed outside pkg-config's search path.
if [ -z "${RAYLIB_CFLAGS+x}" ] || [ -z "${RAYLIB_LIBS+x}" ]; then
    if command -v pkg-config >/dev/null && pkg-config --exists 'raylib >= 5.5'; then
        RAYLIB_CFLAGS=$(pkg-config --cflags raylib)
        RAYLIB_LIBS=$(pkg-config --libs --static raylib)
    elif [ "$(uname -s)" = Linux ] && [ -f .deps/raylib/include/raylib.h ] &&
         [ -f .deps/raylib/lib/libraylib.a ]; then
        # The official Linux archive ships a static library and headers.
        RAYLIB_CFLAGS=-I.deps/raylib/include
        RAYLIB_LIBS='.deps/raylib/lib/libraylib.a -lGL -lX11 -lpthread -ldl -lrt'
    else
        echo 'Raylib 5.5+ development files are required.' >&2
        echo 'See README.md, or set RAYLIB_CFLAGS and RAYLIB_LIBS for a local installation.' >&2
        exit 1
    fi
fi
mkdir -p bin
# Embed the font so the executable can run from any directory.
{
    echo 'static const unsigned char font_data[] = {'
    od -An -v -tu1 assets/SpaceMono-Bold.ttf | awk '{ for (i=1; i<=NF; ++i) printf "%s,", $i; print "" }'
    echo '};'
} > bin/font_data.h
# Flag variables intentionally split into compiler arguments.
# shellcheck disable=SC2086
"$CC" -std=c11 -Wall -Wextra -Wpedantic $CFLAGS ${CPPFLAGS:-} $RAYLIB_CFLAGS -Ibin \
    src/main.c src/audio.c src/input.c src/ui.c src/keyboard.c src/synth.c -o bin/alt-synth ${LDFLAGS:-} $RAYLIB_LIBS -lm
printf 'Built bin/alt-synth\n'
