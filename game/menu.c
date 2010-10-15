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

/// @file menu.c
/// @brief Implements the main menu tree, using the code in Ui.*
/// @details

#include "menu.h"

#include "particle.inl"
#include "mad.h"
#include "char.inl"
#include "profile.inl"

#include "game.h"
#include "quest.h"

#include "controls_file.h"
#include "scancode_file.h"
#include "ui.h"
#include "log.h"
#include "link.h"
#include "game.h"
#include "texture.h"
#include "module_file.h"

// To allow changing settings
#include "sound.h"
#include "input.h"
#include "camera.h"
#include "graphic.h"

#include "egoboo_vfs.h"
#include "egoboo_typedef.h"
#include "egoboo_fileutil.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"

#include "egoboo.h"

#include "egoboo_math.inl"

#include "SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The possible states of the menu state machine
enum e_menu_states
{
    MM_Begin,
    MM_Entering,
    MM_Running,
    MM_Leaving,
    MM_Finish
};

#define MENU_STACK_COUNT   256
#define WIDGET_MAX         100
#define MENU_MAX_GAMETIPS  100

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// "Slidy" buttons used in some of the menus.  They're shiny.
struct s_SlidyButtonState
{
    // string data
    const char ** but_text;

    // widget data
    ui_Widget_t * but;
    size_t        but_count;

    // time
    float lerp;

    // formatting
    int top;
    int left;
};
typedef struct s_SlidyButtonState SlidyButtonState_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the data to display a chosen player in the load player menu
struct s_ChoosePlayer_element
{
    CAP_REF           cap_ref;   ///< the index of the cap_t
    TX_REF            tx_ref;    ///< the index of the icon texture
    chop_definition_t chop;      ///< put this here so we can generate a name without loading an entire profile
};
typedef struct s_ChoosePlayer_element ChoosePlayer_element_t;

#define PLAYER_ELEMENT_INIT \
    { \
        /* cap_ref */ MAX_CAP,             \
        /* tx_ref  */ INVALID_TX_TEXTURE,  \
        /* chop    */ CHOP_DEFINITION_INIT \
    }

//--------------------------------------------------------------------------------------------

/// The data that menu.c uses to store the users' choice of players
struct s_ChoosePlayer_profiles
{
    int count;                                                 ///< the profiles that have been loaded
    ChoosePlayer_element_t pro_data[MAXIMPORTPERPLAYER + 1];   ///< the profile data
};
typedef struct s_ChoosePlayer_profiles ChoosePlayer_profiles_t;

#define PLAYER_PROFILES_INIT { 0, {PLAYER_ELEMENT_INIT} }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the module data that the menu system needs
struct s_mnu_module
{
    EGO_PROFILE_STUFF;                           ///< the "base class" of a profile obbject

    mod_file_t base;                               ///< the data for the "base class" of the module

    // extended data
    TX_REF tex_index;                              ///< the index of the module's tile image

    STRING vfs_path;                               ///< the virtual pathname of the module
    STRING dest_path;                              ///< the path that module data can be written into
};
typedef struct s_mnu_module mnu_module_t;

#define VALID_MOD_RANGE( IMOD ) ( ((IMOD) >= 0) && ((IMOD) < MAX_MODULE) )
#define VALID_MOD( IMOD )       ( VALID_MOD_RANGE( IMOD ) && IMOD < mnu_ModList.count && mnu_ModList.lst[IMOD].loaded )
#define INVALID_MOD( IMOD )     ( !VALID_MOD_RANGE( IMOD ) || IMOD >= mnu_ModList.count || !mnu_ModList.lst[IMOD].loaded )

INSTANTIATE_STACK_STATIC( mnu_module_t, mnu_ModList, MAX_MODULE );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data that menu.c uses to store the users' choice of players
struct s_GameTips
{
    // These are loaded only once
    Uint8  count;                         //< Number of global tips loaded
    STRING hint[MENU_MAX_GAMETIPS];       //< The global hints/tips

    // These are loaded for every module
    Uint8  local_count;                   //< Number of module specific tips loaded
    STRING local_hint[MENU_MAX_GAMETIPS]; //< Module specific hints and tips
};
typedef struct s_GameTips GameTips_t;

//--------------------------------------------------------------------------------------------
// declaration of "private" variables
//--------------------------------------------------------------------------------------------

static int          mnu_stack_index = 0;
static which_menu_t mnu_stack[MENU_STACK_COUNT];

static which_menu_t mnu_whichMenu = emnu_Main;

static module_filter_t mnu_moduleFilter = FILTER_OFF;

static ui_Widget_t mnu_widgetList[WIDGET_MAX];

static int selectedModule = -1;

/* Copyright text variables.  Change these to change how the copyright text appears */
static const char * copyrightText = "Welcome to Egoboo!\nhttp://egoboo.sourceforge.net\nVersion " VERSION "\n";
static int  copyrightLeft = 0;
static int  copyrightTop  = 0;
static display_list_t * copyrightText_tx_ptr = NULL;

/* Options info text variables.  Change these to change how the options text appears */
static const char * tipText = "Put a tip in this box";
static int tipText_left = 0;
static int tipText_top  = 0;
static display_list_t * tipText_tx = NULL;

/* Button position for the "easy" menus, like the main one */
static int buttonLeft = 0;
static int buttonTop = 0;

static int selectedPlayer = 0;           // Which player is currently selected to play

static menu_process_t    _mproc;

static int     mnu_selectedPlayerCount = 0;
static Uint32  mnu_selectedInput[MAX_PLAYER] = {0};
static int     mnu_selectedPlayer[MAX_PLAYER] = {0};

static GameTips_t mnu_GameTip = { 0 };

//--------------------------------------------------------------------------------------------
// declaration of public variables
//--------------------------------------------------------------------------------------------

#define TITLE_TEXTURE_COUNT   MAX_MODULE
#define INVALID_TITLE_TEXTURE TITLE_TEXTURE_COUNT

INSTANTIATE_STACK_STATIC( oglx_texture_t, TxTitleImage, TITLE_TEXTURE_COUNT ); // OpenGL title image surfaces

menu_process_t * MProc             = &_mproc;
bool_t           start_new_player  = bfalse;
bool_t           module_list_valid = bfalse;

/* The font used for drawing text.  It's smaller than the button font */
#define MAX_MENU_DISPLAY 20
#define COPYRIGHT_DISPLAY 0
display_list_t * menuTextureList_ptr = NULL;
TTF_Font          * menuFont    = NULL;

bool_t mnu_draw_background = btrue;

int              loadplayer_count = 0;
LOAD_PLAYER_INFO loadplayer[MAXLOADPLAYER];

//--------------------------------------------------------------------------------------------
// "private" function prototypes
//--------------------------------------------------------------------------------------------

// Implementation of the mnu_stack
static bool_t       mnu_stack_push( which_menu_t menu );
static which_menu_t mnu_stack_pop();
static which_menu_t mnu_stack_peek();
static void         mnu_stack_clear();

// Implementation of the mnu_SlidyButton array
static SlidyButtonState_t * SlidyButtonState_init( SlidyButtonState_t * pstate, float lerp, int id_start, const char *button_text[], ui_Widget_t * button_widget );
static SlidyButtonState_t * SlidyButtonState_update_all( SlidyButtonState_t * pstate, float deltaTime );
static SlidyButtonState_t * SlidyButtonState_draw_all( SlidyButtonState_t * pstate );

// implementation of the mnu_Selected* arrays
static bool_t  mnu_Selected_check_loadplayer( int loadplayer_idx );
static bool_t  mnu_Selected_add( int loadplayer_idx );
static bool_t  mnu_Selected_remove( int loadplayer_idx );
static bool_t  mnu_Selected_add_input( int loadplayer_idx, Uint32 input_bits );
static bool_t  mnu_Selected_remove_input( int loadplayer_idx, Uint32 input_bits );
static int     mnu_Selected_get_loadplayer( int loadplayer_idx );

// implementation of "private" TxTitleImage functions
static void             TxTitleImage_clear_data();
static void             TxTitleImage_release_one( const TX_REF by_reference index );
static void             TxTitleImage_ctor();
static void             TxTitleImage_release_all();
static void             TxTitleImage_dtor();
static oglx_texture_t * TxTitleImage_get_ptr( const TX_REF by_reference itex );

// tipText functions
static void tipText_set_position( TTF_Font * font, const char * text, int spacing );

// copyrightText functions
static void copyrightText_set_position( TTF_Font * font, const char * text, int spacing );

// implementation of "private" ModList functions
static void mnu_ModList_release_images();
void        mnu_ModList_release_all();

// "process" management
static int do_menu_proc_beginning( menu_process_t * mproc );
static int do_menu_proc_running( menu_process_t * mproc );
static int do_menu_proc_leaving( menu_process_t * mproc );

// the hint system
static void   mnu_GameTip_load_global_vfs();
static bool_t mnu_GameTip_load_local_vfs();

// "private" module utility
static void   mnu_load_all_module_info();

// "private" asset function
static TX_REF mnu_get_icon_ref( const CAP_REF by_reference icap, const TX_REF by_reference default_ref );

// implementation of the autoformatting
static void autoformat_init_slidy_buttons();
static void autoformat_init_tip_text();
static void autoformat_init_copyright_text();

// misc other stuff
static void mnu_release_one_module( const MOD_REF by_reference imod );
static void mnu_load_all_module_images_vfs();

//--------------------------------------------------------------------------------------------
// implementation of the menu stack
//--------------------------------------------------------------------------------------------
bool_t mnu_stack_push( which_menu_t menu )
{
    mnu_stack_index = CLIP( mnu_stack_index, 0, MENU_STACK_COUNT ) ;

    if ( mnu_stack_index >= MENU_STACK_COUNT ) return bfalse;

    mnu_stack[mnu_stack_index] = menu;
    mnu_stack_index++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
which_menu_t mnu_stack_pop()
{
    if ( mnu_stack_index < 0 )
    {
        mnu_stack_index = 0;
        return emnu_Main;
    }
    if ( mnu_stack_index > MENU_STACK_COUNT )
    {
        mnu_stack_index = MENU_STACK_COUNT;
    }

    if ( mnu_stack_index == 0 ) return emnu_Main;

    mnu_stack_index--;
    return mnu_stack[mnu_stack_index];
}

//--------------------------------------------------------------------------------------------
which_menu_t mnu_stack_peek()
{
    which_menu_t return_menu = emnu_Main;

    if ( mnu_stack_index > 0 )
    {
        return_menu = mnu_stack[mnu_stack_index-1];
    }

    return return_menu;
}

//--------------------------------------------------------------------------------------------
void mnu_stack_clear()
{
    mnu_stack_index = 0;
    mnu_stack[0] = emnu_Main;
}

//--------------------------------------------------------------------------------------------
// The implementation of the menu process
//--------------------------------------------------------------------------------------------
int do_menu_proc_beginning( menu_process_t * mproc )
{
    // play some music
    sound_play_song( MENU_SONG, 0, -1 );

    // initialize all these structures
    menu_system_begin();        // start the menu menu

    // load all module info at menu initialization
    // this will not change unless a new module is downloaded for a network menu?
    mnu_load_all_module_info();

    // initialize the process state
    mproc->base.valid = btrue;

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_running( menu_process_t * mproc )
{
    int menuResult;

    if ( !process_validate( PROC_PBASE( mproc ) ) ) return -1;

    mproc->was_active = mproc->base.valid;

    if ( mproc->base.paused ) return 0;

    // play the menu music
    mnu_draw_background = !process_running( PROC_PBASE( GProc ) );
    menuResult          = game_do_menu( mproc );

    switch ( menuResult )
    {
        case MENU_SELECT:
            // go ahead and start the game
            process_pause( PROC_PBASE( mproc ) );
            break;

        case MENU_QUIT:
            // the user selected "quit"
            process_kill( PROC_PBASE( mproc ) );
            break;
    }

    if ( mnu_get_menu_depth() <= GProc->menu_depth )
    {
        GProc->menu_depth   = -1;
        GProc->escape_latch = bfalse;

        // We have exited the menu and restarted the game
        GProc->mod_paused = bfalse;
        process_pause( PROC_PBASE( MProc ) );
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_leaving( menu_process_t * mproc )
{
    if ( !process_validate( PROC_PBASE( mproc ) ) ) return -1;

    // terminate the menu system
    menu_system_end();

    // finish the menu song
    sound_finish_song( 500 );

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_run( menu_process_t * mproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_validate( PROC_PBASE( mproc ) ) ) return -1;
    mproc->base.dtime = frameDuration;

    if ( mproc->base.paused ) return 0;

    if ( mproc->base.killme )
    {
        mproc->base.state = proc_leaving;
    }

    switch ( mproc->base.state )
    {
        case proc_beginning:
            proc_result = do_menu_proc_beginning( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            // proc_result = do_menu_proc_entering( mproc );

            mproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = do_menu_proc_running( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = do_menu_proc_leaving( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state  = proc_finishing;
                mproc->base.killme = bfalse;
            }
            break;

        case proc_finishing:
            process_terminate( PROC_PBASE( mproc ) );
            break;

        default:
        case proc_invalid:
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
menu_process_t * menu_process_init( menu_process_t * mproc )
{
    if ( NULL == mproc ) return NULL;

    memset( mproc, 0, sizeof( *mproc ) );

    process_init( PROC_PBASE( mproc ) );

    return mproc;
}

//--------------------------------------------------------------------------------------------
// Code for global initialization/deinitialization of the menu system
//--------------------------------------------------------------------------------------------
int menu_system_begin()
{
    // initializes the menu system
    //
    // Loads resources for the menus, and figures out where things should
    // be positioned.  If we ever allow changing resolution on the fly, this
    // function will have to be updated/called more than once.

    autoformat_init( &gfx );

    menuFont = ui_loadFont( vfs_resolveReadFilename( "mp_data/Bo_Chen.ttf" ), 18 );
    if ( NULL == menuFont )
    {
        log_error( "Could not load the menu font! (\"mp_data/Bo_Chen.ttf\")\n" );
        return 0;
    }

    // allocate the menuTextureList
    menuTextureList_ptr = display_list_ctor( menuTextureList_ptr, MAX_MENU_DISPLAY );

    // Figure out where to draw the copyright text
    copyrightText_set_position( menuFont, copyrightText, 20 );

    // Figure out where to draw the options text
    tipText_set_position( menuFont, tipText, 20 );

    // construct the TxTitleImage array
    TxTitleImage_ctor();

    // Load game hints
    mnu_GameTip_load_global_vfs();

    return 1;
}

//--------------------------------------------------------------------------------------------
void menu_system_end()
{
    // initializes the menu system
    //
    // Loads resources for the menus, and figures out where things should
    // be positioned.  If we ever allow changing resolution on the fly, this
    // function will have to be updated/called more than once.

    if ( NULL != menuFont )
    {
        fnt_freeFont( menuFont );
    }

    // deallocate the menuTextureList
    menuTextureList_ptr = display_list_dtor( menuTextureList_ptr, btrue );

    // destruct the TxTitleImage array
    TxTitleImage_dtor();
}

//--------------------------------------------------------------------------------------------
// Interface for starting and stopping menus
//--------------------------------------------------------------------------------------------
bool_t mnu_begin_menu( which_menu_t which )
{
    if ( !mnu_stack_push( mnu_whichMenu ) ) return bfalse;
    mnu_whichMenu = which;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void mnu_end_menu()
{
    mnu_whichMenu = mnu_stack_pop();
}

//--------------------------------------------------------------------------------------------
int mnu_get_menu_depth()
{
    return mnu_stack_index;
}

//--------------------------------------------------------------------------------------------
// Implementations of the various menus
//--------------------------------------------------------------------------------------------

#define BASE_MENU_STATE        \
    int            state;      \
    int            choice;     \
    oglx_texture_t background

#define MENU_BUTTONS(CNT)            \
    ui_Widget_t    w_buttons[CNT];   \
    const char    *sz_buttons[CNT+1]

#define MENU_LABELS(CNT)             \
    ui_Widget_t    w_labels[CNT];    \
    const char    *sz_labels[CNT+1]

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_MainState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_MainState MainState_t;
//
////--------------------------------------------------------------------------------------------
//static MainState_t *MainState_ctor( MainState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static MainState_t *MainState_dtor( MainState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static MainState_t * doMainMenu_begin( MainState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = MainState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static MainState_t * doMainMenu_entering( MainState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static MainState_t * doMainMenu_running( MainState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static MainState_t * doMainMenu_leaving( MainState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static MainState_t * doMainMenu_finish( MainState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = MainState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
int doMainMenu( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static oglx_texture_t logo;
    static SlidyButtonState_t but_state = { NULL };
    static int menuChoice = 0;
    static SDL_Rect bg_rect, logo_rect;

    int cnt;

    enum e_buttons
    {
        but_new,
        but_load,
        but_options,
        but_quit,
        but_count,
        but_sz_count
    };

    // button widgets
    static ui_Widget_t w_buttons[but_count];

    // Button labels.  Defined here for consistency's sake, rather than leaving them as constants
    static const char *sz_buttons[but_sz_count] =
    {
        "New Game",
        "Load Game",
        "Options",
        "Quit",
        ""
    };

    float fminw = 1, fminh = 1, fmin = 1;
    int result = 0;

    switch ( menuState )
    {
        case MM_Begin:

            menuChoice = 0;

            {
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    // clear the data
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_buttons + cnt, cnt );
                }

                // load the menu image
                ego_texture_load_vfs( &background, "mp_data/menu/menu_main", INVALID_KEY );

                // load the logo image
                ego_texture_load_vfs( &logo,       "mp_data/menu/menu_logo", INVALID_KEY );

                // calculate the centered position of the background
                fminw = ( float ) MIN( GFX_WIDTH , background.imgW ) / ( float ) background.imgW;
                fminh = ( float ) MIN( GFX_HEIGHT, background.imgH ) / ( float ) background.imgW;
                fmin  = MIN( fminw, fminh );

                bg_rect.w = background.imgW * fmin;
                bg_rect.h = background.imgH * fmin;
                bg_rect.x = ( GFX_WIDTH  - bg_rect.w ) * 0.5f;
                bg_rect.y = ( GFX_HEIGHT - bg_rect.h ) * 0.5f;

                // calculate the position of the logo
                fmin  = MIN( bg_rect.w * 0.5f / logo.imgW, bg_rect.h * 0.5f / logo.imgH );

                logo_rect.x = bg_rect.x;
                logo_rect.y = bg_rect.y;
                logo_rect.w = logo.imgW * fmin;
                logo_rect.h = logo.imgH * fmin;

                SlidyButtonState_init( &but_state, 1.0f, 0, sz_buttons, w_buttons );

                // "Copyright" text
                copyrightText_set_position( menuFont, copyrightText, 20 );
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, bg_rect.x,   bg_rect.y,   bg_rect.w,   bg_rect.h, NULL );
                    ui_drawImage( 0, &logo,       logo_rect.x, logo_rect.y, logo_rect.w, logo_rect.h, NULL );
                }

                // "Copyright" text
                display_list_draw( copyrightText_tx_ptr );

                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, -deltaTime );

                // Let lerp wind down relative to the time elapsed
                if ( but_state.lerp <= 0.0f )
                {
                    menuState = MM_Running;
                }
            }
            break;

        case MM_Running:

            // Do normal run
            {
                // Background
                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, bg_rect.x,   bg_rect.y,   bg_rect.w,   bg_rect.h, NULL );
                    ui_drawImage( 0, &logo,       logo_rect.x, logo_rect.y, logo_rect.w, logo_rect.h, NULL );
                }

                // "Copyright" text
                display_list_draw( copyrightText_tx_ptr );

                // Buttons
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_new ) )
                {
                    // begin single player stuff
                    menuChoice = 1;
                }
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_load ) )
                {
                    // begin multi player stuff
                    menuChoice = 2;
                }
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_options ) )
                {
                    // go to options menu
                    menuChoice = 3;
                }
                if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doWidget( w_buttons + but_quit ) )
                {
                    // quit game
                    menuChoice = 4;
                }
                if ( menuChoice != 0 )
                {
                    menuState = MM_Leaving;
                    SlidyButtonState_init( &but_state, 0.0f, 0, sz_buttons, w_buttons );
                }
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards

            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, bg_rect.x,   bg_rect.y,   bg_rect.w,   bg_rect.h, NULL );
                    ui_drawImage( 0, &logo,       logo_rect.x, logo_rect.y, logo_rect.w, logo_rect.h, NULL );
                }

                // "Copyright" text
                display_list_draw( copyrightText_tx_ptr );

                // Buttons
                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, deltaTime );
                if ( but_state.lerp >= 1.0f )
                {
                    menuState = MM_Finish;
                }
            }
            break;

        case MM_Finish:
            {
                // Free the background texture; don't need to hold onto it
                oglx_texture_Release( &background );

                // free the widgets
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    ui_Widget_free( w_buttons + cnt );
                }

                // Set the next menu to load
                result = menuChoice;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    };

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_SinglePlayerState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_SinglePlayerState SinglePlayerState_t;
//
////--------------------------------------------------------------------------------------------
//static SinglePlayerState_t *SinglePlayerState_ctor( SinglePlayerState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static SinglePlayerState_t *SinglePlayerState_dtor( SinglePlayerState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static SinglePlayerState_t * doSinglePlayerMenu_begin( SinglePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = SinglePlayerState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static SinglePlayerState_t * doSinglePlayerMenu_entering( SinglePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static SinglePlayerState_t * doSinglePlayerMenu_running( SinglePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static SinglePlayerState_t * doSinglePlayerMenu_leaving( SinglePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static SinglePlayerState_t * doSinglePlayerMenu_finish( SinglePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = SinglePlayerState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
int doSinglePlayerMenu( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int menuChoice;
    static SlidyButtonState_t but_state = { NULL };

    enum e_buttons
    {
        but_new,
        but_load,
        but_back,
        but_count,
        but_sz_count
    };

    // button widgets
    static ui_Widget_t w_buttons[but_count];

    static const char *sz_buttons[but_sz_count] =
    {
        "New Player",
        "Load Saved Player",
        "Back",
        ""
    };

    int result = 0;
    int cnt;

    switch ( menuState )
    {
        case MM_Begin:

            menuChoice = 0;

            {
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    // clear the data
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_buttons + cnt, cnt );
                }

                // Load resources for this menu
                ego_texture_load_vfs( &background, "mp_data/menu/menu_advent", TRANSCOLOR );

                SlidyButtonState_init( &but_state, 1.0f, 0, sz_buttons, w_buttons );

                // "Copyright" text
                copyrightText_set_position( menuFont, copyrightText, 20 );
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                // Draw the background image
                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0, NULL );
                }

                // "Copyright" text
                display_list_draw( copyrightText_tx_ptr );

                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, -deltaTime );
                if ( but_state.lerp <= 0.0f )
                {
                    menuState = MM_Running;
                }
            }
            break;

        case MM_Running:

            // Draw the background image
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0, NULL );
            }

            // "Copyright" text
            display_list_draw( copyrightText_tx_ptr );

            // Buttons
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_new ) )
            {
                menuChoice = 1;
            }
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_load ) )
            {
                menuChoice = 2;
            }
            if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doWidget( w_buttons + but_back ) )
            {
                menuChoice = 3;     // back
            }
            if ( menuChoice != 0 )
            {
                menuState = MM_Leaving;
                SlidyButtonState_init( &but_state, 0.0f, 0, sz_buttons, w_buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0, NULL );
                }

                // "Copyright" text
                display_list_draw( copyrightText_tx_ptr );

                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, deltaTime );
                if ( but_state.lerp >= 1.0f )
                {
                    menuState = MM_Finish;
                }
            }
            break;

        case MM_Finish:
            {
                // Release the background texture
                oglx_texture_Release( &background );

                // free the widgets
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    ui_Widget_free( w_buttons + cnt );
                }

                // Set the next menu to load
                result = menuChoice;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
static int cmp_mod_ref_mult = 1;

int cmp_mod_ref( const void * vref1, const void * vref2 )
{
    /// @details BB@> Sort MOD REF values based on the rank of the module that they point to.
    ///               Trap all stupid values.

    MOD_REF * pref1 = ( MOD_REF * )vref1;
    MOD_REF * pref2 = ( MOD_REF * )vref2;

    int retval = 0;

    if ( NULL == pref1 && NULL == pref2 )
    {
        return 0;
    }
    else if ( NULL == pref1 )
    {
        return 1;
    }
    else if ( NULL == pref2 )
    {
        return -1;
    }

    if ( *pref1 > mnu_ModList.count && *pref2 > mnu_ModList.count )
    {
        return 0;
    }
    else if ( *pref1 > mnu_ModList.count )
    {
        return 1;
    }
    else if ( *pref2 > mnu_ModList.count )
    {
        return -1;
    }

    // if they are beaten, float them to the end of the list
    retval = ( int )mnu_ModList.lst[*pref1].base.beaten - ( int )mnu_ModList.lst[*pref2].base.beaten;

    if ( 0 == retval )
    {
        // I want to uot the "newest" == "hardest" modules at the front, but this should be opposite for
        // beginner modules
        retval = cmp_mod_ref_mult * strncmp( mnu_ModList.lst[*pref1].base.rank, mnu_ModList.lst[*pref2].base.rank, RANKSIZE );
    }

    if ( 0 == retval )
    {
        retval = strncmp( mnu_ModList.lst[*pref1].base.longname, mnu_ModList.lst[*pref2].base.longname, sizeof( STRING ) );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_ChooseModuleState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_ChooseModuleState ChooseModuleState_t;
//
////--------------------------------------------------------------------------------------------
//static ChooseModuleState_t *ChooseModuleState_ctor( ChooseModuleState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static ChooseModuleState_t *ChooseModuleState_dtor( ChooseModuleState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static ChooseModuleState_t * doChooseModule_begin( ChooseModuleState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = ChooseModuleState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ChooseModuleState_t * doChooseModule_entering( ChooseModuleState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ChooseModuleState_t * doChooseModule_running( ChooseModuleState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ChooseModuleState_t * doChooseModule_leaving( ChooseModuleState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ChooseModuleState_t * doChooseModule_finish( ChooseModuleState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = ChooseModuleState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
bool_t doChooseModule_update_description( ui_Widget_t * lab_ptr, MOD_REF validModules[], size_t validModules_size, int selectedModule )
{
    int i;

    if ( NULL == lab_ptr ) return bfalse;

    if ( selectedModule < 0 || ( size_t )selectedModule >= validModules_size || validModules[selectedModule] <= 0 || selectedModule > validModules[selectedModule] )
    {
        ui_Widget_set_text( lab_ptr, ui_just_topleft, menuFont, NULL );
    }
    else
    {
        char    buffer[1024]  = EMPTY_CSTR;
        const char * rank_string, * name_string;
        char  * carat = buffer, * carat_end = buffer + SDL_arraysize( buffer );
        MOD_REF imodule = validModules[selectedModule];

        mod_file_t * pmod = &( mnu_ModList.lst[imodule].base );

        name_string = "Unnamed";
        if ( CSTR_END != pmod->longname[0] )
        {
            name_string = pmod->longname;
        }
        carat += snprintf( carat, carat_end - carat - 1, "%s\n", name_string );

        rank_string = "Unranked";
        if ( CSTR_END != pmod->rank[0] )
        {
            rank_string = pmod->rank;
        }
        carat += snprintf( carat, carat_end - carat - 1, "Difficulty: %s\n", rank_string );

        if ( pmod->maxplayers > 1 )
        {
            if ( pmod->minplayers == pmod->maxplayers )
            {
                carat += snprintf( carat, carat_end - carat - 1, "%d Players\n", pmod->minplayers );
            }
            else
            {
                carat += snprintf( carat, carat_end - carat - 1, "%d - %d Players\n", pmod->minplayers, pmod->maxplayers );
            }
        }
        else
        {
            if ( 0 != pmod->importamount )
            {
                carat += snprintf( carat, carat_end - carat - 1, "Single Player\n" );
            }
            else
            {
                carat += snprintf( carat, carat_end - carat - 1, "Starter Module\n" );
            }
        }
        carat += snprintf( carat, carat_end - carat - 1, " \n" );

        for ( i = 0; i < SUMMARYLINES; i++ )
        {
            carat += snprintf( carat, carat_end - carat - 1, "%s\n", pmod->summary[i] );
        }

        // Draw a text box
        GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
        ui_Widget_set_text( lab_ptr, ui_just_topleft, menuFont, buffer );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t doChooseModule_update_filter_label( ui_Widget_t * lab_ptr, int which )
{
    if ( NULL == lab_ptr ) return bfalse;

    switch ( which )
    {
        case FILTER_MAIN:    ui_Widget_set_text( lab_ptr, ui_just_centerleft, menuFont, "Main Quest" );       break;
        case FILTER_SIDE:    ui_Widget_set_text( lab_ptr, ui_just_centerleft, menuFont, "Sidequests" );       break;
        case FILTER_TOWN:    ui_Widget_set_text( lab_ptr, ui_just_centerleft, menuFont, "Towns and Cities" ); break;
        case FILTER_FUN:     ui_Widget_set_text( lab_ptr, ui_just_centerleft, menuFont, "Fun Modules" );      break;
        case FILTER_DEBUG:   ui_Widget_set_text( lab_ptr, ui_just_centerleft, menuFont, "Debugging" );        break;
        case FILTER_STARTER: ui_Widget_set_text( lab_ptr, ui_just_centerleft, menuFont, "Starter Modules" );  break;
        default:
        case FILTER_OFF:     ui_Widget_set_text( lab_ptr, ui_just_centerleft, menuFont, "All Modules" );      break;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int doChooseModule( float deltaTime )
{
    /// @details Choose the module

    static oglx_texture_t background;
    static int menuState = MM_Begin;
    static int startIndex;
    static Uint8 keycooldown = 0;

    static int numValidModules;
    static MOD_REF validModules[MAX_MODULE];

    static int moduleMenuOffsetX;
    static int moduleMenuOffsetY;

    static display_list_t * description_lst_ptr = NULL;
    static display_list_t * tip_lst_ptr = NULL;
    static display_item_t     * beaten_tx_ptr = NULL;

    enum e_buttons
    {
        but_filter_fwd,
        but_fwd,
        but_bck,
        but_select,
        but_exit,
        but_count,
        but_sz_count
    };

    // initial button strings
    static const char * sz_buttons[but_sz_count] =
    {
        ">",             // but_filter_fwd
        "->",            // but_fwd
        "<-",            // but_bck
        "Select Module", // but_select
        "Back",          // but_exit
        ""
    };

    enum e_labels
    {
        lab_description,
        lab_filter,
        lab_count,
        lab_sz_count
    };

    // initial button strings
    static const char * sz_labels[lab_sz_count] =
    {
        "",               // lab_description
        "All Modules",    // lab_filter
        NULL
    };

    static ui_Widget_t w_buttons[but_count];
    static ui_Widget_t w_labels [lab_count];

    static int    selectedModule_old    = -1;
    static bool_t selectedModule_update = bfalse;

    int result = 0;
    int i, x, y;
    MOD_REF imod;

    switch ( menuState )
    {
        case MM_Begin:
            {

                if ( !module_list_valid )
                {
                    mnu_load_all_module_info();
                }

                // allocate the list for the discription
                display_list_ctor( description_lst_ptr, MAX_MENU_DISPLAY );

                // allocate the list for the tip
                display_list_ctor( tip_lst_ptr, MAX_MENU_DISPLAY );

                // Reload all modules, something might be unlocked
                mnu_load_all_module_images_vfs();

                // Reset which module we are selecting
                startIndex            = 0;
                selectedModule        = -1;
                selectedModule_old    = -1;
                selectedModule_update = bfalse;
                keycooldown           = 0;

                // blank out the valid modules
                numValidModules = 0;
                for ( i = 0; i < MAX_MODULE; i++ )
                {
                    memset( validModules + i, 0, sizeof( MOD_REF ) );
                }

                // initialize the buttons
                for ( i = 0; i < but_count; i++ )
                {
                    memset( w_buttons + i, 0, sizeof( ui_Widget_t ) );

                    ui_Widget_set_id( w_buttons + i, i );
                    ui_Widget_set_text( w_buttons + i, ui_just_centerleft, NULL, sz_buttons[i] );
                }

                // initialize the labels
                for ( i = 0; i < lab_count; i++ )
                {
                    memset( w_labels + i, 0, sizeof( ui_Widget_t ) );

                    ui_Widget_set_id( w_labels + i, UI_Nothing );
                    ui_Widget_set_text( w_labels + i, ui_just_centerleft, NULL, sz_labels[i] );
                }

                // Figure out at what offset we want to draw the module menu.
                moduleMenuOffsetX = ( GFX_WIDTH  - 640 ) / 2;
                moduleMenuOffsetX = MAX( 0, moduleMenuOffsetX );

                moduleMenuOffsetY = ( GFX_HEIGHT - 480 ) / 2;
                moduleMenuOffsetY = MAX( 0, moduleMenuOffsetY );

                // set the widget positions
                ui_Widget_set_button( w_buttons + but_bck, moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, -1, 30 );
                ui_Widget_set_button( w_buttons + but_fwd, moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, -1, 30 );
                ui_Widget_set_button( w_buttons + but_exit, moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30 );
                ui_Widget_set_button( w_buttons + but_select, moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30 );
                ui_Widget_set_button( w_buttons + but_filter_fwd, moduleMenuOffsetX + 532, moduleMenuOffsetY + 390, -1, -1 );

                // set the label positions
                ui_Widget_set_bound( w_labels + lab_filter, moduleMenuOffsetX + 327, moduleMenuOffsetY + 390, 200, 30 );
                ui_Widget_set_button( w_labels + lab_description, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 250 );

                // initialize the module description
                doChooseModule_update_description( w_labels + lab_description, validModules, SDL_arraysize( validModules ), selectedModule );

                // initialize the module filter text
                doChooseModule_update_filter_label( w_labels + lab_filter, mnu_moduleFilter );

                beaten_tx_ptr = ui_updateText( beaten_tx_ptr, menuFont, 0, 0, "BEATEN" );
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            {
                if ( !module_list_valid )
                {
                    mnu_load_all_module_info();
                    mnu_load_all_module_images_vfs();
                }

                // Find the modules that we want to allow loading for.  If start_new_player
                // is true, we want ones that don't allow imports (e.g. starter modules).
                // Otherwise, we want modules that allow imports
                numValidModules = 0;
                for ( imod = 0; imod < mnu_ModList.count; imod++ )
                {
                    // if this module is not valid given the game options and the
                    // selected players, skip it
                    if ( !mnu_test_by_index( imod, 0, NULL ) ) continue;

                    if ( start_new_player && 0 == mnu_ModList.lst[imod].base.importamount )
                    {
                        if ( FILTER_HIDDEN == mnu_ModList.lst[imod].base.moduletype )  continue;

                        // starter module
                        validModules[numValidModules] = REF_TO_INT( imod );
                        numValidModules++;
                    }
                    else
                    {
                        if ( FILTER_HIDDEN == mnu_ModList.lst[imod].base.moduletype )  continue;
                        if ( FILTER_OFF != mnu_moduleFilter && mnu_ModList.lst[imod].base.moduletype != mnu_moduleFilter ) continue;
                        if ( mnu_selectedPlayerCount > mnu_ModList.lst[imod].base.importamount ) continue;
                        if ( mnu_selectedPlayerCount < mnu_ModList.lst[imod].base.minplayers ) continue;
                        if ( mnu_selectedPlayerCount > mnu_ModList.lst[imod].base.maxplayers ) continue;

                        // regular module
                        validModules[numValidModules] = REF_TO_INT( imod );
                        numValidModules++;
                    }
                }

                // sort the modules by difficulty. easiest to hardest for starting a new character
                // hardest to easiest for loading a module
                cmp_mod_ref_mult = start_new_player ? 1 : -1;
                qsort( validModules, numValidModules, sizeof( MOD_REF ), cmp_mod_ref );

                // load background depending on current filter
                if ( start_new_player )
                {
                    ego_texture_load_vfs( &background, "mp_data/menu/menu_advent", TRANSCOLOR );
                }
                else
                {
                    switch ( mnu_moduleFilter )
                    {
                        case FILTER_MAIN: ego_texture_load_vfs( &background, "mp_data/menu/menu_draco", TRANSCOLOR ); break;
                        case FILTER_SIDE: ego_texture_load_vfs( &background, "mp_data/menu/menu_sidequest", TRANSCOLOR ); break;
                        case FILTER_TOWN: ego_texture_load_vfs( &background, "mp_data/menu/menu_town", TRANSCOLOR ); break;
                        case FILTER_FUN:  ego_texture_load_vfs( &background, "mp_data/menu/menu_funquest", TRANSCOLOR ); break;

                        default:
                        case FILTER_OFF: ego_texture_load_vfs( &background, "mp_data/menu/menu_allquest", TRANSCOLOR ); break;
                    }
                }

                // set the tip text
                if ( 0 == numValidModules )
                {
                    tipText_set_position( menuFont, "Sorry, there are no valid games!\n Please press the \"Back\" button.", 20 );
                }
                else if ( numValidModules <= 3 )
                {
                    tipText_set_position( menuFont, "Press an icon to select a game.", 20 );
                }
                else
                {
                    tipText_set_position( menuFont, "Press an icon to select a game.\nUse the mouse wheel or the \"<-\" and \"->\" buttons to scroll.", 20 );
                }

                // set the position of the module description "button"
                ui_Widget_set_button( w_labels + lab_description, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230 );
            }
            menuState = MM_Running;
            // fall through for now...

        case MM_Running:
            {
                GLXvector4f beat_tint = { 0.5f, 0.25f, 0.25f, 1.0f };
                GLXvector4f normal_tint = { 1.0f, 1.0f, 1.0f, 1.0f };

                if ( !module_list_valid )
                {
                    mnu_load_all_module_info();
                    mnu_load_all_module_images_vfs();
                }

                // Draw the background
                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
                x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
                y = GFX_HEIGHT - background.imgH;

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, x, y, 0, 0, NULL );
                }

                // use the mouse wheel to scan the modules
                if ( cursor_wheel_event_pending() )
                {
                    if ( cursor.z > 0 )
                    {
                        startIndex++;
                    }
                    else if ( cursor.z < 0 )
                    {
                        startIndex--;
                    }

                    cursor_finish_wheel_event();
                }

                // Allow arrow keys to scroll as well
                if ( SDLKEYDOWN( SDLK_RIGHT ) )
                {
                    if ( keycooldown == 0 )
                    {
                        startIndex++;
                        keycooldown = 5;
                    }
                }
                else if ( SDLKEYDOWN( SDLK_LEFT ) )
                {
                    if ( keycooldown == 0 )
                    {
                        startIndex--;
                        keycooldown = 5;
                    }
                }
                else keycooldown = 0;
                if ( keycooldown > 0 ) keycooldown--;

                // Draw the arrows to pick modules
                if ( numValidModules > 3 )
                {
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_bck ) )
                    {
                        startIndex--;
                    }
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_fwd ) )
                    {
                        startIndex++;
                    }
                }

                // restrict the range to valid values
                startIndex = CLIP( startIndex, 0, numValidModules - 3 );

                // Draw buttons for the modules that can be selected
                x = 93;
                y = 20;
                for ( i = startIndex; i < MIN( startIndex + 3, numValidModules ); i++ )
                {
                    // fix the menu images in case one or more of them are undefined
                    MOD_REF          imod       = validModules[i];
                    TX_REF           tex_offset = mnu_ModList.lst[imod].tex_index;
                    oglx_texture_t * ptex       = TxTitleImage_get_ptr( tex_offset );

                    GLfloat * img_tint = normal_tint;

                    // was the module beaten?
                    if ( mnu_ModList.lst[imod].base.beaten )
                    {
                        img_tint = beat_tint;
                    }

                    // move the button id up by 1000, so it will not interfere with the other buttons...
                    // ?? another option would be to make these widgets ??
                    if ( BUTTON_DOWN == ui_doImageButton( 1000 + i, ptex, moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138, img_tint ) )
                    {
                        selectedModule_old = selectedModule;
                        selectedModule     = i;
                    }

                    // Draw a text over the image explaining what it means
                    if ( mnu_ModList.lst[imod].base.beaten )
                    {
                        ui_drawText( beaten_tx_ptr, moduleMenuOffsetX + x + 32, moduleMenuOffsetY + y + 64 );
                    }

                    x += 138 + 20;  // Width of the button, and the spacing between buttons
                }

                // Draw the text description of the selected module
                if ( selectedModule_old != selectedModule )
                {
                    selectedModule_old = selectedModule;

                    doChooseModule_update_description( w_labels + lab_description, validModules, SDL_arraysize( validModules ), selectedModule );
                }

                // draw the description
                ui_doWidget( w_labels + lab_description );

                // And draw the next & back buttons
                if ( selectedModule > -1 )
                {
                    if ( SDLKEYDOWN( SDLK_RETURN ) || BUTTON_UP == ui_doWidget( w_buttons + but_select ) )
                    {
                        // go to the next menu with this module selected
                        selectedModule = REF_TO_INT( validModules[selectedModule] );
                        menuState = MM_Leaving;
                    }
                }

                if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doWidget( w_buttons + but_exit ) )
                {
                    // Signal doMenu to go back to the previous menu
                    selectedModule = -1;
                    menuState = MM_Leaving;
                }

                // Do the module filter button
                if ( !start_new_player )
                {
                    bool_t click_button;

                    // display the filter name
                    ui_doWidget( w_labels + lab_filter );

                    // use the ">" button to change since we are already using arrows to indicate "spin control"-like widgets
                    click_button = ( BUTTON_UP == ui_doWidget( w_buttons + but_filter_fwd ) );

                    if ( click_button )
                    {
                        // Reload the modules with the new filter
                        menuState = MM_Entering;

                        // Swap to the next filter
                        if ( cfg.dev_mode )
                        {
                            mnu_moduleFilter = CLIP( mnu_moduleFilter, FILTER_NORMAL_BEGIN, FILTER_SPECIAL_END );
                            mnu_moduleFilter++;
                            if( mnu_moduleFilter > FILTER_SPECIAL_END ) mnu_moduleFilter = FILTER_NORMAL_BEGIN;
                        }
                        else
                        {
                            mnu_moduleFilter = CLIP( mnu_moduleFilter, FILTER_NORMAL_BEGIN, FILTER_NORMAL_END );
                            mnu_moduleFilter++;
                            if( mnu_moduleFilter > FILTER_NORMAL_END ) mnu_moduleFilter = FILTER_NORMAL_BEGIN;
                        }

                        doChooseModule_update_filter_label( w_labels + lab_filter, mnu_moduleFilter );
                    }
                }
            }
            break;

        case MM_Leaving:

            menuState = MM_Finish;
            // fall through for now

        case MM_Finish:
            {
                oglx_texture_Release( &background );

                // deallocate the list for the discription
                display_list_ctor( description_lst_ptr, MAX_MENU_DISPLAY );

                // initialize dthe buttons
                for ( i = 0; i < but_count; i++ )
                {
                    ui_Widget_free( w_buttons + i );
                }

                // initialize the labels
                for ( i = 0; i < lab_count; i++ )
                {
                    ui_Widget_free( w_labels + i );
                }

                // free all other textures
                beaten_tx_ptr = display_item_free( beaten_tx_ptr, btrue );;

                // deallocate the list for the tip
                display_list_ctor( tip_lst_ptr, MAX_MENU_DISPLAY );

                pickedmodule_index         = -1;
                pickedmodule_path[0]       = CSTR_END;
                pickedmodule_name[0]       = CSTR_END;
                pickedmodule_write_path[0] = CSTR_END;

                if ( selectedModule == -1 )
                {
                    result = -1;
                }
                else
                {
                    // Save the name of the module that we've picked
                    pickedmodule_index = selectedModule;

                    strncpy( pickedmodule_path,       mnu_ModList_get_vfs_path( pickedmodule_index ), SDL_arraysize( pickedmodule_path ) );
                    strncpy( pickedmodule_name,       mnu_ModList_get_name( pickedmodule_index ), SDL_arraysize( pickedmodule_name ) );
                    strncpy( pickedmodule_write_path, mnu_ModList_get_dest_path( pickedmodule_index ), SDL_arraysize( pickedmodule_write_path ) );

                    if ( !game_choose_module( selectedModule, -1 ) )
                    {
                        log_warning( "Tried to select an invalid module. index == %d\n", selectedModule );
                        result = -1;
                    }
                    else
                    {
                        pickedmodule_ready = btrue;
                        result = ( PMod->importamount > 0 ) ? 1 : 2;
                    }
                }
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
bool_t doChoosePlayer_load_profiles( int player, ChoosePlayer_profiles_t * pro_list )
{
    int    i;
    CAP_REF ref_temp;
    STRING  szFilename;
    ChoosePlayer_element_t * pdata;

    // release all of the temporary profiles
    release_all_profiles();
    overrideslots = btrue;

    if ( 0 == bookicon_count )
    {
        load_one_profile_vfs( "mp_data/globalobjects/book.obj", SPELLBOOK );
    }

    // release any data that we have accumulated
    for ( i = 0; i < pro_list->count; i++ )
    {
        pdata = pro_list->pro_data + i;

        TxTexture_free_one( pdata->tx_ref );

        // initialize the data
        pdata->cap_ref = MAX_CAP;
        pdata->tx_ref  = INVALID_TX_TEXTURE;
        chop_definition_init( &( pdata->chop ) );
    }
    pro_list->count = 0;

    if ( player < 0 || player >= MAXLOADPLAYER || player >= loadplayer_count ) return bfalse;

    // grab the player data
    ref_temp = load_one_character_profile_vfs( loadplayer[player].dir, 0, bfalse );
    if ( !LOADED_CAP( ref_temp ) )
    {
        return bfalse;
    }

    // go to the next element in the list
    pdata = pro_list->pro_data + pro_list->count;
    pro_list->count++;

    // set the index of this object
    pdata->cap_ref = ref_temp;

    // grab the inventory data
    for ( i = 0; i < MAXIMPORTOBJECTS; i++ )
    {
        int slot = i + 1;

        snprintf( szFilename, SDL_arraysize( szFilename ), "%s/%d.obj", loadplayer[player].dir, i );

        // load the profile
        ref_temp = load_one_character_profile_vfs( szFilename, slot, bfalse );
        if ( LOADED_CAP( ref_temp ) )
        {
            cap_t * pcap = CapStack.lst + ref_temp;

            // go to the next element in the list
            pdata = pro_list->pro_data + pro_list->count;
            pro_list->count++;

            pdata->cap_ref = ref_temp;

            // load the icon
            snprintf( szFilename, SDL_arraysize( szFilename ), "%s/%d.obj/icon%d", loadplayer[player].dir, i, MAX( 0, pcap->skin_override ) );
            pdata->tx_ref = TxTexture_load_one_vfs( szFilename, ( TX_REF )INVALID_TX_TEXTURE, INVALID_KEY );

            // load the naming
            snprintf( szFilename, SDL_arraysize( szFilename ), "%s/%d.obj/naming.txt", loadplayer[player].dir, i );
            chop_load_vfs( &chop_mem, szFilename, &( pdata->chop ) );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_doChoosePlayer_stats_info
{
    ChoosePlayer_profiles_t objects;
    int                     player;
    int                     player_last;
    display_item_t        * item_ptr;
};

typedef struct s_doChoosePlayer_stats_info doChoosePlayer_stats_info_t;

#define PLAYER_STATS_INFO_INIT                  \
    {                                               \
        /* objects     */   PLAYER_PROFILES_INIT,   \
        /* player      */   -1,                     \
        /* player_last */   -1,                     \
        /* item_ptr    */   NULL                    \
    }

//--------------------------------------------------------------------------------------------
doChoosePlayer_stats_info_t * doChoosePlayer_stats_info_ctor( doChoosePlayer_stats_info_t * ptr )
{
    if ( NULL == ptr )
    {
        ptr = EGOBOO_NEW( doChoosePlayer_stats_info_t );
    }

    // clear the data
    memset( ptr, 0, sizeof( *ptr ) );

    // set special data
    ptr->player      = -1;
    ptr->player_last = -1;

    return ptr;
}

//--------------------------------------------------------------------------------------------
doChoosePlayer_stats_info_t * doChoosePlayer_stats_info_dtor( doChoosePlayer_stats_info_t * ptr, bool_t owner )
{
    if ( NULL == ptr ) return ptr;

    ptr->item_ptr   = display_item_free( ptr->item_ptr, btrue );

    if ( !owner )
    {
        ptr = doChoosePlayer_stats_info_ctor( ptr );
    }
    else
    {
        EGOBOO_DELETE( ptr );
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_ChoosePlayerState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_ChoosePlayerState ChoosePlayerState_t;
//
////--------------------------------------------------------------------------------------------
//static ChoosePlayerState_t *ChoosePlayerState_ctor( ChoosePlayerState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static ChoosePlayerState_t *ChoosePlayerState_dtor( ChoosePlayerState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static ChoosePlayerState_t * doChoosePlayer_begin( ChoosePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = ChoosePlayerState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ChoosePlayerState_t * doChoosePlayer_entering( ChoosePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ChoosePlayerState_t * doChoosePlayer_running( ChoosePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ChoosePlayerState_t * doChoosePlayer_leaving( ChoosePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ChoosePlayerState_t * doChoosePlayer_finish( ChoosePlayerState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = ChoosePlayerState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
doChoosePlayer_stats_info_t * doChoosePlayer_load_stats( doChoosePlayer_stats_info_t * ptr, int player, int mode )
{
    if ( NULL == ptr )
    {
        ptr = doChoosePlayer_stats_info_ctor( ptr );
    }

    if ( NULL == ptr ) return ptr;

    ptr->player_last = ptr->player;
    ptr->player      = player;

    if ( ptr->player < 0 ) mode = 1;

    // handle the profile data
    switch ( mode )
    {
        case 0: // load new ptr->player data

            if ( !doChoosePlayer_load_profiles( ptr->player, &ptr->objects ) )
            {
                ptr->player_last = ptr->player;
                ptr->player      = -1;
            }
            break;

        case 1: // unload player data

            ptr->player_last   = ptr->player;
            ptr->player        = -1;
            ptr->objects.count = 0;

            // release all of the temporary profiles
            release_all_profiles();

            break;
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
doChoosePlayer_stats_info_t * doChoosePlayer_render_stats( doChoosePlayer_stats_info_t * ptr, int x, int y, int width, int height )
{
    int i, x1, y1;

    GLuint list_index = INVALID_GL_ID;

    if ( NULL == ptr )
    {
        ptr = doChoosePlayer_stats_info_ctor( ptr );
    }

    if ( NULL == ptr ) return ptr;

    // make sure we have a valid display_item
    if ( NULL == ptr->item_ptr )
    {
        ptr->item_ptr = display_item_new();
    }

    // make sure that the ptr->item_ptr is primed for a display list
    ptr->item_ptr = display_item_validate_list( ptr->item_ptr );

    // is the list_name valid?
    list_index = display_item_list_name( ptr->item_ptr );
    if ( INVALID_GL_ID == list_index )
    {
        // there was an error. we have no use for the ptr->item_ptr
        ptr->item_ptr = display_item_free( ptr->item_ptr, btrue );
    }

    // start capturing the ogl commands
    if ( INVALID_GL_ID != list_index )
    {
        glNewList( list_index, GL_COMPILE );
    }

    {
        // do the actual display
        x1 = x + 25;
        y1 = y + 25;
        if ( ptr->player >= 0 && ptr->objects.count > 0 )
        {
            CAP_REF icap = ptr->objects.pro_data[0].cap_ref;

            if ( LOADED_CAP( icap ) )
            {
                cap_t * pcap = CapStack.lst + icap;
                Uint8 skin = ( Uint8 )MAX( 0, pcap->skin_override );

                ui_drawButton( UI_Nothing, x, y, width, height, NULL );

                // Character level and class
                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

                // fix class name capitalization
                pcap->classname[0] = toupper( pcap->classname[0] );
                y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "A level %d %s", pcap->level_override + 1, pcap->classname );

                // Armor
                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
                y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 40, "Wearing %s %s", pcap->skinname[skin], HAS_SOME_BITS( pcap->skindressy, 1 << skin ) ? "(Light)" : "(Heavy)" );

                // Life and mana (can be less than maximum if not in easy mode)
                if ( cfg.difficulty >= GAME_NORMAL )
                {
                    y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "Life: %d/%d", MIN(( signed )UFP8_TO_UINT( pcap->life_spawn ), ( int )pcap->life_stat.val.from ), ( int )pcap->life_stat.val.from );
                    y1 = draw_one_bar( pcap->lifecolor, x1, y1, ( signed )UFP8_TO_UINT( pcap->life_spawn ), ( int )pcap->life_stat.val.from );

                    if ( pcap->mana_stat.val.from > 0 )
                    {
                        y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "Mana: %d/%d", MIN(( signed )UFP8_TO_UINT( pcap->mana_spawn ), ( int )pcap->mana_stat.val.from ), ( int )pcap->mana_stat.val.from );
                        y1 = draw_one_bar( pcap->manacolor, x1, y1, ( signed )UFP8_TO_UINT( pcap->mana_spawn ), ( int )pcap->mana_stat.val.from );
                    }
                }
                else
                {
                    y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "Life: %d", ( int )pcap->life_stat.val.from );
                    y1 = draw_one_bar( pcap->lifecolor, x1, y1, ( int )pcap->life_stat.val.from, ( int )pcap->life_stat.val.from );

                    if ( pcap->mana_stat.val.from > 0 )
                    {
                        y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "Mana: %d", ( int )pcap->mana_stat.val.from );
                        y1 = draw_one_bar( pcap->manacolor, x1, y1, ( int )pcap->mana_stat.val.from, ( int )pcap->mana_stat.val.from );
                    }
                }
                y1 += 20;

                // SWID
                y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "Stats" );
                y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "  Str: %s (%d)", describe_value( pcap->strength_stat.val.from,     60, NULL ), ( int )pcap->strength_stat.val.from );
                y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "  Wis: %s (%d)", describe_value( pcap->wisdom_stat.val.from,       60, NULL ), ( int )pcap->wisdom_stat.val.from );
                y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "  Int: %s (%d)", describe_value( pcap->intelligence_stat.val.from, 60, NULL ), ( int )pcap->intelligence_stat.val.from );
                y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "  Dex: %s (%d)", describe_value( pcap->dexterity_stat.val.from,    60, NULL ), ( int )pcap->dexterity_stat.val.from );

                y1 += 20;

                if ( ptr->objects.count > 1 )
                {
                    ChoosePlayer_element_t * pdata;

                    y1 = ui_drawTextBoxImmediate( menuFont, x1, y1, 20, "Inventory" );

                    for ( i = 1; i < ptr->objects.count; i++ )
                    {
                        pdata = ptr->objects.pro_data + i;

                        icap = pdata->cap_ref;
                        if ( LOADED_CAP( icap ) )
                        {
                            const int icon_indent = 8;
                            TX_REF  icon_ref;
                            STRING itemname;
                            cap_t * pcap = CapStack.lst + icap;

                            if ( pcap->nameknown ) strncpy( itemname, chop_create( &chop_mem, &( pdata->chop ) ), SDL_arraysize( itemname ) );
                            else                   strncpy( itemname, pcap->classname,   SDL_arraysize( itemname ) );

                            icon_ref = mnu_get_icon_ref( icap, pdata->tx_ref );
                            draw_one_icon( icon_ref, x1 + icon_indent, y1, NOSPARKLE );

                            if ( icap == SLOT_LEFT + 1 )
                            {
                                ui_drawTextBoxImmediate( menuFont, x1 + icon_indent + ICON_SIZE, y1 + 6, ICON_SIZE, "  Left: %s", itemname );
                                y1 += ICON_SIZE;
                            }
                            else if ( icap == SLOT_RIGHT + 1 )
                            {
                                ui_drawTextBoxImmediate( menuFont, x1 + icon_indent + ICON_SIZE, y1 + 6, ICON_SIZE, "  Right: %s", itemname );
                                y1 += ICON_SIZE;
                            }
                            else
                            {
                                ui_drawTextBoxImmediate( menuFont, x1 + icon_indent + ICON_SIZE, y1 + 6, ICON_SIZE, "  Item: %s", itemname );
                                y1 += ICON_SIZE;
                            }
                        }
                    }
                }
            }
        }
    }

    if ( INVALID_GL_ID != list_index )
    {
        GL_DEBUG_END_LIST();
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
void doChoosePlayer_update_players( int numVertical, int startIndex, ui_Widget_t lst[], size_t lst_size )
{
    int lplayer, splayer;
    int i;

    for ( i = 0; i < numVertical; i++ )
    {
        size_t m = i * 5;
        if ( m > lst_size ) continue;

        lplayer = i + startIndex;
        if ( lplayer >= loadplayer_count ) continue;

        splayer = mnu_Selected_get_loadplayer( lplayer );

        // set the character image
        ui_Widget_set_img( lst + m, ui_just_centered, TxTexture_get_ptr( loadplayer[lplayer].tx_ref ) );

        // set the character name
        ui_Widget_set_text( lst + m, ui_just_centered, menuFont, loadplayer[lplayer].name );
    }
}

//--------------------------------------------------------------------------------------------
bool_t doChoosePlayer_update_select_button( ui_Widget_t * but_ptr, int count )
{
    if ( NULL == but_ptr ) return bfalse;

    if ( count <= 0 )
    {
        ui_Widget_set_text( but_ptr, ui_just_centered, NULL, NULL );
    }
    else if ( 1 == count )
    {
        ui_Widget_set_text( but_ptr, ui_just_centered, NULL, "Select Player" );
    }
    else
    {
        ui_Widget_set_text( but_ptr, ui_just_centered, NULL, "Select Players" );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int doChoosePlayer( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static SlidyButtonState_t but_state = { NULL };

    static int    startIndex_old = 0, startIndex = 0;
    static int    last_player_old = -1;
    static int    last_player     = -1;
    static bool_t new_player = bfalse;

    const int x0 = 20, y0 = 20, icon_size = 42, text_width = 175, button_repeat = 47;

    static int numVertical, numHorizontal;
    static Uint32 BitsInput[4];
    static bool_t device_on[4];

    enum e_buttons
    {
        but_select,
        but_back,
        but_count,
        but_sz_count
    };

    // button widgets
    static ui_Widget_t  w_buttons[but_count];

    static const char * sz_buttons[but_sz_count] =
    {
        "N/A",          // but_select
        "Back",         // but_back
        ""
    };

    static int mnu_selectedPlayerCount_old = -1;

    static doChoosePlayer_stats_info_t stats_info = PLAYER_STATS_INFO_INIT;

    int result = 0;
    int i, j, x, y;
    STRING srcDir, destDir;
    int cnt;

    switch ( menuState )
    {
        case MM_Begin:
            {
                doChoosePlayer_stats_info_ctor( &stats_info );

                last_player_old = -1;
                last_player     = -1;

                for ( cnt = 0; cnt < WIDGET_MAX; cnt++ )
                {
                    memset( mnu_widgetList + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( mnu_widgetList + cnt, 1000 + cnt );
                }

                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_buttons + cnt, cnt );
                    ui_Widget_set_text( w_buttons + cnt, ui_just_centerleft, NULL, sz_buttons[cnt] );
                }

                TxTexture_free_one(( TX_REF )TX_BARS );

                startIndex_old              = 0;
                startIndex                  = 0;
                mnu_selectedPlayerCount_old = 0;
                mnu_selectedPlayerCount     = 0;
                mnu_selectedPlayer[0]       = 0;

                TxTexture_load_one_vfs( "mp_data/nullicon", ( TX_REF )ICON_NULL, INVALID_KEY );

                TxTexture_load_one_vfs( "mp_data/keybicon", ( TX_REF )ICON_KEYB, INVALID_KEY );
                BitsInput[0] = INPUT_BITS_KEYBOARD;
                device_on[0] = keyb.on;

                TxTexture_load_one_vfs( "mp_data/mousicon", ( TX_REF )ICON_MOUS, INVALID_KEY );
                BitsInput[1] = INPUT_BITS_MOUSE;
                device_on[1] = mous.on;

                TxTexture_load_one_vfs( "mp_data/joyaicon", ( TX_REF )ICON_JOYA, INVALID_KEY );
                BitsInput[2] = INPUT_BITS_JOYA;
                device_on[2] = joy[0].on;

                TxTexture_load_one_vfs( "mp_data/joybicon", ( TX_REF )ICON_JOYB, INVALID_KEY );
                BitsInput[3] = INPUT_BITS_JOYB;
                device_on[3] = joy[1].on;

                ego_texture_load_vfs( &background, "mp_data/menu/menu_sleepy", TRANSCOLOR );

                TxTexture_load_one_vfs( "mp_data/bars", ( TX_REF )TX_BARS, INVALID_KEY );

                // load information for all the players that could be imported
                mnu_player_check_import( "mp_players", btrue );

                // reset but_select, or it will mess up the menu.
                // must do it before SlidyButtonState_init()
                sz_buttons[but_select] = "N/A";
                SlidyButtonState_init( &but_state, 0.0f, but_select, sz_buttons, w_buttons );

                // how many players can we pack into the available screen area?
                numVertical   = ( buttonTop - y0 ) / button_repeat - 1;
                numHorizontal = 1;

                x = x0;
                y = y0;
                for ( i = 0; i < numVertical; i++ )
                {
                    int m = i * 5;
                    int player = i + startIndex;

                    if ( player >= loadplayer_count ) continue;

                    ui_Widget_set_button( mnu_widgetList + m, x, y, text_width, icon_size );
                    ui_Widget_LatchMaskAdd( mnu_widgetList + m, UI_LATCH_CLICKED );

                    for ( j = 0, m++; j < 4; j++, m++ )
                    {
                        int k = ICON_KEYB + j;
                        oglx_texture_t * img = TxTexture_get_ptr(( TX_REF )k );

                        ui_Widget_set_button( mnu_widgetList + m, x + text_width + j*icon_size, y, icon_size, icon_size );
                        ui_Widget_set_img( mnu_widgetList + m, ui_just_centered, img );
                        ui_Widget_LatchMaskAdd( mnu_widgetList + m, UI_LATCH_CLICKED );
                    };

                    y += button_repeat;
                };

                doChoosePlayer_update_players( numVertical, startIndex, mnu_widgetList, SDL_arraysize( mnu_widgetList ) );

                if ( loadplayer_count < 10 )
                {
                    tipText_set_position( menuFont, "Choose an input device to select your player(s)", 20 );
                }
                else
                {
                    tipText_set_position( menuFont, "Choose an input device to select your player(s)\nUse the mouse wheel to scroll.", 20 );
                }
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                // Draw the background
                x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
                y = GFX_HEIGHT - background.imgH;

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, x, y, 0, 0, NULL );
                }

                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, -deltaTime );

                // Let lerp wind down relative to the time elapsed
                if ( but_state.lerp <= 0.0f )
                {
                    menuState = MM_Running;
                }
            }
            break;

        case MM_Running:
            {
                // Draw the background
                x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
                y = GFX_HEIGHT - background.imgH;

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, x, y, 0, 0, NULL );
                }

                // Update the select button, if necessary
                if ( mnu_selectedPlayerCount_old != mnu_selectedPlayerCount )
                {
                    mnu_selectedPlayerCount_old = mnu_selectedPlayerCount;

                    doChoosePlayer_update_select_button( w_buttons + but_select, mnu_selectedPlayerCount );
                }

                // use the mouse wheel to scan the characters
                if ( cursor_wheel_event_pending() )
                {
                    if ( cursor.z > 0 )
                    {
                        if ( startIndex + numVertical < loadplayer_count )
                        {
                            startIndex_old = startIndex;
                            startIndex++;
                        }
                    }
                    else if ( cursor.z < 0 )
                    {
                        if ( startIndex > 0 )
                        {
                            startIndex_old = startIndex;
                            startIndex--;
                        }
                    }

                    cursor_finish_wheel_event();
                }

                if ( startIndex_old != startIndex )
                {
                    startIndex_old = startIndex;
                    doChoosePlayer_update_players( numVertical, startIndex, mnu_widgetList, SDL_arraysize( mnu_widgetList ) );
                }

                // Draw the player selection buttons
                x = x0;
                y = y0;
                for ( i = 0; i < numVertical; i++ )
                {
                    int player;
                    int splayer;
                    int m = i * 5;

                    player = i + startIndex;
                    if ( player >= loadplayer_count ) continue;

                    splayer = mnu_Selected_get_loadplayer( player );

                    if ( INVALID_PLAYER != splayer )
                    {
                        ADD_BITS( mnu_widgetList[m].latch_state, UI_LATCH_CLICKED );
                    }
                    else
                    {
                        REMOVE_BITS( mnu_widgetList[m].latch_state, UI_LATCH_CLICKED );
                    }

                    if ( BUTTON_DOWN == ui_doWidget( mnu_widgetList + m ) )
                    {
                        //if ( HAS_SOME_BITS( mnu_widgetList[m].latch_state, UI_LATCH_CLICKED ) && !mnu_Selected_check_loadplayer( player ) )
                        //{
                        //    // button has become clicked
                        //    last_player_old = last_player;
                        //    last_player = player;
                        //    new_player  = btrue;
                        //}
                        //else

                        // remove a selected player by clicking on its button
                        if ( HAS_NO_BITS( mnu_widgetList[m].latch_state, UI_LATCH_CLICKED ) && mnu_Selected_check_loadplayer( player ) )
                        {
                            // button has become unclicked
                            if ( mnu_Selected_remove( player ) )
                            {
                                last_player_old = last_player;
                                last_player = -1;
                            }
                        }
                    }

                    // do each of the input buttons
                    for ( j = 0, m++; j < 4; j++, m++ )
                    {
                        /// @note check for devices being turned on and off each time
                        /// technically we COULD recognize joysticks being inserted and removed while the
                        /// game is running but we don't. I will set this up to handle such future behavior
                        /// anyway :)
                        switch ( j )
                        {
                            case INPUT_DEVICE_KEYBOARD: device_on[j] = keyb.on; break;
                            case INPUT_DEVICE_MOUSE:    device_on[j] = mous.on; break;
                            case INPUT_DEVICE_JOY_A:    device_on[j] = joy[0].on; break;
                            case INPUT_DEVICE_JOY_B:    device_on[j] = joy[1].on; break;
                            default: device_on[j] = bfalse;
                        }

                        // replace any not on device with a null icon
                        if ( !device_on[j] )
                        {
                            mnu_widgetList[m].img = TxTexture_get_ptr(( TX_REF )ICON_NULL );

                            // draw the widget, even though it will not do anything
                            // the menu would looks funny if odd columns missing
                            ui_doWidget( mnu_widgetList + m );
                        }

                        // make the button states reflect the chosen input devices
                        if ( INVALID_PLAYER == splayer || HAS_NO_BITS( mnu_selectedInput[ splayer ], BitsInput[j] ) )
                        {
                            REMOVE_BITS( mnu_widgetList[m].latch_state, UI_LATCH_CLICKED );
                        }
                        else if ( HAS_SOME_BITS( mnu_selectedInput[splayer], BitsInput[j] ) )
                        {
                            ADD_BITS( mnu_widgetList[m].latch_state, UI_LATCH_CLICKED );
                        }

                        if ( BUTTON_DOWN == ui_doWidget( mnu_widgetList + m ) )
                        {
                            if ( HAS_SOME_BITS( mnu_widgetList[m].latch_state, UI_LATCH_CLICKED ) )
                            {
                                bool_t player_added  = bfalse;
                                bool_t control_added = bfalse;

                                // button has become cursor_clicked
                                player_added = bfalse;
                                if ( INVALID_PLAYER == splayer )
                                {
                                    if ( mnu_Selected_add( player ) )
                                    {
                                        player_added = btrue;
                                    }
                                }

                                control_added = bfalse;
                                if ( mnu_Selected_add_input( player, BitsInput[j] ) )
                                {
                                    control_added = btrue;
                                }

                                if ( player_added && control_added )
                                {
                                    last_player_old = last_player;
                                    last_player = player;
                                    new_player  = btrue;
                                }
                            }
                            else if ( INVALID_PLAYER != splayer && HAS_NO_BITS( mnu_widgetList[m].latch_state, UI_LATCH_CLICKED ) )
                            {
                                // button has become unclicked
                                // will remove the player if no inputs are selected
                                mnu_Selected_remove_input( player, BitsInput[j] );

                                // was the player removed?
                                if ( INVALID_PLAYER == mnu_Selected_get_loadplayer( player ) )
                                {
                                    last_player_old = last_player;
                                    last_player = -1;
                                    new_player  = bfalse;
                                }
                            }
                        }
                    }
                }

                // Buttons for going ahead

                // Continue
                if ( mnu_selectedPlayerCount != 0 )
                {
                    if ( SDLKEYDOWN( SDLK_RETURN ) || BUTTON_UP == ui_doWidget( w_buttons + but_select ) )
                    {
                        menuState = MM_Leaving;
                        SlidyButtonState_init( &but_state, 0.0f, but_back, sz_buttons, w_buttons );
                    }
                }

                // Back
                if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doWidget( w_buttons + but_back ) )
                {
                    mnu_selectedPlayerCount = 0;
                    menuState = MM_Leaving;
                    SlidyButtonState_init( &but_state, 0.0f, but_back, sz_buttons, w_buttons );
                }

                // has the player been updated?
                if ( last_player_old != last_player )
                {
                    last_player_old = last_player;

                    if ( -1 == last_player )
                    {
                        // the player has been dismissed

                        // unload the player and release the rendering info
                        doChoosePlayer_load_stats( &stats_info, last_player, 1 );

                        // get rid of the data
                        stats_info.item_ptr = display_item_free( stats_info.item_ptr, btrue );
                    }
                    else
                    {
                        // we have a new player, load the stats and render them
                        doChoosePlayer_load_stats( &stats_info, last_player, 0 );

                        doChoosePlayer_render_stats( &stats_info, GFX_WIDTH - 400, 10, 350, GFX_HEIGHT - 60 );
                    }

                }

                //// show the stats
                //if ( -1 != last_player )
                //{
                //    if ( new_player )
                //    {
                //        // load and display the new player data
                //        new_player = bfalse;
                //        rendered_stats_ptr = doChoosePlayer_show_stats( rendered_stats_ptr, last_player, 0, GFX_WIDTH - 400, 10, 350, GFX_HEIGHT - 60 );
                //    }
                //    else
                //    {
                //        // just display the new player data
                //        rendered_stats_ptr = doChoosePlayer_show_stats( rendered_stats_ptr, last_player, 2, GFX_WIDTH - 400, 10, 350, GFX_HEIGHT - 60 );
                //    }
                //}
                //else
                //{
                //    rendered_stats_ptr = doChoosePlayer_show_stats( rendered_stats_ptr, last_player, 1, GFX_WIDTH - 100, 10, 100, GFX_HEIGHT - 60 );
                //}

                // draw the stats
                if ( NULL != stats_info.item_ptr )
                {
                    display_item_draw( stats_info.item_ptr );
                }

                // tool-tip text
                display_list_draw( tipText_tx );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                // Draw the background
                x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
                y = GFX_HEIGHT - background.imgH;

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, x, y, 0, 0, NULL );
                }

                // Buttons
                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, deltaTime );
                if ( but_state.lerp >= 1.0f )
                {
                    menuState = MM_Finish;
                }
            }
            break;

        case MM_Finish:
            {
                // unload the player(s)
                doChoosePlayer_load_stats( &stats_info, -1, 1 );

                // free all allocated resources
                doChoosePlayer_stats_info_dtor( &stats_info, bfalse );

                oglx_texture_Release( &background );
                TxTexture_free_one(( TX_REF )TX_BARS );

                for ( cnt = 0; cnt < WIDGET_MAX; cnt++ )
                {
                    ui_Widget_free( mnu_widgetList + cnt );
                }

                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    ui_Widget_free( w_buttons + cnt );
                }

                if ( 0 == mnu_selectedPlayerCount )
                {
                    result = -1;
                }
                else
                {
                    // Build the import directory
                    vfs_empty_import_directory();
                    if ( !vfs_mkdir( "import" ) )
                    {
                        log_warning( "game_initialize_imports() - Could not create the import folder. (%s)\n", vfs_getError() );
                    }

                    // set up the slots and the import stuff for the selected players
                    local_import_count = mnu_selectedPlayerCount;
                    for ( i = 0; i < local_import_count; i++ )
                    {
                        selectedPlayer = mnu_selectedPlayer[i];

                        local_import_control[i] = mnu_selectedInput[i];
                        local_import_slot[i]    = i * MAXIMPORTPERPLAYER;

                        // Copy the character to the import directory
                        strncpy( srcDir, loadplayer[selectedPlayer].dir, SDL_arraysize( srcDir ) );
                        snprintf( destDir, SDL_arraysize( destDir ), "/import/temp%04d.obj", local_import_slot[i] );
                        if ( !vfs_copyDirectory( srcDir, destDir ) )
                        {
                            log_warning( "game_initialize_imports() - Failed to copy a import character \"%s\" to \"%s\" (%s)\n", srcDir, destDir, vfs_getError() );
                        }

                        // Copy all of the character's items to the import directory
                        for ( j = 0; j < MAXIMPORTOBJECTS; j++ )
                        {
                            snprintf( srcDir, SDL_arraysize( srcDir ), "%s/%d.obj", loadplayer[selectedPlayer].dir, j );

                            // make sure the source directory exists
                            if ( vfs_isDirectory( srcDir ) )
                            {
                                snprintf( destDir, SDL_arraysize( destDir ), "/import/temp%04d.obj", local_import_slot[i] + j + 1 );
                                vfs_copyDirectory( srcDir, destDir );
                            }
                        }
                    }

                    result = 1;
                }
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_OptionsState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_OptionsState OptionsState_t;
//
////--------------------------------------------------------------------------------------------
//static OptionsState_t *OptionsState_ctor( OptionsState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static OptionsState_t *OptionsState_dtor( OptionsState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static OptionsState_t * doOptions_begin( OptionsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = OptionsState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsState_t * doOptions_entering( OptionsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsState_t * doOptions_running( OptionsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsState_t * doOptions_leaving( OptionsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsState_t * doOptions_finish( OptionsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = OptionsState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
int doOptions( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int menuChoice = 0;
    static SlidyButtonState_t but_state = { NULL };

    enum e_buttons
    {
        but_game,
        but_audio,
        but_input,
        but_video,
        but_back,
        but_count,
        but_sz_count
    };

    // button widgets
    static ui_Widget_t w_buttons[but_count];

    static const char *sz_buttons[but_sz_count] =
    {
        "Game Options",
        "Audio Options",
        "Input Controls",
        "Video Settings",
        "Back",
        ""
    };

    int result = 0;
    int cnt;

    switch ( menuState )
    {
        case MM_Begin:
            menuChoice = 0;

            {
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    // clear the data
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_buttons + cnt, cnt );
                }

                // set up menu variables
                ego_texture_load_vfs( &background, "mp_data/menu/menu_gnome", TRANSCOLOR );

                tipText_set_position( menuFont, "Change your audio, input and video\nsettings here.", 20 );

                SlidyButtonState_init( &but_state, 1.0f, 0, sz_buttons, w_buttons );
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                // Draw the background
                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
                }

                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, -deltaTime );

                // Let lerp wind down relative to the time elapsed
                if ( but_state.lerp <= 0.0f )
                {
                    menuState = MM_Running;
                }
            }
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            // "Options" text
            display_list_draw( tipText_tx );

            // Buttons
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_game ) )
            {
                // game options
                menuChoice = 5;
            }

            if ( BUTTON_UP == ui_doWidget( w_buttons + but_audio ) )
            {
                // audio options
                menuChoice = 1;
            }

            if ( BUTTON_UP == ui_doWidget( w_buttons + but_input ) )
            {
                // input options
                menuChoice = 2;
            }

            if ( BUTTON_UP == ui_doWidget( w_buttons + but_video ) )
            {
                // video options
                menuChoice = 3;
            }

            if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doWidget( w_buttons + but_back ) )
            {
                // back to main menu
                menuChoice = 4;
            }

            if ( menuChoice != 0 )
            {
                menuState = MM_Leaving;
                SlidyButtonState_init( &but_state, 0.0f, 0, sz_buttons, w_buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards

            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
                }

                // Buttons
                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, deltaTime );
                if ( but_state.lerp >= 1.0f )
                {
                    menuState = MM_Finish;
                }
            }

            break;

        case MM_Finish:
            {
                // Free the background texture; don't need to hold onto it
                oglx_texture_Release( &background );

                result = menuChoice;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_OptionsInputState
{
    BASE_MENU_STATE;
};
typedef struct s_OptionsInputState OptionsInputState_t;

//--------------------------------------------------------------------------------------------
static OptionsInputState_t *OptionsInputState_ctor( OptionsInputState_t * ps )
{
    if ( NULL == ps ) return ps;

    /* BLAH */

    return ps;
}

//--------------------------------------------------------------------------------------------
static OptionsInputState_t *OptionsInputState_dtor( OptionsInputState_t * ps )
{
    if ( NULL == ps ) return ps;

    /* BLAH */

    return ps;
}

//--------------------------------------------------------------------------------------------
static OptionsInputState_t * doOptionsInput_begin( OptionsInputState_t * ps, float deltaTime )
{
    if ( NULL == ps ) return ps;

    ps = OptionsInputState_ctor( ps );

    /* BLAH */

    return ps;
};

//--------------------------------------------------------------------------------------------
static OptionsInputState_t * doOptionsInput_entering( OptionsInputState_t * ps, float deltaTime )
{
    if ( NULL == ps ) return ps;

    /* BLAH */

    return ps;
};

//--------------------------------------------------------------------------------------------
static OptionsInputState_t * doOptionsInput_running( OptionsInputState_t * ps, float deltaTime )
{
    if ( NULL == ps ) return ps;

    /* BLAH */

    return ps;
};

//--------------------------------------------------------------------------------------------
static OptionsInputState_t * doOptionsInput_leaving( OptionsInputState_t * ps, float deltaTime )
{
    if ( NULL == ps ) return ps;

    /* BLAH */

    return ps;
};

//--------------------------------------------------------------------------------------------
static OptionsInputState_t * doOptionsInput_finish( OptionsInputState_t * ps, float deltaTime )
{
    if ( NULL == ps ) return ps;

    /* BLAH */

    ps = OptionsInputState_ctor( ps );

    return ps;
};

//--------------------------------------------------------------------------------------------
bool_t doOptionsInput_update_one_button( ui_Widget_t lab_lst[], size_t lab_lst_size, device_controls_t * pdevice, int which )
{
    // update the name of a specific control

    const char * sz_tag;

    if ( which < 0 || ( size_t )which > lab_lst_size ) return bfalse;

    // get a name for this control
    sz_tag = NULL;
    if ( NULL != pdevice && ( size_t )which < pdevice->count )
    {
        sz_tag = scantag_get_string( pdevice->device, pdevice->control[which].tag, pdevice->control[which].is_key );
    }

    // set the label text
    ui_Widget_set_text( lab_lst + which, ui_just_centered, menuFont, sz_tag );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsInput_update_all_buttons( ui_Widget_t lab_lst[], size_t lab_lst_size, device_controls_t * pdevice, int waitingforinput )
{
    int i;

    if ( NULL == lab_lst || 0 == lab_lst_size ) return bfalse;

    // update all control names
    for ( i = CONTROL_BEGIN; i <= CONTROL_END; i++ )
    {
        if ( i == waitingforinput ) continue;

        doOptionsInput_update_one_button( lab_lst, lab_lst_size, pdevice, i );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int doOptionsInput_update_control( ui_Widget_t lab_lst[], size_t lab_lst_size, int idevice, int which )
{
    // Grab the key/button input from the selected device

    device_controls_t * pdevice;
    control_t         * pcontrol;
    int                 tag;
    int                 retval = which;

    // valid device?
    if ( idevice < 0 || ( Uint32 )idevice >= input_device_count ) return -1;
    pdevice = controls + idevice;

    // waiting for any spicific control?
    if ( which < 0 ) return -1;
    pcontrol = pdevice->control + which;

    // is the button available on this device?
    if (( size_t )which >= pdevice->count ) return -1;

    if ( idevice >= INPUT_DEVICE_JOY )
    {
        int ijoy = idevice - INPUT_DEVICE_JOY;

        if ( ijoy < MAXJOYSTICK )
        {
            // is a joy button combo activated?
            for ( tag = 0; tag < scantag_count && -1 != retval; tag++ )
            {
                if ( 0 != scantag[tag].value && ( Uint32 )scantag[tag].value == joy[ijoy].b )
                {
                    pcontrol->tag    = ( Uint32 )scantag[tag].value;
                    pcontrol->is_key = bfalse;
                    retval           = -1;
                }
            }

            // is a joy key activated?
            for ( tag = 0; tag < scantag_count && -1 != retval; tag++ )
            {
                if ( scantag[tag].value < 0 || scantag[tag].value >= SDLK_NUMLOCK ) continue;

                if ( SDLKEYDOWN(( Uint32 )scantag[tag].value ) )
                {
                    pcontrol->tag    = scantag[tag].value;
                    pcontrol->is_key = btrue;
                    retval           = -1;
                }
            }
        }
    }
    else
    {
        switch ( idevice )
        {
            case INPUT_DEVICE_KEYBOARD:
                {
                    // is a keyboard key activated?
                    for ( tag = 0; tag < scantag_count && -1 != retval; tag++ )
                    {
                        if ( scantag[tag].value < 0 || scantag[tag].value >= SDLK_NUMLOCK ) continue;

                        if ( SDLKEYDOWN(( Uint32 )scantag[tag].value ) )
                        {
                            pcontrol->tag    = scantag[tag].value;
                            pcontrol->is_key = btrue;
                            retval           = -1;
                        }
                    }
                }
                break;

            case INPUT_DEVICE_MOUSE:
                {
                    // is a mouse button combo activated?
                    for ( tag = 0; tag < scantag_count && -1 != retval; tag++ )
                    {
                        if ( 0 != scantag[tag].value && ( Uint32 )scantag[tag].value == mous.b )
                        {
                            pcontrol->tag    = scantag[tag].value;
                            pcontrol->is_key = bfalse;
                            retval           = -1;
                        }
                    }

                    // is a mouse key activated?
                    for ( tag = 0; tag < scantag_count && -1 != retval; tag++ )
                    {
                        if ( scantag[tag].value < 0 || scantag[tag].value >= SDLK_NUMLOCK ) continue;

                        if ( SDLKEYDOWN(( Uint32 )scantag[tag].value ) )
                        {
                            pcontrol->tag    = scantag[tag].value;
                            pcontrol->is_key = btrue;
                            retval           = -1;
                        }
                    }
                }
                break;
        }
    }

    if ( -1 == retval )
    {
        doOptionsInput_update_one_button( lab_lst, lab_lst_size, pdevice, which );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsInput_update_player( ui_Widget_t * but_ptr, int player )
{
    Sint32              idevice, iicon;
    device_controls_t * pdevice;
    oglx_texture_t    * img;

    if ( NULL == but_ptr ) return bfalse;

    pdevice = NULL;
    if ( player >= 0 && ( Uint32 )player < input_device_count )
    {
        pdevice = controls + player;
    };

    idevice = -1;
    if ( NULL != pdevice )
    {
        idevice = pdevice->device;
    }

    iicon = -1;
    if ( idevice >= 0 )
    {
        iicon = idevice;

        if ( iicon >= 2 )
        {
            int ijoy = iicon - 2;
            ijoy %= 2;
            iicon = ijoy + 2;
        }
    }

    // The select button image
    if ( iicon < 0 )
    {
        img = TxTexture_get_ptr(( TX_REF )ICON_NULL );
    }
    else
    {
        img = TxTexture_get_ptr(( TX_REF )( ICON_KEYB + iicon ) );
    }
    ui_Widget_set_img( but_ptr, ui_just_centered, img );

    // The select button text
    if ( iicon < 0 )
    {
        ui_Widget_set_text( but_ptr, ui_just_centered, NULL, "Select Player..." );
    }
    else
    {
        ui_Widget_set_text( but_ptr, ui_just_centered, NULL, "Player %i", player + 1 );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int doOptionsInput( float deltaTime )
{
    static int menuState = MM_Begin;
    static int menuChoice = 0;
    static int waitingforinput = -1;
    static SlidyButtonState_t but_state = { NULL };

    enum e_buttons
    {
        but_jump   = CONTROL_JUMP,
        but_luse   = CONTROL_LEFT_USE,
        but_lget   = CONTROL_LEFT_GET,
        but_lpack  = CONTROL_LEFT_PACK,
        but_ruse   = CONTROL_RIGHT_USE,
        but_rget   = CONTROL_RIGHT_GET,
        but_rpack  = CONTROL_RIGHT_PACK,
        but_cam    = CONTROL_CAMERA,
        but_cleft  = CONTROL_CAMERA_LEFT,
        but_cright = CONTROL_CAMERA_RIGHT,
        but_cin    = CONTROL_CAMERA_IN,
        but_cout   = CONTROL_CAMERA_OUT,
        but_up     = CONTROL_UP,
        but_down   = CONTROL_DOWN,
        but_left   = CONTROL_LEFT,
        but_right  = CONTROL_RIGHT,
        but_player,
        but_save,
        but_count,
        but_sz_count
    };

    static const char * sz_buttons[but_sz_count] =
    {
        "...",              // but_jump   = CONTROL_JUMP,
        "...",              // but_luse   = CONTROL_LEFT_USE,
        "...",              // but_lget   = CONTROL_LEFT_GET,
        "...",              // but_lpack  = CONTROL_LEFT_PACK,
        "...",              // but_ruse   = CONTROL_RIGHT_USE,
        "...",              // but_rget   = CONTROL_RIGHT_GET,
        "...",              // but_rpack  = CONTROL_RIGHT_PACK,
        "...",              // but_cam    = CONTROL_CAMERA,
        "...",              // but_cleft  = CONTROL_CAMERA_LEFT,
        "...",              // but_cright = CONTROL_CAMERA_RIGHT,
        "...",              // but_cin    = CONTROL_CAMERA_IN,
        "...",              // but_cout   = CONTROL_CAMERA_OUT,
        "...",              // but_up     = CONTROL_UP,
        "...",              // but_down   = CONTROL_DOWN,
        "...",              // but_left   = CONTROL_LEFT,
        "...",              // but_right  = CONTROL_RIGHT,
        "Player ?",         // but_player
        "Save Settings",    // but_save
        ""
    };

    static ui_Widget_t w_buttons [but_count];

    enum e_labels
    {
        lab_jump   = but_jump,
        lab_luse   = but_luse,
        lab_lget   = but_lget,
        lab_lpack  = but_lpack,
        lab_ruse   = but_ruse,
        lab_rget   = but_rget,
        lab_rpack  = but_rpack,
        lab_cam    = but_cam,
        lab_cleft  = but_cleft,
        lab_cright = but_cright,
        lab_cin    = but_cin,
        lab_cout   = but_cout,
        lab_up     = but_up,
        lab_down   = but_down,
        lab_left   = but_left,
        lab_right  = but_right,
        lab_count,
        lab_sz_count
    };

    static ui_Widget_t w_labels  [lab_count];

    static const char * sz_labels[lab_sz_count] =
    {
        "Jump:",              // lab_jump   = CONTROL_JUMP,
        "Use:",               // lab_luse   = CONTROL_LEFT_USE,
        "Get/Drop:",          // lab_lget   = CONTROL_LEFT_GET,
        "Inventory:",         // lab_lpack  = CONTROL_LEFT_PACK,
        "Use:",               // lab_ruse   = CONTROL_RIGHT_USE,
        "Get/Drop:",          // lab_rget   = CONTROL_RIGHT_GET,
        "Inventory:",         // lab_rpack  = CONTROL_RIGHT_PACK,
        "Camera:",            // lab_cam    = CONTROL_CAMERA,
        "Rotate Left:",       // lab_cleft  = CONTROL_CAMERA_LEFT,
        "Rotate Right:",      // lab_cright = CONTROL_CAMERA_RIGHT,
        "Zoom In:",           // lab_cin    = CONTROL_CAMERA_IN,
        "Zoom Out:",          // lab_cout   = CONTROL_CAMERA_OUT,
        "Up:",                // lab_up     = CONTROL_UP,
        "Down:",              // lab_down   = CONTROL_DOWN,
        "Left:",              // lab_left   = CONTROL_LEFT,
        "Right:",             // lab_right  = CONTROL_RIGHT,
        ""
    };

    static int player = 0;

    int result = 0;
    int cnt;

    Sint32              idevice;
    device_controls_t * pdevice;

    pdevice = NULL;
    if ( player >= 0 && ( Uint32 )player < input_device_count )
    {
        pdevice = controls + player;
    }

    idevice = -1;
    if ( NULL != pdevice )
    {
        idevice = pdevice->device;
    }

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            menuChoice = 0;
            {
                waitingforinput = -1;

                // initialize the buttons
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    // clear the data
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_buttons + cnt, cnt );
                    ui_Widget_set_text( w_buttons + cnt, ui_just_centerleft, NULL, sz_buttons[cnt] );
                }

                // initialize the labels
                for ( cnt = 0; cnt < lab_count; cnt++ )
                {
                    // set up the w_labels
                    memset( w_labels  + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_labels + cnt, UI_Nothing );
                    ui_Widget_set_text( w_labels + cnt, ui_just_centered, menuFont, sz_labels[cnt] );
                }

                // set up the "slidy button"
                SlidyButtonState_init( &but_state, 0.0f, but_save, sz_buttons, w_buttons );

                tipText_set_position( menuFont, "Change input settings here.", 20 );

                // Load the global icons (keyboard, mouse, etc.)
                load_all_global_icons();
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            {
                // set the positions for the active buttons
                ui_Widget_set_bound( w_labels  + lab_luse,   buttonLeft + 000, GFX_HEIGHT - 490, -1, 30 );
                ui_Widget_set_button( w_buttons + but_luse,   buttonLeft + 150, GFX_HEIGHT - 490, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_lget,   buttonLeft + 000, GFX_HEIGHT - 455, -1, 30 );
                ui_Widget_set_button( w_buttons + but_lget,   buttonLeft + 150, GFX_HEIGHT - 455, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_lpack,  buttonLeft + 000, GFX_HEIGHT - 420, -1, 30 );
                ui_Widget_set_button( w_buttons + but_lpack,  buttonLeft + 150, GFX_HEIGHT - 420, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_ruse,   buttonLeft + 300, GFX_HEIGHT - 490, -1, 30 );
                ui_Widget_set_button( w_buttons + but_ruse,   buttonLeft + 450, GFX_HEIGHT - 490, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_rget,   buttonLeft + 300, GFX_HEIGHT - 455, -1, 30 );
                ui_Widget_set_button( w_buttons + but_rget,   buttonLeft + 450, GFX_HEIGHT - 455, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_rpack,  buttonLeft + 300, GFX_HEIGHT - 420, -1, 30 );
                ui_Widget_set_button( w_buttons + but_rpack,  buttonLeft + 450, GFX_HEIGHT - 420, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_jump,   buttonLeft + 000, GFX_HEIGHT - 315, -1, 30 );
                ui_Widget_set_button( w_buttons + but_jump,   buttonLeft + 150, GFX_HEIGHT - 315, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_up,     buttonLeft + 000, GFX_HEIGHT - 280, -1, 30 );
                ui_Widget_set_button( w_buttons + but_up,     buttonLeft + 150, GFX_HEIGHT - 280, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_down,   buttonLeft + 000, GFX_HEIGHT - 245, -1, 30 );
                ui_Widget_set_button( w_buttons + but_down,   buttonLeft + 150, GFX_HEIGHT - 245, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_left,   buttonLeft + 000, GFX_HEIGHT - 210, -1, 30 );
                ui_Widget_set_button( w_buttons + but_left,   buttonLeft + 150, GFX_HEIGHT - 210, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_right,  buttonLeft + 000, GFX_HEIGHT - 175, -1, 30 );
                ui_Widget_set_button( w_buttons + but_right,  buttonLeft + 150, GFX_HEIGHT - 175, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_cam,    buttonLeft + 300, GFX_HEIGHT - 315, -1, 30 );
                ui_Widget_set_button( w_buttons + but_cam,    buttonLeft + 450, GFX_HEIGHT - 315, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_cin,    buttonLeft + 300, GFX_HEIGHT - 280, -1, 30 );
                ui_Widget_set_button( w_buttons + but_cin,    buttonLeft + 450, GFX_HEIGHT - 280, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_cout,   buttonLeft + 300, GFX_HEIGHT - 245, -1, 30 );
                ui_Widget_set_button( w_buttons + but_cout,   buttonLeft + 450, GFX_HEIGHT - 245, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_cleft,  buttonLeft + 300, GFX_HEIGHT - 210, -1, 30 );
                ui_Widget_set_button( w_buttons + but_cleft,  buttonLeft + 450, GFX_HEIGHT - 210, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_cright, buttonLeft + 300, GFX_HEIGHT - 175, -1, 30 );
                ui_Widget_set_button( w_buttons + but_cright, buttonLeft + 450, GFX_HEIGHT - 175, 140, 30 );

                // set the player button position
                ui_Widget_set_button( w_buttons + but_player, buttonLeft + 300, GFX_HEIGHT -  90, 140, 50 );

                // set images for widgets
                doOptionsInput_update_player( w_buttons + but_player, player );

                // initialize the text for all buttons
                doOptionsInput_update_all_buttons( w_buttons, but_right + 1, pdevice, waitingforinput );
            }

            // Fall trough ?
            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            {
                // Background
                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

                // make sure to update the input
                input_read();

                // Someone pressed abort?
                if ( -1 != waitingforinput && SDLKEYDOWN( SDLK_ESCAPE ) )
                {
                    doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );
                    waitingforinput = -1;
                }

                // Update a control, if we are waiting for one
                waitingforinput = doOptionsInput_update_control( w_buttons, but_right + 1, idevice, waitingforinput );

                // Left hand
                ui_drawTextBoxImmediate( NULL, buttonLeft, GFX_HEIGHT - 525, 20, "LEFT HAND", NULL );
                if ( NULL != w_buttons[but_luse].tx_lst )
                {
                    ui_doWidget( w_labels + lab_luse );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_luse ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_luse;
                        ui_Widget_set_text( w_buttons + but_luse, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_lget].tx_lst )
                {

                    ui_doWidget( w_labels + lab_lget );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_lget ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_lget;
                        ui_Widget_set_text( w_buttons + but_lget, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_lpack].tx_lst )
                {
                    ui_doWidget( w_labels + lab_lpack );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_lpack ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_lpack;
                        ui_Widget_set_text( w_buttons + but_lpack, ui_just_centered, menuFont, "..." );
                    }
                }

                // Right hand
                ui_drawTextBoxImmediate( NULL, buttonLeft + 300, GFX_HEIGHT - 525, 20, "RIGHT HAND" );
                if ( NULL != w_buttons[but_ruse].tx_lst )
                {
                    ui_doWidget( w_labels + lab_ruse );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_ruse ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_ruse;
                        ui_Widget_set_text( w_buttons + but_ruse, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_rget].tx_lst )
                {

                    ui_doWidget( w_labels + lab_rget );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_rget ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_rget;
                        ui_Widget_set_text( w_buttons + but_rget, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_rpack].tx_lst )
                {
                    ui_doWidget( w_labels + lab_rpack );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_rpack ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_rpack;
                        ui_Widget_set_text( w_buttons + but_rpack, ui_just_centered, menuFont, "..." );
                    }
                }

                // Controls
                ui_drawTextBoxImmediate( NULL, buttonLeft, GFX_HEIGHT - 350, 20, "CONTROLS" );
                if ( NULL != w_buttons[but_jump].tx_lst )
                {
                    ui_doWidget( w_labels + lab_jump );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_jump ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_jump;
                        ui_Widget_set_text( w_buttons + but_jump, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_up].tx_lst )
                {
                    ui_doWidget( w_labels + lab_up );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_up ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_up;
                        ui_Widget_set_text( w_buttons + but_up, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_down].tx_lst )
                {
                    ui_doWidget( w_labels + lab_down );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_down ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_down;
                        ui_Widget_set_text( w_buttons + but_down, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_left].tx_lst )
                {
                    ui_doWidget( w_labels + lab_left );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_left ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_left;
                        ui_Widget_set_text( w_buttons + but_left, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_right].tx_lst )
                {
                    ui_doWidget( w_labels + lab_right );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_right ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_right;
                        ui_Widget_set_text( w_buttons + but_right, ui_just_centered, menuFont, "..." );
                    }
                }

                // Controls
                ui_drawTextBoxImmediate( NULL, buttonLeft + 300, GFX_HEIGHT - 350, 20, "CAMERA CONTROL" );
                if ( NULL != w_buttons[but_cam].tx_lst )
                {
                    // single button camera control
                    ui_doWidget( w_labels + lab_cam );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_cam ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_cam;
                        ui_Widget_set_text( w_buttons + but_cam, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_cin].tx_lst )
                {
                    ui_doWidget( w_labels + lab_cin );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_cin ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_cin;
                        ui_Widget_set_text( w_buttons + but_cin, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_cout].tx_lst )
                {
                    ui_doWidget( w_labels + lab_cout );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_cout ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_cout;
                        ui_Widget_set_text( w_buttons + but_cout, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_cleft].tx_lst )
                {
                    ui_doWidget( w_labels + lab_cleft );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_cleft ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_cleft;
                        ui_Widget_set_text( w_buttons + but_cleft, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( NULL != w_buttons[but_cright].tx_lst )
                {
                    ui_doWidget( w_labels + lab_cright );
                    if ( BUTTON_UP == ui_doWidget( w_buttons + but_cright ) )
                    {
                        // update the previous button
                        doOptionsInput_update_one_button( w_buttons, but_right + 1, pdevice, waitingforinput );

                        // set the new button
                        waitingforinput = but_cright;
                        ui_Widget_set_text( w_buttons + but_cright, ui_just_centered, menuFont, "..." );
                    }
                }

                if ( BUTTON_UP == ui_doWidget( w_buttons + but_player ) )
                {
                    int new_player;
                    new_player = CLIP( player, -1, MAX( 0, ( int )input_device_count - 1 ) );
                    new_player++;

                    if ( new_player >= 0 && ( Uint32 )new_player >= input_device_count ) new_player = 0;

                    if ( new_player != player )
                    {
                        player = new_player;

                        pdevice = NULL;
                        if ( player >= 0 && ( Uint32 )player < input_device_count )
                        {
                            pdevice = controls + player;
                        }

                        idevice = -1;
                        if ( NULL != pdevice )
                        {
                            idevice = pdevice->device;
                        }

                        doOptionsInput_update_player( w_buttons + but_player, player );

                        // update all buttons for this player
                        doOptionsInput_update_all_buttons( w_buttons, but_right + 1, pdevice, waitingforinput );
                    }
                }

                // Save settings button
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_save ) )
                {
                    // save settings and go back
                    player = 0;
                    input_settings_save_vfs( "controls.txt" );
                    menuState = MM_Leaving;
                }

                // tool-tip text
                display_list_draw( tipText_tx );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );
            }

            // Fall trough
            menuState = MM_Finish;
            break;

        case MM_Finish:
            {
                // free the widgets
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    ui_Widget_free( w_buttons + cnt );
                }

                for ( cnt = 0; cnt < lab_count; cnt++ )
                {
                    ui_Widget_free( w_labels + cnt );
                }

                // Set the next menu to load
                result = 1;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t doOptionsGame_update_difficulty( ui_Widget_t * but_ptr, ui_Widget_t * desc_ptr, int difficulty );
static bool_t doOptionsGame_update_message_count( ui_Widget_t * lab_ptr, int message_count );
static bool_t doOptionsGame_update_message_duration( ui_Widget_t * lab_ptr, Uint16 duration );
static bool_t doOptionsGame_update_cam_autoturn( ui_Widget_t * lab_ptr, Uint8 turn );
static bool_t doOptionsGame_update_fps( ui_Widget_t * lab_ptr, bool_t allowed );
static bool_t doOptionsGame_update_feedback( ui_Widget_t * lab_ptr, FEEDBACK_TYPE type );

//--------------------------------------------------------------------------------------------
//struct s_OptionsGameState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_OptionsGameState OptionsGameState_t;
//
////--------------------------------------------------------------------------------------------
//static OptionsGameState_t *OptionsGameState_ctor( OptionsGameState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static OptionsGameState_t *OptionsGameState_dtor( OptionsGameState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static OptionsGameState_t * doOptionsGame_begin( OptionsGameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = OptionsGameState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsGameState_t * doOptionsGame_entering( OptionsGameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsGameState_t * doOptionsGame_running( OptionsGameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsGameState_t * doOptionsGame_leaving( OptionsGameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsGameState_t * doOptionsGame_finish( OptionsGameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = OptionsGameState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
bool_t doOptionsGame_update_difficulty( ui_Widget_t * but_ptr, ui_Widget_t * desc_ptr, int difficulty )
{
    // Fill out the difficulty description, and the button caption

    bool_t retval = bfalse;

    if ( NULL == but_ptr || NULL == desc_ptr ) return bfalse;

    retval = btrue;
    switch ( difficulty )
    {
        case GAME_EASY:
            ui_Widget_set_text( but_ptr, ui_just_centerleft, menuFont, "Forgiving" );
            ui_Widget_set_text( desc_ptr, ui_just_topleft, menuFont,
                                "FORGIVING (Easy)\n"
                                " - Players gain no bonus XP \n"
                                " - 15%% XP loss upon death\n"
                                " - Monsters take 25%% extra damage by players\n"
                                " - Players take 25%% less damage by monsters\n"
                                " - Halves the chance for Kursed items\n"
                                " - Cannot unlock the final level in this mode\n"
                                " - Life and Mana is refilled after quitting a module" );
            break;

        case GAME_NORMAL:
            ui_Widget_set_text( but_ptr, ui_just_centerleft, menuFont, "Challenging" );
            ui_Widget_set_text( desc_ptr, ui_just_topleft, menuFont,
                                "CHALLENGING (Normal)\n"
                                " - Players gain 10%% bonus XP \n"
                                " - 15%% XP loss upon death \n"
                                " - 15%% money loss upon death" );
            break;

        case GAME_HARD:
            ui_Widget_set_text( but_ptr, ui_just_centerleft, menuFont, "Punishing" );
            ui_Widget_set_text( desc_ptr, ui_just_topleft, menuFont,
                                "PUNISHING (Hard)\n"
                                " - Monsters award 20%% extra xp! \n"
                                " - 15%% XP loss upon death\n"
                                " - 15%% money loss upon death\n"
                                " - No respawning\n"
                                " - Channeling life can kill you\n"
                                " - Players take 25%% more damage\n"
                                " - Doubles the chance for Kursed items" );
            break;

        default:
            ui_Widget_set_text( but_ptr, ui_just_nothing, NULL, NULL );
            ui_Widget_set_text( desc_ptr, ui_just_nothing, NULL, NULL );
            retval = bfalse;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsGame_update_message_count( ui_Widget_t * lab_ptr, int message_count )
{
    bool_t retval;

    if ( NULL == lab_ptr ) return bfalse;

    if ( 0 == message_count )
    {
        retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "None" );
    }
    else
    {
        retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "%i", message_count );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsGame_update_message_duration( ui_Widget_t * lab_ptr, Uint16 duration )
{
    bool_t retval;

    if ( NULL == lab_ptr ) return bfalse;

    if ( duration <= 100 )
    {
        retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "Short" );
    }
    else if ( duration <= 150 )
    {
        retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "Normal" );
    }
    else
    {
        retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "Long" );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsGame_update_cam_autoturn( ui_Widget_t * lab_ptr, Uint8 turn )
{
    bool_t retval;

    if ( NULL == lab_ptr ) return bfalse;

    if ( CAM_TURN_GOOD == turn )
    {
        retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "Off" );
    }
    else if ( turn )
    {
        retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "Fast" );
    }
    else
    {
        retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "On" );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsGame_update_fps( ui_Widget_t * lab_ptr, bool_t allowed )
{
    if ( NULL == lab_ptr ) return bfalse;

    return ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, allowed ? "On" : "Off" );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsGame_update_feedback( ui_Widget_t * lab_ptr, FEEDBACK_TYPE type )
{
    bool_t retval;

    if ( NULL == lab_ptr ) return bfalse;

    // Billboard feedback
    switch ( type )
    {
        case FEEDBACK_OFF:    retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "Disabled" ); break;
        case FEEDBACK_TEXT:   retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "Enabled" ); break;
        case FEEDBACK_NUMBER: retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "Debug" ); break;
        default: retval = ui_Widget_set_text( lab_ptr, ui_just_centerleft, NULL, "Unknown" ); break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int doOptionsGame( float deltaTime )
{
    /// @details Game options menu

    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int menuChoice = 0;
    static SlidyButtonState_t but_state = { NULL };
    static int difficulty_old = 0;

    int cnt;

    // button widgets
    enum e_buttons
    {
        but_difficulty,       // Difficulty
        but_msg_count,        // Max messages
        but_msg_duration,     // Message duration
        but_autoturn,         // Autoturn camera
        but_fps,              // Show FPS
        but_feedback,         // Feedback
        but_save,             // Save button
        but_count,
        but_sz_count
    };

    static const char *sz_buttons[but_sz_count] =
    {
        "N/A",            // but_difficulty
        "N/A",            // but_msg_count
        "N/A",            // but_msg_duration
        "N/A",            // but_autoturn
        "N/A",            // but_fps
        "N/A",            // but_feedback
        "Save Settings",  // but_save
        ""
    };

    static ui_Widget_t w_buttons[but_count];

    enum e_labels
    {
        lab_difficulty   = but_difficulty,
        lab_msg_count    = but_msg_count,
        lab_msg_duration = but_msg_duration,
        lab_autoturn     = but_autoturn,
        lab_fps          = but_fps,
        lab_feedback     = but_feedback,
        lab_diff_desc,
        lab_count,
        lab_sz_count
    };

    static ui_Widget_t w_labels [lab_count];

    static const char *sz_labels[lab_sz_count] =
    {
        "Game Difficulty:",     // lab_difficulty
        "Max  Messages:",       // lab_msg_count
        "Message Duration:",    // lab_msg_duration
        "Autoturn Camera:",     // lab_autoturn
        "Display FPS:",         // lab_fps
        "Floating Text:",       // lab_feedback
        "N/A",                  // lab_diff_desc
        ""
    };

    int  result = 0;
    int i;

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables

            menuChoice = 0;
            {
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    // set up the w_buttons
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_buttons + cnt, ( ui_id_t )cnt );
                    ui_Widget_set_text( w_buttons + cnt, ui_just_centerleft, NULL, sz_buttons[cnt] );
                }

                for ( cnt = 0; cnt < lab_count; cnt++ )
                {
                    // set up the w_labels
                    memset( w_labels  + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_labels + cnt, UI_Nothing );
                    ui_Widget_set_text( w_labels + cnt, ui_just_centerleft, menuFont, sz_labels[cnt] );
                }

                // load the background image
                ego_texture_load_vfs( &background, "mp_data/menu/menu_fairy", TRANSCOLOR );

                difficulty_old = cfg.difficulty;

                // auto-format the tip text
                tipText_set_position( menuFont, "Change game settings here.", 20 );

                // auto-format the slidy buttons
                SlidyButtonState_init( &but_state, 0.0f, but_save, sz_buttons, w_buttons );

                // set some special text
                doOptionsGame_update_difficulty( w_buttons + but_difficulty, w_labels + lab_diff_desc, cfg.difficulty );

                // auto-format the buttons and labels
                for ( i = but_difficulty; i <= but_feedback; i++ )
                {
                    int j = GFX_HEIGHT - ( but_feedback - but_difficulty + 5 ) * 35 + i * 35;

                    ui_Widget_set_button( w_buttons + i, buttonLeft + 515, j, 100, 30 );
                    ui_Widget_set_bound( w_labels  + i, buttonLeft + 350, j, -1, -1 );
                }

                // custom format buttons and labels
                ui_Widget_set_bound( w_labels + lab_diff_desc,  buttonLeft + 150, 50, -1, -1 );
                ui_Widget_set_bound( w_labels + lab_difficulty, buttonLeft,       50, -1, -1 );
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                // Draw the background
                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, ( GFX_WIDTH  / 2 ) + ( background.imgW / 2 ), GFX_HEIGHT - background.imgH, 0, 0, NULL );
                }

                doOptionsGame_update_message_count( w_buttons + but_msg_count, cfg.message_count_req );
                doOptionsGame_update_message_duration( w_buttons + but_msg_duration, cfg.message_duration );
                doOptionsGame_update_cam_autoturn( w_buttons + but_autoturn, cfg.autoturncamera );
                doOptionsGame_update_fps( w_buttons + but_fps, cfg.fps_allowed );
                doOptionsGame_update_feedback( w_buttons + but_feedback, cfg.feedback );
            }

            // Fall trough?
            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            {
                // Background
                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, ( GFX_WIDTH / 2 ) - ( background.imgW / 2 ), GFX_HEIGHT - background.imgH, 0, 0, NULL );
                }

                // Module difficulty
                ui_doWidget( w_labels + lab_difficulty );
                ui_doWidget( w_labels + lab_diff_desc );
                ui_doWidget( w_labels  + lab_difficulty );
                if ( !PMod->active && BUTTON_UP == ui_doWidget( w_buttons + but_difficulty ) )
                {
                    cfg.difficulty = CLIP( cfg.difficulty, 0, GAME_HARD );

                    // Increase difficulty
                    cfg.difficulty++;
                    if ( cfg.difficulty > GAME_HARD ) cfg.difficulty = 0;

                    // handle the difficulty description
                    doOptionsGame_update_difficulty( w_buttons + but_difficulty, w_labels + lab_diff_desc, cfg.difficulty );
                }

                // Text messages
                ui_doWidget( w_labels + lab_msg_count );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_msg_count ) )
                {
                    cfg.message_count_req = CLIP( cfg.message_count_req, 0, MAX_MESSAGE - 1 );

                    cfg.message_count_req++;
                    if ( cfg.message_count_req > MAX_MESSAGE ) cfg.message_count_req = 0;
                    if ( cfg.message_count_req < 4 && cfg.message_count_req != 0 ) cfg.message_count_req = 4;

                    // handle the difficulty description
                    doOptionsGame_update_message_count( w_buttons + but_msg_count, cfg.message_count_req );
                }

                // Message time
                ui_doWidget( w_labels + lab_msg_duration );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_msg_duration ) )
                {
                    cfg.message_duration = CLIP( cfg.message_duration, 0, 250 );
                    cfg.message_duration += 50;

                    if ( cfg.message_duration >= 250 )
                    {
                        cfg.message_duration = 50;
                    }

                    doOptionsGame_update_message_duration( w_buttons + but_msg_duration, cfg.message_duration );
                }

                // Autoturn camera
                ui_doWidget( w_labels + lab_autoturn );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_autoturn ) )
                {
                    if ( CAM_TURN_GOOD == cfg.autoturncamera )
                    {
                        cfg.autoturncamera = CAM_TURN_NONE;
                    }
                    else if ( cfg.autoturncamera )
                    {
                        cfg.autoturncamera = CAM_TURN_GOOD;
                    }
                    else
                    {
                        cfg.autoturncamera = CAM_TURN_AUTO;
                    }

                    doOptionsGame_update_cam_autoturn( w_buttons + but_autoturn, cfg.autoturncamera );
                }

                // Show the fps?
                ui_doWidget( w_labels + lab_fps );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_fps ) )
                {
                    cfg.fps_allowed = !cfg.fps_allowed;

                    doOptionsGame_update_fps( w_buttons + but_fps, cfg.fps_allowed );
                }

                // Feedback
                ui_doWidget( w_labels + lab_feedback );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_feedback ) )
                {
                    cfg.feedback = CLIP( cfg.feedback, 0, FEEDBACK_COUNT - 1 );

                    if ( cfg.dev_mode )
                    {
                        // increment the type
                        cfg.feedback = ( FEEDBACK_TYPE )( cfg.feedback + 1 );

                        // make the list wrap around
                        if ( cfg.feedback >= FEEDBACK_COUNT )
                        {
                            cfg.feedback = FEEDBACK_OFF;
                        }
                    }
                    else if ( FEEDBACK_OFF == cfg.feedback )
                    {
                        cfg.feedback = FEEDBACK_TEXT;
                    }
                    else
                    {
                        cfg.feedback = FEEDBACK_OFF;
                    }

                    doOptionsGame_update_feedback( w_buttons + but_feedback, cfg.feedback );
                }

                // Save settings
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_save ) )
                {
                    // synchronize the config values with the various game subsystems
                    setup_synch( &cfg );

                    // save the setup file
                    setup_upload( &cfg );
                    setup_write();

                    menuState = MM_Leaving;
                }

                // tool-tip text
                display_list_draw( tipText_tx );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, ( GFX_WIDTH  / 2 ) + ( background.imgW / 2 ), GFX_HEIGHT - background.imgH, 0, 0, NULL );
                }
            }

            // Fall trough
            menuState = MM_Finish;
            break;

        case MM_Finish:
            // free all allocated data
            {
                // Free the background texture; don't need to hold onto it
                oglx_texture_Release( &background );

                // free the widgets
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    ui_Widget_free( w_buttons + cnt );
                }

                for ( cnt = 0; cnt < lab_count; cnt++ )
                {
                    ui_Widget_free( w_labels  + cnt );
                }

                ui_Widget_free( w_labels + lab_diff_desc );
                ui_Widget_free( w_labels + lab_difficulty );

                // Set the next menu to load
                result = 1;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_OptionsAudioState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_OptionsAudioState OptionsAudioState_t;
//
////--------------------------------------------------------------------------------------------
//static OptionsAudioState_t *OptionsAudioState_ctor( OptionsAudioState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static OptionsAudioState_t *OptionsAudioState_dtor( OptionsAudioState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static OptionsAudioState_t * doOptionsAudio_begin( OptionsAudioState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = OptionsAudioState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsAudioState_t * doOptionsAudio_entering( OptionsAudioState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsAudioState_t * doOptionsAudio_running( OptionsAudioState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsAudioState_t * doOptionsAudio_leaving( OptionsAudioState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsAudioState_t * doOptionsAudio_finish( OptionsAudioState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = OptionsAudioState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
bool_t doOptionsAudio_update_sound_on( ui_Widget_t * but_ptr, bool_t val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, val ? "On" : "Off" );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsAudio_update_sound_volume( ui_Widget_t * but_ptr, Uint8 val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "%i", val );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsAudio_update_music_on( ui_Widget_t * but_ptr, bool_t val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, val ? "On" : "Off" );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsAudio_update_music_volume( ui_Widget_t * but_ptr, Uint8 val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "%i", val );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsAudio_update_sound_channels( ui_Widget_t * but_ptr, Uint16 val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "%i", val );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsAudio_update_buffer_size( ui_Widget_t * but_ptr, Uint16 val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "%i", val );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsAudio_update_quality( ui_Widget_t * but_ptr, bool_t val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, val ?  "Normal" : "High" );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsAudio_update_footfall( ui_Widget_t * but_ptr, bool_t val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, val ? "Enabled" : "Disabled" );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsAudio_update_settings( egoboo_config_t * pcfg )
{
    if ( NULL == pcfg ) return bfalse;

    // synchronize the config values with the various game subsystems
    setup_synch( pcfg );

    // save the setup file
    setup_upload( pcfg );
    setup_write();

    // Reload the sound system
    sound_restart();

    // Do we restart the music?
    if ( pcfg->music_allowed )
    {
        load_all_music_sounds_vfs();
        fade_in_music( musictracksloaded[songplaying] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int doOptionsAudio( float deltaTime )
{
    /// @details Audio options menu

    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int menuChoice = 0;
    static SlidyButtonState_t but_state = { NULL };

    bool_t old_sound_allowed       = cfg.sound_allowed;
    Uint8  old_sound_volume        = cfg.sound_volume;
    bool_t old_music_allowed       = cfg.music_allowed;
    Uint8  old_music_volume        = cfg.music_volume;
    Uint16 old_sound_channel_count = cfg.sound_channel_count;
    Uint16 old_sound_buffer_size   = cfg.sound_buffer_size;
    bool_t old_sound_highquality   = cfg.sound_highquality;
    bool_t old_sound_footfall      = cfg.sound_footfall;

    // button widgets
    enum e_buttons
    {
        but_on,
        but_vol,
        but_mus_on,
        but_mus_vol,
        but_channels,
        but_buffer,
        but_quality,
        but_footsteps,
        but_save,
        but_count,
        but_sz_count
    };

    static const char *sz_buttons[but_sz_count] =
    {
        "N/A",              // but_on
        "N/A",              // but_vol
        "N/A",              // but_mus_on
        "N/A",              // but_mus_vol
        "N/A",              // but_channels
        "N/A",              // but_buffer
        "N/A",              // but_quality
        "N/A",              // but_footsteps
        "Save Settings",    // but_save
        ""
    };

    static ui_Widget_t w_buttons[but_count];

    // label widgets
    enum e_labels
    {
        lab_on,
        lab_vol,
        lab_mus_on,
        lab_mus_vol,
        lab_channels,
        lab_buffer,
        lab_quality,
        lab_footsteps,
        lab_count,
        lab_sz_count
    };

    static const char *sz_labels[lab_sz_count] =
    {
        "Sound:",           // lab_on        - Enable sound
        "Sound Volume:",    // lab_vol       - Sound volume
        "Music:",           // lab_mus_on    - Enable music
        "Music Volume:",    // lab_mus_vol   - Music volume
        "Sound Channels:",  // lab_channels  - Sound channels
        "Buffer Size:",     // lab_buffer    - Sound buffer
        "Sound Quality:",   // lab_quality   - Sound quality
        "Footstep Sounds:", // lab_footsteps - Play footsteps
        ""
    };

    static ui_Widget_t w_labels[lab_count];

    int result = 0;
    int cnt;

    switch ( menuState )
    {
        case MM_Begin:
            menuChoice = 0;

            {
                // set up the w_buttons
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_buttons + cnt, cnt );
                    ui_Widget_set_text( w_buttons + cnt, ui_just_centered, NULL, sz_buttons[cnt] );
                }

                // set up the w_labels
                for ( cnt = 0; cnt < lab_count; cnt++ )
                {
                    memset( w_labels  + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_labels + cnt, UI_Nothing );
                    ui_Widget_set_text( w_labels + cnt, ui_just_centerleft, menuFont, sz_labels[cnt] );
                }

                // set up menu variables
                ego_texture_load_vfs( &background, "mp_data/menu/menu_sound", TRANSCOLOR );

                tipText_set_position( menuFont, "Change audio settings here.", 20 );

                SlidyButtonState_init( &but_state, 0.0f, but_save, sz_buttons, w_buttons );

                // Format the buttons
                ui_Widget_set_bound( w_labels  + lab_on,        buttonLeft + 000, GFX_HEIGHT - 455, -1, 30 );
                ui_Widget_set_button( w_buttons + but_on,        buttonLeft + 150, GFX_HEIGHT - 455, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_vol,       buttonLeft + 000, GFX_HEIGHT - 420, -1, 30 );
                ui_Widget_set_button( w_buttons + but_vol,       buttonLeft + 150, GFX_HEIGHT - 420, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_mus_on,    buttonLeft + 000, GFX_HEIGHT - 350, -1, 30 );
                ui_Widget_set_button( w_buttons + but_mus_on,    buttonLeft + 150, GFX_HEIGHT - 350, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_mus_vol,   buttonLeft + 000, GFX_HEIGHT - 315, -1, 30 );
                ui_Widget_set_button( w_buttons + but_mus_vol,   buttonLeft + 150, GFX_HEIGHT - 315, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_footsteps, buttonLeft + 000, GFX_HEIGHT - 245, -1, 30 );
                ui_Widget_set_button( w_buttons + but_footsteps, buttonLeft + 150, GFX_HEIGHT - 245, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_channels,  buttonLeft + 000, GFX_HEIGHT - 210, -1, 30 );
                ui_Widget_set_button( w_buttons + but_channels,  buttonLeft + 150, GFX_HEIGHT - 210, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_buffer,    buttonLeft + 000, GFX_HEIGHT - 175, -1, 30 );
                ui_Widget_set_button( w_buttons + but_buffer,    buttonLeft + 150, GFX_HEIGHT - 175, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_quality,   buttonLeft + 000, GFX_HEIGHT - 140, -1, 30 );
                ui_Widget_set_button( w_buttons + but_quality,   buttonLeft + 150, GFX_HEIGHT - 140, 140, 30 );
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            {
                // copy these values on entering
                old_sound_allowed       = cfg.sound_allowed;
                old_sound_volume        = cfg.sound_volume;
                old_music_allowed       = cfg.music_allowed;
                old_music_volume        = cfg.music_volume;
                old_sound_channel_count = cfg.sound_channel_count;
                old_sound_buffer_size   = cfg.sound_buffer_size;
                old_sound_highquality   = cfg.sound_highquality;
                old_sound_footfall      = cfg.sound_footfall;

                // update button text
                doOptionsAudio_update_sound_on( w_buttons + but_on,        cfg.sound_allowed );
                doOptionsAudio_update_sound_volume( w_buttons + but_vol,       cfg.sound_volume );
                doOptionsAudio_update_music_on( w_buttons + but_mus_on,    cfg.music_allowed );
                doOptionsAudio_update_music_volume( w_buttons + but_mus_vol,   cfg.music_volume );
                doOptionsAudio_update_sound_channels( w_buttons + but_channels,  cfg.sound_channel_count );
                doOptionsAudio_update_buffer_size( w_buttons + but_buffer,    cfg.sound_buffer_size );
                doOptionsAudio_update_quality( w_buttons + but_quality,   cfg.sound_highquality );
                doOptionsAudio_update_footfall( w_buttons + but_footsteps, cfg.sound_footfall );
            }

            // Fall trough
            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            {
                // Background
                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

                // Draw the background
                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0, NULL );
                }

                // Buttons
                ui_doWidget( w_labels + lab_on );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_on ) )
                {
                    cfg.sound_allowed = !cfg.sound_allowed;

                    if ( old_sound_allowed != cfg.sound_allowed )
                    {
                        old_sound_allowed = cfg.sound_allowed;
                        doOptionsAudio_update_sound_on( w_buttons + but_on,        cfg.sound_allowed );
                    }
                }

                ui_doWidget( w_labels + lab_vol );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_vol ) )
                {
                    cfg.sound_volume += 5;
                    if ( cfg.sound_volume > 100 ) cfg.sound_volume = 0;

                    if ( old_sound_volume != cfg.sound_volume )
                    {
                        old_sound_volume = cfg.sound_volume;
                        doOptionsAudio_update_sound_volume( w_buttons + but_vol,       cfg.sound_volume );
                    }
                }

                ui_doWidget( w_labels + lab_mus_on );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_mus_on ) )
                {
                    cfg.music_allowed = !cfg.music_allowed;

                    if ( old_music_allowed != cfg.music_allowed )
                    {
                        old_music_allowed = cfg.music_allowed;
                        doOptionsAudio_update_music_on( w_buttons + but_mus_on,    cfg.music_allowed );
                    }
                }

                ui_doWidget( w_labels + lab_mus_vol );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_mus_vol ) )
                {
                    cfg.music_volume += 5;
                    if ( cfg.music_volume > 100 ) cfg.music_volume = 0;

                    if ( old_music_volume != cfg.music_volume )
                    {
                        old_music_volume = cfg.music_volume;
                        doOptionsAudio_update_music_volume( w_buttons + but_mus_vol,   cfg.music_volume );
                    }
                }

                ui_doWidget( w_labels + lab_channels );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_channels ) )
                {
                    if ( cfg.sound_channel_count < 8 )
                    {
                        cfg.sound_channel_count = 8;
                    }
                    else
                    {
                        cfg.sound_channel_count <<= 1;
                    }

                    if ( cfg.sound_channel_count > 128 )
                    {
                        cfg.sound_channel_count = 8;
                    }

                    if ( old_sound_channel_count != cfg.sound_channel_count )
                    {
                        old_sound_buffer_size = cfg.sound_channel_count;
                        doOptionsAudio_update_sound_channels( w_buttons + but_channels,  cfg.sound_channel_count );
                    }
                }

                ui_doWidget( w_labels + lab_buffer );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_buffer ) )
                {
                    if ( cfg.sound_buffer_size < 512 )
                    {
                        cfg.sound_buffer_size = 512;
                    }
                    else
                    {
                        cfg.sound_buffer_size <<= 1;
                    }

                    if ( cfg.sound_buffer_size > 8196 )
                    {
                        cfg.sound_buffer_size = 512;
                    }

                    if ( old_sound_buffer_size != cfg.sound_buffer_size )
                    {
                        old_sound_buffer_size = cfg.sound_buffer_size;
                        doOptionsAudio_update_buffer_size( w_buttons + but_buffer,    cfg.sound_buffer_size );
                    }
                }

                ui_doWidget( w_labels + lab_quality );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_quality ) )
                {
                    cfg.sound_highquality = !cfg.sound_highquality;

                    if ( old_sound_highquality != cfg.sound_highquality )
                    {
                        old_sound_highquality = cfg.sound_highquality;
                        doOptionsAudio_update_quality( w_buttons + but_quality,   cfg.sound_highquality );
                    }
                }

                ui_doWidget( w_labels + lab_footsteps );
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_footsteps ) )
                {
                    cfg.sound_footfall = !cfg.sound_footfall;

                    if ( old_sound_footfall != cfg.sound_footfall )
                    {
                        old_sound_footfall = cfg.sound_footfall;
                        doOptionsAudio_update_footfall( w_buttons + but_footsteps, cfg.sound_footfall );
                    }
                }

                // Save settings
                if ( BUTTON_UP == ui_doWidget( w_buttons + but_save ) )
                {
                    doOptionsAudio_update_settings( &cfg );
                    menuState = MM_Leaving;
                }

                // tool-tip text
                display_list_draw( tipText_tx );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
                }
            }

            // Fall trough
            menuState = MM_Finish;
            break;

        case MM_Finish:
            {
                // Free the background texture; don't need to hold onto it
                oglx_texture_Release( &background );

                // release the widgets
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    ui_Widget_free( w_buttons + cnt );
                }

                for ( cnt = 0; cnt < lab_count; cnt++ )
                {
                    ui_Widget_free( w_labels + cnt );
                }

                // Set the next menu to load
                result = 1;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_OptionsVideoState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_OptionsVideoState OptionsVideoState_t;
//
////--------------------------------------------------------------------------------------------
//static OptionsVideoState_t *OptionsVideoState_ctor( OptionsVideoState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static OptionsVideoState_t *OptionsVideoState_dtor( OptionsVideoState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static OptionsVideoState_t * doOptionsVideo_begin( OptionsVideoState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = OptionsVideoState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsVideoState_t * doOptionsVideo_entering( OptionsVideoState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsVideoState_t * doOptionsVideo_running( OptionsVideoState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsVideoState_t * doOptionsVideo_leaving( OptionsVideoState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static OptionsVideoState_t * doOptionsVideo_finish( OptionsVideoState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = OptionsVideoState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
bool_t doOptionsVideo_coerce_aspect_ratio( int width, int height, float * pratio, STRING * psz_ratio )
{
    /// @details BB@> coerce the aspect ratio of the screen to some standard size

    float req_aspect_ratio;

    if ( 0 == height || NULL == pratio || NULL == psz_ratio ) return bfalse;

    req_aspect_ratio = ( float )width / ( float )height;

    if ( req_aspect_ratio > 0.0 && req_aspect_ratio < 0.5f*(( 5.0f / 4.0f ) + ( 4.0f / 3.0f ) ) )
    {
        *pratio = 5.0f / 4.0f;
        strncpy( *psz_ratio, "5:4", sizeof( *psz_ratio ) );
    }
    else if ( req_aspect_ratio >= 0.5f*(( 5.0f / 4.0f ) + ( 4.0f / 3.0f ) ) && req_aspect_ratio < 0.5f*(( 4.0f / 3.0f ) + ( 8.0f / 5.0f ) ) )
    {
        *pratio = 4.0f / 3.0f;
        strncpy( *psz_ratio, "4:3", sizeof( *psz_ratio ) );
    }
    else if ( req_aspect_ratio >= 0.5f*(( 4.0f / 3.0f ) + ( 8.0f / 5.0f ) ) && req_aspect_ratio < 0.5f*(( 8.0f / 5.0f ) + ( 5.0f / 3.0f ) ) )
    {
        *pratio = 8.0f / 5.0f;
        strncpy( *psz_ratio, "8:5", sizeof( *psz_ratio ) );
    }
    else if ( req_aspect_ratio >= 0.5f*(( 8.0f / 5.0f ) + ( 5.0f / 3.0f ) ) && req_aspect_ratio < 0.5f*(( 5.0f / 3.0f ) + ( 16.0f / 9.0f ) ) )
    {
        *pratio = 5.0f / 3.0f;
        strncpy( *psz_ratio, "5:3", sizeof( *psz_ratio ) );
    }
    else
    {
        *pratio = 16.0f / 9.0f;
        strncpy( *psz_ratio, "16:9", sizeof( *psz_ratio ) );
    }

    return btrue;

}

//--------------------------------------------------------------------------------------------
int doOptionsVideo_fix_fullscreen_resolution( egoboo_config_t * pcfg, SDLX_screen_info_t * psdl_scr, STRING * psz_screen_size )
{
    STRING     sz_aspect_ratio = "unknown";
    float      req_screen_area  = ( float )pcfg->scrx_req * ( float )pcfg->scry_req;
    float      min_diff = 0.0f;
    SDL_Rect * found_rect = NULL, ** pprect = NULL;

    float       aspect_ratio;

    doOptionsVideo_coerce_aspect_ratio( pcfg->scrx_req, pcfg->scry_req, &aspect_ratio, &sz_aspect_ratio );

    found_rect = NULL;
    pprect = psdl_scr->video_mode_list;
    while ( NULL != *pprect )
    {
        SDL_Rect * prect = *pprect;

        float sdl_aspect_ratio;
        float sdl_screen_area;
        float diff, diff1, diff2;

        sdl_aspect_ratio = ( float )prect->w / ( float )prect->h;
        sdl_screen_area  = prect->w * prect->h;

        diff1 = log( sdl_aspect_ratio / aspect_ratio );
        diff2 = log( sdl_screen_area / req_screen_area );

        diff = 2.0f * ABS( diff1 ) + ABS( diff2 );

        if ( NULL == found_rect || diff < min_diff )
        {
            found_rect = prect;
            min_diff   = diff;

            if ( 0.0f == min_diff ) break;
        }

        pprect++;
    }

    if ( NULL != found_rect )
    {
        pcfg->scrx_req = found_rect->w;
        pcfg->scry_req = found_rect->h;
    }
    else
    {
        // we cannot find an approximate screen size

        switch ( pcfg->scrx_req )
        {
                // Normal resolutions
            case 1024:
                pcfg->scry_req  = 768;
                strncpy( sz_aspect_ratio, "4:3", sizeof( sz_aspect_ratio ) );
                break;

            case 640:
                pcfg->scry_req = 480;
                strncpy( sz_aspect_ratio, "4:3", sizeof( sz_aspect_ratio ) );
                break;

            case 800:
                pcfg->scry_req = 600;
                strncpy( sz_aspect_ratio, "4:3", sizeof( sz_aspect_ratio ) );
                break;

                // 1280 can be both widescreen and normal
            case 1280:
                if ( pcfg->scry_req > 800 )
                {
                    pcfg->scry_req = 1024;
                    strncpy( sz_aspect_ratio, "5:4", sizeof( sz_aspect_ratio ) );
                }
                else
                {
                    pcfg->scry_req = 800;
                    strncpy( sz_aspect_ratio, "8:5", sizeof( sz_aspect_ratio ) );
                }
                break;

                // Widescreen resolutions
            case 1440:
                pcfg->scry_req = 900;
                strncpy( sz_aspect_ratio, "8:5", sizeof( sz_aspect_ratio ) );
                break;

            case 1680:
                pcfg->scry_req = 1050;
                strncpy( sz_aspect_ratio, "8:5", sizeof( sz_aspect_ratio ) );
                break;

            case 1920:
                pcfg->scry_req = 1200;
                strncpy( sz_aspect_ratio, "8:5", sizeof( sz_aspect_ratio ) );
                break;

                // unknown
            default:
                doOptionsVideo_coerce_aspect_ratio( pcfg->scrx_req, pcfg->scry_req, &aspect_ratio, &sz_aspect_ratio );
                break;
        }
    }

    snprintf( *psz_screen_size, sizeof( *psz_screen_size ), "%dx%d - %s", pcfg->scrx_req, pcfg->scry_req, sz_aspect_ratio );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_antialiasing( ui_Widget_t * but_ptr, Uint8 val )
{
    bool_t retval = bfalse;

    if ( NULL == but_ptr ) return bfalse;

    if ( 0 == val )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Off" );
    }
    else
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "X%i", val );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_texture_filter( ui_Widget_t * but_ptr, Uint8 val )
{
    bool_t retval = bfalse;

    if ( NULL == but_ptr ) return bfalse;

    retval = bfalse;

    if ( val >= TX_ANISOTROPIC )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Ansiotropic %i", val - TX_ANISOTROPIC );
    }
    else switch ( val )
        {
            case TX_UNFILTERED:
                retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Unfiltered" );
                break;

            case TX_LINEAR:
                retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Linear" );
                break;

            case TX_MIPMAP:
                retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Mipmap" );
                break;

            case TX_BILINEAR:
                retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Bilinear" );
                break;

            case TX_TRILINEAR_1:
                retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Trilinear 1" );
                break;

            case TX_TRILINEAR_2:
                retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Trilinear 2" );
                break;

            default:
                retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Unknown" );
                break;
        }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_dither( ui_Widget_t * but_ptr, bool_t val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, val ? "Yes" : "No" );
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_fullscreen( ui_Widget_t * but_ptr, bool_t val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, val ? "True" : "False" );;
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_reflections( ui_Widget_t * but_ptr, bool_t allowed, bool_t do_prt, Uint8 fade )
{
    bool_t retval = bfalse;

    if ( NULL == but_ptr ) return bfalse;

    if ( !allowed )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Off" );
    }
    else if ( !do_prt )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Low" );
    }
    else if ( 0 == fade )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Medium" );
    }
    else
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "High" );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_shadows( ui_Widget_t * but_ptr, bool_t allowed, bool_t sprite )
{
    bool_t retval = bfalse;

    if ( NULL == but_ptr ) return bfalse;

    if ( !allowed )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Off" );
    }
    else if ( sprite )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Normal" );
    }
    else
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Best" );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_z_buffer( ui_Widget_t * but_ptr, int val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "%d", val );
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_max_lights( ui_Widget_t * but_ptr, int val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "%i", val );
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_3d_effects( ui_Widget_t * but_ptr, bool_t use_phong, bool_t use_perspective, bool_t overlay_allowed, bool_t background_allowed )
{
    bool_t retval = bfalse;

    if ( NULL == but_ptr ) return bfalse;

    if ( use_phong && use_perspective && overlay_allowed && background_allowed )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Off" );
    }
    else if ( !use_phong )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Okay" );
    }
    else if ( !use_perspective && overlay_allowed && background_allowed )
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Superb" );
    }
    else
    {
        retval = ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "Good" );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_water_quality( ui_Widget_t * but_ptr, bool_t val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, val ? "True" : "False" );
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_max_particles( ui_Widget_t * but_ptr, Uint16 val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "%i", val );
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_widescreen( ui_Widget_t * but_ptr, bool_t val )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, val ? "X" : NULL );
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_update_resolution( ui_Widget_t * but_ptr, int x, int y )
{
    if ( NULL == but_ptr ) return bfalse;

    return ui_Widget_set_text( but_ptr, ui_just_centerleft, NULL, "%ix%i", x, y );
}

//--------------------------------------------------------------------------------------------
bool_t doOptionsVideo_update_settings( egoboo_config_t * pcfg )
{
    if ( NULL == pcfg ) return bfalse;

    // synchronize the config values with the various game subsystems
    setup_synch( pcfg );

    // save the setup file
    setup_upload( pcfg );
    setup_write();

    // Reload some of the graphics
    load_graphics();

    return btrue;
}

//--------------------------------------------------------------------------------------------
int doOptionsVideo( float deltaTime )
{
    /// @details Video options menu

    const int max_anisotropic = 16;

    static SlidyButtonState_t but_state = { NULL };

    enum e_buttons
    {
        but_antialiasing =  0,  // Antialaising
        but_dither           ,  // Fast & ugly
        but_fullscreen       ,  // Fullscreen
        but_reflections      ,  // Reflections
        but_filtering        ,  // Texture filtering
        but_shadow           ,  // Shadows
        but_zbuffer          ,  // Z bit
        but_maxlights        ,  // Fog
        but_3dfx             ,  // Special effects
        but_multiwater       ,  // Multi water layer
        but_widescreen       ,  // Widescreen
        but_screensize       ,  // Screen resolution
        but_maxparticles     ,  // Max particles
        but_save             ,
        but_count,
        but_sz_count
    };

    static ui_Widget_t w_buttons[but_count];
    static const char *sz_buttons[but_sz_count];

    enum e_labels
    {
        lab_antialiasing = but_antialiasing,
        lab_dither       = but_dither,
        lab_fullscreen   = but_fullscreen,
        lab_reflections  = but_reflections,
        lab_filtering    = but_filtering,
        lab_shadow       = but_shadow,
        lab_zbuffer      = but_zbuffer,
        lab_maxlights    = but_maxlights,
        lab_3dfx         = but_3dfx,
        lab_multiwater   = but_multiwater,
        lab_widescreen   = but_widescreen,
        lab_screensize   = but_screensize,
        lab_maxparticles = but_maxparticles,
        lab_count,
        lab_sz_count
    };

    // button widgets
    static ui_Widget_t w_labels [lab_count];
    static const char *sz_labels[lab_sz_count] =
    {
        "Antialiasing:",            // lab_antialiasing
        "Dithering:",               // lab_dither
        "Fullscreen:",              // lab_fullscreen
        "Reflections:",             // lab_reflections
        "Texture Filtering:",        // lab_filtering
        "Shadows:",                 // lab_shadow
        "Z Bit:",                   // lab_zbuffer
        "Max Lights:",              // lab_maxlights
        "Special Effects:",         // lab_3dfx
        "Water Quality:",           // lab_multiwater
        "Widescreen:",              // lab_widescreen
        "Resolution:",              // lab_screensize
        "Max Particles:",           // lab_maxparticles
        ""
    };

    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int    menuChoice = 0;

    static bool_t widescreen;
    static float  aspect_ratio;
    static STRING sz_screen_size;

    int cnt, result = 0;

    switch ( menuState )
    {
        case MM_Begin:

            menuChoice = 0;
            {
                // initialize most of the the sz_buttons ary
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    // initialize most of the the sz_buttons ary
                    sz_buttons[cnt] = "N/A";
                }
                sz_buttons[but_save] = "Save Settings";
                sz_buttons[but_sz_count-1] = "";

                // set up the w_buttons
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    // clear the data
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );

                    ui_Widget_set_id( w_buttons  + cnt, cnt );
                    ui_Widget_set_text( w_buttons + cnt, ui_just_centerleft, NULL, sz_buttons[cnt] );
                }

                // set up the w_labels
                for ( cnt = 0; cnt < lab_count; cnt++ )
                {
                    memset( w_labels  + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_labels  + cnt, UI_Nothing );
                    ui_Widget_set_text( w_labels + cnt, ui_just_centerleft, menuFont, sz_labels[cnt] );
                }

                // set up menu variables
                ego_texture_load_vfs( &background, "mp_data/menu/menu_video", TRANSCOLOR );

                tipText_set_position( menuFont, "Change video settings here.", 20 );

                ui_Widget_set_bound( w_labels  + lab_antialiasing, buttonLeft + 000, GFX_HEIGHT - 245, -1, 30 );
                ui_Widget_set_button( w_buttons + but_antialiasing, buttonLeft + 150, GFX_HEIGHT - 245, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_dither,       buttonLeft + 000, GFX_HEIGHT - 175, -1, 30 );
                ui_Widget_set_button( w_buttons + but_dither,       buttonLeft + 150, GFX_HEIGHT - 175, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_fullscreen,   buttonLeft + 000, GFX_HEIGHT - 140, -1, 30 );
                ui_Widget_set_button( w_buttons + but_fullscreen,   buttonLeft + 150, GFX_HEIGHT - 140, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_reflections,  buttonLeft + 000, GFX_HEIGHT - 280, -1, 30 );
                ui_Widget_set_button( w_buttons + but_reflections,  buttonLeft + 150, GFX_HEIGHT - 280, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_filtering,    buttonLeft + 000, GFX_HEIGHT - 315, -1, 30 );
                ui_Widget_set_button( w_buttons + but_filtering,    buttonLeft + 150, GFX_HEIGHT - 315, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_shadow,       buttonLeft + 000, GFX_HEIGHT - 385, -1, 30 );
                ui_Widget_set_button( w_buttons + but_shadow,       buttonLeft + 150, GFX_HEIGHT - 385, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_zbuffer,      buttonLeft + 300, GFX_HEIGHT - 350, -1, 30 );
                ui_Widget_set_button( w_buttons + but_zbuffer,      buttonLeft + 450, GFX_HEIGHT - 350, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_maxlights,    buttonLeft + 300, GFX_HEIGHT - 315, -1, 30 );
                ui_Widget_set_button( w_buttons + but_maxlights,    buttonLeft + 450, GFX_HEIGHT - 315, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_3dfx,         buttonLeft + 300, GFX_HEIGHT - 280, -1, 30 );
                ui_Widget_set_button( w_buttons + but_3dfx,         buttonLeft + 450, GFX_HEIGHT - 280, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_multiwater,   buttonLeft + 300, GFX_HEIGHT - 245, -1, 30 );
                ui_Widget_set_button( w_buttons + but_multiwater,   buttonLeft + 450, GFX_HEIGHT - 245, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_maxparticles, buttonLeft + 300, GFX_HEIGHT - 210, -1, 30 );
                ui_Widget_set_button( w_buttons + but_maxparticles, buttonLeft + 450, GFX_HEIGHT - 210, 140, 30 );

                ui_Widget_set_bound( w_labels  + lab_widescreen,   buttonLeft + 300, GFX_HEIGHT - 105, -1, 30 );
                ui_Widget_set_button( w_buttons + but_widescreen,   buttonLeft + 450, GFX_HEIGHT - 105, 30, 30 );

                ui_Widget_set_bound( w_labels  + lab_screensize,   buttonLeft + 300, GFX_HEIGHT - 140, -1, 30 );
                ui_Widget_set_button( w_buttons + but_screensize,   buttonLeft + 450, GFX_HEIGHT - 140, 140, 30 );

                SlidyButtonState_init( &but_state, 0.0f, but_save, sz_buttons, w_buttons );
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                // Draw the background
                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
                }

                // make the but_maxparticles inactive if the module is running
                if ( PMod->active )
                {
                    ui_Widget_set_id( w_buttons + but_maxparticles, UI_Nothing );
                }

                // limit some initial values
#if defined(__unix__)
                // Clip linux defaults to valid values so that the game doesn't crash on startup
                if ( cfg.scrz_req == 32 ) cfg.scrz_req = 24;
                if ( cfg.scrd_req == 32 ) cfg.scrd_req = 24;
#endif

                if ( cfg.texturefilter_req > TX_ANISOTROPIC + max_anisotropic ) cfg.texturefilter_req = 0;

                if ( cfg.scrz_req != 32 && cfg.scrz_req != 16 && cfg.scrz_req != 24 )
                {
                    cfg.scrz_req = 16;              // Set to default
                }

                if ( cfg.fullscreen_req && NULL != sdl_scr.video_mode_list )
                {
                    doOptionsVideo_fix_fullscreen_resolution( &cfg, &sdl_scr, &sz_screen_size );

                    aspect_ratio = ( float )cfg.scrx_req / ( float )cfg.scry_req;
                    widescreen = ( aspect_ratio > ( 4.0f / 3.0f ) );
                }
                else
                {
                    aspect_ratio = ( float )cfg.scrx_req / ( float )cfg.scry_req;
                    widescreen = ( aspect_ratio > ( 4.0f / 3.0f ) );
                }

                // Load all the current video settings
                doVideoOptions_update_antialiasing( w_buttons + but_antialiasing, cfg.multisamples );

                // Texture filtering
                doVideoOptions_update_texture_filter( w_buttons + but_filtering, cfg.texturefilter_req );

                doVideoOptions_update_dither( w_buttons + but_dither, cfg.use_dither );

                doVideoOptions_update_fullscreen( w_buttons + but_fullscreen, cfg.fullscreen_req );

                doVideoOptions_update_reflections( w_buttons + but_reflections, cfg.reflect_allowed, cfg.reflect_prt, cfg.reflect_fade );

                doVideoOptions_update_shadows( w_buttons + but_shadow, cfg.shadow_allowed, cfg.shadow_sprite );

                doVideoOptions_update_z_buffer( w_buttons + but_zbuffer, cfg.scrz_req );

                doVideoOptions_update_max_lights( w_buttons + but_maxlights, cfg.dyna_count_req );

                doVideoOptions_update_3d_effects( w_buttons + but_3dfx, cfg.use_phong, cfg.use_perspective, cfg.overlay_allowed, cfg.background_allowed );

                doVideoOptions_update_water_quality( w_buttons + but_multiwater, cfg.twolayerwater_allowed );

                doVideoOptions_update_max_particles( w_buttons + but_maxparticles, cfg.particle_count_req );

                doVideoOptions_update_widescreen( w_buttons + but_widescreen, widescreen );

                doVideoOptions_update_resolution( w_buttons + but_screensize, cfg.scrx_req, cfg.scry_req );
            }

            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            // Antialiasing Button
            ui_doWidget( w_labels + lab_antialiasing );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_antialiasing ) )
            {
                // make the multi-sampling even

                // set some arbitrary limit
                cfg.multisamples = CLIP( cfg.multisamples, 0, EGO_MAX_MULTISAMPLES );

                // iterate the the multisamples through the valid values (0,1,2,4,8,...)
                if ( cfg.multisamples <= 1 )
                {
                    cfg.multisamples = 2;
                }
                else
                {
                    cfg.multisamples <<= 1;
                }

                // wrap it around
                if ( cfg.multisamples > EGO_MAX_MULTISAMPLES ) cfg.multisamples = 0;

                // update the button
                doVideoOptions_update_antialiasing( w_buttons + but_antialiasing, cfg.multisamples );
            }

            // Dithering
            ui_doWidget( w_labels + lab_dither );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_dither ) )
            {
                cfg.use_dither = !cfg.use_dither;

                doVideoOptions_update_dither( w_buttons + but_dither, cfg.use_dither );
            }

            // Fullscreen
            ui_doWidget( w_labels + lab_fullscreen );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_fullscreen ) )
            {
                cfg.fullscreen_req = !cfg.fullscreen_req;

                doVideoOptions_update_fullscreen( w_buttons + but_fullscreen, cfg.fullscreen_req );
            }

            // Reflection
            ui_doWidget( w_labels + lab_reflections );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_reflections ) )
            {
                if ( cfg.reflect_allowed && cfg.reflect_fade == 0 && cfg.reflect_prt )
                {
                    cfg.reflect_allowed = bfalse;
                    cfg.reflect_fade    = 255;
                    cfg.reflect_prt     = bfalse;
                }
                else if ( cfg.reflect_allowed && !cfg.reflect_prt )
                {
                    cfg.reflect_fade = 255;
                    cfg.reflect_prt  = btrue;
                }
                else if ( cfg.reflect_allowed && cfg.reflect_fade == 255 && cfg.reflect_prt )
                {
                    cfg.reflect_fade = 0;
                }
                else
                {
                    cfg.reflect_allowed = btrue;
                    cfg.reflect_fade    = 255;
                    cfg.reflect_prt     = bfalse;
                }

                doVideoOptions_update_reflections( w_buttons + but_reflections, cfg.reflect_allowed, cfg.reflect_prt, cfg.reflect_fade );
            }

            // Texture Filtering
            ui_doWidget( w_labels + lab_filtering );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_filtering ) )
            {
                cfg.texturefilter_req = CLIP( cfg.texturefilter_req, TX_UNFILTERED, TX_ANISOTROPIC + max_anisotropic );

                cfg.texturefilter_req++;

                if ( cfg.texturefilter_req > TX_ANISOTROPIC + max_anisotropic )
                {
                    cfg.texturefilter_req = TX_UNFILTERED;
                }

                doVideoOptions_update_texture_filter( w_buttons + but_filtering, cfg.texturefilter_req );
            }

            // Shadows
            ui_doWidget( w_labels + lab_shadow );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_shadow ) )
            {
                if ( cfg.shadow_allowed && !cfg.shadow_sprite )
                {
                    cfg.shadow_allowed = bfalse;
                    cfg.shadow_sprite = bfalse;                // Just in case
                }
                else
                {
                    if ( cfg.shadow_allowed && cfg.shadow_sprite )
                    {
                        cfg.shadow_sprite = bfalse;
                    }
                    else
                    {
                        cfg.shadow_allowed = btrue;
                        cfg.shadow_sprite = btrue;
                    }
                }

                doVideoOptions_update_shadows( w_buttons + but_shadow, cfg.shadow_allowed, cfg.shadow_sprite );
            }

            // Z bit
            ui_doWidget( w_labels + lab_zbuffer );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_zbuffer ) )
            {
                if ( cfg.scrz_req < 0 )
                {
                    cfg.scrz_req = 8;
                }
                else
                {
                    cfg.scrz_req += 8;
                }

#if defined(__unix__)
                if ( cfg.scrz_req > 24 ) cfg.scrz_req = 8;            // Linux max is 24
#else
                if ( cfg.scrz_req > 32 ) cfg.scrz_req = 8;            // Others can have up to 32 bit!
#endif

                doVideoOptions_update_z_buffer( w_buttons + but_zbuffer, cfg.scrz_req );
            }

            // Max dynamic lights
            ui_doWidget( w_labels + lab_maxlights );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_maxlights ) )
            {
                cfg.dyna_count_req = CLIP( cfg.dyna_count_req, 8, TOTAL_MAX_DYNA - 1 );

                cfg.dyna_count_req += 8;

                if ( cfg.dyna_count_req > TOTAL_MAX_DYNA ) cfg.dyna_count_req = 8;

                doVideoOptions_update_max_lights( w_buttons + but_maxlights, cfg.dyna_count_req );
            }

            // Perspective correction, overlay, underlay and Phong mapping
            ui_doWidget( w_labels + lab_3dfx );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_3dfx ) )
            {
                if ( cfg.use_phong && cfg.use_perspective && cfg.overlay_allowed && cfg.background_allowed )
                {
                    cfg.use_phong          = bfalse;
                    cfg.use_perspective    = bfalse;
                    cfg.overlay_allowed    = bfalse;
                    cfg.background_allowed = bfalse;
                }
                else if ( !cfg.use_phong )
                {
                    cfg.use_phong = btrue;
                }
                else if ( !cfg.use_perspective && cfg.overlay_allowed && cfg.background_allowed )
                {
                    cfg.use_perspective = btrue;
                }
                else
                {
                    cfg.overlay_allowed = btrue;
                    cfg.background_allowed = btrue;
                }

                doVideoOptions_update_3d_effects( w_buttons + but_3dfx, cfg.use_phong, cfg.use_perspective, cfg.overlay_allowed, cfg.background_allowed );
            }

            // Water Quality
            ui_doWidget( w_labels + lab_multiwater );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_multiwater ) )
            {
                cfg.twolayerwater_allowed = !cfg.twolayerwater_allowed;

                doVideoOptions_update_water_quality( w_buttons + but_multiwater, cfg.twolayerwater_allowed );
            }

            // Max particles
            ui_doWidget( w_labels + lab_maxparticles );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_maxparticles ) )
            {
                cfg.particle_count_req = CLIP( cfg.particle_count_req, 128, TOTAL_MAX_PRT - 1 );

                cfg.particle_count_req += 128;

                if ( cfg.particle_count_req > TOTAL_MAX_PRT ) cfg.particle_count_req = 256;

                doVideoOptions_update_max_particles( w_buttons + but_maxparticles, cfg.particle_count_req );
            }

            // Widescreen
            ui_doWidget( w_labels + lab_widescreen );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_widescreen ) )
            {
                bool_t old_widescreen = widescreen;

                // toggle widescreen
                widescreen = !widescreen;

                if ( old_widescreen )
                {
                    // switch the display from widescreen to non-widescreen

                    // Set to default non-widescreen resolution
                    cfg.scrx_req = 800;
                    cfg.scry_req = 600;
                }
                else
                {
                    // switch the display from non-widescreen to widescreen

                    // Set to "default" widescreen resolution
                    cfg.scrx_req = 960;
                    cfg.scry_req = 600;
                }

                doVideoOptions_update_widescreen( w_buttons + but_widescreen, widescreen );
                doVideoOptions_update_resolution( w_buttons + but_screensize, cfg.scrx_req, cfg.scry_req );
            }

            // Screen Resolution
            ui_doWidget( w_labels + lab_screensize );
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_screensize ) )
            {
                float req_area;

                cfg.scrx_req *= 1.1f;
                cfg.scry_req *= 1.1f;

                req_area = cfg.scrx_req * cfg.scry_req;

                // use 1920x1200 as a kind of max resolution
                if ( req_area > 1920 * 1200 )
                {
                    // reset the screen size to the minimum
                    if ( widescreen )
                    {
                        // "default" widescreen
                        cfg.scrx_req = 960;
                        cfg.scry_req = 600;
                    }
                    else
                    {
                        // "default"
                        cfg.scrx_req = 800;
                        cfg.scry_req = 600;
                    }
                }

                if ( cfg.fullscreen_req && NULL != sdl_scr.video_mode_list )
                {
                    // coerce the screen size to a valid fullscreen mode
                    doOptionsVideo_fix_fullscreen_resolution( &cfg, &sdl_scr, &sz_screen_size );
                }

                aspect_ratio = ( float )cfg.scrx_req / ( float )cfg.scry_req;

                // 1.539 is "half way" between normal aspect ratio (4/3) and anamorphic (16/9)
                widescreen = ( aspect_ratio > ( 1.539f ) );

                doVideoOptions_update_widescreen( w_buttons + but_widescreen, widescreen );
                doVideoOptions_update_resolution( w_buttons + but_screensize, cfg.scrx_req, cfg.scry_req );
            }

            // Save settings button
            if ( BUTTON_UP == ui_doWidget( w_buttons + but_save ) )
            {
                doOptionsVideo_update_settings( &cfg );

                menuChoice = 1;
            }

            if ( menuChoice != 0 )
            {
                menuState = MM_Leaving;
                SlidyButtonState_init( &but_state, 0.0f, but_save, sz_buttons, w_buttons );
            }

            // tool-tip text
            display_list_draw( tipText_tx );
            break;

        case MM_Leaving:

            {
                // Do buttons sliding out and background fading
                // Do the same stuff as in MM_Entering, but backwards
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
                }
            }

            // Fall trough
            menuState = MM_Finish;
            break;

        case MM_Finish:
            {
                // Free the background texture; don't need to hold onto it
                oglx_texture_Release( &background );

                // free the widgets
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    ui_Widget_free( w_buttons + cnt );
                }

                for ( cnt = 0; cnt < lab_count; cnt++ )
                {
                    ui_Widget_free( w_labels  + cnt );
                }

                // Set the next menu to load
                result = menuChoice;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_ShowResultsState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_ShowResultsState ShowResultsState_t;
//
////--------------------------------------------------------------------------------------------
//static ShowResultsState_t *ShowResultsState_ctor( ShowResultsState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static ShowResultsState_t *ShowResultsState_dtor( ShowResultsState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static ShowResultsState_t * doShowResults_begin( ShowResultsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = ShowResultsState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ShowResultsState_t * doShowResults_entering( ShowResultsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ShowResultsState_t * doShowResults_running( ShowResultsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ShowResultsState_t * doShowResults_leaving( ShowResultsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ShowResultsState_t * doShowResults_finish( ShowResultsState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = ShowResultsState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
int doShowResults( float deltaTime )
{
    static TTF_Font      * ttf_ptr;

    static int     menuState = MM_Begin;
    static int     count;
    static char*   game_hint;
    static char    buffer[1024] = EMPTY_CSTR;

    int menuResult = 0;

    switch ( menuState )
    {
        case MM_Begin:
            {
                Uint8 i;
                char * carat = buffer, * carat_end = buffer + SDL_arraysize( buffer );

                ttf_ptr = ui_getFont();
                count = 0;

                // Prepeare the summary text
                for ( i = 0; i < SUMMARYLINES; i++ )
                {
                    carat += snprintf( carat, carat_end - carat - 1, "%s\n", mnu_ModList.lst[( MOD_REF )selectedModule].base.summary[i] );
                }

                // Randomize the next game hint, but only if not in hard mode
                game_hint = CSTR_END;
                if ( cfg.difficulty <= GAME_NORMAL )
                {
                    // Should be okay to randomize the seed here, the random seed isn't standardized or
                    // used elsewhere before the module is loaded.
                    srand( time( NULL ) );
                    if ( mnu_GameTip_load_local_vfs() )       game_hint = mnu_GameTip.local_hint[rand() % mnu_GameTip.local_count];
                    else if ( mnu_GameTip.count > 0 )     game_hint = mnu_GameTip.hint[rand() % mnu_GameTip.count];
                }
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:

            menuState = MM_Running;
            // pass through

        case MM_Running:
            {
                int text_h, text_w;
                ui_drawButton( UI_Nothing, 30, 30, GFX_WIDTH  - 60, GFX_HEIGHT - 65, NULL );

                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

                // the module name
                menuTextureList_ptr = ui_updateTextBox( menuTextureList_ptr, menuFont, 50, 80, 20, mnu_ModList.lst[( MOD_REF )selectedModule].base.longname );
                ui_drawTextBox( menuTextureList_ptr, 50, 80, 291, 230 );

                // Draw a text box
                menuTextureList_ptr = ui_updateTextBox( menuTextureList_ptr, menuFont, 50, 120, 20, buffer );
                ui_drawTextBox( menuTextureList_ptr, 50, 120, 291, 230 );

                // Loading game... please wait
                fnt_getTextSize( ttf_ptr, &text_w, &text_h, "Loading module..." );
                ui_drawTextBoxImmediate( ttf_ptr, ( GFX_WIDTH / 2 ) - text_w / 2, GFX_HEIGHT - 200, 20, "Loading module..." );

                // Draw the game tip
                if ( VALID_CSTR( game_hint ) )
                {
                    fnt_getTextSize( menuFont, &text_w, &text_h, "GAME TIP" );
                    ui_drawTextBoxImmediate( menuFont, ( GFX_WIDTH / 2 )  - text_w / 2, GFX_HEIGHT - 150, 20, "GAME TIP" );

                    fnt_getTextSize( menuFont, &text_w, &text_h, game_hint );       /// @todo ZF@> : this doesn't work as I intended, ui_get_TextSize() does not take line breaks into account
                    ui_drawTextBoxImmediate( menuFont, GFX_WIDTH / 2 - text_w / 2, GFX_HEIGHT - 110, 20, game_hint );
                }

				//ZF> Disabling this counter will speed up load times quite a bit...
                // keep track of the iterations through this section for a timer
                /*
				count++;
                if ( count > UPDATE_SKIP )
                {
                    menuState  = MM_Leaving;
                }
				*/
                menuState  = MM_Leaving;
            }
            break;

        case MM_Leaving:

            menuState = MM_Finish;
            // pass through

        case MM_Finish:
            {
                menuResult = 1;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return menuResult;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_NotImplementedState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_NotImplementedState NotImplementedState_t;
//
////--------------------------------------------------------------------------------------------
//static NotImplementedState_t *NotImplementedState_ctor( NotImplementedState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static NotImplementedState_t *NotImplementedState_dtor( NotImplementedState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static NotImplementedState_t * doNotImplemented_begin( NotImplementedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = NotImplementedState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static NotImplementedState_t * doNotImplemented_entering( NotImplementedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static NotImplementedState_t * doNotImplemented_running( NotImplementedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static NotImplementedState_t * doNotImplemented_leaving( NotImplementedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static NotImplementedState_t * doNotImplemented_finish( NotImplementedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = NotImplementedState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
int doNotImplemented( float deltaTime )
{
    int x, y;
    int w, h;

    ui_Widget_t w_buttons[1];
    char notImplementedMessage[] = "Not implemented yet!  Check back soon!";

    fnt_getTextSize( ui_getFont(), &w, &h, notImplementedMessage );
    w += 50; // add some space on the sides

    x = GFX_WIDTH  / 2 - w / 2;
    y = GFX_HEIGHT / 2 - 17;

    ui_Widget_set_id( w_buttons + 0, 0 );
    ui_Widget_set_text( w_buttons + 0, ui_just_centered, menuFont, notImplementedMessage );
    ui_Widget_set_button( w_buttons + 0, x, y, w, -1 );

    if ( BUTTON_UP == ui_doWidget( w_buttons + 0 ) )
    {
        return 1;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_GamePausedState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_GamePausedState GamePausedState_t;
//
////--------------------------------------------------------------------------------------------
//static GamePausedState_t *GamePausedState_ctor( GamePausedState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static GamePausedState_t *GamePausedState_dtor( GamePausedState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static GamePausedState_t * doGamePaused_begin( GamePausedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = GamePausedState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static GamePausedState_t * doGamePaused_entering( GamePausedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static GamePausedState_t * doGamePaused_running( GamePausedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static GamePausedState_t * doGamePaused_leaving( GamePausedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static GamePausedState_t * doGamePaused_finish( GamePausedState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = GamePausedState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
int doGamePaused( float deltaTime )
{
    static int menuState = MM_Begin;
    static int menuChoice = 0;
    static SlidyButtonState_t but_state = { NULL };

    enum
    {
        but_quit,
        but_restart,
        but_return,
        but_options,
        but_count,
        but_sz_count
    };

    // button widgets
    static ui_Widget_t w_buttons[but_count];

    static const char * sz_buttons[but_sz_count] =
    {
        "Quit Module",
        "Restart Module",
        "Return to Module",
        "Options",
        ""
    };

    int result = 0, cnt;

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            menuChoice = 0;

            {

                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    // clear the data
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_buttons + cnt, cnt );
                }

                if ( PMod->exportvalid && !local_stats.allpladead ) sz_buttons[0] = "Save and Exit";
                else                                                sz_buttons[0] = "Quit Module";

                SlidyButtonState_init( &but_state, 1.0f, 0, sz_buttons, w_buttons );
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            {
                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, -deltaTime );

                // Let lerp wind down relative to the time elapsed
                if ( but_state.lerp <= 0.0f )
                {
                    menuState = MM_Running;
                }
            }
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            // Buttons
            for ( cnt = 0; cnt < 4; cnt ++ )
            {
                if ( BUTTON_UP == ui_doWidget( w_buttons + cnt ) )
                {
                    // audio options
                    menuChoice = cnt + 1;
                }
            }

            // Quick return to game
            if ( SDLKEYDOWN( SDLK_ESCAPE ) ) menuChoice = 3;

            if ( menuChoice != 0 )
            {
                menuState = MM_Leaving;
                SlidyButtonState_init( &but_state, 0.0f, 0, sz_buttons, w_buttons );
            }
            break;

        case MM_Leaving:
            // Do sz_buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                // Buttons
                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, deltaTime );
                if ( but_state.lerp >= 1.0f )
                {
                    menuState = MM_Finish;
                }
            }
            break;

        case MM_Finish:
            {
                // Free the background texture; don't need to hold onto it

                // free the widgets
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    ui_Widget_free( w_buttons + cnt );
                }

                // Set the next menu to load
                result = menuChoice;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//struct s_ShowEndgameState
//{
//    BASE_MENU_STATE;
//};
//typedef struct s_ShowEndgameState ShowEndgameState_t;
//
////--------------------------------------------------------------------------------------------
//static ShowEndgameState_t *ShowEndgameState_ctor( ShowEndgameState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static ShowEndgameState_t *ShowEndgameState_dtor( ShowEndgameState_t * ps )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//}
//
////--------------------------------------------------------------------------------------------
//static ShowEndgameState_t * doShowEndgame_begin( ShowEndgameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    ps = ShowEndgameState_ctor( ps );
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ShowEndgameState_t * doShowEndgame_entering( ShowEndgameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ShowEndgameState_t * doShowEndgame_running( ShowEndgameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ShowEndgameState_t * doShowEndgame_leaving( ShowEndgameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    return ps;
//};
//
////--------------------------------------------------------------------------------------------
//static ShowEndgameState_t * doShowEndgame_finish( ShowEndgameState_t * ps, float deltaTime )
//{
//    if ( NULL == ps ) return ps;
//
//    /* BLAH */
//
//    ps = ShowEndgameState_ctor( ps );
//
//    return ps;
//};

//--------------------------------------------------------------------------------------------
int doShowEndgame( float deltaTime )
{
    static int menuState = MM_Begin;
    static int menuChoice = 0;
    static int x, y, w, h;
    static SlidyButtonState_t but_state = { NULL };

    enum
    {
        but_exit,
        but_count,
        but_sz_count
    };

    // button widgets
    static ui_Widget_t w_buttons[but_count];

    static const char * sz_buttons[but_sz_count] =
    {
        "BLAH",
        ""
    };

    int cnt, retval;

    retval = 0;
    switch ( menuState )
    {
        case MM_Begin:
            {
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    // clear the data
                    memset( w_buttons + cnt, 0, sizeof( ui_Widget_t ) );
                    ui_Widget_set_id( w_buttons + cnt, cnt );
                }

                SlidyButtonState_init( &but_state, 1.0f, 0, sz_buttons, w_buttons );

                if ( PMod->exportvalid )
                {
                    sz_buttons[but_exit] = "Save and Exit";
                }
                else
                {
                    sz_buttons[but_exit] = "Exit Game";
                }

                x = 70;
                y = 70;
                w = GFX_WIDTH  - 2 * x;
                h = GFX_HEIGHT - 2 * y;
            }

            // let this fall through into MM_Entering
            menuState = MM_Entering;

        case MM_Entering:
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                menuTextureList_ptr = ui_updateTextBox( menuTextureList_ptr, menuFont, x, y, 20, endtext );
                ui_drawTextBox( menuTextureList_ptr, x, y, w, h );
                SlidyButtonState_draw_all( &but_state );

                SlidyButtonState_update_all( &but_state, -deltaTime );

                // Let lerp wind down relative to the time elapsed
                if ( but_state.lerp <= 0.0f )
                {
                    menuState = MM_Running;
                }
            }
            break;

        case MM_Running:
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            // Buttons
            for ( cnt = 0; cnt < 1; cnt ++ )
            {
                if ( BUTTON_UP == ui_doWidget( w_buttons + cnt ) )
                {
                    // audio options
                    menuChoice = cnt + 1;
                    menuState = MM_Leaving;
                }
            }

            // escape also kills this menu
            if ( SDLKEYDOWN( SDLK_ESCAPE ) )
            {
                menuChoice = 1;
                menuState = MM_Leaving;
                SlidyButtonState_init( &but_state, 0.0f, 0, sz_buttons, w_buttons );
            }

            menuTextureList_ptr = ui_updateTextBox( menuTextureList_ptr, menuFont, x, y, 20, endtext );
            ui_drawTextBox( menuTextureList_ptr, x, y, w, h );

            break;

        case MM_Leaving:
            // Do sz_buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            {
                GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - but_state.lerp );

                menuTextureList_ptr = ui_updateTextBox( menuTextureList_ptr, menuFont, x, y, 20, endtext );
                ui_drawTextBox( menuTextureList_ptr, x, y, w, h );

                // Buttons
                SlidyButtonState_draw_all( &but_state );
                SlidyButtonState_update_all( &but_state, deltaTime );
                if ( but_state.lerp >= 1.0f )
                {
                    menuState = MM_Finish;
                }
            }
            break;

        case MM_Finish:
            {
                bool_t reloaded = bfalse;

                // try to pop the last module off the module stack
                reloaded = link_pop_module();

                // try to go to the world map
                // if( !reloaded )
                // {
                //    reloaded = link_load_parent( mnu_ModList.lst[pickedmodule_index].base.parent_modname, mnu_ModList.lst[pickedmodule_index].base.parent_pos );
                // }

                // fix the menu that is returned when you break out of the game
                if ( PMod->beat && start_new_player )
                {
                    // we started with a new player and beat the module... yay!
                    // now we want to graduate to the ChoosePlayer menu to
                    // build our party

                    start_new_player = bfalse;

                    // if we beat a beginner module, we want to
                    // go to ChoosePlayer instead of ChooseModule.
                    if ( mnu_stack_peek() == emnu_ChooseModule )
                    {
                        mnu_stack_pop();
                        mnu_stack_push( emnu_ChoosePlayer );
                    }
                }

                // actually quit the module
                if ( !reloaded )
                {
                    game_finish_module();
                    pickedmodule_index = -1;
                    process_kill( PROC_PBASE( GProc ) );
                }

                // free the widgets
                for ( cnt = 0; cnt < but_count; cnt++ )
                {
                    ui_Widget_free( w_buttons + cnt );
                }

                // Set the next menu to load
                retval = menuChoice;
            }

            // reset the ui
            ui_Reset();

            // make sure that if we come back to this menu, it resets properly
            menuState = MM_Begin;

            break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// place this last so that we do not have to prototype every menu function
int doMenu( float deltaTime )
{
    /// @details the global function that controls the navigation between menus

    int retval, result = 0;

    if ( mnu_whichMenu == emnu_Main )
    {
        mnu_stack_clear();
    };

    retval = MENU_NOTHING;

    switch ( mnu_whichMenu )
    {
        case emnu_Main:
            result = doMainMenu( deltaTime );
            if ( result != 0 )
            {
                if ( result == 1 )      { mnu_begin_menu( emnu_ChooseModule ); start_new_player = btrue; }
                else if ( result == 2 ) { mnu_begin_menu( emnu_ChoosePlayer ); start_new_player = bfalse; }
                else if ( result == 3 ) { mnu_begin_menu( emnu_Options ); }
                else if ( result == 4 ) retval = MENU_QUIT;  // need to request a quit somehow
            }
            break;

        case emnu_SinglePlayer:
            result = doSinglePlayerMenu( deltaTime );

            if ( result != 0 )
            {
                if ( result == 1 )
                {
                    mnu_begin_menu( emnu_ChooseModule );
                    start_new_player = btrue;
                }
                else if ( result == 2 )
                {
                    mnu_begin_menu( emnu_ChoosePlayer );
                    start_new_player = bfalse;
                }
                else if ( result == 3 )
                {
                    mnu_end_menu();
                    retval = MENU_END;
                }
                else
                {
                    mnu_begin_menu( emnu_NewPlayer );
                }
            }
            break;

        case emnu_ChooseModule:
            result = doChooseModule( deltaTime );

            if ( result == -1 )     { mnu_end_menu(); retval = MENU_END; }
            else if ( result == 1 ) mnu_begin_menu( emnu_ShowMenuResults );  // imports are not valid (starter module)
            else if ( result == 2 ) mnu_begin_menu( emnu_ShowMenuResults );  // imports are valid

            break;

        case emnu_ChoosePlayer:
            result = doChoosePlayer( deltaTime );

            if ( result == -1 )     { mnu_end_menu(); retval = MENU_END; }
            else if ( result == 1 ) mnu_begin_menu( emnu_ChooseModule );

            break;

        case emnu_Options:
            result = doOptions( deltaTime );
            if ( result != 0 )
            {
                if ( result == 1 )      mnu_begin_menu( emnu_AudioOptions );
                else if ( result == 2 ) mnu_begin_menu( emnu_InputOptions );
                else if ( result == 3 ) mnu_begin_menu( emnu_VideoOptions );
                else if ( result == 4 ) { mnu_end_menu(); retval = MENU_END; }
                else if ( result == 5 ) mnu_begin_menu( emnu_GameOptions );
            }
            break;

        case emnu_GameOptions:
            result = doOptionsGame( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_AudioOptions:
            result = doOptionsAudio( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_VideoOptions:
            result = doOptionsVideo( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_InputOptions:
            result = doOptionsInput( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_ShowMenuResults:
            result = doShowResults( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_SELECT;
            }
            break;

        case emnu_GamePaused:
            result = doGamePaused( deltaTime );
            if ( result != 0 )
            {
                if ( result == 1 )
                {
                    // "Quit Module"

                    bool_t reloaded = bfalse;

                    mnu_end_menu();

                    // try to pop the last module off the module stack
                    reloaded = link_pop_module();

                    // try to go to the world map
                    // if( !reloaded )
                    // {
                    //    reloaded = link_load_parent( mnu_ModList.lst[pickedmodule_index].base.parent_modname, mnu_ModList.lst[pickedmodule_index].base.parent_pos );
                    // }

                    if ( !reloaded )
                    {
                        game_finish_module();
                        process_kill( PROC_PBASE( GProc ) );
                    }

                    result = MENU_QUIT;
                }
                else if ( result == 2 )
                {
                    // "Restart Module"
                    mnu_end_menu();
                    game_begin_module( PMod->loadname, ( Uint32 )~0 );
                    retval = MENU_END;
                }
                else if ( result == 3 )
                {
                    // "Return to Module"
                    mnu_end_menu();
                    retval = MENU_END;
                }
                else if ( result == 4 )
                {
                    // "Options"
                    mnu_begin_menu( emnu_Options );
                }
            }
            break;

        case emnu_ShowEndgame:
            result = doShowEndgame( deltaTime );
            if ( result == 1 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_NotImplemented:
        default:
            result = doNotImplemented( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// Auto formatting functions
//--------------------------------------------------------------------------------------------
void autoformat_init( gfx_config_t * pgfx )
{
    autoformat_init_slidy_buttons();
    autoformat_init_tip_text();
    autoformat_init_copyright_text();

    if ( NULL != pgfx )
    {
        ui_set_virtual_screen( pgfx->vw, pgfx->vh, GFX_WIDTH, GFX_HEIGHT );
    }
}

//--------------------------------------------------------------------------------------------
void autoformat_init_slidy_buttons()
{
    // Figure out where to draw the buttons
    buttonLeft = 40;
    buttonTop = GFX_HEIGHT - 20;
}

//--------------------------------------------------------------------------------------------
void autoformat_init_tip_text()
{
    // set the text
    tipText = NULL;

    // Draw the options text to the right of the buttons
    tipText_left = 280;

    // And relative to the bottom of the screen
    tipText_top = GFX_HEIGHT;
}

//--------------------------------------------------------------------------------------------
void autoformat_init_copyright_text()
{
    // set the text
    copyrightText = "Welcome to Egoboo!\nhttp://egoboo.sourceforge.net\nVersion " VERSION "\n";

    // Draw the copyright text to the right of the buttons
    copyrightLeft = 280;

    // And relative to the bottom of the screen
    copyrightTop = GFX_HEIGHT;
}

//--------------------------------------------------------------------------------------------
// Implementation of tipText
//--------------------------------------------------------------------------------------------
void tipText_set_position( TTF_Font * font, const char * text, int spacing )
{
    int w, h;
    const char * new_text = NULL;

    autoformat_init_tip_text();

    if ( NULL == text ) return;

    new_text = fnt_getTextBoxSize( font, spacing, &w, &h, text );

    // set the text
    tipText = new_text;

    // Draw the options text to the right of the buttons
    tipText_left = 280;

    // And relative to the bottom of the screen
    tipText_top = GFX_HEIGHT - h - spacing;

    // actually set the texture
    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
    tipText_tx = ui_updateTextBox_literal( tipText_tx, menuFont, tipText_left, tipText_top, 20, tipText );
}

//--------------------------------------------------------------------------------------------
// Implementation of copyrightText
//--------------------------------------------------------------------------------------------
void copyrightText_set_position( TTF_Font * font, const char * text, int spacing )
{
    int w, h;

    autoformat_init_copyright_text();

    if ( NULL == text ) return;

    copyrightLeft = 0;
    copyrightLeft = 0;

    // Figure out where to draw the copyright text
    fnt_getTextBoxSize( font, 20, &w, &h, text );

    // set the text
    copyrightText = text;

    // Draw the copyright text to the right of the buttons
    copyrightLeft = 280;

    // And relative to the bottom of the screen
    copyrightTop = GFX_HEIGHT - h - spacing;

    // set the rendered ogl texture
    copyrightText_tx_ptr = ui_updateTextBox( copyrightText_tx_ptr, menuFont, copyrightLeft, copyrightTop, 20, copyrightText );
}

//--------------------------------------------------------------------------------------------
// Asset management
//--------------------------------------------------------------------------------------------
void mnu_load_all_module_images_vfs()
{
    /// @details ZZ@> This function loads the title image for each module.  Modules without a
    ///     title are marked as invalid

    STRING loadname;
    MOD_REF imod;
    vfs_FILE* filesave;

    // release all allocated data from the mnu_ModList and empty the list
    mnu_ModList_release_images();

    // Log a directory list
    filesave = vfs_openWrite( "/debug/modules.txt" );
    if ( NULL != filesave )
    {
        vfs_printf( filesave, "This file logs all of the modules found\n" );
        vfs_printf( filesave, "** Denotes an invalid module\n" );
        vfs_printf( filesave, "## Denotes an unlockable module\n\n" );
    }

    // load all the title images for modules that we are going to display
    for ( imod = 0; imod < mnu_ModList.count; imod++ )
    {
        if ( !mnu_ModList.lst[imod].loaded )
        {
            vfs_printf( filesave, "**.  %s\n", mnu_ModList.lst[imod].vfs_path );
        }
        else if ( mnu_test_by_index( imod, 0, NULL ) )
        {
            // @note just because we can't load the title image DOES NOT mean that we ignore the module
            snprintf( loadname, SDL_arraysize( loadname ), "%s/gamedat/title", mnu_ModList.lst[imod].vfs_path );

            mnu_ModList.lst[imod].tex_index = TxTitleImage_load_one_vfs( loadname );

            vfs_printf( filesave, "%02d.  %s\n", REF_TO_INT( imod ), mnu_ModList.lst[imod].vfs_path );
        }
        else
        {
            vfs_printf( filesave, "##.  %s\n", mnu_ModList.lst[imod].vfs_path );
        }
    }

    if ( NULL != filesave )
    {
        vfs_close( filesave );
    }
}

//--------------------------------------------------------------------------------------------
TX_REF mnu_get_icon_ref( const CAP_REF by_reference icap, const TX_REF by_reference default_ref )
{
    /// @details BB@> This function gets the proper icon for a an object profile.
    //
    //     In the character preview section of the menu system, we do not load
    //     entire profiles, just the character definition file ("data.txt")
    //     and one icon. Sometimes, though the item is actually a spell effect which means
    //     that we need to display the book icon.

    TX_REF icon_ref = ( TX_REF )ICON_NULL;
    bool_t is_spell_fx, is_book, draw_book;

    cap_t * pitem_cap;

    if ( !LOADED_CAP( icap ) ) return icon_ref;
    pitem_cap = CapStack.lst + icap;

    // what do we need to draw?
    is_spell_fx = ( NO_SKIN_OVERRIDE != pitem_cap->spelleffect_type );
    is_book     = ( SPELLBOOK == icap );
    draw_book   = ( is_book || is_spell_fx ) && ( bookicon_count > 0 );

    if ( !draw_book )
    {
        icon_ref = default_ref;
    }
    else if ( draw_book )
    {
        size_t iskin = 0;

        if ( pitem_cap->spelleffect_type != 0 )
        {
            iskin = pitem_cap->spelleffect_type;
        }
        else if ( pitem_cap->skin_override != 0 )
        {
            iskin = pitem_cap->skin_override;
        }

        iskin = CLIP( iskin, 0, bookicon_count );

        icon_ref = bookicon_ref[ iskin ];
    }

    return icon_ref;
}

//--------------------------------------------------------------------------------------------
// module utilities
//--------------------------------------------------------------------------------------------
int mnu_get_mod_number( const char *szModName )
{
    /// @details ZZ@> This function returns -1 if the module does not exist locally, the module
    ///    index otherwise

    MOD_REF modnum;
    int     retval = -1;

    for ( modnum = 0; modnum < mnu_ModList.count; modnum++ )
    {
        if ( 0 == strcmp( mnu_ModList.lst[modnum].vfs_path, szModName ) )
        {
            retval = REF_TO_INT( modnum );
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_test_by_index( const MOD_REF by_reference modnumber, size_t buffer_len, char * buffer )
{
    int            cnt;
    mnu_module_t * pmod;
    bool_t         allowed;

    if ( INVALID_MOD( modnumber ) ) return bfalse;
    pmod = mnu_ModList.lst + modnumber;

    // First check if we are in developers mode or that the right module has been beaten before
    allowed = bfalse;
    if ( cfg.dev_mode || module_has_idsz_vfs( pmod->base.reference, pmod->base.unlockquest.id, buffer_len, buffer ) )
    {
        allowed = btrue;
    }
    else if ( pmod->base.importamount > 0 )
    {
        int player_count = 0;
        int player_allowed = 0;

        // If that did not work, then check all selected players directories, but only if it isn't a starter module
        for ( cnt = 0; cnt < mnu_selectedPlayerCount; cnt++ )
        {
            int                quest_level = QUEST_NONE;
            LOAD_PLAYER_INFO * ploadplayer = loadplayer + mnu_selectedPlayer[cnt];

            player_count++;

            quest_level = quest_get_level( ploadplayer->quest_log, SDL_arraysize( ploadplayer->quest_log ), pmod->base.unlockquest.id );

            // find beaten quests or quests with proper level
            if ( quest_level <= QUEST_BEATEN || pmod->base.unlockquest.level <= quest_level )
            {
                player_allowed++;
            }
        }

        allowed = ( player_allowed == player_count );
    }

    return allowed;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_test_by_name( const char *szModName )
{
    /// @details ZZ@> This function tests to see if a module can be entered by
    ///    the players

    bool_t retval;

    // find the module by name
    int modnumber = mnu_get_mod_number( szModName );

    retval = bfalse;
    if ( modnumber >= 0 )
    {
        retval = mnu_test_by_index(( MOD_REF )modnumber, 0, NULL );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void mnu_module_init( mnu_module_t * pmod )
{
    if ( NULL == pmod ) return;

    // clear the module
    memset( pmod, 0, sizeof( *pmod ) );

    pmod->tex_index = INVALID_TITLE_TEXTURE;
}

//--------------------------------------------------------------------------------------------
void mnu_load_all_module_info()
{
    vfs_search_context_t * ctxt;

    const char *vfs_ModPath;
    STRING      loadname;

    // reset the module list
    mnu_ModList_release_all();

    // Search for all .mod directories and load the module info
    ctxt = vfs_findFirst( "mp_modules", "mod", VFS_SEARCH_DIR );
    vfs_ModPath = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( vfs_ModPath ) && mnu_ModList.count < MAX_MODULE )
    {
        mnu_module_t * pmod = mnu_ModList.lst + ( MOD_REF )mnu_ModList.count;

        // clear the module
        mnu_module_init( pmod );

        // save the filename
        snprintf( loadname, SDL_arraysize( loadname ), "%s/gamedat/menu.txt", vfs_ModPath );
        if ( NULL != module_load_info_vfs( loadname, &( pmod->base ) ) )
        {
            mnu_ModList.count++;

            // mark the module data as loaded
            pmod->loaded = btrue;

            // save the module path
            strncpy( pmod->vfs_path, vfs_ModPath, SDL_arraysize( pmod->vfs_path ) );

            // Save the user data directory version of the module path.
            // @note This is kinda a cheat since we know that the virtual paths all begin with "mp_" at the moment.
            // If that changes, this line must be changed as well.
            snprintf( pmod->dest_path, SDL_arraysize( pmod->dest_path ), "/%s", vfs_ModPath + 3 );

            // same problem as above
            strncpy( pmod->name, vfs_ModPath + 11, SDL_arraysize( pmod->name ) );
        };

        ctxt = vfs_findNext( &ctxt );
        vfs_ModPath = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );

    module_list_valid = btrue;
}

//--------------------------------------------------------------------------------------------
void mnu_release_one_module( const MOD_REF by_reference imod )
{
    mnu_module_t * pmod;

    if ( !VALID_MOD( imod ) ) return;
    pmod = mnu_ModList.lst + imod;

    TxTitleImage_release_one( pmod->tex_index );
    pmod->tex_index = INVALID_TITLE_TEXTURE;
}

//--------------------------------------------------------------------------------------------
// Implementation of the ModList struct
//--------------------------------------------------------------------------------------------
mod_file_t * mnu_ModList_get_base( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return &( mnu_ModList.lst[( MOD_REF )imod].base );
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_vfs_path( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return mnu_ModList.lst[( MOD_REF )imod].vfs_path;
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_dest_path( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return mnu_ModList.lst[( MOD_REF )imod].dest_path;
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_name( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return mnu_ModList.lst[( MOD_REF )imod].name;
}

//--------------------------------------------------------------------------------------------
void mnu_ModList_release_all()
{
    MOD_REF cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        // release any allocated data
        if ( cnt < mnu_ModList.count )
        {
            mnu_release_one_module( cnt );
        }

        memset( mnu_ModList.lst + cnt, 0, sizeof( mnu_module_t ) );
    }

    mnu_ModList.count = 0;
}

//--------------------------------------------------------------------------------------------
void mnu_ModList_release_images()
{
    MOD_REF cnt;
    int tnc;

    tnc = -1;
    for ( cnt = 0; cnt < mnu_ModList.count; cnt++ )
    {
        if ( !mnu_ModList.lst[cnt].loaded ) continue;
        tnc = REF_TO_INT( cnt );

        TxTitleImage_release_one( mnu_ModList.lst[cnt].tex_index );
        mnu_ModList.lst[cnt].tex_index = INVALID_TITLE_TEXTURE;
    }
    TxTitleImage.count = 0;

    // make sure that mnu_ModList.count is the right size, in case some modules were unloaded?
    mnu_ModList.count = tnc + 1;
}

//--------------------------------------------------------------------------------------------
// Functions for implementing the TxTitleImage array of textures
//--------------------------------------------------------------------------------------------
void TxTitleImage_clear_data()
{
    TxTitleImage.count = 0;
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_ctor()
{
    /// @details ZZ@> This function clears out all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        oglx_texture_ctor( TxTitleImage.lst + cnt );
    }

    TxTitleImage_clear_data();
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_release_one( const TX_REF by_reference index )
{
    if ( index < 0 || index >= MAX_MODULE ) return;

    oglx_texture_Release( TxTitleImage.lst + index );
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_release_all()
{
    /// @details ZZ@> This function releases all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        TxTitleImage_release_one( cnt );
    }

    TxTitleImage_clear_data();
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_dtor()
{
    /// @details ZZ@> This function clears out all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        oglx_texture_dtor( TxTitleImage.lst + cnt );
    }

    TxTitleImage_clear_data();
}

//--------------------------------------------------------------------------------------------
TX_REF TxTitleImage_load_one_vfs( const char *szLoadName )
{
    /// @details ZZ@> This function loads a title in the specified image slot, forcing it into
    ///    system memory.  Returns btrue if it worked

    TX_REF itex;

    if ( INVALID_CSTR( szLoadName ) ) return ( TX_REF )INVALID_TITLE_TEXTURE;

    if ( TxTitleImage.count >= TITLE_TEXTURE_COUNT ) return ( TX_REF )INVALID_TITLE_TEXTURE;

    itex  = ( TX_REF )TxTitleImage.count;
    if ( INVALID_GL_ID != ego_texture_load_vfs( TxTitleImage.lst + itex, szLoadName, INVALID_KEY ) )
    {
        TxTitleImage.count++;
    }
    else
    {
        itex = ( TX_REF )INVALID_TITLE_TEXTURE;
    }

    return itex;
}

//--------------------------------------------------------------------------------------------
oglx_texture_t * TxTitleImage_get_ptr( const TX_REF by_reference itex )
{
    if ( itex >= TxTitleImage.count || itex >= MAX_MODULE ) return NULL;

    return TxTitleImage.lst + itex;
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_reload_all()
{
    /// @details ZZ@> This function re-loads all the current textures back into
    ///               OpenGL texture memory using the cached SDL surfaces

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_TEXTURE_COUNT; cnt++ )
    {
        oglx_texture_t * ptex = TxTitleImage.lst + cnt;

        if ( ptex->valid )
        {
            oglx_texture_Convert( ptex, ptex->surface, INVALID_KEY );
        }
    }
}

//--------------------------------------------------------------------------------------------
// Implementation of the mnu_GameTip system
//--------------------------------------------------------------------------------------------
void mnu_GameTip_load_global_vfs()
{
    /// ZF@> This function loads all of the game hints and tips
    STRING buffer;
    vfs_FILE *fileread;
    Uint8 cnt;

    // reset the count
    mnu_GameTip.count = 0;

    // Open the file with all the tips
    fileread = vfs_openRead( "mp_data/gametips.txt" );
    if ( NULL == fileread )
    {
        log_warning( "Could not load the game tips and hints. (\"mp_data/gametips.txt\")\n" );
        return;
    }

    // Load the data
    for ( cnt = 0; cnt < MENU_MAX_GAMETIPS && !vfs_eof( fileread ); cnt++ )
    {
        if ( goto_colon( NULL, fileread, btrue ) )
        {
            // Read the line
            fget_string( fileread, buffer, SDL_arraysize( buffer ) );
            strcpy( mnu_GameTip.hint[cnt], buffer );

            // Make it look nice
            str_decode( mnu_GameTip.hint[cnt], SDL_arraysize( mnu_GameTip.hint[cnt] ), mnu_GameTip.hint[cnt] );
            //str_add_linebreaks( mnu_GameTip.hint[cnt], SDL_arraysize( mnu_GameTip.hint[cnt] ), 50 );

            // Keep track of how many we have total
            mnu_GameTip.count++;
        }
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
bool_t mnu_GameTip_load_local_vfs()
{
    /// ZF@> This function loads all module specific hints and tips. If this fails, the game will
    //       default to the global hints and tips instead

    STRING buffer;
    vfs_FILE *fileread;
    Uint8 cnt;

    // reset the count
    mnu_GameTip.local_count = 0;

    // Open all the tips
    fileread = vfs_openRead( "mp_data/gametips.txt" );
    if ( NULL == fileread ) return bfalse;

    // Load the data
    for ( cnt = 0; cnt < MENU_MAX_GAMETIPS && !vfs_eof( fileread ); cnt++ )
    {
        if ( goto_colon( NULL, fileread, btrue ) )
        {
            // Read the line
            fget_string( fileread, buffer, SDL_arraysize( buffer ) );
            strcpy( mnu_GameTip.local_hint[cnt], buffer );

            // Make it look nice
            str_decode( mnu_GameTip.local_hint[cnt], SDL_arraysize( mnu_GameTip.local_hint[cnt] ), mnu_GameTip.local_hint[cnt] );
            //str_add_linebreaks( mnu_GameTip.local_hint[cnt], SDL_arraysize( mnu_GameTip.local_hint[cnt] ), 50 );

            // Keep track of how many we have total
            mnu_GameTip.local_count++;
        }
    }

    vfs_close( fileread );

    return mnu_GameTip.local_count > 0;
}

//--------------------------------------------------------------------------------------------
// Implementation of the mnu_SlidyButton array
//--------------------------------------------------------------------------------------------
SlidyButtonState_t * SlidyButtonState_init( SlidyButtonState_t * pstate, float lerp, int id_start, const char *button_text[], ui_Widget_t * button_widget )
{
    size_t i, count;

    if ( NULL == pstate ) return pstate;

    // clear the state
    memset( pstate, 0, sizeof( *pstate ) );

    autoformat_init_slidy_buttons();

    // return if there is no valid text
    if ( NULL == button_text || NULL == button_text[0] || '\0' == button_text[0][0] ) return pstate;

    // Figure out where to start drawing the buttons
    for ( count = 0, i = id_start; NULL != button_text[i] && '\0' != button_text[i][0]; count++, i++ )
    {
        buttonTop -= 35;
    }

    // if there are no buttons, return
    if ( 0 == count ) return pstate;

    // set the correct parameters
    pstate->lerp      = lerp;
    pstate->but_text  = ( const char ** )( NULL == button_text ? NULL : button_text + id_start );
    pstate->but       = ( NULL == button_widget ) ? NULL : button_widget + id_start;
    pstate->but_count = count;

    // if there are widgets
    if ( NULL != button_widget )
    {
        // automatically configure the widgets
        for ( i = 0; i < count; i++ )
        {
            size_t          j = i + id_start;
            ui_Widget_t * pw  = button_widget + j;
            const char *  txt = ( NULL == button_text ) ? NULL : button_text[j];

            float x = buttonLeft - ( 360 - i * 35 )  * pstate->lerp;
            float y = buttonTop + i * 35;

            ui_Widget_set_id( pw, j );
            ui_Widget_set_button( pw, x, y, 200, 30 );
            ui_Widget_set_text( pw, ui_just_centered, NULL, txt );
        }
    }

    return pstate;
}

//--------------------------------------------------------------------------------------------
SlidyButtonState_t * SlidyButtonState_update_all( SlidyButtonState_t * pstate, float deltaTime )
{
    size_t i;

    if ( NULL == pstate ) return pstate;

    pstate->lerp += ( deltaTime * 1.5f );

    for ( i = 0; i < pstate->but_count; i++ )
    {
        ui_Widget_t * pw = pstate->but + i;

        float x = buttonLeft - ( 360 - i * 35 )  * pstate->lerp;
        float y = pw->vy;

        ui_Widget_set_pos( pw, x, y );
    }

    return pstate;
}

//--------------------------------------------------------------------------------------------
SlidyButtonState_t * SlidyButtonState_draw_all( SlidyButtonState_t * pstate )
{
    size_t i;

    if ( NULL == pstate ) return pstate;

    for ( i = 0; i < pstate->but_count; i++ )
    {
        ui_doWidget( pstate->but + i );
    }

    return pstate;
}

//--------------------------------------------------------------------------------------------
// implementation of the mnu_Selected* arrays
//--------------------------------------------------------------------------------------------
bool_t mnu_Selected_check_loadplayer( int loadplayer_idx )
{
    int i;
    if ( loadplayer_idx > loadplayer_count ) return bfalse;

    for ( i = 0; i < MAX_PLAYER && i < mnu_selectedPlayerCount; i++ )
    {
        if ( mnu_selectedPlayer[i] == loadplayer_idx ) return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
int mnu_Selected_get_loadplayer( int loadplayer_idx )
{
    int cnt, selected_index;

    if ( loadplayer_idx > loadplayer_count ) return INVALID_PLAYER;

    selected_index = INVALID_PLAYER;
    for ( cnt = 0; cnt < MAX_PLAYER && cnt < mnu_selectedPlayerCount; cnt++ )
    {
        if ( mnu_selectedPlayer[ cnt ] == loadplayer_idx )
        {
            selected_index = cnt;
            break;
        }
    }

    return selected_index;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_Selected_add( int loadplayer_idx )
{
    if ( loadplayer_idx > loadplayer_count || mnu_selectedPlayerCount >= MAX_PLAYER ) return bfalse;
    if ( mnu_Selected_check_loadplayer( loadplayer_idx ) ) return bfalse;

    mnu_selectedPlayer[mnu_selectedPlayerCount] = loadplayer_idx;
    mnu_selectedInput[mnu_selectedPlayerCount]  = INPUT_BITS_NONE;
    mnu_selectedPlayerCount++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_Selected_remove( int loadplayer_idx )
{
    int i;
    bool_t found = bfalse;

    if ( loadplayer_idx > loadplayer_count || mnu_selectedPlayerCount <= 0 ) return bfalse;

    if ( mnu_selectedPlayerCount == 1 )
    {
        if ( mnu_selectedPlayer[0] == loadplayer_idx )
        {
            mnu_selectedPlayerCount = 0;
        };
    }
    else
    {
        for ( i = 0; i < MAX_PLAYER && i < mnu_selectedPlayerCount; i++ )
        {
            if ( mnu_selectedPlayer[i] == loadplayer_idx )
            {
                found = btrue;
                break;
            }
        }

        if ( found )
        {
            i++;
            for ( /* nothing */; i < MAX_PLAYER && i < mnu_selectedPlayerCount; i++ )
            {
                mnu_selectedPlayer[i-1] = mnu_selectedPlayer[i];
                mnu_selectedInput[i-1]  = mnu_selectedInput[i];
            }

            mnu_selectedPlayerCount--;
        }
    };

    return found;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_Selected_add_input( int loadplayer_idx, BIT_FIELD input_bits )
{
    int i;
    bool_t done, retval = bfalse;

    int selected_index = -1;

    for ( i = 0; i < mnu_selectedPlayerCount; i++ )
    {
        if ( mnu_selectedPlayer[i] == loadplayer_idx )
        {
            selected_index = i;
            break;
        }
    }

    if ( -1 == selected_index )
    {
        mnu_Selected_add( loadplayer_idx );
    }

    if ( selected_index >= 0 && selected_index < mnu_selectedPlayerCount )
    {
        for ( i = 0; i < mnu_selectedPlayerCount; i++ )
        {
            if ( i == selected_index )
            {
                // add in the selected bits for the selected loadplayer_idx
                retval = ( 0 != input_bits ) && ( input_bits != ( input_bits & mnu_selectedInput[i] ) );
                ADD_BITS( mnu_selectedInput[i], input_bits );
            }
            else
            {
                // remove the selectd bits from all other players
                REMOVE_BITS( mnu_selectedInput[i], input_bits );
            }
        }
    }

    // Do the tricky part of removing all players with invalid inputs from the list
    // It is tricky because removing a loadplayer_idx changes the value of the loop control
    // value mnu_selectedPlayerCount within the loop.
    done = bfalse;
    while ( !done )
    {
        // assume the best
        done = btrue;

        for ( i = 0; i < mnu_selectedPlayerCount; i++ )
        {
            if ( INPUT_BITS_NONE == mnu_selectedInput[i] )
            {
                // we found one
                done = bfalse;
                mnu_Selected_remove( mnu_selectedPlayer[i] );
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_Selected_remove_input( int loadplayer_idx, Uint32 input_bits )
{
    int i;
    bool_t retval = bfalse;

    for ( i = 0; i < MAX_PLAYER && i < mnu_selectedPlayerCount; i++ )
    {
        if ( mnu_selectedPlayer[i] == loadplayer_idx )
        {
            bool_t removing_bits = bfalse;
            bool_t removing_player = bfalse;

            removing_bits = HAS_SOME_BITS( input_bits, mnu_selectedInput[i] );

            REMOVE_BITS( mnu_selectedInput[i], input_bits );

            // This part is not so tricky as in mnu_Selected_add_input.
            // Even though we are modding the loop control variable, it is never
            // tested in the loop because we are using the break command to
            // break out of the loop immediately

            removing_player = bfalse;
            if ( INPUT_BITS_NONE == mnu_selectedInput[i] )
            {
                removing_player = mnu_Selected_remove( loadplayer_idx );
            }

            retval = removing_bits || removing_player;

            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// Implementation of the loadplayer array
//--------------------------------------------------------------------------------------------
void loadplayer_init()
{
    // restart from nothing
    loadplayer_count = 0;
    release_all_profile_textures();

    chop_data_init( &chop_mem );
}

//--------------------------------------------------------------------------------------------
bool_t loadplayer_import_one( const char * foundfile )
{
    STRING filename;
    LOAD_PLAYER_INFO * pinfo;
    int skin = 0;

    if ( !VALID_CSTR( foundfile ) || !vfs_exists( foundfile ) ) return bfalse;

    if ( loadplayer_count >= MAXLOADPLAYER ) return bfalse;

    pinfo = loadplayer + loadplayer_count;

    snprintf( filename, SDL_arraysize( filename ), "%s/skin.txt", foundfile );
    skin = read_skin_vfs( filename );

    //snprintf( filename, SDL_arraysize(filename), "%s" SLASH_STR "tris.md2", foundfile );
    //md2_load_one( vfs_resolveReadFilename(filename), &(MadStack.lst[loadplayer_count].md2_data) );

    snprintf( filename, SDL_arraysize( filename ), "%s/icon%d", foundfile, skin );
    pinfo->tx_ref = TxTexture_load_one_vfs( filename, ( TX_REF )INVALID_TX_TEXTURE, INVALID_KEY );

    // quest info
    snprintf( pinfo->dir, SDL_arraysize( pinfo->dir ), "%s", str_convert_slash_net(( char* )foundfile, strlen( foundfile ) ) );
    quest_log_download_vfs( pinfo->quest_log, SDL_arraysize( pinfo->quest_log ), pinfo->dir );

    // load the chop data
    snprintf( filename, SDL_arraysize( filename ), "%s/naming.txt", foundfile );
    chop_load_vfs( &chop_mem, filename, &( pinfo->chop ) );

    // generate the name from the chop
    snprintf( pinfo->name, SDL_arraysize( pinfo->name ), "%s", chop_create( &chop_mem, &( pinfo->chop ) ) );

    loadplayer_count++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
// Player utilities
//--------------------------------------------------------------------------------------------
void mnu_player_check_import( const char *dirname, bool_t initialize )
{
    /// @details ZZ@> This function figures out which players may be imported, and loads basic
    ///     data for each

    vfs_search_context_t * ctxt;
    const char *foundfile;

    if ( initialize )
    {
        loadplayer_init();
    };

    // Search for all objects
    ctxt = vfs_findFirst( dirname, "obj", VFS_SEARCH_DIR );
    foundfile = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( foundfile ) && loadplayer_count < MAXLOADPLAYER )
    {
        loadplayer_import_one( foundfile );

        ctxt = vfs_findNext( &ctxt );
        foundfile = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );
}
