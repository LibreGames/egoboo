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

/// @file menu.h
/// @details Implements the main menu tree, using the code in Ui.*.

#include "network.h"
#include "profile.h"

#include "egoboo_process.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_mod_file;
struct s_gfx_config;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a process that controls the menu system
struct s_menu_process
{
    process_t base;

    bool_t was_active;
    bool_t escape_requested, escape_latch;

    int    ticks_next, ticks_now;
};
typedef struct s_menu_process menu_process_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// All the possible Egoboo menus.  yay!
enum e_which_menu
{
    emnu_Main,
    emnu_SinglePlayer,
    emnu_MultiPlayer,
    emnu_ChooseModule,
    emnu_ChoosePlayer,
    emnu_ShowMenuResults,
    emnu_Options,
    emnu_GameOptions,
    emnu_VideoOptions,
    emnu_AudioOptions,
    emnu_InputOptions,
    emnu_NewPlayer,
    emnu_LoadPlayer,
    emnu_HostGame,
    emnu_JoinGame,
    emnu_GamePaused,
    emnu_ShowEndgame,
    emnu_NotImplemented
};
typedef enum e_which_menu which_menu_t;

enum e_menu_retvals
{
    MENU_SELECT   =  1,
    MENU_NOTHING  =  0,
    MENU_END      = -1,
    MENU_QUIT     = -2
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Input player control
#define MAXLOADPLAYER     100

/// data for caching the which players may be loaded
struct s_load_player_info
{
    STRING name;					///< the object's name
    STRING dir;						///< the object's full path
    TX_REF tx_ref;					///< the index of the texture
	quest_t quest_log[MAX_QUESTS];	///< all the quests this player has

    chop_definition_t chop;   ///< put this here so we can generate a name without loading an entire profile
};
typedef struct s_load_player_info LOAD_PLAYER_INFO;

extern int              loadplayer_count;
extern LOAD_PLAYER_INFO loadplayer[MAXLOADPLAYER];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern bool_t mnu_draw_background;

extern menu_process_t * MProc;

extern bool_t module_list_valid;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// code for initializing and deinitializing the menu system
int  menu_system_begin();
void menu_system_end();

// global function to control navigation of the game menus
int doMenu( float deltaTime );

// code to start and stop menus
bool_t mnu_begin_menu( which_menu_t which );
void   mnu_end_menu();
int    mnu_get_menu_depth();

void  mnu_player_check_import( const char *dirname, bool_t initialize );

// "public" implmentation of the TxTitleImage array
void   TxTitleImage_reload_all();
TX_REF TxTitleImage_load_one_vfs( const char *szLoadName );

extern bool_t start_new_player;

// "public" implementation of mnu_ModList
struct s_mod_file * mnu_ModList_get_base( int imod );
const char *        mnu_ModList_get_vfs_path( int imod );
const char *        mnu_ModList_get_dest_path( int imod );
const char *        mnu_ModList_get_name( int imod );

// "public" module utilities
int    mnu_get_mod_number( const char *szModName );
bool_t mnu_test_by_name( const char *szModName );
bool_t mnu_test_by_index( const MOD_REF by_reference modnumber, size_t buffer_len, char * buffer );

// "public" menu process hooks
int                  do_menu_proc_run( menu_process_t * mproc, double frameDuration );
menu_process_t     * menu_process_init( menu_process_t * mproc );

// "public" reset of the autoformatting
void autoformat_init( struct s_gfx_config * pgfx );

#define egoboo_Menu_h