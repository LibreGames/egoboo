#pragma once

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

///
/// @file
/// @brief Input Definitions
/// @details Definition of the interfaces for external devices like mice, keyboard(s), joysticks...

#include "egoboo_types.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum e_control_list
{
  KEY_JUMP = 0,
  KEY_LEFT_USE,
  KEY_LEFT_GET,
  KEY_LEFT_PACK,
  KEY_RIGHT_USE,
  KEY_RIGHT_GET,
  KEY_RIGHT_PACK,
  KEY_MESSAGE,
  KEY_CAMERA_LEFT,
  KEY_CAMERA_RIGHT,
  KEY_CAMERA_IN,
  KEY_CAMERA_OUT,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,

  MOS_JUMP,
  MOS_LEFT_USE,
  MOS_LEFT_GET,
  MOS_LEFT_PACK,
  MOS_RIGHT_USE,
  MOS_RIGHT_GET,
  MOS_RIGHT_PACK,
  MOS_CAMERA,

  JOA_JUMP,
  JOA_LEFT_USE,
  JOA_LEFT_GET,
  JOA_LEFT_PACK,
  JOA_RIGHT_USE,
  JOA_RIGHT_GET,
  JOA_RIGHT_PACK,
  JOA_CAMERA,

  JOB_JUMP,
  JOB_LEFT_USE,
  JOB_LEFT_GET,
  JOB_LEFT_PACK,
  JOB_RIGHT_USE,
  JOB_RIGHT_GET,
  JOB_RIGHT_PACK,
  JOB_CAMERA,
  CONTROL_LST_COUNT,

  // !!!! OMG !!!! - these aliases have to be last or they mess up the automatic ordering
  KEY_FIRST = KEY_JUMP,
  MOS_FIRST = MOS_JUMP,
  JOA_FIRST = JOA_JUMP,
  JOB_FIRST = JOB_JUMP,

  KEY_LAST = MOS_FIRST,
  MOS_LAST = JOA_FIRST,
  JOA_LAST = JOB_FIRST,
  JOB_LAST = CONTROL_LST_COUNT

};
typedef enum e_control_list CONTROL_LIST;

enum e_input_type
{
  INPUT_MOUS = 0,
  INPUT_KEYB,
  INPUT_JOYA,
  INPUT_JOYB,
  INPUT_COUNT
};
typedef enum e_input_type INPUT_TYPE;

enum e_input_bits
{
  /// @details Input devices
  INBITS_NONE  =               0,
  INBITS_MOUS  = 1 << INPUT_MOUS,
  INBITS_KEYB  = 1 << INPUT_KEYB,
  INBITS_JOYA  = 1 << INPUT_JOYA,
  INBITS_JOYB  = 1 << INPUT_JOYB
};
typedef enum e_input_bits INPUT_BITS;


enum e_control_type
{
  CONTROL_JUMP = 0,
  CONTROL_LEFT_USE,
  CONTROL_LEFT_GET,
  CONTROL_LEFT_PACK,
  CONTROL_RIGHT_USE,
  CONTROL_RIGHT_GET,
  CONTROL_RIGHT_PACK,
  CONTROL_MESSAGE,
  CONTROL_CAMERA_LEFT,
  CONTROL_CAMERA_RIGHT,
  CONTROL_CAMERA_IN,
  CONTROL_CAMERA_OUT,
  CONTROL_UP,
  CONTROL_DOWN,
  CONTROL_LEFT,
  CONTROL_RIGHT,
  CONTROL_COUNT,

  CONTROL_CAMERA = CONTROL_MESSAGE
};
typedef enum e_control_type CONTROL;

struct sGame;

//--------------------------------------------------------------------------------------------
struct sLatch
{
  float    x;        ///< x value
  float    y;        ///< y value
  Uint32   b;        ///< button(s) mask
};
typedef struct sLatch Latch_t;

INLINE bool_t Latch_clear(Latch_t * pl) { if(NULL == pl) return bfalse; memset(pl, 0, sizeof(Latch_t)); return btrue; }



//--------------------------------------------------------------------------------------------
#define PLALST_COUNT   (1<<3)                          // 2 to a power...  2^3

struct sPlayer
{
  egoboo_key_t        ekey;
  bool_t            Active;

  bool_t            is_local;

  CHR_REF           chr_ref;                 ///< Which character?
  Latch_t            latch;                   ///< Local latches
  Uint8             device;                  ///< Input device
};
typedef struct sPlayer Player_t;

#ifdef __cplusplus
  typedef TList<sPlayer, PLALST_COUNT> PlaList_t;
  typedef TPList<sPlayer, PLALST_COUNT> PPla_t;
#else
  typedef Player_t PlaList_t[PLALST_COUNT];
  typedef Player_t * PPla_t;
#endif

Player_t * Player_new(Player_t *ppla);
bool_t    Player_delete(Player_t *ppla);
Player_t * Player_renew(Player_t *ppla);

#define VALID_PLA_RANGE(XX) ( /* ((XX)>=0) && */ ((XX)<PLALST_COUNT) )
#define VALID_PLA(LST, XX)  ( VALID_PLA_RANGE(XX) && LST[XX].Active )

//--------------------------------------------------------------------------------------------
#define MOUSEBUTTON         4

struct s_mouse
{
  bool_t   ui;                   ///< Is the mouse connected to the ui?
  bool_t   game;                 ///< Is the mouse connected to the game?
  float    sense;                ///< Sensitivity threshold
  float    sustain;              ///< Falloff rate for old movement
  float    cover;                ///< For falloff
  Latch_t  latch;
  Latch_t  latch_old;            ///< For sustain
  Latch_t  dlatch;
  Sint32   z;                    ///< Mouse wheel movement counter
  Uint8    button[MOUSEBUTTON];  ///< Mouse button states
};
typedef struct s_mouse MOUSE;

extern MOUSE mous;

//--------------------------------------------------------------------------------------------
#define JOYBUTTON           8                        ///< Maximum number of joystick buttons

struct s_joystick
{
  SDL_Joystick *sdl_device;
  bool_t        on;                     ///< Is the holy joystick live?
  Latch_t         latch;
  Uint8         button[JOYBUTTON];
};
typedef struct s_joystick JOYSTICK;

extern JOYSTICK joy[2];

//--------------------------------------------------------------------------------------------
// SDL specific declarations
struct s_keyboard
{
  bool_t   on;                 ///< Is the keyboard live?
  bool_t   mode;
  Uint8    delay;              ///< For slowing down chat input

  Uint8   *state;
  Latch_t    latch;
};
typedef struct s_keyboard KEYBOARD;

extern KEYBOARD keyb;

#define SDLKEYDOWN(k) (NULL !=keyb.state && 0!=keyb.state[k])


//--------------------------------------------------------------------------------------------
//Tags
void read_all_tags( char *szFilename );
void read_controls( char *szFilename );




//--------------------------------------------------------------------------------------------
struct sKeyboardBuffer
{
  bool_t done;
  int    write;                 ///< The cursor position
  int    writemin;              ///< The starting cursor position
  STRING buffer;                ///< The input message
};
typedef struct sKeyboardBuffer KeyboardBuffer_t;

//--------------------------------------------------------------------------------------------
void   input_setup( void );
void   input_read( void );
bool_t input_reset_press(KEYBOARD * pk);

bool_t input_read_mouse(MOUSE * pm);
bool_t input_read_key(KEYBOARD * pk);
bool_t input_read_joystick(JOYSTICK * pj);

KeyboardBuffer_t * KeyboardBuffer_getState( void );

void check_add( Uint8 key, char bigletter, char littleletter );
