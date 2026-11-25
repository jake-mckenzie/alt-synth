#include "input.h"
#include "keyboard.h"

#include <math.h>
#include <string.h>

// toggle help and handle Escape before deciding whether playing is enabled
static void update_window_shortcuts( InputState *input )
{
  if ( IsKeyPressed( KEY_F1 ) )
  { input->help = !input->help; }

  if ( IsKeyPressed( KEY_ESCAPE ) )
  {
    if ( input->help )
    { input->help = false; }
    else
    { input->quit = true; }
  }
}

// apply view, tone, octave, and volume controls, then return the current mouse key
static int update_playing_controls( InputState *input, const UiLayout *layout,
                                    Synth *synth, int mouse_key )
{
  if ( IsKeyPressed( KEY_F2 ) || ( IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) &&
                             CheckCollisionPointRec( GetMousePosition(), layout->view_button )))
  {
    input->computer_view = !input->computer_view;
    synth_silence( synth );
    input->blocked = true;
    mouse_key = input->computer_view ? computer_hit( GetMousePosition(), layout->keys ) : KEY_NULL;
  }

  bool filter_click = IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) &&
                      CheckCollisionPointRec( GetMousePosition(), layout->filter_button );

  if ( filter_click || control_pressed( KEY_TAB, mouse_key ))
  { synth_set_tone( synth, ( Tone )( ( synth->tone + 1 ) % TONE_COUNT )); }

  int old_base = input->base;

  if ( IsKeyPressed( KEY_LEFT ) && input->base > 24 )
  { input->base -= 12; }

  if ( IsKeyPressed( KEY_RIGHT ) && input->base < 84 )
  { input->base += 12; }

  if ( old_base != input->base )
  { synth_silence( synth ); }

  if ( control_pressed( KEY_MINUS, mouse_key ))
  { synth->volume = fmaxf( 0, synth->volume - .05f ); }

  if ( control_pressed( KEY_EQUAL, mouse_key ))
  { synth->volume = fminf( 1, synth->volume + .05f ); }

  return mouse_key;
}

// report whether a playing control must still be released after silencing
static bool playing_controls_held( const InputState *input )
{
  if ( IsKeyDown( KEY_SPACE ) || IsMouseButtonDown( MOUSE_BUTTON_LEFT ) )
  { return true; }

  bool physical[ MIDI_NOTES ] = { false };
  read_note_keys( physical, input->base, input->computer_view );

  for ( int note = 0; note < MIDI_NOTES; ++note )
  {
    if ( physical[ note ] )
    { return true; }
  }

  return false;
}

// collect playing controls, update synth gates, and return the state for drawing
UiFrame input_update( InputState *input, const UiLayout *layout, Synth *synth,
                      bool held[ MIDI_NOTES ] )
{
  bool sustain = false;
  update_window_shortcuts( input );

  int mouse_key = input->computer_view ? computer_hit( GetMousePosition(), layout->keys ) : KEY_NULL;
  bool focused = IsWindowFocused() && !IsWindowMinimized() && !input->help;
  if ( focused )
  {
    mouse_key = update_playing_controls( input, layout, synth, mouse_key );

    read_note_keys( held, input->base, input->computer_view );
    sustain = IsKeyDown( KEY_SPACE );

    if ( IsMouseButtonDown( MOUSE_BUTTON_LEFT ))
    {
      Vector2 mouse = GetMousePosition();
      int note = input->computer_view ? computer_note( mouse_key, input->base ) : -1;

      if ( !input->computer_view )
      {
        int index = keyboard_hit( mouse.x, mouse.y, layout->keys );

        if ( index >= 0 )
        { note = input->base + index; }
      }

      if ( note >= 0 )
      { held[ note ] = true; }

      if ( mouse_key == KEY_SPACE )
      { sustain = true; }
    }
  }
  else
  {
    input->blocked = true;
    synth_silence( synth );
  }
  // suppress existing holds after panic or focus loss until controls are released
  if ( focused && control_pressed( KEY_BACKSPACE, mouse_key ))
  {
    input->blocked = true;
    synth_silence( synth );
  }

  if ( input->blocked )
  {
    input->blocked = playing_controls_held( input );

    memset( held, 0, MIDI_NOTES * sizeof( *held ));
    sustain = false;
  }

  synth_notes( synth, held, sustain );

  return ( UiFrame ){
    .base = input->base,
    .mouse_key = mouse_key,
    .held = held,
    .computer_view = input->computer_view,
    .sustain = sustain,
    .input_enabled = focused && !input->blocked,
    .help = input->help
  };
}
