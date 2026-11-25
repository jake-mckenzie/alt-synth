#ifndef ALT_SYNTH_KEYBOARD_H
#define ALT_SYNTH_KEYBOARD_H

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

// core tests compile only the display-independent keyboard code
#ifndef ALT_SYNTH_HEADLESS
#include <raylib.h>

// return the display label for a piano key index
const char *piano_key_label( int index );

// draw the piano with note labels and held or sustained key highlights
void draw_keyboard( KeyRect bounds, int base, const bool held[ MIDI_NOTES ],
                    const Synth *synth );

// resolve a QWERTY key to a MIDI note, or return -1 for a non-playing key
int computer_note( int code, int base );

// apply physical note presses from the active keyboard mapping
void read_note_keys( bool held[ MIDI_NOTES ], int base, bool computer_view );

// check for a physical key press or a mouse press on the matching keycap
bool control_pressed( int code, int mouse_key );

// return the physical key under the mouse, or KEY_NULL outside keycaps
int computer_hit( Vector2 mouse, KeyRect bounds );

// draw the QWERTY layout with playable notes and sustain feedback
void draw_computer_keyboard( KeyRect bounds, int base, const bool held[ MIDI_NOTES ],
                             const Synth *synth, bool sustain, int mouse_key,
                             bool input_enabled );
#endif

#endif
