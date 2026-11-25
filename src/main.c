#include "input.h"

#include <stdio.h>
#include <string.h>

#define AUDIO_FRAMES 1024

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

// open the audio device and start a mono stream when available
static AudioStream start_audio( void )
{
  InitAudioDevice();
  if ( !IsAudioDeviceReady() )
  { return ( AudioStream ){ 0 }; }

  SetAudioStreamBufferSizeDefault( AUDIO_FRAMES );
  AudioStream stream = LoadAudioStream( SAMPLE_RATE, 32, 1 );
  if ( IsAudioStreamValid( stream ) )
  { PlayAudioStream( stream ); }

  return stream;
}

// render requested audio buffers, or render once for the waveform without audio
static void update_audio( Synth *synth, AudioStream stream, bool ready, float samples[ AUDIO_FRAMES ] )
{
  if ( !ready )
  {
    synth_render( synth, samples, AUDIO_FRAMES );
    return;
  }

  while ( IsAudioStreamProcessed( stream ) )
  {
    synth_render( synth, samples, AUDIO_FRAMES );
    UpdateAudioStream( stream, samples, AUDIO_FRAMES );
  }
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
  AudioStream stream = start_audio();
  bool audio_ready = IsAudioDeviceReady() && IsAudioStreamValid( stream );
  float samples[ AUDIO_FRAMES ] = { 0 };
  InputState input = { .base = 60, .computer_view = true }; // C4 (middle C).

  while ( !input.quit && !WindowShouldClose() )
  {
    UiLayout layout = ui_layout();
    bool held[ MIDI_NOTES ] = { false };
    UiFrame frame = input_update( &input, &layout, &synth, held );
    frame.audio_ready = audio_ready;
    update_audio( &synth, stream, audio_ready, samples );
    ui_draw( &layout, &frame, &synth, samples, AUDIO_FRAMES );
  }

  if ( audio_ready )
  {
    StopAudioStream( stream );
    UnloadAudioStream( stream );
  }
  if ( IsAudioDeviceReady() )
  { CloseAudioDevice(); }

  ui_shutdown();
  CloseWindow();

  return 0;
}
