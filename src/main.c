#include "../bin/font_data.h"
#include <raylib.h>
#include "synth.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define AUDIO_FRAMES 1024

// Keep each note's physical key and display label together for every keyboard view.
typedef struct
{
  int code;
  const char *label;

} NoteBinding;

static const NoteBinding bindings[ KEY_COUNT ] = 
{
  { KEY_A, "A" }, { KEY_W, "W" }, { KEY_S, "S" }, { KEY_E, "E" }, { KEY_D, "D" }, { KEY_F, "F" }, 
  { KEY_T, "T" }, { KEY_G, "G" }, { KEY_Y, "Y" }, { KEY_H, "H" }, { KEY_U, "U" }, { KEY_J, "J" }, 
  { KEY_K, "K" }, { KEY_O, "O" }, { KEY_L, "L" }, { KEY_P, "P" }, { KEY_SEMICOLON, ";" }, 
  { KEY_APOSTROPHE, "'" }
};

#define TEXT_SCALE 1.3f
static const Color background = { 17, 19, 23, 255 };
static const Color panel_color = { 25, 28, 33, 255 };
static const Color border = { 54, 59, 66, 255 };
static const Color text_color = { 228, 230, 224, 255 };
static const Color muted = { 147, 154, 160, 255 };
static const Color accent = { 255, 182, 77, 255 };
static const Color signal_color = { 120, 218, 191, 255 };
static Font font;
static float text_growth = 1;

// Draw a UI label with the shared font size scale and character spacing.
static void text( const char *label, float x, float y, float size, Color color )
{
  DrawTextEx( font, label, ( Vector2 ){ x, y }, size * TEXT_SCALE * text_growth, text_growth, color );
}

// Center text using the same measured font metrics as the label renderer.
static void centered( const char *label, Rectangle bounds, float size, Color color )
{
  Vector2 extent = MeasureTextEx( font, label, size * TEXT_SCALE * text_growth, text_growth );
  text( label, bounds.x + ( bounds.width - extent.x ) / 2, 
       bounds.y + ( bounds.height - extent.y ) / 2, size, color );
}

// Draw a dashboard panel and its section heading.
static void panel( Rectangle r, const char *title )
{
  DrawRectangleRounded( r, 0.035f, 8, panel_color );
  DrawRectangleRoundedLinesEx( r, 0.035f, 8, 1, border );
  text( title, r.x + 20, r.y + 15, 18, muted );
}

// Draw a rounded action button with a hover state when interaction is enabled.
static void draw_button( Rectangle bounds, const char *label, float size, 
                        bool primary, bool enabled )
{
  bool hover = enabled && CheckCollisionPointRec( GetMousePosition(), bounds );
  DrawRectangleRounded( bounds, .15f, 6, hover ? border : ( primary ? panel_color : background ));
  DrawRectangleRoundedLinesEx( bounds, .15f, 6, 1, primary ? accent : muted );
  centered( label, bounds, size, primary ? accent : text_color );
}

// Convert display-independent keyboard bounds to a Raylib rectangle.
static Rectangle rectangle( KeyRect r )
{
  return ( Rectangle ){ r.x, r.y, r.width, r.height };
}

// Draw the piano with note labels and held or sustained key highlights.
static void draw_keyboard( KeyRect bounds, int base, const bool held[ MIDI_NOTES ],const Synth *synth )
{
  for ( int pass = 0; pass < 2; ++pass )
  {
    for ( int i = 0; i < KEY_COUNT; ++i )
    {
      bool black = note_is_black(  i );

      if ( black != ( pass == 1 ))
      { continue; }

      Rectangle r = rectangle(keyboard_rect( i, bounds ));
      int note = base + i;
      bool latched = synth->sounding[ note ];
      Color fill = black ? ( Color ){ 30, 33, 39, 255 } : ( Color ){ 224, 225, 217, 255 };
      Color ink = black ? text_color : background;
      
      if ( held[ note ] )
      { fill = accent; }
      else if ( latched )
      { fill = signal_color; }

      if ( held[ note ] || latched )
      { ink = background; }

      DrawRectangleRec( r, fill );
      DrawRectangleLinesEx( r, black ? 2 : 1, background );

      if ( black )
      { DrawRectangle(( int )( r.x + 4 ), ( int )( r.y + 4 ), ( int )( r.width - 8 ), 3, Fade( WHITE, .12f )); }

      centered( bindings[ i ].label, ( Rectangle ){ r.x, r.y + r.height - 64 * text_growth, r.width, 26 * text_growth }, 24, ink );
      char name[ 8 ];
      note_name( note, name, sizeof( name ));
      centered( name, ( Rectangle ){ r.x, r.y + r.height - 33 * text_growth, r.width, 22 * text_growth }, 16, Fade( ink, .7f ));

      if ( held[ note ] )
      { DrawRectangle((int)(r.x + 5), (int)(r.y + r.height - 6), (int)(r.width - 10), 3, ink); }
    }
  }
}

typedef struct
{
  int code;
  const char *label;
  float units;

} ComputerKey;

// Standard ANSI QWERTY, including row stagger and full-width modifier keys.
static const ComputerKey computer_rows[ QWERTY_ROWS ][ QWERTY_COLUMNS ] =
{
  {
    { KEY_GRAVE, "`", 1 },
    { KEY_ONE, "1", 1 },
    { KEY_TWO, "2", 1 },
    { KEY_THREE, "3", 1 },
    { KEY_FOUR, "4", 1 },
    { KEY_FIVE, "5", 1 },
    { KEY_SIX, "6", 1 },
    { KEY_SEVEN, "7", 1 },
    { KEY_EIGHT, "8", 1 },
    { KEY_NINE, "9", 1 },
    { KEY_ZERO, "0", 1 },
    { KEY_MINUS, "-", 1 },
    { KEY_EQUAL, "=", 1 },
    { KEY_BACKSPACE, "Back", 2 }
  },
  {
    { KEY_TAB, "Tab", 1.5 },
    { KEY_Q, "Q", 1 },
    { KEY_W, "W", 1 },
    { KEY_E, "E", 1 },
    { KEY_R, "R", 1 },
    { KEY_T, "T", 1 },
    { KEY_Y, "Y", 1 },
    { KEY_U, "U", 1 },
    { KEY_I, "I", 1 },
    { KEY_O, "O", 1 },
    { KEY_P, "P", 1 },
    { KEY_LEFT_BRACKET, "[", 1 },
    { KEY_RIGHT_BRACKET, "]", 1 },
    { KEY_BACKSLASH, "\\", 1.5 }
  },
  {
    { KEY_CAPS_LOCK, "Caps", 1.75 },
    { KEY_A, "A", 1 },
    { KEY_S, "S", 1 },
    { KEY_D, "D", 1 },
    { KEY_F, "F", 1 },
    { KEY_G, "G", 1 },
    { KEY_H, "H", 1 },
    { KEY_J, "J", 1 },
    { KEY_K, "K", 1 },
    { KEY_L, "L", 1 },
    { KEY_SEMICOLON, ";", 1 },
    { KEY_APOSTROPHE, "'", 1 },
    { KEY_ENTER, "Enter", 2.25 }
  },
  {
    { KEY_LEFT_SHIFT, "Shift", 2.25 },
    { KEY_Z, "Z", 1 },
    { KEY_X, "X", 1 },
    { KEY_C, "C", 1 },
    { KEY_V, "V", 1 },
    { KEY_B, "B", 1 },
    { KEY_N, "N", 1 },
    { KEY_M, "M", 1 },
    { KEY_COMMA, ",", 1 },
    { KEY_PERIOD, ".", 1 },
    { KEY_SLASH, "/", 1 },
    { KEY_RIGHT_SHIFT, "Shift", 2.75 }
  },
  {
    { KEY_LEFT_CONTROL, "Ctrl", 1.25 },
    { KEY_LEFT_SUPER, "Win", 1.25 },
    { KEY_LEFT_ALT, "Alt", 1.25 },
    { KEY_SPACE, "Space / sustain", 6.25 },
    { KEY_RIGHT_ALT, "Alt", 1.25 },
    { KEY_RIGHT_SUPER, "Win", 1.25 },
    { KEY_KB_MENU, "Menu", 1.25 },
    { KEY_RIGHT_CONTROL, "Ctrl", 1.25 }
  }
};

// Resolve a physical QWERTY key to a valid MIDI note; controls return -1.
static int computer_note( int code, int base )
{
  for ( int row = 0; row < QWERTY_ROWS; ++row )
  {
    for ( int i = 0; i < QWERTY_COLUMNS && computer_rows[ row ][ i ].label; ++i )
    {
      if ( computer_rows[ row ][ i ].code != code )
      { continue; }

      int offset = qwerty_note_offset( row, i );

      if ( offset == NO_NOTE )
      { return -1; }

      int note = base + offset;
      return note >= 0 && note < MIDI_NOTES ? note : -1;
    }
  }
  return -1;
}

// Apply physical note presses from the active keyboard mapping.
static void read_note_keys( bool held[ MIDI_NOTES ], int base, bool computer_view )
{
  if ( !computer_view )
  {
    for ( int i = 0; i < KEY_COUNT; ++i )
    { held[ base + i ] = IsKeyDown( bindings[ i ].code ); }

    return;
  }

  for ( int row = 0; row < QWERTY_ROWS; ++row )
  {
    for ( int i = 0; i < QWERTY_COLUMNS && computer_rows[ row ][ i ].label; ++i )
    {
      int offset = qwerty_note_offset( row, i );

      if ( offset != NO_NOTE )
      { held[ base + offset ] = IsKeyDown( computer_rows[ row ][ i ].code ); }
    }
  }
}

// Accept a control from either its physical key or a mouse press on its keycap.
static bool control_pressed( int code, int mouse_key )
{
  return IsKeyPressed( code ) || ( IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) &&
    mouse_key == code );
}

// Convert a staggered QWERTY key width and row into screen bounds.
static Rectangle computer_key_rect( KeyRect bounds, int row, float offset, float units )
{
  float unit = bounds.width / 15;
  float row_height = bounds.height / QWERTY_ROWS;

  return ( Rectangle )
  {
    bounds.x + offset * unit + 2, bounds.y + row * row_height + 2,
    units * unit - 4, row_height - 4
  };
}

// Return the physical key under the mouse, or KEY_NULL outside keycaps.
static int computer_hit( Vector2 mouse, KeyRect bounds )
{
  for ( int row = 0; row < QWERTY_ROWS; ++row )
  {
    float offset = 0;
    for ( int i = 0; i < QWERTY_COLUMNS && computer_rows[ row ][ i ].label; ++i )
    {
      ComputerKey key = computer_rows[ row ][ i ];
      Rectangle r = computer_key_rect( bounds, row, offset, key.units );

      if ( CheckCollisionPointRec( mouse, r ))
      { return key.code; }

      offset += key.units;
    }
  }
  return KEY_NULL;
}

// Draw the QWERTY layout with playable notes and sustain feedback.
static void draw_computer_keyboard( KeyRect bounds, int base, const bool held[ MIDI_NOTES ],
                                   const Synth *synth, bool sustain, int mouse_key,
                                   bool input_enabled )
{
  for ( int row = 0; row < QWERTY_ROWS; ++row )
  {
    float offset = 0;
    for ( int i = 0; i < QWERTY_COLUMNS && computer_rows[ row ][ i ].label; ++i )
    {
      ComputerKey key = computer_rows[ row ][ i ];
      Rectangle r = computer_key_rect( bounds, row, offset, key.units );
      offset += key.units;
      int note = computer_note( key.code, base );
      bool enabled = key.code != KEY_CAPS_LOCK;
      bool pressed = note >= 0 ? held[ note ] : enabled && input_enabled &&
                          ( IsKeyDown( key.code ) || 
                          ( mouse_key == key.code &&
                            IsMouseButtonDown( MOUSE_BUTTON_LEFT )));

      if ( key.code == KEY_SPACE )
      { pressed = sustain; }

      bool latched = note >= 0 && synth->sounding[ note ];
      Color fill = background, ink = enabled ? text_color : muted;

      if ( note >= 0 )
      { fill = note_is_black(note) ? background : border; }

      if ( pressed || latched )
      {
        fill = pressed ? accent : signal_color;
        ink = background;
      }

      DrawRectangleRounded( r, .12f, 6, fill );
      DrawRectangleRoundedLinesEx( r, .12f, 6, 1, note >= 0 ? muted : border );

      if ( note >= 0 )
      {
        centered( key.label, ( Rectangle ){ r.x, r.y, r.width, r.height * .55f }, 20, ink );
        char name[ 8 ];
        note_name( note, name, sizeof( name ));
        centered( name, ( Rectangle ){ r.x, r.y + r.height * .55f, r.width, r.height * .45f },
                 14, ink );
      }
      else
      { centered( key.label, r, 18, ink ); }
    }
  }
}

// Plot the most recent audio buffer against its sample-derived time window.
static void draw_wave( Rectangle r, const float *samples )
{
  panel( r, "OUTPUT / WAVEFORM" );
  Rectangle plot = { r.x + 20, r.y + 52 * text_growth, r.width - 40, r.height - 80 * text_growth };

  for ( int i = 0; i <= 4; ++i )
  {
    float y = plot.y + plot.height * i / 4;
    DrawLineEx( ( Vector2 ){ plot.x, y }, ( Vector2 ){ plot.x + plot.width, y }, 1, border );
  }

  for ( int i = 1; i < AUDIO_FRAMES; ++i )
  {
    Vector2 a = { plot.x + plot.width * ( i - 1 ) / ( AUDIO_FRAMES - 1 ),
                 plot.y + plot.height * ( .5f - .45f * samples[ i - 1 ] )};
    Vector2 b = { plot.x + plot.width * i / ( AUDIO_FRAMES - 1 ),
                 plot.y + plot.height * ( .5f - .45f * samples[ i ] )};
    DrawLineEx( a, b, 1.5f, signal_color );
  }

  text( "0 ms", plot.x, r.y + r.height - 24 * text_growth, 15, muted );
  text( "23.2 ms", plot.x + plot.width - 100 * text_growth, r.y + r.height - 24 * text_growth, 15, muted );
}

// Display voice gate envelopes on a MIDI axis that includes all sounding notes; not an FFT.
static void draw_pitches( Rectangle r, const Synth *synth )
{
  panel( r, "VOICES / PITCH LEVELS" );
  Rectangle plot = { r.x + 24, r.y + 52 * text_growth, r.width - 48, r.height - 80 * text_growth };
  float bottom = plot.y + plot.height;
  DrawLineEx( ( Vector2 ){ plot.x, bottom }, ( Vector2 ){ plot.x + plot.width, bottom}, 1, border );
  // Keep the usual pitch axis unless an extended QWERTY note falls outside it.
  int low = 24, high = 108;

  for ( int note = 0; note < MIDI_NOTES; ++note )
  {
    if ( synth->level[ note ] < .001f )
    { continue; }

    if ( note < low )
    { low = note; }

    if ( note > high )
    { high = note; }
  }

  for ( int note = low; note <= high; ++note )
  {
    float x = plot.x + plot.width * ( note - low ) / ( high - low );

    if ( note % 12 == 0 )
    {
      DrawLineEx(( Vector2 ){ x, plot.y }, ( Vector2 ){ x, bottom }, 1, border );
      char label[ 8 ];
      note_name( note, label, sizeof( label ));
      text( label, x - 9, bottom + 6, 14, muted );
    }

    if ( synth->level[ note ] < .001f)
    { continue; }

    float y = bottom - plot.height * synth->level[ note ];
    DrawLineEx( ( Vector2 ){ x, bottom }, ( Vector2 ){ x, y }, 4, accent );
    DrawCircleV( ( Vector2 ){ x, y }, 3, signal_color );
  }
}

// Draw one aligned control binding and its action in the help overlay.
static void help_row( float x, float y, const char *keys, const char *action )
{
  text( keys, x, y, 18, accent );
  text( action, x + 150, y, 18, text_color );
}

// Draw the key mapping and grouped controls over a dimmed application view.
static void draw_help( int width, int height )
{
  DrawRectangle( 0, 0, width, height, Fade( BLACK, .75f ));
  float box_width = fminf( width - 64, 1080 );
  Rectangle box = { ( width - box_width ) / 2, ( height - 536 ) / 2.0f, box_width, 536 };
  panel( box, "alt-synth / keyboard help" );
  float left = box.x + 24;
  text( "Hold notes for chords. Release keys to stop.", left, box.y + 56, 18, text_color );

  // Keycaps follow the piano's staggered layout, using the same labels as the keys.
  float key_start = left + 160;
  float unit = ( box.width - 48 - 160 ) / WHITE_KEY_COUNT;
  int whites = 0;
  text( "BLACK KEYS", left, box.y + 112, 18, muted );
  text( "WHITE KEYS", left, box.y + 158, 18, muted );

  for ( int i = 0; i < KEY_COUNT; ++i )
  {
    bool black = note_is_black( i );
    float cap_width = unit - 10;
    float x = key_start + whites * unit + 5;
    float y = box.y + 148;

    if ( black )
    {
      x = key_start + whites * unit - cap_width / 2;
      y = box.y + 102;
    }
    else
    { ++whites; }

    Rectangle cap = { x, y, cap_width, 38 };
    DrawRectangleRounded( cap, .15f, 6, black ? background : border );
    DrawRectangleRoundedLinesEx( cap, .15f, 6, 1, black ? border : muted );
    centered( bindings[ i ].label, cap, 20, black ? accent : text_color );
  }

  float divider_y = box.y + 210;
  DrawLineEx( ( Vector2 ){ left, divider_y }, ( Vector2 ){ box.x + box.width - 24, divider_y }, 1, border );
  float right = box.x + box.width / 2 + 16;
  text( "PLAYING", left, box.y + 232, 18, muted );
  text( "CONTROLS", right, box.y + 232, 18, muted );
  help_row( left, box.y + 274, "Mouse", "Hold / drag notes" );
  help_row( left, box.y + 316, "Space", "Hold sustain" );
  help_row( left, box.y + 358, "Left/Right", "Shift octave" );
  help_row( left, box.y + 400, "- / =", "Adjust volume" );
  help_row( right, box.y + 274, "Tab/Filter", "Cycle sounds" );
  help_row( right, box.y + 316, "Backspace", "Silence notes" );
  help_row( right, box.y + 358, "F1", "Toggle help" );
  help_row( right, box.y + 400, "Esc", "Close help" );

  text( "QWERTY adds more notes. Caps Lock is unused.", left, box.y + 478, 18, muted );
  text( "F2 switches views. Esc quits outside help.", left, box.y + 502, 18, muted );
}

// Print command-line help or manage the window, input, audio stream, and rendering loop.
int main( int argc, char **argv )
{
  if ( argc > 1 )
  {
    if ( strcmp( argv[ 1 ], "--help" ) == 0 )
    {
      puts( "alt-synth: a polyphonic piano keyboard" );

      for ( int black = 0; black <= 1; ++black )
      {
        printf( "%s keys:", black ? "Black" : "White" );
        for ( int i = 0; i < KEY_COUNT; ++i )
        {
          if ( note_is_black( i ) == ( bool )black )
          { printf( " %s", bindings[ i ].label ); }
        }
        putchar( '\n' );
      }

      puts( "Space sustain | arrows octave | -/= volume | Backspace silence | "
           "Tab filter | F2 keyboard view | F1 help | Esc quit\n"
           "QWERTY view: extra keys play notes; Caps Lock is unused. Click controls to use "
           "them." );
      return 0;
    }

    fprintf( stderr, "Usage: %s [--help]\n", argv[ 0 ] );
    return 1;
  }

  SetConfigFlags( FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT );
  InitWindow( 1280, 760, "alt-synth" );

  if ( !IsWindowReady() )
  { return 1; }

  SetWindowMinSize( 960, 640 );
  SetExitKey( KEY_NULL );
  SetTargetFPS( 120 );
  font = LoadFontFromMemory( ".ttf", font_data, ( int )sizeof( font_data ), 64, NULL, 0 );

  if ( !IsFontValid( font ) )
  {
      CloseWindow();
      return 1;
  }

  SetTextureFilter( font.texture, TEXTURE_FILTER_BILINEAR );

  Synth synth;
  synth_init( &synth );
  InitAudioDevice();
  bool audio_ready = IsAudioDeviceReady();
  AudioStream stream = { 0 };
  float samples[ AUDIO_FRAMES ] = { 0 };

  if ( audio_ready )
  {
    SetAudioStreamBufferSizeDefault( AUDIO_FRAMES );
    stream = LoadAudioStream( SAMPLE_RATE, 32, 1 );

    if ( !IsAudioStreamValid( stream ) )
    { audio_ready = false; }
    else
    { PlayAudioStream( stream ); }
  }

  int base = 60; // C4 (middle C).
  bool help = false, quit = false, sustain = false, blocked = false;
  bool computer_view = false;

  while ( !quit && !WindowShouldClose() )
  {
    int width = GetScreenWidth(), height = GetScreenHeight();
    text_growth = fminf( 1.6f, sqrtf( fmaxf( 1, height / 760.0f ) ));
    float margin = 24, gap = 16;
    float keyboard_height = fminf( 260, height * .36f );
    // Give the keyboard half of the extra vertical space in taller windows.
    keyboard_height += fmaxf( 0, height - 760 ) * .5f;
    if ( computer_view )
    { keyboard_height *= 1.125f; }
    KeyRect keys = { margin + 16, height - keyboard_height - 48 * text_growth, width - 2 * ( margin + 16 ),
                    keyboard_height };
    Rectangle keyboard_panel = { margin, keys.y - 56 * text_growth, width - 2 * margin, keyboard_height + 72 * text_growth };
    float graph_y = 128 * text_growth, graph_height = keyboard_panel.y - graph_y - gap;
    float graph_width = ( width - 2 * margin - gap ) / 2;
    Rectangle filter_button = { width - margin - 336 * text_growth, 73 * text_growth,
                                336 * text_growth, 42 * text_growth };
    Rectangle view_button = { width - 48 - 250 * text_growth, keyboard_panel.y + 8 * text_growth,
                              250 * text_growth, 38 * text_growth };
    bool held[ MIDI_NOTES ] = { false };

    if ( IsKeyPressed( KEY_F1 ) )
    { help = !help; }

    if ( IsKeyPressed( KEY_ESCAPE ) )
    {
      if ( help )
      { help = false; }
      else
      { quit = true; }
    }

    int mouse_key = computer_view ? computer_hit( GetMousePosition(), keys ) : KEY_NULL;
    bool focused = IsWindowFocused() && !IsWindowMinimized() && !help;
    if ( focused )
    {
      if ( IsKeyPressed( KEY_F2 ) || ( IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) &&
                                 CheckCollisionPointRec( GetMousePosition(), view_button )))
      {
        computer_view = !computer_view;
        synth_silence( &synth );
        blocked = true;
        mouse_key = computer_view ? computer_hit( GetMousePosition(), keys ) : KEY_NULL;
      }

      bool filter_click = IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) &&
                          CheckCollisionPointRec( GetMousePosition(), filter_button );

      if ( filter_click || control_pressed( KEY_TAB, mouse_key ))
      { synth_set_tone(&synth, (Tone)((synth.tone + 1) % TONE_COUNT)); }

      int old_base = base;

      if ( IsKeyPressed( KEY_LEFT ) && base > 24 )
      { base -= 12; }

      if ( IsKeyPressed( KEY_RIGHT ) && base < 84 )
      { base += 12; }

      if ( old_base != base )
      { synth_silence( &synth ); }

      if ( control_pressed( KEY_MINUS, mouse_key ))
      { synth.volume = fmaxf( 0, synth.volume - .05f ); }

      if ( control_pressed( KEY_EQUAL, mouse_key ))
      { synth.volume = fminf( 1, synth.volume + .05f ); }

      read_note_keys( held, base, computer_view );
      sustain = IsKeyDown( KEY_SPACE );

      if ( IsMouseButtonDown( MOUSE_BUTTON_LEFT ))
      {
        Vector2 mouse = GetMousePosition();
        int note = computer_view ? computer_note( mouse_key, base ) : -1;

        if ( !computer_view )
        {
          int index = keyboard_hit( mouse.x, mouse.y, keys );

          if ( index >= 0 )
          { note = base + index; }
        }

        if ( note >= 0 )
        { held[ note ] = true; }

        if ( mouse_key == KEY_SPACE )
        { sustain = true; }
      }
    }
    else
    {
      blocked = true;
      synth_silence( &synth );
      sustain = false;
    }
    // Panic/focus loss suppresses existing holds until all playing controls are
    // released.
    if ( focused && control_pressed( KEY_BACKSPACE, mouse_key ))
    {
      blocked = true;
      synth_silence( &synth );
    }

    if ( blocked )
    {
      bool any = IsKeyDown( KEY_SPACE ) || IsMouseButtonDown( MOUSE_BUTTON_LEFT );
      bool physical[ MIDI_NOTES ] = { false };
      read_note_keys( physical, base, computer_view );

      for ( int note = 0; note < MIDI_NOTES; ++note )
      { any = any || physical[note]; }

      if ( !any )
      { blocked = false; }

      memset( held, 0, sizeof( held ));
      sustain = false;
    }

    synth_notes( &synth, held, sustain );

    if ( audio_ready )
    {
      while ( IsAudioStreamProcessed( stream ))
      {
        synth_render( &synth, samples, AUDIO_FRAMES );
        UpdateAudioStream( stream, samples, AUDIO_FRAMES );
      }
    }
    else
    { synth_render( &synth, samples, AUDIO_FRAMES ); }

    BeginDrawing();
    ClearBackground( background );
    text( "alt-synth", margin, 22, 36, accent );
    text( "POLYPHONIC KEYBOARD", margin + 290 * text_growth, 34 * text_growth, 17, muted );
    char status[ 100 ], first[ 8 ], last[ 8 ];
    note_name( base + ( computer_view ? QWERTY_MIN_OFFSET : 0 ), first, sizeof( first ));
    note_name( base + ( computer_view ? QWERTY_MAX_OFFSET : KEY_COUNT - 1 ), last, sizeof( last ));
    snprintf( status, sizeof( status ), "%s-%s  VOL %d%%  SUSTAIN %s", first, last,
             ( int )lroundf( synth.volume * 100 ), sustain ? "ON" : "OFF" );
    text( status, margin, 80 * text_growth, 22, text_color );

    if ( !audio_ready )
    { text("AUDIO UNAVAILABLE", width - 345 * text_growth, 32 * text_growth, 20, accent); }
    else
    { text("F1 HELP", width - 155 * text_growth, 32 * text_growth, 20, muted); }

    char filter_label[ 64 ];
    snprintf( filter_label, sizeof( filter_label ), "FILTER: %s", tone_name( synth.tone ));
    draw_button( filter_button, filter_label, 20, true, !help );
    draw_wave( ( Rectangle ){ margin, graph_y, graph_width, graph_height }, samples );
    draw_pitches( ( Rectangle ){ margin + graph_width + gap, graph_y, graph_width, graph_height }, &synth );
    panel( keyboard_panel, computer_view ? "LIGHTER = NATURAL NOTES / DARKER = SHARPS"
                                        : "HOLD KEYS TO PLAY / SPACE TO SUSTAIN" );
    draw_button( view_button, computer_view ? "F2: QWERTY VIEW" : "F2: PIANO VIEW", 16, false, !help );

    if ( computer_view )
    { draw_computer_keyboard(keys, base, held, &synth, sustain, mouse_key, focused && !blocked); }
    else
    { draw_keyboard(keys, base, held, &synth); }

    text("ARROWS octave   -/= volume   BACKSPACE silence   ESC quit", margin, height - 29 * text_growth, 18, muted);

    if ( help )
    {
      // Scale the help panel and its spacing together with its text.
      float growth = text_growth;
      Camera2D camera = { .zoom = growth };
      BeginMode2D( camera );
      text_growth = 1;
      draw_help( ( int )( width / growth ), ( int )( height / growth ));
      text_growth = growth;
      EndMode2D();
    }

    EndDrawing();
  }
  if ( audio_ready )
  {
    StopAudioStream( stream );
    UnloadAudioStream( stream );
  }
  if ( IsAudioDeviceReady() )
  { CloseAudioDevice(); }

  UnloadFont( font );
  CloseWindow();

  return 0;
}
