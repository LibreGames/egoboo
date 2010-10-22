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

/// @file mesh_BSP.h
/// @brief
/// @details

#include <cmath>

#include "egoboo_typedef.h"

#include "bsp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_mpd;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the BSP structure housing the mesh
struct mpd_BSP
{
    ego_oct_bb       volume;
    ego_BSP_leaf_ary nodes;
    ego_BSP_tree     tree;

    mpd_BSP()                    { /* nothing */ }
    mpd_BSP( ego_mpd   * pmesh ) { ctor_this( this, pmesh ); }
    ~mpd_BSP()                   { dtor_this( this ); }

    static mpd_BSP root;

    static mpd_BSP * ctor_this( mpd_BSP * pbsp, ego_mpd * pmesh );
    static mpd_BSP * dtor_this( mpd_BSP * );
    static bool_t    alloc( mpd_BSP * pbsp, ego_mpd * pmesh );
    static bool_t    dealloc( mpd_BSP * pbsp );

    static bool_t    fill( mpd_BSP * pbsp );

    static int       collide( mpd_BSP * pbsp, ego_BSP_aabb * paabb, ego_BSP_leaf_pary * colst );
};

