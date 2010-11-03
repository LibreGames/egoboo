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

/// @file game.c
/// @brief The code for controlling the game
/// @details

#include "game.h"

#include "object_BSP.h"
#include "mesh_BSP.h"

#include "mad.h"

#include "file_formats/controls_file.h"
#include "file_formats/scancode_file.h"

#include "physics.inl"
#include "clock.h"
#include "link.h"
#include "ui.h"
#include "font_bmp.h"
#include "font_ttf.h"
#include "log.h"
#include "system.h"
#include "script.h"
#include "sound.h"
#include "graphic.h"
#include "passage.h"
#include "input.h"
#include "menu.h"
#include "network.h"
#include "mesh.inl"
#include "texture.h"
#include "file_formats/wawalite_file.h"
#include "clock.h"
#include "file_formats/spawn_file.h"
#include "camera.h"
#include "file_formats/id_md2.h"
#include "collision.h"
#include "graphic_fan.h"
#include "quest.h"

#include "script_compile.h"
#include "script.h"

#include "egoboo_vfs.h"
#include "egoboo_typedef.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo.h"

#include "extensions/SDL_extensions.h"

#include "egoboo_console.h"
#if defined(USE_LUA_CONSOLE)
#include "lua_console.h"
#endif

#include "char.inl"
#include "particle.inl"
#include "enchant.inl"
#include "profile.inl"

#include <SDL_image.h>

#include <time.h>
#include <assert.h>
#include <float.h>
#include <string.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Data needed to specify a line-of-sight test
struct ego_line_of_sight_info
{
    float x0, y0, z0;
    float x1, y1, z1;
    Uint32 stopped_by;

    CHR_REF collide_chr;
    Uint32  collide_fx;
    int     collide_x;
    int     collide_y;
};

static bool_t collide_ray_with_mesh( ego_line_of_sight_info * plos );
static bool_t collide_ray_with_characters( ego_line_of_sight_info * plos );
static bool_t do_line_of_sight( ego_line_of_sight_info * plos );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static ego_mpd           _mesh[2];
static ego_camera          _camera[2];

static ego_game_process    _gproc;
static ego_game_module_data      gmod;

PROFILE_DECLARE( game_update_loop );
PROFILE_DECLARE( gfx_loop );
PROFILE_DECLARE( game_single_update );

PROFILE_DECLARE( talk_to_remotes );
PROFILE_DECLARE( net_dispatch_packets );
PROFILE_DECLARE( check_stats );
PROFILE_DECLARE( read_local_latches );
PROFILE_DECLARE( PassageStack_check_music );
PROFILE_DECLARE( cl_talkToHost );

PROFILE_DECLARE( misc_update );
PROFILE_DECLARE( object_io );
PROFILE_DECLARE( initialize_all_objects );
PROFILE_DECLARE( bump_all_objects );
PROFILE_DECLARE( move_all_objects );
PROFILE_DECLARE( finalize_all_objects );
PROFILE_DECLARE( game_update_clocks );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t  overrideslots      = bfalse;

// End text
char   endtext[MAXENDTEXT] = EMPTY_CSTR;
size_t endtext_carat = 0;

ego_mpd           * PMesh   = _mesh + 0;
ego_camera          * PCamera = _camera + 0;
ego_game_module_data     * PMod    = &gmod;
ego_game_process    * GProc   = &_gproc;

ego_pit_info pits = { bfalse, bfalse, ZERO_VECT3 };

FACING_T glo_useangle = 0;                                        // actually still used

Uint32                  animtile_update_and = 0;
ego_animtile_instance   animtile[2];
ego_damagetile_instance damagetile;
ego_weather_instance    weather;
ego_water_instance      water;
ego_fog_instance        fog;

ego_local_stats local_stats;
ego_import_data local_import;

stat_lst StatList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// game initialization / deinitialization - not accessible by scripts
static void reset_timers();

// looping - stuff called every loop - not accessible by scripts
static int  game_update();
static void game_update_heartbeat();
static void game_update_local_respawn();
static void game_update_pits();
static void game_update_controls();
static void game_update_network();
static void game_update_clocks();
static void game_update_network_stats();
static void game_update_local_stats();

static void check_stats();
static void tilt_characters_to_terrain();
static void do_damage_tiles( void );
static void read_local_latches( void );
static void let_all_characters_think();

// module initialization / deinitialization - not accessible by scripts
static bool_t game_load_module_data( const char *smallname );
static void   game_release_module_data();
static void   game_load_all_profiles( const char *modname );

static void   activate_spawn_file_vfs();
static void   activate_alliance_file_vfs();
static void   load_all_global_objects();

static bool_t chr_setup_apply( const CHR_REF & ichr, spawn_file_info_t *pinfo );

static void   game_reset_players();

// Model stuff
static void log_madused_vfs( const char *savename );

// misc
static bool_t game_begin_menu( ego_menu_process * mproc, which_menu_t which );
static void   game_end_menu( ego_menu_process * mproc );

static void   do_game_hud();

// manage the game's vfs mount points
static void   game_clear_vfs_paths();

static void do_weather_spawn_particles();

static void sort_stat_list();

//--------------------------------------------------------------------------------------------
// Random Things
//--------------------------------------------------------------------------------------------
void export_one_character( const CHR_REF & character, const CHR_REF & owner, int number, bool_t is_local )
{
    /// @details ZZ@> This function exports a character

    int tnc = 0;
    ego_cap * pcap;
    ego_pro * pobj;

    STRING fromdir;
    STRING todir;
    STRING fromfile;
    STRING tofile;
    STRING todirname;
    STRING todirfullname;

    if ( !PMod->exportvalid ) return;

    // Don't export enchants
    remove_all_character_enchants( character );

    pobj = ego_chr::get_ppro( character );
    if ( NULL == pobj ) return;

    pcap = ego_chr::get_pcap( character );
    if ( NULL == pcap ) return;

    // do not export items that can't be exported
    if ( pcap->isitem && !pcap->cancarrytonextmodule ) return;

    // TWINK_BO.OBJ
    SDL_snprintf( todirname, SDL_arraysize( todirname ), "%s", str_encode_path( ChrObjList.get_data_ref( owner ).name ) );

    // Is it a character or an item?
    if ( owner != character )
    {
        // Item is a subdirectory of the owner directory...
        SDL_snprintf( todirfullname, SDL_arraysize( todirfullname ), "%s/%d.obj", todirname, number );
    }
    else
    {
        // Character directory
        SDL_snprintf( todirfullname, SDL_arraysize( todirfullname ), "%s", todirname );
    }

    // players/twink.obj or players/twink.obj/0.obj
    if ( is_local )
    {
        SDL_snprintf( todir, SDL_arraysize( todir ), "/players/%s", todirfullname );
    }
    else
    {
        SDL_snprintf( todir, SDL_arraysize( todir ), "/remote/%s", todirfullname );
    }

    // modules/advent.mod/objects/advent.obj
    SDL_snprintf( fromdir, SDL_arraysize( fromdir ), "%s", pobj->name );

    // Delete all the old items
    if ( owner == character )
    {
        for ( tnc = 0; tnc < MAXIMPORTOBJECTS; tnc++ )
        {
            SDL_snprintf( tofile, SDL_arraysize( tofile ), "%s/%d.obj", todir, tnc ); /*.OBJ*/
            vfs_removeDirectoryAndContents( tofile, btrue );
        }
    }

    // Make the directory
    vfs_mkdir( todir );

    // Build the DATA.TXT file
    SDL_snprintf( tofile, SDL_arraysize( tofile ), "%s/data.txt", todir ); /*DATA.TXT*/
    export_one_character_profile_vfs( tofile, character );

    // Build the SKIN.TXT file
    SDL_snprintf( tofile, SDL_arraysize( tofile ), "%s/skin.txt", todir ); /*SKIN.TXT*/
    export_one_character_skin_vfs( tofile, character );

    // Build the NAMING.TXT file
    SDL_snprintf( tofile, SDL_arraysize( tofile ), "%s/naming.txt", todir ); /*NAMING.TXT*/
    export_one_character_name_vfs( tofile, character );

    // Copy all of the misc. data files
    SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/message.txt", fromdir ); /*MESSAGE.TXT*/
    SDL_snprintf( tofile, SDL_arraysize( tofile ), "%s/message.txt", todir ); /*MESSAGE.TXT*/
    vfs_copyFile( fromfile, tofile );

    SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/tris.md2", fromdir ); /*TRIS.MD2*/
    SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/tris.md2", todir ); /*TRIS.MD2*/
    vfs_copyFile( fromfile, tofile );

    SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/copy.txt", fromdir ); /*COPY.TXT*/
    SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/copy.txt", todir ); /*COPY.TXT*/
    vfs_copyFile( fromfile, tofile );

    SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/script.txt", fromdir );
    SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/script.txt", todir );
    vfs_copyFile( fromfile, tofile );

    SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/enchant.txt", fromdir );
    SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/enchant.txt", todir );
    vfs_copyFile( fromfile, tofile );

    SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/credits.txt", fromdir );
    SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/credits.txt", todir );
    vfs_copyFile( fromfile, tofile );

    // Build the QUEST.TXT file
    SDL_snprintf( tofile, SDL_arraysize( tofile ), "%s/quest.txt", todir );
    export_one_character_quest_vfs( tofile, character );

    // Copy all of the particle files
    for ( tnc = 0; tnc < MAX_PIP_PER_PROFILE; tnc++ )
    {
        SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/part%d.txt", fromdir, tnc );
        SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/part%d.txt", todir,   tnc );
        vfs_copyFile( fromfile, tofile );
    }

    // Copy all of the sound files
    for ( tnc = 0; tnc < MAX_WAVE; tnc++ )
    {
        SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/sound%d.wav", fromdir, tnc );
        SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/sound%d.wav", todir,   tnc );
        vfs_copyFile( fromfile, tofile );

        SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/sound%d.ogg", fromdir, tnc );
        SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/sound%d.ogg", todir,   tnc );
        vfs_copyFile( fromfile, tofile );
    }

    // Copy all of the image files (try to copy all supported formats too)
    for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
    {
        Uint8 type;

        for ( type = 0; type < maxformattypes; type++ )
        {
            SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/tris%d%s", fromdir, tnc, TxFormatSupported[type] );
            SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/tris%d%s", todir,   tnc, TxFormatSupported[type] );
            vfs_copyFile( fromfile, tofile );

            SDL_snprintf( fromfile, SDL_arraysize( fromfile ), "%s/icon%d%s", fromdir, tnc, TxFormatSupported[type] );
            SDL_snprintf( tofile, SDL_arraysize( tofile ),   "%s/icon%d%s", todir,   tnc, TxFormatSupported[type] );
            vfs_copyFile( fromfile, tofile );
        }
    }
}

//--------------------------------------------------------------------------------------------
void export_all_players( bool_t require_local )
{
    /// @details ZZ@> This function saves all the local players in the
    ///    PLAYERS directory

    bool_t is_local;
    PLA_REF ipla;
    int number;
    CHR_REF character, item;

    // Don't export if the module isn't running
    if (( rv_success != GProc->running() ) ) return;

    // Stop if export isn't valid
    if ( !PMod->exportvalid ) return;

    log_info( "export_all_players() - Exporting all player characters.\n" );

    // Check each player
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player * ppla = PlaStack + ipla;
        if ( !ppla->valid ) continue;

        is_local = ( 0 != ppla->device.bits );
        if ( require_local && !is_local ) continue;

        // Is it alive?
        character = ppla->index;
        ego_obj_chr * pchr = ChrObjList.get_allocated_data_ptr( character );
        if ( !INGAME_PCHR( pchr ) || !pchr->alive ) continue;

        // Export the character
        export_one_character( character, character, 0, is_local );

        // Export the left hand item
        item = pchr->holdingwhich[SLOT_LEFT];
        if ( INGAME_CHR( item ) && ChrObjList.get_data_ref( item ).isitem )
        {
            export_one_character( item, character, SLOT_LEFT, is_local );
        }

        // Export the right hand item
        item = pchr->holdingwhich[SLOT_RIGHT];
        if ( INGAME_CHR( item ) && ChrObjList.get_data_ref( item ).isitem )
        {
            export_one_character( item, character, SLOT_RIGHT, is_local );
        }

        // Export the inventory
        number = 0;
        PACK_BEGIN_LOOP( item, pchr->pack.next )
        {
            if ( number >= MAXINVENTORY ) break;

            if ( ChrObjList.get_data_ref( item ).isitem )
            {
                export_one_character( item, character, number + 2, is_local );
                number++;
            }
        }
        PACK_END_LOOP( item );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void log_madused_vfs( const char *savename )
{
    /// @details ZZ@> This is a debug function for checking model loads

    vfs_FILE* hFileWrite;
    PRO_REF cnt;

    hFileWrite = vfs_openWrite( savename );
    if ( hFileWrite )
    {
        vfs_printf( hFileWrite, "Slot usage for objects in last module loaded...\n" );
        //vfs_printf( hFileWrite, "%d of %d frames used...\n", Md2FrameList_index, MAXFRAME );

        for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
        {
            if ( LOADED_PRO( cnt ) )
            {
                CAP_REF icap = pro_get_icap( cnt );
                MAD_REF imad = pro_get_imad( cnt );

                vfs_printf( hFileWrite, "%3d %32s %s\n", cnt.get_value(), CapStack[icap].classname, MadStack[imad].name );
            }
            else if ( cnt <= 36 )    vfs_printf( hFileWrite, "%3d  %32s.\n", cnt.get_value(), "Slot reserved for import players" );
            else                    vfs_printf( hFileWrite, "%3d  %32s.\n", cnt.get_value(), "Slot Unused" );
        }

        vfs_close( hFileWrite );
    }
}

//--------------------------------------------------------------------------------------------
void activate_alliance_file_vfs( /*const char *modname*/ )
{
    /// @details ZZ@> This function reads the alliance file
    STRING szTemp;
    TEAM_REF teama, teamb;
    vfs_FILE *fileread;

    // Load the file
    fileread = vfs_openRead( "mp_data/alliance.txt" );
    if ( fileread )
    {
        while ( goto_colon( NULL, fileread, btrue ) )
        {
            fget_string( fileread, szTemp, SDL_arraysize( szTemp ) );
            teama = ( szTemp[0] - 'A' ) % TEAM_MAX;

            fget_string( fileread, szTemp, SDL_arraysize( szTemp ) );
            teamb = ( szTemp[0] - 'A' ) % TEAM_MAX;
            TeamStack[teama].hatesteam[teamb.get_value()] = bfalse;
        }

        vfs_close( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void update_used_lists()
{
    ChrObjList.update_used();
    PrtObjList.update_used();
    EncObjList.update_used();
}

//--------------------------------------------------------------------------------------------
void update_all_objects()
{
    ego_chr::stoppedby_tests = ego_prt::stoppedby_tests = 0;
    ego_chr::pressure_tests  = ego_prt::pressure_tests  = 0;

    update_all_characters();
    update_all_particles();
    update_all_enchants();
}

//--------------------------------------------------------------------------------------------
void move_all_objects()
{
    ego_mpd::mpdfx_tests = 0;

    move_all_particles();
    move_all_characters();
}

//--------------------------------------------------------------------------------------------
void cleanup_all_objects()
{
    cleanup_all_characters();
    cleanup_all_particles();
    cleanup_all_enchants();
}

//--------------------------------------------------------------------------------------------
void increment_all_object_update_counters()
{
    increment_all_character_update_counters();
    increment_all_particle_update_counters();
    increment_all_enchant_update_counters();
}

//--------------------------------------------------------------------------------------------
void object_physics_initialize()
{
    character_physics_initialize_all();
    particle_physics_initialize_all();
}

//--------------------------------------------------------------------------------------------
void initialize_all_objects()
{
    /// @details BB@> begin the code for updating in-game objects

    // update all object timers etc.
    update_all_objects();

    // fix the list optimization, in case update_all_objects() turned some objects off.
    //update_used_lists();

    object_physics_initialize();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void object_physics_finalize( float dt )
{
    character_physics_finalize_all( dt );
    particle_physics_finalize_all( dt );
}

//--------------------------------------------------------------------------------------------
void finalize_all_objects()
{
    /// @details BB@> end the code for updating in-game objects

    // actually move all of the objects
    object_physics_finalize( 1.0f );

    // update the object's update counter for every active object
    increment_all_object_update_counters();

    // do end-of-life care for all objects
    cleanup_all_objects();
}

//--------------------------------------------------------------------------------------------
void game_update_network_stats()
{
    PLA_REF ipla;

    net_stats.pla_count_local   = 0;
    net_stats.pla_count_network = 0;
    net_stats.pla_count_total   = 0;

    net_stats.pla_count_local_alive   = 0;
    net_stats.pla_count_network_alive = 0;
    net_stats.pla_count_total_alive   = 0;

    net_stats.pla_count_local_dead   = 0;
    net_stats.pla_count_network_dead = 0;
    net_stats.pla_count_total_dead   = 0;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        ego_chr * pchr;
        ego_player * ppla;
        bool_t net_player = bfalse;

        // ignore ivalid players
        ppla = PlaStack + ipla;
        if ( !ppla->valid ) continue;

        // fix bad players
        if ( !VALID_CHR( ppla->index ) )
        {
            ego_player::dtor( ppla );
            continue;
        }

        // don't worry about players that aren't in the game
        if ( !INGAME_CHR( ppla->index ) )
        {
            continue;
        }

        // alias some variables
        ichr = ppla->index;
        pchr = ChrObjList.get_data_ptr( ichr );

        // no input bits means that the player is remote.
        net_player = ( INPUT_BITS_NONE == ppla->device.bits );

        // who is alive
        if ( net_player )
        {
            net_stats.pla_count_network++;
        }
        else
        {
            net_stats.pla_count_local++;
        }

        // count the total number of players
        net_stats.pla_count_total = net_stats.pla_count_local + net_stats.pla_count_network;

        if ( net_player )
        {
            if ( pchr->alive )
            {
                net_stats.pla_count_network_alive++;
            }
            else
            {
                net_stats.pla_count_network_dead++;
            }
        }
        else
        {
            if ( pchr->alive )
            {
                net_stats.pla_count_local_alive++;
            }
            else
            {
                net_stats.pla_count_local_dead++;
            }
        }

        // get the totals
        net_stats.pla_count_total_alive = net_stats.pla_count_local_alive + net_stats.pla_count_network_alive;
        net_stats.pla_count_total_dead  = net_stats.pla_count_local_dead + net_stats.pla_count_network_dead;

        // let the status of local players modify local game experience (sound, camera, ...)
        if ( pchr->alive && !net_player )
        {
            if ( pchr->see_invisible_level > 0 )
            {
                local_stats.seeinvis_level += pchr->see_invisible_level;
            }

            if ( pchr->see_kurse_level > 0 )
            {
                local_stats.seekurse_level += pchr->see_kurse_level;
            }

            if ( pchr->darkvision_level > 0 )
            {
                local_stats.seedark_level += pchr->darkvision_level;
            }

            if ( pchr->grogtime > 0 )
            {
                local_stats.grog_level += pchr->grogtime;
            }

            if ( pchr->dazetime > 0 )
            {
                local_stats.daze_level += pchr->dazetime;
            }

            local_stats.listen_level += ego_chr_data::get_skill( pchr, MAKE_IDSZ( 'L', 'I', 'S', 'T' ) );
        }
    }

    // blunt the local_stats (this assumes they all share the same camera view)
    if ( 0 != net_stats.pla_count_local_alive )
    {
        local_stats.seeinvis_level /= net_stats.pla_count_local_alive;
        local_stats.seekurse_level /= net_stats.pla_count_local_alive;
        local_stats.seedark_level  /= net_stats.pla_count_local_alive;
        local_stats.grog_level     /= net_stats.pla_count_local_alive;
        local_stats.daze_level     /= net_stats.pla_count_local_alive;
    }
}

//--------------------------------------------------------------------------------------------
void game_update_local_stats()
{
    PLA_REF ipla;

    if ( 0 == net_stats.pla_count_local_alive ) return;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        ego_chr * pchr;
        ego_player * ppla;
        bool_t net_player = bfalse;

        // ignore ivalid players
        ppla = PlaStack + ipla;
        if ( !ppla->valid ) continue;

        // fix bad players
        if ( !VALID_CHR( ppla->index ) )
        {
            ego_player::dtor( ppla );
            continue;
        }

        // don't worry about players that aren't in the game
        if ( !INGAME_CHR( ppla->index ) )
        {
            continue;
        }

        // alias some variables
        ichr = ppla->index;
        pchr = ChrObjList.get_data_ptr( ichr );

        // no input bits means that the player is remote.
        net_player = ( INPUT_BITS_NONE == ppla->device.bits );
        if ( net_player ) continue;

        // let the status of local players modify local game experience (sound, camera, ...)
        if ( pchr->alive )
        {
            if ( pchr->see_invisible_level > 0 )
            {
                local_stats.seeinvis_level += pchr->see_invisible_level;
            }

            if ( pchr->see_kurse_level > 0 )
            {
                local_stats.seekurse_level += pchr->see_kurse_level;
            }

            if ( pchr->darkvision_level > 0 )
            {
                local_stats.seedark_level += pchr->darkvision_level;
            }

            if ( pchr->grogtime > 0 )
            {
                local_stats.grog_level += pchr->grogtime;
            }

            if ( pchr->dazetime > 0 )
            {
                local_stats.daze_level += pchr->dazetime;
            }

            local_stats.listen_level += ego_chr_data::get_skill( pchr, MAKE_IDSZ( 'L', 'I', 'S', 'T' ) );
        }
    }

    // blunt the local_stats (this assumes they all share the same camera view)
    if ( 0 != net_stats.pla_count_local_alive )
    {
        local_stats.seeinvis_level /= net_stats.pla_count_local_alive;
        local_stats.seekurse_level /= net_stats.pla_count_local_alive;
        local_stats.seedark_level  /= net_stats.pla_count_local_alive;
        local_stats.grog_level     /= net_stats.pla_count_local_alive;
        local_stats.daze_level     /= net_stats.pla_count_local_alive;
    }
}

//--------------------------------------------------------------------------------------------
void game_update_local_respawn()
{
    PLA_REF ipla;

    // check for autorespawn
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        ego_chr * pchr;

        ego_player * ppla = PlaStack + ipla;
        if ( !ppla->valid ) continue;

        if ( !INGAME_CHR( ppla->index ) ) continue;

        ichr = ppla->index;
        pchr = ChrObjList.get_data_ptr( ichr );

        if ( !pchr->alive )
        {
            if ( cfg.difficulty < GAME_HARD && ( 0 == net_stats.pla_count_total_alive ) && SDLKEYDOWN( SDLK_SPACE ) && PMod->respawnvalid && 0 == timer_revive )
            {
                ego_chr::respawn( ichr );
                pchr->experience *= EXPKEEP;        // Apply xp Penalty

                if ( cfg.difficulty > GAME_EASY )
                {
                    pchr->money *= EXPKEEP;        // Apply money loss
                }
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
void game_update_heartbeat()
{
    // This stuff doesn't happen so often

    // down the timer
    if ( timer_heartbeat > 0 ) timer_heartbeat--;

    // throttle the execution of this function
    if ( 0 != timer_heartbeat ) return;

    // automatically reset the timer
    timer_heartbeat = ONESECOND;

    // update the respawning of local players
    game_update_local_respawn();
}

//--------------------------------------------------------------------------------------------
int game_update()
{
    /// @details ZZ@> This function does several iterations of character movements and such
    ///    to keep the game in sync.

    int tnc;
    int update_loop_cnt;
    PLA_REF ipla;

    update_lag = 0;
    update_loop_cnt = 0;

    // measure the actual lag
    update_lag = signed( true_update ) - signed( update_wld );

    // don't do anything if we are not behind
    if ( update_wld > true_update ) return 0;

    // throttle the number of iterations through the loop
    int max_iterations = single_update_mode ? 1 : 2 * TARGET_UPS;
    max_iterations = SDL_min( max_iterations, estimated_updates + 2 );

    if ( update_lag > max_iterations )
    {
        log_warning( "Local machine not keeping up with the required updates-per-second. %d/%d\n", max_iterations, update_lag );
    }

    /// @todo claforte@> Put that back in place once networking is functional (Jan 6th 2001)
    /// do stuff inside this loop if it might cause problems on a laggy machine
    for ( tnc = 0; update_wld < true_update && tnc < max_iterations; tnc++ )
    {
        PROFILE_BEGIN( game_single_update );
        {
            //---- begin the code for updating misc. game stuff
            PROFILE_BEGIN( misc_update );
            {
                srand( PMod->randsave );
                PMod->randsave = rand();

                // read the inputs in here, in case the game is laggy
                // should do this before game_update_network() so that
                // any latches that are broadcast will be in synch with this machine
                game_update_controls();

                // do all the net processes here, too, for the same reason
                game_update_network();

                // count the players of various types
                game_update_network_stats();

                // apply sound/video effects to the local display
                game_update_local_stats();

                // stuff that happens only every ONESECOND
                game_update_heartbeat();

                // other misc. stuff
                BillboardList_update_all();
                animate_tiles();
                move_water( &water );
                looped_update_all_sound();
                do_damage_tiles();
                game_update_pits();
                do_weather_spawn_particles();
            }
            PROFILE_END2( misc_update );
            //---- end the code for updating misc. game stuff

            //---- begin the code object I/O
            PROFILE_BEGIN( object_io );
            {
                let_all_characters_think();           // sets the non-player latches
                unbuffer_all_player_latches();        // sets the player latches
            }
            PROFILE_END2( object_io );
            //---- end the code object I/O

            //---- begin the code for updating in-game objects
            PROFILE_BEGIN( initialize_all_objects );
            initialize_all_objects();
            PROFILE_END2( initialize_all_objects );
            {
                PROFILE_BEGIN( bump_all_objects );
                bump_all_objects( &ego_obj_BSP::root );    // do the interactions
                PROFILE_END2( bump_all_objects );

                PROFILE_BEGIN( move_all_objects );
                move_all_objects();                   // this function may clear some latches
                PROFILE_END2( move_all_objects );
            }
            PROFILE_BEGIN( finalize_all_objects );
            finalize_all_objects();
            PROFILE_END2( finalize_all_objects );
            //---- end the code for updating in-game objects

            // put the camera movement inside here
            ego_camera::move( PCamera, PMesh );

            // Timers

            // down the revive timer
            if ( timer_revive > 0 ) timer_revive--;

            // Clocks
            clock_wld += UPDATE_SKIP;
            clock_enc_stat++;
            clock_chr_stat++;

            // Counters
            update_wld++;
            update_loop_cnt++;

            net_synchronize_game_clock();
        }
        PROFILE_END2( game_single_update );

        // estimate how much time the main loop is taking per second
        est_single_update_time = 0.9 * est_single_update_time + 0.1 * PROFILE_QUERY( game_single_update );
        est_single_ups         = 0.9 * est_single_ups         + 0.1 * ( 1.0 / PROFILE_QUERY( game_single_update ) );
    }

    return update_loop_cnt;
}

//--------------------------------------------------------------------------------------------
void game_update_clocks()
{
    /// @details ZZ@> This function updates the game timers

    // the low pas filter pafameters
    const float fold = 0.77f;
    const float fnew = 1.0f - fold;

    // track changes to the game process
    static bool_t game_on_old = bfalse, game_on_new = bfalse;
    bool_t game_on_change = bfalse;

    // track changes to the network state
    static bool_t net_on_old = bfalse, net_on_new = bfalse;
    bool_t net_on_change = bfalse;

    // variables to store the raw number of ticks
    static Uint32 ego_ticks_old = 0;
    static Uint32 ego_ticks_new = 0;
    Sint32 ego_ticks_diff = 0;

    // grab the current number of ticks
    ego_ticks_old  = ego_ticks_new;
    ego_ticks_new  = egoboo_get_ticks();
    ego_ticks_diff = ego_ticks_new - ego_ticks_old;

    // track changes in the game state
    game_on_old = game_on_new;
    game_on_new = ( rv_success == GProc->running() ) && !GProc->mod_paused;
    game_on_change = ( game_on_old != game_on_new );

    // track changes in the network state
    net_on_old = net_on_new;
    net_on_new = network_initialized();
    net_on_change = ( net_on_old != net_on_new );

    // determine the amount of ticks for various conditions
    if ( !ups_clk.initialized )
    {
        ups_clk.init();
        ego_ticks_diff = 0;
    }
    else
    {
        // determine how many frames and game updates there have been since the last time
        ups_clk.update_counters();

        // increment the ups_clk's ticks
        ups_clk.update_ticks();

        if ( single_update_mode )
        {
            // the correct assumption for single frame mode
            ego_ticks_diff = SDL_max( UPDATE_SKIP * ups_clk.update_dif, FRAME_SKIP * ups_clk.frame_dif );
        }
        else if ( net_on_new )
        {
            // no pause for network games
            ego_ticks_diff = ups_clk.tick_dif;
        }
        else if ( game_on_new )
        {
            // normal operation
            ego_ticks_diff = ups_clk.tick_dif;
        }
        else
        {
            // this should only happen for paused, non-network games
            ego_ticks_diff = 0;
        }
    }

    // make sure that there is at least one tick since the last function call
    if ( 0 == ego_ticks_diff ) return;

    //---- deal with the update/frame stabilization

    // update the game clock
    clock_lst  = clock_all;
    clock_all += ego_ticks_diff;

    // Use the number of updates that should have been performed up to this point (true_update)
    // to try to regulate the update speed of the game
    // By limiting this loop to 10, you are essentially saying that the update loop
    // can go 10 times as fast as normal to help update_wld catch up to true_update,
    // but it can't completely bog doen the game
    true_update = clock_all / UPDATE_SKIP;

    // get the number of frames that should have happened so far in a similar way
    true_frame  = clock_all / FRAME_SKIP;

    // estimate the number of updates necessary next time around
    estimated_updates = SDL_max( 0, true_update - update_wld );

    //---- calculate the updates-per-second

    if ( ups_clk.update_cnt > 0 && ups_clk.tick_cnt > 0 )
    {
        stabilized_ups_sum    = stabilized_ups_sum * fold + fnew * ( float ) ups_clk.update_cnt / (( float ) ups_clk.tick_cnt / TICKS_PER_SEC );
        stabilized_ups_weight = stabilized_ups_weight * fold + fnew;

        // blank these every so often so that the numbers don't overflow
        if ( ups_clk.update_cnt > 10 * TARGET_UPS )
        {
            ups_clk.blank();
        }
    }

    if ( stabilized_ups_weight > 0.5f )
    {
        stabilized_ups = stabilized_ups_sum / stabilized_ups_weight;
    }
}

//--------------------------------------------------------------------------------------------
void reset_timers()
{
    /// @details ZZ@> This function resets the timers...

    clock_stt = ticks_now = ticks_last = egoboo_get_ticks();

    // all of the clocks
    clock_lst = 0;
    clock_all = 0;
    clock_wld = 0;
    clock_enc_stat  = 0;
    clock_chr_stat  = 0;

    // all of the timers
    timer_heartbeat = 0;        /// @note ZF@> This one should timeout on module startup so start at 0
    timer_pit = PIT_DELAY;

    // all of the counters
    update_wld  = 0;
    true_update = 0;
    estimated_updates = 0;
    frame_all   = 0;
    true_frame  = 0;

    // some derived variables that must be reset
    net_stats.out_of_sync = bfalse;
    pits.kill = pits.teleport = bfalse;
}

//--------------------------------------------------------------------------------------------
int game_do_menu( ego_menu_process * mproc )
{
    /// @details BB@> do menus

    int menuResult;
    bool_t need_menu = bfalse;

    need_menu = bfalse;
    if ( flip_pages_requested() )
    {
        // someone else (and that means the game) has drawn a frame
        // so we just need to draw the menu over that frame
        need_menu = btrue;

        // force the menu to be displayed immediately when the game stops
        mproc->dtime = 1.0f / ( float )cfg.framelimit;
    }
    else if ( rv_success != GProc->running() )
    {
        // the menu's frame rate is controlled by a timer
        mproc->ticks_now = SDL_GetTicks();
        if ( mproc->ticks_now > mproc->ticks_next )
        {
            // FPS limit
            float frameskip = float( TICKS_PER_SEC ) / float( cfg.framelimit );
            mproc->ticks_next = mproc->ticks_now + frameskip;

            need_menu = btrue;
            mproc->dtime = 1.0f / float( cfg.framelimit );
        }
    }

    menuResult = 0;
    if ( need_menu )
    {
        ui_beginFrame( mproc->dtime );
        {
            menuResult = doMenu( mproc->dtime );
            request_flip_pages();
        }
        ui_endFrame();
    }

    return menuResult;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_game_process * ego_game_process::ctor_this( ego_game_process * gproc )
{
    if ( NULL == gproc ) return NULL;

    ego_process::ctor_this( gproc );
    ego_game_process_data::ctor_this( gproc );

    // initialize all the profile variables
    PROFILE_INIT( game_update_loop );
    PROFILE_INIT( game_single_update );
    PROFILE_INIT( gfx_loop );

    PROFILE_INIT( talk_to_remotes );
    PROFILE_INIT( net_dispatch_packets );
    PROFILE_INIT( check_stats );
    PROFILE_INIT( read_local_latches );
    PROFILE_INIT( PassageStack_check_music );
    PROFILE_INIT( cl_talkToHost );

    return gproc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv ego_game_process::do_beginning()
{
    if ( NULL == this )  return rv_error;
    result = -1;

    if ( !validate() ) return rv_error;

    BillboardList_init_all();

    escape_latch = bfalse;

    // initialize math objects
    make_randie();
    make_turntosin();

    // Linking system
    log_info( "Initializing module linking... " );
    if ( link_build_vfs( "mp_data/link.txt", LinkList ) ) log_message( "Success!\n" );
    else log_message( "Failure!\n" );

    // initialize the collision system
    collision_system_begin();

    // initialize the "profile system"
    profile_system_begin();

    // do some graphics initialization
    // make_lightdirectionlookup();
    make_enviro();
    ego_camera::ctor_this( PCamera );

    // try to start a new module
    if ( !game_begin_module( pickedmodule.path, Uint32( ~0L ) ) )
    {
        // failure - kill the game process
        kill();
        MProc->resume();
    }

    ego_obj_BSP::ctor_this( &ego_obj_BSP::root, &mpd_BSP::root );

    // initialize all the profile variables
    PROFILE_RESET( game_update_loop );
    PROFILE_RESET( game_single_update );
    PROFILE_RESET( gfx_loop );

    PROFILE_RESET( talk_to_remotes );
    PROFILE_RESET( net_dispatch_packets );
    PROFILE_RESET( check_stats );
    PROFILE_RESET( read_local_latches );
    PROFILE_RESET( PassageStack_check_music );
    PROFILE_RESET( cl_talkToHost );

    // reset the fps clock
    fps_clk.init();
    stabilized_fps_sum    = 0.1f * TARGET_FPS;
    stabilized_fps_weight = 0.1f;

    // reset the ups clock
    ups_clk.init();
    stabilized_ups_sum    = 0.1f * TARGET_UPS;
    stabilized_ups_weight = 0.1f;

    // re-initialize these variables
    est_max_fps          =  TARGET_FPS;
    est_render_time      =  1.0f / TARGET_FPS;

    est_update_time      =  1.0f / TARGET_UPS;
    est_max_ups          =  TARGET_UPS;

    est_gfx_time         =  1.0f / TARGET_FPS;
    est_max_gfx          =  TARGET_FPS;

    est_single_update_time  = 1.0f / TARGET_UPS;
    est_single_ups          = TARGET_UPS;

    est_update_game_time  = 1.0f / TARGET_UPS;
    est_max_game_ups      = TARGET_UPS;

    // Initialize the process
    valid = btrue;

    // go to the next state
    result = 0;
    state = proc_entering;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
void game_update_controls()
{
    // update the states of the input devices
    input_read();

    // convert the input device states to player latches
    PROFILE_BEGIN( read_local_latches );
    {
        read_local_latches();
    }
    PROFILE_END2( read_local_latches );
}

//--------------------------------------------------------------------------------------------
void game_update_network()
{
    // NETWORK PORT

    // handle any new packets from the host
    PROFILE_BEGIN( net_dispatch_packets );
    {
        net_dispatch_packets();
    }
    PROFILE_END2( net_dispatch_packets );

    // sends the player's local latches to the host
    PROFILE_BEGIN( cl_talkToHost );
    {
        cl_talkToHost();
    }
    PROFILE_END2( cl_talkToHost );

    // handle all player latches sent from the "remotes"
    PROFILE_BEGIN( talk_to_remotes );
    {
        sv_talkToRemotes();
    }
    PROFILE_END2( talk_to_remotes );

}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_game_process::do_running()
{
    static was_in_game = bfalse;

    int update_loops = 0;

    if ( NULL == this )  return rv_error;
    result = -1;

    if ( !validate() ) return rv_error;

    was_active  = valid;

    if ( paused )
    {
        result = 0;
        return rv_success;
    }

    ups_ticks_now = SDL_GetTicks();
    if (( !single_update_mode && ups_ticks_now > ups_ticks_next ) || ( single_update_mode && single_update_requested ) )
    {
        // UPS limit
        ups_ticks_next = ups_ticks_now + UPDATE_SKIP / 4;

        PROFILE_BEGIN( game_update_loop );
        {
            // start the console mode?
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_MESSAGE ) )
            {
                // reset the keyboard buffer
                SDL_EnableKeyRepeat( 20, SDL_DEFAULT_REPEAT_DELAY );
                console_mode = btrue;
                console_done = bfalse;
                keyb.buffer_count = 0;
                keyb.buffer[0] = CSTR_END;
            }

            // This is the control loop
            if ( network_initialized() && console_done )
            {
                net_send_message();
            }

            // performance planning
            PROFILE_BEGIN( game_update_clocks );
            {
                game_update_clocks();
            }
            PROFILE_END2( game_update_clocks );

            // This is what would display network chat message,
            // so that part of this function needs to be here.
            // maybe everything else belongs inside the if structure, below?
            PROFILE_BEGIN( check_stats );
            {
                check_stats();
            }
            PROFILE_END2( check_stats );

            // checking the music. should not have any excect except in-game?
            PROFILE_BEGIN( PassageStack_check_music );
            {
                PassageStack_check_music();
            }
            PROFILE_END2( PassageStack_check_music );

            // do the updates
            if ( mod_paused && !network_initialized() )
            {
                clock_wld = clock_all;
            }
            else
            {
                if ( network_waiting_for_players() )
                {
                    // do all the network io
                    game_update_network();

                    // count the players of various types
                    game_update_network_stats();

                    // force no lag
                    clock_wld = clock_all;
                }
                else
                {
                    update_loops = game_update();
                }

                est_update_game_time = 0.9f * est_update_game_time + 0.1f * est_single_update_time * update_loops;
                est_max_game_ups     = 0.9f * est_max_game_ups     + 0.1f * 1.0f / est_update_game_time;
            }
        }
        PROFILE_END2( game_update_loop );

        // estimate the main-loop update time is taking per inner-loop iteration
        // do a kludge to average out the effects of functions like PassageStack_check_music()
        // even when the inner loop does not execute
        if ( update_loops > 0 )
        {
            est_update_time = 0.9 * est_update_time + 0.1 * PROFILE_QUERY( game_update_loop ) / update_loops;
            est_max_ups     = 0.9 * est_max_ups     + 0.1 * ( update_loops / PROFILE_QUERY( game_update_loop ) );
        }
        else
        {
            est_update_time = 0.9 * est_update_time + 0.1 * PROFILE_QUERY( game_update_loop );
            est_max_ups     = 0.9 * est_max_ups     + 0.1 * ( 1.0 / PROFILE_QUERY( game_update_loop ) );
        }

        single_update_requested = bfalse;
    }

    // Do the display stuff
    fps_ticks_now = SDL_GetTicks();
    if (( !single_update_mode && fps_ticks_now > fps_ticks_next ) || ( single_update_mode && single_frame_requested ) )
    {
        // FPS limit
        float  frameskip = ( float )TICKS_PER_SEC / ( float )cfg.framelimit;
        fps_ticks_next = fps_ticks_now + frameskip;

        PROFILE_BEGIN( gfx_loop );
        {
            gfx_main();

            msgtimechange++;
        }
        PROFILE_END2( gfx_loop );

        // estimate how much time the main loop is taking per second
        est_gfx_time = 0.9 * est_gfx_time + 0.1 * PROFILE_QUERY( gfx_loop );
        est_max_gfx  = 0.9 * est_max_gfx  + 0.1 * ( 1.0 / PROFILE_QUERY( gfx_loop ) );

        // estimate how much time the main loop is taking per second
        est_render_time = est_gfx_time * TARGET_FPS;
        est_max_fps     = 0.9 * est_max_fps + 0.1 * ( 1.0 - est_update_time * TARGET_UPS ) / PROFILE_QUERY( gfx_loop );

        single_frame_requested = bfalse;
    }

    if ( escape_requested )
    {
        escape_requested = bfalse;

        if ( !escape_latch )
        {
            if ( PMod->beat )
            {
                game_begin_menu( MProc, emnu_ShowEndgame );
            }
            else
            {
                game_begin_menu( MProc, emnu_GamePaused );
            }

            escape_latch = btrue;
            mod_paused   = btrue;
        }
    }

    // requiring result == 0 will never allow this to self-terminate
    result = 0;

    // go to the next state?
    if ( 1 == result )
    {
        state = proc_leaving;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_game_process::do_leaving()
{
    if ( NULL == this )  return rv_error;
    result = -1;

    if ( !validate() ) return rv_error;

    // get rid of all module data
    game_quit_module();

    // resume the menu
    MProc->resume();

    // deallocate any dynamically allocated collision memory
    collision_system_end();

    // deallocate any data used by the profile system
    profile_system_end();

    // deallocate any dynamically allocated scripting memory
    scripting_system_end();

    // clean up any remaining models that might have dynamic data
    MadStack_release_all();

    // reset the fps counter
    fps_clk.init();
    stabilized_fps_sum    = 0.1f * stabilized_fps_sum / stabilized_fps_weight;
    stabilized_fps_weight = 0.1f;

    PROFILE_FREE( game_update_loop );
    PROFILE_FREE( game_single_update );
    PROFILE_FREE( gfx_loop );

    PROFILE_FREE( talk_to_remotes );
    PROFILE_FREE( net_dispatch_packets );
    PROFILE_FREE( check_stats );
    PROFILE_FREE( read_local_latches );
    PROFILE_FREE( PassageStack_check_music );
    PROFILE_FREE( cl_talkToHost );

    result = 1;

    // go to the next state
    if ( 1 == result )
    {
        state  = proc_finishing;
        killme = bfalse;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_game_process::do_finishing()
{
    if ( NULL == this )  return rv_error;
    result = -1;

    if ( !validate() ) return rv_error;

    // resume the menu process
    if ( NULL != MProc )
    {
        MProc->resume();
    }

    // shut off the game process
    terminate();

    result = 0;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF prt_find_target( float pos_x, float pos_y, float pos_z, FACING_T facing,
                         const PIP_REF & particletype, const TEAM_REF & team, const CHR_REF & donttarget, const CHR_REF & oldtarget )
{
    /// @details ZF@> This is the new improved targeting system for particles. Also includes distance in the Z direction.

    const float max_dist2 = WIDE * WIDE;

    ego_pip * ppip;

    CHR_REF besttarget = CHR_REF( MAX_CHR );
    float  longdist2 = max_dist2;

    if ( !LOADED_PIP( particletype ) ) return CHR_REF( MAX_CHR );
    ppip = PipStack + particletype;

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        bool_t target_friend, target_enemy;

        if ( !pchr->alive || pchr->isitem || pchr->pack.is_packed ) continue;

        // prefer targeting riders over the mount itself
        if ( pchr->ismount && ( INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) || INGAME_CHR( pchr->holdingwhich[SLOT_RIGHT] ) ) ) continue;

        // ignore invictus
        if ( IS_INVICTUS_PCHR_RAW( pchr ) ) continue;

        // we are going to give the player a break and not target things that
        // can't be damaged, unless the particle is homing. If it homes in,
        // the he damagetime could drop off en route.
        if ( !ppip->homing && ( 0 != pchr->damagetime ) ) continue;

        // Don't retarget someone we already had or not supposed to target
        if ( cnt == oldtarget || cnt == donttarget ) continue;

        target_friend = ppip->onlydamagefriendly && team == ego_chr::get_iteam( cnt );
        target_enemy  = !ppip->onlydamagefriendly && team_hates_team( team, ego_chr::get_iteam( cnt ) );

        if ( target_friend || target_enemy )
        {
            FACING_T angle = - facing + vec_to_facing( pchr->pos.x - pos_x , pchr->pos.y - pos_y );

            // Only proceed if we are facing the target
            if ( angle < ppip->targetangle || angle > ( 0xFFFF - ppip->targetangle ) )
            {
                float dist2 =
                    POW( SDL_abs( pchr->pos.x - pos_x ), 2 ) +
                    POW( SDL_abs( pchr->pos.y - pos_y ), 2 ) +
                    POW( SDL_abs( pchr->pos.z - pos_z ), 2 );

                if ( dist2 < longdist2 && dist2 <= max_dist2 )
                {
                    glo_useangle = angle;
                    besttarget = cnt;
                    longdist2 = dist2;
                }
            }
        }
    }
    CHR_END_LOOP();

    // All done
    return besttarget;
}

//--------------------------------------------------------------------------------------------
bool_t check_target( ego_chr * psrc, const CHR_REF & ichr_test, IDSZ idsz, BIT_FIELD targeting_bits )
{
    bool_t retval = bfalse;

    bool_t is_hated, hates_me;
    bool_t is_friend, is_prey, is_predator, is_mutual;
    ego_chr * ptst;

    // Skip non-existing objects
    if ( !PROCESSING_PCHR( psrc ) ) return bfalse;

    ptst = ChrObjList.get_allocated_data_ptr( ichr_test );
    if ( !INGAME_PCHR( ptst ) ) return bfalse;

    // Skip hidden characters
    if ( ptst->is_hidden ) return bfalse;

    // Players only?
    if (( HAS_SOME_BITS( targeting_bits, TARGET_PLAYERS ) || HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) ) && !VALID_PLA( ptst->is_which_player ) ) return bfalse;

    // Skip held objects
    if ( IS_ATTACHED_PCHR( ptst ) ) return bfalse;

    // Allow to target ourselves?
    if ( psrc == ptst && HAS_NO_BITS( targeting_bits, TARGET_SELF ) ) return bfalse;

    // Don't target our holder if we are an item and being held
    if ( psrc->isitem && psrc->attachedto == GET_REF_PCHR( ptst ) ) return bfalse;

    // Allow to target dead stuff stuff?
    if ( ptst->alive == HAS_SOME_BITS( targeting_bits, TARGET_DEAD ) ) return bfalse;

    // Don't target invisible stuff, unless we can actually see them
    if ( !chr_can_see_object( GET_REF_PCHR( psrc ), ichr_test ) ) return bfalse;

    // Need specific skill? ([NONE] always passes)
    if ( HAS_SOME_BITS( targeting_bits, TARGET_SKILL ) && 0 != ego_chr_data::get_skill( ptst, idsz ) ) return bfalse;

    // Require player to have specific quest?
    if ( HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) && VALID_PLA( ptst->is_which_player ) )
    {
        int quest_level = QUEST_NONE;
        ego_player * ppla = PlaStack + ptst->is_which_player;

        quest_level = quest_get_level( ppla->quest_log, SDL_arraysize( ppla->quest_log ), idsz );

        // find only active quests?
        // this makes it backward-compatible with zefz's version
        if ( quest_level < 0 ) return bfalse;
    }

    is_hated = team_hates_team( psrc->team, ptst->team );
    hates_me = team_hates_team( ptst->team, psrc->team );

    // Target neutral items? (still target evil items, could be pets)
    if (( ptst->isitem || IS_INVICTUS_PCHR_RAW( ptst ) ) && !HAS_SOME_BITS( targeting_bits, TARGET_ITEMS ) )
    {
        return bfalse;
    }

    // Only target those of proper team. Skip this part if it's a item
    if ( !ptst->isitem )
    {
        if (( HAS_NO_BITS( targeting_bits, TARGET_ENEMIES ) && is_hated ) ) return bfalse;
        if (( HAS_NO_BITS( targeting_bits, TARGET_FRIENDS ) && !is_hated ) ) return bfalse;
    }

    // these options are here for ideas of ways to mod this function
    is_friend    = !is_hated && !hates_me;
    is_prey      =  is_hated && !hates_me;
    is_predator  = !is_hated &&  hates_me;
    is_mutual    =  is_hated &&  hates_me;

    // This is the last and final step! Check for specific IDSZ too?
    if ( IDSZ_NONE == idsz )
    {
        retval = btrue;
    }
    else
    {
        bool_t match_idsz = ( idsz == pro_get_idsz( ptst->profile_ref, IDSZ_PARENT ) ) ||
                            ( idsz == pro_get_idsz( ptst->profile_ref, IDSZ_TYPE ) );

        if ( match_idsz )
        {
            if ( !HAS_SOME_BITS( targeting_bits, TARGET_INVERTID ) ) retval = btrue;
        }
        else
        {
            if ( HAS_SOME_BITS( targeting_bits, TARGET_INVERTID ) ) retval = btrue;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_find_target( ego_chr * psrc, float max_dist, IDSZ idsz, BIT_FIELD targeting_bits )
{
    /// @details ZF@> This is the new improved AI targeting system. Also includes distance in the Z direction.
    ///     If max_dist is 0 then it searches without a max limit.

    ego_line_of_sight_info los_info;

    Uint16 cnt;
    CHR_REF best_target = CHR_REF( MAX_CHR );
    float  best_dist2, max_dist2;

    size_t search_list_size = 0;
    CHR_REF search_list[MAX_CHR];

    if ( !PROCESSING_PCHR( psrc ) ) return CHR_REF( MAX_CHR );

    max_dist2 = max_dist * max_dist;

    if ( HAS_SOME_BITS( targeting_bits, TARGET_PLAYERS ) )
    {
        PLA_REF ipla;
        for ( ipla = 0; ipla < MAX_PLAYER; ipla ++ )
        {
            ego_player * ppla = PlaStack + ipla;

            if ( !ppla->valid ) continue;

            if ( !INGAME_CHR( ppla->index ) ) continue;

            search_list[search_list_size] = ppla->index;
            search_list_size++;
        }
    }
    else
    {
        CHR_BEGIN_LOOP_ACTIVE( tnc, pchr )
        {
            search_list[search_list_size] = tnc;
            search_list_size++;
        }
        CHR_END_LOOP();
    }

    // set the line-of-sight source
    los_info.x0         = psrc->pos.x;
    los_info.y0         = psrc->pos.y;
    los_info.z0         = psrc->pos.z + psrc->bump.height;
    los_info.stopped_by = psrc->stoppedby;

    best_target = CHR_REF( MAX_CHR );
    best_dist2  = max_dist2;
    for ( cnt = 0; cnt < search_list_size; cnt++ )
    {
        float  dist2;
        fvec3_t   diff;
        ego_chr * ptst;
        CHR_REF ichr_test = search_list[cnt];

        ptst = ChrObjList.get_allocated_data_ptr( ichr_test );
        if ( !INGAME_PCHR( ptst ) ) continue;

        if ( !check_target( psrc, ichr_test, idsz, targeting_bits ) ) continue;

        diff  = fvec3_sub( psrc->pos.v, ptst->pos.v );
        dist2 = fvec3_dot_product( diff.v, diff.v );

        if (( 0 == max_dist2 || dist2 <= max_dist2 ) && ( MAX_CHR == best_target || dist2 < best_dist2 ) )
        {
            // Invictus chars do not need a line of sight
            if ( !IS_INVICTUS_PCHR_RAW( psrc ) )
            {
                // set the line-of-sight source
                los_info.x1 = ptst->pos.x;
                los_info.y1 = ptst->pos.y;
                los_info.z1 = ptst->pos.z + SDL_max( 1, ptst->bump.height );

                if ( do_line_of_sight( &los_info ) ) continue;
            }

            // Set the new best target found
            best_target = ichr_test;
            best_dist2  = dist2;
        }
    }

    // make sure the target is valid
    if ( !INGAME_CHR( best_target ) ) best_target = CHR_REF( MAX_CHR );

    return best_target;
}

//--------------------------------------------------------------------------------------------
void do_damage_tiles()
{
    // do the damage tile stuff

    CHR_BEGIN_LOOP_ACTIVE( character, pchr )
    {
        ego_cap * pcap;

        if ( !INGAME_CHR( character ) ) continue;

        pcap = pro_get_pcap( pchr->profile_ref );
        if ( NULL == pcap ) continue;

        // if the object is not really in the game, do nothing
        if ( pchr->is_hidden || !pchr->alive ) continue;

        // if you are being held by something, you are protected
        if ( IS_ATTACHED_PCHR( pchr ) ) continue;

        // are we on a damage tile?
        if ( !ego_mpd::grid_is_valid( PMesh, pchr->onwhichgrid ) ) continue;
        if ( 0 == ego_mpd::test_fx( PMesh, pchr->onwhichgrid, MPDFX_DAMAGE ) ) continue;

        // are we low enough?
        if ( pchr->pos.z > pchr->enviro.grid_level + DAMAGERAISE ) continue;

        // allow reaffirming damage to things like torches, even if they are being held,
        // but make the tolerance closer so that books won't burn so easily
        if ( !INGAME_CHR( pchr->attachedto ) || pchr->pos.z < pchr->enviro.grid_level + DAMAGERAISE )
        {
            if ( pchr->reaffirmdamagetype == damagetile.type )
            {
                if ( 0 == ( update_wld & TILEREAFFIRMAND ) )
                {
                    reaffirm_attached_particles( character );
                }
            }
        }

        // do not do direct damage to items that are being held
        if ( INGAME_CHR( pchr->attachedto ) ) continue;

        // don't do direct damage to invulnerable objects
        if ( IS_INVICTUS_PCHR_RAW( pchr ) ) continue;

        //@todo: sound of lava sizzling and such
        // distance = SDL_abs( PCamera->track_pos.x - pchr->pos.x ) + SDL_abs( PCamera->track_pos.y - pchr->pos.y );

        //if ( distance < damagetile.min_distance )
        //{
        //    damagetile.min_distance = distance;
        //}

        //if ( distance < damagetile.min_distance + 256 )
        //{
        //    damagetile.sound_time = 0;
        //}

        if ( 0 == pchr->damagetime )
        {
            int actual_damage;
            actual_damage = damage_character( character, ATK_BEHIND, damagetile.amount, damagetile.type, TEAM_REF( TEAM_DAMAGE ), CHR_REF( MAX_CHR ), DAMFX_NBLOC | DAMFX_ARMO, bfalse );
            pchr->damagetime = DAMAGETILETIME;

            if (( actual_damage > 0 ) && ( -1 != damagetile.parttype ) && 0 == ( update_wld & damagetile.partand ) )
            {
                spawn_one_particle_global( pchr->pos, ATK_FRONT, damagetile.parttype, 0 );
            }
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void game_update_pits()
{
    /// @details ZZ@> This function kills any character in a deep pit...

    if ( pits.kill || pits.teleport )
    {
        // Decrease the timer
        if ( timer_pit > 0 ) timer_pit--;

        if ( timer_pit == 0 )
        {
            // Reset timer
            timer_pit = PIT_DELAY;

            // Kill any particles that fell in a pit, if they die in water...
            PRT_BEGIN_LOOP_ACTIVE( iprt, prt_bdl )
            {
                if ( prt_bdl.prt_ptr->pos.z < PITDEPTH && prt_bdl.pip_ptr->end_water )
                {
                    ego_obj_prt::request_terminate( &prt_bdl );
                }
            }
            PRT_END_LOOP();

            // Kill or teleport any characters that fell in a pit...
            CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
            {
                // Is it a valid character?
                if ( IS_INVICTUS_PCHR_RAW( pchr ) || !pchr->alive ) continue;

                if ( IS_ATTACHED_PCHR( pchr ) ) continue;

                // Do we kill it?
                if ( pits.kill && pchr->pos.z < PITDEPTH )
                {
                    // Got one!
                    kill_character( ichr, CHR_REF( MAX_CHR ), bfalse );
                    pchr->vel.x = 0;
                    pchr->vel.y = 0;

                    /// @note ZF@> Disabled, the pitfall sound was intended for pits.teleport only
                    /// Play sound effect
                    /// sound_play_chunk( pchr->pos, g_wavelist[GSND_PITFALL] );
                }

                // Do we teleport it?
                if ( pits.teleport && pchr->pos.z < PITDEPTH * 4 )
                {
                    bool_t teleported;

                    // Teleport them back to a "safe" spot
                    teleported = chr_teleport( ichr, pits.teleport_pos.x, pits.teleport_pos.y, pits.teleport_pos.z, pchr->ori.facing_z );

                    if ( !teleported )
                    {
                        // Kill it instead
                        kill_character( ichr, CHR_REF( MAX_CHR ), bfalse );
                    }
                    else
                    {
                        // Stop movement
                        pchr->vel.z = 0;
                        pchr->vel.x = 0;
                        pchr->vel.y = 0;

                        // Play sound effect
                        if ( VALID_PLA( pchr->is_which_player ) )
                        {
                            sound_play_chunk_full( g_wavelist[GSND_PITFALL] );
                        }
                        else
                        {
                            sound_play_chunk( pchr->pos, g_wavelist[GSND_PITFALL] );
                        }

                        // Do some damage (same as damage tile)
                        damage_character( ichr, ATK_BEHIND, damagetile.amount, damagetile.type, TEAM_REF( TEAM_DAMAGE ), ego_chr::get_pai( ichr )->bumplast, DAMFX_NBLOC | DAMFX_ARMO, bfalse );
                    }
                }
            }
            CHR_END_LOOP();
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn_particles()
{
    /// @details ZZ@> This function drops snowflakes or rain or whatever, also swings the camera

    bool_t   spawn_one    = bfalse;

    if ( weather.time > 0 )
    {
        weather.time--;
        if ( 0 == weather.time )
        {
            spawn_one = btrue;
            weather.time = weather.timer_reset;
        }
    }

    if ( spawn_one )
    {
        int          cnt;
        PLA_REF      weather_ipla( MAX_PLAYER );
        ego_player * weather_ppla = NULL;
        ego_chr    * weather_pchr = NULL;
        PRT_REF      weather_iprt( MAX_PRT );

        if ( !VALID_PLA( weather.iplayer ) )
        {
            weather.iplayer = MAX_PLAYER;
        }

        // Find a valid player
        weather_ipla = weather.iplayer;
        weather_ppla = NULL;
        weather_pchr = NULL;
        for ( cnt = 0; cnt < MAX_PLAYER; cnt++ )
        {
            ego_player * tmp_ppla = NULL;
            ego_chr    * tmp_pchr = NULL;

            weather_ipla = PLA_REF(( weather_ipla.get_value() + 1 ) % MAX_PLAYER );

            tmp_ppla = PlaStack + weather_ipla;
            if ( !tmp_ppla->valid ) continue;

            if ( !INGAME_CHR( tmp_ppla->index ) ) continue;
            tmp_pchr = ChrObjList.get_data_ptr( tmp_ppla->index );

            // no weather if in a pack
            if ( tmp_pchr->pack.is_packed ) continue;

            weather_ppla = tmp_ppla;
            weather_pchr = tmp_pchr;
        }

        // Did we find one?
        if ( NULL != weather_ppla )
        {
            weather.iplayer = weather_ipla;
        }

        // if the character valid, spawn a weather particle over its head
        weather_iprt = MAX_PRT;
        if ( NULL != weather_pchr )
        {
            weather_iprt = spawn_one_particle_global( weather_pchr->pos, ATK_FRONT, weather.particle, 0 );
        }

        // is the particle valid?
        ego_prt * pprt = PrtObjList.get_allocated_data_ptr( weather_iprt );
        if ( NULL != pprt )
        {
            bool_t destroy_particle = bfalse;

            if ( weather.over_water && !prt_is_over_water( weather_iprt ) )
            {
                destroy_particle = btrue;
            }
            else if ( prt_test_wall( pprt, NULL ) )
            {
                destroy_particle = btrue;
            }
            else
            {
                // Weather particles spawned at the edge of the map look ugly, so don't spawn them there
                if ( pprt->pos.x < EDGE || pprt->pos.x > PMesh->gmem.edge_x - EDGE )
                {
                    destroy_particle = btrue;
                }
                else if ( pprt->pos.y < EDGE || pprt->pos.y > PMesh->gmem.edge_y - EDGE )
                {
                    destroy_particle = btrue;
                }
            }

            if ( destroy_particle )
            {
                PrtObjList.free_one( weather_iprt );
            }
        }
    }

    PCamera->swing = ( PCamera->swing + PCamera->swingrate ) & 0x3FFF;
}

//--------------------------------------------------------------------------------------------
void read_player_local_latch( const PLA_REF & player )
{
    /// @details ZZ@> This function converts input readings to latch settings, so players can
    ///    move around

    TURN_T        cam_turn;
    float         fsin, fcos, dist, scale;
    fvec3_t       vcam_fwd, vcam_rgt;
    latch_input_t sum;

    ego_chr          * pchr;
    ego_player       * ppla;
    ego_input_device * pdevice;

    if ( INVALID_PLA( player ) ) return;
    ppla = PlaStack + player;

    pdevice = &( ppla->device );

    if ( !INGAME_CHR( ppla->index ) ) return;
    pchr = ChrObjList.get_data_ptr( ppla->index );

    // is the device a local device or an internet device?
    if ( pdevice->bits == EMPTY_BIT_FIELD ) return;

    // Clear the player's latch buffers
    latch_input_init( &( sum ) );

    // generate the transforms relative to the camera
    cam_turn = TO_TURN( PCamera->ori.facing_z );
    fsin = turntosin[cam_turn];
    fcos = turntocos[cam_turn];

    //vcam_fwd = mat_getCamForward( PCamera->mView );
    vcam_fwd.x = fsin;
    vcam_fwd.y = fcos;
    vcam_fwd.z = 0.0f;

    //vcam_rgt = mat_getCamRight  ( PCamera->mView );
    vcam_rgt.x = fcos;
    vcam_rgt.y = -fsin;
    vcam_rgt.z = 0.0f;

    // Mouse routines
    if ( HAS_SOME_BITS( pdevice->bits, INPUT_BITS_MOUSE ) && mous.on )
    {
        latch_2d_t joy_latch = LATCH_2D_INIT;
        fvec3_t    joy_latch_trans = ZERO_VECT3;

        // Read buttons
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_JUMP ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_JUMP );
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_USE ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_LEFT );
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_GET ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_ALTLEFT );
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_PACK ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_PACKLEFT );
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_USE ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_RIGHT );
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_GET ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_ALTRIGHT );
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_PACK ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_PACKRIGHT );

        if (( CAM_TURN_GOOD == PCamera->turn_mode && 1 == net_stats.pla_count_local ) ||
            !control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) )  // Don't allow movement in camera control mode
        {
            dist = SQRT( mous.x * mous.x + mous.y * mous.y );
            if ( dist > 0.0f )
            {
                scale = mous.sense / dist;
                if ( dist < mous.sense )
                {
                    scale = dist / mous.sense;
                }

                scale = scale / mous.sense;
                joy_latch.dir[kX] = mous.x * scale;
                joy_latch.dir[kY] = mous.y * scale;

                if ( CAM_TURN_GOOD == PCamera->turn_mode &&
                     1 == net_stats.pla_count_local &&
                     control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) == 0 )  joy_latch.dir[kX] = 0;

                joy_latch = ego_chr::convert_latch_2d( pchr, joy_latch );

                joy_latch_trans.x = joy_latch.dir[kX] * vcam_rgt.x + joy_latch.dir[kY] * vcam_fwd.x;
                joy_latch_trans.y = joy_latch.dir[kX] * vcam_rgt.y + joy_latch.dir[kY] * vcam_fwd.y;
                joy_latch_trans.z = joy_latch.dir[kX] * vcam_rgt.z + joy_latch.dir[kY] * vcam_fwd.z;
            }
        }

        // add the latch values to the sum
        sum.raw[kX] += joy_latch.dir[kX];
        sum.raw[kY] += joy_latch.dir[kY];

        sum.dir[kX] += joy_latch_trans.x;
        sum.dir[kY] += joy_latch_trans.y;
        sum.dir[kZ] += joy_latch_trans.z;

        ADD_BITS( sum.b, joy_latch.b );
    }

    // Joystick A routines
    if ( HAS_SOME_BITS( pdevice->bits, INPUT_BITS_JOYA ) && joy[0].on )
    {
        latch_2d_t joy_latch = LATCH_2D_INIT;
        fvec3_t joy_latch_trans = ZERO_VECT3;

        // Read buttons
        if ( control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_JUMP ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_JUMP );
        if ( control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_LEFT_USE ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_LEFT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_LEFT_GET ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_ALTLEFT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_LEFT_PACK ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_PACKLEFT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_RIGHT_USE ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_RIGHT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_RIGHT_GET ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_ALTRIGHT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_RIGHT_PACK ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_PACKRIGHT );

        if (( CAM_TURN_GOOD == PCamera->turn_mode && 1 == net_stats.pla_count_local ) ||
            !control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_CAMERA ) )
        {
            joy_latch.dir[kX] = joy[0].x;
            joy_latch.dir[kY] = joy[0].y;

            dist = joy_latch.dir[kX] * joy_latch.dir[kX] + joy_latch.dir[kY] * joy_latch.dir[kY];
            if ( dist > 1.0f )
            {
                scale = 1.0f / SQRT( dist );
                joy_latch.dir[kX] *= scale;
                joy_latch.dir[kY] *= scale;
            }

            if ( CAM_TURN_GOOD == PCamera->turn_mode &&
                 1 == net_stats.pla_count_local &&
                 !control_is_pressed( INPUT_DEVICE_JOY_A, CONTROL_CAMERA ) )  joy_latch.dir[kX] = 0;

            joy_latch = ego_chr::convert_latch_2d( pchr, joy_latch );

            joy_latch_trans.x = joy_latch.dir[kX] * vcam_rgt.x + joy_latch.dir[kY] * vcam_fwd.x;
            joy_latch_trans.y = joy_latch.dir[kX] * vcam_rgt.y + joy_latch.dir[kY] * vcam_fwd.y;
            joy_latch_trans.z = joy_latch.dir[kX] * vcam_rgt.z + joy_latch.dir[kY] * vcam_fwd.z;
        }

        // add the latch values to the sum
        sum.raw[kX] += joy_latch.dir[kX];
        sum.raw[kY] += joy_latch.dir[kY];

        sum.dir[kX] += joy_latch_trans.x;
        sum.dir[kY] += joy_latch_trans.y;
        sum.dir[kZ] += joy_latch_trans.z;

        ADD_BITS( sum.b, joy_latch.b );
    }

    // Joystick B routines
    if ( HAS_SOME_BITS( pdevice->bits, INPUT_BITS_JOYB ) && joy[1].on )
    {
        latch_2d_t joy_latch = LATCH_2D_INIT;
        fvec3_t joy_latch_trans = ZERO_VECT3;

        // Read buttons
        if ( control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_JUMP ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_JUMP );
        if ( control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_LEFT_USE ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_LEFT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_LEFT_GET ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_ALTLEFT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_LEFT_PACK ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_PACKLEFT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_RIGHT_USE ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_RIGHT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_RIGHT_GET ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_ALTRIGHT );
        if ( control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_RIGHT_PACK ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_PACKRIGHT );

        if (( CAM_TURN_GOOD == PCamera->turn_mode && 1 == net_stats.pla_count_local ) ||
            !control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_CAMERA ) )
        {
            joy_latch.dir[kX] = joy[1].x;
            joy_latch.dir[kY] = joy[1].y;

            dist = joy_latch.dir[kX] * joy_latch.dir[kX] + joy_latch.dir[kY] * joy_latch.dir[kY];
            if ( dist > 1.0f )
            {
                scale = 1.0f / SQRT( dist );
                joy_latch.dir[kX] *= scale;
                joy_latch.dir[kY] *= scale;
            }

            if ( CAM_TURN_GOOD == PCamera->turn_mode &&
                 1 == net_stats.pla_count_local &&
                 !control_is_pressed( INPUT_DEVICE_JOY_B, CONTROL_CAMERA ) )  joy_latch.dir[kX] = 0;

            joy_latch = ego_chr::convert_latch_2d( pchr, joy_latch );

            joy_latch_trans.x = joy_latch.dir[kX] * vcam_rgt.x + joy_latch.dir[kY] * vcam_fwd.x;
            joy_latch_trans.y = joy_latch.dir[kX] * vcam_rgt.y + joy_latch.dir[kY] * vcam_fwd.y;
            joy_latch_trans.z = joy_latch.dir[kX] * vcam_rgt.z + joy_latch.dir[kY] * vcam_fwd.z;
        }

        // add the latch values to the sum
        sum.raw[kX] += joy_latch.dir[kX];
        sum.raw[kY] += joy_latch.dir[kY];

        sum.dir[kX] += joy_latch_trans.x;
        sum.dir[kY] += joy_latch_trans.y;
        sum.dir[kZ] += joy_latch_trans.z;

        ADD_BITS( sum.b, joy_latch.b );
    }

    // Keyboard routines
    if ( HAS_SOME_BITS( pdevice->bits, INPUT_BITS_KEYBOARD ) && keyb.on )
    {
        latch_2d_t joy_latch = LATCH_2D_INIT;
        fvec3_t joy_latch_trans = ZERO_VECT3;

        // Read buttons
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_JUMP ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_JUMP );
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_LEFT_USE ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_LEFT );
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_LEFT_GET ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_ALTLEFT );
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_LEFT_PACK ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_PACKLEFT );
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_RIGHT_USE ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_RIGHT );
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_RIGHT_GET ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_ALTRIGHT );
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_RIGHT_PACK ) )
            ADD_BITS( joy_latch.b, LATCHBUTTON_PACKRIGHT );

        if (( CAM_TURN_GOOD == PCamera->turn_mode && 1 == net_stats.pla_count_local ) ||
            !control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_CAMERA ) )
        {
            joy_latch.dir[kX] = ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_LEFT ) );
            joy_latch.dir[kY] = ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) );

            if ( CAM_TURN_GOOD == PCamera->turn_mode &&
                 1 == net_stats.pla_count_local )  joy_latch.dir[kX] = 0;

            joy_latch = ego_chr::convert_latch_2d( pchr, joy_latch );

            joy_latch_trans.x = joy_latch.dir[kX] * vcam_rgt.x + joy_latch.dir[kY] * vcam_fwd.x;
            joy_latch_trans.y = joy_latch.dir[kX] * vcam_rgt.y + joy_latch.dir[kY] * vcam_fwd.y;
            joy_latch_trans.z = joy_latch.dir[kX] * vcam_rgt.z + joy_latch.dir[kY] * vcam_fwd.z;
        }

        // add the latch values to the sum
        sum.raw[kX] += joy_latch.dir[kX];
        sum.raw[kY] += joy_latch.dir[kY];

        sum.dir[kX] += joy_latch_trans.x;
        sum.dir[kY] += joy_latch_trans.y;
        sum.dir[kZ] += joy_latch_trans.z;

        ADD_BITS( sum.b, joy_latch.b );
    }

    // add in some sustain
    input_device_add_latch( pdevice, sum );

    // copy the latch
    ppla->local_latch   = pdevice->latch;
    ppla->local_latch.b = sum.b;
}

//--------------------------------------------------------------------------------------------
void read_local_latches( void )
{
    /// @details ZZ@> This function emulates AI thinkin' by setting latches from input devices

    PLA_REF cnt;

    for ( cnt = 0; cnt < MAX_PLAYER; cnt++ )
    {
        read_player_local_latch( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void check_stats()
{
    /// @details ZZ@> This function lets the players check character stats

    static int stat_check_timer = 0;
    static int stat_check_delay = 0;

    int ticks;
    if ( console_mode ) return;

    ticks = egoboo_get_ticks();
    if ( ticks > stat_check_timer + 20 )
    {
        stat_check_timer = ticks;
    }

    stat_check_delay -= 20;
    if ( stat_check_delay > 0 ) return;

    // Show map cheat
    if ( cfg.dev_mode && mapvalid && SDLKEYDOWN( SDLK_LSHIFT ) && SDLKEYDOWN( SDLK_m ) )
    {
        mapon = !mapon;
        youarehereon = btrue;
        stat_check_delay = 150;
    }

    // XP CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_x ) )
    {
        PLA_REF docheat = PLA_REF( MAX_PLAYER );
        if ( SDLKEYDOWN( SDLK_1 ) )  docheat = 0;
        else if ( SDLKEYDOWN( SDLK_2 ) )  docheat = 1;
        else if ( SDLKEYDOWN( SDLK_3 ) )  docheat = 2;
        else if ( SDLKEYDOWN( SDLK_4 ) )  docheat = 3;

        // Apply the cheat if valid
        if ( VALID_PLA( docheat ) )
        {
            ego_player * ppla = PlaStack + docheat;
            if ( INGAME_CHR( ppla->index ) )
            {
                Uint32  xpgain;
                ego_chr * pchr = ChrObjList.get_data_ptr( ppla->index );
                ego_cap * pcap = pro_get_pcap( pchr->profile_ref );

                // Give 10% of XP needed for next level
                xpgain = 0.1f * ( pcap->experience_forlevel[SDL_min( pchr->experience_level+1, MAXLEVEL )] - pcap->experience_forlevel[pchr->experience_level] );
                give_experience( pchr->ai.index, xpgain, XP_DIRECT, btrue );
                stat_check_delay = 1;
            }
        }
    }

    // LIFE CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_z ) )
    {
        PLA_REF docheat = PLA_REF( MAX_PLAYER );

        if ( SDLKEYDOWN( SDLK_1 ) )  docheat = 0;
        else if ( SDLKEYDOWN( SDLK_2 ) )  docheat = 1;
        else if ( SDLKEYDOWN( SDLK_3 ) )  docheat = 2;
        else if ( SDLKEYDOWN( SDLK_4 ) )  docheat = 3;

        // Apply the cheat if valid
        if ( VALID_PLA( docheat ) )
        {
            ego_player * ppla = PlaStack + docheat;

            if ( INGAME_CHR( ppla->index ) )
            {
                // Heal 1 life
                heal_character( ppla->index, ppla->index, 256, btrue );
                stat_check_delay = 1;
            }
        }
    }

    // Display armor stats?
    if ( SDLKEYDOWN( SDLK_LSHIFT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_armor( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_armor( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_armor( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_armor( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_armor( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_armor( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_armor( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_armor( 7 ); stat_check_delay = 1000; }
    }

    // Display enchantment stats?
    else if ( SDLKEYDOWN( SDLK_LCTRL ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_full_status( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_full_status( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_full_status( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_full_status( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_full_status( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_full_status( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_full_status( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_full_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character special powers?
    else if ( SDLKEYDOWN( SDLK_LALT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_magic_status( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_magic_status( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_magic_status( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_magic_status( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_magic_status( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_magic_status( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_magic_status( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_magic_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character stats?
    else
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_stat( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_stat( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_stat( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_stat( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_stat( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_stat( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_stat( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_stat( 7 ); stat_check_delay = 1000; }
    }
}

//--------------------------------------------------------------------------------------------
void show_stat( int statindex )
{
    /// @details ZZ@> This function shows the more specific stats for a character

    CHR_REF character;
    int     level;
    char    gender[8] = EMPTY_CSTR;

    if ( statindex < StatList.count )
    {
        character = StatList[statindex];

        if ( INGAME_CHR( character ) )
        {
            ego_cap * pcap;
            ego_chr * pchr = ChrObjList.get_data_ptr( character );

            pcap = pro_get_pcap( pchr->profile_ref );

            // Name
            debug_printf( "=%s=", ego_chr::get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_CAPITAL ) );

            // Level and gender and class
            gender[0] = 0;
            if ( pchr->alive )
            {
                int itmp;
                const char * gender_str;

                gender_str = ego_chr::get_gender_name( character, NULL, 0 );

                level = 1 + pchr->experience_level;
                itmp = level % 10;
                if ( 1 == itmp )
                {
                    debug_printf( "~%dst level %s%s", level, gender_str, pcap->classname );
                }
                else if ( 2 == itmp )
                {
                    debug_printf( "~%dnd level %s%s", level, gender_str, pcap->classname );
                }
                else if ( 3 == itmp )
                {
                    debug_printf( "~%drd level %s%s", level, gender_str, pcap->classname );
                }
                else
                {
                    debug_printf( "~%dth level %s%s", level, gender_str, pcap->classname );
                }
            }
            else
            {
                debug_printf( "~Dead %s", pcap->classname );
            }

            // Stats
            debug_printf( "~STR:~%2d~WIS:~%2d~DEF:~%d", SFP8_TO_SINT( pchr->strength ), SFP8_TO_SINT( pchr->wisdom ), 255 - pchr->defense );
            debug_printf( "~INT:~%2d~DEX:~%2d~EXP:~%d", SFP8_TO_SINT( pchr->intelligence ), SFP8_TO_SINT( pchr->dexterity ), pchr->experience );
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_armor( int statindex )
{
    /// @details ZF@> This function shows detailed armor information for the character

    STRING tmps;
    CHR_REF ichr;

    Uint8  skinlevel;

    ego_cap * pcap;
    ego_chr * pchr;

    if ( statindex >= StatList.count ) return;

    ichr = StatList[statindex];
    if ( !INGAME_CHR( ichr ) ) return;

    pchr = ChrObjList.get_data_ptr( ichr );
    skinlevel = pchr->skin;

    pcap = ego_chr::get_pcap( ichr );
    if ( NULL == pcap ) return;

    // Armor Name
    debug_printf( "=%s=", pcap->skinname[skinlevel] );

    // Armor Stats
    debug_printf( "~DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d", 255 - pcap->defense[skinlevel],
                  GET_DAMAGE_RESIST( pcap->damagemodifier[DAMAGE_SLASH][skinlevel] ),
                  GET_DAMAGE_RESIST( pcap->damagemodifier[DAMAGE_CRUSH][skinlevel] ),
                  GET_DAMAGE_RESIST( pcap->damagemodifier[DAMAGE_POKE ][skinlevel] ) );

    debug_printf( "~HOLY:~%i~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP:~%i",
                  GET_DAMAGE_RESIST( pcap->damagemodifier[DAMAGE_HOLY][skinlevel] ),
                  GET_DAMAGE_RESIST( pcap->damagemodifier[DAMAGE_EVIL][skinlevel] ),
                  GET_DAMAGE_RESIST( pcap->damagemodifier[DAMAGE_FIRE][skinlevel] ),
                  GET_DAMAGE_RESIST( pcap->damagemodifier[DAMAGE_ICE ][skinlevel] ),
                  GET_DAMAGE_RESIST( pcap->damagemodifier[DAMAGE_ZAP ][skinlevel] ) );

    debug_printf( "~Type: %s", ( pcap->skindressy & ( 1 << skinlevel ) ) ? "Light Armor" : "Heavy Armor" );

    // jumps
    tmps[0] = CSTR_END;
    switch ( pcap->jump_number )
    {
        case 0:  SDL_snprintf( tmps, SDL_arraysize( tmps ), "None    (%i)", pchr->jump_number_reset ); break;
        case 1:  SDL_snprintf( tmps, SDL_arraysize( tmps ), "Novice  (%i)", pchr->jump_number_reset ); break;
        case 2:  SDL_snprintf( tmps, SDL_arraysize( tmps ), "Skilled (%i)", pchr->jump_number_reset ); break;
        case 3:  SDL_snprintf( tmps, SDL_arraysize( tmps ), "Adept   (%i)", pchr->jump_number_reset ); break;
        default: SDL_snprintf( tmps, SDL_arraysize( tmps ), "Master  (%i)", pchr->jump_number_reset ); break;
    };

    debug_printf( "~Speed:~%3.0f~Jump Skill:~%s", pchr->maxaccel_reset*80, tmps );
}

//--------------------------------------------------------------------------------------------
bool_t get_chr_regeneration( ego_chr * pchr, int * pliferegen, int * pmanaregen )
{
    /// @details ZF@> Get a character's life and mana regeneration, considering all sources

    int local_liferegen, local_manaregen;
    CHR_REF ichr;

    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;
    ichr = GET_REF_PCHR( pchr );

    if ( NULL == pliferegen ) pliferegen = &local_liferegen;
    if ( NULL == pmanaregen ) pmanaregen = &local_manaregen;

    // set the base values
    ( *pmanaregen ) = pchr->mana_return / MANARETURNSHIFT;
    ( *pliferegen ) = pchr->life_return;

    // Don't forget to add gains and costs from enchants
    ENC_BEGIN_LOOP_ACTIVE( enchant, penc )
    {
        if ( penc->target_ref == ichr )
        {
            ( *pliferegen ) += penc->target_life;
            ( *pmanaregen ) += penc->target_mana;
        }

        if ( penc->owner_ref == ichr )
        {
            ( *pliferegen ) += penc->owner_life;
            ( *pmanaregen ) += penc->owner_mana;
        }
    }
    ENC_END_LOOP();

    return btrue;
}

//--------------------------------------------------------------------------------------------
void show_full_status( int statindex )
{
    /// @details ZF@> This function shows detailed armor information for the character including magic

    CHR_REF character;
    int manaregen, liferegen;
    ego_chr * pchr;

    if ( statindex >= StatList.count ) return;

    character = StatList[statindex];
    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return;

    // clean up the enchant list
    cleanup_character_enchants( pchr );

    // Enchanted?
    debug_printf( "=%s is %s=", ego_chr::get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ), INGAME_ENC( pchr->firstenchant ) ? "enchanted" : "unenchanted" );

    // Armor Stats
    debug_printf( "~DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d",
                  255 - pchr->defense,
                  GET_DAMAGE_RESIST( pchr->damagemodifier[DAMAGE_SLASH] ),
                  GET_DAMAGE_RESIST( pchr->damagemodifier[DAMAGE_CRUSH] ),
                  GET_DAMAGE_RESIST( pchr->damagemodifier[DAMAGE_POKE ] ) );

    debug_printf( "~HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
                  GET_DAMAGE_RESIST( pchr->damagemodifier[DAMAGE_HOLY] ),
                  GET_DAMAGE_RESIST( pchr->damagemodifier[DAMAGE_EVIL] ),
                  GET_DAMAGE_RESIST( pchr->damagemodifier[DAMAGE_FIRE] ),
                  GET_DAMAGE_RESIST( pchr->damagemodifier[DAMAGE_ICE ] ),
                  GET_DAMAGE_RESIST( pchr->damagemodifier[DAMAGE_ZAP ] ) );

    get_chr_regeneration( pchr, &liferegen, &manaregen );

    debug_printf( "Mana Regen:~%4.2f Life Regen:~%4.2f", SFP8_TO_FLOAT( manaregen ), SFP8_TO_FLOAT( liferegen ) );
}

//--------------------------------------------------------------------------------------------
void show_magic_status( int statindex )
{
    /// @details ZF@> Displays special enchantment effects for the character

    CHR_REF character;
    const char * missile_str;
    ego_chr * pchr;

    if ( statindex >= StatList.count ) return;

    character = StatList[statindex];

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return;

    // clean up the enchant list
    cleanup_character_enchants( pchr );

    // Enchanted?
    debug_printf( "=%s is %s=", ego_chr::get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ), INGAME_ENC( pchr->firstenchant ) ? "enchanted" : "unenchanted" );

    // Enchantment status
    debug_printf( "~See Invisible: %s~~See Kurses: %s",
                  pchr->see_invisible_level ? "Yes" : "No",
                  pchr->see_kurse_level     ? "Yes" : "No" );

    debug_printf( "~Channel Life: %s~~Waterwalking: %s",
                  pchr->canchannel ? "Yes" : "No",
                  pchr->waterwalk ? "Yes" : "No" );

    switch ( pchr->missiletreatment )
    {
        case MISSILE_REFLECT: missile_str = "Reflect"; break;
        case MISSILE_DEFLECT: missile_str = "Deflect"; break;

        default:
        case MISSILE_NORMAL : missile_str = "None";    break;
    }

    debug_printf( "~Flying: %s~~Missile Protection: %s", IS_FLYING_PCHR( pchr ) ? "Yes" : "No", missile_str );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
    /// @details ZZ@> This function sets all of the character's starting tilt values

    Uint8 twist;

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        if ( !INGAME_CHR( cnt ) ) continue;

        if ( pchr->stickybutt && ego_mpd::grid_is_valid( PMesh, pchr->onwhichgrid ) )
        {
            twist = PMesh->gmem.grid_list[pchr->onwhichgrid].twist;
            pchr->ori.map_facing_y = map_twist_y[twist];
            pchr->ori.map_facing_x = map_twist_x[twist];
        }
        else
        {
            pchr->ori.map_facing_y = MAP_TURN_OFFSET;
            pchr->ori.map_facing_x = MAP_TURN_OFFSET;
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void import_dir_profiles_vfs( const char * dirname )
{
    STRING newloadname;
    STRING filename;
    int cnt;

    if ( NULL == PMod || INVALID_CSTR( dirname ) ) return;

    if ( !PMod->importvalid || 0 == PMod->importamount ) return;

    for ( cnt = 0; cnt < PMod->importamount*MAXIMPORTPERPLAYER; cnt++ )
    {
        // Make sure the object exists...
        SDL_snprintf( filename, SDL_arraysize( filename ), "%s/temp%04d.obj", dirname, cnt );
        SDL_snprintf( newloadname, SDL_arraysize( newloadname ), "%s/data.txt", filename );

        if ( vfs_exists( newloadname ) )
        {
            // new player found
            if ( 0 == ( cnt % MAXIMPORTPERPLAYER ) ) import_data.player++;

            // store the slot info
            import_data.slot = cnt;

            // load it
            import_data.slot_lst[cnt] = load_one_profile_vfs( filename, MAX_PROFILE );
            import_data.max_slot      = SDL_max( import_data.max_slot, cnt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void load_all_profiles_import()
{
    int cnt;

    // Clear the import slots...
    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        import_data.slot_lst[cnt] = 10000;
    }
    import_data.max_slot = -1;

    // This overwrites existing loaded slots that are loaded globally
    overrideslots = btrue;
    import_data.player = -1;
    import_data.slot   = -100;

    import_dir_profiles_vfs( "mp_import" );
    import_dir_profiles_vfs( "mp_remote" );
}

//--------------------------------------------------------------------------------------------
void game_load_module_profiles( const char *modname )
{
    /// @details BB@> Search for .obj directories int the module directory and load them

    vfs_search_context_t * ctxt;
    const char *filehandle;
    STRING newloadname;

    import_data.slot = -100;
    make_newloadname( modname, "objects", newloadname );

    ctxt = vfs_findFirst( newloadname, "obj", VFS_SEARCH_DIR );
    filehandle = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( filehandle ) )
    {
        load_one_profile_vfs( filehandle, MAX_PROFILE );

        ctxt = vfs_findNext( &ctxt );
        filehandle = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
void game_load_global_profiles()
{
    // load all special objects
    load_one_profile_vfs( "mp_data/globalobjects/book.obj", SPELLBOOK );

    // load the objects from various import directories
    load_all_profiles_import();
}

//--------------------------------------------------------------------------------------------
void game_load_all_profiles( const char *modname )
{
    /// @details ZZ@> This function loads a module's local objects and overrides the global ones already loaded

    // Log all of the script errors
    parseerror = bfalse;

    // clear out any old object definitions
    release_all_pro();

    // load the global objects
    game_load_global_profiles();

    // load the objects from the module's directory
    game_load_module_profiles( modname );
}

//--------------------------------------------------------------------------------------------
bool_t chr_setup_apply( const CHR_REF & ichr, spawn_file_info_t *pinfo )
{
    ego_chr * pchr, *pparent;
    ego_ai_bundle tmp_bdl_ai;

    // trap bad pointers
    if ( NULL == pinfo ) return bfalse;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    pparent = NULL;
    if ( INGAME_CHR( CHR_REF( pinfo->parent ) ) ) pparent = ChrObjList.get_data_ptr( CHR_REF( pinfo->parent ) );

    pchr->money += pinfo->money;
    if ( pchr->money > MAXMONEY )  pchr->money = MAXMONEY;
    if ( pchr->money < 0 )  pchr->money = 0;

    pchr->ai.content = pinfo->content;
    pchr->ai.passage = pinfo->passage;

    if ( pinfo->attach == ATTACH_INVENTORY )
    {
        // Inventory character
        chr_inventory_add_item( ichr, CHR_REF( pinfo->parent ) );

        ADD_BITS( pchr->ai.alert, ALERTIF_GRABBED );  // Make spellbooks change
        pchr->attachedto = pinfo->parent;  // Make grab work

        scr_run_chr_script( ego_ai_bundle::set( &tmp_bdl_ai, pchr ) );  // Empty the grabbed messages

        pchr->attachedto = CHR_REF( MAX_CHR );  // Fix grab

    }
    else if ( pinfo->attach == ATTACH_LEFT || pinfo->attach == ATTACH_RIGHT )
    {
        // Wielded character
        grip_offset_t grip_off = ( ATTACH_LEFT == pinfo->attach ) ? GRIP_LEFT : GRIP_RIGHT;
        attach_character_to_mount( ichr, CHR_REF( pinfo->parent ), grip_off );

        // Handle the "grabbed" messages
        scr_run_chr_script( ego_ai_bundle::set( &tmp_bdl_ai, pchr ) );
    }

    // Set the starting pinfo->level
    if ( pinfo->level > 0 )
    {
        while ( signed( pchr->experience_level ) < pinfo->level && pchr->experience < MAX_XP )
        {
            give_experience( ichr, 25, XP_DIRECT, btrue );
            do_level_up( ichr );
        }
    }

    // automatically identify and unkurse all player starting equipment? I think yes.
    if ( start_new_player && NULL != pparent && VALID_PLA( pparent->is_which_player ) )
    {
        ego_chr *pitem;
        pchr->nameknown = btrue;

        // Unkurse both inhand items
        if ( INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
        {
            pitem = ChrObjList.get_data_ptr( ichr );
            pitem->iskursed = bfalse;
        }
        if ( INGAME_CHR( pchr->holdingwhich[SLOT_RIGHT] ) )
        {
            pitem = ChrObjList.get_data_ptr( ichr );
            pitem->iskursed = bfalse;
        }

    }

    // adjust the price of items that are spawned in a shop

    return btrue;
}

//--------------------------------------------------------------------------------------------
// gcc does not define this function on linux (at least not Ubuntu),
// but it is defined under MinGW, which is yucky.
// I actually had to spend like 45 minutes looking up the compiler flags
// to catch this... good documentation, guys!
//#if defined(__GNUC__) && !(defined (__MINGW) || defined(__MINGW32__))
//int EGO_strlwr( char * str )
//{
//    if ( NULL == str ) return -1;
//
//    while ( CSTR_END != *str )
//    {
//        *str = SDL_tolower( *str );
//        str++;
//    }
//
//    return 0;
//}
//#endif

//--------------------------------------------------------------------------------------------
bool_t activate_spawn_file_load_object( spawn_file_info_t * psp_info )
{
    /// @details BB@> Try to load a global object named int psp_info->spawn_coment into
    ///               slot psp_info->slot

    STRING filename;
    PRO_REF ipro;

    if ( NULL == psp_info || psp_info->slot < 0 ) return bfalse;

    ipro = psp_info->slot;
    if ( LOADED_PRO( ipro ) ) return bfalse;

    // trim any excess spaces off the psp_info->spawn_coment
    str_trim( psp_info->spawn_coment );

    if ( NULL == SDL_strstr( psp_info->spawn_coment, ".obj" ) )
    {
        strcat( psp_info->spawn_coment, ".obj" );
    }

    SDL_strlwr( psp_info->spawn_coment );

    // do the loading
    if ( CSTR_END != psp_info->spawn_coment[0] )
    {
        // we are relying on the virtual mount point "mp_objects", so use
        // the vfs/PHYSFS file naming conventions
        SDL_snprintf( filename, SDL_arraysize( filename ), "mp_objects/%s", psp_info->spawn_coment );

        psp_info->slot = load_one_profile_vfs( filename, psp_info->slot );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t activate_spawn_file_spawn( spawn_file_info_t * psp_info )
{
    int     tnc, local_index = 0;
    CHR_REF new_object;
    PRO_REF iprofile;
    bool_t  activated = bfalse;

    if ( NULL == psp_info || !psp_info->do_spawn || psp_info->slot < 0 ) return bfalse;

    iprofile = PRO_REF( psp_info->slot );

    // Spawn the character
    new_object = spawn_one_character( psp_info->pos, iprofile, TEAM_REF( psp_info->team ), psp_info->skin, psp_info->facing, psp_info->pname, CHR_REF( MAX_CHR ) );

    // grab a pointer to the ego_obj_chr, if it was allocated
    ego_obj_chr * pobject = ChrObjList.get_allocated_data_ptr( new_object );
    if ( NULL == pobject ) goto activate_spawn_file_spawn_exit;

    // is the object in the corect state?
    if ( !DEFINED_PCHR( pobject ) ) goto activate_spawn_file_spawn_exit;

    // determine the attachment
    if ( psp_info->attach == ATTACH_NONE )
    {
        // Free character
        psp_info->parent = new_object.get_value();
        make_one_character_matrix( new_object );
    }

    // apply the setup information
    activated = chr_setup_apply( new_object, psp_info );

    // Turn on PlaStack.count input devices
    if ( psp_info->stat )
    {
        // what we do depends on what kind of module we're loading
        if ( 0 == PMod->importamount && PlaStack.count < PMod->playeramount )
        {
            // a single player module

            bool_t player_added;

            player_added = bfalse;
            if ( 0 == net_stats.pla_count_local )
            {
                // the first player gets everything
                player_added = add_player( new_object, PLA_REF( PlaStack.count ), ( Uint32 )( ~0 ) );
            }
            else
            {
                PLA_REF ipla;
                BIT_FIELD bits;

                // each new player steals an input device from the 1st player
                bits = 1 << net_stats.pla_count_local;
                for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
                {
                    REMOVE_BITS( PlaStack[ipla].device.bits, bits );
                }

                player_added = add_player( new_object, PLA_REF( PlaStack.count ), bits );
            }

            if ( start_new_player && player_added )
            {
                // !!!! make sure the player is identified !!!!
                pobject->nameknown = btrue;
            }
        }
        else if ( PlaStack.count < PMod->importamount && PlaStack.count < PMod->playeramount && PlaStack.count < local_import.count )
        {
            // A multiplayer module

            bool_t player_added;

            local_index = -1;
            for ( tnc = 0; tnc < local_import.count; tnc++ )
            {
                if ( pobject->profile_ref <= import_data.max_slot && pobject->profile_ref < MAX_PROFILE )
                {
                    int islot = ( pobject->profile_ref ).get_value();

                    if ( import_data.slot_lst[islot] == local_import.slot[tnc] )
                    {
                        local_index = tnc;
                        break;
                    }
                }
            }

            player_added = bfalse;
            if ( -1 != local_index )
            {
                // It's a local PlaStack.count
                player_added = add_player( new_object, PLA_REF( PlaStack.count ), local_import.control[local_index] );
            }
            else
            {
                // It's a remote PlaStack.count
                player_added = add_player( new_object, PLA_REF( PlaStack.count ), INPUT_BITS_NONE );
            }

            // if for SOME REASON your player is not identified, give him
            // about a 50% chance to get identified every time you enter a module
            if ( player_added && !pobject->nameknown )
            {
                float frand = rand() / ( float )RAND_MAX;

                if ( frand > 0.5f )
                {
                    pobject->nameknown = btrue;
                }
            }
        }

        // Turn on the stat display
        if ( StatList.add( new_object ) )
        {
            pobject->draw_stats = btrue;
        }
    }

activate_spawn_file_spawn_exit:

    // This object is done spawning
    POBJ_END_SPAWN( pobject );

    return activated;
}

//--------------------------------------------------------------------------------------------
void activate_spawn_file_vfs()
{
    /// @details ZZ@> This function sets up character data, loaded from "SPAWN.TXT"

    const char       *newloadname;
    vfs_FILE         *fileread;
    spawn_file_info_t sp_info;

    // Turn some back on
    newloadname = "mp_data/spawn.txt";
    fileread = vfs_openRead( newloadname );

    PlaStack.count = 0;
    sp_info.parent = MAX_CHR;
    if ( NULL == fileread )
    {
        log_error( "Cannot read file: %s\n", newloadname );
    }
    else
    {
        sp_info.parent = MAX_CHR;
        while ( spawn_file_scan( fileread, &sp_info ) )
        {
            int save_slot = sp_info.slot;

            // check to see if the slot is valid
            if ( -1 == sp_info.slot || sp_info.slot >= MAX_PROFILE )
            {
                log_warning( "Invalid slot %d for \"%s\" in file \"%s\"\n", sp_info.slot, sp_info.spawn_coment, newloadname );
                continue;
            }

            // If nothing is in that slot, try to load it
            if ( sp_info.slot >= 0 && !LOADED_PRO( PRO_REF( sp_info.slot ) ) )
            {
                activate_spawn_file_load_object( &sp_info );
            }

            // do we have a valid profile, yet?
            if ( sp_info.slot >= 0 && !LOADED_PRO( PRO_REF( sp_info.slot ) ) )
            {
                // no, give a warning if it is useful
                if ( save_slot > PMod->importamount * MAXIMPORTPERPLAYER )
                {
                    log_warning( "The object \"%s\"(slot %d) in file \"%s\" does not exist on this machine\n", sp_info.spawn_coment, save_slot, newloadname );
                }
            }
            else
            {
                // yes, do the spawning if need be
                activate_spawn_file_spawn( &sp_info );
            }
        }

        vfs_close( fileread );
    }

    clear_messages();

    // Make sure local players are displayed first
    sort_stat_list();

    // Fix tilting trees problem
    tilt_characters_to_terrain();
}

//--------------------------------------------------------------------------------------------
void load_all_global_objects()
{
    /// @details ZF@> This function loads all global objects found on the mp_data mount point

    vfs_search_context_t * ctxt;
    const char *filehandle;

    // Warn the user for any duplicate slots
    overrideslots = bfalse;

    // Search for .obj directories and load them
    ctxt = vfs_findFirst( "mp_data/globalobjects", "obj", VFS_SEARCH_DIR );
    filehandle = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( filehandle ) )
    {
        load_one_profile_vfs( filehandle, MAX_PROFILE );

        ctxt = vfs_findNext( &ctxt );
        filehandle = vfs_search_context_get_current( ctxt );
    }

    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
void game_reset_module_data()
{
    // reset all
    log_info( "Resetting module data\n" );

    // unload a lot of data
    reset_teams();
    release_all_profiles();
    free_all_objects();
    reset_messages();
    chop_data_init( &chop_mem );
    game_reset_players();

    reset_end_text();
    renderlist_reset();
}

//--------------------------------------------------------------------------------------------
void game_load_global_assets()
{
    // load a bunch of assets that are used in the module

    // Load all in-game resources
    load_all_global_icons();
    load_blips();
    load_bars();

    // the in-game bitmap font
    font_bmp_load_vfs( "mp_data/font", "mp_data/font.txt" );

    // the cursor for the in-game menus
    load_cursor();
}

//--------------------------------------------------------------------------------------------
void game_load_module_assets( const char *modname )
{
    // load a bunch of assets that are used in the module
    load_global_waves();
    reset_particles();

    if ( NULL == read_wawalite() )
    {
        log_warning( "wawalite.txt not loaded for %s.\n", modname );
    }

    load_basic_textures();
    load_map();

    upload_wawalite();
}

//--------------------------------------------------------------------------------------------
void game_load_all_assets( const char *modname )
{
    game_load_global_assets();

    game_load_module_assets( modname );
}

//--------------------------------------------------------------------------------------------
void game_setup_module( const char *smallname )
{
    /// @details ZZ@> This runs the setup functions for a module

    STRING modname;

    // make sure the object lists are empty
    free_all_objects();

    // generate the module directory
    strncpy( modname, smallname, SDL_arraysize( modname ) );
    str_append_slash_net( modname, SDL_arraysize( modname ) );

    // ust the information in these files to load the module
    activate_passages_file_vfs();        // read and implement the "passage script" passages.txt
    activate_spawn_file_vfs();           // read and implement the "spawn script" spawn.txt
    activate_alliance_file_vfs();        // set up the non-default team interactions
}

//--------------------------------------------------------------------------------------------
bool_t game_load_module_data( const char *smallname )
{
    /// @details ZZ@> This function loads a module

    STRING modname;

    log_info( "Loading module \"%s\"\n", smallname );

    if ( load_ai_script_vfs( "mp_data/script.txt" ) < 0 )
    {
        log_warning( "game_load_module_data() - cannot load the default script\n" );
        goto game_load_module_data_fail;
    }

    // generate the module directory
    strncpy( modname, smallname, SDL_arraysize( modname ) );
    str_append_slash( modname, SDL_arraysize( modname ) );

    // load all module assets
    game_load_all_assets( modname );

    // load all module objects
    game_load_all_profiles( modname );

    if ( NULL == mesh_load( modname, PMesh ) )
    {
        // do not cause the program to fail, in case we are using a script function to load a module
        // just return a failure value and log a warning message for debugging purposes
        log_warning( "game_load_module_data() - Uh oh! Problems loading the mesh! (%s)\n", modname );

        goto game_load_module_data_fail;
    }

    return btrue;

game_load_module_data_fail:

    // release any data that might have been allocated
    game_release_module_data();

    // we are headed back to the menu, so make sure that we have a cursor for the ui
    load_cursor();

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( const CHR_REF & character )
{
    /// @details ZZ@> This function makes sure a character has no attached particles

    PRT_BEGIN_LOOP_ACTIVE( iprt, prt_bdl )
    {
        if ( prt_bdl.prt_ptr->attachedto_ref == character )
        {
            ego_obj_prt::request_terminate( &prt_bdl );
        }
    }
    PRT_END_LOOP();

    if ( INGAME_CHR( character ) )
    {
        // Set the alert for disaffirmation ( wet torch )
        ADD_BITS( ChrObjList.get_data_ref( character ).ai.alert, ALERTIF_DISAFFIRMED );
    }
}

//--------------------------------------------------------------------------------------------
int number_of_attached_particles( const CHR_REF & character )
{
    /// @details ZZ@> This function returns the number of particles attached to the given character

    int     cnt = 0;

    PRT_BEGIN_LOOP_ACTIVE( iprt, prt_bdl )
    {
        if ( prt_bdl.prt_ptr->attachedto_ref == character )
        {
            cnt++;
        }
    }
    PRT_END_LOOP();

    return cnt;
}

//--------------------------------------------------------------------------------------------
int reaffirm_attached_particles( const CHR_REF & character )
{
    /// @details ZZ@> This function makes sure a character has all of its particles

    int     number_added, number_attached;
    int     amount, attempts;
    PRT_REF particle;
    ego_chr * pchr;
    ego_cap * pcap;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return 0;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return 0;
    amount = pcap->attachedprt_amount;

    if ( 0 == amount ) return 0;

    number_attached = number_of_attached_particles( character );
    if ( number_attached >= amount ) return 0;

    number_added = 0;
    for ( attempts = 0; attempts < amount && number_attached < amount; attempts++ )
    {
        particle = spawn_one_particle( pchr->pos, 0, pchr->profile_ref, pcap->attachedprt_pip, character, GRIP_LAST + number_attached, ego_chr::get_iteam( character ), character, PRT_REF( MAX_PRT ), number_attached, CHR_REF( MAX_CHR ) );

        ego_prt * pprt = PrtObjList.get_allocated_data_ptr( particle );
        if ( NULL != pprt )
        {
            pprt = place_particle_at_vertex( pprt, character, pprt->attachedto_vrt_off );
            if ( NULL == pprt ) continue;

            number_added++;
            number_attached++;
        }
    }

    // Set the alert for reaffirmation ( for exploding barrels with fire )
    ADD_BITS( pchr->ai.alert, ALERTIF_REAFFIRMED );

    return number_added;
}

//--------------------------------------------------------------------------------------------
void game_quit_module()
{
    /// @details BB@> all of the de-initialization code after the module actually ends

    // stop the module
    game_module_stop( PMod );

    // get rid of the game/module data
    game_release_module_data();

    // turn off networking
    close_session();

    // reset the "ui" mouse state
    load_cursor();

    // make sure that we have a cursor for the ui
    cursor_reset();
    load_cursor();

    // re-initialize all game/module data
    game_reset_module_data();

    // finish whatever in-game song is playing
    sound_finish_sound();
    sound_play_song( MENU_SONG, 0, -1 );

    // remove the module-dependent mount points from the vfs
    game_clear_vfs_paths();
}

//--------------------------------------------------------------------------------------------
void game_clear_vfs_paths()
{
    /// @details BB@> clear out the all mount points

    // clear out the basic mount points
    egoboo_clear_vfs_paths();

    // clear out the module's mount points
    vfs_remove_mount_point( "mp_objects" );

    // set up the basic mount points again
    egoboo_setup_vfs_paths();
}

//--------------------------------------------------------------------------------------------
bool_t game_setup_vfs_paths( const char * mod_path )
{
    /// @details BB@> set up the virtual mount points for the module's data
    ///               and objects

    const char * path_seperator_1, * path_seperator_2;
    const char * mod_dir_ptr;
    STRING mod_dir_string;

    STRING tmpDir;

    if ( INVALID_CSTR( mod_path ) ) return bfalse;

    // revert to the program's basic mount points
    game_clear_vfs_paths();

    path_seperator_1 = SDL_strrchr( mod_path, SLASH_CHR );
    path_seperator_2 = SDL_strrchr( mod_path, NET_SLASH_CHR );
    path_seperator_1 = SDL_max( path_seperator_1, path_seperator_2 );

    if ( NULL == path_seperator_1 )
    {
        mod_dir_ptr = mod_path;
    }
    else
    {
        mod_dir_ptr = path_seperator_1 + 1;
    }

    strncpy( mod_dir_string, mod_dir_ptr, SDL_arraysize( mod_dir_string ) );

    //==== set the module-dependent mount points

    //---- add the "/modules/*.mod/objects" directories to mp_objects
    SDL_snprintf( tmpDir, sizeof( tmpDir ), "modules" SLASH_STR "%s" SLASH_STR "objects", mod_dir_string );

    // Mount the user's module objects directory at the BEGINNING of the mount point list.
    // This will allow this directory to override all other directories
    vfs_add_mount_point( fs_getDataDirectory(), tmpDir, "mp_objects", 1 );

    // The global module objects directory is next, which will override all default objects
    vfs_add_mount_point( fs_getUserDirectory(), tmpDir, "mp_objects", 1 );

    //---- add the "/basicdat/globalobjects/*" directories to mp_objects
    // The global objects are last. The order should not matter, since the objects should be unique
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "items",            "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "magic",            "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "magic_item",       "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "misc",             "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "monsters",         "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "players",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "potions",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "unique",           "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "weapons",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "work_in_progress", "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "traps",            "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "pets",             "mp_objects", 1 );

    //---- add the "/modules/*.mod/gamedat" directory to mp_data
    SDL_snprintf( tmpDir, sizeof( tmpDir ), "modules" SLASH_STR "%s" SLASH_STR "gamedat",  mod_dir_string );

    // mount the global module gamedat directory before the global basicdat directory
    // This will allow files in the global gamedat directory to override the basicdat directory
    vfs_add_mount_point( fs_getDataDirectory(), tmpDir, "mp_data", 0 );

    // mount the global module gamedat directory before the global module gamedat and before the global basicdat directory
    // This will allow files in the local gamedat directory to override the other two directories
    vfs_add_mount_point( fs_getUserDirectory(), tmpDir, "mp_data", 0 );

    // put the global globalparticles data after the module gamedat data
    // this will allow the module's gamedat particles to override the globalparticles
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalparticles", "mp_data", 1 );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t game_begin_module( const char * modname, Uint32 seed )
{
    /// @details BB@> all of the initialization code before the module actually starts

    if ((( Uint32 )( ~0 ) ) == seed ) seed = time( NULL );

    // make sure the old game has been quit
    game_quit_module();

    reset_timers();

    // set up the virtual file system for the module
    if ( !game_setup_vfs_paths( modname ) ) return bfalse;

    // load all the in-game module data
    srand( seed );
    if ( !game_load_module_data( modname ) )
    {
        game_module_stop( PMod );
        return bfalse;
    };

    game_setup_module( modname );

    // make sure the per-module configuration settings are correct
    ego_config_data::synch( &ego_cfg );

    // initialize the game objects
    initialize_all_objects();
    cursor_reset();
    game_module_reset( PMod, seed );
    ego_camera::reset( PCamera, PMesh );
    make_all_character_matrices( update_wld != 0 );
    attach_all_particles();

    // log debug info for every object loaded into the module
    if ( cfg.dev_mode ) log_madused_vfs( "/debug/slotused.txt" );

    // initialize the network
    network_system_begin( &ego_cfg );
    net_sayHello();

    // start the module
    game_module_start( PMod );

    // initialize the timers as the very last thing
    timeron = bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
egoboo_rv game_update_imports()
{
    /// @details BB@> This function copies all the players into the imports dir to prepare for the next module
    bool_t is_local;
    int tnc, j;
    CHR_REF character;
    PLA_REF player, ipla;
    STRING srcPlayer, srcDir, destDir;
    egoboo_rv retval = rv_success;

    // reload all of the available players
    mnu_player_check_import( "mp_players", btrue );
    mnu_player_check_import( "mp_remote",  bfalse );

    // build the import directory using the player info
    vfs_empty_import_directory();
    if ( !vfs_mkdir( "/import" ) )
    {
        log_warning( "game_update_imports() - Could not create the import folder. (%s)\n", vfs_getError() );
    }

    // export all of the players directly from memory straight to the "import" dir
    for ( player = 0, ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        size_t     player_idx = MAX_PLAYER;
        ego_player * ppla       = PlaStack + ipla;

        if ( !ppla->valid ) continue;

        // Is it alive?
        character = ppla->index;
        if ( !INGAME_CHR( character ) ) continue;

        is_local = ( INPUT_BITS_NONE != ppla->device.bits );

        // find the saved copy of the players that are in memory right now
        for ( tnc = 0; tnc < loadplayer_count; tnc++ )
        {
            if ( 0 == strcmp( loadplayer[tnc].name, ChrObjList.get_data_ref( character ).name ) )
            {
                break;
            }
        }

        if ( tnc == loadplayer_count )
        {
            retval = rv_fail;
            log_warning( "game_update_imports() - cannot find exported file for \"%s\" (\"%s\") \n", ChrObjList.get_data_ref( character ).base_name, str_encode_path( ChrObjList.get_data_ref( character ).name ) ) ;
            continue;
        }

        // grab the controls from the currently loaded players
        // calculate the slot from the current player count
        player_idx = player.get_value();
        local_import.control[player_idx] = ppla->device.bits;
        local_import.slot[player_idx]    = player_idx * MAXIMPORTPERPLAYER;
        player++;

        // Copy the character to the import directory
        if ( is_local )
        {
            SDL_snprintf( srcPlayer, SDL_arraysize( srcPlayer ), "%s", loadplayer[tnc].dir );
        }
        else
        {
            SDL_snprintf( srcPlayer, SDL_arraysize( srcPlayer ), "mp_remote/%s", str_encode_path( loadplayer[tnc].name ) );
        }

        SDL_snprintf( destDir, SDL_arraysize( destDir ), "/import/temp%04d.obj", local_import.slot[tnc] );
        vfs_copyDirectory( srcPlayer, destDir );

        // Copy all of the character's items to the import directory
        for ( j = 0; j < MAXIMPORTOBJECTS; j++ )
        {
            SDL_snprintf( srcDir, SDL_arraysize( srcDir ), "%s/%d.obj", srcPlayer, j );
            SDL_snprintf( destDir, SDL_arraysize( destDir ), "/import/temp%04d.obj", local_import.slot[tnc] + j + 1 );

            vfs_copyDirectory( srcDir, destDir );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void game_release_module_data()
{
    /// @details ZZ@> This function frees up memory used by the module

    ego_mpd   * ptmp;

    // make sure that the object lists are cleared out
    free_all_objects();

    // deal with dynamically allocated game assets
    release_all_graphics();
    release_all_profiles();
    release_all_ai_scripts();

    // deal with the mesh
    passage_system_clear();

    ptmp = PMesh;
    ego_mpd::destroy( &ptmp );

    // restore the original statically allocated ego_mpd   header
    PMesh = _mesh + 0;
}

//--------------------------------------------------------------------------------------------
bool_t attach_one_particle( ego_bundle_prt * pbdl_prt )
{
    ego_prt * pprt;
    ego_chr * pchr;

    if ( NULL == pbdl_prt ) return bfalse;
    pprt = pbdl_prt->prt_ptr;

    if ( !INGAME_CHR( pbdl_prt->prt_ptr->attachedto_ref ) ) return bfalse;
    pchr = ChrObjList.get_data_ptr( pbdl_prt->prt_ptr->attachedto_ref );

    pprt = place_particle_at_vertex( pprt, pprt->attachedto_ref, pprt->attachedto_vrt_off );
    if ( NULL == pprt ) return bfalse;

    // the previous function can inactivate a particle
    if ( PROCESSING_PPRT( pprt ) )
    {
        // Correct facing so swords knock characters in the right direction...
        if ( NULL != pbdl_prt->pip_ptr && HAS_SOME_BITS( pbdl_prt->pip_ptr->damfx, DAMFX_TURN ) )
        {
            pprt->facing = pchr->ori.facing_z;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void attach_all_particles()
{
    /// @details ZZ@> This function attaches particles to their characters so everything gets
    ///    drawn right

    PRT_BEGIN_LOOP_USED( cnt, prt_bdl )
    {
        attach_one_particle( &prt_bdl );
    }
    PRT_END_LOOP()
}

//--------------------------------------------------------------------------------------------
bool_t add_player( const CHR_REF & character, const PLA_REF & player, BIT_FIELD device_bits )
{
    /// @details ZZ@> This function adds a player, returning bfalse if it fails, btrue otherwise

    ego_player * ppla = NULL;
    ego_chr    * pchr = NULL;

    if ( !PlaStack.in_range_ref( player ) ) return bfalse;
    ppla = PlaStack + player;

    // does the player already exist?
    if ( ppla->valid ) return bfalse;

    // re-construct the players
    ego_player::reinit( ppla );

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    // set the reference to the player
    pchr->is_which_player = player;

    // download the quest info
    quest_log_download_vfs( ppla->quest_log, SDL_arraysize( ppla->quest_log ), ego_chr::get_dir_name( character ) );

    //---- skeleton for using a ConfigFile to save quests
    // ppla->quest_file = quest_file_open( ego_chr::get_dir_name(character) );

    ppla->index       = character;
    ppla->valid       = btrue;
    ppla->device.bits = device_bits;

    if ( EMPTY_BIT_FIELD != device_bits )
    {
        pchr->islocalplayer = btrue;
        net_stats.pla_count_local++;

        // reset the camera
        ego_camera::reset_target( PCamera, PMesh );
    }

    PlaStack.count++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void let_all_characters_think()
{
    /// @details ZZ@> This function runs the ai scripts for all eligible objects

    static Uint32 last_update = ( Uint32 )( ~0 );

    // make sure there is only one script update per game update
    if ( update_wld == last_update ) return;
    last_update = update_wld;

    blip_count = 0;

    CHR_BEGIN_LOOP_ACTIVE( character, pchr )
    {
        bool_t is_crushed, is_cleanedup, gained_level, can_think;

        ego_ai_bundle bdl;

        if ( NULL == ego_ai_bundle::set( &bdl, pchr ) ) continue;

        // check for actions that must always be handled
        is_cleanedup = HAS_SOME_BITS( bdl.ai_state_ptr->alert, ALERTIF_CLEANEDUP );
        is_crushed   = HAS_SOME_BITS( bdl.ai_state_ptr->alert, ALERTIF_CRUSHED );
        gained_level = HAS_SOME_BITS( bdl.ai_state_ptr->alert, ALERTIF_LEVELUP );

        // let the script run sometimes even if the item is in your backpack
        can_think = !pchr->pack.is_packed || bdl.cap_ptr->isequipment;

        // only let dead/destroyed things think if they have been crushed/cleanedup or gained a level
        if (( pchr->alive && can_think ) || is_crushed || is_cleanedup || gained_level )
        {
            // Figure out alerts that weren't already set
            set_alerts( &bdl );

            // handle special cases
            if ( is_cleanedup )
            {
                // Cleaned up characters shouldn't be alert to anything else
                bdl.ai_state_ptr->alert = ALERTIF_CLEANEDUP;
            }
            else if ( is_crushed )
            {
                // Crushed characters shouldn't be alert to anything else
                bdl.ai_state_ptr->alert = ALERTIF_CRUSHED;
                bdl.ai_state_ptr->timer = update_wld + 1;
            }
            else if ( gained_level )
            {
                // Level up characters shouldn't be alert to anything else
                bdl.ai_state_ptr->alert = ALERTIF_LEVELUP;
            }

            scr_run_chr_script( &bdl );
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
bool_t game_begin_menu( ego_menu_process * mproc, which_menu_t which )
{
    if ( NULL == mproc ) return bfalse;

    if ( !mproc->running() )
    {
        GProc->menu_depth = mnu_get_menu_depth();
    }

    if ( mnu_begin_menu( which ) )
    {
        mproc->start( );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void game_end_menu( ego_menu_process * mproc )
{
    mnu_end_menu();

    if ( mnu_get_menu_depth() <= GProc->menu_depth )
    {
        mproc->resume();
        GProc->menu_depth = -1;
    }
}

//--------------------------------------------------------------------------------------------
void game_finish_module()
{
    // do the normal export to save all the player settings, do this before game_update_imports()
    export_all_players( btrue );

    // export all the local and remote characters
    game_update_imports();

    // quit the old module
    /// @note ZF@> I think this not needed because this is done elswhere?
    //game_quit_module();
}

//--------------------------------------------------------------------------------------------
void free_all_objects( void )
{
    /// @details BB@> free every instance of the three object types used in the game.

    PrtObjList.free_all();
    EncObjList.free_all();
    free_all_chraracters();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd   * set_PMesh( ego_mpd   * pmpd )
{
    ego_mpd   * pmpd_old = PMesh;

    PMesh = pmpd;

    return pmpd_old;
}

//--------------------------------------------------------------------------------------------
ego_camera * set_PCamera( ego_camera * pcam )
{
    ego_camera * pcam_old = PCamera;

    PCamera = pcam;

    // Matrix init stuff (from remove.c)
    rotmeshtopside    = (( float )sdl_scr.x / sdl_scr.y ) * CAM_ROTMESH_TOPSIDE / ( 1.33333f );
    rotmeshbottomside = (( float )sdl_scr.x / sdl_scr.y ) * CAM_ROTMESH_BOTTOMSIDE / ( 1.33333f );
    rotmeshup         = (( float )sdl_scr.x / sdl_scr.y ) * CAM_ROTMESH_UP / ( 1.33333f );
    rotmeshdown       = (( float )sdl_scr.x / sdl_scr.y ) * CAM_ROTMESH_DOWN / ( 1.33333f );

    return pcam_old;
}

//--------------------------------------------------------------------------------------------
float get_mesh_level( ego_mpd   * pmesh, float x, float y, bool_t waterwalk )
{
    /// @details ZZ@> This function returns the height of a point within a mesh fan, precise
    ///    If waterwalk is nonzero and the fan is watery, then the level returned is the
    ///    level of the water.

    float zdone;

    zdone = ego_mpd::get_level( pmesh, x, y );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = ego_mpd::get_tile( pmesh, x, y );

        if ( 0 != ego_mpd::test_fx( pmesh, tile, MPDFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
bool_t make_water( ego_water_instance * pinst, wawalite_water_t * pdata )
{
    /// @details ZZ@> This function sets up water movements

    int layer, frame, point, cnt;
    float temp;
    Uint8 spek;

    if ( NULL == pinst || NULL == pdata ) return bfalse;

    for ( layer = 0; layer < pdata->layer_count; layer++ )
    {
        pinst->layer[layer].tx.x = 0;
        pinst->layer[layer].tx.y = 0;

        for ( frame = 0; frame < MAXWATERFRAME; frame++ )
        {
            // Do first mode
            for ( point = 0; point < WATERPOINTS; point++ )
            {
                temp = SIN(( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( PI / 2 * layer / MAXWATERLAYER ) );
                pinst->layer_z_add[layer][frame][point] = temp * pdata->layer[layer].amp;
            }
        }
    }

    // Calculate specular highlights
    spek = 0;
    for ( cnt = 0; cnt < 256; cnt++ )
    {
        spek = 0;
        if ( cnt > pdata->spek_start )
        {
            temp = cnt - pdata->spek_start;
            temp = temp / ( 256 - pdata->spek_start );
            temp = temp * temp;
            spek = temp * pdata->spek_level;
        }

        // claforte> Probably need to replace this with a
        //           GL_DEBUG(glColor4f)(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
        if ( gfx.shading == GL_FLAT )
            pinst->spek[cnt] = 0;
        else
            pinst->spek[cnt] = spek;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_end_text()
{
    /// @details ZZ@> This function resets the end-module text

    endtext_carat = SDL_snprintf( endtext, SDL_arraysize( endtext ), "The game has ended..." );

    /*
    if ( PlaStack.count > 1 )
    {
        endtext_carat = SDL_snprintf( endtext, SDL_arraysize( endtext), "Sadly, they were never heard from again..." );
    }
    else
    {
        if ( PlaStack.count == 0 )
        {
            // No players???
            endtext_carat = SDL_snprintf( endtext, SDL_arraysize( endtext), "The game has ended..." );
        }
        else
        {
            // One player
            endtext_carat = SDL_snprintf( endtext, SDL_arraysize( endtext), "Sadly, no trace was ever found..." );
        }
    }
    */

    str_add_linebreaks( endtext, endtext_carat, 20 );
}

//--------------------------------------------------------------------------------------------
void expand_escape_codes( const CHR_REF & ichr, ego_script_state * pstate, char * src, char * src_end, char * dst, char * dst_end )
{
    int    cnt;
    STRING szTmp;

    ego_chr      * pchr, *ptarget, *powner;
    ego_cap      * pchr_cap, *ptarget_cap, *powner_cap;
    CHR_REF      itarget, iowner;
    ego_ai_state * pchr_ai;

    pchr     = !INGAME_CHR( ichr ) ? NULL : ChrObjList.get_data_ptr( ichr );
    pchr_ai  = ( NULL == pchr )    ? NULL : &( pchr->ai );
    pchr_cap = ego_chr::get_pcap( ichr );

    itarget     = ( NULL == pchr_ai ) ? CHR_REF( MAX_CHR ) : pchr_ai->target;
    ptarget     = !INGAME_CHR( itarget ) ? NULL : ChrObjList.get_data_ptr( itarget );
    ptarget_cap = ego_chr::get_pcap( itarget );

    iowner     = ( NULL == pchr_ai ) ? CHR_REF( MAX_CHR ) : pchr_ai->owner;
    powner     = !INGAME_CHR( iowner ) ? NULL : ChrObjList.get_data_ptr( iowner );
    powner_cap = ego_chr::get_pcap( iowner );

    cnt = 0;
    while ( CSTR_END != *src && src < src_end && dst < dst_end )
    {
        if ( '%' == *src )
        {
            char * ebuffer, * ebuffer_end;

            // go to the escape character
            src++;

            // set up the buffer to hold the escape data
            ebuffer     = szTmp;
            ebuffer_end = szTmp + SDL_arraysize( szTmp ) - 1;

            // make the escape buffer an empty string
            *ebuffer = CSTR_END;

            switch ( *src )
            {
                case '%' : // the % symbol
                    {
                        SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%%" );
                    }
                    break;

                case 'n' : // Name
                    {
                        SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%s", ego_chr::get_name( ichr, CHRNAME_ARTICLE ) );
                    }
                    break;

                case 'c':  // Class name
                    {
                        if ( NULL != pchr_cap )
                        {
                            ebuffer     = pchr_cap->classname;
                            ebuffer_end = ebuffer + SDL_arraysize( pchr_cap->classname );
                        }
                    }
                    break;

                case 't':  // Target name
                    {
                        SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%s", ego_chr::get_name( itarget, CHRNAME_ARTICLE ) );
                    }
                    break;

                case 'o':  // Owner name
                    {
                        SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%s", ego_chr::get_name( iowner, CHRNAME_ARTICLE ) );
                    }
                    break;

                case 's':  // Target class name
                    {
                        if ( NULL != ptarget_cap )
                        {
                            ebuffer     = ptarget_cap->classname;
                            ebuffer_end = ebuffer + SDL_arraysize( ptarget_cap->classname );
                        }
                    }
                    break;

                case '0':
                case '1':
                case '2':
                case '3': // Target's skin name
                    {
                        if ( NULL != ptarget_cap )
                        {
                            ebuffer     = ptarget_cap->skinname[( *src )-'0'];
                            ebuffer_end = ebuffer + SDL_arraysize( ptarget_cap->skinname[( *src )-'0'] );
                        }
                    }
                    break;

                case 'a':  // Character's ammo
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->ammoknown )
                            {
                                SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pchr->ammo );
                            }
                            else
                            {
                                SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "?" );
                            }
                        }
                    }
                    break;

                case 'k':  // Kurse state
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->iskursed )
                            {
                                SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "kursed" );
                            }
                            else
                            {
                                SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "unkursed" );
                            }
                        }
                    }
                    break;

                case 'p':  // Character's possessive
                    {
                        ego_chr::get_gender_possessive( ichr, szTmp, SDL_arraysize( szTmp ) );
                    }
                    break;

                case 'm':  // Character's gender
                    {
                        ego_chr::get_gender_name( ichr, szTmp, SDL_arraysize( szTmp ) );
                    }
                    break;

                case 'g':  // Target's possessive
                    {
                        ego_chr::get_gender_possessive( itarget, szTmp, SDL_arraysize( szTmp ) );
                    }
                    break;

                case '#':  // New line (enter)
                    {
                        SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "\n" );
                    }
                    break;

                case 'd':  // tmpdistance value
                    {
                        if ( NULL != pstate )
                        {
                            SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pstate->distance );
                        }
                    }
                    break;

                case 'x':  // tmpx value
                    {
                        if ( NULL != pstate )
                        {
                            SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pstate->x );
                        }
                    }
                    break;

                case 'y':  // tmpy value
                    {
                        if ( NULL != pstate )
                        {
                            SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pstate->y );
                        }
                    }
                    break;

                case 'D':  // tmpdistance value
                    {
                        if ( NULL != pstate )
                        {
                            SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%2d", pstate->distance );
                        }
                    }
                    break;

                case 'X':  // tmpx value
                    {
                        if ( NULL != pstate )
                        {
                            SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%2d", pstate->x );
                        }
                    }
                    break;

                case 'Y':  // tmpy value
                    {
                        if ( NULL != pstate )
                        {
                            SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%2d", pstate->y );
                        }
                    }
                    break;

                default:
                    SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%%%c???", ( *src ) );
                    break;
            }

            if ( CSTR_END == *ebuffer )
            {
                ebuffer     = szTmp;
                ebuffer_end = szTmp + SDL_arraysize( szTmp );
                SDL_snprintf( szTmp, SDL_arraysize( szTmp ), "%%%c???", ( *src ) );
            }

            // make the line capitalized if necessary
            if ( 0 == cnt && NULL != ebuffer )  *ebuffer = SDL_toupper( *ebuffer );

            // Copy the generated text
            while ( CSTR_END != *ebuffer && ebuffer < ebuffer_end && dst < dst_end )
            {
                *dst++ = *ebuffer++;
            }
            *dst = CSTR_END;
        }
        else
        {
            // Copy the letter
            *dst = *src;
            dst++;
        }

        src++;
        cnt++;
    }

    // make sure the destination string is terminated
    if ( dst < dst_end )
    {
        *dst = CSTR_END;
    }
    *dst_end = CSTR_END;
}

//--------------------------------------------------------------------------------------------
bool_t game_choose_module( int imod, int seed )
{
    bool_t retval;

    if ( seed < 0 ) seed = time( NULL );

    if ( NULL == PMod ) PMod = &gmod;

    retval = game_module_setup( PMod, mnu_ModList_get_base( imod ), mnu_ModList_get_vfs_path( imod ), seed );

    if ( retval )
    {
        // give everyone virtual access to the game directories
        game_setup_vfs_paths( pickedmodule.path );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void do_game_hud()
{
    int y = 0;

    if ( flip_pages_requested() && cfg.dev_mode )
    {
        GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );
        if ( fpson )
        {
            y = draw_string( 0, y, "%2.3f FPS, %2.3f UPS", stabilized_fps, stabilized_ups );
            y = draw_string( 0, y, "estimated max FPS %2.3f", est_max_fps ); \
        }

        y = draw_string( 0, y, "Menu time %f", MProc->dtime );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t collide_ray_with_mesh( ego_line_of_sight_info * plos )
{
    Uint32 fan_last;

    int Dx, Dy;
    int ix, ix_stt, ix_end;
    int iy, iy_stt, iy_end;

    int Dbig, Dsmall;
    int ibig, ibig_stt, ibig_end;
    int ismall, ismall_stt, ismall_end;
    int dbig, dsmall;
    int TwoDsmall, TwoDsmallMinusTwoDbig, TwoDsmallMinusDbig;

    bool_t steep;

    if ( NULL == plos ) return bfalse;

    if ( 0 == plos->stopped_by ) return bfalse;

    ix_stt = plos->x0 / GRID_SIZE;
    ix_end = plos->x1 / GRID_SIZE;

    iy_stt = plos->y0 / GRID_SIZE;
    iy_end = plos->y1 / GRID_SIZE;

    Dx = plos->x1 - plos->x0;
    Dy = plos->y1 - plos->y0;

    steep = ( SDL_abs( Dy ) >= SDL_abs( Dx ) );

    // determine which are the big and small values
    if ( steep )
    {
        ibig_stt = iy_stt;
        ibig_end = iy_end;

        ismall_stt = ix_stt;
        ismall_end = ix_end;
    }
    else
    {
        ibig_stt = ix_stt;
        ibig_end = ix_end;

        ismall_stt = iy_stt;
        ismall_end = iy_end;
    }

    // set up the big loop variables
    dbig = 1;
    Dbig = ibig_end - ibig_stt;
    if ( Dbig < 0 )
    {
        dbig = -1;
        Dbig = -Dbig;
        ibig_end--;
    }
    else
    {
        ibig_end++;
    }

    // set up the small loop variables
    dsmall = 1;
    Dsmall = ismall_end - ismall_stt;
    if ( Dsmall < 0 )
    {
        dsmall = -1;
        Dsmall = -Dsmall;
    }

    // pre-compute some common values
    TwoDsmall             = 2 * Dsmall;
    TwoDsmallMinusTwoDbig = TwoDsmall - 2 * Dbig;
    TwoDsmallMinusDbig    = TwoDsmall - Dbig;

    fan_last = INVALID_TILE;
    for ( ibig = ibig_stt, ismall = ismall_stt;  ibig != ibig_end;  ibig += dbig )
    {
        Uint32 fan;

        if ( steep )
        {
            ix = ismall;
            iy = ibig;
        }
        else
        {
            ix = ibig;
            iy = ismall;
        }

        // check to see if the "ray" collides with the mesh
        fan = ego_mpd::get_tile_int( PMesh, ix, iy );
        if ( INVALID_TILE != fan && fan != fan_last )
        {
            Uint32 collide_fx = ego_mpd::test_fx( PMesh, fan, plos->stopped_by );
            // collide the ray with the mesh

            if ( 0 != collide_fx )
            {
                plos->collide_x  = ix;
                plos->collide_y  = iy;
                plos->collide_fx = collide_fx;

                return btrue;
            }

            fan_last = fan;
        }

        // go to the next step
        if ( TwoDsmallMinusDbig > 0 )
        {
            TwoDsmallMinusDbig += TwoDsmallMinusTwoDbig;
            ismall             += dsmall;
        }
        else
        {
            TwoDsmallMinusDbig += TwoDsmall;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t collide_ray_with_characters( ego_line_of_sight_info * plos )
{

    if ( NULL == plos ) return bfalse;

    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        // do line/character intersection
    }
    CHR_END_LOOP();

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t do_line_of_sight( ego_line_of_sight_info * plos )
{
    bool_t mesh_hit = bfalse, chr_hit = bfalse;
    mesh_hit = collide_ray_with_mesh( plos );

    /*if ( mesh_hit )
    {
        plos->x1 = (plos->collide_x + 0.5f) * GRID_SIZE;
        plos->y1 = (plos->collide_y + 0.5f) * GRID_SIZE;
    }

    chr_hit = collide_ray_with_characters( plos );
    */

    return mesh_hit || chr_hit;
}

//--------------------------------------------------------------------------------------------
void game_reset_players()
{
    /// @details ZZ@> This function clears the player list data

    // Reset the local data stuff
    local_stats.init();

    net_reset_players();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t upload_water_layer_data( ego_water_layer_instance inst[], wawalite_water_layer_t data[], int layer_count )
{
    int layer;

    if ( NULL == inst || 0 == layer_count ) return bfalse;

    // clear all data
    SDL_memset( inst, 0, layer_count * sizeof( *inst ) );

    // set the frame
    for ( layer = 0; layer < layer_count; layer++ )
    {
        inst[layer].frame = generate_randmask( 0 , WATERFRAMEAND );
    }

    if ( NULL != data )
    {
        for ( layer = 0; layer < layer_count; layer++ )
        {
            inst[layer].z         = data[layer].z;
            inst[layer].amp       = data[layer].amp;

            inst[layer].dist      = data[layer].dist;

            inst[layer].light_dir = data[layer].light_dir / 63.0f;
            inst[layer].light_add = data[layer].light_add / 63.0f;

            inst[layer].tx_add    = data[layer].tx_add;

            inst[layer].alpha     = data[layer].alpha;

            inst[layer].frame_add = data[layer].frame_add;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_water_data( ego_water_instance * pinst, wawalite_water_t * pdata )
{
    //int layer;

    if ( NULL == pinst ) return bfalse;

    SDL_memset( pinst, 0, sizeof( *pinst ) );

    if ( NULL != pdata )
    {
        // upload the data

        pinst->surface_level    = pdata->surface_level;
        pinst->douse_level      = pdata->douse_level;

        pinst->is_water         = pdata->is_water;
        pinst->overlay_req      = pdata->overlay_req;
        pinst->background_req   = pdata->background_req;

        pinst->light            = pdata->light;

        pinst->foregroundrepeat = pdata->foregroundrepeat;
        pinst->backgroundrepeat = pdata->backgroundrepeat;

        // upload the layer data
        pinst->layer_count   = pdata->layer_count;
        upload_water_layer_data( pinst->layer, pdata->layer, pdata->layer_count );
    }

    // fix the light in case of self-lit textures
    //if ( pdata->light )
    //{
    //    for ( layer = 0; layer < pinst->layer_count; layer++ )
    //    {
    //        pinst->layer[layer].light_add = 1.0f;  // Some cards don't support light lights...
    //    }
    //}

    make_water( pinst, pdata );

    // Allow slow machines to ignore the fancy stuff
    if ( !cfg.twolayerwater_allowed && pinst->layer_count > 1 )
    {
        int iTmp = pdata->layer[0].light_add;
        iTmp = ( pdata->layer[1].light_add * iTmp * INV_FF ) + iTmp;
        if ( iTmp > 255 ) iTmp = 255;

        pinst->layer_count        = 1;
        pinst->layer[0].light_add = iTmp * INV_FF;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_weather_data( ego_weather_instance * pinst, wawalite_weather_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    SDL_memset( pinst, 0, sizeof( *pinst ) );

    // set a default value
    pinst->timer_reset = 10;

    if ( NULL != pdata )
    {
        // copy the data
        pinst->timer_reset = pdata->timer_reset;
        pinst->over_water  = pdata->over_water;
        pinst->particle = pdata->particle;
    }

    // set the new data
    pinst->time = pinst->timer_reset;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_fog_data( ego_fog_instance * pinst, wawalite_fog_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    SDL_memset( pinst, 0, sizeof( *pinst ) );

    pdata->top      = 0;
    pdata->bottom   = -100;
    pinst->on       = cfg.fog_allowed;

    if ( NULL != pdata )
    {
        pinst->on     = pdata->found && pinst->on;
        pinst->top    = pdata->top;
        pinst->bottom = pdata->bottom;

        pinst->red    = pdata->red * 0xFF;
        pinst->grn    = pdata->grn * 0xFF;
        pinst->blu    = pdata->blu * 0xFF;
    }

    pinst->distance = ( pdata->top - pdata->bottom );
    pinst->on       = ( pinst->distance < 1.0f ) && pinst->on;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_damagetile_data( ego_damagetile_instance * pinst, wawalite_damagetile_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    SDL_memset( pinst, 0, sizeof( *pinst ) );

    //pinst->sound_time   = TILESOUNDTIME;
    //pinst->min_distance = 9999;

    if ( NULL != pdata )
    {
        pinst->amount.base  = pdata->amount;
        pinst->amount.rand  = 1;
        pinst->type         = pdata->type;

        pinst->parttype     = pdata->parttype;
        pinst->partand      = pdata->partand;
        pinst->sound_index  = CLIP( pdata->sound_index, INVALID_SOUND, MAX_WAVE );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_animtile_data( ego_animtile_instance inst[], wawalite_animtile_t * pdata, size_t animtile_count )
{
    Uint32 cnt;

    if ( NULL == inst || 0 == animtile_count ) return bfalse;

    SDL_memset( inst, 0, sizeof( *inst ) );

    for ( cnt = 0; cnt < animtile_count; cnt++ )
    {
        inst[cnt].frame_and  = ( 1 << ( cnt + 2 ) ) - 1;
        inst[cnt].base_and   = ~inst[cnt].frame_and;
        inst[cnt].frame_add  = 0;
    }

    if ( NULL != pdata )
    {
        inst[0].update_and = pdata->update_and;
        inst[0].frame_and  = pdata->frame_and;
        inst[0].base_and   = ~inst[0].frame_and;

        for ( cnt = 1; cnt < animtile_count; cnt++ )
        {
            inst[cnt].update_and = pdata->update_and;
            inst[cnt].frame_and  = ( inst[cnt-1].frame_and << 1 ) | 1;
            inst[cnt].base_and   = ~inst[cnt].frame_and;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_light_data( wawalite_data_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    // upload the lighting data
    light_nrm[kX] = pdata->light_x;
    light_nrm[kY] = pdata->light_y;
    light_nrm[kZ] = pdata->light_z;
    light_a = pdata->light_a;

    if ( SDL_abs( light_nrm[kX] ) + SDL_abs( light_nrm[kY] ) + SDL_abs( light_nrm[kZ] ) > 0.0f )
    {
        float fTmp = SQRT( light_nrm[kX] * light_nrm[kX] + light_nrm[kY] * light_nrm[kY] + light_nrm[kZ] * light_nrm[kZ] );

        // get the extra magnitude of the direct light
        if ( gfx.usefaredge )
        {
            // we are outside, do the direct light as sunlight
            light_d = 1.0f;
            light_a = light_a / fTmp;
            light_a = CLIP( light_a, 0.0f, 1.0f );
        }
        else
        {
            // we are inside. take the lighting values at face value.
            //light_d = (1.0f - light_a) * fTmp;
            //light_d = CLIP(light_d, 0.0f, 1.0f);
            light_d = CLIP( fTmp, 0.0f, 1.0f );
        }

        light_nrm[kX] /= fTmp;
        light_nrm[kY] /= fTmp;
        light_nrm[kZ] /= fTmp;
    }

    //make_lighttable( pdata->light_x, pdata->light_y, pdata->light_z, pdata->light_a );
    //make_lighttospek();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_phys_data( wawalite_physics_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    // upload the physics data
    hillslide      = pdata->hillslide;
    slippyfriction = pdata->slippyfriction;
    noslipfriction = pdata->noslipfriction;
    airfriction    = pdata->airfriction;
    waterfriction  = pdata->waterfriction;
    gravity        = pdata->gravity;

    // estimate a value for platstick
    platstick = SDL_min( slippyfriction, noslipfriction );
    platstick = platstick  * platstick;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_graphics_data( wawalite_graphics_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    // Read extra data
    gfx.exploremode = pdata->exploremode;
    gfx.usefaredge  = pdata->usefaredge;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_camera_data( wawalite_camera_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    PCamera->swing     = pdata->swing;
    PCamera->swingrate = pdata->swingrate;
    PCamera->swingamp  = pdata->swingamp;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void upload_wawalite()
{
    /// @details ZZ@> This function sets up water and lighting for the module

    wawalite_data_t * pdata = &wawalite_data;

    upload_phys_data( &( pdata->phys ) );
    upload_graphics_data( &( pdata->graphics ) );
    upload_light_data( pdata );                         // this statement depends on data from upload_graphics_data()
    upload_camera_data( &( pdata->camera ) );
    upload_fog_data( &fog, &( pdata->fog ) );
    upload_water_data( &water, &( pdata->water ) );
    upload_weather_data( &weather, &( pdata->weather ) );
    upload_damagetile_data( &damagetile, &( pdata->damagetile ) );
    upload_animtile_data( animtile, &( pdata->animtile ), SDL_arraysize( animtile ) );
}

//--------------------------------------------------------------------------------------------
bool_t game_module_setup( ego_game_module_data * pinst, mod_file_t * pdata, const char * loadname, Uint32 seed )
{
    // Prepares a module to be played
    if ( NULL == pdata ) return bfalse;

    if ( !game_module_init( pinst ) ) return bfalse;

    pinst->importamount   = pdata->importamount;
    pinst->exportvalid    = pdata->allowexport;
    pinst->exportreset    = pdata->allowexport;
    pinst->playeramount   = pdata->maxplayers;
    pinst->importvalid    = ( pinst->importamount > 0 );
    pinst->respawnvalid   = ( bfalse != pdata->respawnvalid );
    pinst->respawnanytime = ( RESPAWN_ANYTIME == pdata->respawnvalid );

    strncpy( pinst->loadname, loadname, SDL_arraysize( pinst->loadname ) );

    pinst->active = bfalse;
    pinst->beat   = bfalse;
    pinst->seed   = seed;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t game_module_init( ego_game_module_data * pinst )
{
    if ( NULL == pinst ) return bfalse;

    SDL_memset( pinst, 0, sizeof( *pinst ) );

    pinst->seed = Uint32( ~0L );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t game_module_reset( ego_game_module_data * pinst, Uint32 seed )
{
    if ( NULL == pinst ) return bfalse;

    pinst->beat        = bfalse;
    pinst->exportvalid = pinst->exportreset;
    pinst->seed        = seed;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t game_module_start( ego_game_module_data * pinst )
{
    /// @details BB@> Let the module go

    if ( NULL == pinst ) return bfalse;

    pinst->active = btrue;

    srand( pinst->seed );
    pinst->randsave = rand();
    randindex = rand() % RANDIE_COUNT;

    network_set_host_active( btrue ); // very important or the input will not work

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t game_module_stop( ego_game_module_data * pinst )
{
    /// @details BB@> stop the module

    if ( NULL == pinst ) return bfalse;

    pinst->active      = bfalse;

    // network stuff
    network_set_host_active( bfalse );

    return btrue;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t * read_wawalite( /* const char *modname */ )
{
    int cnt, waterspeed_count, windspeed_count;

    wawalite_data_t * pdata;
    wawalite_water_layer_t * ilayer;

    // if( INVALID_CSTR(modname) ) return NULL;

    pdata = read_wawalite_file_vfs( "mp_data/wawalite.txt", NULL );
    if ( NULL == pdata ) return NULL;

    SDL_memcpy( &wawalite_data, pdata, sizeof( wawalite_data_t ) );

    // limit some values
    wawalite_data.damagetile.sound_index = CLIP( wawalite_data.damagetile.sound_index, INVALID_SOUND, MAX_WAVE );

    for ( cnt = 0; cnt < MAXWATERLAYER; cnt++ )
    {
        wawalite_data.water.layer[cnt].light_dir = CLIP( wawalite_data.water.layer[cnt].light_dir, 0, 63 );
        wawalite_data.water.layer[cnt].light_add = CLIP( wawalite_data.water.layer[cnt].light_add, 0, 63 );
    }

    windspeed_count = 0;
    fvec3_self_clear( windspeed.v );

    waterspeed_count = 0;
    fvec3_self_clear( waterspeed.v );

    ilayer = wawalite_data.water.layer + 0;
    if ( wawalite_data.water.background_req )
    {
        // this is a bit complicated.
        // it is the best I can do at reverse engineering what I did in render_world_background()

        const float cam_height = 1500.0f;
        const float default_bg_repeat = 4.0f;

        windspeed_count++;

        windspeed.x += -ilayer->tx_add.x * GRID_SIZE / ( wawalite_data.water.backgroundrepeat / default_bg_repeat ) * ( cam_height + 1.0f / ilayer->dist.x ) / cam_height;
        windspeed.y += -ilayer->tx_add.y * GRID_SIZE / ( wawalite_data.water.backgroundrepeat / default_bg_repeat ) * ( cam_height + 1.0f / ilayer->dist.y ) / cam_height;
        windspeed.z += -0;
    }
    else
    {
        waterspeed_count++;

        waterspeed.x += -ilayer->tx_add.x * GRID_SIZE;
        waterspeed.y += -ilayer->tx_add.y * GRID_SIZE;
        waterspeed.z += -0;
    }

    ilayer = wawalite_data.water.layer + 1;
    if ( wawalite_data.water.overlay_req )
    {
        windspeed_count++;

        windspeed.x += -600 * ilayer->tx_add.x * GRID_SIZE / wawalite_data.water.foregroundrepeat * 0.04f;
        windspeed.y += -600 * ilayer->tx_add.y * GRID_SIZE / wawalite_data.water.foregroundrepeat * 0.04f;
        windspeed.z += -0;
    }
    else
    {
        waterspeed_count++;

        waterspeed.x += -ilayer->tx_add.x * GRID_SIZE;
        waterspeed.y += -ilayer->tx_add.y * GRID_SIZE;
        waterspeed.z += -0;
    }

    if ( waterspeed_count > 1 )
    {
        waterspeed.x /= ( float )waterspeed_count;
        waterspeed.y /= ( float )waterspeed_count;
        waterspeed.z /= ( float )waterspeed_count;
    }

    if ( windspeed_count > 1 )
    {
        windspeed.x /= ( float )windspeed_count;
        windspeed.y /= ( float )windspeed_count;
        windspeed.z /= ( float )windspeed_count;
    }

    return &wawalite_data;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite( const char *modname, wawalite_data_t * pdata )
{
    /// @details BB@> Prepare and write the wawalite file

    int cnt;
//    STRING filename;

    if ( !VALID_CSTR( modname ) || NULL == pdata ) return bfalse;

    // limit some values
    pdata->damagetile.sound_index = CLIP( pdata->damagetile.sound_index, INVALID_SOUND, MAX_WAVE );

    for ( cnt = 0; cnt < MAXWATERLAYER; cnt++ )
    {
        pdata->water.layer[cnt].light_dir = CLIP( pdata->water.layer[cnt].light_dir, 0, 63 );
        pdata->water.layer[cnt].light_add = CLIP( pdata->water.layer[cnt].light_add, 0, 63 );
    }

    return write_wawalite_file_vfs( pdata );
}

//--------------------------------------------------------------------------------------------
Uint8 get_local_alpha( int light )
{
    if ( local_stats.seeinvis_level > 0 )
    {
        light = SDL_max( light, INVISIBLE );
//        light *= local_seeinvis_level + 1;
    }

    return CLIP( light, 0, 255 );
}

//--------------------------------------------------------------------------------------------
Uint8 get_local_light( int light )
{
    if ( 0xFFL == light ) return light;

    /// @note ZF@> Why should Darkvision reveal invisible?
    /// @note BB@> It doesn't. INVISIBLE doesn't have anything to do with invisibility,
    ///            it is just a generic threshold for whether an object can be visible to a character.
    ///            If it is actually magically invisible, then that is handled elsewhere.

    if ( local_stats.seedark_level > 0 )
    {
        light = SDL_max( light, INVISIBLE );
        light *= local_stats.seedark_level + 1;
    }

    return CLIP( light, 0, 254 );
}

//--------------------------------------------------------------------------------------------
bool_t do_shop_drop( const CHR_REF & idropper, const CHR_REF & iitem )
{
    ego_chr * pdropper, * pitem;
    bool_t inshop;

    pitem = ChrObjList.get_allocated_data_ptr( iitem );
    if ( !INGAME_PCHR( pitem ) ) return bfalse;

    pdropper = ChrObjList.get_allocated_data_ptr( idropper );
    if ( !INGAME_PCHR( pdropper ) ) return bfalse;

    inshop = bfalse;
    if ( pitem->isitem && ShopStack.count > 0 )
    {
        CHR_REF iowner;

        int ix = pitem->pos.x / GRID_SIZE;
        int iy = pitem->pos.y / GRID_SIZE;

        iowner = ShopStack_find_owner( ix, iy );
        if ( INGAME_CHR( iowner ) )
        {
            int price;
            ego_chr * powner = ChrObjList.get_data_ptr( iowner );

            inshop = btrue;

            price = ego_chr::get_price( iitem );

            // Are they are trying to sell junk or quest items?
            if ( 0 == price )
            {
                ego_ai_state::add_order( &( powner->ai ), ( Uint32 ) price, SHOP_BUY );
            }
            else
            {
                pdropper->money += price;
                pdropper->money  = CLIP( pdropper->money, 0, MAXMONEY );

                powner->money -= price;
                powner->money  = CLIP( powner->money, 0, MAXMONEY );

                ego_ai_state::add_order( &( powner->ai ), ( Uint32 ) price, SHOP_BUY );
            }
        }
    }

    return inshop;
}

//--------------------------------------------------------------------------------------------
bool_t do_shop_buy( const CHR_REF & ipicker, const CHR_REF & iitem )
{
    bool_t can_grab, can_pay, in_shop;
    int price;

    ego_chr * ppicker, * pitem;

    pitem = ChrObjList.get_allocated_data_ptr( iitem );
    if ( !INGAME_PCHR( pitem ) ) return bfalse;

    ppicker = ChrObjList.get_allocated_data_ptr( ipicker );
    if ( !INGAME_PCHR( ppicker ) ) return bfalse;

    can_grab = btrue;
    can_pay  = btrue;
    in_shop  = bfalse;

    if ( pitem->isitem && ShopStack.count > 0 )
    {
        CHR_REF iowner;

        int ix = pitem->pos.x / GRID_SIZE;
        int iy = pitem->pos.y / GRID_SIZE;

        iowner = ShopStack_find_owner( ix, iy );
        if ( INGAME_CHR( iowner ) )
        {
            ego_chr * powner = ChrObjList.get_data_ptr( iowner );

            in_shop = btrue;
            price   = ego_chr::get_price( iitem );

            if ( ppicker->money >= price )
            {
                // Okay to sell
                ego_ai_state::add_order( &( powner->ai ), ( Uint32 ) price, SHOP_SELL );

                ppicker->money -= price;
                ppicker->money  = CLIP( ppicker->money, 0, MAXMONEY );

                powner->money  += price;
                powner->money   = CLIP( powner->money, 0, MAXMONEY );

                can_grab = btrue;
                can_pay  = btrue;
            }
            else
            {
                // Don't allow purchase
                ego_ai_state::add_order( &( powner->ai ), price, SHOP_NOAFFORD );
                can_grab = bfalse;
                can_pay  = bfalse;
            }
        }
    }

    /// print some feedback messages
    /// @note: some of these are handled in scripts, so they could be disabled
    /*if( can_grab )
    {
        if( in_shop )
        {
            if( can_pay )
            {
                debug_printf( "%s bought %s", ego_chr::get_name( ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), ego_chr::get_name( iitem, CHRNAME_ARTICLE ) );
            }
            else
            {
                debug_printf( "%s can't afford %s", ego_chr::get_name( ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), ego_chr::get_name( iitem, CHRNAME_ARTICLE ) );
            }
        }
        else
        {
            debug_printf( "%s picked up %s", ego_chr::get_name( ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), ego_chr::get_name( iitem, CHRNAME_ARTICLE ) );
        }
    }*/

    return can_grab;
}

//--------------------------------------------------------------------------------------------
bool_t do_shop_steal( const CHR_REF & ithief, const CHR_REF & iitem )
{
    // Pets can try to steal in addition to invisible characters

    bool_t can_steal;

    ego_chr * pthief, * pitem;

    pitem = ChrObjList.get_allocated_data_ptr( iitem );
    if ( !INGAME_PCHR( pitem ) ) return bfalse;

    pthief = ChrObjList.get_allocated_data_ptr( ithief );
    if ( !INGAME_PCHR( pthief ) ) return bfalse;

    can_steal = btrue;
    if ( pitem->isitem && ShopStack.count > 0 )
    {
        CHR_REF iowner;

        int ix = pitem->pos.x / GRID_SIZE;
        int iy = pitem->pos.y / GRID_SIZE;

        iowner = ShopStack_find_owner( ix, iy );
        if ( INGAME_CHR( iowner ) )
        {
            IPair  tmp_rand = {1, 100};
            Uint8  detection;
            ego_chr * powner = ChrObjList.get_data_ptr( iowner );

            detection = generate_irand_pair( tmp_rand );

            can_steal = btrue;
            if ( chr_can_see_object( iowner, ithief ) || detection <= 5 || ( detection - ( pthief->dexterity >> 7 ) + ( powner->wisdom >> 7 ) ) > 50 )
            {
                ego_ai_state::add_order( &( powner->ai ), SHOP_STOLEN, SHOP_THEFT );
                powner->ai.target = ithief;
                can_steal = bfalse;
            }
        }
    }

    return can_steal;
}

//--------------------------------------------------------------------------------------------
bool_t do_item_pickup( const CHR_REF & ichr, const CHR_REF & iitem )
{
    bool_t can_grab;
    bool_t is_invis, can_steal, in_shop;
    ego_chr * pchr, * pitem;
    int ix, iy;

    // ?? lol what ??
    if ( ichr == iitem ) return bfalse;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    pitem = ChrObjList.get_allocated_data_ptr( iitem );
    if ( !INGAME_PCHR( pitem ) ) return bfalse;
    ix = pitem->pos.x / GRID_SIZE;
    iy = pitem->pos.y / GRID_SIZE;

    // assume that there is no shop so that the character can grab anything
    can_grab = btrue;
    in_shop = INGAME_CHR( ShopStack_find_owner( ix, iy ) );

    if ( in_shop )
    {
        // check for a stealthy pickup
        is_invis  = !chr_can_see_object( ichr, iitem );

        // pets are automatically stealthy
        can_steal = is_invis || pchr->isitem;

        if ( can_steal )
        {
            can_grab = do_shop_steal( ichr, iitem );

            if ( !can_grab )
            {
                debug_printf( "%s was detected!!", ego_chr::get_name( ichr, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
            }
            else
            {
                debug_printf( "%s stole %s", ego_chr::get_name( ichr, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ), ego_chr::get_name( iitem, CHRNAME_ARTICLE ) );
            }
        }
        else
        {
            can_grab = do_shop_buy( ichr, iitem );
        }
    }

    return can_grab;
}

//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_0( ego_mpd   * pmesh, int grid_x, int grid_y, bool_t waterwalk )
{
    float zdone = ego_mpd::get_max_vertex_0( pmesh, grid_x, grid_y );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = ego_mpd::get_tile( pmesh, grid_x, grid_y );

        if ( 0 != ego_mpd::test_fx( pmesh, tile, MPDFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_1( ego_mpd   * pmesh, int grid_x, int grid_y, ego_oct_bb   * pbump, bool_t waterwalk )
{
    float zdone = ego_mpd::get_max_vertex_1( pmesh, grid_x, grid_y, pbump->mins[OCT_X], pbump->mins[OCT_Y], pbump->maxs[OCT_X], pbump->maxs[OCT_Y] );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = ego_mpd::get_tile( pmesh, grid_x, grid_y );

        if ( 0 != ego_mpd::test_fx( pmesh, tile, MPDFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_2( ego_mpd   * pmesh, ego_chr * pchr )
{
    /// @details BB@> the object does not overlap a single grid corner. Check the 4 corners of the collision volume

    int corner;
    int ix_off[4] = {1, 1, 0, 0};
    int iy_off[4] = {0, 1, 1, 0};

    fvec4_base_t pos_x;
    fvec4_base_t pos_y;
    float        zmax;

    for ( corner = 0; corner < 4; corner++ )
    {
        pos_x[corner] = pchr->pos.x + (( 0 == ix_off[corner] ) ? pchr->chr_min_cv.mins[OCT_X] : pchr->chr_min_cv.maxs[OCT_X] );
        pos_y[corner] = pchr->pos.y + (( 0 == iy_off[corner] ) ? pchr->chr_min_cv.mins[OCT_Y] : pchr->chr_min_cv.maxs[OCT_Y] );
    }

    zmax = get_mesh_level( pmesh, pos_x[0], pos_y[0], pchr->waterwalk );
    for ( corner = 1; corner < 4; corner++ )
    {
        float fval = get_mesh_level( pmesh, pos_x[corner], pos_y[corner], pchr->waterwalk );
        zmax = SDL_max( zmax, fval );
    }

    return zmax;
}

//--------------------------------------------------------------------------------------------
float get_chr_level( ego_mpd   * pmesh, ego_chr * pchr )
{
    float zmax;
    int ix, ixmax, ixmin;
    int iy, iymax, iymin;

    int grid_vert_count = 0;
    int grid_vert_x[1024];
    int grid_vert_y[1024];

    ego_oct_bb   bump;

    if ( NULL == pmesh || !PROCESSING_PCHR( pchr ) ) return 0;

    // certain scenery items like doors and such just need to be able to
    // collide with the mesh. They all have 0.0f == pchr->bump.size
    if ( 0.0f == pchr->bump_stt.size )
    {
        return get_mesh_level( pmesh, pchr->pos.x, pchr->pos.y, pchr->waterwalk );
    }

    // otherwise, use the small collision volume to determine which tiles the object overlaps
    // move the collision volume so that it surrounds the object
    bump.mins[OCT_X]  = pchr->chr_min_cv.mins[OCT_X]  + pchr->pos.x;
    bump.maxs[OCT_X]  = pchr->chr_min_cv.maxs[OCT_X]  + pchr->pos.x;
    bump.mins[OCT_Y]  = pchr->chr_min_cv.mins[OCT_Y]  + pchr->pos.y;
    bump.maxs[OCT_Y]  = pchr->chr_min_cv.maxs[OCT_Y]  + pchr->pos.y;

    // determine the size of this object in tiles
    ixmin = bump.mins[OCT_X] / GRID_SIZE; ixmin = CLIP( ixmin, 0, pmesh->info.tiles_x - 1 );
    ixmax = bump.maxs[OCT_X] / GRID_SIZE; ixmax = CLIP( ixmax, 0, pmesh->info.tiles_x - 1 );

    iymin = bump.mins[OCT_Y] / GRID_SIZE; iymin = CLIP( iymin, 0, pmesh->info.tiles_y - 1 );
    iymax = bump.maxs[OCT_Y] / GRID_SIZE; iymax = CLIP( iymax, 0, pmesh->info.tiles_y - 1 );

    // do the simplest thing if the object is just on one tile
    if ( ixmax == ixmin && iymax == iymin )
    {
        return get_mesh_max_vertex_2( pmesh, pchr );
    }

    // hold off on these calculations in case they are not necessary
    bump.mins[OCT_Z]  = pchr->chr_min_cv.mins[OCT_Z]  + pchr->pos.z;
    bump.maxs[OCT_Z]  = pchr->chr_min_cv.maxs[OCT_Z]  + pchr->pos.z;
    bump.mins[OCT_XY] = pchr->chr_min_cv.mins[OCT_XY] + ( pchr->pos.x + pchr->pos.y );
    bump.maxs[OCT_XY] = pchr->chr_min_cv.maxs[OCT_XY] + ( pchr->pos.x + pchr->pos.y );
    bump.mins[OCT_YX] = pchr->chr_min_cv.mins[OCT_YX] + ( -pchr->pos.x + pchr->pos.y );
    bump.maxs[OCT_YX] = pchr->chr_min_cv.maxs[OCT_YX] + ( -pchr->pos.x + pchr->pos.y );

    // otherwise, make up a list of tiles that the object might overlap
    for ( iy = iymin; iy <= iymax; iy++ )
    {
        float grid_y = iy * TILE_ISIZE;

        for ( ix = ixmin; ix <= ixmax; ix++ )
        {
            float ftmp;
            int   itile;
            float grid_x = ix * TILE_ISIZE;

            ftmp = grid_x + grid_y;
            if ( ftmp < bump.mins[OCT_XY] || ftmp > bump.maxs[OCT_XY] ) continue;

            ftmp = -grid_x + grid_y;
            if ( ftmp < bump.mins[OCT_YX] || ftmp > bump.maxs[OCT_YX] ) continue;

            itile = ego_mpd::get_tile_int( pmesh, ix, iy );
            if ( INVALID_TILE == itile ) continue;

            grid_vert_x[grid_vert_count] = ix;
            grid_vert_y[grid_vert_count] = iy;
            grid_vert_count++;
        }
    }

    // we did not intersect a single tile corner
    // this could happen for, say, a very long, but thin shape that fits between the tiles.
    // the current system would not work for that shape
    if ( 0 == grid_vert_count )
    {
        return get_mesh_max_vertex_2( pmesh, pchr );
    }
    else
    {
        int cnt;
        float fval;

        // scan through the vertices that we know will interact with the object
        zmax = get_mesh_max_vertex_1( pmesh, grid_vert_x[0], grid_vert_y[0], &bump, pchr->waterwalk );
        for ( cnt = 1; cnt < grid_vert_count; cnt ++ )
        {
            fval = get_mesh_max_vertex_1( pmesh, grid_vert_x[cnt], grid_vert_y[cnt], &bump, pchr->waterwalk );
            zmax = SDL_max( zmax, fval );
        }
    }

    if ( zmax == -1e6 ) zmax = 0.0f;

    return zmax;
}

//--------------------------------------------------------------------------------------------
egoboo_rv move_water( ego_water_instance * pwater )
{
    /// @details ZZ@> This function animates the water overlays

    int layer;

    if ( NULL == pwater ) return rv_error;

    for ( layer = 0; layer < MAXWATERLAYER; layer++ )
    {
        ego_water_layer_instance * player = pwater->layer + layer;

        player->tx.x += player->tx_add.x;
        player->tx.y += player->tx_add.y;

        if ( player->tx.x >  1.0f )  player->tx.x -= 1.0f;
        if ( player->tx.y >  1.0f )  player->tx.y -= 1.0f;
        if ( player->tx.x < -1.0f )  player->tx.x += 1.0f;
        if ( player->tx.y < -1.0f )  player->tx.y += 1.0f;

        player->frame = ( player->frame + player->frame_add ) & WATERFRAMEAND;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
void cleanup_character_enchants( ego_chr * pchr )
{
    if ( NULL == pchr ) return;

    // clean up the enchant list
    pchr->firstenchant = cleanup_enchant_list( pchr->firstenchant, &( pchr->firstenchant ) );
}

//--------------------------------------------------------------------------------------------
void sort_stat_list()
{
    /// @details ZZ@> This function puts all of the local players on top of the StatList

    PLA_REF ipla;

    for ( ipla = 0; ipla < PlaStack.count; ipla++ )
    {
        ego_player * ppla = PlaStack + ipla;
        if ( !ppla->valid ) continue;

        if ( INPUT_BITS_NONE != ppla->device.bits )
        {
            StatList.move_to_top( ppla->index );
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t stat_lst::remove( const CHR_REF & ichr )
{
    int cnt;

    bool_t stat_found = bfalse;

    for ( cnt = 0; cnt < count; cnt++ )
    {
        if ( lst[cnt] == ichr )
        {
            stat_found = btrue;
            break;
        }
    }

    if ( stat_found )
    {
        // move the entries to fill in the hole
        for ( cnt++; cnt < count; cnt++ )
        {
            lst[cnt-1] = lst[cnt];
        }
        count--;
    }

    return stat_found;
}

//--------------------------------------------------------------------------------------------
bool_t stat_lst::add( const CHR_REF & ichr )
{
    /// @details ZZ@> This function adds a status display to the do list

    if ( count >= MAXSTAT ) return bfalse;

    if ( !ChrObjList.valid_index_range( ichr.get_value() ) ) return bfalse;

    bool_t found = bfalse;
    for ( int i = 0; i < count; i++ )
    {
        if ( lst[i] == ichr )
        {
            found = btrue;
            break;
        }
    }

    if ( !found )
    {
        lst[count] = ichr;
        count++;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t stat_lst::move_to_top( const CHR_REF & ichr )
{
    /// @details ZZ@> This function puts the character on top of the StatList

    int cnt, oldloc;

    // Find where it is
    oldloc = count;

    // scan the list
    for ( cnt = 0; cnt < count; cnt++ )
    {
        if ( lst[cnt] == ichr )
        {
            oldloc = cnt;
            break;
        }
    }

    // bubble the item to the top of the list, if found
    if ( oldloc < count )
    {
        int tnc = oldloc;

        // Move all the lower ones up
        while ( tnc > 0 )
        {
            tnc--;
            lst[tnc+1] = lst[tnc];
        }

        // Put the ichr in the top slot
        lst[0] = ichr;
    }

    return oldloc < count;
}
