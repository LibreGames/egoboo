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
/// @brief Egoboo Module Manager
/// @details Definitions for managing egoboo modules

#include "ogl_texture.h"
#include "egoboo_types.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct sGame;
struct sGraphics_Data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define RANKSIZE 8						///< Max characters for module difficulity
#define SUMMARYLINES 8                  ///< Max lines of module description
#define SUMMARYSIZE  80					///< Max characters in a single description line

#define MAXMODULE           100         ///< Number of modules

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_import_info
{
  bool_t valid;                ///< Can it import?

  OBJ_REF object;
  int     player;

  size_t  amount;
  int     min_pla;
  int     max_pla;

  OBJ_REF slot_lst[1024];

};
typedef struct s_import_info IMPORT_INFO;

bool_t import_info_clear(IMPORT_INFO * ii);
bool_t import_info_add(IMPORT_INFO * ii, OBJ_REF obj);

//--------------------------------------------------------------------------------------------

typedef struct s_mod_info
{
  egoboo_key_t ekey;

  char            rank[RANKSIZE];               ///< Number of stars
  char            longname[32];                 ///< Module names
  char            loadname[32];                 ///< Module load names
  Uint8           importamount;                 ///< # of import characters
  bool_t          allowexport;                  ///< Export characters?
  Uint8           minplayers;                   ///< Number of players
  Uint8           maxplayers;                   //
  bool_t          monstersonly;                 ///< Only allow monsters
  RESPAWN_MODE    respawnmode;                ///< What kind of respawn is allowed?
  //bool_t          rts_control;

  STRING host;                         ///< what is the host of this module? blank if network not being used.
  Uint32 tx_title_idx;                  ///< which texture do we use?
  bool_t is_hosted;                    ///< is this module here, or on the server?
  bool_t is_verified;
} MOD_INFO;

MOD_INFO * ModInfo_new( MOD_INFO * pmi );
bool_t     ModInfo_delete( MOD_INFO * pmi );
MOD_INFO * ModInfo_renew( MOD_INFO * pmi );

//--------------------------------------------------------------------------------------------
struct sModSummary
{
  egoboo_key_t ekey;

  int    numlines;                                   ///< Lines in summary
  char   val;
  char   summary[SUMMARYLINES][SUMMARYSIZE];      ///< Quest description
};
typedef struct sModSummary ModSummary_t;

ModSummary_t * ModSummary_new   ( ModSummary_t * ms );
bool_t         ModSummary_delete( ModSummary_t * ms );
ModSummary_t * ModSummary_renew ( ModSummary_t * ms );

//--------------------------------------------------------------------------------------------
struct sModState
{
  egoboo_key_t ekey;

  bool_t Active;                     ///< Is the control loop still going?
  bool_t Paused;                     ///< Is the control loop paused?

  bool_t loaded;
  Uint32 seed;                       ///< the seed for the module
  bool_t respawnvalid;               ///< Can players respawn with Spacebar?
  bool_t respawnanytime;             ///< True if it's a small level...
  bool_t exportvalid;                ///< Can it export?
  //bool_t rts_control;                ///< Play as a real-time stragedy? BAD REMOVE
  bool_t beat;                       ///< Show Module Ended text?

  int    import_amount;
  int    import_min_pla;
  int    import_max_pla;
  bool_t import_valid;
};
typedef struct sModState ModState_t;

ModState_t * ModState_new(ModState_t * ms, MOD_INFO * mi, Uint32 seed);
bool_t       ModState_delete(ModState_t * ms);
ModState_t * ModState_renew(ModState_t * ms, MOD_INFO * mi, Uint32 seed);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t module_load( struct sGame * gs, const char *smallname );
void   module_release(  struct sGame * gs  );
void   module_quit( struct sGame * gs );

bool_t module_reference_matches( const char *szLoadName, IDSZ idsz );
void   module_add_idsz( const char *szLoadName, IDSZ idsz );
int    module_find( const char *smallname, MOD_INFO * mi_ary, size_t mi_size );
bool_t module_read_data( struct s_mod_info * pmod, const char *szLoadName );
bool_t module_read_summary( char *szLoadName, ModSummary_t * ms );

void ModInfo_clear_all_titleimages( MOD_INFO * mi_ary, size_t mi_count );

void release_map(struct sGraphics_Data * gfx);

bool_t translate_spawn_file( struct sGame * gs, const char * modname );