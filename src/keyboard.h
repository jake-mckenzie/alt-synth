#ifndef ALT_SYNTH_KEYBOARD_H
#define ALT_SYNTH_KEYBOARD_H

#include "synth.h"

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
