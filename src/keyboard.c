#include "keyboard.h"

// Lay out a chromatic key within a two-octave piano range; indices must be valid.
KeyRect keyboard_rect( int index, KeyRect bounds )
{
  float white_width = bounds.width / WHITE_KEY_COUNT;
  int whites = 0;

  for ( int i = 0; i < index; ++i )
  {
    if ( !note_is_black( i ) )
    { ++whites; }
  }

  if ( note_is_black( index ) )
  {
    return ( KeyRect ){ bounds.x + whites * white_width - white_width * 0.31f, bounds.y,
                     white_width * 0.62f, bounds.height * 0.62f };
  }

  return ( KeyRect ){ bounds.x + whites * white_width, bounds.y, white_width, bounds.height };
}

// Return the piano key under a point, prioritizing overlapping black keys.
int keyboard_hit( float x, float y, KeyRect bounds )
{
  // Black keys overlap white keys, so they take precedence for mouse input.
  for ( int pass = 0; pass < 2; ++pass )
  {
    for ( int i = 0; i < KEY_COUNT; ++i )
    {
      if ( note_is_black( i ) != ( pass == 0 ))
      { continue; }

      KeyRect r = keyboard_rect( i, bounds );

      if ( x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height )
      { return i; }
    }
  }
  return -1;
}

// Return a QWERTY key's semitone offset, or NO_NOTE for a control, disabled key, or empty cell.
int qwerty_note_offset( int row, int column )
{
  // Home-row piano bindings stay familiar. Other rows extend the playable range.
  // Tab, Space, -/=, and Backspace retain their control actions.
  static const int offsets[ QWERTY_ROWS ][ QWERTY_COLUMNS ] =
  {
    { 34, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, NO_NOTE, NO_NOTE, NO_NOTE, NO_NOTE },
    { NO_NOTE, 18, 1, 3, 19, 6, 8, 10, 20, 13, 15, 21, 22, 23, NO_NOTE },
    { NO_NOTE, 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, -23, NO_NOTE, NO_NOTE },
    { -22, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -21, NO_NOTE, NO_NOTE, NO_NOTE },
    { -20, -19, -18, NO_NOTE, -17, -16, -15, -14, NO_NOTE, NO_NOTE, NO_NOTE, NO_NOTE, NO_NOTE,
     NO_NOTE, NO_NOTE }
  };

  if ( row < 0 || row >= QWERTY_ROWS || column < 0 || column >= QWERTY_COLUMNS )
  { return NO_NOTE; }

  return offsets[ row ][ column ];
}

#ifndef ALT_SYNTH_HEADLESS
#include "ui.h"

// pair each piano note's physical key with its display label
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
  { KEY_APOSTROPHE, "'" }, { KEY_Q, "Q" }, { KEY_R, "R" }, { KEY_I, "I" },
  { KEY_LEFT_BRACKET, "[" }, { KEY_RIGHT_BRACKET, "]" }, { KEY_BACKSLASH, "\\" },
  { KEY_ONE, "1" }
};

// Return the display label for a piano key index.
const char *piano_key_label( int index )
{
  return bindings[ index ].label;
}

// Convert display-independent keyboard bounds to a Raylib rectangle.
static Rectangle rectangle( KeyRect r )
{
  return ( Rectangle ){ r.x, r.y, r.width, r.height };
}

// Draw the piano with note labels and held or sustained key highlights.
void draw_keyboard( KeyRect bounds, int base, const bool held[ MIDI_NOTES ],
                    const Synth *synth )
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

      centered( bindings[ i ].label, ( Rectangle ){ r.x, r.y + r.height - 64, r.width, 26 }, 24, ink );
      char name[ 8 ];
      note_name( note, name, sizeof( name ));
      centered( name, ( Rectangle ){ r.x, r.y + r.height - 33, r.width, 22 }, 16, Fade( ink, .7f ));

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

// resolve a QWERTY position to a playable MIDI note, or return -1
static int computer_note_at( int row, int column, int base )
{
  int offset = qwerty_note_offset( row, column );
  if ( offset == NO_NOTE || base < -offset || base >= MIDI_NOTES - offset )
  { return -1; }

  return base + offset;
}

// Resolve a QWERTY key to a MIDI note, or return -1 for a non-playing key.
int computer_note( int code, int base )
{
  for ( int row = 0; row < QWERTY_ROWS; ++row )
  {
    for ( int i = 0; i < QWERTY_COLUMNS && computer_rows[ row ][ i ].label; ++i )
    {
      if ( computer_rows[ row ][ i ].code != code )
      { continue; }

      return computer_note_at( row, i, base );
    }
  }
  return -1;
}

// Apply physical note presses from the active keyboard mapping.
void read_note_keys( bool held[ MIDI_NOTES ], int base, bool computer_view )
{
  if ( !computer_view )
  {
    for ( int i = 0; i < KEY_COUNT; ++i )
    {
      if ( base >= -i && base < MIDI_NOTES - i )
      { held[ base + i ] = IsKeyDown( bindings[ i ].code ); }
    }

    return;
  }

  for ( int row = 0; row < QWERTY_ROWS; ++row )
  {
    for ( int i = 0; i < QWERTY_COLUMNS && computer_rows[ row ][ i ].label; ++i )
    {
      int note = computer_note_at( row, i, base );
      if ( note >= 0 )
      { held[ note ] = IsKeyDown( computer_rows[ row ][ i ].code ); }
    }
  }
}

// Check for a physical key press or a mouse press on the matching keycap.
bool control_pressed( int code, int mouse_key )
{
  return IsKeyPressed( code ) || ( IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) &&
    mouse_key == code );
}

// Convert a staggered QWERTY key width and row into screen bounds.
static Rectangle computer_key_rect( KeyRect bounds, int row, float offset, float units )
{
  float unit = bounds.width / 15;
  float row_height = bounds.height / QWERTY_ROWS;

  return ( Rectangle ){ bounds.x + offset * unit + 2, bounds.y + row * row_height + 2,
    units * unit - 4, row_height - 4 };
}

// Return the physical key under the mouse, or KEY_NULL outside keycaps.
int computer_hit( Vector2 mouse, KeyRect bounds )
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
void draw_computer_keyboard( KeyRect bounds, int base, const bool held[ MIDI_NOTES ],
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
      int note = computer_note_at( row, i, base );
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
#endif
