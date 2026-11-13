//go:build raylib

package main

import (
	"fmt"
	"os"
	"runtime"

	"github.com/jake-mckenzie/alt-synth/internal/chords"
	"github.com/jake-mckenzie/alt-synth/internal/rayui"
)

// init keeps Raylib's window and graphics context on the process main thread.
func init() {
	runtime.LockOSThread()
}

// main launches the graphical chord viewer.
func main() {
	if err := rayui.Run(chords.NewCatalog()); err != nil {
		fmt.Fprintf(os.Stderr, "alt-synth: %v\n", err)
		os.Exit(1)
	}
}
