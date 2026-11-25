#ifndef ALT_SYNTH_INPUT_H
#define ALT_SYNTH_INPUT_H

#include "ui.h"

// input state that persists between frames
typedef struct
{
  int base; // MIDI note assigned to the first piano key
  bool computer_view, help, quit;
  bool blocked; // suppress held controls until they are released after silencing
} InputState;

// poll controls, update input and synth state, and fill the caller's zeroed held array
// the returned frame borrows that array for drawing
UiFrame input_update( InputState *input, const UiLayout *layout, Synth *synth,
                      bool held[ MIDI_NOTES ] );

#endif
