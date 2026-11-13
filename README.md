# alt-synth

A desktop guitar chord viewer built with Go and Raylib. Browse voicings, play
chords, and explore their notes with waveform and frequency spectrum views.

## Build and run

Requires Go 1.25+, Make, and a C compiler with cgo enabled. The default Linux
build uses X11 and needs OpenGL, X11, and xkbcommon development libraries.
The Go module supplies Raylib's native source; a separate Raylib installation
is not required.

```sh
git clone git@github.com:jake-mckenzie/alt-synth.git
cd alt-synth
go mod download
make run
```

To launch an existing build, run `./bin/alt-synth`.

## Controls

| Key | Action |
| --- | --- |
| `←` / `h`, `→` / `l` | Previous / next base chord |
| `↑` / `k`, `↓` / `j` | Select an available accidental, major, or minor chord |
| `1`–`9` (also numpad) | Select a voicing; unavailable numbers do nothing |
| `a`–`g` | Hold to select and play a chord; release to stop |
| `Space` / `p` | Play / stop |
| `x` | Toggle compact / full-neck view |
| `n` | Toggle finger numbers / fret numbers |
| `t` | Cycle themes |
| `F1` / `/` / `?` | Toggle help |
| `Esc` | Close help, or quit when help is closed |
| `q` | Quit |

Click the base-chord row or an available variant in the dial to select it.
Changing chords retains the voicing number when available, otherwise selects
voicing 1. The initial theme is CRT Amber; press `t` once for Neon Dark.

## Chords

Three voicings each for **A, Am, B, Bb, C, Cm, D, Dm, E, Em, F, F#, G, and Gm**.

Diagrams show high e at the top and low E at the bottom. `O` means open and `X`
means muted. Fingers are numbered 1 (index) through 4 (little finger). Full-neck
view covers frets 1–27; fret-number view uses the compact layout.

## Development

| Command | Purpose |
| --- | --- |
| `make build` | Build `bin/alt-synth` |
| `make run` | Build and launch |
| `make BUILD=release build` | Build a smaller release binary |
| `make test` | Test catalog, navigation, and pitch calculations |
| `make check` | Check formatting, run tests and race checks, and vet |
| `make clean` | Remove build output |

`make check` includes the Raylib UI tests and requires the same native libraries
as the desktop build. Tests do not replace a manual check of the window and audio.

`cmd/alt-synth` starts the app. Under `internal/`, `rayui` handles rendering,
input, and audio; `app` manages chord selection; `chords` holds the catalog; and
`signal` supplies pitch and waveform calculations.

The bundled Space Mono Bold font uses the
[SIL Open Font License](internal/rayui/assets/OFL.txt).
