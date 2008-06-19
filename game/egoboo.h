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
#include "menu.h"
#include "configfile.h"
#include "Md2.h"

#include "ogl_texture.h"        // OpenGL texture loader

#include "egoboo_config.h"          // system dependent configuration information
#include "egoboo_math.inl"            // vector and matrix math
#include "egoboo_types.inl"           // Typedefs for egoboo

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

struct CGame_t;



// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#    define EXTERN
#    define EQ(x) =x;
#else
#    define EXTERN extern
#    define EQ(x)
#endif

EXTERN const char VERSION[] EQ( "2.7.x" );   // Version of the game

#define NOSPARKLE           255

#define DELAY_RESIZE            50                      // Time it takes to resize a character



#define MAXIMPORT           (32*9)                  // Number of subdirs in IMPORT directory
#define EXPKEEP 0.85                                // Experience to keep when respawning
#define NOHIDE              127                     // Don't hide

typedef enum damage_effects_bits_e
{
  DAMFX_NONE           = 0,                       // Damage effects
  DAMFX_ARMO           = 1 << 0,                  // Armor piercing
  DAMFX_BLOC           = 1 << 1,                  // Cannot be blocked by shield
  DAMFX_ARRO           = 1 << 2,                  // Only hurts the one it's attached to
  DAMFX_TURN           = 1 << 3,                  // Turn to attached direction
  DAMFX_TIME           = 1 << 4                   //
} DAMFX_BITS;

//Particle Texture Types
typedef enum part_type
{
  PART_NORMAL,
  PART_SMOOTH,
  PART_FAST
} PART_TYPE;

typedef enum blud_level_e
{
  BLUD_NONE = 0,
  BLUD_NORMAL,
  BLUD_ULTRA                         // This makes any damage draw blud
} BLUD_LEVEL;

#define SPELLBOOK           127                     // The spellbook model TODO: change this badly thing


#define NOQUEST             -2						// Quest not found
#define QUESTBEATEN         -1                      // Quest is beaten

#define MAXSTAT             16                      // Maximum status displays

#define MAXMESSAGE          6                       // Number of messages
#define MAXTOTALMESSAGE     1024                    //
#define MESSAGESIZE         80                      //
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)
#define DELAY_MESSAGE         200                     // Time to keep the message alive


#define DELAY_DAMAGETILE      32                      // Invincibility time
#define DELAY_DAMAGE          16                      // Invincibility time
#define DELAY_DEFEND          16                      // Invincibility time


#define MAXLINESIZE         1024                    //


#define IDSZ_NONE            MAKE_IDSZ("NONE")       // [NONE]



#define RAISE 5 //25                               // Helps correct z level
#define SHADOWRAISE 5                               //
#define DAMAGERAISE 25                              //

#define MAXWATERLAYER 2                             // Maximum water layers
#define MAXWATERFRAME 512                           // Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)             //
#define WATERPOINTS 4                               // Points in a water fan
#define WATERMODE 4                                 // Ummm...  For making it work, yeah...



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


typedef struct tile_animated_t
{
  int    updateand; //  EQ( 7 );                        // New tile every 7 frames
  Uint16 frameand; //  EQ( 3 );              // Only 4 frames
  Uint16 baseand; //  EQ( 0xfffc );          //
  Uint16 bigframeand; //  EQ( 7 );           // For big tiles
  Uint16 bigbaseand; //  EQ( 0xfff8 );       //
  float  framefloat; //  EQ( 0 );              // Current frame
  Uint16 frameadd; //  EQ( 0 );              // Current frame
} TILE_ANIMATED;

EXTERN TILE_ANIMATED GTile_Anim;

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
typedef struct blip_t
{
  Uint16          x;
  Uint16          y;
  COLR            c;

  // !!! wrong, but it will work !!!
  IRect           rect;           // The blip rectangles
} BLIP;

EXTERN BLIP BlipList[MAXBLIP];

EXTERN Uint8           mapon  EQ( bfalse );
EXTERN Uint8           youarehereon  EQ( bfalse );


EXTERN Uint8           timeron     EQ( bfalse );      // Game timer displayed?
EXTERN Uint32          timervalue  EQ( 0 );           // Timer time ( 50ths of a second )


EXTERN Sint32          ups_clock             EQ( 0 );             // The number of ticks this second
EXTERN Uint32          ups_loops             EQ( 0 );             // The number of frames drawn this second
EXTERN float           stabilized_ups        EQ( TARGETUPS );
EXTERN float           stabilized_ups_sum    EQ( TARGETUPS );
EXTERN float           stabilized_ups_weight EQ( 1 );

EXTERN Sint32          fps_clock             EQ( 0 );             // The number of ticks this second
EXTERN Uint32          fps_loops             EQ( 0 );             // The number of frames drawn this second
EXTERN float           stabilized_fps        EQ( TARGETFPS );
EXTERN float           stabilized_fps_sum    EQ( TARGETFPS );
EXTERN float           stabilized_fps_weight EQ( 1 );


EXTERN Uint8           outofsync  EQ( 0 );    //Is this only for RTS? Can it be removed then?
EXTERN Uint8           parseerror EQ( 0 );    //Do we have an script error?

#define DELAY_TURN 16

//Networking
EXTERN int                     localplayer_count;                 // Number of imports from this machine
EXTERN Uint8                   localplayer_control[16];           // For local imports
EXTERN OBJ_REF                 localplayer_slot[16];              // For local imports

// EWWWW. GLOBALS ARE EVIL.

typedef struct water_layer_t
{
  Uint16    lightlevel_fp8; // General light amount (0-63)
  Uint16    lightadd_fp8;   // Ambient light amount (0-63)
  Uint16    alpha_fp8;      // Transparency

  float     u;              // Coordinates of texture
  float     v;              //
  float     uadd;           // Texture movement
  float     vadd;           //

  float     amp;            // Amplitude of waves
  float     z;              // Base height of water
  float     zadd[MAXWATERFRAME][WATERMODE][WATERPOINTS];
  Uint8     color[MAXWATERFRAME][WATERMODE][WATERPOINTS];
  Uint16    frame;          // Frame
  Uint16    frameadd;       // Speed

  float     distx;          // For distant backgrounds
  float     disty;          //
} WATER_LAYER;

typedef struct water_info_t
{
  Uint8     shift ; // EQ( 3 );
  float     surfacelevel; // EQ( 0 );          // Surface level for water striders
  float     douselevel; // EQ( 0 );            // Surface level for torches
  bool_t    light; // EQ( 0 );                 // Is it light ( default is alpha )
  Uint8     spekstart; // EQ( 128 );           // Specular begins at which light value
  Uint8     speklevel_fp8; // EQ( 128 );           // General specular amount (0-255)
  bool_t    iswater ; // EQ( btrue );          // Is it water?  ( Or lava... )

  int         layer_count; // EQ( 0 );              // Number of layers
  WATER_LAYER layer[MAXWATERLAYER];

  Uint32    spek[256];             // Specular highlights
} WATER_INFO;

EXTERN float     foregroundrepeat  EQ( 1 );     //
EXTERN float     backgroundrepeat  EQ( 1 );     //

//Fog stuff
typedef struct fog_info_t
{
  bool_t          on; // EQ( bfalse );            // Do ground fog?
  float           bottom; // EQ( 0.0 );          //
  float           top; // EQ( 100 );             //
  float           distance; // EQ( 100 );        //
  Uint8           red; // EQ( 255 );             //  Fog collour
  Uint8           grn; // EQ( 255 );             //
  Uint8           blu; // EQ( 255 );             //
  bool_t          affectswater;
} FOG_INFO;

EXTERN FOG_INFO GFog;

/*OpenGL Textures*/
typedef enum tx_type_e
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


//Texture filtering
typedef enum tx_filters_e
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
EXTERN IRect                    iconrect;                   // The 32x32 icon rectangle
EXTERN IRect                    trimrect;                   // The menu trim rectangle


EXTERN IRect                    tabrect[NUMBAR];            // The tab rectangles
EXTERN IRect                    barrect[NUMBAR];            // The bar rectangles

EXTERN Uint16                   blipwidth;
EXTERN Uint16                   blipheight;

EXTERN float                    mapscale EQ( 1.0 );
EXTERN IRect                    maprect;                    // The map rectangle

#define SPARKLESIZE 28
#define SPARKLEADD 2
#define MAPSIZE 96

EXTERN Uint8                   lightdirectionlookup[UINT16_SIZE];// For lighting characters

#define MEG              0x00100000
#define BUFFER_SIZE     (4 * MEG)


typedef struct twist_entry_t
{
  Uint32       lr;           // For surface normal of mesh
  Uint32       ud;           //
  vect3        nrm;          // For sliding down steep hills
  bool_t       flat;         //
} TWIST_ENTRY;

EXTERN TWIST_ENTRY twist_table[256];

typedef enum order_t
{
  MESSAGE_MOVE    = 0,
  MESSAGE_ATTACK,
  MESSAGE_ASSIST,
  MESSAGE_STAND,
  MESSAGE_TERRAIN,
  MESSAGE_ENTERPASSAGE
} ORDER;

typedef enum color_e
{
  COLOR_WHITE = 0,
  COLOR_RED,
  COLOR_YELLOW,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_PURPLE
} COLOR;

typedef enum move_t
{
  MOVE_MELEE = 300,
  MOVE_RANGED = -600,
  MOVE_DISTANCE = -175,
  MOVE_RETREAT = 900,
  MOVE_CHARGE = 1500,
  MOVE_FOLLOW = 0
} MOVE;

typedef enum search_bits_e
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


//------------------------------------
// Physics variables
//------------------------------------

typedef struct CPhysicsData_t
{
  bool_t   initialized;

  float    hillslide;                 // Friction
  float    slippyfriction;            //
  float    airfriction;               //
  float    waterfriction;             //
  float    noslipfriction;            //
  float    platstick;                 //

  float    gravity;                   // Gravitational accel
} CPhysicsData;

CPhysicsData * CPhysicsData_new(CPhysicsData * phys);
bool_t         CPhysicsData_delete(CPhysicsData * phys);
CPhysicsData * CPhysicsData_renew(CPhysicsData * phys);


EXTERN char            cFrameName[16];                                     // MD2 Frame Name

// Display messages
typedef struct message_element_t
{
  Sint16    time;                                //
  char      textdisplay[MESSAGESIZE];            // The displayed text

} MESSAGE_ELEMENT;

typedef struct MessageData_t
{

  // Message files
  Uint16  total;                                         // The number of messages
  Uint32  totalindex;                                    // Where to put letter

  Uint32  index[MAXTOTALMESSAGE];                        // Where it is
  char    text[MESSAGEBUFFERSIZE];                       // The text buffer
} MessageData;

typedef struct MessageQueue_t
{
  int             count;

  Uint16          start;
  MESSAGE_ELEMENT list[MAXMESSAGE];
  float           timechange;
} MessageQueue;

// My lil' random number table
#define MAXRAND 4096
EXTERN Uint16 randie[MAXRAND];
#define RANDIE(PST) randie[PST->randie_index]; PST->randie_index++; PST->randie_index %= MAXRAND;


EXTERN Uint32 particletrans_fp8  EQ( 0x80 );
EXTERN Uint32 antialiastrans_fp8  EQ( 0xC0 );


//Network Stuff
//#define CHARVEL 5.0


typedef struct ConfigData_t
{
  STRING basicdat_dir;
  STRING gamedat_dir;
  STRING mnu_dir;
  STRING globalparticles_dir;
  STRING modules_dir;
  STRING music_dir;
  STRING objects_dir;
  STRING import_dir;
  STRING players_dir;

  STRING nullicon_bitmap;
  STRING keybicon_bitmap;
  STRING mousicon_bitmap;
  STRING joyaicon_bitmap;
  STRING joybicon_bitmap;

  STRING tile0_bitmap;
  STRING tile1_bitmap;
  STRING tile2_bitmap;
  STRING tile3_bitmap;
  STRING watertop_bitmap;
  STRING waterlow_bitmap;
  STRING phong_bitmap;
  STRING plan_bitmap;
  STRING blip_bitmap;
  STRING font_bitmap;
  STRING icon_bitmap;
  STRING bars_bitmap;
  STRING particle_bitmap;
  STRING title_bitmap;

  STRING menu_main_bitmap;
  STRING menu_advent_bitmap;
  STRING menu_sleepy_bitmap;
  STRING menu_gnome_bitmap;

  STRING debug_file;
  STRING passage_file;
  STRING aicodes_file;
  STRING actions_file;
  STRING alliance_file;
  STRING fans_file;
  STRING fontdef_file;
  STRING mnu_file;
  STRING money1_file;
  STRING money5_file;
  STRING money25_file;
  STRING money100_file;
  STRING weather1_file;
  STRING weather2_file;
  STRING script_file;
  STRING ripple_file;
  STRING scancode_file;
  STRING playlist_file;
  STRING spawn_file;
  STRING wawalite_file;
  STRING defend_file;
  STRING splash_file;
  STRING mesh_file;
  STRING setup_file;
  STRING log_file;
  STRING controls_file;
  STRING data_file;
  STRING copy_file;
  STRING enchant_file;
  STRING message_file;
  STRING naming_file;
  STRING modules_file;
  STRING skin_file;
  STRING credits_file;
  STRING quest_file;

  int    uifont_points;
  int    uifont_points2;
  STRING uifont_ttf;

  //Global Sounds
  STRING coinget_sound;
  STRING defend_sound;
  STRING coinfall_sound;
  STRING lvlup_sound;

  //------------------------------------
  //Setup Variables
  //------------------------------------
  bool_t zreflect;                   // Reflection z buffering?
  int    maxtotalmeshvertices;       // of vertices
  bool_t fullscreen;                 // Start in fullscreen?
  int    scrd;                       // Screen bit depth
  int    scrx;                       // Screen X size
  int    scry;                       // Screen Y size
  int    scrz;                       // Screen z-buffer depth ( 8 unsupported )
  int    maxmessage;                 //
  bool_t messageon;                  // Messages?
  int    wraptolerance;              // Status bar
  bool_t  staton;                    // Draw the status bars?
  bool_t  render_overlay;            // Draw overlay?
  bool_t  render_background;         // Do we render the water as a background?
  GLenum  perspective;               // Perspective correct textures?
  bool_t  dither;                    // Dithering?
  Uint8   reffadeor;                 // 255 = Don't fade reflections
  GLenum  shading;                   // Gourad shading?
  bool_t  antialiasing;              // Antialiasing?
  bool_t  refon;                     // Reflections?
  bool_t  shaon;                     // Shadows?
  int     texturefilter;             // Texture filtering?
  bool_t  wateron;                   // Water overlays?
  bool_t  shasprite;                 // Shadow sprites?
  bool_t  phongon;                   // Do phong overlay? (Outdated?)
  bool_t  twolayerwateron;           // Two layer water?
  bool_t  overlayvalid;              // Allow large overlay?
  bool_t  backgroundvalid;           // Allow large background?
  bool_t  fogallowed;                // Draw fog? (Not implemented!)
  int     particletype;              // Particle Effects image
  bool_t  vsync;                     // Wait for vertical sync?
  bool_t  gfxacceleration;           // Force OpenGL graphics acceleration?

  bool_t  allow_sound;       // Allow playing of sound?
  bool_t  allow_music;       // Allow music and loops?
  int     musicvolume;       // The sound volume of music
  int     soundvolume;       // Volume of sounds played
  int     maxsoundchannel;   // Max number of sounds playing at the same time
  int     buffersize;        // Buffer size set in setup.txt

  Uint8 autoturncamera;           // Type of camera control...

  bool_t  request_network;              // Try to connect?
  int     lag;                    // Lag tolerance
  STRING  net_hosts[8];                            // Name for hosting session
  STRING  net_messagename;         // Name for messages
  Uint8   fpson;                    // FPS displayed?

  // Debug options
  SDL_GrabMode GrabMouse;
  bool_t HideMouse;
  bool_t DevMode;
  // Debug options

} ConfigData;

char * get_config_string(ConfigData * cd, char * szin, char ** szout);
char * get_config_string_name(ConfigData * cd, STRING * pconfig_string);


EXTERN bool_t          usefaredge;                     // Far edge maps? (Outdoor)
EXTERN float       doturntime;                     // Time for smooth turn

EXTERN STRING      CStringTmp1, CStringTmp2;
EXTERN ConfigData CData_default, CData;

struct CNet_t;
struct CClient_t;
struct CServer_t;


//--------------------------------------------------------------------------------------------
struct ClockState_t;

typedef struct MachineState_t
{
  bool_t initialized;

  struct ClockState_t * clk;
} MachineState;

MachineState * Get_MachineState(void);

#include "module.h"

EXTERN char *globalname  EQ( NULL );   // For debuggin' fgoto_colon
EXTERN char *globalparsename  EQ( NULL );  // The SCRIPT.TXT filename


bool_t add_quest_idsz( char *whichplayer, IDSZ idsz );
int    modify_quest_idsz( char *whichplayer, IDSZ idsz, int adjustment );
int    check_player_quest( char *whichplayer, IDSZ idsz );
