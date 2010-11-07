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

/// \file collision.h

#include "egoboo_math.h"

#include "hash.h"
#include "bbox.h"
#include "bsp.h"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

struct ego_chr;
struct ego_prt;

struct ego_obj_BSP;

//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------

struct collision_ref
{
    friend struct collision_node;

    //---- constructors
    collision_ref() { invalidate(); }

    collision_ref( const CHR_REF & ref ) { assign( ref ); }
    collision_ref( const ENC_REF & ref ) { assign( ref ); }
    collision_ref( const PRT_REF & ref ) { assign( ref ); }
    collision_ref( const Uint32  & ref ) { assign( ref ); }

    //---- accessors
    CHR_REF get_chr()  const { return ( type_chr  == _type ) ? CHR_REF( _ref )  : CHR_REF( MAX_CHR ); }
    ENC_REF get_enc()  const { return ( type_enc  == _type ) ? ENC_REF( _ref )  : ENC_REF( MAX_ENC ); }
    PRT_REF get_prt()  const { return ( type_prt  == _type ) ? PRT_REF( _ref )  : PRT_REF( MAX_PRT ); }
    Uint32  get_tile() const { return ( type_tile == _type ) ? Uint32( _ref )   : INVALID_TILE;     }

    //---- operators
    collision_ref & operator = ( const CHR_REF & ref )  { assign( ref ); return *this; }
    collision_ref & operator = ( const ENC_REF & ref )  { assign( ref ); return *this; }
    collision_ref & operator = ( const PRT_REF & ref )  { assign( ref ); return *this; }
    collision_ref & operator = ( const Uint32  & ref )  { assign( ref ); return *this; }

    //---- other functions
    void invalidate() { _assign_base( type_unknown, REF_T( -1 ) ); }

private:

    enum _ref_type { type_unknown = -1, type_chr = 0, type_enc, type_prt, type_tile };

    bool_t assign( const CHR_REF & ref ) { bool_t retval = bfalse; invalidate(); if ( ref.get_value() < MAX_CHR ) { _assign_base( type_chr, ref.get_value() ); retval = btrue; } return retval; }
    bool_t assign( const ENC_REF & ref ) { bool_t retval = bfalse; invalidate(); if ( ref.get_value() < MAX_ENC ) { _assign_base( type_enc, ref.get_value() ); retval = btrue; } return retval; }
    bool_t assign( const PRT_REF & ref ) { bool_t retval = bfalse; invalidate(); if ( ref.get_value() < MAX_PRT ) { _assign_base( type_prt, ref.get_value() ); retval = btrue; } return retval; }
    bool_t assign( const Uint32  & ref ) { bool_t retval = bfalse; invalidate(); if ( INVALID_TILE != ref ) { _assign_base( type_tile, ref ); retval = btrue; } return retval; }

    void _assign_base( _ref_type type, REF_T ref ) { _type = type; _ref = ref; }

    _ref_type _type;
    REF_T     _ref;
};

//--------------------------------------------------------------------------------------------
// struct collision_node
//--------------------------------------------------------------------------------------------

///
/// \author BB
/// \brief element for storing pair-wise "collision" data
///
/// \details this does not use the "standard" method of inheritance from ego_hash_node, where an
/// instance of ego_hash_node is embedded inside collision_node as collision_node::base or something.
/// Instead, a separate lists of free hash_nodes and free CoNodes are kept, and they are
/// associated through the ego_hash_node::data pointer when the hash node is added to the
/// ego_hash_list

struct collision_node
{
    // the "colliding" objects
    collision_ref src;

    // the "collided with" objects
    collision_ref dst;

    // some information about the estimated collision
    float      tmin, tmax;
    ego_oct_bb cv;

    collision_node() { ctor( this ); }

    static collision_node * ctor( collision_node * );
    static Uint8            generate_hash( collision_node * coll );

    //---- tests for various collision types
    bool_t is_chr_chr() { return ( collision_ref::type_chr == src._type ) && ( collision_ref::type_chr == dst._type ); };

    bool_t is_chr_prt() { return ( collision_ref::type_chr == src._type ) && ( collision_ref::type_prt == dst._type ); };
    bool_t is_prt_chr() { return ( collision_ref::type_prt == src._type ) && ( collision_ref::type_chr == dst._type ); };

    bool_t is_chr_tile() { return ( collision_ref::type_chr == src._type ) && ( collision_ref::type_tile == dst._type ); };
    bool_t is_tile_chr() { return ( collision_ref::type_tile == src._type ) && ( collision_ref::type_chr == dst._type ); };

    bool_t is_prt_tile() { return ( collision_ref::type_prt == src._type ) && ( collision_ref::type_tile == dst._type ); };
    bool_t is_tile_prt() { return ( collision_ref::type_tile == src._type ) && ( collision_ref::type_prt == dst._type ); };

    bool_t has_chr() { return ( collision_ref::type_chr == src._type ) || ( collision_ref::type_chr == dst._type ); };
    bool_t has_enc() { return ( collision_ref::type_enc == src._type ) || ( collision_ref::type_enc == dst._type ); };
    bool_t has_prt() { return ( collision_ref::type_prt == src._type ) || ( collision_ref::type_prt == dst._type ); };
    bool_t has_tile() { return ( collision_ref::type_tile == src._type ) || ( collision_ref::type_tile == dst._type ); };
};

/// a function suitable for sorting collision_node arrays using quicksort
int collision_node_cmp( const void * pleft, const void * pright );

/// a functional to allow std::sort() to order a list of collision_node data
struct collision_node_greater : public std::binary_function <collision_node, collision_node, bool>
{
    bool operator()(
        const collision_node& _Left,
        const collision_node& _Right
    ) const;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// dynamically allocated array of collision_nodes
typedef t_dary< collision_node > collision_node_ary;

// dynamically allocated array of ego_hash_nodes
typedef t_dary<ego_hash_node >  hash_node_ary;

//--------------------------------------------------------------------------------------------
// struct collision_hash_list
//--------------------------------------------------------------------------------------------

/// \author BB
///
/// \brief a specialization of ego_hash_list for collisions

struct collision_hash_list : public ego_hash_list
{
    friend struct collision_system;

    static collision_hash_list * create( int size, collision_hash_list * ptr = NULL );
    static bool_t                destroy( collision_hash_list ** ppchlst, bool_t own_ptr = btrue );

    static collision_hash_list * retor_all( collision_hash_list * ptr, int size = -1 )
    {
        ptr = dtor_all( ptr );
        ptr = ctor_all( ptr, size );

        return ptr;
    }

protected:

    static bool_t insert_unique( collision_hash_list *, collision_node * pdata, collision_node_ary & cdata, hash_node_ary & hnlst );

    // hide these here so that we must create/destroy them?
    explicit collision_hash_list( int size ) : ego_hash_list( size ), inserted( 0 ) {};
    ~collision_hash_list() {};

private:

    static collision_hash_list * ctor_all( collision_hash_list *ptr, int size = -1 ) { if ( NULL != ptr ) ptr = new( ptr ) collision_hash_list( size ); return ptr; }
    static collision_hash_list * dtor_all( collision_hash_list *ptr ) { if ( NULL != ptr ) ptr->~collision_hash_list(); return ptr; }

    int inserted;
};

//--------------------------------------------------------------------------------------------
// struct collision_system
//--------------------------------------------------------------------------------------------

/// \author BB
///
/// \brief an encapsulation of the collision system

struct collision_system
{
    static bool_t begin();
    static void   end();

    static void bump_all_objects( ego_obj_BSP * pbsp );

    static int hash_nodes_inserted;

protected:
    static bool_t _hash_initialized;
    static bool_t _initialized;

    static collision_hash_list   * _hash_ptr;
    static hash_node_ary           _hn_ary;                 ///< the available ego_hash_node collision nodes for the collision_hash_list
    static collision_node_ary      _co_ary;                 ///< the available collision_node    data pointed to by the ego_hash_node nodes
    static leaf_child_list_t       _coll_leaf_lst;
    static collision_node_ary      _coll_node_lst;

    static collision_hash_list * get_hash_ptr( int size );

    static bool_t fill_bumplists( ego_obj_BSP * pbsp );
    static bool_t fill_interaction_list( collision_hash_list * pchlst, collision_node_ary & cn_lst, hash_node_ary & hn_lst );

    static egoboo_rv bump_prepare( ego_obj_BSP * pbsp );
    static void      bump_begin();
    static bool_t    bump_all_platforms( collision_node_ary * pcn_ary );
    static bool_t    bump_all_mounts( collision_node_ary * pcn_ary );
    static bool_t    bump_all_collisions( collision_node_ary * pcn_ary );
    //static void      bump_end();
};

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTIONS
//--------------------------------------------------------------------------------------------

bool_t detach_character_from_platform( ego_chr * pchr );
bool_t detach_particle_from_platform( ego_prt * pprt );

bool_t calc_grip_cv( ego_chr * pmount, int grip_offset, ego_oct_bb   * grip_cv_ptr, fvec3_base_t grip_origin_ary, fvec3_base_t grip_up_ary );

void update_all_platform_attachments();

