//********************************************************************************************
//* Egoboo - input.c
//*
//* Keyboard, mouse, and joystick handling code.
//*
//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "input.inl"
#include "Ui.h"
#include "Log.h"
#include "Network.h"
#include "Client.h"

#include "egoboo_utility.h"
#include "egoboo.h"

// The SDL virtual key values are the ASCII values of a 102-key qwerty keyboard
// SDL does not automatically translate the shift key.
char _shiftvals[SDLK_LAST];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

KeyboardBuffer _keybuff;

MOUSE mous =
{
  btrue,            // on
  2.0f,             // sense
  0.9f,             // sustain
  0.1f,             // cover
  {0.0f, 0.0f, 0},  // latch
  {0.0f, 0.0f, 0},  // latch_old
  {0.0f, 0.0f, 0},  // dlatch
  0,                // z
};

JOYSTICK joy[2] =
{
  // "joya"
  {
    NULL,            // sdl_device
    bfalse,          // on
    {0.0f, 0.0f, 0}  // latch
  },

  // "joyb"
  {
    NULL,            // sdl_device
    bfalse,          // on
    {0.0f, 0.0f, 0}  // latch
  }
};

KEYBOARD keyb =
{
  btrue,            //  on
  bfalse,           // mode
  20,               // delay

  NULL,             //  state
  {0.0f, 0.0f, 0}   // latch
};

//--------------------------------------------------------------------------------------------
KeyboardBuffer * KeyboardBuffer_getState() { return &_keybuff; };
void             input_init_keybuffer();

//--------------------------------------------------------------------------------------------

//Key/Control input defenitions
#define MAXTAG              128                     // Number of tags in scancode.txt
#define TAGSIZE             32                      // Size of each tag

typedef struct scancode_data_t
{
  char   name[TAGSIZE];             // Scancode names
  Uint32 value;                     // Scancode values
} SCANCODE_DATA;

typedef struct scantag_list_t
{
  int           count;
  SCANCODE_DATA data[MAXTAG];
} SCANTAG_LIST;

SCANTAG_LIST tags;


//--------------------------------------------------------------------------------------------
// Tags
static void reset_tags();
static int read_tag( FILE *fileread );
static int tag_value( char *string );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void input_setup()
{
  if ( SDL_NumJoysticks() > 0 )
  {
    joy[0].sdl_device = SDL_JoystickOpen( 0 );
    joy[0].on = (NULL != joy[0].sdl_device);
  }

  if ( SDL_NumJoysticks() > 1 )
  {
    joy[1].sdl_device = SDL_JoystickOpen( 1 );
    joy[1].on = (NULL != joy[1].sdl_device);
  }

  input_init_keybuffer();
  memset( &_keybuff, 0, sizeof(KeyboardBuffer));

  // I would love a way to enable and disable this, but there is no
  // SDL_DisableKeyRepeat() function
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL); 
}

//--------------------------------------------------------------------------------------------
bool_t input_read_mouse(MOUSE * pm)
{
  int x,y,b;

  if(NULL == pm) return bfalse;

  if ( pm->on )
  {
    b = SDL_GetRelativeMouseState( &x, &y );

    pm->dlatch.x = x;
    pm->dlatch.y = y;

    pm->latch.x += pm->dlatch.x;
    pm->latch.x = CLIP(pm->latch.x, 0, CData.scrx);

    pm->latch.y += pm->dlatch.y;
    pm->latch.y = CLIP(pm->latch.y, 0, CData.scry);

    pm->button[0] = ( b & SDL_BUTTON( 1 ) ) ? 1 : 0;
    pm->button[1] = ( b & SDL_BUTTON( 3 ) ) ? 1 : 0;
    pm->button[2] = ( b & SDL_BUTTON( 2 ) ) ? 1 : 0;  // Middle is 2 on SDL
    pm->button[3] = ( b & SDL_BUTTON( 4 ) ) ? 1 : 0;
  }
  else
  {
    SDL_GetMouseState( &x, &y );

    pm->latch.x = x;
    pm->latch.y = y;

    pm->dlatch.x = pm->dlatch.y = 0;

    pm->button[0] = 0;
    pm->button[1] = 0;
    pm->button[2] = 0;
    pm->button[3] = 0;
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t input_read_key(KEYBOARD * pk)
{
  if(NULL == pk) return bfalse;

  pk->state = SDL_GetKeyState( NULL );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t input_read_joystick(JOYSTICK * pj)
{
  int button;
  float jx, jy;
  const float jthresh = .25;

  if(NULL == pj) return bfalse;

  if (!pj->on || NULL == pj->sdl_device )
  {
    CLatch_clear( &(pj->latch) );

    return (NULL != pj->sdl_device);
  };

  jx = (Sint16)SDL_JoystickGetAxis( pj->sdl_device, 0 ) / (float)(1<<15);
  jy = (Sint16)SDL_JoystickGetAxis( pj->sdl_device, 1 ) / (float)(1<<15);

  pj->latch.x = jx;
  pj->latch.y = jy;
  if ( ABS( jx ) + ABS( jy ) >= jthresh )
  {
    float jr2, r2;

    r2 = jx * jx + jy * jy;
    jr2 = MAX( 0 , r2-jthresh*jthresh) / (1.0f - jthresh*jthresh);

    pj->latch.x *= jr2 / r2;
    pj->latch.y *= jr2 / r2;
  }

  button = SDL_JoystickNumButtons( pj->sdl_device );
  while ( button >= 0 )
  {
    pj->button[button] = SDL_JoystickGetButton( pj->sdl_device, button );
    button--;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t input_reset_press(KEYBOARD * pk)
{
  // ZZ> This function resets key press information

  return (NULL != pk);

  /*PORT
      int cnt;
      cnt = 0;
      while(cnt < 256)
      {
          keypress[cnt] = bfalse;
          cnt++;
      }
  */
}


//--------------------------------------------------------------------------------------------
//Tag Reading---------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void reset_tags()
{
  //This function resets the tags
  tags.count = 0;
}

//--------------------------------------------------------------------------------------------
int read_tag( FILE *fileread )
{
  // ZZ> This function finds the next tag, returning btrue if it found one

  if ( tags.count < MAXTAG )
  {
    if ( fget_next_string( fileread, tags.data[tags.count].name, sizeof( tags.data[tags.count].name ) ) )
    {
      tags.data[tags.count].value = fget_int( fileread );
      tags.count++;
      return btrue;
    }
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void read_all_tags( char *szFilename )
{
  // ZZ> This function reads the scancode.txt file

  FILE* fileread;

  reset_tags();
  fileread = fs_fileOpen( PRI_WARN, NULL, szFilename, "r" );
  if ( NULL == fileread )
  {
    log_warning("Could not read input codes (%s)\n", szFilename);
    return;
  };

  while ( read_tag( fileread ) );
  fs_fileClose( fileread );
}

//--------------------------------------------------------------------------------------------
int tag_value( char *string )
{
  // ZZ> This function matches the string with its tag, and returns the value...
  //     It will return 255 if there are no matches.

  int cnt;

  for (cnt = 0; cnt < tags.count; cnt++ )
  {
    if (0 == strcmp( string, tags.data[cnt].name ))
    {
      // They match
      return tags.data[cnt].value;
    }
  }

  // No matches
  return -1;
}

//--------------------------------------------------------------------------------------------
void read_controls( char *szFilename )
{
  // ZZ> This function reads the "CONTROLS.TXT" file

  FILE* fileread;
  char currenttag[TAGSIZE];
  int tnc;
  INPUT_TYPE   input;
  CONTROL_LIST control;

  log_info( "read_controls() - reading control settings from the %s file.\n", szFilename );

  fileread = fs_fileOpen( PRI_WARN, NULL, szFilename, "r" );
  if ( NULL != fileread )
  {
    input = INPUT_KEYB;
    for ( control = KEY_FIRST, tnc = 0; control < KEY_LAST; control = ( CONTROL_LIST )( control + 1 ), tnc++ )
    {
      fget_next_string( fileread, currenttag, sizeof( currenttag ) );
      control_list[input][tnc].value = tag_value( currenttag );
      control_list[input][tnc].is_key = ( currenttag[0] == 'K' );
    }

    input = INPUT_MOUS;
    for ( control = MOS_FIRST, tnc = 0; control < MOS_LAST; control = ( CONTROL_LIST )( control + 1 ), tnc++ )
    {
      fget_next_string( fileread, currenttag, sizeof( currenttag ) );
      control_list[input][tnc].value = tag_value( currenttag );
      control_list[input][tnc].is_key = ( currenttag[0] == 'K' );
    }

    input = INPUT_JOYA;
    for ( control = JOA_FIRST, tnc = 0; control < JOA_LAST; control = ( CONTROL_LIST )( control + 1 ), tnc++ )
    {
      fget_next_string( fileread, currenttag, sizeof( currenttag ) );
      control_list[input][tnc].value = tag_value( currenttag );
      control_list[input][tnc].is_key = ( currenttag[0] == 'K' );
    }

    input = INPUT_JOYB;
    for ( control = JOB_FIRST, tnc = 0; control < JOB_LAST; control = ( CONTROL_LIST )( control + 1 ), tnc++ )
    {
      fget_next_string( fileread, currenttag, sizeof( currenttag ) );
      control_list[input][tnc].value = tag_value( currenttag );
      control_list[input][tnc].is_key = ( currenttag[0] == 'K' );
    }

    fs_fileClose( fileread );
  }
  else log_warning( "Could not load input settings (%s)\n", szFilename );
}

//--------------------------------------------------------------------------------------------
bool_t input_handleSDLEvent(SDL_Event * evt)
{
  bool_t handled = bfalse;

  switch ( evt->type )
  {
    case SDL_KEYDOWN:

      // kandle the keyboard input here, where it is a lot easier!
      if(evt->key.keysym.sym == SDLK_RETURN || evt->key.keysym.sym == SDLK_KP_ENTER)
      {
        // flip the mode
        keyb.mode = !keyb.mode;

        // we are done if the keybuffer is not empty and the mode
        // just changed to "off"
        _keybuff.done = ((_keybuff.write-_keybuff.writemin)>0) && !keyb.mode;

        if(keyb.mode)
        {
          int len;

          // reset the buffer
          _keybuff.write    = 0;
          _keybuff.writemin = 0;

          // Copy the name
          strncpy(_keybuff.buffer, CData.net_messagename, sizeof(STRING));
          strcat(_keybuff.buffer, "> ");
          len = strlen(_keybuff.buffer);

          _keybuff.write    = len;
          _keybuff.writemin = len;
        }
      }
      else if(keyb.mode && _keybuff.write < sizeof(STRING)-4)
      {
        if(evt->key.keysym.sym == SDLK_BACKSPACE)
        {
          int minpos = MAX(0, _keybuff.writemin);

          _keybuff.write--; if(_keybuff.write <= minpos) _keybuff.write = minpos;
        }
        else
        {
          Uint32 mod = evt->key.keysym.mod;
          int    sym = evt->key.keysym.sym;
          bool_t altkeys = HAS_SOME_BITS(mod, KMOD_CTRL|KMOD_ALT);
          bool_t shifted = HAS_SOME_BITS(mod, KMOD_SHIFT) && !altkeys;
          bool_t caps    = HAS_SOME_BITS(mod, KMOD_CAPS) && !altkeys;

          // do not accept control or alt modified keystrokes
          if(!altkeys && sym<256 && isprint(sym))
          {
            if(shifted)
            {
              _keybuff.buffer[_keybuff.write++] = _shiftvals[sym];
            }
            else if(caps)
            {
              _keybuff.buffer[_keybuff.write++] = toupper(sym);
            }
            else if(!altkeys)
            {
              _keybuff.buffer[_keybuff.write++] = sym;
            }
          }
        };

        _keybuff.buffer[_keybuff.write] = '\0';
      }
      handled = btrue;
      break;

  }


  return handled;
}

//--------------------------------------------------------------------------------------------
void input_read()
{
  // ZZ> This function gets all the current player input states

  int cnt;
  SDL_Event evt;

  // Run through SDL's event loop to get info in the way that we want
  // it for the Gui code
  while ( SDL_PollEvent( &evt ) )
  {
    bool_t handled = bfalse;

    if(!handled) handled = ui_handleSDLEvent( &evt );
    if(!handled) handled = input_handleSDLEvent( &evt );
  }

  // Get immediate mode state for the rest of the game
  input_read_key(&keyb);
  input_read_mouse(&mous);

  SDL_JoystickUpdate();
  input_read_joystick(joy);
  input_read_joystick(joy + 1);

  // Joystick mask
  joy[0].latch.b = 0;
  joy[1].latch.b = 0;
  for ( cnt = 0; cnt < JOYBUTTON; cnt++ )
  {
    joy[0].latch.b |= ( joy[0].button[cnt] << cnt );
    joy[1].latch.b |= ( joy[1].button[cnt] << cnt );
  }

  // Mouse mask
  mous.latch.b = 0;
  for ( cnt = 0; cnt < 4; cnt++ )
  {
    mous.latch.b |= ( mous.button[cnt] << cnt );
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CPlayer * Player_new(CPlayer *ppla)    
{ 
  //fprintf( stdout, "Player_new()\n");

  if(NULL==ppla) return ppla; 

  memset(ppla, 0, sizeof(CPlayer));
  ppla->used = bfalse;

  return ppla; 
};

//--------------------------------------------------------------------------------------------
bool_t   Player_delete(CPlayer *ppla) 
{ 
  if(NULL==ppla) return bfalse;

  ppla->used = bfalse;
  
  return btrue; 
};

//--------------------------------------------------------------------------------------------
CPlayer * Player_renew(CPlayer *ppla)
{
  Player_delete(ppla);
  return Player_new(ppla);
};

//--------------------------------------------------------------------------------------------
void input_init_keybuffer()
{
  int i;

  // do the default ASCII upper case
  for(i=0; i<SDLK_LAST; i++)
  {
    _shiftvals[i] = toupper(i);
  }

  // do some special cases
  _shiftvals[SDLK_1]            = '!';
  _shiftvals[SDLK_2]            = '@';
  _shiftvals[SDLK_3]            = '#';
  _shiftvals[SDLK_4]            = '$';
  _shiftvals[SDLK_5]            = '%';
  _shiftvals[SDLK_6]            = '^';
  _shiftvals[SDLK_7]            = '&';
  _shiftvals[SDLK_8]            = '*';
  _shiftvals[SDLK_9]            = '(';
  _shiftvals[SDLK_0]            = ')';
  _shiftvals[SDLK_QUOTE]        = '\"';
  _shiftvals[SDLK_SPACE]        = ' ';
  _shiftvals[SDLK_SEMICOLON]    = ':';
  _shiftvals[SDLK_PERIOD]       = '>';
  _shiftvals[SDLK_COMMA]        = '<';
  _shiftvals[SDLK_BACKQUOTE]    = '`';
  _shiftvals[SDLK_MINUS]        = '_';
  _shiftvals[SDLK_EQUALS]       = '+';
  _shiftvals[SDLK_LEFTBRACKET]  = '{';
  _shiftvals[SDLK_RIGHTBRACKET] = '}';
  _shiftvals[SDLK_BACKSLASH]    = '|';
  _shiftvals[SDLK_SLASH]        = '?';
}