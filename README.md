# alt-synth

A polyphonic desktop synth in C and Raylib. Play from your keyboard or mouse,
with piano and QWERTY displays, a waveform, and active-note levels.

![alt-synth piano view](docs/images/piano-view.png)

![alt-synth QWERTY keyboard view](docs/images/qwerty-view.png)

## Releases

[GitHub Releases](https://github.com/jake-mckenzie/alt-synth/releases) provides
Linux x86-64 packages. Extract the archive and run `./alt-synth/alt-synth`.
Requires glibc 2.35+, an X11 display or XWayland, and system graphics/audio libraries.
Check the download against the release's `checksums.txt`.

## Build and run

Requires a C11 compiler, `pkg-config`, and Raylib 5.5+ development files.

```sh
./build.sh
./bin/alt-synth
```

The font is embedded. No Go or Make is needed. `build.sh` finds Raylib through
`pkg-config` or `.deps/raylib`; custom installations can set `PKG_CONFIG_PATH`
or both `RAYLIB_CFLAGS` and `RAYLIB_LIBS`.

<details>
<summary>Build Raylib locally if it is not installed</summary>

On Debian/Ubuntu, install the native dependencies, then download and build the
Raylib source with Ninja. Run these commands from the alt-synth directory:

```sh
sudo apt-get install build-essential pkg-config cmake ninja-build \
    libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev
git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git .deps/raylib-source
cmake -S .deps/raylib-source -B .deps/raylib-build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX="$PWD/.deps/raylib"
cmake --build .deps/raylib-build
cmake --install .deps/raylib-build
./build.sh
```

</details>

## Controls

For playable examples and a note-to-key reference, see the
[classic game music guide](docs/playing-guide.md).

| Control | Action |
| --- | --- |
| `A S D F G H J K L ; '` | White notes |
| `W E T Y U O P` | Black notes |
| Mouse hold / drag | Play notes; QWERTY Space holds sustain |
| `Space` | Hold sustain |
| `←` / `→` | Shift octave |
| `-` / `=` | Adjust volume |
| Tone button / `F3` / `Tab` | Cycle sounds |
| View button / `F2` | Switch piano / QWERTY display |
| `Backspace` | Silence notes |
| Help button / `F1` | Toggle help |
| `Esc` | Close help, or quit |

Starts in **QWERTY view** (C#2–A#6) with **Chiptune** selected. Press **F2**
to switch to Piano view (C4–F5 at the starting octave). Other sounds are Piano,
Warm keys, Bell, and Soft synth. Sounds are synthesized; no samples are required.
Lighter QWERTY keys play natural notes; darker ones play sharps. Amber keys are
held; green keys are sustained. Piano mode uses the bindings above.
QWERTY mode makes 55 keys playable, including modifiers: the lower
row adds bass notes, unused upper-row keys and digits extend the high range, and
modifiers add lower notes. The original home-row piano bindings stay the same.
Caps Lock is intentionally inactive. Tab, Space, `-`/`=`, and Backspace keep
their control actions, accessible by mouse too. Switching views silences notes; release
held controls before resuming.
Opening help or leaving the window silences notes; release playing controls before
resuming. The synth can play 10 or more notes at once. Which physical key
combinations register together depends on your keyboard’s rollover and any
desktop shortcuts. External MIDI input is not implemented.

## Development

```sh
./test.sh
CFLAGS='-O1 -g -fsanitize=address,undefined' ./test.sh
CFLAGS='-O0 -g' ./build.sh
```

Core tests include the complete QWERTY mapping and run without Raylib or a display. Check UI and audio changes manually.
Compiler overrides: `CC`, `CPPFLAGS`, `CFLAGS`, `LDFLAGS`. Delete `bin/` to clean
app output; `.deps/` keeps the local Raylib build.

[Raylib](https://www.raylib.com/) uses the zlib/libpng license.
The bundled Space Mono Bold font uses the [SIL Open Font License](assets/OFL.txt).
