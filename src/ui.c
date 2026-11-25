#include "../bin/font_data.h"
#include "ui.h"
#include "keyboard.h"

#include <math.h>
#include <stdio.h>

#define MARGIN 24
#define GAP 16
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

// measure a label using the same font scale and spacing as drawing
static Vector2 measure_text( const char *label, float size )
{
  return MeasureTextEx( font, label, size * TEXT_SCALE, 1 );
}

// Center text using the same measured font metrics as the label renderer.
void centered( const char *label, Rectangle bounds, float size, Color color )
{
  Vector2 extent = measure_text( label, size );
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
    x += measure_text( key, size ).x + 8;
    text( action, x, y, size, text_color );
    x += measure_text( action, size ).x + 28;
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
static void draw_button( Rectangle bounds, const char *label, bool enabled )
{
  bool hover = enabled && CheckCollisionPointRec( GetMousePosition(), bounds );
  DrawRectangleRounded( bounds, .15f, 6, hover ? border : panel_color);
  DrawRectangleRoundedLinesEx( bounds, .15f, 6, 1, muted );
  centered( label, bounds, 17, text_color );
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
static void draw_wave( Rectangle r, const float *samples, size_t frames )
{
  panel( r, "OUTPUT / WAVEFORM" );
  Rectangle plot = graph_plot( r, 20 );

  for ( int i = 0; i <= 4; ++i )
  {
    float y = plot.y + plot.height * i / 4;
    DrawLineEx( ( Vector2 ){ plot.x, y }, ( Vector2 ){ plot.x + plot.width, y }, 1, border );
  }

  for ( size_t i = 1; i < frames; ++i )
  {
    Vector2 a = { plot.x + plot.width * ( i - 1 ) / ( frames - 1 ),
                 plot.y + plot.height * ( .5f - .45f * samples[ i - 1 ] )};
    Vector2 b = { plot.x + plot.width * i / ( frames - 1 ),
                 plot.y + plot.height * ( .5f - .45f * samples[ i ] )};
    DrawLineEx( a, b, 1.5f, signal_color );
  }

  text( "0 ms", plot.x, plot.y + plot.height + 4, 15, muted );
  char duration[ 32 ];
  snprintf( duration, sizeof( duration ), "%.1f ms", 1000.0 * frames / SAMPLE_RATE );
  text( duration, plot.x + plot.width - 100, plot.y + plot.height + 4, 15, muted );
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
  help_row( right, box.y + 274, "F3 / Tab", "Cycle tones" );
  help_row( right, box.y + 316, "Backspace", "Silence notes" );
  help_row( right, box.y + 358, "F1", "Toggle help" );
  help_row( right, box.y + 400, "Esc", "Close help" );

  text( "QWERTY adds more notes. Caps Lock is unused.", left, box.y + 478, 18, muted );
  text( "F2 switches views. Esc quits outside help.", left, box.y + 502, 18, muted );
}

// load the embedded UI font after the window opens, returning false on failure
bool ui_init( void )
{
  font = LoadFontFromMemory( ".ttf", font_data, ( int )sizeof( font_data ), 128, NULL, 0 );
  if ( !IsFontValid( font ) )
  { return false; }

  SetTextureFilter( font.texture, TEXTURE_FILTER_BILINEAR );
  return true;
}

// release the UI font while the graphics context is still available
void ui_shutdown( void )
{
  UnloadFont( font );
}

// compute logical bounds and align mouse coordinates to the drawing scale
UiLayout ui_layout( void )
{
  // use the same logical coordinates for drawing and mouse hit testing
  float screen_width = GetScreenWidth(), screen_height = GetScreenHeight();
  float scale = fmaxf( 1, fminf( screen_width / UI_WINDOW_WIDTH, screen_height / UI_WINDOW_HEIGHT ));
  float width = screen_width / scale, height = screen_height / scale;
  SetMouseScale( 1 / scale, 1 / scale );
  // reclaim space for the graphs as the window approaches its minimum height
  float compact = fminf( 1, fmaxf( 0, ( 720 - height ) / 80.f ));
  float keyboard_height = height * .5f;
  float keyboard_bottom = height - 24 + 4 * compact;
  KeyRect keys = { MARGIN + 16, keyboard_bottom - keyboard_height,
                  width - 2 * ( MARGIN + 16 ), keyboard_height };
  Rectangle tone_button = { width - MARGIN - 176, 23, 176, 42 };
  Rectangle view_button = tone_button;
  view_button.x -= tone_button.width + GAP;
  Rectangle help_button = view_button;
  help_button.x -= view_button.width + GAP;

  return ( UiLayout ){ width, height, scale, compact, keys, help_button, view_button, tone_button };
}

// draw the title, status, controls, and shortcut hints
static void draw_header( const UiLayout *layout, const UiFrame *frame, const Synth *synth )
{
  text( "alt-synth", MARGIN, 22, 36, accent );
  char status[ 100 ], first[ 8 ], last[ 8 ];
  note_name( frame->base + ( frame->computer_view ? QWERTY_MIN_OFFSET : 0 ), first, sizeof( first ));
  note_name( frame->base + ( frame->computer_view ? QWERTY_MAX_OFFSET : KEY_COUNT - 1 ), last, sizeof( last ));
  snprintf( status, sizeof( status ), "%s-%s  VOL %d%%  SUSTAIN %s", first, last,
           ( int )lroundf( synth->volume * 100 ), frame->sustain ? "ON" : "OFF" );
  text( status, MARGIN, 80, 22, text_color );

  if ( !frame->audio_ready )
  {
    const char *warning = "AUDIO UNAVAILABLE";
    float warning_x = layout->width - MARGIN - measure_text( warning, 17 ).x;
    text( warning, warning_x, 82, 17, accent );
  }

  char tone_label[ 64 ];
  snprintf( tone_label, sizeof( tone_label ), "F3: %s", tone_name( synth->tone ));
  draw_button( layout->view_button, frame->computer_view ? "F2: QWERTY" : "F2: PIANO", !frame->help );
  draw_button( layout->tone_button, tone_label, !frame->help );

  draw_shortcuts( MARGIN, 120 );
}

// draw a dashboard frame using the same control bounds used for input
void ui_draw( const UiLayout *layout, const UiFrame *frame, const Synth *synth,
              const float *samples, size_t sample_count )
{
  KeyRect keys = layout->keys;
  float keyboard_header = 56 - 8 * layout->compact;
  Rectangle keyboard_panel = { MARGIN, keys.y - keyboard_header, layout->width - 2 * MARGIN,
                               keys.height + keyboard_header + 16 };
  float graph_y = 152 - 8 * layout->compact;
  float graph_height = keyboard_panel.y - graph_y - ( GAP - 4 * layout->compact );
  float graph_width = ( layout->width - 2 * MARGIN - GAP ) / 2;

  BeginDrawing();
  ClearBackground( background );
  BeginMode2D( ( Camera2D ){ .zoom = layout->scale } );

  draw_header( layout, frame, synth );
  draw_wave( ( Rectangle ){ MARGIN, graph_y, graph_width, graph_height }, samples, sample_count );
  draw_pitches( ( Rectangle ){ MARGIN + graph_width + GAP, graph_y, graph_width, graph_height }, synth );
  panel( keyboard_panel, frame->computer_view ? NULL : "HOLD KEYS TO PLAY / SPACE TO SUSTAIN" );

  if ( frame->computer_view )
  {
    draw_computer_keyboard( keys, frame->base, frame->held, synth, frame->sustain,
                            frame->mouse_key, frame->input_enabled );
  }
  else
  { draw_keyboard( keys, frame->base, frame->held, synth ); }

  if ( frame->help )
  { draw_help( layout->width, layout->height ); }

  draw_button( layout->help_button, frame->help ? "F1: CLOSE HELP" : "F1: HELP", true );

  EndMode2D();
  EndDrawing();
}

// print the keyboard bindings and controls to standard output
void ui_print_help( void )
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
       "F1 help | F2 keyboard view | F3/Tab tone | Esc quit\n"
       "QWERTY view: extra keys play notes; Caps Lock is unused. Click controls to use "
       "them." );
}
