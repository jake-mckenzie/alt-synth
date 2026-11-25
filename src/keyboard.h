#ifndef ALT_SYNTH_KEYBOARD_H
#define ALT_SYNTH_KEYBOARD_H

#include "synth.h"

#define KEY_COUNT 25 // chromatic piano keys across two octaves, including both ends
#define WHITE_KEY_COUNT 15 // white piano keys used to size the layout
#define QWERTY_ROWS 5 // number of displayed keyboard rows
#define QWERTY_COLUMNS 15 // maximum key slots per row
#define QWERTY_MIN_OFFSET (-23) // lowest playable semitone offset from the base note
#define QWERTY_MAX_OFFSET 34 // highest playable semitone offset from the base note
#define NO_NOTE MIDI_NOTES // sentinel for controls and empty key slots

// Keyboard geometry is independent of Raylib so hit testing runs without a display.
typedef struct
{
    float x, y, width, height;

} KeyRect;

// return a piano key rectangle for an index from 0 through KEY_COUNT - 1
KeyRect keyboard_rect( int index, KeyRect bounds );

// return the piano key index under a point, or -1 outside the keys
// black keys take priority where they overlap white keys
int keyboard_hit( float x, float y, KeyRect bounds );

// return a QWERTY cell's semitone offset from the base note
// return NO_NOTE for a non-playing cell or coordinates outside the grid
int qwerty_note_offset( int row, int column );

// core tests compile only the display-independent keyboard code
#ifndef ALT_SYNTH_HEADLESS
#include <raylib.h>

// return the display label for an index from 0 through KEY_COUNT - 1
const char *piano_key_label( int index );

// draw the piano with note labels and held or sustained key highlights
void draw_keyboard( KeyRect bounds, int base, const bool held[ MIDI_NOTES ],
                    const Synth *synth );

// resolve a QWERTY key to a MIDI note, or return -1 if unmapped or out of range
int computer_note( int code, int base );

// write physical key states for mapped notes, leaving other entries untouched
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
