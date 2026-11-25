#ifndef ALT_SYNTH_UI_H
#define ALT_SYNTH_UI_H

#include "keyboard.h"

#include <raylib.h>

#define UI_WINDOW_WIDTH 1280 // initial window width and horizontal scale reference
#define UI_WINDOW_HEIGHT 760 // initial window height and vertical scale reference

// logical window dimensions and control bounds shared by input and rendering
typedef struct
{
  float width, height; // viewport size in logical pixels
  float scale; // physical pixels per logical pixel
  float compact; // layout compression from 0 to 1 near the minimum height
  KeyRect keys;
  Rectangle help_button, view_button, tone_button;
} UiLayout;

// current input and display state for a single rendered frame
typedef struct
{
  int base; // MIDI base note for the current octave
  int mouse_key; // QWERTY key under the pointer, or KEY_NULL
  const bool *held; // borrowed MIDI note states, valid through ui_draw
  bool computer_view, sustain;
  bool input_enabled; // show live control presses when playing is enabled
  bool audio_ready, help;
} UiFrame;

// load the embedded font after opening the window, returning false on failure
bool ui_init( void );

// release the font before closing the window
void ui_shutdown( void );

// compute the current layout and apply its inverse scale to mouse coordinates
UiLayout ui_layout( void );

// draw the dashboard and optional help overlay using the latest audio samples
void ui_draw( const UiLayout *layout, const UiFrame *frame, const Synth *synth,
              const float *samples, size_t sample_count );

// print the command-line controls and piano bindings
void ui_print_help( void );

// colors shared by the dashboard and keyboard views
extern const Color background;
extern const Color border;
extern const Color text_color;
extern const Color muted;
extern const Color accent;
extern const Color signal_color;

// center a label using the shared UI font metrics
void centered( const char *label, Rectangle bounds, float size, Color color );

#endif
