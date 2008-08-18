/* Egoboo - egoboo.h
 * Disgusting, hairy, way too monolithic header file for the whole darn
 * project.  In severe need of cleaning up.  Venture here with extreme
 * caution, and bring one of those canaries with you to make sure you
 * don't run out of oxygen.
 */

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

#pragma once

#include "proto.h"
#include "Menu.h"
#include "configfile.h"
#include "Md2.h"

#include "ogl_texture.h"        // OpenGL texture loader

#include "egoboo_config.h"          // system dependent configuration information
#include "egoboo_math.h"            // vector and matrix math
#include "egoboo_types.h"           // Typedefs for egoboo

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

struct sGame;



// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#    define EXTERN
#    define EQ(x) =x;
#else
#    define EXTERN extern
#    define EQ(x)
#endif

EXTERN const char VERSION[] EQ( "2.7.x" );   // Version of the game


#define DEFAULT_SCREEN_W 640
#define DEFAULT_SCREEN_H 480

enum e_color
{
  COLR_WHITE = 0,
  COLR_RED,
  COLR_YELLOW,
  COLR_GREEN,
  COLR_BLUE,
  COLR_PURPLE
};
typedef enum e_color COLR;

#define NOSPARKLE           255

#define DELAY_RESIZE            50                      // Time it takes to resize a character


#define EXPKEEP 0.85                                // Experience to keep when respawning
#define NOHIDE              127                     // Don't hide

typedef enum e_damage_effects_bits
{
  DAMFX_NONE           = 0,                       // Damage effects
  DAMFX_ARMO           = 1 << 0,                  // Armor piercing
  DAMFX_BLOC           = 1 << 1,                  // Cannot be blocked by shield
  DAMFX_ARRO           = 1 << 2,                  // Only hurts the one it's attached to
  DAMFX_TURN           = 1 << 3,                  // Turn to attached direction
  DAMFX_TIME           = 1 << 4                   //
} DAMFX_BITS;

//Particle Texture Types
enum e_part_type
{
  PART_NORMAL,
  PART_SMOOTH,
  PART_FAST
};
typedef enum e_part_type PART_TYPE;



#define SPELLBOOK           127                     // The spellbook model TODO: change this badly thing

#define NOQUEST             -2						// Quest not found
#define QUESTBEATEN         -1                      // Quest is beaten

#define MAXSTAT             16                      // Maximum status displays



#define DELAY_DAMAGETILE      32                      // Invincibility time
#define DELAY_DAMAGE          16                      // Invincibility time
#define DELAY_DEFEND          16                      // Invincibility time


#define MAXLINESIZE         1024                    //


#define IDSZ_NONE            MAKE_IDSZ("NONE")       // [NONE]



#define RAISE 5 //25                               // Helps correct z level
#define SHADOWRAISE 5                               //
#define DAMAGERAISE 25                              //




#define NUMBAR                          6           // Number of status bars
#define TABX                            32          // Size of little name tag on the bar
#define BARX                            112         // Size of bar
#define BARY                            16          //
#define NUMTICK                         10          // Number of ticks per row
#define TICKX                           8           // X size of each tick
#define MAXTICK                         (NUMTICK*5) // Max number of ticks to draw

#define MAXTEXTURE                      512         // Max number of textures
#define MAXICONTX                       MAXTEXTURE+1


/* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000.0f


#define TARGETFPS                       30.0f
#define FRAMESKIP                       (TICKS_PER_SEC/TARGETFPS)    // 1000 tics per sec / 50 fps = 20 ticks per frame

#define EMULATEUPS                      50.0f
#define TARGETUPS                       30.0f
#define UPDATESCALE                     (EMULATEUPS/(stabilized_ups_sum/stabilized_ups_weight))
#define UPDATESKIP                      (TICKS_PER_SEC/TARGETUPS)    // 1000 tics per sec / 50 fps = 20 ticks per frame
#define ONESECOND                       (TICKS_PER_SEC/UPDATESKIP)    // 1000 tics per sec / 20 ticks per frame = 50 fps

#define STOPBOUNCING                    0.5f         // To make objects stop bouncing
#define STOPBOUNCINGPART                1.0f         // To make particles stop bouncing


struct s_tile_animated
{
  int    updateand;         // New tile every 7 frames
  Uint16 frameand;          // Only 4 frames
  Uint16 baseand;           //
  Uint16 bigframeand;       // For big tiles
  Uint16 bigbaseand;        //
  float  framefloat;        // Current frame
  Uint16 frameadd;          // Current frame
};
typedef struct s_tile_animated TILE_ANIMATED;

bool_t tile_animated_reset(TILE_ANIMATED * t);

#define NORTH 16384                                 // Character facings
#define SOUTH 49152                                 //
#define EAST 32768                                  //
#define WEST 0                                      //

#define FRONT 0                                     // Attack directions
#define BEHIND 32768                                //
#define LEFT 49152                                  //
#define RIGHT 16384                                 //


//Minimap stuff
#define MAXBLIP     32     //Max number of blips displayed on the map
EXTERN Uint16          numblip  EQ( 0 );
typedef struct s_blip
{
  Uint16          x;
  Uint16          y;
  COLR    c;

  // !!! wrong, but it will work !!!
  IRect_t           rect;           // The blip rectangles
} BLIP;

EXTERN BLIP BlipList[MAXBLIP];

EXTERN Uint8           mapon  EQ( bfalse );
EXTERN Uint8           youarehereon  EQ( bfalse );


EXTERN Uint8           timeron     EQ( bfalse );      // Game timer displayed?
EXTERN Uint32          timervalue  EQ( 0 );           // Timer_t time ( 50ths of a second )


EXTERN Sint32          ups_clock             EQ( 0 );             // The number of ticks this second
EXTERN Uint32          ups_loops             EQ( 0 );             // The number of frames drawn this second
EXTERN float           stabilized_ups        EQ( TARGETUPS );
EXTERN float           stabilized_ups_sum    EQ( TARGETUPS );
EXTERN float           stabilized_ups_weight EQ( 1 );

EXTERN Uint8           outofsync  EQ( 0 );    //Is this only for RTS? Can it be removed then?
EXTERN Uint8           parseerror EQ( 0 );    //Do we have an script error?

#define DELAY_TURN 16

//Networking
EXTERN int                     localplayer_count;                 // Number of imports from this machine
EXTERN Uint8                   localplayer_control[16];           // For local imports
EXTERN OBJ_REF                 localplayer_slot[16];              // For local imports

// EWWWW. GLOBALS ARE EVIL.


EXTERN float     foregroundrepeat  EQ( 1 );     //
EXTERN float     backgroundrepeat  EQ( 1 );     //

//Fog stuff
struct s_fog_info
{
  bool_t          on; // EQ( bfalse );            // Do ground fog?
  float           bottom; // EQ( 0.0 );          //
  float           top; // EQ( 100 );             //
  float           distance; // EQ( 100 );        //
  Uint8           red; // EQ( 255 );             //  Fog collour
  Uint8           grn; // EQ( 255 );             //
  Uint8           blu; // EQ( 255 );             //
  bool_t          affectswater;
};
typedef struct s_fog_info FOG_INFO;

bool_t fog_info_reset(FOG_INFO * f);


/*Special Textures*/
typedef enum e_tx_type
{
  TX_PARTICLE = 0,
  TX_TILE_0,
  TX_TILE_1,
  TX_TILE_2,
  TX_TILE_3,
  TX_WATER_TOP,
  TX_WATER_LOW,
  TX_PHONG,
  TX_LAST
} TX_TYPE;

/*Special Textures*/
typedef enum e_ico_type
{
  ICO_NULL,
  ICO_KEYB,
  ICO_MOUS,
  ICO_JOYA,
  ICO_JOYB,
  ICO_BOOK_0,   // The first book icon
  ICO_COUNT
} ICO_TYPE;


//Texture filtering
typedef enum e_tx_filters
{
  TX_UNFILTERED,
  TX_LINEAR,
  TX_MIPMAP,
  TX_BILINEAR,
  TX_TRILINEAR_1,
  TX_TRILINEAR_2,
  TX_ANISOTROPIC
} TX_FILTERS;

//Anisotropic filtering - yay! :P
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF


//Interface stuff
EXTERN IRect_t                    iconrect;                   // The 32x32 icon rectangle
EXTERN IRect_t                    trimrect;                   // The menu trim rectangle


EXTERN IRect_t                    tabrect[NUMBAR];            // The tab rectangles
EXTERN IRect_t                    barrect[NUMBAR];            // The bar rectangles

EXTERN Uint16                   blipwidth;
EXTERN Uint16                   blipheight;

EXTERN float                    mapscale EQ( 1.0 );
EXTERN IRect_t                    maprect;                    // The map rectangle

#define SPARKLESIZE 28
#define SPARKLEADD 2
#define MAPSIZE 96

EXTERN Uint8                   lightdirectionlookup[UINT16_SIZE];// For lighting characters

#define MEG              0x00100000
#define BUFFER_SIZE     (4 * MEG)


struct s_twist_entry
{
  Uint32       lr;           // For surface normal of mesh
  Uint32       ud;           //
  vect3        nrm;          // For sliding down steep hills
  bool_t       flat;         //
};
typedef struct s_twist_entry TWIST_ENTRY;

EXTERN TWIST_ENTRY twist_table[256];

typedef enum e_ORDER
{
  MESSAGE_SIGNAL = 0,
  MESSAGE_MOVE = 1,
  MESSAGE_ATTACK,
  MESSAGE_ASSIST,
  MESSAGE_STAND,
  MESSAGE_TERRAIN
} ORDER;

typedef enum e_SIGNAL
{
  SIGNAL_BUY     = 0,
  SIGNAL_SELL,
  SIGNAL_REJECT,
  SIGNAL_ENTERPASSAGE,
} SIGNAL;


typedef enum e_move
{
  MOVE_MELEE = 300,
  MOVE_RANGED = -600,
  MOVE_DISTANCE = -175,
  MOVE_RETREAT = 900,
  MOVE_CHARGE = 1500,
  MOVE_FOLLOW = 0
} MOVE;

typedef enum e_search_bits
{
  SEARCH_DEAD      = 1 << 0,
  SEARCH_ENEMIES   = 1 << 1,
  SEARCH_FRIENDS   = 1 << 2,
  SEARCH_ITEMS     = 1 << 3,
  SEARCH_INVERT    = 1 << 4
} SEARCH_BITS;






//------------------------------------
//Particle variables
//------------------------------------
EXTERN float           textureoffset[256];         // For moving textures


EXTERN char            cFrameName[16];                                     // MD2 Frame Name

// My lil' random number table
#define MAXRAND 4096
EXTERN Uint16 randie[MAXRAND];

#define RANDIE(IND) randie[IND]; IND++; IND %= MAXRAND;
#define FRAND(PSEED) ( 2.0f*( float ) ego_rand_32(PSEED) / ( float ) (1 << 16) / ( float ) (1 << 16) - 1.0f )
#define RAND(PSEED, MINVAL, MAXVAL) ((((ego_rand_32(PSEED) >> 16) * (MAXVAL-MINVAL)) >> 16)  + MINVAL)
#define IRAND(PSEED, BITS) ( ego_rand_32(PSEED) & ((1<<BITS)-1) )


EXTERN Uint32 particletrans_fp8  EQ( 0x80 );
EXTERN Uint32 antialiastrans_fp8  EQ( 0xC0 );


//Network Stuff
//#define CHARVEL 5.0


EXTERN bool_t          usefaredge;                     // Far edge maps? (Outdoor)
EXTERN float       doturntime;                     // Time for smooth turn

EXTERN STRING      CStringTmp1, CStringTmp2;


struct sNet;
struct sClient;
struct sServer;


//--------------------------------------------------------------------------------------------
struct sClockState;

struct sMachineState
{
  egoboo_key_t ekey;

  time_t i_actual_time;
  double f_bishop_time;
  int    i_bishop_time_y;
  int    i_bishop_time_m;
  int    i_bishop_time_d;
  float  f_bishop_time_h;

  struct sClockState * clk;
};
typedef struct sMachineState MachineState_t;

MachineState_t * get_MachineState(void);
retval_t         MachineState_update(MachineState_t * mac);


#include "module.h"

EXTERN const char *globalname  EQ( NULL );   // For debuggin' fgoto_colon
EXTERN const char *globalparsename  EQ( NULL );  // The SCRIPT.TXT filename


bool_t add_quest_idsz( char *whichplayer, IDSZ idsz );
int    modify_quest_idsz( char *whichplayer, IDSZ idsz, int adjustment );
int    check_player_quest( char *whichplayer, IDSZ idsz );
