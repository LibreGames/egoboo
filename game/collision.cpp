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

/// \file collision.c
/// \brief The code that handles collisions between in-game objects
/// \details

#include "egoboo_typedef_cpp.inl"

#include "collision.h"

#include "mesh_BSP.h"
#include "object_BSP.h"

#include "log.h"
#include "hash.h"
#include "game.h"
#include "extensions/SDL_extensions.h"
#include "network.h"

#include "char.inl"
#include "particle.inl"
#include "enchant.inl"
#include "profile.inl"
#include "physics.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAKE_HASH(AA,BB)         CLIP_TO_08BITS( ((AA) * 0x0111 + 0x006E) + ((BB) * 0x0111 + 0x006E) )

#define CHR_MAX_COLLISIONS       512*16
#define COLLISION_HASH_NODES     (CHR_MAX_COLLISIONS*2)
#define COLLISION_LIST_SIZE      256

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// data block used to communicate between the different "modules" governing the character-particle collision
struct ego_chr_prt_collsion_data
{
    // object parameters
    ego_cap * pcap;
    ego_pip * ppip;

    // collision parameters
    ego_oct_vec odepth;
    float     dot;
    fvec3_t   nrm;

    // collision modifications
    bool_t   mana_paid;
    int      max_damage, actual_damage;
    fvec3_t  vdiff;

    // collision reaction
    fvec3_t impulse;
    bool_t  terminate_particle;
    bool_t  prt_bumps_chr;
    bool_t  prt_damages_chr;
};

//--------------------------------------------------------------------------------------------

/// one element of the data for partitioning character and particle positions
struct ego_bumplist
{
    size_t   chrnum;                  // Number on the block
    CHR_REF  chr;                     // For character collisions

    size_t   prtnum;                  // Number on the block
    CHR_REF  prt;                     // For particle collisions
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//static bool_t add_chr_chr_interaction( collision_hash_list * pclst, const CHR_REF & ichr_a, const CHR_REF & ichr_b, collision_node_ary * pcn_lst, hash_node_ary * phn_lst );
//static bool_t add_chr_prt_interaction( collision_hash_list * pclst, const CHR_REF & ichr_a, const PRT_REF & iprt_b, collision_node_ary * pcn_lst, hash_node_ary * phn_lst );

static bool_t detect_chr_chr_interaction_valid( const CHR_REF & ichr_a, const CHR_REF & ichr_b );
static bool_t detect_chr_prt_interaction_valid( const CHR_REF & ichr_a, const PRT_REF & iprt_b );

//static bool_t detect_chr_chr_interaction( const CHR_REF & ichr_a, const CHR_REF & ichr_b );
//static bool_t detect_chr_prt_interaction( const CHR_REF & ichr_a, const PRT_REF & iprt_b );

static bool_t do_chr_platform_detection( const CHR_REF & ichr_a, const CHR_REF & ichr_b );
static bool_t do_prt_platform_detection( const PRT_REF & iprt_a, const CHR_REF & ichr_b );

static bool_t attach_chr_to_platform( ego_chr * pchr, ego_chr * pplat );
static bool_t attach_prt_to_platform( ego_prt * pprt, ego_chr * pplat );

static float  estimate_chr_prt_normal( ego_chr * pchr, ego_prt * pprt, fvec3_base_t nrm, fvec3_base_t vdiff );

static bool_t do_chr_mount_detection( const CHR_REF & ichr_a, const CHR_REF & ichr_b );
//static bool_t do_mounts( const CHR_REF & ichr_a, const CHR_REF & ichr_b );
static egoboo_rv do_chr_platform_physics( collision_node * d, ego_bundle_chr & bdl_item, ego_bundle_chr & bdl_plat );

static bool_t do_chr_chr_collision( collision_node * d );
static bool_t do_chr_chr_collision_interaction( collision_node * d, ego_bundle_chr & bdl_a, ego_bundle_chr & bdl_b );

static bool_t do_chr_prt_collision_deflect( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata );
static bool_t do_chr_prt_collision_recoil( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata );
static bool_t do_chr_prt_collision_damage( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata );
static bool_t do_chr_prt_collision_bump( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata );
static bool_t do_chr_prt_collision_handle_bump( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata );
static bool_t do_chr_prt_collision_init( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata );
static bool_t do_chr_prt_collision( collision_node * d );

static bool_t update_chr_platform_attachment( ego_chr * pchr );
static ego_bundle_prt & update_prt_platform_attachment( ego_bundle_prt & bdl );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int    collision_system::hash_nodes_inserted = 0;
bool_t collision_system::_hash_initialized = bfalse;
bool_t collision_system::_initialized = bfalse;

collision_hash_list   * collision_system::_hash_ptr = NULL;
hash_node_ary           collision_system::_hn_ary;                 ///< the available ego_hash_node collision nodes for the collision_hash_list
collision_node_ary      collision_system::_co_ary;                 ///< the available collision_node    data pointed to by the ego_hash_node nodes
leaf_child_list_t       collision_system::_coll_leaf_lst;
collision_node_ary      collision_system::_coll_node_lst;

//--------------------------------------------------------------------------------------------
// collision system
//--------------------------------------------------------------------------------------------
bool_t collision_system::begin()
{
    if ( !_initialized )
    {
        if ( !collision_node_ary::alloc( &_co_ary, CHR_MAX_COLLISIONS ) ) goto collision_system__begin_fail;

        if ( !hash_node_ary::alloc( &_hn_ary, COLLISION_HASH_NODES ) ) goto collision_system__begin_fail;

        if ( !collision_node_ary::alloc( &_coll_node_lst, COLLISION_LIST_SIZE ) ) goto collision_system__begin_fail;

        // delete the object BSP data
        ego_obj_BSP::dtor_this( &ego_obj_BSP::root );

        _initialized = btrue;
    }

    return btrue;

collision_system__begin_fail:

    collision_node_ary::dealloc( &_co_ary );
    hash_node_ary::dealloc( &_hn_ary );
    collision_node_ary::dealloc( &_coll_node_lst );

    if ( !_coll_leaf_lst.empty() ) _coll_leaf_lst.clear();

    _initialized = bfalse;

    log_error( "Cannot initialize the collision system" );

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void collision_system::end()
{
    if ( _hash_initialized )
    {
        collision_hash_list::destroy( &_hash_ptr );
        _hash_initialized = bfalse;

        // delete the object BSP data
        ego_obj_BSP::dtor_this( &ego_obj_BSP::root );

        // delete the object BSP data
        mpd_BSP::dtor_this( &mpd_BSP::root );
    }
    _hash_ptr = NULL;

    if ( _initialized )
    {
        collision_node_ary::dealloc( &_co_ary );
        hash_node_ary::dealloc( &_hn_ary );
        collision_node_ary::dealloc( &_coll_node_lst );

        if ( !_coll_leaf_lst.empty() ) _coll_leaf_lst.clear();

        _initialized = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
collision_hash_list * collision_system::get_hash_ptr( int size )
{
    /// \author BB
    /// \details  allows access to a "private" CHashList singleton object. This will automatically
    ///               initialze the _Colist_singleton and (almost) prevent anything from messing up
    ///               the initialization.

    // make sure that the collsion system was started
    begin();

    // if the _hash_ptr doesn't exist, create it (and initialize it)
    if ( NULL == _hash_ptr )
    {
        _hash_ptr         = collision_hash_list::create( size );
        _hash_initialized = ( NULL != _hash_ptr );
    }

    // it the pointer exists, but it (somehow) not initialized, do the initialization
    if ( NULL != _hash_ptr && !_hash_initialized )
    {
        _hash_ptr         = collision_hash_list::retor_all( _hash_ptr, size );
        _hash_initialized = ( NULL != _hash_ptr );
    }

    return _hash_initialized ? _hash_ptr : NULL;
}

//--------------------------------------------------------------------------------------------
bool_t collision_system::fill_interaction_list( collision_hash_list * pchlst, collision_node_ary & cn_lst, hash_node_ary & hn_lst )
{
    ego_mpd_info * mi;
    ego_BSP_aabb         tmp_aabb;

    if ( NULL == pchlst ) return bfalse;

    mi = &( PMesh->info );

    // allocate a ego_BSP_aabb   once, to be shared for all collision tests
    ego_BSP_aabb::init( &tmp_aabb, ego_obj_BSP::root.tree.dimensions );

    // renew the collision_node hash table.
    ego_hash_list::renew( pchlst );

    //---- find the character/particle interactions

    // Find the character-character interactions.
    // Loop only through the list of objects that are both active and in the BSP tree
    hash_nodes_inserted = 0;
    CHR_BEGIN_LOOP_BSP( ichr_a, pchr_a )
    {
        ego_oct_bb     tmp_oct;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_chr_bb( pchr_a, 0.0f, 1.0f, tmp_oct );

        // convert the ego_oct_bb to a correct ego_BSP_aabb
        ego_BSP_aabb::from_oct_bb( tmp_aabb, tmp_oct );

        // find all collisions with other characters and particles
        if ( !_coll_leaf_lst.empty() ) _coll_leaf_lst.clear();
        ego_obj_BSP::collide( &( ego_obj_BSP::root ), tmp_aabb, _coll_leaf_lst );

        // transfer valid _coll_leaf_lst entries to pchlst entries
        // and sort them by their initial times
        if ( _coll_leaf_lst.size() > 0 )
        {
            for ( leaf_child_list_t::iterator it = _coll_leaf_lst.begin(); it != _coll_leaf_lst.end(); it++ )
            {
                size_t      coll_ref;
                collision_node    tmp_codata;
                bool_t      do_insert;
                BIT_FIELD   test_platform;

                if ( NULL == ( *it ) ) continue;

                // assume the worst
                do_insert = bfalse;

                coll_ref = ( *it )->index;
                if ( LEAF_CHR == ( *it )->data_type )
                {
                    // collided with a character
                    CHR_REF ichr_b = CHR_REF( coll_ref );

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_chr_interaction_valid( ichr_a, ichr_b ) )
                    {
                        bool_t found = bfalse;

                        ego_chr * pchr_b = ChrObjList.get_data_ptr( ichr_b );

                        collision_node::ctor( &tmp_codata );

                        // platform physics overrides normal interactions, so
                        // do the platform test first
                        //
                        // In the case that two platforms are interacting, the platform detection routine
                        // should have already determined which platform is "on top of" the other. So,
                        // only one of the two following routines will actually be called

                        if ( !found && pchr_a->platform && pchr_b->canuseplatforms )
                        {
                            found = phys_intersect_oct_bb( pchr_a->chr_max_cv, pchr_a->pos, pchr_a->vel, pchr_b->chr_min_cv, pchr_b->pos, pchr_b->vel, PHYS_CLOSE_TOLERANCE_OBJ1, NULL, &( tmp_codata.tmin ), &( tmp_codata.tmax ) );
                        }

                        if ( !found && pchr_b->platform && pchr_a->canuseplatforms )
                        {
                            found = phys_intersect_oct_bb( pchr_a->chr_min_cv, pchr_a->pos, pchr_a->vel, pchr_b->chr_max_cv, pchr_b->pos, pchr_b->vel, PHYS_CLOSE_TOLERANCE_OBJ2, NULL, &( tmp_codata.tmin ), &( tmp_codata.tmax ) );
                        }

                        // try the normal test if the platform test fails
                        if ( !found )
                        {
                            found = phys_intersect_oct_bb( pchr_a->chr_min_cv, pchr_a->pos, pchr_a->vel, pchr_b->chr_min_cv, pchr_b->pos, pchr_b->vel, PHYS_CLOSE_TOLERANCE_NONE, &( tmp_codata.cv ), &( tmp_codata.tmin ), &( tmp_codata.tmax ) );
                        }

                        // detect a when the possible collision occurred
                        if ( found )
                        {
                            tmp_codata.src = ichr_a;
                            tmp_codata.dst = ichr_b;

                            do_insert = btrue;
                        }
                    }
                }
                else if ( LEAF_PRT == ( *it )->data_type )
                {
                    // collided with a particle
                    PRT_REF iprt_b = PRT_REF( coll_ref );

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_prt_interaction_valid( ichr_a, iprt_b ) )
                    {
                        bool_t found = bfalse;

                        ego_prt * pprt_b = PrtObjList.get_data_ptr( iprt_b );

                        collision_node::ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the ego_cap for these objects here
                        test_platform = PHYS_CLOSE_TOLERANCE_NONE;
                        test_platform |= pchr_a->platform ? PHYS_CLOSE_TOLERANCE_OBJ1 : 0;

                        // detect a when the possible collision occurred

                        // clear the collision volume
                        tmp_codata.cv.invalidate();

                        // platform physics overrides normal interactions, so
                        // do the platform test first
                        if ( PHYS_CLOSE_TOLERANCE_NONE != test_platform && !found )
                        {
                            found = phys_intersect_oct_bb( pchr_a->chr_max_cv, pchr_a->pos, pchr_a->vel, pprt_b->prt_max_cv, ego_prt::get_pos( pprt_b ), pprt_b->vel, test_platform, NULL, &( tmp_codata.tmin ), &( tmp_codata.tmax ) );
                        }

                        // try the normal test if the platform test fails
                        if ( !found )
                        {
                            found = phys_intersect_oct_bb( pchr_a->chr_min_cv, pchr_a->pos, pchr_a->vel, pprt_b->prt_max_cv, ego_prt::get_pos( pprt_b ), pprt_b->vel, test_platform, &( tmp_codata.cv ), &( tmp_codata.tmin ), &( tmp_codata.tmax ) );
                        }

                        if ( found )
                        {
                            tmp_codata.src = ichr_a;
                            tmp_codata.dst = iprt_b;

                            do_insert = btrue;
                        }
                    }
                }

                if ( do_insert )
                {
                    if ( collision_hash_list::insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst ) )
                    {
                        hash_nodes_inserted++;
                    }
                }
            }
        }
    }
    CHR_END_LOOP();

    //---- find the character and particle interactions with the mesh (not implemented)

    //// search through all characters. Use the ChrObjList.used_ref, for a change
    //_coll_leaf_lst.top = 0;
    //for ( i = 0; i < ChrObjList.used_count; i++ )
    //{
    //    CHR_REF ichra = ChrObjList.used_ref[i];
    //    if ( !INGAME_CHR( ichra ) ) continue;

    //    // find all character collisions with mesh tiles
    //    mpd_BSP::collide( &mpd_BSP::root, &( ChrObjList.get_data_ref(ichra).prt_cv ), &_coll_leaf_lst );
    //    if ( _coll_leaf_lst.top > 0 )
    //    {
    //        int j;

    //        for ( j = 0; j < _coll_leaf_lst.top; j++ )
    //        {
    //            int coll_ref;
    //            collision_node tmp_codata;

    //            coll_ref = _coll_leaf_lst[j];

    //            collision_node::ctor( &tmp_codata );
    //            tmp_codata.src.get_chr()  = ichra;
    //            tmp_codata.tileb = coll_ref;

    //            collision_hash_list::insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst );
    //        }
    //    }
    //}

    //// search through all particles. Use the PrtObjList.used_ref, for a change
    //_coll_leaf_lst.top = 0;
    //for ( i = 0; i < PrtObjList.used_count; i++ )
    //{
    //    PRT_REF iprta = PrtObjList.used_ref[i];
    //    if ( !INGAME_PRT( iprta ) ) continue;

    //    // find all particle collisions with mesh tiles
    //    mpd_BSP::collide( &mpd_BSP::root, &( PrtObjList.get_data_ref(iprta).prt_cv ), &_coll_leaf_lst );
    //    if ( _coll_leaf_lst.top > 0 )
    //    {
    //        int j;

    //        for ( j = 0; j < _coll_leaf_lst.top; j++ )
    //        {
    //            int coll_ref;
    //            collision_node tmp_codata;

    //            coll_ref = _coll_leaf_lst[j];

    //            collision_node::ctor( &tmp_codata );
    //            tmp_codata.src.get_prt()  = iprta;
    //            tmp_codata.tileb = coll_ref;

    //            collision_hash_list::insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst );
    //        }
    //    }
    //}

    // do this manually in C
    //ego_BSP_aabb::deinit( &tmp_aabb );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t collision_system::fill_bumplists( ego_obj_BSP * pbsp )
{
    /// \author BB
    /// \details  Fill in the ego_obj_BSP for this frame
    ///
    /// \note do not use ego_obj_BSP::empty every frame, because the number of pre-allocated nodes can be quite large.
    /// Instead, just remove the nodes from the tree, fill the tree, and then prune any empty leaves

    if ( NULL == pbsp ) return bfalse;

    // Remove any unused branches from the tree.
    // If you do this after ego_BSP_tree::dealloc_nodes() it will remove all branches
    if ( 7 == ( frame_all & 7 ) )
    {
        ego_BSP_tree::prune( &( pbsp->tree ) );
    }

    // empty out the BSP node lists
    ego_obj_BSP::empty( pbsp );

    // fill up the BSP list based on the current locations
    ego_obj_BSP::fill( pbsp );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void collision_system::bump_all_objects( ego_obj_BSP * pbsp )
{
    /// \author ZZ
    /// \details  This function handles characters hitting other characters or particles

    // prepare the collision node list for tracking object interactions
    if ( rv_success == bump_prepare( pbsp ) )
    {
        // set up all object(s) bumping
        bump_begin();

        // handle interaction with mounts
        bump_all_mounts( &_coll_node_lst );

        // handle interaction with platforms
        bump_all_platforms( &_coll_node_lst );

        // handle all the collisions
        bump_all_collisions( &_coll_node_lst );
    }

    // The following functions need to be called any time you actually change a charcter's position
    keep_weapons_with_holders();
    attach_all_particles();
    make_all_character_matrices( update_wld != 0 );
    update_all_platform_attachments();
}

//--------------------------------------------------------------------------------------------
bool_t collision_system::bump_all_platforms( collision_node_ary * pcn_ary )
{
    /// \author BB
    /// \details  Detect all character and particle interactions with platforms, then attach them.
    ///
    /// \note it is important to only attach the character to a platform once, so its
    ///  weight does not get applied to multiple platforms

    size_t     cnt;
    collision_node * d;

    if ( NULL == pcn_ary ) return bfalse;

    //---- Detect all platform attachments
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = ( *pcn_ary ) + cnt;

        if ( d->is_chr_chr() )
        {
            do_chr_platform_detection( d->src.get_chr(), d->dst.get_chr() );
        }
        else if ( d->is_chr_prt() )
        {
            do_prt_platform_detection( d->dst.get_prt(), d->src.get_chr() );
        }
        else if ( d->is_prt_chr() )
        {
            do_prt_platform_detection( d->src.get_prt(), d->dst.get_chr() );
        }
    }

    //---- Do the actual platform attachments.

    // Doing the attachments after detecting the best platform
    // prevents an object from attaching it to multiple platforms as it
    // is still trying to find the best one
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = ( *pcn_ary ) + cnt;

        // only look at character-character interactions
        //if ( MAX_PRT != d->src.get_prt() && MAX_PRT != d->dst.get_prt() ) continue;

        if ( d->is_chr_chr() )
        {
            if ( INGAME_CHR( d->src.get_chr() ) && INGAME_CHR( d->dst.get_chr() ) )
            {
                if ( ChrObjList.get_data_ref( d->src.get_chr() ).targetplatform_overlap > 0.0f && ChrObjList.get_data_ref( d->src.get_chr() ).targetplatform_ref == d->dst.get_chr() )
                {
                    attach_chr_to_platform( ChrObjList.get_data_ptr( d->src.get_chr() ), ChrObjList.get_data_ptr( d->dst.get_chr() ) );
                }
                else if ( ChrObjList.get_data_ref( d->dst.get_chr() ).targetplatform_overlap > 0.0f && ChrObjList.get_data_ref( d->dst.get_chr() ).targetplatform_ref == d->src.get_chr() )
                {
                    attach_chr_to_platform( ChrObjList.get_data_ptr( d->dst.get_chr() ), ChrObjList.get_data_ptr( d->src.get_chr() ) );
                }

            }
        }
        else if ( d->is_chr_prt() )
        {
            if ( INGAME_CHR( d->src.get_chr() ) && INGAME_PRT( d->dst.get_prt() ) )
            {
                if ( PrtObjList.get_data_ref( d->dst.get_prt() ).targetplatform_overlap > 0.0f && PrtObjList.get_data_ref( d->dst.get_prt() ).targetplatform_ref == d->src.get_chr() )
                {
                    attach_prt_to_platform( PrtObjList.get_data_ptr( d->dst.get_prt() ), ChrObjList.get_data_ptr( d->src.get_chr() ) );
                }
            }
        }
        else if ( d->is_prt_chr() )
        {
            if ( INGAME_CHR( d->dst.get_chr() ) && INGAME_PRT( d->src.get_prt() ) )
            {
                if ( PrtObjList.get_data_ref( d->src.get_prt() ).targetplatform_overlap > 0.0f &&  PrtObjList.get_data_ref( d->src.get_prt() ).targetplatform_ref == d->dst.get_chr() )
                {
                    attach_prt_to_platform( PrtObjList.get_data_ptr( d->src.get_prt() ), ChrObjList.get_data_ptr( d->dst.get_chr() ) );
                }
            }
        }
    }

    //---- remove any bad platforms

    CHR_BEGIN_LOOP_PROCESSING( ichr, pchr )
    {
        if ( MAX_CHR != pchr->onwhichplatform_ref && pchr->onwhichplatform_update < update_wld )
        {
            detach_character_from_platform( pchr );
        }
    }
    CHR_END_LOOP();

    PRT_BEGIN_LOOP_ALLOCATED_BDL( iprt, bdl_prt )
    {
        if ( MAX_CHR != bdl_prt.prt_ptr()->onwhichplatform_ref && bdl_prt.prt_ptr()->onwhichplatform_update < update_wld )
        {
            detach_particle_from_platform( bdl_prt.prt_ptr() );
        }
    }
    PRT_END_LOOP();

    return btrue;
}

//--------------------------------------------------------------------------------------------
egoboo_rv collision_system::bump_prepare( ego_obj_BSP * pbsp )
{
    collision_hash_list * pchlst;
    size_t        co_node_count = 0;

    // create a collision hash table that can keep track of 512
    // binary collisions per frame
    pchlst = get_hash_ptr( -1 );
    if ( NULL == pchlst )
    {
        log_error( "%s - cannot access the collision_hash_list singleton", __FUNCTION__ );
        return rv_error;
    }

    // set up the collision node array
    _co_ary.top = _co_ary.size();

    // set up the hash node array
    _hn_ary.top = _hn_ary.size();

    // fill up the BSP structures
    fill_bumplists( pbsp );

    // use the BSP structures to detect possible binary interactions
    fill_interaction_list( pchlst, _co_ary, _hn_ary );

    //// convert the collision_hash_list into a collision_node_ary and sort
    //co_node_count = ego_hash_list::count_nodes( pchlst );

    //if ( co_node_count > 0 )
    //{
    //    ego_hash_list::iterator it;

    //    _coll_node_lst.top = 0;

    //    ego_hash_list::iterator::ctor_this( &it );
    //    ego_hash_list::iterator::set_begin( &it, pchlst );
    //    for ( /* nothing */; !ego_hash_list::iterator::done( &it, pchlst ); ego_hash_list::iterator::next( &it, pchlst ) )
    //    {
    //        collision_node * ptr = ( collision_node * )ego_hash_list::iterator::ptr( &it );
    //        if ( NULL == ptr ) break;

    //        collision_node_ary::push_back( &_coll_node_lst, *ptr );
    //    }

    //    if ( _coll_node_lst.top > 1 )
    //    {
    //        // arrange the actual nodes by time order
    //        std::sort( _coll_node_lst.begin(), _coll_node_lst.end(), collision_node_greater() );
    //    }
    //}

    return ( co_node_count > 0 ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
void collision_system::bump_begin()
{
    // blank the accumulators
    //CHR_BEGIN_LOOP_PROCESSING( cnt, pchr )
    //{
    //    phys_data_blank_accumulators( &(pchr->phys) );
    //}
    //CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
bool_t collision_system::bump_all_mounts( collision_node_ary * pcn_ary )
{
    /// \author BB
    /// \details  Detect all character interactions with mounts, then attach them.

    size_t     cnt;
    collision_node * d;

    if ( NULL == pcn_ary ) return bfalse;

    //---- Detect all mount attachments
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = ( *pcn_ary ) + cnt;

        if ( d->is_chr_chr() )
        {
            do_chr_mount_detection( d->src.get_chr(), d->dst.get_chr() );
        }
    }

    //---- Do the actual mount attachments.

    // Doing the attachments after detecting the best mount
    // prevents an object from attaching it to multiple mounts as it
    // is still trying to find the best one
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = ( *pcn_ary ) + cnt;

        // only look at character-character interactions
        if ( INGAME_CHR( d->src.get_chr() ) && INGAME_CHR( d->dst.get_chr() ) )
        {
            if ( ChrObjList.get_data_ref( d->src.get_chr() ).targetmount_overlap > 0.0f && ChrObjList.get_data_ref( d->src.get_chr() ).targetmount_ref == d->dst.get_chr() )
            {
                attach_character_to_mount( d->src.get_chr(), d->dst.get_chr(), GRIP_ONLY );
            }
            else if ( ChrObjList.get_data_ref( d->dst.get_chr() ).targetmount_overlap > 0.0f && ChrObjList.get_data_ref( d->dst.get_chr() ).targetmount_ref == d->src.get_chr() )
            {
                attach_character_to_mount( d->dst.get_chr(), d->src.get_chr(), GRIP_ONLY );
            }
        }
    }

    return btrue;
}

////--------------------------------------------------------------------------------------------
//bool_t collision_system::bump_all_mounts( collision_node_ary * pcn_ary )
//{
//    /// \author BB
//    /// \details  Detect all character interactions with mounts, then attach them.
//
//    int        cnt;
//    collision_node * d;
//
//    if ( NULL == pcn_ary ) return bfalse;
//
//    // Do mounts
//    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
//    {
//        d = (*pcn_ary) + cnt;
//
//        // only look at character-character interactions
//        if ( d->is_chr_chr() ) continue;
//
//        do_mounts( d->src.get_chr(), d->dst.get_chr() );
//    }
//
//    return btrue;
//}

//--------------------------------------------------------------------------------------------
bool_t collision_system::bump_all_collisions( collision_node_ary * pcn_ary )
{
    /// \author BB
    /// \details  Detect all character-character and character-particle collisions (with exclusions
    ///               for the mounts and platforms found in the previous steps)

    size_t cnt;

    // do all interactions
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        bool_t handled = bfalse;

        // use this form of the function call so that we could add more modules or
        // rearrange them without needing to change anything
        if ( !handled )
        {
            handled = do_chr_chr_collision(( *pcn_ary ) + cnt );
        }

        if ( !handled )
        {
            handled = do_chr_prt_collision(( *pcn_ary ) + cnt );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
collision_node * collision_node::ctor( collision_node * n )
{
    if ( NULL == n ) return n;

    // the "colliding" objects
    n->src.invalidate();

    // the "collided with" objects
    n->dst.invalidate();

    // initialize the time
    n->tmin = n->tmax = -1.0f;

    return n;
}

//--------------------------------------------------------------------------------------------
Uint8 collision_node::generate_hash( collision_node * coll )
{
    REF_T AA, BB;

    AA = ( REF_T )( Uint32( ~0 ) );
    if ( INGAME_CHR( coll->src.get_chr() ) )
    {
        AA = ( coll->src.get_chr() ).get_value();
    }
    else if ( INGAME_PRT( coll->src.get_prt() ) )
    {
        AA = ( coll->src.get_prt() ).get_value();
    }

    BB = ( REF_T )( Uint32( ~0 ) );
    if ( INGAME_CHR( coll->dst.get_chr() ) )
    {
        BB = ( coll->dst.get_chr() ).get_value();
    }
    else if ( INGAME_PRT( coll->dst.get_prt() ) )
    {
        BB = ( coll->dst.get_prt() ).get_value();
    }
    else if ( FANOFF != coll->dst.get_tile() )
    {
        BB = coll->dst.get_tile();
    }

    return MAKE_HASH( AA, BB );
}

//--------------------------------------------------------------------------------------------
int collision_node_cmp( const void * vleft, const void * vright )
{
    int   itmp;
    float ftmp;

    collision_node * pleft  = ( collision_node * )vleft;
    collision_node * pright = ( collision_node * )vright;

    // sort by initial time first
    ftmp = pleft->tmin - pright->tmin;
    if ( ftmp <= 0.0f ) return -1;
    else if ( ftmp >= 0.0f ) return 1;

    // fort by final time second
    ftmp = pleft->tmax - pright->tmax;
    if ( ftmp <= 0.0f ) return -1;
    else if ( ftmp >= 0.0f ) return 1;

    itmp = ego_sint( pleft->src.get_chr().get_value() ) - ego_sint( pright->src.get_chr().get_value() );
    if ( 0 != itmp ) return itmp;

    itmp = ego_sint( pleft->src.get_prt().get_value() ) - ego_sint( pright->src.get_prt().get_value() );
    if ( 0 != itmp ) return itmp;

    itmp = ego_sint( pleft->src.get_chr().get_value() ) - ego_sint( pright->src.get_chr().get_value() );
    if ( 0 != itmp ) return itmp;

    itmp = ego_sint( pleft->dst.get_prt().get_value() ) - ego_sint( pright->dst.get_prt().get_value() );
    if ( 0 != itmp ) return itmp;

    itmp = ego_sint( pleft->dst.get_chr().get_value() ) - ego_sint( pright->dst.get_chr().get_value() );
    if ( 0 != itmp ) return itmp;

    itmp = ego_sint( pleft->dst.get_tile() ) - ego_sint( pright->dst.get_tile() );
    if ( 0 != itmp ) return itmp;

    return 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
collision_hash_list * collision_hash_list::create( int size, collision_hash_list * ptr )
{
    if ( NULL == ptr )
    {
        ptr = new collision_hash_list( size );
    }
    else
    {
        ptr = new( ptr ) collision_hash_list( size );
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool_t collision_hash_list::destroy( collision_hash_list ** ppchlst, bool_t own_ptr )
{
    if ( NULL == ppchlst || NULL == *ppchlst ) return bfalse;

    if ( own_ptr )
    {
        delete *ppchlst;
        *ppchlst = NULL;
    }
    else
    {
        ( *ppchlst )->~collision_hash_list();
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t collision_hash_list::insert_unique( collision_hash_list * pchlst, collision_node * pdata, collision_node_ary & free_cdata, hash_node_ary & free_hnodes )
{
    Uint32 hashval = 0;
    collision_node * d;

    ego_hash_node * hn;
    bool_t found;
    size_t count;

    if ( NULL == pchlst || NULL == pdata ) return bfalse;

    // find the hash value for this interaction
    hashval = collision_node::generate_hash( pdata );

    found = bfalse;
    count = ego_hash_list::get_count( pchlst, hashval );
    if ( count > 0 )
    {
        size_t k;

        // this hash already exists. check to see if the binary collision exists, too
        hn = ego_hash_list::get_node( pchlst, hashval );
        for ( k = 0; k < count; k++ )
        {
            if ( 0 == collision_node_cmp( hn->data, pdata ) )
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if ( !found )
    {
        size_t old_count;
        ego_hash_node * old_head, * new_head, * tmp_hn;

        // pick a free collision data
        d = collision_node_ary::pop_back( &free_cdata );

        // fill it in
        *d = *pdata;

        // generate a new hash node
        tmp_hn = hash_node_ary::pop_back( &free_hnodes );

        // link the hash node to the free CoNode
        tmp_hn->data = d;

        // insert the node at the front of the collision list for this hash
        old_head = ego_hash_list::get_node( pchlst, hashval );
        new_head = ego_hash_node::insert_before( old_head, tmp_hn );
        ego_hash_list::set_node( pchlst, hashval, new_head );

        // add 1 to the count at this hash
        old_count = ego_hash_list::get_count( pchlst, hashval );
        ego_hash_list::set_count( pchlst, hashval, old_count + 1 );
    }

    return !found;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t detect_chr_chr_interaction_valid( const CHR_REF & ichr_a, const CHR_REF & ichr_b )
{
    ego_chr *pchr_a, *pchr_b;

    // Don't interact with self
    if ( ichr_a == ichr_b ) return bfalse;

    // Ignore invalid characters
    pchr_a = ChrObjList.get_allocated_data_ptr( ichr_a );
    if ( !INGAME_PCHR( pchr_a ) ) return bfalse;

    // Ignore invalid characters
    pchr_b = ChrObjList.get_allocated_data_ptr( ichr_b );
    if ( !INGAME_PCHR( pchr_b ) ) return bfalse;

    // don't interact if there is no interaction
    if ( 0.0f == pchr_a->bump_stt.size || 0.0f == pchr_b->bump_stt.size ) return bfalse;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pchr_b->is_hidden ) return bfalse;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detect_chr_prt_interaction_valid( const CHR_REF & ichr_a, const PRT_REF & iprt_b )
{
    ego_chr * pchr_a;
    ego_prt * pprt_b;

    // Ignore invalid characters
    pchr_a = ChrObjList.get_allocated_data_ptr( ichr_a );
    if ( !INGAME_PCHR( pchr_a ) ) return bfalse;

    // Ignore invalid characters
    if ( !INGAME_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtObjList.get_data_ptr( iprt_b );

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pprt_b->is_hidden ) return bfalse;

    // particles don't "collide" with anything they are attached to.
    // that only happens through doing bump particle damage
    if ( ichr_a == pprt_b->attachedto_ref ) return bfalse;

    // don't interact if there is no interaction...
    // the particles and characters should not have been added to the list unless they
    // are valid for collision

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_mount_detection( const CHR_REF & ichr_a, const CHR_REF & ichr_b )
{
    size_t cnt;

    fvec3_t grip_up, grip_origin, vdiff, pdiff;

    ego_chr * pchr_a, * pchr_b;
    ego_chr * pmount, * prider;

    bool_t mount_updated = bfalse;
    bool_t mount_a, mount_b;

    float   vnrm;

    ego_oct_bb    grip_cv, rider_cv;

    ego_oct_vec odepth;
    float z_overlap;

    // how well does the position of the character's feet match the mount?
    float hrz_overlap1, hrz_overlap2, hrz_overlap, overlap;

    // make sure that A is valid
    pchr_a = ChrObjList.get_allocated_data_ptr( ichr_a );
    if ( !INGAME_PCHR( pchr_a ) ) return bfalse;

    // make sure that B is valid
    pchr_b = ChrObjList.get_allocated_data_ptr( ichr_b );
    if ( !INGAME_PCHR( pchr_b ) ) return bfalse;

    // only check possible rider-mount interactions
    mount_a = chr_can_mount( ichr_b, ichr_a ) && !INGAME_CHR( pchr_b->attachedto );
    mount_b = chr_can_mount( ichr_a, ichr_b ) && !INGAME_CHR( pchr_a->attachedto );
    if ( !mount_a && !mount_b ) return bfalse;

    // if a rider is already mounted, don't do anything
    if ( mount_a ) mount_a = !INGAME_CHR( pchr_b->attachedto );
    if ( mount_b ) mount_b = !INGAME_CHR( pchr_a->attachedto );
    if ( !mount_a && !mount_b ) return bfalse;

    // determine whether any collision is possible
    z_overlap  = SDL_min( pchr_b->chr_min_cv.maxs[OCT_Z] + pchr_b->pos.z, pchr_a->chr_min_cv.maxs[OCT_Z] + pchr_a->pos.z ) -
                 SDL_max( pchr_b->chr_min_cv.mins[OCT_Z] + pchr_b->pos.z, pchr_a->chr_min_cv.mins[OCT_Z] + pchr_a->pos.z );

    if ( z_overlap <= 0.0f ) return bfalse;

    // if both riders are mounts determine how to arrange them
    if ( mount_a && mount_b )
    {
        bool_t chara_on_top;
        float depth_a, depth_b;
        float depth_max;

        depth_a = ( pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z] ) - pchr_a->pos.z;
        depth_b = ( pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] ) - pchr_b->pos.z;

        depth_max = SDL_min( pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z] + MOUNTTOLERANCE, pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] + MOUNTTOLERANCE ) -
                    SDL_max( pchr_b->pos.z, pchr_a->pos.z );

        chara_on_top = SDL_abs( depth_max - depth_a ) < SDL_abs( depth_max - depth_b );

        if ( chara_on_top )
        {
            mount_a = bfalse;
        }
        else
        {
            mount_b = bfalse;
        }
    }

    // determine who is the rider and who is the mount
    pmount = prider = NULL;
    if ( mount_a )
    {
        pmount = pchr_a;
        prider = pchr_b;
    }
    else if ( mount_b )
    {
        pmount = pchr_b;
        prider = pchr_a;
    }
    if ( NULL == pmount || NULL == prider ) return bfalse;

    // determine the grip properties
    calc_grip_cv( pmount, GRIP_LEFT, &grip_cv, grip_origin.v, grip_up.v );

    // determine if the rider is falling on the grip
    // (maybe not "falling", but just approaching?)

    // standing on the ground?
    if ( prider->enviro.grounded ) return bfalse;

    // falling?
    vdiff = fvec3_sub( prider->vel.v, pmount->vel.v );
    if ( vdiff.z > 0.0f ) return bfalse;

    // approaching?
    pdiff = fvec3_sub( prider->pos.v, grip_origin.v );
    vnrm = fvec3_dot_product( vdiff.v, pdiff.v );
    if ( vnrm >= 0.0f ) return bfalse;

    // convert the rider foot position to an oct vector
    ego_oct_bb::add_vector( prider->chr_min_cv, prider->pos.v, rider_cv );

    // initialize the overlap depths
    odepth[OCT_Z] = odepth[OCT_X] = odepth[OCT_Y] = odepth[OCT_XY] = odepth[OCT_YX] = 0.0f;

    // Calculate the interaction depths. The size of the rider doesn't matter.
    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        odepth[cnt] = SDL_min( grip_cv.maxs[cnt], rider_cv.maxs[cnt] ) - SDL_max( grip_cv.mins[cnt], rider_cv.mins[cnt] );
        if ( odepth[cnt] <= 0.0f ) return bfalse;
    }

    // fix the diagonal distances
    odepth[OCT_XY] *= INV_SQRT_TWO;
    odepth[OCT_YX] *= INV_SQRT_TWO;

    // estimate the amount of overlap area in the horizontal direction
    hrz_overlap1 = odepth[OCT_X ] * odepth[OCT_Y ];
    hrz_overlap2 = odepth[OCT_XY] * odepth[OCT_YX];
    hrz_overlap  = SDL_min( hrz_overlap1, hrz_overlap2 );
    if ( hrz_overlap <= 0.0f ) return bfalse;

    // convert this to an estimate of the volume
    overlap = hrz_overlap * odepth[OCT_Z];

    // check for the best possible attachment
    if ( mount_b )
    {
        if ( overlap > pchr_a->targetmount_overlap )
        {
            // set, but do not attach the mounts yet
            pchr_a->targetmount_overlap = overlap;
            pchr_a->targetmount_ref     = ichr_b;
            mount_updated = btrue;
        }
    }
    else if ( mount_a )
    {
        if ( overlap > pchr_a->targetmount_overlap )
        {
            // set, but do not attach the mounts yet
            pchr_b->targetmount_overlap = overlap;
            pchr_b->targetmount_ref   = ichr_a;
            mount_updated = btrue;
        }
    }

    return mount_updated;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_platform_detection( const CHR_REF & ichr_a, const CHR_REF & ichr_b )
{
    int cnt;

    ego_chr * pchr_a, * pchr_b;

    bool_t platform_updated = bfalse;
    bool_t platform_a, platform_b;
    bool_t mount_a, mount_b;

    ego_oct_vec opos_object;
    ego_oct_bb    platform_min_cv, platform_max_cv;

    ego_oct_vec odepth;
    float z_overlap;

    // how well does the position of the character's feet match the platform?
    float hrz_overlap1, hrz_overlap2, hrz_overlap, vrt_overlap, overlap;

    // make sure that A is valid
    pchr_a = ChrObjList.get_allocated_data_ptr( ichr_a );
    if ( !INGAME_PCHR( pchr_a ) ) return bfalse;

    // make sure that B is valid
    pchr_b = ChrObjList.get_allocated_data_ptr( ichr_b );
    if ( !INGAME_PCHR( pchr_b ) ) return bfalse;

    // if you are mounted, only your mount is affected by platforms
    if ( IS_ATTACHED_PCHR( pchr_a ) || IS_ATTACHED_PCHR( pchr_b ) ) return bfalse;

    // only check possible object-platform interactions
    platform_a = pchr_b->canuseplatforms && pchr_a->platform;
    platform_b = pchr_a->canuseplatforms && pchr_b->platform;
    if ( !platform_a && !platform_b ) return bfalse;

    // If we can mount this platform, skip it
    mount_a = chr_can_mount( ichr_b, ichr_a );
    if ( mount_a && pchr_a->enviro.walk_level < pchr_b->pos.z + pchr_b->bump.height + PLATTOLERANCE )
        return bfalse;

    // If we can mount this platform, skip it
    mount_b = chr_can_mount( ichr_a, ichr_b );
    if ( mount_b && pchr_b->enviro.walk_level < pchr_a->pos.z + pchr_a->bump.height + PLATTOLERANCE )
        return bfalse;

    // determine whether any collision is possible
    z_overlap  = SDL_min( pchr_b->chr_max_cv.maxs[OCT_Z] + pchr_b->pos.z, pchr_a->chr_max_cv.maxs[OCT_Z] + pchr_a->pos.z ) -
                 SDL_max( pchr_b->chr_max_cv.mins[OCT_Z] + pchr_b->pos.z, pchr_a->chr_max_cv.mins[OCT_Z] + pchr_a->pos.z );

    if ( z_overlap <= 0.0f ) return bfalse;

    // if both objects are platforms determine how to arrange them
    if ( platform_a && platform_b )
    {
        bool_t chara_on_top;
        float depth_a, depth_b;
        float depth_max;

        depth_a = ( pchr_b->pos.z + pchr_b->chr_max_cv.maxs[OCT_Z] ) - pchr_a->pos.z;
        depth_b = ( pchr_a->pos.z + pchr_a->chr_max_cv.maxs[OCT_Z] ) - pchr_b->pos.z;

        depth_max = SDL_min( pchr_b->pos.z + pchr_b->chr_max_cv.maxs[OCT_Z], pchr_a->pos.z + pchr_a->chr_max_cv.maxs[OCT_Z] ) -
                    SDL_max( pchr_b->pos.z, pchr_a->pos.z );

        chara_on_top = SDL_abs( depth_max - depth_a ) < SDL_abs( depth_max - depth_b );

        if ( chara_on_top )
        {
            platform_a = bfalse;
        }
        else
        {
            platform_b = bfalse;
        }
    }

    // determine how the characters can be attached
    // The surface that the object will rest on is given by chr_min_cv.maxs[OCT_Z],
    // not chr_max_cv.maxs[OCT_Z] or chr_cv.maxs[OCT_Z]
    platform_min_cv.invalidate();
    if ( platform_a )
    {
        ego_oct_vec::ctor_this( &opos_object, pchr_b->pos );
        ego_oct_bb::add_vector( pchr_a->chr_min_cv, pchr_a->pos.v, platform_min_cv );
        ego_oct_bb::add_vector( pchr_a->chr_max_cv, pchr_a->pos.v, platform_max_cv );
    }
    else if ( platform_b )
    {
        ego_oct_vec::ctor_this( &opos_object, pchr_a->pos );
        ego_oct_bb::add_vector( pchr_b->chr_min_cv, pchr_b->pos.v, platform_min_cv );
        ego_oct_bb::add_vector( pchr_b->chr_max_cv, pchr_b->pos.v, platform_max_cv );
    }

    // initialize the overlap depths
    odepth[OCT_Z] = odepth[OCT_X] = odepth[OCT_Y] = odepth[OCT_XY] = odepth[OCT_YX] = 0.0f;

    // the size of the object doesn't matter
    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        odepth[cnt] = SDL_min( platform_max_cv.maxs[cnt] - opos_object[cnt], opos_object[cnt] - platform_max_cv.mins[cnt] );
        if ( odepth[cnt] <= 0.0f ) return bfalse;
    }

    // fix the diagonal distances
    odepth[OCT_XY] *= INV_SQRT_TWO;
    odepth[OCT_YX] *= INV_SQRT_TWO;

    // estimate the amount of overlap area in the horizontal direction
    hrz_overlap1 = odepth[OCT_X ] * odepth[OCT_Y ];
    hrz_overlap2 = odepth[OCT_XY] * odepth[OCT_YX];
    hrz_overlap  = SDL_min( hrz_overlap1, hrz_overlap2 );
    if ( hrz_overlap <= 0.0f ) return bfalse;

    // estimate the amount of overlap height
    vrt_overlap = PLATTOLERANCE - SDL_abs( opos_object[OCT_Z] - platform_min_cv.maxs[OCT_Z] );
    vrt_overlap = CLIP( vrt_overlap, 0.0f, PLATTOLERANCE );
    if ( vrt_overlap <= 0.0f ) return bfalse;

    // convert this to an estimate of the volume
    overlap = hrz_overlap * vrt_overlap;

    // check for the best possible attachment
    if ( platform_b )
    {
        if ( overlap > pchr_a->targetplatform_overlap )
        {
            // set, but do not attach the platforms yet
            pchr_a->targetplatform_overlap = overlap;
            pchr_a->targetplatform_ref     = ichr_b;
            platform_updated = btrue;
        }
    }
    else if ( platform_a )
    {
        if ( overlap > pchr_a->targetplatform_overlap )
        {
            // set, but do not attach the platforms yet
            pchr_b->targetplatform_overlap = overlap;
            pchr_b->targetplatform_ref   = ichr_a;
            platform_updated = btrue;
        }
    }

    return platform_updated;
}

//--------------------------------------------------------------------------------------------
bool_t do_prt_platform_detection( const PRT_REF & iprt_a, const CHR_REF & ichr_b )
{
    int cnt;

    ego_prt * pprt_a;
    ego_chr * pchr_b;

    bool_t platform_updated = bfalse;
    bool_t platform_b;

    ego_oct_vec opos_object;
    ego_oct_bb    platform_min_cv, platform_max_cv;

    ego_oct_vec odepth;
    float z_overlap;

    // how well does the position of the character's feet match the platform?
    float hrz_overlap1, hrz_overlap2, hrz_overlap, vrt_overlap, overlap;

    // make sure that A is valid
    if ( !DEFINED_PRT( iprt_a ) ) return bfalse;
    pprt_a = PrtObjList.get_data_ptr( iprt_a );

    // make sure that B is valid
    pchr_b = ChrObjList.get_allocated_data_ptr( ichr_b );
    if ( !INGAME_PCHR( pchr_b ) ) return bfalse;

    // if you are mounted, only your mount is affected by platforms
    if ( DEFINED_CHR( pprt_a->attachedto_ref ) || IS_ATTACHED_PCHR( pchr_b ) ) return bfalse;

    // only check possible object-platform interactions
    platform_b = /* pprt_a->canuseplatforms && */ pchr_b->platform;
    if ( !platform_b ) return bfalse;

    // determine whether any collision is possible
    z_overlap  = SDL_min( pchr_b->chr_max_cv.maxs[OCT_Z] + pchr_b->pos.z, pprt_a->prt_min_cv.maxs[OCT_Z] + pprt_a->pos.z ) -
                 SDL_max( pchr_b->chr_max_cv.mins[OCT_Z] + pchr_b->pos.z, pprt_a->prt_min_cv.mins[OCT_Z] + pprt_a->pos.z );

    if ( z_overlap <= 0.0f ) return bfalse;

    // The surface that the object will rest on is given by chr_min_cv.maxs[OCT_Z],
    // not chr_max_cv.maxs[OCT_Z] or chr_cv.maxs[OCT_Z]
    ego_oct_vec::ctor_this( &opos_object, pprt_a->pos );
    ego_oct_bb::add_vector( pchr_b->chr_min_cv, pchr_b->pos.v, platform_min_cv );
    ego_oct_bb::add_vector( pchr_b->chr_max_cv, pchr_b->pos.v, platform_max_cv );

    // initialize the overlap depths
    odepth[OCT_Z] = odepth[OCT_X] = odepth[OCT_Y] = odepth[OCT_XY] = odepth[OCT_YX] = 0.0f;

    // the size of the object doesn't matter
    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        odepth[cnt] = SDL_min( platform_max_cv.maxs[cnt] - opos_object[cnt], opos_object[cnt] - platform_max_cv.mins[cnt] );
        if ( odepth[cnt] <= 0.0f ) return bfalse;
    }

    // fix the diagonal distances
    odepth[OCT_XY] *= INV_SQRT_TWO;
    odepth[OCT_YX] *= INV_SQRT_TWO;

    // estimate the amount of overlap area in the horizontal direction
    hrz_overlap1 = odepth[OCT_X ] * odepth[OCT_Y ];
    hrz_overlap2 = odepth[OCT_XY] * odepth[OCT_YX];
    hrz_overlap  = SDL_min( hrz_overlap1, hrz_overlap2 );
    if ( hrz_overlap <= 0.0f ) return bfalse;

    // estimate the amount of overlap height
    vrt_overlap = PLATTOLERANCE - SDL_abs( opos_object[OCT_Z] - platform_min_cv.maxs[OCT_Z] );
    vrt_overlap = CLIP( vrt_overlap, 0.0f, PLATTOLERANCE );
    if ( vrt_overlap <= 0.0f ) return bfalse;

    // convert this to an estimate of the volume
    overlap = hrz_overlap * vrt_overlap;

    // check for the best possible attachment
    if ( overlap > pprt_a->targetplatform_overlap )
    {
        // set, but do not attach the platforms yet
        pprt_a->targetplatform_overlap = overlap;
        pprt_a->targetplatform_ref     = ichr_b;
        platform_updated = btrue;
    }

    return platform_updated;
}

//--------------------------------------------------------------------------------------------
bool_t attach_chr_to_platform( ego_chr * pchr, ego_chr * pplat )
{
    /// \author BB
    /// \details  attach a character to a platform

    ego_cap * pchr_cap;

    // verify that we do not have two dud pointers
    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;
    if ( !PROCESSING_PCHR( pplat ) ) return bfalse;

    pchr_cap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pchr_cap ) return bfalse;

    // check if they can be connected
    if ( !pchr_cap->canuseplatforms || IS_FLYING_PCHR( pchr ) ) return bfalse;
    if ( !pplat->platform ) return bfalse;

    // let the code know that the platform attachment was confirmed this update
    pchr->onwhichplatform_update = update_wld;

    // is there a new attachment?
    if ( pchr->onwhichplatform_ref != GET_REF_PCHR( pplat ) )
    {
        // formally detach the character from the old platform
        if ( CHR_REF( MAX_CHR ) != pchr->onwhichplatform_ref )
        {
            detach_character_from_platform( pchr );
        }

        // attach the character to this plaform
        pchr->onwhichplatform_ref = GET_REF_PCHR( pplat );

        // update the character-platform properties
        chr_calc_environment( pchr );

        // tell the platform that we bumped into it
        // this is necessary for key buttons to work properly, for instance
        ego_ai_state::set_bumplast( &( pplat->ai ), GET_REF_PCHR( pchr ) );
    }

    // blank out the targetplatform_ref, so we know we are done searching
    pchr->targetplatform_ref     = CHR_REF( MAX_CHR );
    pchr->targetplatform_overlap = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detach_character_from_platform( ego_chr * pchr )
{
    /// \author BB
    /// \details  detach a character from a platform

    ego_chr * pplat;

    // verify that we do not have two dud pointers
    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;

    pchr->onwhichplatform_update   = 0;
    pchr->targetplatform_ref       = CHR_REF( MAX_CHR );
    pchr->targetplatform_overlap   = 0.0f;

    // grab a pointer to the old platform
    pplat = NULL;
    if ( PROCESSING_CHR( pchr->onwhichplatform_ref ) )
    {
        pplat = ChrObjList.get_data_ptr( pchr->onwhichplatform_ref );
    }

    // undo the attachment
    pchr->onwhichplatform_ref     = CHR_REF( MAX_CHR );

    // do some platform adjustments
    if ( NULL != pplat )
    {
        // adjust the platform's weight, if necessary
        pplat->holdingweight -= pchr->onwhichplatform_weight;
        pplat->holdingweight = SDL_max( 0.0f, pplat->holdingweight );

        // update the character-platform properties
        chr_calc_environment( pchr );
    }

    // fix some character variables
    pchr->onwhichplatform_weight  = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_prt_to_platform( ego_prt * pprt, ego_chr * pplat )
{
    /// \author BB
    /// \details  attach a particle to a platform

    ego_pip   * pprt_pip;

    // verify that we do not have two dud pointers
    if ( !PROCESSING_PPRT( pprt ) ) return bfalse;
    if ( !PROCESSING_PCHR( pplat ) ) return bfalse;

    pprt_pip = ego_prt::get_ppip( GET_REF_PPRT( pprt ) );
    if ( NULL == pprt_pip ) return bfalse;

    // check if they can be connected
    if ( !pplat->platform ) return bfalse;

    // let the code know that the platform attachment was confirmed this update
    pprt->onwhichplatform_update = update_wld;

    // do the attachment
    if ( pprt->onwhichplatform_ref != GET_REF_PCHR( pplat ) )
    {
        ego_bundle_prt bdl( pprt );

        pprt->onwhichplatform_ref = GET_REF_PCHR( pplat );

        prt_calc_environment( bdl );
    }

    // blank the targetplatform_* stuff so we know we are done searching
    pprt->targetplatform_ref     = CHR_REF( MAX_CHR );
    pprt->targetplatform_overlap = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detach_particle_from_platform( ego_prt * pprt )
{
    /// \author BB
    /// \details  attach a particle to a platform

    // verify that we do not have two dud pointers
    if ( !DEFINED_PPRT( pprt ) ) return bfalse;

    // grab all of the particle info
    ego_bundle_prt bdl_prt( pprt );

    // check if they can be connected
    if ( INGAME_CHR( pprt->onwhichplatform_ref ) ) return bfalse;

    // undo the attachment
    pprt->onwhichplatform_ref    = CHR_REF( MAX_CHR );
    pprt->onwhichplatform_update = 0;
    pprt->targetplatform_ref     = CHR_REF( MAX_CHR );
    pprt->targetplatform_overlap   = 0.0f;

    // get the correct particle environment
    prt_calc_environment( bdl_prt );

    return btrue;
}

//--------------------------------------------------------------------------------------------
egoboo_rv do_chr_platform_physics( collision_node * d, ego_bundle_chr & bdl_item, ego_bundle_chr & bdl_plat )
{
    // we know that ichr_a is a platform and ichr_b is on it
    egoboo_rv retval = rv_error;
    ego_chr * loc_pitem, * loc_pplat;

    int     rot_a, rot_b;
    float   walk_lerp, plat_lerp, vel_lerp;
    fvec3_t platform_up, vdiff;
    float   vnrm;
    const float max_vnrm = 50.0f;

    // aliases to make notation easier
    loc_pitem = bdl_item;
    loc_pplat = bdl_plat;

    // does the item exist?
    if ( !PROCESSING_PCHR( loc_pitem ) ) return rv_error;

    // does the platform exist?
    if ( !PROCESSING_PCHR( loc_pplat ) ) return rv_error;

    // wrong platform for this character
    if ( loc_pitem->onwhichplatform_ref != GET_REF_PCHR( loc_pplat ) ) return rv_error;

    // grab the "up" vector for the platform
    ego_chr::get_MatUp( loc_pplat, &platform_up );
    fvec3_self_normalize( platform_up.v );

    // what is the "up" velocity relative to the platform?
    vdiff = fvec3_sub( loc_pitem->vel.v, loc_pplat->vel.v );
    vnrm = fvec3_dot_product( vdiff.v, platform_up.v );

    // if the character is moving too fast relative to the platform, then bounce instead of attaching
    if ( SDL_abs( vnrm ) > max_vnrm )
    {
        goto do_chr_platform_physics_fail;
    }

    // re-compute the walk_lerp value, since we need a custom value here
    walk_lerp = ( loc_pitem->pos.z - ( loc_pplat->pos.z + loc_pplat->chr_min_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;

    if ( walk_lerp < 0.0f )
    {
        plat_lerp = 1.0f;
    }
    else
    {
        plat_lerp = 1.0f - walk_lerp;
        plat_lerp = CLIP( plat_lerp, 0.0f, 1.0f );
    }

    // If your velocity is going up much faster than the
    // platform, there is no need to suck you to the level of the platform.
    // This was one of the things preventing you from jumping from platforms
    // properly
    vel_lerp = 0.0f;
    if ( vnrm > 0.0f )
    {
        vel_lerp = SDL_abs( vnrm ) / max_vnrm;
        vel_lerp = CLIP( vel_lerp, 0.0f, 1.0f );
    }

    // determine the rotation rates
    rot_b = loc_pitem->ori.facing_z - loc_pitem->ori_old.facing_z;
    rot_a = loc_pplat->ori.facing_z - loc_pplat->ori_old.facing_z;

    retval = rv_fail;
    {
        float apos_plat_coeff = 1.0f, avel_coeff = 1.0f, facing_coeff = 1.0f;

        if ( vel_lerp > 0.0f )
        {
            // movement away from the platform in the z-direction can reduce the platform interaction
            plat_lerp  *= 1.0f - vel_lerp;
        }

        if ( walk_lerp < -1.0f )
        {
            // The item is basically stuck in the platform.
            apos_plat_coeff = 0.125f * 2.0f;
            avel_coeff      = 1.0f;
            facing_coeff    = 1.0f;
            retval          = rv_success;
        }
        else if ( walk_lerp < 0.0f )
        {
            // The item is slightly inside the platform
            apos_plat_coeff = 0.125f;
            avel_coeff      = 0.25f;
            facing_coeff    = 1.0f;
            retval          = rv_success;
        }
        else if ( plat_lerp > 0.0f )
        {
            // The item is above the platform
            apos_plat_coeff = 0.125f * plat_lerp;
            avel_coeff      = 0.25f * plat_lerp;
            facing_coeff    = plat_lerp;
            retval          = rv_success;
        }

        phys_data_accumulate_apos_plat_index( &( loc_pitem->phys ), ( 1.0f + loc_pitem->enviro.walk_level - loc_pitem->pos.z ) * apos_plat_coeff, kZ );
        phys_data_accumulate_avel_index( &( loc_pitem->phys ), ( loc_pplat->vel.z  - loc_pitem->vel.z ) * avel_coeff, kZ );
        loc_pitem->ori.facing_z += ( float )( rot_a - rot_b ) * facing_coeff;
    }

    // rv_fail indicates that the platform did not attach.
    // in that case, we need to drop down to do_chr_platform_physics_fail
    if ( retval != rv_fail )
    {
        return retval;
    }

do_chr_platform_physics_fail:

    detach_character_from_platform( loc_pitem );

    return rv_fail;
}

//--------------------------------------------------------------------------------------------
float estimate_chr_prt_normal( ego_chr * pchr, ego_prt * pprt, fvec3_base_t nrm, fvec3_base_t vdiff )
{
    fvec3_t collision_size;
    float dot;

    collision_size.x = SDL_max( pchr->chr_min_cv.maxs[OCT_X] - pchr->chr_min_cv.mins[OCT_X], 2.0f * pprt->bump_min.size );
    collision_size.y = SDL_max( pchr->chr_min_cv.maxs[OCT_Y] - pchr->chr_min_cv.mins[OCT_Y], 2.0f * pprt->bump_min.size );
    collision_size.z = SDL_max( pchr->chr_min_cv.maxs[OCT_Z] - pchr->chr_min_cv.mins[OCT_Z], 2.0f * pprt->bump_min.height );

    // estimate the "normal" for the collision, using the center-of-mass difference
    nrm[kX] = pprt->pos.x - pchr->pos.x;
    nrm[kY] = pprt->pos.y - pchr->pos.y;
    nrm[kZ] = pprt->pos.z - ( pchr->pos.z + 0.5f * ( pchr->chr_min_cv.maxs[OCT_Z] + pchr->chr_min_cv.mins[OCT_Z] ) );

    // scale the collision box
    nrm[kX] /= collision_size.x;
    nrm[kY] /= collision_size.y;
    nrm[kZ] /= collision_size.z;

    // scale the z-normals so that the collision volume will act somewhat like a cylinder
    nrm[kZ] *= nrm[kZ] * nrm[kZ];

    // reject the reflection request if the particle is moving in the wrong direction
    vdiff[kX] = pprt->vel.x - pchr->vel.x;
    vdiff[kY] = pprt->vel.y - pchr->vel.y;
    vdiff[kZ] = pprt->vel.z - pchr->vel.z;
    dot       = fvec3_dot_product( vdiff, nrm );

    // we really never should have the condition that dot > 0, unless the particle is "fast"
    if ( dot >= 0.0f )
    {
        fvec3_t vtmp;

        // If the particle is "fast" relative to the object size, it can happen that the particle
        // can be more than halfway through the character before it is detected.

        vtmp.x = vdiff[kX] / collision_size.x;
        vtmp.y = vdiff[kY] / collision_size.y;
        vtmp.z = vdiff[kZ] / collision_size.z;

        // If it is fast, re-evaluate the normal in a different way
        if ( vtmp.x*vtmp.x + vtmp.y*vtmp.y + vtmp.z*vtmp.z > 0.5f*0.5f )
        {
            // use the old position, which SHOULD be before the collision
            // to determine the normal
            nrm[kX] = pprt->pos_old.x - pchr->pos_old.x;
            nrm[kY] = pprt->pos_old.y - pchr->pos_old.y;
            nrm[kZ] = pprt->pos_old.z - ( pchr->pos_old.z + 0.5f * ( pchr->chr_min_cv.maxs[OCT_Z] + pchr->chr_min_cv.mins[OCT_Z] ) );

            // scale the collision box
            nrm[kX] /= collision_size.x;
            nrm[kY] /= collision_size.y;
            nrm[kZ] /= collision_size.z;

            // scale the z-normals so that the collision volume will act somewhat like a cylinder
            nrm[kZ] *= nrm[kZ] * nrm[kZ];
        }
    }

    // assume the function fails
    dot = 0.0f;

    // does the normal exist?
    if ( fvec3_length_abs( nrm ) > 0.0f )
    {
        // Make the normal a unit normal
        fvec3_t ntmp = fvec3_normalize( nrm );
        SDL_memcpy( nrm, ntmp.v, sizeof( fvec3_base_t ) );

        // determine the actual dot product
        dot = fvec3_dot_product( vdiff, nrm );
    }

    return dot;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision_test( const ego_bundle_chr & bdl_a, const ego_bundle_chr & bdl_b )
{
    /// BB@> break off the collision tests to this function

    CHR_REF loc_ichr_a, loc_ichr_b;
    ego_chr * loc_pchr_a, * loc_pchr_b;
    ego_cap * loc_pcap_a, * loc_pcap_b;

    // make some aliases for easier notation
    loc_ichr_a = bdl_a;
    loc_pchr_a = bdl_a;
    loc_pcap_a = bdl_a;

    loc_ichr_b = bdl_b;
    loc_pchr_b = bdl_b;
    loc_pcap_b = bdl_b;

    // items can interact with platforms but not with other characters/items
    if ( loc_pchr_a->isitem && !loc_pchr_b->platform ) return bfalse;
    if ( loc_pchr_b->isitem && !loc_pchr_a->platform ) return bfalse;

    // make sure that flying platforms fo not interact
    // I think this would mess up the platforms in the tourist starter
    if (( loc_pchr_a->is_flying_platform && loc_pchr_a->platform ) && ( loc_pchr_b->is_flying_platform && loc_pchr_b->platform ) ) return bfalse;

    // don't interact with your mount, or your held items
    if ( loc_ichr_a == loc_pchr_b->attachedto || loc_ichr_b == loc_pchr_a->attachedto ) return bfalse;

    // don't do anything if there is no bump size
    if ( 0.0f == loc_pchr_a->bump_stt.size || 0.0f == loc_pchr_b->bump_stt.size ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_plat_collision_test( const ego_bundle_chr & bdl_item, const ego_bundle_chr & bdl_plat )
{
    /// \author BB
    /// \details break off the collision tests to this function

    CHR_REF loc_ichr_plat;
    ego_chr * loc_pchr_item, * loc_pchr_plat;

    // make some aliases for easier notation
    loc_pchr_item = bdl_item;

    loc_ichr_plat = bdl_plat;
    loc_pchr_plat = bdl_plat;

    // items can interact with platforms but not with other characters/items
    if ( !loc_pchr_item->canuseplatforms || !loc_pchr_plat->platform ) return bfalse;

    // are these two attached?
    if ( loc_ichr_plat != loc_pchr_item->onwhichplatform_ref ) return bfalse;

    // if the item is flying, there is no normal platform interaction
    if ( loc_pchr_item->platform && loc_pchr_item->is_flying_platform ) return bfalse;

    // don't interact if you are mounted or held by anything
    if ( IS_ATTACHED_PCHR( loc_pchr_item ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision_interaction( collision_node * d, ego_bundle_chr & bdl_a, ego_bundle_chr & bdl_b )
{
    CHR_REF loc_ichr_a, loc_ichr_b;
    ego_chr * loc_pchr_a, * loc_pchr_b;
    ego_cap * loc_pcap_a, * loc_pcap_b;

    float interaction_strength = 1.0f;
    float wta, wtb;

    fvec3_t   nrm;
    int exponent = 1;

    ego_oct_vec opos_a, opos_b, odepth;
    bool_t    collision = bfalse, bump = bfalse;

    // make some aliases for easier notation
    loc_ichr_a = bdl_a;
    loc_pchr_a = bdl_a;
    loc_pcap_a = bdl_a;

    loc_ichr_b = bdl_b;
    loc_pchr_b = bdl_b;
    loc_pcap_b = bdl_b;

    // skip objects that are inside inventories
    if ( loc_pchr_a->pack.is_packed || loc_pchr_b->pack.is_packed ) return bfalse;

    interaction_strength = 1.0f;

    // ghosts don't interact much
    interaction_strength *= loc_pchr_a->gfx_inst.alpha * INV_FF;
    interaction_strength *= loc_pchr_b->gfx_inst.alpha * INV_FF;

    // are we interacting with a previous mount?
    interaction_strength *= calc_dismount_lerp( loc_pchr_a, loc_pchr_b );

    // ensure that we have a collision volume
    if ( ego_oct_bb::empty( d->cv ) )
    {
        bool_t found;

        // If you got to this point and this bounding box is empty,
        // then you detected a platform interaction that was not used.
        // Recalculate everything assuming a normal (not platform) interaction

        found = phys_intersect_oct_bb( loc_pchr_a->chr_min_cv, loc_pchr_a->pos, loc_pchr_a->vel, loc_pchr_b->chr_min_cv, loc_pchr_b->pos, loc_pchr_b->vel, 0, &( d->cv ), &( d->tmin ), &( d->tmax ) );

        // if no valid collision was found, return with a fail code
        if ( !found || ego_oct_bb::empty( d->cv ) || d->tmax < 0.0f || d->tmin > 1.0f )
        {
            return bfalse;
        }
    }

    // reduce the interaction strength with platforms
    // that are overlapping with the platform you are actually on
    if ( loc_pchr_a->platform && INGAME_CHR( loc_pchr_b->onwhichplatform_ref ) && loc_ichr_a != loc_pchr_b->onwhichplatform_ref )
    {
        float lerp_z = ( loc_pchr_b->pos.z - ( loc_pchr_a->pos.z + loc_pchr_a->chr_min_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        // if the platform is close to your level, ignore it
        interaction_strength *= SDL_abs( lerp_z );
    }

    if ( loc_pchr_b->platform && INGAME_CHR( loc_pchr_a->onwhichplatform_ref ) && loc_ichr_b != loc_pchr_a->onwhichplatform_ref )
    {
        float lerp_z = ( loc_pchr_a->pos.z - ( loc_pchr_b->pos.z + loc_pchr_b->chr_min_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        // if the platform is close to your level, ignore it
        interaction_strength *= SDL_abs( lerp_z );
    }

    // estimate the collision volume and depth from a 10% overlap
    {
        int cnt;

        ego_oct_bb   src1, src2;

        ego_oct_bb   exp1, exp2, intersect;

        float tmp_min, tmp_max;

        tmp_min = d->tmin;
        tmp_max = d->tmin + ( d->tmax - d->tmin ) * 0.1f;

        // shift the source bounding boxes to be centered on the given positions
        ego_oct_bb::add_vector( loc_pchr_a->chr_min_cv, loc_pchr_a->pos.v, src1 );
        ego_oct_bb::add_vector( loc_pchr_b->chr_min_cv, loc_pchr_b->pos.v, src2 );

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb( src1, loc_pchr_a->vel, tmp_min, tmp_max, exp1 );
        phys_expand_oct_bb( src2, loc_pchr_b->vel, tmp_min, tmp_max, exp2 );

        // determine the intersection of these two volumes
        ego_oct_bb::do_intersection( exp1, exp2, intersect );

        for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
        {
            odepth[cnt]  = intersect.maxs[cnt] - intersect.mins[cnt];
        }

        // scale the diagonal components so that they are actually distances
        odepth[OCT_XY] *= INV_SQRT_TWO;
        odepth[OCT_YX] *= INV_SQRT_TWO;
    }

    // use the info from the collision volume to determine whether the objects are colliding
    collision = ( d->tmin > 0.0f );

    //------------------
    // do character-character interactions

    // calculate a "mass" for each object, taking into account possible infinite masses
    character_physics_get_mass_pair( loc_pchr_a, loc_pchr_b, &wta, &wtb );

    // create an "octagonal position" for each object
    ego_oct_vec::ctor_this( &opos_a, loc_pchr_a->pos );
    ego_oct_vec::ctor_this( &opos_b, loc_pchr_b->pos );

    // adjust the center-of-mass
    opos_a[OCT_Z] += ( loc_pchr_a->chr_min_cv.maxs[OCT_Z] + loc_pchr_a->chr_min_cv.mins[OCT_Z] ) * 0.5f;
    opos_b[OCT_Z] += ( loc_pchr_b->chr_min_cv.maxs[OCT_Z] + loc_pchr_b->chr_min_cv.mins[OCT_Z] ) * 0.5f;

    // make the object more like a table if there is a platform-like interaction
    if ( loc_pchr_a->canuseplatforms && loc_pchr_b->platform ) exponent += 2;
    if ( loc_pchr_b->canuseplatforms && loc_pchr_a->platform ) exponent += 2;

    if ( phys_estimate_chr_chr_normal( opos_a, opos_b, odepth, exponent, nrm.v ) )
    {
        fvec3_t   vel_a, vel_b;
        fvec3_t   vpara_a, vperp_a;
        fvec3_t   vpara_b, vperp_b;
        fvec3_t   imp_a, imp_b;
        float     vdot;

        vel_a = loc_pchr_a->vel;
        vel_b = loc_pchr_b->vel;

        vdot = fvec3_dot_product( nrm.v, vel_a.v );
        vperp_a.x = nrm.x * vdot;
        vperp_a.y = nrm.y * vdot;
        vperp_a.z = nrm.z * vdot;
        vpara_a   = fvec3_sub( vel_a.v, vperp_a.v );

        vdot = fvec3_dot_product( nrm.v, vel_b.v );
        vperp_b.x = nrm.x * vdot;
        vperp_b.y = nrm.y * vdot;
        vperp_b.z = nrm.z * vdot;
        vpara_b   = fvec3_sub( vel_b.v, vperp_b.v );

        // clear the "impulses"
        fvec3_self_clear( imp_a.v );
        fvec3_self_clear( imp_b.v );

        // what type of "collision" is this? (impulse or pressure)
        if ( collision )
        {
            // an actual bump, use impulse to make the objects bounce appart

            // generic coefficient of restitution
            float cr = 0.5f;

            if (( wta < 0.0f && wtb < 0.0f ) || ( wta == wtb ) )
            {
                float factor = 0.5f * ( 1.0f - cr );

                imp_a.x = factor * ( vperp_b.x - vperp_a.x );
                imp_a.y = factor * ( vperp_b.y - vperp_a.y );
                imp_a.z = factor * ( vperp_b.z - vperp_a.z );

                imp_b.x = factor * ( vperp_a.x - vperp_b.x );
                imp_b.y = factor * ( vperp_a.y - vperp_b.y );
                imp_b.z = factor * ( vperp_a.z - vperp_b.z );
            }
            else if (( wta < 0.0f ) || ( wtb == 0.0f ) )
            {
                float factor = ( 1.0f - cr );

                imp_b.x = factor * ( vperp_a.x - vperp_b.x );
                imp_b.y = factor * ( vperp_a.y - vperp_b.y );
                imp_b.z = factor * ( vperp_a.z - vperp_b.z );
            }
            else if (( wtb < 0.0f ) || ( wta == 0.0f ) )
            {
                float factor = ( 1.0f - cr );

                imp_a.x = factor * ( vperp_b.x - vperp_a.x );
                imp_a.y = factor * ( vperp_b.y - vperp_a.y );
                imp_a.z = factor * ( vperp_b.z - vperp_a.z );
            }
            else
            {
                float factor;

                factor = ( 1.0f - cr ) * wtb / ( wta + wtb );
                imp_a.x = factor * ( vperp_b.x - vperp_a.x );
                imp_a.y = factor * ( vperp_b.y - vperp_a.y );
                imp_a.z = factor * ( vperp_b.z - vperp_a.z );

                factor = ( 1.0f - cr ) * wta / ( wta + wtb );
                imp_b.x = factor * ( vperp_a.x - vperp_b.x );
                imp_b.y = factor * ( vperp_a.y - vperp_b.y );
                imp_b.z = factor * ( vperp_a.z - vperp_b.z );
            }

            // add in the bump impulses
            {
                fvec3_t _tmp_vec = fvec3_scale( imp_a.v, interaction_strength );

                phys_data_accumulate_avel( &( loc_pchr_a->phys ), _tmp_vec.v );
            }

            {
                fvec3_t _tmp_vec = fvec3_scale( imp_b.v, interaction_strength );

                phys_data_accumulate_avel( &( loc_pchr_b->phys ), _tmp_vec.v );
            }

            bump = btrue;
        }
        else
        {
            // not a bump at all. two objects are rubbing against one another
            // and continually overlapping. use pressure to push them appart.

            float tmin;

            tmin = 1e6;
            if ( nrm.x != 0.0f )
            {
                tmin = SDL_min( tmin, odepth[OCT_X] / SDL_abs( nrm.x ) );
            }
            if ( nrm.y != 0.0f )
            {
                tmin = SDL_min( tmin, odepth[OCT_Y] / SDL_abs( nrm.y ) );
            }
            if ( nrm.z != 0.0f )
            {
                tmin = SDL_min( tmin, odepth[OCT_Z] / SDL_abs( nrm.z ) );
            }

            if ( nrm.x + nrm.y != 0.0f )
            {
                tmin = SDL_min( tmin, odepth[OCT_XY] / SDL_abs( nrm.x + nrm.y ) );
            }

            if ( -nrm.x + nrm.y != 0.0f )
            {
                tmin = SDL_min( tmin, odepth[OCT_YX] / SDL_abs( -nrm.x + nrm.y ) );
            }

            if ( tmin < 1e6 )
            {
                const float pressure_strength = 0.125f;

                if ( wta >= 0.0f )
                {
                    float ratio = ( float )SDL_abs( wtb ) / (( float )SDL_abs( wta ) + ( float )SDL_abs( wtb ) );

                    imp_a.x = tmin * nrm.x * ratio * pressure_strength;
                    imp_a.y = tmin * nrm.y * ratio * pressure_strength;
                    imp_a.z = tmin * nrm.z * ratio * pressure_strength;
                }

                if ( wtb >= 0.0f )
                {
                    float ratio = ( float )SDL_abs( wta ) / (( float )SDL_abs( wta ) + ( float )SDL_abs( wtb ) );

                    imp_b.x = -tmin * nrm.x * ratio * pressure_strength;
                    imp_b.y = -tmin * nrm.y * ratio * pressure_strength;
                    imp_b.z = -tmin * nrm.z * ratio * pressure_strength;
                }
            }

            // add in the bump impulses
            {
                fvec3_t _tmp_vec = fvec3_scale( imp_a.v, interaction_strength );

                phys_data_accumulate_apos_coll( &( loc_pchr_a->phys ), _tmp_vec.v );
            }

            {
                fvec3_t _tmp_vec = fvec3_scale( imp_b.v, interaction_strength );

                phys_data_accumulate_apos_coll( &( loc_pchr_b->phys ), _tmp_vec.v );
            }

            // you could "bump" something if you changed your velocity, even if you were still touching
            bump = ( fvec3_dot_product( loc_pchr_a->vel.v, nrm.v ) * fvec3_dot_product( loc_pchr_a->vel_old.v, nrm.v ) < 0 ) ||
                   ( fvec3_dot_product( loc_pchr_b->vel.v, nrm.v ) * fvec3_dot_product( loc_pchr_b->vel_old.v, nrm.v ) < 0 );
        }

        // add in the friction due to the "collision"
        // assume coeff of friction of 0.5
        if ( fvec3_length_abs( imp_a.v ) > 0.0f && fvec3_length_abs( vpara_a.v ) > 0.0f && loc_pchr_a->dismount_timer <= 0 )
        {
            float imp, vel, factor;

            imp = 0.5f * fvec3_length( imp_a.v );
            vel = fvec3_length( vpara_a.v );

            factor = imp / vel;
            factor = CLIP( factor, 0.0f, 1.0f );

            {
                fvec3_t _tmp_vec = fvec3_scale( vpara_a.v, -factor * interaction_strength );

                phys_data_accumulate_avel( &( loc_pchr_a->phys ), _tmp_vec.v );
            }
        }

        if ( fvec3_length_abs( imp_b.v ) > 0.0f && fvec3_length_abs( vpara_b.v ) > 0.0f && loc_pchr_b->dismount_timer <= 0 )
        {
            float imp, vel, factor;

            imp = 0.5f * fvec3_length( imp_b.v );
            vel = fvec3_length( vpara_b.v );

            factor = imp / vel;
            factor = CLIP( factor, 0.0f, 1.0f );

            {
                fvec3_t _tmp_vec = fvec3_scale( vpara_b.v, -factor * interaction_strength );

                phys_data_accumulate_avel( &( loc_pchr_b->phys ), _tmp_vec.v );
            }
        }
    }

    if ( bump )
    {
        ego_ai_state::set_bumplast( &( loc_pchr_a->ai ), loc_ichr_b );
        ego_ai_state::set_bumplast( &( loc_pchr_b->ai ), loc_ichr_a );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision( collision_node * d )
{
    /// \author BB
    /// \details Take a collision node and actually do all of the collision physics

    bool_t retval = bfalse;                         // assume that the collision will fail

    ego_bundle_chr bdl_a, bdl_b;
    CHR_REF loc_ichr_a, loc_ichr_b;
    ego_chr * loc_pchr_a = NULL, * loc_pchr_b = NULL;

    ego_bundle_chr * pbdl_item = NULL, * pbdl_plat = NULL;

    bool_t do_platform, do_collision;

    // make sure that the collision node exists
    if ( NULL == d ) return bfalse;

    // make sure it is the correct type of collision node
    if ( !d->is_chr_chr() ) return bfalse;

    if ( !INGAME_CHR( d->src.get_chr() ) ) return bfalse;
    if ( NULL == ego_bundle_chr::set( &bdl_a, ChrObjList.get_data_ptr( d->src.get_chr() ) ) ) return bfalse;

    // make some aliases for easier notation
    loc_ichr_a = bdl_a;
    loc_pchr_a = bdl_a;

    // make sure the item is not packed,
    // though it should never have been entered into the collision BSP if it was
    if ( loc_pchr_a->pack.is_packed ) return bfalse;

    if ( !INGAME_CHR( d->dst.get_chr() ) ) return bfalse;
    if ( NULL == ego_bundle_chr::set( &bdl_b, ChrObjList.get_data_ptr( d->dst.get_chr() ) ) ) return bfalse;

    // make some aliases for easier notation
    loc_ichr_b = bdl_b;
    loc_pchr_b = bdl_b;

    // make sure the item is not packed,
    // though it should never have been entered into the collision BSP if it was
    if ( loc_pchr_b->pack.is_packed ) return bfalse;

    pbdl_item = NULL;
    pbdl_plat = NULL;

    do_collision = bfalse;
    do_platform  = bfalse;

    if ( loc_ichr_a == loc_pchr_b->onwhichplatform_ref )
    {
        // platform interaction. if the onwhichplatform_ref is set, then
        // all collision tests have been met

        do_platform = do_chr_plat_collision_test( bdl_b, bdl_a );

        if ( do_platform )
        {
            pbdl_item = &bdl_b;
            pbdl_plat = &bdl_a;
        }
    }

    if ( loc_ichr_b == loc_pchr_a->onwhichplatform_ref )
    {
        // platform interaction. if the onwhichplatform_ref is set, then
        // all collision tests have been met

        do_platform = do_chr_plat_collision_test( bdl_a, bdl_b );

        if ( do_platform )
        {
            pbdl_item = &bdl_a;
            pbdl_plat = &bdl_b;
        }
    }

    if ( !do_platform )
    {
        // make sure that the collision is allowed
        do_collision = do_chr_chr_collision_test( bdl_a, bdl_b );
    }

    // do we have a character-platform interaction?
    if ( do_platform )
    {
        // do the actual platform math
        egoboo_rv plat_retval = do_chr_platform_physics( d, *pbdl_item, *pbdl_plat );

        if ( rv_fail == plat_retval )
        {
            // the platform interaction failed.
            // fall back to the character-character interaction code
            do_platform  = bfalse;
            do_collision = btrue;
        }
        else
        {
            // this is handled
            retval = ( rv_success == plat_retval );
        }
    }

    // do we have a normal character-character interaction?
    if ( do_collision )
    {
        // do the actual collision math
        retval = do_chr_chr_collision_interaction( d, bdl_a, bdl_b );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t do_prt_platform_physics( ego_prt * pprt, ego_chr * pplat, ego_chr_prt_collsion_data * pdata )
{
    /// \author BB
    /// \details  handle the particle interaction with a platform it is not attached "on".
    ///               \note gravity is not handled here
    ///               \note use bump_min here since we are interested in the physical size of the particle

    bool_t plat_collision = bfalse;
    bool_t z_collide, was_z_collide;

    if ( NULL == pdata ) return bfalse;

    // is the platform a platform?
    if ( !PROCESSING_PCHR( pplat ) || !pplat->platform ) return bfalse;

    // can the particle interact with it?
    if ( !PROCESSING_PPRT( pprt ) || INGAME_CHR( pprt->attachedto_ref ) ) return bfalse;

    // this is handled elsewhere
    if ( GET_REF_PCHR( pplat ) == pprt->onwhichplatform_ref ) return bfalse;

    // Test to see whether the particle is in the right position to interact with the platform.
    // You have to be closer to a platform to interact with it than for a general object,
    // but the vertical distance is looser.
    plat_collision = test_interaction_close_1( pplat->chr_max_cv, pplat->pos, pprt->bump_min, ego_prt::get_pos( pprt ), btrue );

    if ( !plat_collision ) return bfalse;

    // the only way to get to this point is if the two objects don't collide
    // but they are within the PLATTOLERANCE of each other in the z direction

    // are they colliding for the first time?
    z_collide     = ( pprt->pos.z - pprt->bump_min.height < pplat->pos.z + pplat->chr_max_cv.maxs[OCT_Z] ) && ( pprt->pos.z + pprt->bump_min.height > pplat->pos.z + pplat->chr_max_cv.mins[OCT_Z] );
    was_z_collide = ( pprt->pos_old.z - pprt->bump_min.height < pplat->pos_old.z + pplat->chr_max_cv.maxs[OCT_Z] ) && ( pprt->pos_old.z + pprt->bump_min.height > pplat->pos_old.z + pplat->chr_max_cv.mins[OCT_Z] );

    if ( z_collide && !was_z_collide )
    {
        // Particle is falling onto the platform
        pprt->pos.z = pplat->pos.z + pplat->chr_max_cv.maxs[OCT_Z];
        pprt->vel.z = pplat->vel.z - pprt->vel.z * pdata->ppip->dampen;

        // This should prevent raindrops from stacking up on the top of trees and other
        // objects
        if ( pdata->ppip->end_ground && pplat->platform )
        {
            pdata->terminate_particle = btrue;
        }

        plat_collision = btrue;
    }
    else if ( z_collide && was_z_collide )
    {
        // colliding this time and last time. particle is *embedded* in the platform
        pprt->pos.z = pplat->pos.z + pplat->chr_max_cv.maxs[OCT_Z] + pprt->bump_min.height;

        if ( pprt->vel.z - pplat->vel.z < 0 )
        {
            pprt->vel.z = pplat->vel.z * pdata->ppip->dampen + platstick * pplat->vel.z;
        }
        else
        {
            pprt->vel.z = pprt->vel.z * ( 1.0f - platstick ) + pplat->vel.z * platstick;
        }
        pprt->vel.x = pprt->vel.x * ( 1.0f - platstick ) + pplat->vel.x * platstick;
        pprt->vel.y = pprt->vel.y * ( 1.0f - platstick ) + pplat->vel.y * platstick;

        plat_collision = btrue;
    }
    else
    {
        // not colliding this time or last time. particle is just near the platform
        float lerp_z = (( pprt->pos.z - pprt->bump_min.height ) - ( pplat->pos.z + pplat->chr_max_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        if ( lerp_z > 0.0f )
        {
            float tmp_platstick = platstick * lerp_z;
            pprt->vel.z = pprt->vel.z * ( 1.0f - tmp_platstick ) + pplat->vel.z * tmp_platstick;
            pprt->vel.x = pprt->vel.x * ( 1.0f - tmp_platstick ) + pplat->vel.x * tmp_platstick;
            pprt->vel.y = pprt->vel.y * ( 1.0f - tmp_platstick ) + pplat->vel.y * tmp_platstick;

            plat_collision = btrue;
        }
    }

    return plat_collision;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_deflect( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata )
{
    bool_t prt_deflected = bfalse;

    bool_t chr_is_invictus, chr_can_deflect;
    bool_t prt_wants_deflection;
    FACING_T direction;
    ego_pip  *ppip;

    if ( NULL == pdata ) return bfalse;

    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;

    if ( !INGAME_PPRT( pprt ) ) return bfalse;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack + pprt->pip_ref;

    /// \note ZF@> Simply ignore characters with invictus for now, it causes some strange effects
    if ( IS_INVICTUS_PCHR_RAW( pchr ) ) return btrue;

    // find the "attack direction" of the particle
    direction = vec_to_facing( pchr->pos.x - pprt->pos.x, pchr->pos.y - pprt->pos.y );
    direction = pchr->ori.facing_z - direction + ATK_BEHIND;

    // shield block?
    chr_is_invictus = is_invictus_direction( direction, GET_REF_PCHR( pchr ), ppip->damfx );

    // determine whether the character is magically protected from missile attacks
    prt_wants_deflection  = ( MISSILE_NORMAL != pchr->missiletreatment ) &&
                            ( pprt->owner_ref != GET_REF_PCHR( pchr ) ) && !pdata->ppip->bump_money;

    chr_can_deflect = ( 0 != pchr->damagetime ) && ( pdata->max_damage > 0 );

    // try to deflect the particle
    prt_deflected = bfalse;
    pdata->mana_paid = bfalse;
    if ( chr_is_invictus || ( prt_wants_deflection && chr_can_deflect ) )
    {
        // Initialize for the billboard
        const float lifetime = 3;
        SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};
        GLXvector4f tint  = { 0.0f, 0.75f, 1.00f, 1.00f };

        // magically deflect the particle or make a ricochet if the character is invictus
        int treatment;

        treatment     = MISSILE_DEFLECT;
        prt_deflected = btrue;
        if ( prt_wants_deflection )
        {
            treatment = pchr->missiletreatment;
            pdata->mana_paid = cost_mana( pchr->missilehandler, pchr->missilecost << 8, pprt->owner_ref );
            prt_deflected = pdata->mana_paid;
        }

        if ( prt_deflected )
        {
            // Treat the missile
            if ( treatment == MISSILE_DEFLECT )
            {
                // Deflect the incoming ray off the normal
                pdata->impulse.x -= 2.0f * pdata->dot * pdata->nrm.x;
                pdata->impulse.y -= 2.0f * pdata->dot * pdata->nrm.y;
                pdata->impulse.z -= 2.0f * pdata->dot * pdata->nrm.z;
            }
            else if ( treatment == MISSILE_REFLECT )
            {
                // Reflect it back in the direction it came
                pdata->impulse.x -= 2.0f * pprt->vel.x;
                pdata->impulse.y -= 2.0f * pprt->vel.y;
                pdata->impulse.z -= 2.0f * pprt->vel.z;

                // Change the owner of the missile
                pprt->team              = pchr->team;
                pprt->owner_ref         = GET_REF_PCHR( pchr );
                pdata->ppip->homing     = bfalse;
            }

            // Change the direction of the particle
            if ( pdata->ppip->rotatetoface )
            {
                // Turn to face new direction
                pprt->facing = vec_to_facing( pprt->vel.x , pprt->vel.y );
            }

            // Blocked!
            spawn_defense_ping( pchr, pprt->owner_ref );
            chr_make_text_billboard( GET_REF_PCHR( pchr ), "Blocked!", text_color, tint, lifetime, ( BIT_FIELD )bb_opt_all );

            // If the attack was blocked by a shield, then check if the block caused a knockback
            if ( chr_is_invictus && ACTION_IS_TYPE( pchr->mad_inst.action.which, P ) )
            {
                bool_t using_shield;
                CHR_REF item;

                // Figure out if we are really using a shield or if it is just a invictus frame
                using_shield = bfalse;

                // Check right hand for a shield
                item = pchr->holdingwhich[SLOT_RIGHT];
                if ( INGAME_CHR( item ) && pchr->ai.lastitemused == item )
                {
                    using_shield = btrue;
                }

                // Check left hand for a shield
                if ( !using_shield )
                {
                    item = pchr->holdingwhich[SLOT_LEFT];
                    if ( INGAME_CHR( item ) && pchr->ai.lastitemused == item )
                    {
                        using_shield = btrue;
                    }
                }

                // Now we have the block rating and know the enemy
                if ( INGAME_CHR( pprt->owner_ref ) && using_shield )
                {
                    ego_chr *pshield = ChrObjList.get_data_ptr( item );
                    ego_chr *pattacker = ChrObjList.get_data_ptr( pprt->owner_ref );
                    int total_block_rating;

                    // use the character block skill plus the base block rating of the shield and adjust for strength
                    total_block_rating = ego_chr_data::get_skill( pchr, MAKE_IDSZ( 'B', 'L', 'O', 'C' ) );
                    total_block_rating += ego_chr_data::get_skill( pshield, MAKE_IDSZ( 'B', 'L', 'O', 'C' ) );
                    total_block_rating -= SFP8_TO_SINT( pattacker->strength ) * 4;            //-4% per attacker strength
                    total_block_rating += SFP8_TO_SINT( pchr->strength )      * 2;            //+2% per defender strength

                    // Now determine the result of the block
                    if ( generate_randmask( 1, 100 ) <= total_block_rating )
                    {
                        // Defender won, the block holds
                        // Add a small stun to the attacker for about 0.8 seconds
                        pattacker->reloadtime += 40;
                    }
                    else
                    {
                        // Attacker broke the block and batters away the shield
                        // Time to raise shield again (about 0.8 seconds)
                        pchr->reloadtime += 40;
                        sound_play_chunk( pchr->pos, g_wavelist[GSND_SHIELDBLOCK] );
                    }
                }
            }

        }
    }

    return prt_deflected;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_recoil( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata )
{
    /// \author BB
    /// \details  make the character and particle recoil from the collision
    float prt_mass;
    float attack_factor;

    if ( NULL == pdata ) return 0;

    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;

    if ( !INGAME_PPRT( pprt ) ) return bfalse;

    if ( 0.0f == fvec3_length_abs( pdata->impulse.v ) ) return btrue;

    if ( !pdata->ppip->allowpush ) return bfalse;

    // do the reaction force of the particle on the character

    // determine how much the attack is "felt"
    attack_factor = 1.0f;
    if ( DAMAGE_CRUSH == pprt->damagetype )
    {
        // very blunt type of attack, the maximum effect
        attack_factor = 1.0f;
    }
    else if ( DAMAGE_POKE == pprt->damagetype )
    {
        // very focused type of attack, the minimum effect
        attack_factor = 0.5f;
    }
    else
    {
        // all other damage types are in the middle
        attack_factor = INV_SQRT_TWO;
    }

    prt_mass = 1.0f;
    if ( 0 == pdata->max_damage )
    {
        // this is a particle like the wind particles in the whirlwind
        // make the particle have some kind of predictable constant effect
        // relative to any character;
        prt_mass = pchr->phys.weight / 10.0f;
    }
    else
    {
        // determine an "effective mass" for the particle, based on its max damage
        // and velocity

        float prt_vel2;
        float prt_ke;

        // the damage is basically like the kinetic energy of the particle
        prt_vel2 = fvec3_dot_product( pdata->vdiff.v, pdata->vdiff.v );

        // It can happen that a damage particle can hit something
        // at almost zero velocity, which would make for a huge "effective mass".
        // by making a reasonable "minimum velocity", we limit the maximum mass to
        // something reasonable
        prt_vel2 = SDL_max( 100.0f, prt_vel2 );

        // get the "kinetic energy" from the damage
        prt_ke = 3.0f * pdata->max_damage;

        // the faster the particle is going, the smaller the "mass" it
        // needs to do the damage
        prt_mass = prt_ke / ( 0.5f * prt_vel2 );
    }

    // now, we have the particle's impulse and mass
    // Do the impulse to the object that was hit
    // If the particle was magically deflected, there is no rebound on the target
    if ( pchr->phys.weight != INFINITE_WEIGHT && !pdata->mana_paid )
    {
        float factor = attack_factor;

        if ( pchr->phys.weight > 0 )
        {
            // limit the prt_mass to be something relative to this object
            float loc_prt_mass = CLIP( prt_mass, 1.0f, 2.0f * pchr->phys.weight );

            factor *= loc_prt_mass / pchr->phys.weight;
        }

        // modify it by the the severity of the damage
        // reduces the damage below pdata->actual_damage == pchr->life_max
        // and it doubles it if pdata->actual_damage is really huge
        //factor *= 2.0f * ( float )pdata->actual_damage / ( float )( SDL_abs( pdata->actual_damage ) + pchr->life_max );

        factor = CLIP( factor, 0.0f, 3.0f );

        // calculate the "impulse"
        {
            fvec3_t _tmp_vec = fvec3_scale( pdata->impulse.v, -factor );

            phys_data_accumulate_avel( &( pchr->phys ), _tmp_vec.v );
        }
    }

    // if the particle is attached to a weapon, the particle can force the
    // weapon (actually, the weapon's holder) to rebound.
    if ( INGAME_CHR( pprt->attachedto_ref ) )
    {
        ego_chr * ptarget;
        CHR_REF iholder;

        ptarget = NULL;

        // transmit the force of the blow back to the character that is
        // holding the weapon

        iholder = ego_chr::get_lowest_attachment( pprt->attachedto_ref, bfalse );
        if ( INGAME_CHR( iholder ) )
        {
            ptarget = ChrObjList.get_data_ptr( iholder );
        }
        else
        {
            iholder = ego_chr::get_lowest_attachment( pprt->owner_ref, bfalse );
            if ( INGAME_CHR( iholder ) )
            {
                ptarget = ChrObjList.get_data_ptr( iholder );
            }
        }

        if ( ptarget->phys.weight != INFINITE_WEIGHT )
        {
            float factor = attack_factor;

            if ( ptarget->phys.weight > 0 )
            {
                // limit the prt_mass to be something relative to this object
                float loc_prt_mass = CLIP( prt_mass, 1.0f, 2.0f * ptarget->phys.weight );

                factor *= ( float ) loc_prt_mass / ( float )ptarget->phys.weight;
            }

            factor = CLIP( factor, 0.0f, 3.0f );

            // in the SAME direction as the particle
            {
                fvec3_t _tmp_vec = fvec3_scale( pdata->impulse.v, factor );

                phys_data_accumulate_avel( &( ptarget->phys ), _tmp_vec.v );
            }
        }
    }

    // apply the impulse to the particle velocity
    phys_data_accumulate_avel( &( pprt->phys ), pdata->impulse.v );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_damage( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata )
{
    ENC_REF enchant, ego_enc_next;
    bool_t prt_needs_impact;

    if ( NULL == pdata ) return bfalse;
    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;
    if ( !INGAME_PPRT( pprt ) ) return bfalse;

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pchr );

    // Check all enchants to see if they are removed
    enchant = pchr->firstenchant;
    while ( enchant != MAX_ENC )
    {
        ego_enc_next = EncObjList.get_data_ref( enchant ).nextenchant_ref;
        if ( ego_enc::is_removed( enchant, pprt->profile_ref ) )
        {
            remove_enchant( enchant, NULL );
        }
        enchant = ego_enc_next;
    }

    // Do confuse effects
    if ( pdata->ppip->grogtime > 0 && pdata->pcap->canbegrogged )
    {
        ADD_BITS( pchr->ai.alert, ALERTIF_CONFUSED );
        if ( pdata->ppip->grogtime > pchr->grogtime )
        {
            pchr->grogtime = SDL_max( 0, pchr->grogtime + pdata->ppip->grogtime );
        }
    }
    if ( pdata->ppip->dazetime > 0 && pdata->pcap->canbedazed )
    {
        ADD_BITS( pchr->ai.alert, ALERTIF_CONFUSED );
        if ( pdata->ppip->dazetime > pchr->dazetime )
        {
            pchr->dazetime = SDL_max( 0, pchr->dazetime + pdata->ppip->dazetime );
        }
    }

    //---- Damage the character, if necessary
    if (( pprt->damage.base + pprt->damage.base ) != 0 )
    {
        prt_needs_impact = pdata->ppip->rotatetoface || INGAME_CHR( pprt->attachedto_ref );
        if ( INGAME_CHR( pprt->owner_ref ) )
        {
            ego_chr * powner = ChrObjList.get_data_ptr( pprt->owner_ref );
            ego_cap * powner_cap = pro_get_pcap( powner->profile_ref );

            if ( powner_cap->isranged ) prt_needs_impact = btrue;
        }

        // DAMFX_ARRO means that it only does damage to the one it's attached to
        if ( HAS_NO_BITS( pdata->ppip->damfx, DAMFX_ARRO ) && !( prt_needs_impact && !( pdata->dot < 0.0f ) ) )
        {
            FACING_T direction;
            IPair loc_damage = pprt->damage;

            direction = vec_to_facing( pprt->vel.x , pprt->vel.y );
            direction = pchr->ori.facing_z - direction + ATK_BEHIND;

            // These things only apply if the particle has an owner
            if ( INGAME_CHR( pprt->owner_ref ) )
            {
                CHR_REF item;
                int drain;
                ego_chr * powner = ChrObjList.get_data_ptr( pprt->owner_ref );

                // Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penalty)
                // +2% bonus for every point of intelligence and/or wisdom above 14. Below 14 gives -2% instead!
                if ( pdata->ppip->intdamagebonus )
                {
                    float percent;
                    percent = (( SFP8_TO_SINT( powner->intelligence ) ) - 14 ) * 2;
                    percent /= 100;
                    loc_damage.base *= 1.00f + percent;
                    loc_damage.rand *= 1.00f + percent;
                }

                if ( pdata->ppip->wisdamagebonus )
                {
                    float percent;
                    percent = ( SFP8_TO_SINT( powner->wisdom ) - 14 ) * 2;
                    percent /= 100;
                    loc_damage.base *= 1.00f + percent;
                    loc_damage.rand *= 1.00f + percent;
                }

                // Steal some life
                if ( pprt->lifedrain > 0 )
                {
                    drain = pchr->life;
                    pchr->life = CLIP( pchr->life, 1, pchr->life - pprt->lifedrain );
                    drain -= pchr->life;
                    powner->life = SDL_min( powner->life + drain, powner->life_max );
                }

                // Steal some mana
                if ( pprt->manadrain > 0 )
                {
                    drain = pchr->mana;
                    pchr->mana = CLIP( pchr->mana, 0, pchr->mana - pprt->manadrain );
                    drain -= pchr->mana;
                    powner->mana = SDL_min( powner->mana + drain, powner->mana_max );
                }

                // Notify the attacker of a scored hit
                ADD_BITS( powner->ai.alert, ALERTIF_SCOREDAHIT );
                powner->ai.hitlast = GET_REF_PCHR( pchr );

                // Tell the weapons who the attacker hit last
                item = powner->holdingwhich[SLOT_LEFT];
                if ( INGAME_CHR( item ) )
                {
                    ChrObjList.get_data_ref( item ).ai.hitlast = GET_REF_PCHR( pchr );
                }

                item = powner->holdingwhich[SLOT_RIGHT];
                if ( INGAME_CHR( item ) )
                {
                    ChrObjList.get_data_ref( item ).ai.hitlast = GET_REF_PCHR( pchr );
                }
            }

            // handle vulnerabilities, double the damage
            if ( ego_chr::has_vulnie( GET_REF_PCHR( pchr ), pprt->profile_ref ) )
            {
                // Double the damage
                loc_damage.base = ( loc_damage.base << 1 );
                loc_damage.rand = ( loc_damage.rand << 1 ) | 1;

                ADD_BITS( pchr->ai.alert, ALERTIF_HITVULNERABLE );
            }

            // Damage the character
            pdata->actual_damage = damage_character( GET_REF_PCHR( pchr ), direction, loc_damage, pprt->damagetype, pprt->team, pprt->owner_ref, pdata->ppip->damfx, bfalse );
        }
    }

    //---- estimate the impulse on the particle
    if ( pdata->dot < 0.0f )
    {
        if ( 0 == SDL_abs( pdata->max_damage ) || ( SDL_abs( pdata->max_damage ) - SDL_abs( pdata->actual_damage ) ) == 0 )
        {
            // the simple case
            pdata->impulse.x -= pprt->vel.x;
            pdata->impulse.y -= pprt->vel.y;
            pdata->impulse.z -= pprt->vel.z;
        }
        else
        {
            float recoil;
            fvec3_t vfinal, impulse_tmp;

            vfinal.x = pprt->vel.x - 2 * pdata->dot * pdata->nrm.x;
            vfinal.y = pprt->vel.y - 2 * pdata->dot * pdata->nrm.y;
            vfinal.z = pprt->vel.z - 2 * pdata->dot * pdata->nrm.z;

            // assume that the particle damage is like the kinetic energy,
            // then vfinal must be scaled by recoil^2
            recoil = ( float )SDL_abs( SDL_abs( pdata->max_damage ) - SDL_abs( pdata->actual_damage ) ) / ( float )SDL_abs( pdata->max_damage );

            vfinal.x *= recoil * recoil;
            vfinal.y *= recoil * recoil;
            vfinal.z *= recoil * recoil;

            impulse_tmp = fvec3_sub( vfinal.v, pprt->vel.v );

            pdata->impulse.x += impulse_tmp.x;
            pdata->impulse.y += impulse_tmp.y;
            pdata->impulse.z += impulse_tmp.z;
        }

    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_bump( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata )
{
    bool_t prt_belongs_to_chr;
    bool_t prt_hates_chr, prt_attacks_chr, prt_hateonly;
    bool_t valid_onlydamagefriendly;
    bool_t valid_friendlyfire;
    bool_t valid_onlydamagehate;

    if ( NULL == pdata ) return bfalse;
    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;
    if ( !INGAME_PPRT( pprt ) ) return bfalse;

    // if the particle was deflected, then it can't bump the character
    if ( IS_INVICTUS_PCHR_RAW( pchr ) || pprt->attachedto_ref == GET_REF_PCHR( pchr ) ) return bfalse;

    prt_belongs_to_chr = ( GET_REF_PCHR( pchr ) == pprt->owner_ref );

    if ( !prt_belongs_to_chr )
    {
        // no simple owner relationship. Check for something deeper.
        CHR_REF prt_owner = ego_prt::get_iowner( GET_REF_PPRT( pprt ), 0 );
        if ( INGAME_CHR( prt_owner ) )
        {
            CHR_REF chr_wielder = ego_chr::get_lowest_attachment( GET_REF_PCHR( pchr ), btrue );
            CHR_REF prt_wielder = ego_chr::get_lowest_attachment( prt_owner, btrue );

            if ( !INGAME_CHR( chr_wielder ) ) chr_wielder = GET_REF_PCHR( pchr );
            if ( !INGAME_CHR( prt_wielder ) ) prt_wielder = prt_owner;

            prt_belongs_to_chr = ( chr_wielder == prt_wielder );
        }
    }

    // does the particle team hate the character's team
    prt_hates_chr = team_hates_team( pprt->team, pchr->team );

    // Only bump into hated characters?
    prt_hateonly = PipStack[pprt->pip_ref].hateonly;
    valid_onlydamagehate = prt_hates_chr && PipStack[pprt->pip_ref].hateonly;

    // allow neutral particles to attack anything
    prt_attacks_chr = prt_hates_chr || (( TEAM_NULL != pchr->team ) && ( TEAM_NULL == pprt->team ) );

    // this is the onlydamagefriendly condition from the particle search code
    valid_onlydamagefriendly = ( pdata->ppip->onlydamagefriendly && pprt->team == pchr->team ) ||
                               ( !pdata->ppip->onlydamagefriendly && prt_attacks_chr );

    // I guess "friendly fire" does not mean "self fire", which is a bit unfortunate.
    valid_friendlyfire = ( pdata->ppip->friendlyfire && !prt_hates_chr && !prt_belongs_to_chr ) ||
                         ( !pdata->ppip->friendlyfire && prt_attacks_chr );

    pdata->prt_bumps_chr =  valid_friendlyfire || valid_onlydamagefriendly || valid_onlydamagehate;

    return pdata->prt_bumps_chr;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_handle_bump( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata )
{
    if ( NULL == pdata || !pdata->prt_bumps_chr ) return bfalse;

    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;
    if ( !INGAME_PPRT( pprt ) ) return bfalse;

    if ( !pdata->prt_bumps_chr ) return bfalse;

    // Catch on fire
    spawn_bump_particles( GET_REF_PCHR( pchr ), GET_REF_PPRT( pprt ) );

    // handle some special particle interactions
    if ( pdata->ppip->end_bump )
    {
        if ( pdata->ppip->bump_money )
        {
            ego_chr * pcollector = pchr;

            // Let mounts collect money for their riders
            if ( pchr->ismount && INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
            {
                pcollector = ChrObjList.get_data_ptr( pchr->holdingwhich[SLOT_LEFT] );

                // if the mount's rider can't get money, the mount gets to keep the money!
                if ( !pcollector->cangrabmoney )
                {
                    pcollector = pchr;
                }
            }

            if ( pcollector->cangrabmoney && pcollector->alive && 0 == pcollector->damagetime && pcollector->money < MAXMONEY )
            {
                pcollector->money += pdata->ppip->bump_money;
                pcollector->money = CLIP( pcollector->money, 0, MAXMONEY );

                // the coin disappears when you pick it up
                pdata->terminate_particle = btrue;
            }
        }
        else
        {
            // Only hit one character, not several
            pdata->terminate_particle = btrue;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_init( ego_chr * pchr, ego_prt * pprt, ego_chr_prt_collsion_data * pdata )
{
    /// must use bump_padded here since that is the range for character-particle interactions

    bool_t full_collision;

    if ( NULL == pdata ) return bfalse;

    SDL_memset( pdata, 0, sizeof( *pdata ) );

    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;
    if ( !INGAME_PPRT( pprt ) ) return bfalse;

    // initialize the collision data
    pdata->pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pdata->pcap ) return bfalse;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    pdata->ppip = PipStack + pprt->pip_ref;

    // measure the collision depth
    full_collision = get_depth_1( pchr->chr_min_cv, pchr->pos, pprt->bump_padded, ego_prt::get_pos( pprt ), btrue, pdata->odepth );

    //// measure the collision depth in the last update
    //// the objects were not touching last frame, so they must have collided this frame
    //collision = !get_depth_close_1( pchr_a->chr_min_cv, pchr_a->pos_old, pprt_b->bump, pprt_b->pos_old, btrue, odepth_old );

    // estimate the maximum possible "damage" from this particle
    // other effects can magnify this number, like vulnerabilities
    // or DAMFX_* bits
    pdata->max_damage = SDL_abs( pprt->damage.base ) + SDL_abs( pprt->damage.rand );

    return full_collision;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision( collision_node * d )
{
    /// \author BB
    /// \details  this function goes through all of the steps to handle character-particle
    ///               interactions. A basic interaction has been detected. This needs to be refined
    ///               and then handled. The function returns bfalse if the basic interaction was wrong
    ///               or if the interaction had no effect.
    ///
    /// \note This function is a little more complicated than the character-character case because
    ///       of the friend-foe logic as well as the damage and other special effects that particles can do.

    bool_t retval = bfalse;

    CHR_REF ichr_a;
    PRT_REF iprt_b;

    ego_chr * pchr_a;
    ego_prt * pprt_b;

    bool_t prt_deflected;
    bool_t prt_can_hit_chr;

    bool_t plat_collision, full_collision;

    ego_chr_prt_collsion_data cn_lst;

    if ( NULL == d || !d->is_chr_prt() ) return bfalse;
    ichr_a = d->src.get_chr();
    iprt_b = d->dst.get_prt();

    // make sure that it is on
    pchr_a = ChrObjList.get_allocated_data_ptr( ichr_a );
    if ( !INGAME_PCHR( pchr_a ) ) return bfalse;

    // skip dead objects
    if ( !pchr_a->alive ) return bfalse;

    // skip objects that are inside inventories
    if ( pchr_a->pack.is_packed ) return bfalse;

    if ( !INGAME_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtObjList.get_data_ptr( iprt_b );

    if ( ichr_a == pprt_b->attachedto_ref ) return bfalse;

    // detect a full collision
    full_collision = do_chr_prt_collision_init( pchr_a, pprt_b, &cn_lst );

    // platform interaction. we can still have a platform interaction even if there
    // is not a "full_collision" since the z-distance these
    plat_collision = bfalse;
    if ( pchr_a->platform && !INGAME_CHR( pprt_b->attachedto_ref ) )
    {
        plat_collision = do_prt_platform_physics( pprt_b, pchr_a, &cn_lst );
    }

    // if there is no collision, no point in going farther
    if ( !full_collision && !plat_collision ) return bfalse;

    // estimate the "normal" for the collision, using the center-of-mass difference
    // put this off until this point to reduce calling this "expensive" function
    cn_lst.dot = estimate_chr_prt_normal( pchr_a, pprt_b, cn_lst.nrm.v, cn_lst.vdiff.v );

    // handle particle deflection.
    // if the platform collision was already handled, there is nothing left to do
    prt_deflected = bfalse;
    //if ( full_collision && !plat_collision )
    {
        // determine whether the particle is deflected by the character
        if ( cn_lst.dot < 0.0f )
        {
            prt_deflected = do_chr_prt_collision_deflect( pchr_a, pprt_b, &cn_lst );
            if ( prt_deflected )
            {
                retval = btrue;
            }
        }
    }

    // do "damage" to the character
    if ( !prt_deflected && 0 == pchr_a->damagetime )
    {
        // Check reaffirmation of particles
        if ( pchr_a->reaffirmdamagetype == pprt_b->damagetype )
        {
            ego_cap *pcap_a = ego_chr::get_pcap( pchr_a->ai.index );

            // This prevents books in shops from being burned
            if ( pchr_a->isshopitem && pcap_a->spelleffect_type != NO_SKIN_OVERRIDE )
            {
                retval = ( 0 != reaffirm_attached_particles( ichr_a ) );
            }
        }

        // refine the logic for a particle to hit a character
        prt_can_hit_chr = do_chr_prt_collision_bump( pchr_a, pprt_b, &cn_lst );

        // does the particle damage/heal the character?
        if ( prt_can_hit_chr )
        {
            // we can't even get to this point if the character is completely invulnerable (invictus)
            // or can't be damaged this round
            cn_lst.prt_damages_chr = do_chr_prt_collision_damage( pchr_a, pprt_b, &cn_lst );
            if ( cn_lst.prt_damages_chr )
            {
                retval = btrue;
            }
        }
    }

    // make the character and particle recoil from the collision
    if ( fvec3_length_abs( cn_lst.impulse.v ) > 0.0f )
    {
        if ( do_chr_prt_collision_recoil( pchr_a, pprt_b, &cn_lst ) )
        {
            retval = btrue;
        }
    }

    // handle a couple of special cases
    if ( cn_lst.prt_bumps_chr )
    {
        if ( do_chr_prt_collision_handle_bump( pchr_a, pprt_b, &cn_lst ) )
        {
            retval = btrue;
        }
    }

    // terminate the particle if needed
    if ( cn_lst.terminate_particle )
    {
        ego_obj_prt::request_terminate( iprt_b );
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t update_chr_platform_attachment( ego_chr * pchr )
{
    ego_chr * pplat;

    if ( !DEFINED_PCHR( pchr ) || !INGAME_CHR( pchr->onwhichplatform_ref ) ) return bfalse;

    pplat = ChrObjList.get_data_ptr( pchr->onwhichplatform_ref );

    // add the weight to the platform based on the new zlerp
    if ( pchr->enviro.walk_lerp < pchr->enviro.grid_lerp )
    {
        pchr->onwhichplatform_weight = pchr->phys.weight * ( 1.0f - pchr->enviro.walk_lerp );
        pplat->holdingweight        += pchr->onwhichplatform_weight;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void update_all_chr_platform_attachments()
{
    CHR_BEGIN_LOOP_PROCESSING( cnt, pchr )
    {
        update_chr_platform_attachment( pchr );
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
ego_bundle_prt & update_prt_platform_attachment( ego_bundle_prt & bdl )
{
    ego_chr * pplat;
    ego_prt * loc_pprt;

    loc_pprt = bdl;
    if ( NULL == loc_pprt ) return bdl;

    if ( !INGAME_CHR( loc_pprt->onwhichplatform_ref ) ) return bdl;

    pplat = ChrObjList.get_data_ptr( loc_pprt->onwhichplatform_ref );

    return bdl;
}

//--------------------------------------------------------------------------------------------
void update_all_prt_platform_attachments()
{
    PRT_BEGIN_LOOP_ALLOCATED_BDL( cnt, bdl )
    {
        update_prt_platform_attachment( bdl );
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void update_all_platform_attachments()
{
    /// \author BB
    /// \details do all the platform-related stuff that must be done every update

    update_all_chr_platform_attachments();
    update_all_prt_platform_attachments();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t calc_grip_cv( ego_chr * pmount, int grip_offset, ego_oct_bb   * grip_cv_ptr, fvec3_base_t grip_origin_ary, fvec3_base_t grip_up_ary )
{
    /// \author BB
    /// \details  use a standard size for the grip

    // take the character size from the adventurer model
    const float default_chr_height = 88.0f;
    const float default_chr_radius = 22.0f;

    gfx_mad_instance * pmount_inst;

    int       grip_count;
    Uint16    grip_verts[GRIP_VERTS];

    int       vertex, cnt;
    ego_oct_bb tmp_cv;

    if ( NULL == grip_cv_ptr || !DEFINED_PCHR( pmount ) ) return bfalse;

    // alias this variable for notation simplicity
    pmount_inst = &( pmount->gfx_inst );

    // tune the grip radius
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        tmp_cv.mins[cnt] = -default_chr_radius * pmount->fat * 0.5f;
        tmp_cv.maxs[cnt] =  default_chr_radius * pmount->fat * 0.5f;
    }

    // tune the vertical extent of the grip so that a character can mount
    // a chocobone and hide in a pot
    tmp_cv.mins[OCT_Z] = -default_chr_height * pmount->fat * 1.5f;
    tmp_cv.maxs[OCT_Z] =  default_chr_height * pmount->fat * 0.5f;

    // make the grip into a cylinder-like shape
    tmp_cv.mins[OCT_XY] *= SQRT_TWO;
    tmp_cv.maxs[OCT_XY] *= SQRT_TWO;
    tmp_cv.mins[OCT_YX] *= SQRT_TWO;
    tmp_cv.maxs[OCT_YX] *= SQRT_TWO;

    // do the automatic vertex update
    vertex = ego_sint( pmount_inst->vrt_count ) - ego_sint( grip_offset );
    vertex = SDL_max( 0, vertex );
    gfx_mad_instance::update_vertices( pmount_inst, pmount->mad_inst.state, gfx_range( vertex, vertex + grip_offset ), bfalse );

    // calculate the grip vertices
    for ( grip_count = 0, cnt = 0; cnt < GRIP_VERTS && ( size_t )( vertex + cnt ) < pmount_inst->vrt_count; grip_count++, cnt++ )
    {
        grip_verts[cnt] = vertex + cnt;
    }
    for ( /* nothing */ ; cnt < GRIP_VERTS; cnt++ )
    {
        grip_verts[cnt] = 0xFFFF;
    }

    // assume the worst
    fvec3_self_clear( grip_origin_ary );

    // calculate grip_origin_ary and grip_up_ary
    if ( 4 == grip_count )
    {
        fvec4_t grip_points[GRIP_VERTS], grip_nupoints[GRIP_VERTS];
        fvec3_t grip_vecs[3];

        // Calculate grip point locations with linear interpolation and other silly things
        convert_grip_to_local_points( pmount, grip_verts, grip_points );

        // Do the transform the vertices for the grip's "up" vector
        TransformVertices( &( pmount_inst->matrix ), grip_points, grip_nupoints, GRIP_VERTS );

        // determine the grip vectors
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            grip_vecs[cnt] = fvec3_sub( grip_nupoints[cnt + 1].v, grip_nupoints[0].v );
        }

        // grab the grip's "up" vector
        fvec3_base_assign( grip_up_ary, fvec3_normalize( grip_vecs[2].v ) );

        // grab the origin of the grip
        fvec3_base_copy( grip_origin_ary, grip_nupoints[0].v );
    }
    else if ( 0 == grip_count )
    {
        // grab the grip's "up" vector
        grip_up_ary[kX] = grip_up_ary[kY] = 0.0f;
        grip_up_ary[kZ] = -SGN( gravity );

        // grab the origin of the grip
        fvec3_base_assign( grip_origin_ary, ego_chr::get_pos( pmount ) );
    }
    else if ( grip_count > 0 )
    {
        fvec4_t grip_points[GRIP_VERTS], grip_nupoints[GRIP_VERTS];

        // Calculate grip point locations with linear interpolation and other silly things
        convert_grip_to_local_points( pmount, grip_verts, grip_points );

        // Do the transform the vertices for the grip's origin
        TransformVertices( &( pmount_inst->matrix ), grip_points, grip_nupoints, 1 );

        // grab the grip's "up" vector
        grip_up_ary[kX] = grip_up_ary[kY] = 0.0f;
        grip_up_ary[kZ] = -SGN( gravity );

        // grab the origin of the grip
        fvec3_base_copy( grip_origin_ary, grip_nupoints[0].v );
    }

    // add in the "origin" of the grip
    ego_oct_bb::add_vector( tmp_cv, grip_origin_ary, *grip_cv_ptr );

    return btrue;
}

bool collision_node_greater::operator()
(
    const collision_node& _Left,
    const collision_node& _Right
) const
{
    int   itmp;
    float ftmp;

    // sort by initial time first
    ftmp = _Left.tmin - _Right.tmin;
    if ( ftmp <= 0.0f ) return false;
    else if ( ftmp >= 0.0f ) return true;

    // fort by final time second
    ftmp = _Left.tmax - _Right.tmax;
    if ( ftmp <= 0.0f ) return false;
    else if ( ftmp >= 0.0f ) return true;

    itmp = ego_sint( _Left.src.get_chr().get_value() ) - ego_sint( _Right.src.get_chr().get_value() );
    if ( 0 != itmp ) return itmp > 0;

    itmp = ego_sint( _Left.src.get_prt().get_value() ) - ego_sint( _Right.src.get_prt().get_value() );
    if ( 0 != itmp ) return itmp > 0;

    itmp = ego_sint( _Left.src.get_chr().get_value() ) - ego_sint( _Right.src.get_chr().get_value() );
    if ( 0 != itmp ) return itmp > 0;

    itmp = ego_sint( _Left.dst.get_prt().get_value() ) - ego_sint( _Right.dst.get_prt().get_value() );
    if ( 0 != itmp ) return itmp > 0;

    itmp = ego_sint( _Left.dst.get_chr().get_value() ) - ego_sint( _Right.dst.get_chr().get_value() );
    if ( 0 != itmp ) return itmp > 0;

    itmp = ego_sint( _Left.src.get_tile() ) - ego_sint( _Right.dst.get_tile() );
    if ( 0 != itmp ) return itmp > 0;

    return false;
}

//--------------------------------------------------------------------------------------------
//bool_t add_chr_chr_interaction( collision_hash_list * pchlst, const CHR_REF & ichr_a, const CHR_REF & ichr_b, collision_node_ary * pcn_lst, hash_node_ary * phn_lst )
//{
//    Uint32 hashval = 0;
//    int count;
//    bool_t found;
//
//    ego_hash_node * n;
//    collision_node    * d;
//
//    if ( NULL == pchlst || NULL == pcn_lst || NULL == phn_lst ) return bfalse;
//
//    // there is no situation in the game where we allow characters to interact with themselves
//    if ( ichr_a == ichr_b ) return bfalse;
//
//    // create a hash that is order-independent
//    hashval = MAKE_HASH( (ichr_a ).get_value(), (ichr_b ).get_value() );
//
//    found = bfalse;
//    count = pchlst->subcount[hashval];
//    if ( count > 0 )
//    {
//        int i;
//
//        // this hash already exists. check to see if the binary collision exists, too
//        n = pchlst->sublist[hashval];
//        for ( i = 0; i < count; i++ )
//        {
//            d = ( collision_node * )( n->data );
//
//            // make sure to test both orders
//            if (( d->src.get_chr() == ichr_a && d->dst.get_chr() == ichr_b ) || ( d->src.get_chr() == ichr_b && d->dst.get_chr() == ichr_a ) )
//            {
//                found = btrue;
//                break;
//            }
//        }
//    }
//
//    // insert this collision
//    if ( !found )
//    {
//        // pick a free collision data
//        EGOBOO_ASSERT( collision_node_ary::get_top( pcn_lst ) < CHR_MAX_COLLISIONS );
//        d = collision_node_ary::pop_back( pcn_lst );
//
//        // fill it in
//        collision_node::ctor( d );
//        d->src.get_chr() = ichr_a;
//        d->dst.get_chr() = ichr_b;
//
//        // generate a new hash node
//        EGOBOO_ASSERT( hash_node_ary::get_top( phn_lst ) < COLLISION_HASH_NODES );
//        n = hash_node_ary::pop_back( phn_lst );
//
//        ego_hash_node::ctor_this( n, ( void* )d );
//
//        // insert the node
//        pchlst->subcount[hashval]++;
//        pchlst->sublist[hashval] = ego_hash_node::insert_before( pchlst->sublist[hashval], n );
//    }
//
//    return !found;
//}

//--------------------------------------------------------------------------------------------
//bool_t add_chr_prt_interaction( collision_hash_list * pchlst, const CHR_REF & ichr_a, const PRT_REF & iprt_b, collision_node_ary * pcn_lst, hash_node_ary * phn_lst )
//{
//    bool_t found;
//    int    count;
//    Uint32 hashval = 0;
//
//    ego_hash_node * n;
//    collision_node    * d;
//
//    if ( NULL == pchlst ) return bfalse;
//
//    // create a hash that is order-independent
//    hashval = MAKE_HASH( (ichr_a ).get_value(), (iprt_b ).get_value() );
//
//    found = bfalse;
//    count = pchlst->subcount[hashval];
//    if ( count > 0 )
//    {
//        int i ;
//
//        // this hash already exists. check to see if the binary collision exists, too
//        n = pchlst->sublist[hashval];
//        for ( i = 0; i < count; i++ )
//        {
//            d = ( collision_node * )( n->data );
//            if ( d->src.get_chr() == ichr_a && d->dst.get_prt() == iprt_b )
//            {
//                found = btrue;
//                break;
//            }
//        }
//    }
//
//    // insert this collision
//    if ( !found )
//    {
//        // pick a free collision data
//        EGOBOO_ASSERT( collision_node_ary::get_top( pcn_lst ) < CHR_MAX_COLLISIONS );
//        d = collision_node_ary::pop_back( pcn_lst );
//
//        // fill it in
//        collision_node::ctor( d );
//        d->src.get_chr() = ichr_a;
//        d->dst.get_prt() = iprt_b;
//
//        // generate a new hash node
//        EGOBOO_ASSERT( hash_node_ary::get_top( phn_lst ) < COLLISION_HASH_NODES );
//        n = hash_node_ary::pop_back( phn_lst );
//
//        ego_hash_node::ctor_this( n, ( void* )d );
//
//        // insert the node
//        pchlst->subcount[hashval]++;
//        pchlst->sublist[hashval] = ego_hash_node::insert_before( pchlst->sublist[hashval], n );
//    }
//
//    return !found;
//}

////--------------------------------------------------------------------------------------------
//bool_t do_mounts( const CHR_REF & ichr_a, const CHR_REF & ichr_b )
//{
//    float xa, ya, za;
//    float xb, yb, zb;
//
//    ego_chr * pchr_a, * pchr_b;
//    ego_cap * pcap_a, * pcap_b;
//
//    bool_t mount_a, mount_b;
//    float  dx, dy, dist;
//    float  depth_z;
//
//    bool_t collide_x  = bfalse;
//    bool_t collide_y  = bfalse;
//    bool_t collide_xy = bfalse;
//
//    bool_t mounted;
//
//    // make sure that A is valid
//    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
//    pchr_a = ChrObjList.get_data_ptr(ichr_a);
//
//    pcap_a = ego_chr::get_pcap( ichr_a );
//    if ( NULL == pcap_a ) return bfalse;
//
//    // make sure that B is valid
//    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
//    pchr_b = ChrObjList.get_data_ptr(ichr_b);
//
//    pcap_b = ego_chr::get_pcap( ichr_b );
//    if ( NULL == pcap_b ) return bfalse;
//
//    // can either of these objects mount the other?
//    mount_a = chr_can_mount( ichr_b, ichr_a );
//    mount_b = chr_can_mount( ichr_a, ichr_b );
//
//    if ( !mount_a && !mount_b ) return bfalse;
//
//    // Ready for position calulations
//    xa = pchr_a->pos.x;
//    ya = pchr_a->pos.y;
//    za = pchr_a->pos.z;
//
//    xb = pchr_b->pos.x;
//    yb = pchr_b->pos.y;
//    zb = pchr_b->pos.z;
//
//    mounted = bfalse;
//    if ( !mounted && mount_b && ( pchr_a->vel.z - pchr_b->vel.z ) < 0 )
//    {
//        // A falling on B?
//        fvec4_t   point[1], nupoint[1];
//
//        // determine the actual location of the mount point
//        {
//            int vertex;
//            gfx_mad_instance * pinst = &( pchr_b->inst );
//
//            vertex = ego_sint(pinst->vrt_count ) - ego_sint(GRIP_LEFT);
//
//            // do the automatic update
//            gfx_mad_instance::update_vertices( pinst, vertex, vertex, bfalse );
//
//            // Calculate grip point locations with linear interpolation and other silly things
//            point[0].x = pinst->vrt_lst[vertex].pos[XX];
//            point[0].y = pinst->vrt_lst[vertex].pos[YY];
//            point[0].z = pinst->vrt_lst[vertex].pos[ZZ];
//            point[0].w = 1.0f;
//
//            // Do the transform
//            TransformVertices( &( pinst->matrix ), point, nupoint, 1 );
//        }
//
//        dx = SDL_abs( xa - nupoint[0].x );
//        dy = SDL_abs( ya - nupoint[0].y );
//        dist = dx + dy;
//        depth_z = za - nupoint[0].z;
//
//        if ( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
//        {
//            // estimate the collisions this frame
//            collide_x  = ( dx <= pchr_a->bump.size * 2 );
//            collide_y  = ( dy <= pchr_a->bump.size * 2 );
//            collide_xy = ( dist <= pchr_a->bump.size_big * 2 );
//
//            if ( collide_x && collide_y && collide_xy )
//            {
//                attach_character_to_mount( ichr_a, ichr_b, GRIP_ONLY );
//                mounted = INGAME_CHR( pchr_a->attachedto );
//            }
//        }
//    }
//
//    if ( !mounted && mount_a && ( pchr_b->vel.z - pchr_a->vel.z ) < 0 )
//    {
//        // B falling on A?
//
//        fvec4_t   point[1], nupoint[1];
//
//        // determine the actual location of the mount point
//        {
//            int vertex;
//            gfx_mad_instance * pinst = &( pchr_a->inst );
//
//            vertex = ego_sint(pinst->vrt_count ) - ego_sint(GRIP_LEFT);
//
//            // do the automatic update
//            gfx_mad_instance::update_vertices( pinst, vertex, vertex, bfalse );
//
//            // Calculate grip point locations with linear interpolation and other silly things
//            point[0].x = pinst->vrt_lst[vertex].pos[XX];
//            point[0].y = pinst->vrt_lst[vertex].pos[YY];
//            point[0].z = pinst->vrt_lst[vertex].pos[ZZ];
//            point[0].w = 1.0f;
//
//            // Do the transform
//            TransformVertices( &( pinst->matrix ), point, nupoint, 1 );
//        }
//
//        dx = SDL_abs( xb - nupoint[0].x );
//        dy = SDL_abs( yb - nupoint[0].y );
//        dist = dx + dy;
//        depth_z = zb - nupoint[0].z;
//
//        if ( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
//        {
//            // estimate the collisions this frame
//            collide_x  = ( dx <= pchr_b->bump.size * 2 );
//            collide_y  = ( dy <= pchr_b->bump.size * 2 );
//            collide_xy = ( dist <= pchr_b->bump.size_big * 2 );
//
//            if ( collide_x && collide_y && collide_xy )
//            {
//                attach_character_to_mount( ichr_b, ichr_a, GRIP_ONLY );
//                mounted = INGAME_CHR( pchr_a->attachedto );
//            }
//        }
//    }
//
//    return mounted;
//}
