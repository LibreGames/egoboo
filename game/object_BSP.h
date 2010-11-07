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

#include "bsp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_chr;
struct mpd_BSP;
struct ego_bundle_prt;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// the BSP structure housing an object
struct ego_obj_BSP
{
    // the BSP of characters for character-character and character-particle interactions
    ego_BSP_tree     tree;

    static ego_obj_BSP root;
    static int         chr_count;      ///< the number of characters in the ego_obj_BSP::root structure
    static int         prt_count;      ///< the number of particles  in the ego_obj_BSP::root structure

    ego_obj_BSP() {};
    ~ego_obj_BSP() { dtor_this( this ); };

    static bool_t ctor_this( ego_obj_BSP * pbsp, mpd_BSP * pmesh_bsp );
    static bool_t dtor_this( ego_obj_BSP * pbsp );

    static bool_t alloc( ego_obj_BSP * pbsp, int depth );
    static bool_t dealloc( ego_obj_BSP * pbsp );

    static bool_t fill( ego_obj_BSP * pbsp );
    static bool_t empty( ego_obj_BSP * pbsp );

    //bool_t insert_leaf( ego_obj_BSP * pbsp, ego_BSP_leaf * pnode, int depth, int address_x[], int address_y[], int address_z[] );
    static bool_t insert_chr( ego_obj_BSP * pbsp, ego_chr * pchr );
    static bool_t insert_prt( ego_obj_BSP * pbsp, const ego_bundle_prt & bdl_prt );

    static int    collide( ego_obj_BSP * pbsp, ego_BSP_aabb & bbox, leaf_child_list_t & colst );
};

