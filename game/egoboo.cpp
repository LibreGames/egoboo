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

/// @file egoboo.c
/// @brief Code for the main program process
/// @details

#define DECLARE_GLOBALS

#include "log.h"
#include "clock.h"
#include "system.h"
#include "graphic.h"
#include "network.h"
#include "sound.h"
#include "profile.inl"
#include "ui.h"
#include "font_bmp.h"
#include "input.h"
#include "game.h"
#include "menu.h"

#include "char.inl"
#include "particle.inl"
#include "enchant.inl"
#include "collision.h"

#include "file_formats/scancode_file.h"
#include "file_formats/controls_file.h"

#include "extensions/SDL_extensions.h"

#include "egoboo_fileutil.h"
#include "egoboo_setup.h"
#include "egoboo_vfs.h"
#include "egoboo_console.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

#include <SDL.h>
#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void memory_cleanUp( void );
static int  ego_init_SDL();
static void console_begin();
static void console_end();

static void object_systems_begin( void );
static void object_systems_end( void );

static void _quit_game( ego_main_process * pgame );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static ClockState_t    * _gclock = NULL;
static ego_main_process     _eproc;

static bool_t _sdl_atexit_registered    = bfalse;
static bool_t _sdl_initialized_base     = bfalse;

static void * _top_con = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_main_process     * EProc   = &_eproc;

ego_local_shared_stats local_stats;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv ego_main_process::do_beginning()
{
    if ( NULL == this )  return rv_error;
    result = -1;

    if ( !validate() ) return rv_error;

    // initialize the virtual filesystem first
    vfs_init();
    egoboo_setup_vfs_paths();

    // Initialize logging next, so that we can use it everywhere.
    log_init( vfs_resolveWriteFilename( "/debug/log.txt" ) );
    log_setLoggingLevel( 3 );

    // start initializing the various subsystems
    log_message( "Starting Egoboo " VERSION " ...\n" );
    log_info( "PhysFS file system version %s has been initialized...\n", vfs_getVersion() );

    sys_initialize();
    clk_init();
    _gclock = clk_create( "global clock", -1 );

    // read the "setup.txt" file
    setup_read_vfs();

    // download the "setup.txt" values into the cfg struct
    setup_download( &cfg );

    // do basic system initialization
    ego_init_SDL();
    gfx_system_begin();
    console_begin();
    network_system_begin( &cfg );

    log_info( "Initializing SDL_Image version %d.%d.%d... ", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL );
    GLSetup_SupportedFormats();

    // read all the scantags
    scantag_read_all_vfs( "mp_data/scancode.txt" );

    if ( fs_ensureUserFile( "controls.txt", btrue ) )
    {
        input_settings_load_vfs( "/controls.txt" );
    }

    // synchronize the config values with the various game subsystems
    // do this after the ego_init_SDL() and ogl_init() in case the config values are clamped
    // to valid values
    setup_synch( &cfg );

    // initialize the sound system
    sound_initialize();
    load_all_music_sounds_vfs();

    // make sure that a bunch of stuff gets initialized properly
    object_systems_begin();
    game_module_init( PMod );
    ego_mpd::ctor_this( PMesh );
    init_all_graphics();
    profile_system_begin();

    // setup the menu system's gui
    ui_begin( vfs_resolveReadFilename( "mp_data/Bo_Chen.ttf" ), 24 );

    // make sure that we have a cursor for the ui
    load_cursor();

    // clear out the import directory
    vfs_empty_import_directory();

    // register the memory_cleanUp function to automatically run whenever the program exits
    atexit( memory_cleanUp );

    // initialize the game process, but do not activate it
    GProc = ego_game_process::ctor_this( GProc );

    // initialize the menu process, and...
    MProc = ego_menu_process::ctor_all( MProc );
    // activate the it
    MProc->start();

    // go to the next state
    result = 1;
    state = proc_entering;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_main_process::do_running()
{
    static bool_t _explore_mode_keyready = btrue;
    static bool_t _wizard_mode_keyready  = btrue;
    static bool_t _i_win_keyready        = btrue;

    bool_t menu_valid, game_valid;
    bool_t mod_ctrl = bfalse, mod_shift = bfalse;

    if ( NULL == this )  return rv_error;
    result = -1;

    if ( !validate() ) return rv_error;

    was_active  = valid;

    menu_valid = MProc->validate( );
    game_valid = GProc->validate( );
    if ( !menu_valid && !game_valid )
    {
        kill( );
        result = 1;
        goto ego_main_process_do_running_end;
    }

    if ( paused )
    {
        result = 0;
        goto ego_main_process_do_running_end;
    }

    if ( rv_success == MProc->running() )
    {
        // menu settings
        SDL_WM_GrabInput( SDL_GRAB_OFF );
        SDL_ShowCursor( SDL_DISABLE );
    }
    else
    {
        // in-game settings
        SDL_ShowCursor( cfg.hide_mouse ? SDL_DISABLE : SDL_ENABLE );
        SDL_WM_GrabInput( cfg.grab_mouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
    }

    // Clock updates each frame
    clk_frameStep( _gclock );
    frameDuration = clk_getFrameDuration( _gclock );

    // read the input values
    input_read();
    mod_ctrl  = SDLKEYDOWN( SDLK_LCTRL ) || SDLKEYDOWN( SDLK_RCTRL );
    mod_shift = SDLKEYDOWN( SDLK_LSHIFT ) || SDLKEYDOWN( SDLK_RSHIFT );

    if ( pickedmodule_ready && ( rv_success != MProc->running() ) )
    {
        // a new module has been picked

        // reset the flag
        pickedmodule_ready = bfalse;

        // start the game process
        GProc->start();
    }

    // Test the panic button
    if ( SDLKEYDOWN( SDLK_q ) && mod_ctrl )
    {
        // terminate the program
        kill( );
    }

    if ( cfg.dev_mode )
    {
        if ( !SDLKEYDOWN( SDLK_F10 ) )
        {
            single_frame_keyready = btrue;
        }
        else if ( single_frame_keyready && SDLKEYDOWN( SDLK_F10 ) )
        {
            if ( !single_frame_mode )
            {
                single_frame_mode = btrue;
            }

            // request one update and one frame
            single_frame_requested  = btrue;
            single_update_requested = btrue;
            single_frame_keyready   = bfalse;
        }
    }

    // Check for screenshots
    if ( !SDLKEYDOWN( SDLK_F11 ) )
    {
        screenshot_keyready = btrue;
    }
    else if ( screenshot_keyready && SDLKEYDOWN( SDLK_F11 ) && keyb.on )
    {
        screenshot_keyready = bfalse;
        screenshot_requested = btrue;
    }

    // handle the explore mode
    if ( cfg.dev_mode && keyb.on )
    {
        bool_t explore_mode_key = SDLKEYDOWN( SDLK_F9 ) && mod_shift && !mod_ctrl;

        if ( !explore_mode_key )
        {
            _explore_mode_keyready = btrue;
        }
        else if ( _explore_mode_keyready )
        {
            _explore_mode_keyready = bfalse;

            PlaStack_toggle_all_explore();

            log_info( "Toggling explore mode\n" );
        }
    }

    // handle the wizard mode
    if ( cfg.dev_mode && keyb.on )
    {
        bool_t wizard_mode_key = SDLKEYDOWN( SDLK_F9 ) && mod_ctrl && !mod_shift;

        if ( !wizard_mode_key )
        {
            _wizard_mode_keyready = btrue;
        }
        else if ( _wizard_mode_keyready )
        {
            _wizard_mode_keyready = bfalse;

            PlaStack_toggle_all_wizard();

            log_info( "Toggling wizard mode\n" );
        }
    }

    // handle the "i win"
    if ( PlaStack_has_wizard() && keyb.on && NULL != PMod && PMod->active )
    {
        bool_t i_win_key = SDLKEYDOWN( SDLK_F9 ) && !mod_shift && !mod_ctrl;

        if ( !i_win_key )
        {
            _i_win_keyready = btrue;
        }
        else if ( _i_win_keyready )
        {
            // super secret "I win" button

            //PMod->beat        = btrue;
            //PMod->exportvalid = btrue;

            CHR_BEGIN_LOOP_ACTIVE( itarget, ptarget )
            {
                // don't kill players
                if ( !VALID_PLA( ptarget->is_which_player ) )
                {
                    PLA_REF ipla;

                    // only kill characters that the players hate
                    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
                    {
                        ego_chr    * pchr = NULL;

                        pchr = pla_get_pchr( ipla );
                        if ( NULL == pchr ) continue;

                        if ( team_hates_team( pchr->baseteam, ptarget->team ) )
                        {
                            kill_character( itarget, ( CHR_REF )511, bfalse );
                            break;
                        }
                    }
                }
            }
            CHR_END_LOOP();

            _i_win_keyready = bfalse;

            log_info( "\"I WIN!\" button\n" );
        }
    }

    // handle an escape by passing it on to all active sub-processes
    if ( escape_requested )
    {
        escape_requested = bfalse;

        // use the escape key to get out of single frame mode
        single_frame_mode = bfalse;

        if ( rv_success == GProc->running() )
        {
            GProc->escape_requested = btrue;
        }

        if ( rv_success == MProc->running() )
        {
            MProc->escape_requested = btrue;
        }
    }

    // run the sub-processes
    ProcessEngine.run( GProc, EProc->frameDuration );
    ProcessEngine.run( MProc, EProc->frameDuration );

    // a heads up display that can be used to debug values that are used by both the menu and the game
    // do_game_hud();

    result = 0;

ego_main_process_do_running_end:

    if ( 1 == result )
    {
        state = proc_leaving;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_main_process::do_leaving()
{
    if ( NULL == this )  return rv_error;
    result = -1;

    if ( !validate() ) return rv_error;

    // make sure that the
    if ( !GProc->terminated )
    {
        ProcessEngine.run( GProc, frameDuration );
    }

    if ( !MProc->terminated )
    {
        ProcessEngine.run( MProc, frameDuration );
    }

    if ( GProc->terminated && MProc->terminated )
    {
        terminate();
    }

    if ( terminated )
    {
        // hopefully this will only happen once
        object_systems_end();
        clk_destroy( &_gclock );
        console_end();
        ui_end();
        gfx_system_end();
        egoboo_clear_vfs_paths();
    }

    result = terminated ? 0 : 1;

    if ( 1 == result )
    {
        state  = proc_finishing;
        killme = bfalse;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_main_process::do_finishing()
{
    if ( NULL == this )  return rv_error;
    result = -1;

    if ( !validate() ) return rv_error;

    terminate();

    return rv_success;
}

//--------------------------------------------------------------------------------------------
//int ego_main_process::Run( ego_main_process * eproc, double frameDuration )
//{
//    int result = 0, proc_result = 0;
//
//    if ( !ego_process::validate( eproc ) ) return -1;
//    eproc->dtime = frameDuration;
//
//    if ( !eproc->paused ) return 0;
//
//    if ( eproc->killme )
//    {
//        eproc->state = proc_leaving;
//    }
//
//    switch ( eproc->state )
//    {
//        case proc_beginning:
//            proc_result = ego_main_process::do_beginning( eproc );
//
//            if ( 1 == proc_result )
//            {
//                eproc->state = proc_entering;
//            }
//            break;
//
//        case proc_entering:
//            // proc_result = ego_main_process::do_entering( eproc );
//
//            eproc->state = proc_running;
//            break;
//
//        case proc_running:
//            proc_result = ego_main_process::do_running( eproc );
//
//            if ( 1 == proc_result )
//            {
//                eproc->state = proc_leaving;
//            }
//            break;
//
//        case proc_leaving:
//            proc_result = ego_main_process::do_leaving( eproc );
//
//            if ( 1 == proc_result )
//            {
//                eproc->state  = proc_finishing;
//                eproc->killme = bfalse;
//            }
//            break;
//
//        case proc_finishing:
//            ego_process::terminate( eproc );
//            break;
//    }
//
//    return result;
//}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int SDL_main( int argc, char **argv )
{
    /// @details ZZ@> This is where the program starts and all the high level stuff happens

    int result = 0;

    // initialize the process
    ego_main_process::init( EProc, argc, argv );

    // Initialize the process
    EProc->start();

    // turn on all basic services
    EProc->do_beginning();

    // run the processes
    request_clear_screen();
    while ( !EProc->killme && !EProc->terminated )
    {
        // put a throttle on the ego process
        EProc->ticks_now = SDL_GetTicks();
        if ( EProc->ticks_now < EProc->ticks_next ) continue;

        // update the timer: 10ms delay between loops
        EProc->ticks_next = EProc->ticks_now + 10;

        // clear the screen if needed
        do_clear_screen();

        EProc->do_running();

        // flip the graphics page if need be
        do_flip_pages();

        // let the OS breathe. It may delay as long as 10ms
        SDL_Delay( 1 );
    }

    // terminate the game and menu processes
    GProc->kill();
    MProc->kill();
    while ( !EProc->terminated )
    {
        result = EProc ->do_leaving();
    }

    return result;
}

//--------------------------------------------------------------------------------------------
void memory_cleanUp( void )
{
    /// @details ZF@> This function releases all loaded things in memory and cleans up everything properly

    log_info( "memory_cleanUp() - Attempting to clean up loaded things in memory... " );

    // quit any existing game
    _quit_game( EProc );

    // synchronize the config values with the various game subsystems
    setup_synch( &cfg );

    // quit the setup system, making sure that the setup file is written
    setup_upload( &cfg );
    setup_write();
    setup_quit();

    // delete all the graphics allocated by SDL and OpenGL
    delete_all_graphics();

    // make sure that the current control configuration is written
    input_settings_save_vfs( "controls.txt" );

    // shut down the ui
    ui_end();

    // shut down the network
    network_system_end();

    // shut down the clock services
    clk_destroy( &_gclock );
    clk_shutdown();

    // deallocate any dynamically allocated collision memory
    collision_system_end();

    // deallocate any dynamically allocated scripting memory
    scripting_system_end();

    // deallocate all dynamically allocated memory for characters, particles, and enchants
    object_systems_end();

    // clean up any remaining models that might have dynamic data
    MadList_dtor();

    log_message( "Success!\n" );
    log_info( "Exiting Egoboo " VERSION " the good way...\n" );

    // shut down the log services
    log_shutdown();
}

//--------------------------------------------------------------------------------------------
int ego_init_SDL()
{
    ego_init_SDL_base();
    input_init();

    return _sdl_initialized_base;
}

//--------------------------------------------------------------------------------------------
void ego_init_SDL_base()
{
    if ( _sdl_initialized_base ) return;

    log_info( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
    if ( SDL_Init( 0 ) < 0 )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    if ( !_sdl_atexit_registered )
    {
        atexit( SDL_Quit );
        _sdl_atexit_registered = bfalse;
    }

    log_info( "Intializing SDL Timing Services... " );
    if ( SDL_InitSubSystem( SDL_INIT_TIMER ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    log_info( "Intializing SDL Event Threading... " );
    if ( SDL_InitSubSystem( SDL_INIT_EVENTTHREAD ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    _sdl_initialized_base = btrue;
}

//--------------------------------------------------------------------------------------------
void console_begin()
{
    /// @details BB@> initialize the console. This must happen after the screen has been defines,
    ///     otherwise sdl_scr.x == sdl_scr.y == 0 and the screen will be defined to
    ///     have no area...

    SDL_Rect blah = {0, 0, ( Uint16 )sdl_scr.x, ( Uint16 )( sdl_scr.y / 4 )};

#if defined(USE_LUA_CONSOLE)
    _top_con = lua_console_create( NULL, blah );
#else
    // without a callback, this console just dumps the input and generates no output
    _top_con = egoboo_console_create( NULL, blah, NULL, NULL );
#endif
}

//--------------------------------------------------------------------------------------------
void console_end()
{
    /// @details BB@> de-initialize the top console

#if defined(USE_LUA_CONSOLE)
    {
        lua_console_t * ptmp = ( lua_console_t* )_top_con;
        lua_console_destroy( &ptmp );
    }
#else
    // without a callback, this console just dumps the input and generates no output
    {
        egoboo_console_t * ptmp = ( egoboo_console_t* )_top_con;
        egoboo_console_destroy( &ptmp, SDL_TRUE );
    }
#endif

    _top_con = NULL;
}

//--------------------------------------------------------------------------------------------
void object_systems_begin( void )
{
    /// @details BB@> initialize all the object systems

    particle_system_begin();
    enchant_system_begin();
    character_system_begin();
}

//--------------------------------------------------------------------------------------------
void object_systems_end( void )
{
    /// @details BB@> quit all the object systems

    particle_system_end();
    enchant_system_end();
    character_system_end();
}

//--------------------------------------------------------------------------------------------
void _quit_game( ego_main_process * pgame )
{
    /// @details ZZ@> This function exits the game entirely

    if ( pgame->running() )
    {
        game_quit_module();
    }

    // tell the game to kill itself
    pgame->kill();

    vfs_empty_import_directory();
}

//--------------------------------------------------------------------------------------------
ego_main_process * ego_main_process::init( ego_main_process * eproc, int argc, char **argv )
{
    if ( NULL == eproc ) return NULL;

    eproc = ego_main_process::ctor_all( eproc );

    eproc->argv0 = ( argc > 0 ) ? argv[0] : NULL;

    return eproc;
}

//--------------------------------------------------------------------------------------------
void egoboo_clear_vfs_paths()
{
    /// @details BB@> clear out the basic mount points

    vfs_remove_mount_point( "mp_data" );
    vfs_remove_mount_point( "mp_modules" );
    vfs_remove_mount_point( "mp_players" );
    vfs_remove_mount_point( "mp_remote" );
}

//--------------------------------------------------------------------------------------------
void egoboo_setup_vfs_paths()
{
    /// @details BB@> set the basic mount points used by the main program

    //---- tell the vfs to add the basic search paths
    vfs_set_base_search_paths();

    //---- mount all of the default global directories

    // mount the global basicdat directory t the beginning of the list
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat", "mp_data", 1 );

    // Create a mount point for the /user/modules directory
    vfs_add_mount_point( fs_getUserDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /data/modules directory
    vfs_add_mount_point( fs_getDataDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /user/players directory
    vfs_add_mount_point( fs_getUserDirectory(), "players", "mp_players", 1 );

    // Create a mount point for the /data/players directory
    /// @note ZF@> Let's remove the local players folder since it caused so many problems for people
    //vfs_add_mount_point( fs_getDataDirectory(), "players", "mp_players", 1 );

    // Create a mount point for the /user/import directory
    vfs_add_mount_point( fs_getUserDirectory(), "import", "mp_import", 1 );

    // Create a mount point for the /user/remote directory
    vfs_add_mount_point( fs_getUserDirectory(), "remote", "mp_remote", 1 );
}

//--------------------------------------------------------------------------------------------
Uint32 egoboo_get_ticks( void )
{
    Uint32 ticks = 0;

    if ( single_frame_mode )
    {
        ticks = UPDATE_SKIP * update_wld;
    }
    else
    {
        ticks = SDL_GetTicks();
    }

    return ticks;
}

