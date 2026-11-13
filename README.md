# Alt-Synth

A guitar chord viewer built with Go and Raylib. Browse chord shapes, hear them
played back, and see their waveforms and frequency spectra. Starts in CRT Amber;
press `t` once for Neon Dark: near-black, neon green, cyan, and purple accents.

## Run it

You’ll need Go 1.25+, Make, and a C compiler with cgo enabled. On Linux, install
the OpenGL, X11, and xkbcommon development libraries; on macOS, install Xcode
Command Line Tools. The Go module includes Raylib itself.

```bash
cd ~/Projects/alt-synth
go mod download
go mod verify
make run
```

## Controls

| Key | Action |
| --- | --- |
| `←` / `h`, `→` / `l` | Previous / next chord |
| `↑` / `k`, `↓` / `j` | Move between available accidental, major, and minor chords |
| `1`–`9` (also numpad) | Select a voicing; unavailable numbers do nothing |
| `x` | Toggle compact / full-neck view |
| `n` | Toggle finger numbers / fret numbers |
| `t` | Next theme |
| `a`–`g` | Hold to select and play a chord; release to stop |
| `Space` / `p` | Play / stop |
| `F1` / `?` | Toggle help |
| `q` / `Esc` | Quit (`Esc` closes help first) |

## Chords

Three voicings each for **A, Am, B, Bb, C, Cm, D, Dm, E, Em, F, F#, G, and Gm**.

Diagrams put high e at the top and low E at the bottom. `O` means open, `X`
means muted, and finger numbers run from 1 (index) to 4 (little finger).
Full-neck view shows frets 1–27; fret-number view uses the compact layout.
Changing chords keeps your voicing number, or returns to 1 if it isn’t available.

## Development

| Command | Purpose |
| --- | --- |
| `make build` | Build `bin/alt-synth` |
| `make BUILD=release build` | Build a smaller release binary |
| `make test` | Run the Go tests without graphics dependencies |
| `make check` | Run formatting, Go and Raylib tests, race checks, and vet |
| `make clean` | Remove build output |

`make check` needs the same graphics dependencies as the desktop build.

The desktop interface lives in `internal/rayui`, with the chord catalog in
`internal/chords`, navigation in `internal/app`, and pitch calculations in
`internal/signal`. The entry point is `cmd/alt-synth`.

`make build-raylib` and `make run-raylib` remain aliases for compatibility.

The bundled Space Mono Bold font uses the [SIL Open Font License](internal/rayui/assets/OFL.txt).
