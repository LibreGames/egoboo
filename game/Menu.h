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

#include "char.h"
#include "ogl_texture.h"
#include "module.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum mnu_e
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

struct CGame_t;

// in C++ this would inherit from ProcState
typedef struct MenuProc_t
{
  egoboo_key ekey;

  // the "inherited" structure
  ProcState proc;

  // extra data for the menus
  enum mnu_e whichMenu, lastMenu;
  int        MenuResult;

  double     dUpdate;

  struct CClient_t * cl;
  struct CServer_t * sv;
  struct CNet_t    * net;

  GLtexture TxTitleImage[MAXMODULE];      /* title images */

  int selectedModule;
  int validModules_count;
  int validModules[MAXMODULE];

} MenuProc;

MenuProc * MenuProc_new(MenuProc *ms);
bool_t     MenuProc_delete(MenuProc * ms);
MenuProc * MenuProc_renew(MenuProc *ms);
bool_t     MenuProc_init(MenuProc * ms);
bool_t     MenuProc_init_ingame(MenuProc * ms);

//--------------------------------------------------------------------------------------------
// All the different menus.  yay!

//Input player control
#define MAXLOADPLAYER     100
typedef struct load_player_info_t
{
  STRING name;
  STRING dir;
} LOAD_PLAYER_INFO;

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

int mnu_Run( MenuProc * ms );
int mnu_RunIngame( MenuProc * ms );

Uint32 mnu_load_titleimage( MenuProc * mproc, int titleimage, char *szLoadName );
void   mnu_free_all_titleimages(MenuProc * mproc);
size_t mnu_load_mod_data(MenuProc * mproc, MOD_INFO * mi, size_t sz);

bool_t mnu_load_cl_images(MenuProc * mproc);
void   mnu_free_all_titleimages(MenuProc * mproc);
void   mnu_prime_titleimage(MenuProc * mproc);
void   mnu_prime_modules(MenuProc * mproc);