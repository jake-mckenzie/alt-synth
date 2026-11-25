#ifndef ALT_SYNTH_UI_H
#define ALT_SYNTH_UI_H

#include "keyboard_layout.h"

#include <raylib.h>

#define UI_WINDOW_WIDTH 1280
#define UI_WINDOW_HEIGHT 760

// logical window dimensions and control bounds shared by input and rendering
typedef struct
{
  float width, height, scale, compact;
  KeyRect keys;
  Rectangle help_button, view_button, tone_button;
} UiLayout;

// current input and display state for a single rendered frame
typedef struct
{
  int base, mouse_key;
  const bool *held;
  bool computer_view, sustain, input_enabled, audio_ready, help;
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
