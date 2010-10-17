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

/// @file mad.h

#include "mad_defs.h"

#include "file_formats/id_md2.h"
#include "md2.h"

#include <SDL_opengl.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct Mix_Chunk;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The definition of the Egoboo model type
struct ego_mad
{
    EGO_PROFILE_STUFF;

    Uint16  frameliptowalkframe[4][16];        ///< For walk animations

    int     action_map[ACTION_COUNT];          ///< actual action = action_map[requested action]
    bool_t  action_valid[ACTION_COUNT];        ///< bfalse if not valid
    int     action_stt[ACTION_COUNT];          ///< First frame of anim
    int     action_end[ACTION_COUNT];          ///< One past last frame

    //---- per-object data ----

    // model data
    ego_MD2_Model * md2_ptr;                       ///< the pointer that will eventually be used
};

DECLARE_STACK_EXTERN( ego_mad, MadStack, MAX_MAD );

#define VALID_MAD_RANGE( IMAD ) ( ((IMAD) >= 0) && ((IMAD) < MAX_MAD) )
#define LOADED_MAD( IMAD )       ( VALID_MAD_RANGE( IMAD ) && MadStack.lst[IMAD].loaded )

void MadList_init();
void MadList_dtor();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void    init_all_mad();
void    release_all_mad();
bool_t  release_one_mad( const MAD_REF by_reference imad );
MAD_REF load_one_model_profile_vfs( const char* tmploadname, const MAD_REF by_reference object );

int    action_which( char cTmp );
void   load_action_names_vfs( const char* loadname );

void   mad_make_equally_lit( const MAD_REF by_reference imad );

int    mad_get_action( const MAD_REF by_reference imad, int action );
Uint32 mad_get_actionfx( const MAD_REF by_reference imad, int action );
int    randomize_action( int action, int slot );
