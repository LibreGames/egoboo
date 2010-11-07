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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_mpd;
struct ego_camera;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// JF - Added so that the video mode might be determined outside of the graphics code
extern bool_t          meshnotexture;
extern TX_REF          meshlasttexture;             ///< Last texture used

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void animate_all_tiles( ego_mpd * pmesh );
void render_fan( ego_mpd * pmesh, Uint32 fan );
void render_hmap_fan( ego_mpd * pmesh, Uint32 fan );
void render_water_fan( ego_mpd * pmesh, Uint32 fan, Uint8 layer );

void animate_tiles( void );

void   do_grid_lighting( ego_mpd * pmesh, ego_camera * pcam );
