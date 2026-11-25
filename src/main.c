#include "../bin/font_data.h"
#include "keyboard.h"
#include "ui_style.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define AUDIO_FRAMES 1024
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 760

#define TEXT_SCALE 1.3f
const Color background = { 17, 19, 23, 255 };
static const Color panel_color = { 25, 28, 33, 255 };
const Color border = { 54, 59, 66, 255 };
const Color text_color = { 228, 230, 224, 255 };
const Color muted = { 147, 154, 160, 255 };
const Color accent = { 255, 182, 77, 255 };
const Color signal_color = { 120, 218, 191, 255 };
static Font font;

// Draw a UI label with the shared font size scale and character spacing.
static void text( const char *label, float x, float y, float size, Color color )
{
  DrawTextEx( font, label, ( Vector2 ){ x, y }, size * TEXT_SCALE, 1, color );
}

// Center text using the same measured font metrics as the label renderer.
void centered( const char *label, Rectangle bounds, float size, Color color )
{
  Vector2 extent = MeasureTextEx( font, label, size * TEXT_SCALE, 1 );
  text( label, bounds.x + ( bounds.width - extent.x ) / 2, 
       bounds.y + ( bounds.height - extent.y ) / 2, size, color );
}

// draw the shortcut row with highlighted keys and evenly spaced actions
static void draw_shortcuts( float x, float y )
{
  static const char *const shortcuts[][ 2 ] =
  {
    { "Left / Right", "Octave" },
    { "- / =", "Volume" },
    { "Backspace", "Silence" },
    { "Esc", "Quit" }
  };
  const float size = 16;

  for ( size_t i = 0; i < sizeof( shortcuts ) / sizeof( shortcuts[ 0 ] ); ++i )
  {
    const char *key = shortcuts[ i ][ 0 ];
    const char *action = shortcuts[ i ][ 1 ];
    text( key, x, y, size, accent );
    x += MeasureTextEx( font, key, size * TEXT_SCALE, 1 ).x + 8;
    text( action, x, y, size, text_color );
    x += MeasureTextEx( font, action, size * TEXT_SCALE, 1 ).x + 28;
  }
}

// draw a dashboard panel with an optional section heading
static void panel( Rectangle r, const char *title )
{
  DrawRectangleRounded( r, 0.035f, 8, panel_color );
  DrawRectangleRoundedLinesEx( r, 0.035f, 8, 1, border );
  if ( title )
  { text( title, r.x + 20, r.y + 15, 18, muted ); }
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

// reserve room for the graph title and axis labels, including in short windows
static Rectangle graph_plot( Rectangle panel_bounds, float side_inset )
{
  float compact = fminf( 1, fmaxf( 0, ( 128 - panel_bounds.height ) / 32 ));
  float top_inset = 52 - 12 * compact;
  float bottom_inset = 28 - 4 * compact;

  return ( Rectangle ){ panel_bounds.x + side_inset, panel_bounds.y + top_inset,
    panel_bounds.width - 2 * side_inset, panel_bounds.height - top_inset - bottom_inset };
}

// Plot the most recent audio buffer against its sample-derived time window.
static void draw_wave( Rectangle r, const float *samples )
{
  panel( r, "OUTPUT / WAVEFORM" );
  Rectangle plot = graph_plot( r, 20 );

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

  text( "0 ms", plot.x, plot.y + plot.height + 4, 15, muted );
  text( "23.2 ms", plot.x + plot.width - 100, plot.y + plot.height + 4, 15, muted );
}

// Display voice gate envelopes on a MIDI axis that includes all sounding notes; not an FFT.
static void draw_pitches( Rectangle r, const Synth *synth )
{
  panel( r, "VOICES / PITCH LEVELS" );
  Rectangle plot = graph_plot( r, 24 );
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
      text( label, x - 9, bottom + 4, 14, muted );
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
static void draw_help( float width, float height )
{
  DrawRectangleRec( ( Rectangle ){ 0, 0, width, height }, Fade( BLACK, .75f ));
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
    centered( piano_key_label( i ), cap, 20, black ? accent : text_color );
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
          { printf( " %s", piano_key_label( i ) ); }
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
  InitWindow( WINDOW_WIDTH, WINDOW_HEIGHT, "alt-synth" );

  if ( !IsWindowReady() )
  { return 1; }

  SetWindowMinSize( 960, 640 );
  SetExitKey( KEY_NULL );
  SetTargetFPS( 120 );
  font = LoadFontFromMemory( ".ttf", font_data, ( int )sizeof( font_data ), 128, NULL, 0 );

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
  bool computer_view = true;

  while ( !quit && !WindowShouldClose() )
  {
    // use the same logical coordinates for drawing and mouse hit testing
    float screen_width = GetScreenWidth(), screen_height = GetScreenHeight();
    float scale = fmaxf( 1, fminf( screen_width / WINDOW_WIDTH, screen_height / WINDOW_HEIGHT ));
    Camera2D camera = { .zoom = scale };
    float width = screen_width / scale, height = screen_height / scale;
    SetMouseScale( 1 / scale, 1 / scale );
    float margin = 24, gap = 16;
    // reclaim space for the graphs as the window approaches its minimum height
    float compact = fminf( 1, fmaxf( 0, ( 720 - height ) / 80.f ));
    float keyboard_height = height * .5f;
    float keyboard_bottom = height - 24 + 4 * compact;
    KeyRect keys = { margin + 16, keyboard_bottom - keyboard_height,
                    width - 2 * ( margin + 16 ), keyboard_height };
    Rectangle filter_button = { width - margin - 270, 73, 270, 42 };
    Rectangle view_button = { filter_button.x - gap - 200, filter_button.y, 200, filter_button.height };
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
    BeginMode2D( camera );
    text( "alt-synth", margin, 22, 36, accent );
    text( "POLYPHONIC KEYBOARD", margin + 290, 34, 17, muted );
    char status[ 100 ], first[ 8 ], last[ 8 ];
    note_name( base + ( computer_view ? QWERTY_MIN_OFFSET : 0 ), first, sizeof( first ));
    note_name( base + ( computer_view ? QWERTY_MAX_OFFSET : KEY_COUNT - 1 ), last, sizeof( last ));
    snprintf( status, sizeof( status ), "%s-%s  VOL %d%%  SUSTAIN %s", first, last,
             ( int )lroundf( synth.volume * 100 ), sustain ? "ON" : "OFF" );
    float status_size = 22;
    float status_width = MeasureTextEx( font, status, status_size * TEXT_SCALE, 1 ).x;
    float status_available = view_button.x - margin - gap;
    if ( status_width > status_available )
    { status_size *= status_available / status_width; }
    text( status, margin, 80, status_size, text_color );

    if ( !audio_ready )
    { text("AUDIO UNAVAILABLE", width - 345, 32, 20, accent); }
    else
    { text("F1 HELP", width - 155, 32, 20, muted); }

    char filter_label[ 64 ];
    snprintf( filter_label, sizeof( filter_label ), "FILTER: %s", tone_name( synth.tone ));
    draw_button( filter_button, filter_label, 20, true, !help );
    draw_button( view_button, computer_view ? "F2: QWERTY VIEW" : "F2: PIANO VIEW", 16, false, !help );

    float keyboard_header = computer_view ? 16 : 56 - 8 * compact;
    Rectangle keyboard_panel = { margin, keys.y - keyboard_header, width - 2 * margin,
                                 keyboard_height + keyboard_header + 16 };
    draw_shortcuts( margin, 120 );

    float graph_y = 152 - 8 * compact;
    float graph_height = keyboard_panel.y - graph_y - ( gap - 4 * compact );
    float graph_width = ( width - 2 * margin - gap ) / 2;
    draw_wave( ( Rectangle ){ margin, graph_y, graph_width, graph_height }, samples );
    draw_pitches( ( Rectangle ){ margin + graph_width + gap, graph_y, graph_width, graph_height }, &synth );
    panel( keyboard_panel, computer_view ? NULL : "HOLD KEYS TO PLAY / SPACE TO SUSTAIN" );

    if ( computer_view )
    { draw_computer_keyboard(keys, base, held, &synth, sustain, mouse_key, focused && !blocked); }
    else
    { draw_keyboard(keys, base, held, &synth); }

    if ( help )
    { draw_help(width, height); }

    EndMode2D();
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
