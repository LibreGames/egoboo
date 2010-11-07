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

#include "egoboo_typedef_cpp.h"
#include "egoboo_math.h"
#include "egoboo_process.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_mpd;
struct ego_camera;
struct ego_script_state;

struct s_mod_file;
struct s_wawalite_animtile;
struct s_wawalite_damagetile;
struct s_wawalite_weather;
struct s_wawalite_water;
struct s_wawalite_fog;

struct ego_menu_process;

struct ego_chr;
struct ego_prt;
struct ego_bundle_prt;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EXPKEEP 0.85f                                ///< Experience to keep when respawning

/// The possible pre-defined orders
enum e_order
{
    ORDER_NONE  = 0,
    ORDER_ATTACK,
    ORDER_ASSIST,
    ORDER_STAND,
    ORDER_TERRAIN,
    ORDER_COUNT
};

//--------------------------------------------------------------------------------------------

/// The bitmasks used by the check_target() function which is used in various character search
/// functions like chr_find_target() or ego_passage::find_object_in()
enum e_targeting_bits
{
    TARGET_DEAD            = ( 1 << 0 ),        ///< Target dead stuff
    TARGET_ENEMIES        = ( 1 << 1 ),        ///< Target enemies
    TARGET_FRIENDS        = ( 1 << 2 ),        ///< Target friends
    TARGET_ITEMS        = ( 1 << 3 ),        ///< Target items
    TARGET_INVERTID        = ( 1 << 4 ),        ///< Target everything but the specified IDSZ
    TARGET_PLAYERS        = ( 1 << 5 ),        ///< Target only players
    TARGET_SKILL        = ( 1 << 6 ),        ///< Target needs the specified skill IDSZ
    TARGET_QUEST        = ( 1 << 7 ),        ///< Target needs the specified quest IDSZ
    TARGET_SELF            = ( 1 << 8 )        ///< Allow self as a target?
};

//--------------------------------------------------------------------------------------------

struct ego_game_process_data
{
    double frameDuration;
    bool_t mod_paused, pause_key_ready;
    bool_t was_active;

    int    menu_depth;
    bool_t escape_requested, escape_latch;

    int    fps_ticks_next, fps_ticks_now;
    int    ups_ticks_next, ups_ticks_now;

    ego_game_process_data() { clear( this ); }

    static ego_game_process_data * ctor_this( ego_game_process_data * ptr ) { return clear( ptr ); }

private:

    static ego_game_process_data * clear( ego_game_process_data * ptr )
    {
        if ( NULL == ptr ) return NULL;

        SDL_memset( ptr, 0, sizeof( *ptr ) );

        ptr->menu_depth = -1;
        ptr->pause_key_ready = btrue;

        return ptr;
    }
};

/// a process that controls a single game
struct ego_game_process : public ego_game_process_data, public ego_process
{

    static ego_game_process * ctor_this( ego_game_process * ptr );

    // "process" management
    static int Run( ego_game_process * gproc, double frameDuration );

protected:
    virtual egoboo_rv do_beginning();
    virtual egoboo_rv do_running();
    virtual egoboo_rv do_leaving();
    virtual egoboo_rv do_finishing();
};

extern ego_game_process * GProc;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//#define TILESOUNDTIME 16                ///< Not used anywhere
#define TILEREAFFIRMAND  3

#define MAXWATERLAYER 2                             ///< Maximum water layers
#define MAXWATERFRAME 512                           ///< Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)
#define WATERPOINTS 4                               ///< Points in a water fan

// Inventory stuff
#define MAXINVENTORY        6
#define MAXIMPORTOBJECTS    (MAXINVENTORY + 2)      ///< left hand + right hand + MAXINVENTORY
#define MAXIMPORTPERPLAYER  (1 + MAXIMPORTOBJECTS)  ///< player + MAXIMPORTOBJECTS

/// The bitmasks for various in-game actions
enum e_latchbutton_bits
{
    LATCHBUTTON_LEFT      = ( 1 << 0 ),                      ///< Character button presses
    LATCHBUTTON_RIGHT     = ( 1 << 1 ),
    LATCHBUTTON_JUMP      = ( 1 << 2 ),
    LATCHBUTTON_ALTLEFT   = ( 1 << 3 ),                      ///< ( Alts are for grab/drop )
    LATCHBUTTON_ALTRIGHT  = ( 1 << 4 ),
    LATCHBUTTON_PACKLEFT  = ( 1 << 5 ),                     ///< ( Packs are for inventory cycle )
    LATCHBUTTON_PACKRIGHT = ( 1 << 6 ),
    LATCHBUTTON_RESPAWN   = ( 1 << 7 )
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The actual state of the animated tiles in-game
struct ego_animtile_instance
{
    int    update_and;             ///< New tile every 7 frames
    Uint16 frame_and;
    Uint16 base_and;
    Uint16 frame_add;
};

extern Uint32                animtile_update_and;
extern ego_animtile_instance animtile[2];

//--------------------------------------------------------------------------------------------

/// The actual in-game state of the damage tiles
struct ego_damagetile_instance
{
    IPair   amount;                    ///< Amount of damage
    int    type;

    int    parttype;
    Uint32 partand;
    int    sound_index;

    //Sint16  sound_time;           // this is not used anywhere in the game
    //Uint16  min_distance;           // this is not used anywhere in the game
};

extern ego_damagetile_instance damagetile;

//--------------------------------------------------------------------------------------------

/// The data describing the weather state
struct ego_weather_instance
{
    int     timer_reset;        ///< How long between each spawn?
    bool_t  over_water;         ///< Only spawn over water?
    Uint8   particle;           ///< Which particle to spawn?

    PLA_REF iplayer;
    int     time;                ///< 0 is no weather
};

extern ego_weather_instance weather;

//--------------------------------------------------------------------------------------------

/// The data describing the state of a water layer
struct ego_water_layer_instance
{
    Uint16    frame;        ///< Frame
    Uint32    frame_add;      ///< Speed

    float     z;            ///< Base height of water
    float     amp;            ///< Amplitude of waves

    fvec2_t   dist;         ///< some indication of "how far away" the layer is if it is an overlay

    fvec2_t   tx;           ///< Coordinates of texture

    float     light_dir;    ///< direct  reflectivity 0 - 1
    float     light_add;    ///< ambient reflectivity 0 - 1

    Uint8     alpha;        ///< layer transparency

    fvec2_t   tx_add;            ///< Texture movement
};

/// The data describing the water state
struct ego_water_instance
{
    float  surface_level;          ///< Surface level for water striders
    float  douse_level;            ///< Surface level for torches
    bool_t is_water;         ///< Is it water?  ( Or lava... )
    bool_t overlay_req;
    bool_t background_req;
    bool_t light;            ///< Is it light ( default is alpha )

    float  foregroundrepeat;
    float  backgroundrepeat;

    Uint32 spek[256];              ///< Specular highlights

    int                      layer_count;
    ego_water_layer_instance layer[MAXWATERLAYER];

    float  layer_z_add[MAXWATERLAYER][MAXWATERFRAME][WATERPOINTS];
};

extern ego_water_instance water;

//--------------------------------------------------------------------------------------------

/// The in-game fog state
/// \warn Fog is currently not used
struct ego_fog_instance
{
    bool_t  on;            ///< Do ground fog?
    float   top, bottom;
    Uint8   red, grn, blu;
    float   distance;
};

extern ego_fog_instance fog;

//--------------------------------------------------------------------------------------------

/// the module data that the game needs
struct ego_game_module_data
{
    Uint8   importamount;               ///< Number of imports for this module
    bool_t  exportvalid;                ///< Can it export?
    bool_t  exportreset;                ///< Allow to export when module is reset?
    Uint8   playeramount;               ///< How many players?
    bool_t  importvalid;                ///< Can it import?
    bool_t  respawnvalid;               ///< Can players respawn with Spacebar?
    bool_t  respawnanytime;             ///< True if it's a small level...
    STRING  loadname;                     ///< Module load names

    bool_t  active;                     ///< Is the control loop still going?
    bool_t  beat;                       ///< Show Module Ended text?
    Uint32  seed;                       ///< The module seed
    Uint32  randsave;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PIT_DELAY 20

/// Pitty stuff
struct ego_pit_info
{
    bool_t     kill;          ///< Do they kill?
    bool_t     teleport;      ///< Do they teleport?
    fvec3_t    teleport_pos;
};

extern ego_pit_info pits;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Shared stats
struct ego_local_stats
{
    int     seeinvis_level;
    int     seedark_level;
    int     grog_level;
    int     daze_level;
    int     seekurse_level;
    int     listen_level;                  ///< Players with listen skill?

    // ESP ability
    TEAM_REF sense_enemy_team;
    IDSZ     sense_enemy_ID;

    ego_local_stats() { init(); };

    void init();
};

extern ego_local_stats local_stats;

//--------------------------------------------------------------------------------------------

#define IMPORT_MAX 16

// Imports
struct ego_import_data
{
    int           count;                 ///< Number of imports from this machine
    BIT_FIELD     control[IMPORT_MAX];  ///< Input bits for each imported player
    int           slot[IMPORT_MAX];     ///< For local imports

    ego_import_data() { init(); }

    void init()
    {
        for ( int i = 0; i < IMPORT_MAX; i++ )
        {
            control[i] = 0;
            slot[i]    = -1;
        }
        count = 0;
    }
};

extern ego_import_data local_import;

//--------------------------------------------------------------------------------------------
// Status displays

#define MAXSTAT             10                      ///< Maximum status displays

struct stat_lst
{
    bool_t  on;
    int     count;

    stat_lst() { init(); }

    void init()
    {
        on = btrue;
        count = 0;
        for ( int i = 0; i < MAXSTAT; i++ )
        {
            lst[i] = CHR_REF( MAX_CHR );
        }
    }

    CHR_REF & operator []( const size_t val ) { return lst[val]; }

    bool_t remove( const CHR_REF & ichr );
    bool_t add( const CHR_REF & ichr );
    bool_t move_to_top( const CHR_REF & ichr );

private:

    CHR_REF lst[MAXSTAT];
};

extern stat_lst StatList;

//--------------------------------------------------------------------------------------------
// End text
#define MAXENDTEXT 1024

extern char   endtext[MAXENDTEXT];     ///< The end-module text
extern size_t endtext_carat;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern bool_t    overrideslots;          ///< Override existing slots?

extern struct ego_mpd         * PMesh;
extern struct ego_camera          * PCamera;
extern struct ego_game_module_data * PMod;

extern FACING_T  glo_useangle;                                        ///< actually still used

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// the hook for deinitializing an old module
void   game_quit_module();

// the hook for exporting all the current players and reloading them
egoboo_rv game_update_imports();
void   game_finish_module();
bool_t game_begin_module( const char * modname, const Uint32 seed );

// Exporting stuff
void export_one_character( const CHR_REF & character, const CHR_REF & owner, const int number, const bool_t is_local );
void export_all_players( const bool_t require_local );

// Messages
void show_stat( const int statindex );
void show_armor( const int statindex );
void show_full_status( const int statindex );
void show_magic_status( const int statindex );

// End Text
void reset_end_text();

// Particles
int     number_of_attached_particles( const CHR_REF & character );
int     spawn_bump_particles( const CHR_REF & character, const PRT_REF & particle );
void    disaffirm_attached_particles( const CHR_REF & character );
int     reaffirm_attached_particles( const CHR_REF & character );
ego_prt * place_particle_at_vertex( ego_prt * pprt, const CHR_REF & character, const int vertex_offset );

/// Player
void   read_player_local_latch( const PLA_REF & player );
bool_t add_player( const CHR_REF & character, const BIT_FIELD device );

// AI targeting
CHR_REF chr_find_target( ego_chr * psrc, const float max_dist, const IDSZ idsz, const BIT_FIELD targeting_bits );
CHR_REF prt_find_target( const float pos_x, const float pos_y, const float pos_z, const FACING_T facing,
                         const PIP_REF & particletype, const TEAM_REF & team, const CHR_REF & donttarget, const CHR_REF & oldtarget );

// object initialization
void  free_all_objects( void );

// Data
struct ego_mpd * set_PMesh( ego_mpd * pmpd );
struct ego_camera  * set_PCamera( ego_camera * pcam );

bool_t upload_animtile_data( ego_animtile_instance pinst[], s_wawalite_animtile * pdata, const size_t animtile_count );
bool_t upload_damagetile_data( ego_damagetile_instance * pinst, s_wawalite_damagetile * pdata );
bool_t upload_weather_data( ego_weather_instance * pinst, s_wawalite_weather * pdata );
bool_t upload_water_data( ego_water_instance * pinst, s_wawalite_water * pdata );
bool_t upload_fog_data( ego_fog_instance * pinst, s_wawalite_fog * pdata );

float get_mesh_level( ego_mpd * pmesh, const float x, const float y, const bool_t waterwalk );

bool_t make_water( ego_water_instance * pinst, const s_wawalite_water * pdata );

bool_t game_choose_module( const int imod, const int seed );

int game_do_menu( ego_menu_process * mproc );

void expand_escape_codes( const CHR_REF & ichr, ego_script_state * pstate, char * src, char * src_end, char * dst, char * dst_end );

void upload_wawalite();

bool_t game_module_setup( ego_game_module_data * pinst, s_mod_file * pdata, const char * loadname, const Uint32 seed );
bool_t game_module_init( ego_game_module_data * pinst );
bool_t game_module_reset( ego_game_module_data * pinst, const Uint32 seed );
bool_t game_module_start( ego_game_module_data * pinst );
bool_t game_module_stop( ego_game_module_data * pinst );

bool_t check_target( ego_chr * psrc, const CHR_REF & ichr_test, IDSZ idsz, BIT_FIELD targeting_bits );

void attach_all_particles();

struct s_wawalite_data * read_wawalite();
bool_t write_wawalite( const char *modname, s_wawalite_data * pdata );

Uint8 get_local_alpha( const int alpha );
Uint8 get_local_light( const int light );

bool_t do_shop_drop( const CHR_REF & idropper, const CHR_REF & iitem );

bool_t do_shop_buy( const CHR_REF & ipicker, const CHR_REF & ichr );
bool_t do_shop_steal( const CHR_REF & ithief, const CHR_REF & iitem );
bool_t do_item_pickup( const CHR_REF & ichr, const CHR_REF & iitem );

bool_t get_chr_regeneration( ego_chr * pchr, int *pliferegen, int * pmanaregen );

float get_chr_level( ego_mpd * pmesh, ego_chr * pchr );

egoboo_rv move_water( ego_water_instance * pwater );

// manage the game's vfs mount points
bool_t game_setup_vfs_paths( const char * mod_path );

void cleanup_character_enchants( ego_chr * pchr );

bool_t attach_one_particle( const ego_bundle_prt & bdl_prt );
