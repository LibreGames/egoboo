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

/// @file collision.h

#include "egoboo_math.h"

#include "hash.h"
#include "bbox.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_obj_BSP;
struct ego_chr;
struct ego_prt_data;
struct ego_oct_bb  ;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// element for storing pair-wise "collision" data
/// @note this does not use the "standard" method of inheritance from ego_hash_node, where an
/// instance of ego_hash_node is embedded inside ego_CoNode as ego_CoNode::base or something.
/// Instead, a separate lists of free hash_nodes and free CoNodes are kept, and they are
/// associated through the ego_hash_node::data pointer when the hash node is added to the
/// ego_hash_list
struct ego_CoNode
{
    // the "colliding" objects
    CHR_REF chra;
    PRT_REF prta;

    // the "collided with" objects
    CHR_REF chrb;
    PRT_REF prtb;
    Uint32  tileb;

    // some information about the estimated collision
    float    tmin, tmax;
    ego_oct_bb   cv;
};

ego_CoNode * CoNode_ctor( ego_CoNode * );
Uint8      CoNode_generate_hash( ego_CoNode * coll );
int        CoNode_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
// a template-like definition of a dynamically allocated array of ego_CoNode elements
DECLARE_DYNAMIC_ARY( CoNode_ary, ego_CoNode );

//--------------------------------------------------------------------------------------------
// a template-like definition of a dynamically allocated array of ego_hash_node elements
DECLARE_DYNAMIC_ARY( HashNode_ary, ego_hash_node );

//--------------------------------------------------------------------------------------------

/// a useful re-typing of the CHashList_t, in case we need to add more variables or functionality later
typedef ego_hash_list CHashList_t;

CHashList_t * CHashList_ctor( CHashList_t * pchlst, int size );
CHashList_t * CHashList_dtor( CHashList_t * pchlst );
bool_t        CHashList_insert_unique( CHashList_t * pchlst, ego_CoNode * pdata, CoNode_ary * cdata, HashNode_ary * hnlst );

CHashList_t * CHashList_get_Instance( int size );

//--------------------------------------------------------------------------------------------
extern int CHashList_inserted;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// global functions

bool_t collision_system_begin();
void   collision_system_end();

void bump_all_objects( struct ego_obj_BSP * pbsp );

bool_t detach_character_from_platform( struct ego_chr * pchr );
bool_t detach_particle_from_platform( struct ego_prt_data * pprt );

bool_t calc_grip_cv( struct ego_chr * pmount, int grip_offset, struct ego_oct_bb   * grip_cv_ptr, fvec3_base_t grip_origin_ary, fvec3_base_t grip_up_ary );

void update_all_platform_attachments();