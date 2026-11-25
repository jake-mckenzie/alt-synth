#include "audio.h"
#include "input.h"

#include <stdio.h>
#include <string.h>

// open the window and load the UI font, returning false if either fails
static bool open_window( void )
{
  SetConfigFlags( FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT );
  InitWindow( UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT, "alt-synth" );

  if ( !IsWindowReady() )
  { return false; }

  SetWindowMinSize( 960, 640 );
  SetExitKey( KEY_NULL );
  SetTargetFPS( 120 );
  if ( !ui_init() )
  {
    CloseWindow();
    return false;
  }

  return true;
}

// Print command-line help or manage the window, input, audio stream, and rendering loop.
int main( int argc, char **argv )
{
  if ( argc > 1 )
  {
    if ( strcmp( argv[ 1 ], "--help" ) == 0 )
    {
      ui_print_help();
      return 0;
    }

    fprintf( stderr, "Usage: %s [--help]\n", argv[ 0 ] );
    return 1;
  }

  if ( !open_window() )
  { return 1; }

  Synth synth;
  synth_init( &synth );
  AudioOutput audio;
  audio_init( &audio );
  InputState input = { .base = 60, .computer_view = true }; // C4 (middle C).

  while ( !input.quit && !WindowShouldClose() )
  {
    UiLayout layout = ui_layout();
    bool held[ MIDI_NOTES ] = { false };
    UiFrame frame = input_update( &input, &layout, &synth, held );
    frame.audio_ready = audio.ready;
    audio_update( &audio, &synth );
    ui_draw( &layout, &frame, &synth, audio.samples, AUDIO_FRAMES );
  }

  audio_shutdown( &audio );
  ui_shutdown();
  CloseWindow();

  return 0;
}
