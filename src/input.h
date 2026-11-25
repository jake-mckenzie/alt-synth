#ifndef ALT_SYNTH_INPUT_H
#define ALT_SYNTH_INPUT_H

#include "ui.h"

// input state that persists between frames
typedef struct
{
  int base;
  bool computer_view, help, quit, blocked;
} InputState;

// update synth gates and return a frame borrowing the caller's zeroed held-note array
UiFrame input_update( InputState *input, const UiLayout *layout, Synth *synth,
                      bool held[ MIDI_NOTES ] );

#endif
