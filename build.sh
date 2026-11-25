#!/bin/sh
set -eu
cd "$(dirname "$0")"
: "${CC:=cc}"
: "${CFLAGS:=-O2}"
platform=$(uname -s)
suffix=
case "$platform" in
    MINGW*|MSYS*|CYGWIN*) suffix=.exe ;;
esac
export PKG_CONFIG_PATH="$PWD/.deps/raylib/lib/pkgconfig:$PWD/.deps/raylib/lib64/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
# Overrides allow Raylib installed outside pkg-config's search path.
if [ -z "${RAYLIB_CFLAGS+x}" ] || [ -z "${RAYLIB_LIBS+x}" ]; then
    if command -v pkg-config >/dev/null && pkg-config --exists 'raylib >= 5.5'; then
        RAYLIB_CFLAGS=$(pkg-config --cflags raylib)
        RAYLIB_LIBS=$(pkg-config --libs --static raylib)
    elif [ -f .deps/raylib/include/raylib.h ] && [ -f .deps/raylib/lib/libraylib.a ]; then
        RAYLIB_CFLAGS=-I.deps/raylib/include
        case "$platform" in
            Linux) platform_libs='-lGL -lX11 -lpthread -ldl -lrt' ;;
            Darwin)
                platform_libs='-framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo -framework QuartzCore'
                ;;
            MINGW*|MSYS*|CYGWIN*) platform_libs='-static-libgcc -lopengl32 -lgdi32 -lwinmm' ;;
            *) echo "Unsupported Raylib archive platform: $platform" >&2; exit 1 ;;
        esac
        RAYLIB_LIBS=".deps/raylib/lib/libraylib.a $platform_libs"
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
    src/main.c src/audio.c src/input.c src/ui.c src/keyboard.c src/synth.c -o "bin/alt-synth$suffix" ${LDFLAGS:-} $RAYLIB_LIBS -lm
printf 'Built bin/alt-synth%s\n' "$suffix"
