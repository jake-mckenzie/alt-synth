#ifndef ALT_SYNTH_KEYBOARD_LAYOUT_H
#define ALT_SYNTH_KEYBOARD_LAYOUT_H

#include "synth.h"

#define KEY_COUNT 25
#define WHITE_KEY_COUNT 15
#define QWERTY_ROWS 5
#define QWERTY_COLUMNS 15
#define QWERTY_MIN_OFFSET (-23)
#define QWERTY_MAX_OFFSET 34
#define NO_NOTE MIDI_NOTES

// Keyboard geometry is independent of Raylib so hit testing runs without a display.
typedef struct
{
    float x, y, width, height;

} KeyRect;

// Lay out a chromatic key within a two-octave piano range; indices must be valid.
KeyRect keyboard_rect( int index, KeyRect bounds );

// Return the piano key under a point, prioritizing overlapping black keys.
int keyboard_hit( float x, float y, KeyRect bounds );

// Return a QWERTY key's semitone offset, or NO_NOTE for a control, disabled key, or empty cell.
int qwerty_note_offset( int row, int column );

#endif
