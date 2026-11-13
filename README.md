# alt-synth

A small polyphonic desktop synthesizer written in C with Raylib. Play a piano from
your computer keyboard or mouse. Held notes light up on screen; the waveform shows
the audio output and the pitch display shows active voices.

## Build and run

Requires a C11 compiler, `pkg-config`, and Raylib 5.5 or later with its development
headers. No Make or Go toolchain is used.

```sh
./build.sh
./bin/alt-synth
```

The font is embedded in the executable, so you can launch it from any directory.
Raylib provides windowing and audio; its native libraries must be available at
runtime if you link it dynamically. The app can still display the keyboard when
an audio device is unavailable.

If Raylib is not installed, install your distribution's Raylib development
package, or build it once from the official source with CMake and Ninja. On
Debian/Ubuntu, the source build needs these packages:

```sh
sudo apt-get install build-essential pkg-config cmake ninja-build \
    libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev
```

From the alt-synth directory, the following commands download Raylib 5.5 and
install a static library locally in `.deps/raylib` (ignored by Git):

```sh
git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git .deps/raylib-source
cmake -S .deps/raylib-source -B .deps/raylib-build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX="$PWD/.deps/raylib"
cmake --build .deps/raylib-build
cmake --install .deps/raylib-build
./build.sh
```

`build.sh` discovers this local installation automatically. For another location,
set `PKG_CONFIG_PATH`, or supply both `RAYLIB_CFLAGS` and `RAYLIB_LIBS`.
`CC`, `CPPFLAGS`, `CFLAGS`, and `LDFLAGS` are also supported.

## Playing

| Controls | Action |
| --- | --- |
| `A S D F G H J K L ; '` | White keys, from C through the following F |
| `W E T Y U O P` | Black keys in the row above their white-key gaps |
| Filter button / `Tab` | Cycle piano, warm keys, bell, and soft synth |
| Left mouse button | Hold or drag across the piano to play |
| `Space` | Hold the sustain pedal |
| `←` / `→` | Shift the keyboard down / up an octave |
| `-` / `=` | Decrease / increase volume |
| `Backspace` | Silence all notes |
| `F1` | Open / close help |
| `Esc` | Close help, or quit |

The initial range is C3–F4, with all 11 white keys on the home row. Play several
keys together for chords. Amber keys are held; green keys are sustained. Releasing Space releases sustained notes.
Changing octave clears previous notes. Opening help or leaving the window silences
notes; release the playing controls before starting again. Computer keyboards may
limit how many simultaneous key presses they can report.

The default **Piano** tone uses a hammer-like attack and decaying string harmonics:
bright overtones fade quickly, leaving a quieter fundamental and string tail.
The **Filter** button cycles Piano, Warm keys, Bell, and Soft synth. Soft synth
uses rounded, detuned saw waves with a softer attack and longer release; it stays
full while a key is held. These sounds are synthesized rather than sampled.
Switching presets preserves held notes and crossfades briefly.

The pitch bars show synthesizer voice gate levels on a fixed C1–C8 axis, not an
FFT or input from a microphone. The keyboard uses MIDI note numbers internally; external
MIDI devices are not connected yet.

## Development

```sh
./test.sh
CFLAGS='-O1 -g -fsanitize=address,undefined' ./test.sh
CFLAGS='-O0 -g' ./build.sh
```

Tests cover note frequencies, keyboard geometry and hit detection, polyphonic
output, sound presets, piano overtone decay and re-strikes, sustained synth
harmonics, sustain, release envelopes, and silence. They run without Raylib or a
display. Check the window, held-key highlighting, and audio manually after a UI
change. Remove `bin/` to clean app build output; keep `.deps/` to retain Raylib.

`src/main.c` handles the Raylib interface and input. `src/synth.c` contains note
math, keyboard geometry, and synthesis, independent of the display and audio device.

The bundled Space Mono Bold font uses the
[SIL Open Font License](assets/OFL.txt). [Raylib](https://www.raylib.com/) uses the
zlib/libpng license.
