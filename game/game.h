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
/// @brief Declarations for the Game.
/// @details Definitions of all the basic structures needed to create and run egoboo games.

#include "input.h"
#include "module.h"

#include "char.h"
#include "particle.h"
#include "enchant.h"
#include "passage.h"
#include "mesh.h"
#include "Menu.h"
#include "script.h"
#include "graphic.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct sNet;
struct sConfigData;
struct sStatus;
struct sChr;
struct sSoundState;
struct sGraphics;
struct s_script_global_values;
struct sGameStack;

struct sClient;
struct sServer;

enum e_Action;
enum e_Team;
enum e_Experience;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// This is for random naming
#define CHOPPERMODEL                    32
#define MAXCHOP                         (OBJLST_COUNT*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPBUFFERSIZE                   (MAXCHOP*CHOPSIZE)

struct sChopData
{
  Uint16   count;                  ///< The number of name parts
  Uint32   write;                  ///< The data pointer
  char     text[CHOPBUFFERSIZE];    ///< The name parts
  Uint16   start[MAXCHOP];         ///< The first character of each part
};
typedef struct sChopData ChopData_t;

//--------------------------------------------------------------------------------------------
//Weather and water gfx
struct s_weather_info
{
  bool_t    active;
  bool_t    require_water;    ///< Only spawn over water?
  int       timereset;        ///< Rate at which weather particles spawn
  float     time;
  PLA_REF   player;
};
typedef struct s_weather_info WEATHER_INFO;

bool_t Weather_init(WEATHER_INFO * w);

//--------------------------------------------------------------------------------------------
#define MAXENDTEXT 1024

enum e_net_status_bits
{
  NET_STATUS_LOCAL   = 0,
  NET_STATUS_CLIENT  = (1 << 0),
  NET_STATUS_SERVER  = (1 << 1)
};

typedef enum e_net_status NET_STATUS_BITS;

struct sGame
{
  egoboo_key_t ekey;

  ProcState_t  proc;

  bool_t Single_frame;       ///< Is the game in single frame mode?
  bool_t Do_frame;           ///< Tell the game to process one frame

  // in-game menu stuff
  MenuProc_t igm;

  // random stuff
  Uint32 randie_index;       ///< The current index of the random number table

  // module parameters
  MOD_INFO      mod;
  ModState_t    modstate;
  ModSummary_t  modtxt;

  // game loop variables
  double dFrame, dUpdate;

  // links to important data
  Uint32                net_status;
  struct sNet         * ns;
  struct sConfigData  * cd;
  struct sClient      * cl;
  struct sServer      * sv;

  // Object/profile stuff
  int        ObjFreeList_count;
  OBJ_REF    ObjFreeList[CHRLST_COUNT];
  ObjList_t  ObjList;
  Uint16     skintoicon[MAXTEXTURE];       ///< convert object skins to texture references
  ChopData_t chop;                         ///< per-object random naming data

  // profiles
  CapList_t CapList;
  EveList_t EveList;
  MadList_t MadList;

  int PipList_count;
  PipList_t PipList;

  // character data
  ChrHeap_t ChrHeap;
  ChrList_t ChrList;

  // enchant data
  EncHeap_t EncHeap;
  EncList_t EncList;

  // particle data
  PrtHeap_t PrtHeap;
  PrtList_t PrtList;

  // passage data
  Uint32    PassList_count;
  Passage_t PassList[PASSLST_COUNT];

  // shop data
  Uint16 ShopList_count;
  Shop_t ShopList[PASSLST_COUNT];

  // team data
  TeamList_t TeamList;

  // local variables
  float     PlaList_level;
  int       PlaList_count;                                 ///< Number of players
  PlaList_t PlaList;

  // environmant info
  WEATHER_INFO  Weather;    ///< particles for weather spawning
  PhysicsData_t phys;       ///< gravity, friction, etc.

  // Mesh and Tile stuff
  Mesh_t        Mesh;
  TILE_ANIMATED Tile_Anim;
  TILE_DAMAGE   Tile_Dam;

  // messages
  MessageData_t MsgList;

  // script values
  ScriptInfo_t ScriptList;

  // per-module graphics info
  Graphics_Data_t GfxData;

  // Game clocks
  Sint32 stt_clock;             ///< SDL_GetTickCount() at start
  Sint32 lst_clock;             ///< The last total of ticks so far
  Sint32 all_clock;             ///< The total number of ticks so far

  Sint32 wld_clock;             ///< The sync clock
  Uint32 wld_frame;             ///< The number of frames that should have been drawn

  Uint8  timeron;               ///< Game timer displayed?
  Uint32 timervalue;            ///< Timer_t time ( 50ths of a second )

  // Misc stuff
  float  pits_clock;             ///< For pit kills
  bool_t pits_kill;              ///< Do they kill?

  bool_t somepladead;           ///< someone is dead
  bool_t allpladead;            ///< everyone is dead?

  char endtext[MAXENDTEXT];     ///< The end-module text

};
typedef struct sGame Game_t;

Game_t * Game_create(struct sNet * net,  struct sClient * cl, struct sServer * sv);
bool_t   Game_destroy(Game_t ** gs );
bool_t   Game_renew(Game_t * gs);

INLINE ScriptInfo_t    * Game_getScriptInfo(Game_t * gs) { if(NULL ==gs) return NULL; return &(gs->ScriptList); }
INLINE ProcState_t     * Game_getProcedure(Game_t * gs)  { if(NULL ==gs) return NULL; return &(gs->proc); }
INLINE MenuProc_t      * Game_getMenuProc(Game_t * gs)   { if(NULL ==gs) return NULL; return &(gs->igm); }
INLINE Mesh_t          * Game_getMesh(Game_t * gs)       { if(NULL ==gs) return NULL; return &(gs->Mesh); }
INLINE Graphics_Data_t * Game_getGfx(Game_t * gs)        { if(NULL ==gs) return NULL; return &(gs->GfxData); }

retval_t Game_registerNetwork( Game_t * gs, struct sNet    * net, bool_t destroy );
retval_t Game_registerClient ( Game_t * gs, struct sClient * cl,  bool_t destroy  );
retval_t Game_registerServer ( Game_t * gs, struct sServer * sv,  bool_t destroy  );
retval_t Game_updateNetStatus( Game_t * gs );

bool_t Game_isLocal  ( Game_t * gs );
bool_t Game_isNetwork( Game_t * gs );
bool_t Game_isClientServer( Game_t * gs );
bool_t Game_isServer ( Game_t * gs );
bool_t Game_isClient ( Game_t * gs );
bool_t Game_hasServer ( Game_t * gs );
bool_t Game_hasClient ( Game_t * gs );

bool_t CapList_new( Game_t * gs );
bool_t CapList_delete( Game_t * gs );
bool_t CapList_renew( Game_t * gs );

bool_t EveList_new( Game_t * gs );
bool_t EveList_delete( Game_t * gs );
bool_t EveList_renew( Game_t * gs );

bool_t MadList_new( Game_t * gs );
bool_t MadList_delete( Game_t * gs );
bool_t MadList_renew( Game_t * gs );

bool_t PipList_new( Game_t * gs );
bool_t PipList_delete( Game_t * gs );
bool_t PipList_renew( Game_t * gs );

bool_t ChrList_new( Game_t * gs );
bool_t ChrList_delete( Game_t * gs );
bool_t ChrList_renew( Game_t * gs );

bool_t EncList_new( Game_t * gs );
bool_t EncList_delete( Game_t * gs );
bool_t EncList_renew( Game_t * gs );

bool_t PrtList_new( Game_t * gs );
bool_t PrtList_delete( Game_t * gs );
bool_t PrtList_renew( Game_t * gs );

bool_t PassList_new( Game_t * gs );
bool_t PassList_delete( Game_t * gs );
bool_t PassList_renew( Game_t * gs );

bool_t ShopList_new( Game_t * gs );
bool_t ShopList_delete( Game_t * gs );
bool_t ShopList_renew( Game_t * gs );

bool_t TeamList_new( Game_t * gs );
bool_t TeamList_delete( Game_t * gs );
bool_t TeamList_renew( Game_t * gs );

bool_t PlaList_new( Game_t * gs );
bool_t PlaList_delete( Game_t * gs );
bool_t PlaList_renew( Game_t * gs );

//--------------------------------------------------------------------------------------------

struct sConfigData
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
  STRING spawn2_file;
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

  int    uifont_points_large;
  int    uifont_points_small;
  STRING uifont_ttf;

  //Global Sounds
  STRING coinget_sound;
  STRING defend_sound;
  STRING coinfall_sound;
  STRING lvlup_sound;

  //------------------------------------
  //Setup Variables
  //------------------------------------
  bool_t zreflect;                   ///< Reflection z buffering?
  int    maxtotalmeshvertices;       ///< of vertices
  bool_t fullscreen;                 ///< Start in fullscreen?
  int    scrd;                       ///< Screen bit depth
  int    scrx;                       ///< Screen X size
  int    scry;                       ///< Screen Y size
  int    scrz;                       ///< Screen z-buffer depth ( 8 unsupported )
  int    maxmessage;                 //
  bool_t messageon;                  ///< Messages?
  int    wraptolerance;              ///< Status_t bar
  bool_t  staton;                    ///< Draw the status bars?
  bool_t  render_overlay;            ///< Draw overlay?
  bool_t  render_background;         ///< Do we render the water as a background?
  GLenum  perspective;               ///< Perspective correct textures?
  bool_t  dither;                    ///< Dithering?
  Uint8   reffadeor;                 ///< 255 = Don't fade reflections
  GLenum  shading;                   ///< Gourad shading?
  bool_t  antialiasing;              ///< Antialiasing?
  bool_t  refon;                     ///< Reflections?
  bool_t  shaon;                     ///< Shadows?
  int     texturefilter;             ///< Texture filtering?
  bool_t  wateron;                   ///< Water overlays?
  bool_t  shasprite;                 ///< Shadow sprites?
  bool_t  phongon;                   ///< Do phong overlay? (Outdated?)
  bool_t  twolayerwateron;           ///< Two layer water?
  bool_t  overlayvalid;              ///< Allow large overlay?
  bool_t  backgroundvalid;           ///< Allow large background?
  bool_t  fogallowed;                ///< Draw fog? (Not implemented!)
  int     particletype;              ///< Particle Effects image
  bool_t  vsync;                     ///< Wait for vertical sync?
  bool_t  gfxacceleration;           ///< Force OpenGL graphics acceleration?

  bool_t  allow_sound;       ///< Allow playing of sound?
  bool_t  allow_music;       ///< Allow music and loops?
  int     musicvolume;       ///< The sound volume of music
  int     soundvolume;       ///< Volume of sounds played
  int     maxsoundchannel;   ///< Max number of sounds playing at the same time
  int     buffersize;        ///< Buffer size set in setup.txt

  Uint8 autoturncamera;           ///< Type of camera control...

  bool_t  request_network;              ///< Try to connect?
  int     lag;                    ///< Lag tolerance
  STRING  net_hosts[8];                            ///< Name for hosting session
  STRING  net_messagename;         ///< Name for messages
  Uint8   fpson;                    ///< FPS displayed?

  // Debug options
  SDL_GrabMode GrabMouse;
  bool_t HideMouse;
  bool_t DevMode;
  // Debug options

};
typedef struct sConfigData ConfigData_t;

char * get_config_string(ConfigData_t * cd, char * szin, char ** szout);
char * get_config_string_name(ConfigData_t * cd, STRING * pconfig_string);

extern ConfigData_t CData;

//--------------------------------------------------------------------------------------------
struct sSearchInfo
{
  bool_t  initialize;
  CHR_REF besttarget;                                      ///< For find_target
  Uint16  bestangle;
  Uint16  useangle;
  int     bestdistance;
  CHR_REF nearest;
  float   distance;
};
typedef struct sSearchInfo SearchInfo_t;

SearchInfo_t * SearchInfo_new(SearchInfo_t * psearch);

//--------------------------------------------------------------------------------------------

void   make_newloadname( char *modname, char *appendname, char *newloadname );
bool_t export_one_character( Game_t * gs, CHR_REF character, Uint16 owner, int number, bool_t export_profile );
void   export_all_local_players( Game_t * gs, bool_t export_profile  );
int    MessageQueue_get_free(MessageQueue_t * mq);
bool_t display_message( Game_t * gs, int message, CHR_REF chr_ref );

void load_action_names( char* loadname );
void read_setup( char* filename );

void make_lightdirectionlookup( void );
void make_enviro( void );
void draw_chr_info( struct sGame * gs );
bool_t do_screenshot( void );
void move_water( WATER_LAYER wlayer[], size_t layer_count, float dUpdate );

CHR_REF search_best_leader( Game_t * gs, TEAM_REF team, CHR_REF exclude );
void call_for_help( Game_t * gs, CHR_REF character );
void give_experience( Game_t * gs, CHR_REF character, int amount, enum e_Experience xptype );
void give_team_experience( Game_t * gs, TEAM_REF team, int amount, enum e_Experience xptype );
void setup_alliances( Game_t * gs, char *modname );
void check_respawn( Game_t * gs );
void update_timers( Game_t * gs );
void reset_teams( Game_t * gs );
void reset_messages( Game_t * gs );
void reset_timers( Game_t * gs );

void set_default_config_data(struct sConfigData * pcon);

int load_all_messages( Game_t * gs, EGO_CONST char *szObjectpath, EGO_CONST char *szObjectname );

void attach_particle_to_character( Game_t * gs, PRT_REF particle, CHR_REF chr_ref, Uint16 vertoffset );

bool_t prt_search_block( Game_t * gs, SearchInfo_t * psearch, int block_x, int block_y, PRT_REF iprt, Uint16 facing,
                         bool_t request_friends, bool_t allow_anyone, TEAM_REF team,
                         CHR_REF donttarget, CHR_REF oldtarget );

bool_t prt_search_wide( Game_t * gs, SearchInfo_t * psearch, PRT_REF iprt, Uint16 facing,
                        Uint16 targetangle, bool_t request_friends, bool_t allow_anyone,
                        TEAM_REF team, CHR_REF donttarget, CHR_REF oldtarget );

bool_t chr_search_distant( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, int maxdist, bool_t ask_enemies, bool_t ask_dead );

bool_t chr_search_block_nearest( Game_t * gs, SearchInfo_t * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                               bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz );

bool_t chr_search_wide_nearest( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, bool_t ask_items,
                                bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz );

bool_t chr_search_wide( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, bool_t ask_items,
                        bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz, bool_t excludeid );

bool_t chr_search_block( Game_t * gs, SearchInfo_t * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                         bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz,
                         bool_t excludeid );

bool_t chr_search_nearby( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, bool_t ask_items,
                          bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ ask_idsz );

void release_all_models(Game_t * gs);
void init_all_models(Game_t * gs);

void setup_characters( Game_t * gs, char *modname );
void ChrList_resynch(Game_t * gs);


struct sGameStack * Get_GameStack( void );

void set_alerts( Game_t * gs, CHR_REF character, float dUpdate );

void   print_status( Game_t * gs, Uint16 statindex );
bool_t add_status( Game_t * gs, CHR_REF character );
bool_t remove_stat( Game_t * gs, struct sChr * pchr );
void   sort_statlist( Game_t * gs );


bool_t reset_characters( Game_t * gs );

bool_t chr_is_player( Game_t * gs, CHR_REF character);


bool_t count_players(Game_t * gs);
void clear_message_queue(MessageQueue_t * q);
void clear_messages( MessageData_t * md);

void load_global_icons( struct sGraphics_Data * gfx );

void recalc_character_bumpers( Game_t * gs );

char * naming_generate( Game_t * gs, Obj_t * pobj );
void naming_read( struct sGame * gs, EGO_CONST char * szModpath, EGO_CONST char * szObjectname, Obj_t * pobj);
void naming_prime( struct sGame * gs );

bool_t decode_escape_sequence( Game_t * gs, char * buffer, size_t buffer_size, EGO_CONST char * message, CHR_REF chr_ref );
void   animate_tiles( TILE_ANIMATED * t, float dUpdate );

void    clear_all_passages(struct sGame * gs );

void   make_prtlist( struct sGame * gs );
bool_t PlaList_set_latch( struct sGame * gs, struct sPlayer * player );
void   set_local_latches( struct sGame * gs );
void make_onwhichfan( struct sGame * gs );
void do_bumping( struct sGame * gs, float dUpdate );

void do_weather_spawn( struct sGame * gs, float dUpdate );
void stat_return( struct sGame * gs, float dUpdate );
void pit_kill( struct sGame * gs, float dUpdate );

void reset_players( struct sGame * gs );
void resize_characters( struct sGame * gs, float dUpdate );

Uint16 terp_dir( Uint16 majordir, float dx, float dy, float dUpdate );
Uint16 terp_dir_fast( Uint16 majordir, float dx, float dy, float dUpdate );

void   append_end_text( struct sGame * gs, int message, CHR_REF character );

void spawn_bump_particles( struct sGame * gs, CHR_REF character, PRT_REF particle );
void setup_particles( struct sGame * gs );
