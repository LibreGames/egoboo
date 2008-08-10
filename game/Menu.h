//********************************************************************************************
//* Egoboo - menu.h
//*
//* Implements the main menu tree, using the code in Ui.*.
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

#include "ogl_texture.h"
#include "egoboo_types.h"
#include "module.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum e_mnu
{
  mnu_NotImplemented,
  mnu_Main,
  mnu_SinglePlayer,
  mnu_MultiPlayer,
  mnu_Network,
  mnu_HostGame,
  mnu_UnhostGame,
  mnu_JoinGame,
  mnu_ChooseModule,
  mnu_ChoosePlayer,
  mnu_TestResults,
  mnu_Options,
  mnu_VideoOptions,
  mnu_AudioOptions,
  mnu_InputOptions,
  mnu_NewPlayer,
  mnu_LoadPlayer,
  mnu_NetworkOff,
  mnu_Inventory,
  mnu_Quit
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct sGame;

// in C++ this would inherit from ProcState_t
struct sMenuProc
{
  egoboo_key_t ekey;

  // the "inherited" structure
  ProcState_t proc;

  // extra data for the menus
  enum e_mnu whichMenu, lastMenu;
  int        MenuResult;

  double     dUpdate;

  struct sClient * cl;
  struct sServer * sv;
  struct sNet    * net;

  GLtexture TxTitleImage[MAXMODULE];      /* title images */

  int selectedModule;
  int validModules_count;
  int validModules[MAXMODULE];

};
typedef struct sMenuProc MenuProc_t;

MenuProc_t * MenuProc_new(MenuProc_t *ms);
bool_t       MenuProc_delete(MenuProc_t * ms);
MenuProc_t * MenuProc_renew(MenuProc_t *ms);
bool_t       MenuProc_init(MenuProc_t * ms);
bool_t       MenuProc_init_ingame(MenuProc_t * ms);
bool_t       MenuProc_resynch(MenuProc_t * ms, struct sGame * gs);

//--------------------------------------------------------------------------------------------
// All the different menus.  yay!

//Input player control
#define MAXLOADPLAYER     100
struct s_load_player_info
{
  STRING name;
  STRING dir;
};
typedef struct s_load_player_info LOAD_PLAYER_INFO;

extern int              loadplayer_count;
extern LOAD_PLAYER_INFO loadplayer[MAXLOADPLAYER];

void mnu_frameStep();
void mnu_saveSettings();
void mnu_service_select();
void mnu_start_or_join();
void mnu_pick_player( int module );
void mnu_module_loading( int module );
void mnu_choose_host();
void mnu_choose_module();
void mnu_boot_players();
void mnu_end_text();
void mnu_initial_text();

void mnu_enterMenuMode();
void mnu_exitMenuMode();

int mnu_Run( MenuProc_t * ms );
int mnu_RunIngame( MenuProc_t * ms );

Uint32 mnu_load_titleimage( MenuProc_t * mproc, int titleimage, char *szLoadName );
void   mnu_free_all_titleimages(MenuProc_t * mproc);
size_t mnu_load_mod_data(MenuProc_t * mproc, MOD_INFO * mi, size_t sz);

bool_t mnu_load_cl_images(MenuProc_t * mproc);
void   mnu_free_all_titleimages(MenuProc_t * mproc);
void   mnu_prime_titleimage(MenuProc_t * mproc);
void   mnu_prime_modules(MenuProc_t * mproc);
