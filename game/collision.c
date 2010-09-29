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

/// @file collision.c
/// @brief The code that handles collisions between in-game objects
/// @details

#include "collision.h"

#include "log.h"
#include "hash.h"
#include "game.h"
#include "SDL_extensions.h"
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
struct s_chr_prt_collsion_data
{
    // object parameters
    cap_t * pcap;
    pip_t * ppip;

    // collision parameters
    oct_vec_t odepth;
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

typedef struct s_chr_prt_collsion_data chr_prt_collsion_data_t;

//--------------------------------------------------------------------------------------------

/// one element of the data for partitioning character and particle positions
struct s_bumplist
{
    size_t   chrnum;                  // Number on the block
    CHR_REF  chr;                     // For character collisions

    size_t   prtnum;                  // Number on the block
    CHR_REF  prt;                     // For particle collisions
};
typedef struct s_bumplist bumplist_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t add_chr_chr_interaction( CHashList_t * pclst, const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst );
static bool_t add_chr_prt_interaction( CHashList_t * pclst, const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst );

static bool_t detect_chr_chr_interaction_valid( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b );
static bool_t detect_chr_prt_interaction_valid( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b );

//static bool_t detect_chr_chr_interaction( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b );
//static bool_t detect_chr_prt_interaction( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b );

static bool_t do_chr_platform_detection( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b );
static bool_t do_prt_platform_detection( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b );

static bool_t attach_chr_to_platform( chr_t * pchr, chr_t * pplat );
static bool_t attach_prt_to_platform( prt_t * pprt, chr_t * pplat );

static bool_t fill_interaction_list( CHashList_t * pclst, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst );
static bool_t fill_bumplists( obj_BSP_t * pbsp );

static egoboo_rv bump_prepare( obj_BSP_t * pbsp );
static void      bump_begin();
static bool_t    bump_all_platforms( CoNode_ary_t * pcn_ary );
static bool_t    bump_all_mounts( CoNode_ary_t * pcn_ary );
static bool_t    bump_all_collisions( CoNode_ary_t * pcn_ary );
static void      bump_end();

static bool_t do_mounts( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b );
static egoboo_rv do_chr_platform_physics( CoNode_t * d, chr_bundle_t *pbdl_item, chr_bundle_t *pbdl_plat );
static float  estimate_chr_prt_normal( chr_t * pchr, prt_t * pprt, fvec3_base_t nrm, fvec3_base_t vdiff );
static bool_t do_chr_chr_collision( CoNode_t * d );

static bool_t do_chr_chr_collision_interaction( CoNode_t * d, chr_bundle_t *pbdl_a, chr_bundle_t *pbdl_b );

static bool_t do_chr_prt_collision_deflect( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_recoil( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_damage( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_handle_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_init( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision( CoNode_t * d );

static bool_t update_chr_platform_attachment( chr_t * pchr );
static prt_bundle_t * update_prt_platform_attachment( prt_bundle_t * pbdl );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_ARY( CoNode_ary,   CoNode_t );
IMPLEMENT_DYNAMIC_ARY( HashNode_ary, hash_node_t );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//static bumplist_t bumplist[MAXMESHFAN/16];

static CHashList_t   * _CHashList_ptr = NULL;
static HashNode_ary_t  _hn_ary;                 ///< the available hash_node_t collision nodes for the CHashList_t
static CoNode_ary_t    _co_ary;                 ///< the available CoNode_t    data pointed to by the hash_node_t nodes
static BSP_leaf_pary_t _coll_leaf_lst;
static CoNode_ary_t    _coll_node_lst;

static bool_t _collision_hash_initialized = bfalse;
static bool_t _collision_system_initialized = bfalse;

int CHashList_inserted = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t collision_system_begin()
{
    if ( !_collision_system_initialized )
    {
        if ( NULL == CoNode_ary_ctor( &_co_ary, CHR_MAX_COLLISIONS ) ) goto collision_system_begin_fail;

        if ( NULL == HashNode_ary_ctor( &_hn_ary, COLLISION_HASH_NODES ) ) goto collision_system_begin_fail;

        if ( NULL == BSP_leaf_pary_ctor( &_coll_leaf_lst, COLLISION_LIST_SIZE ) ) goto collision_system_begin_fail;

        if ( NULL == CoNode_ary_ctor( &_coll_node_lst, COLLISION_LIST_SIZE ) ) goto collision_system_begin_fail;

        _collision_system_initialized = btrue;
    }

    return btrue;

collision_system_begin_fail:

    CoNode_ary_dtor( &_co_ary );
    HashNode_ary_dtor( &_hn_ary );
    BSP_leaf_pary_dtor( &_coll_leaf_lst );
    CoNode_ary_dtor( &_coll_node_lst );

    _collision_system_initialized = bfalse;

    log_error( "Cannot initialize the collision system" );

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void collision_system_end()
{
    if ( _collision_hash_initialized )
    {
        hash_list_destroy( &_CHashList_ptr );
        _collision_hash_initialized = bfalse;
    }
    _CHashList_ptr = NULL;

    if ( _collision_system_initialized )
    {
        CoNode_ary_dtor( &_co_ary );
        HashNode_ary_dtor( &_hn_ary );
        BSP_leaf_pary_dtor( &_coll_leaf_lst );
        CoNode_ary_dtor( &_coll_node_lst );

        _collision_system_initialized = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CoNode_t * CoNode_ctor( CoNode_t * n )
{
    if ( NULL == n ) return n;

    // clear all data
    memset( n, 0, sizeof( *n ) );

    // the "colliding" objects
    n->chra = ( CHR_REF )MAX_CHR;
    n->prta = TOTAL_MAX_PRT;

    // the "collided with" objects
    n->chrb  = ( CHR_REF )MAX_CHR;
    n->prtb  = TOTAL_MAX_PRT;
    n->tileb = FANOFF;

    // initialize the time
    n->tmin = n->tmax = -1.0f;

    return n;
}

//--------------------------------------------------------------------------------------------
Uint8 CoNode_generate_hash( CoNode_t * coll )
{
    REF_T AA, BB;

    AA = ( Uint32 )( ~(( Uint32 )0 ) );
    if ( INGAME_CHR( coll->chra ) )
    {
        AA = REF_TO_INT( coll->chra );
    }
    else if ( INGAME_PRT( coll->prta ) )
    {
        AA = REF_TO_INT( coll->prta );
    }

    BB = ( Uint32 )( ~(( Uint32 )0 ) );
    if ( INGAME_CHR( coll->chrb ) )
    {
        BB = REF_TO_INT( coll->chrb );
    }
    else if ( INGAME_PRT( coll->prtb ) )
    {
        BB = REF_TO_INT( coll->prtb );
    }
    else if ( FANOFF != coll->tileb )
    {
        BB = coll->tileb;
    }

    return MAKE_HASH( AA, BB );
}

//--------------------------------------------------------------------------------------------
int CoNode_cmp( const void * vleft, const void * vright )
{
    int   itmp;
    float ftmp;

    CoNode_t * pleft  = ( CoNode_t * )vleft;
    CoNode_t * pright = ( CoNode_t * )vright;

    // sort by initial time first
    ftmp = pleft->tmin - pright->tmin;
    if ( ftmp <= 0.0f ) return -1;
    else if ( ftmp >= 0.0f ) return 1;

    // fort by final time second
    ftmp = pleft->tmax - pright->tmax;
    if ( ftmp <= 0.0f ) return -1;
    else if ( ftmp >= 0.0f ) return 1;

    itmp = ( signed )REF_TO_INT( pleft->chra ) - ( signed )REF_TO_INT( pright->chra );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->prta ) - ( signed )REF_TO_INT( pright->prta );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->chra ) - ( signed )REF_TO_INT( pright->chra );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->prtb ) - ( signed )REF_TO_INT( pright->prtb );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->chrb ) - ( signed )REF_TO_INT( pright->chrb );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )pleft->tileb - ( signed )pright->tileb;
    if ( 0 != itmp ) return itmp;

    return 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHashList_t * CHashList_ctor( CHashList_t * pchlst, int size )
{
    return hash_list_ctor( pchlst, size );
}

//--------------------------------------------------------------------------------------------
CHashList_t * CHashList_dtor( CHashList_t * pchlst )
{
    return hash_list_dtor( pchlst );
}

//--------------------------------------------------------------------------------------------
CHashList_t * CHashList_get_Instance( int size )
{
    /// @details BB@> allows access to a "private" CHashList singleton object. This will automatically
    ///               initialze the _Colist_singleton and (almost) prevent anything from messing up
    ///               the initialization.

    // make sure that the collsion system was started
    collision_system_begin();

    // if the _CHashList_ptr doesn't exist, create it (and initialize it)
    if ( NULL == _CHashList_ptr )
    {
        _CHashList_ptr              = hash_list_create( size );
        _collision_hash_initialized = ( NULL != _CHashList_ptr );
    }

    // it the pointer exists, but it (somehow) not initialized, do the initialization
    if ( NULL != _CHashList_ptr && !_collision_hash_initialized )
    {
        _CHashList_ptr              = CHashList_ctor( _CHashList_ptr, size );
        _collision_hash_initialized = ( NULL != _CHashList_ptr );
    }

    return _collision_hash_initialized ? _CHashList_ptr : NULL;
}

//--------------------------------------------------------------------------------------------
bool_t CHashList_insert_unique( CHashList_t * pchlst, CoNode_t * pdata, CoNode_ary_t * free_cdata, HashNode_ary_t * free_hnodes )
{
    Uint32 hashval = 0;
    CoNode_t * d;

    hash_node_t * hn;
    bool_t found;
    size_t count;

    if ( NULL == pchlst || NULL == pdata ) return bfalse;

    // find the hash value for this interaction
    hashval = CoNode_generate_hash( pdata );

    found = bfalse;
    count = hash_list_get_count( pchlst, hashval );
    if ( count > 0 )
    {
        size_t k;

        // this hash already exists. check to see if the binary collision exists, too
        hn = hash_list_get_node( pchlst, hashval );
        for ( k = 0; k < count; k++ )
        {
            if ( 0 == CoNode_cmp( hn->data, pdata ) )
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
        hash_node_t * old_head, * new_head, * hn;

        // pick a free collision data
        d = CoNode_ary_pop_back( free_cdata );

        // fill it in
        *d = *pdata;

        // generate a new hash node
        hn = HashNode_ary_pop_back( free_hnodes );

        // link the hash node to the free CoNode
        hn->data = d;

        // insert the node at the front of the collision list for this hash
        old_head = hash_list_get_node( pchlst, hashval );
        new_head = hash_node_insert_before( old_head, hn );
        hash_list_set_node( pchlst, hashval, new_head );

        // add 1 to the count at this hash
        old_count = hash_list_get_count( pchlst, hashval );
        hash_list_set_count( pchlst, hashval, old_count + 1 );
    }

    return !found;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t detect_chr_chr_interaction_valid( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b )
{
    chr_t *pchr_a, *pchr_b;

    // Don't interact with self
    if ( ichr_a == ichr_b ) return bfalse;

    // Ignore invalid characters
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // Ignore invalid characters
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    // don't interact if there is no interaction
    if ( 0.0f == pchr_a->bump_stt.size || 0.0f == pchr_b->bump_stt.size ) return bfalse;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pchr_b->is_hidden ) return bfalse;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detect_chr_prt_interaction_valid( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b )
{
    chr_t * pchr_a;
    prt_t * pprt_b;

    // Ignore invalid characters
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // Ignore invalid characters
    if ( !INGAME_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

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
bool_t fill_interaction_list( CHashList_t * pchlst, CoNode_ary_t * cn_lst, HashNode_ary_t * hn_lst )
{
    ego_mpd_info_t * mi;
    BSP_aabb_t       tmp_aabb;

    if ( NULL == pchlst || NULL == cn_lst || NULL == hn_lst ) return bfalse;

    mi = &( PMesh->info );

    // allocate a BSP_aabb_t once, to be shared for all collision tests
    BSP_aabb_ctor( &tmp_aabb, obj_BSP_root.tree.dimensions );

    // renew the CoNode_t hash table.
    hash_list_renew( pchlst );

    //---- find the character/particle interactions

    // Find the character-character interactions.
    // Loop only through the list of objects that are both active and in the BSP tree
    CHashList_inserted = 0;
    CHR_BEGIN_LOOP_BSP( ichr_a, pchr_a )
    {
        oct_bb_t   tmp_oct;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_chr_bb( pchr_a, 0.0f, 1.0f, &tmp_oct );

        // convert the oct_bb_t to a correct BSP_aabb_t
        BSP_aabb_from_oct_bb( &tmp_aabb, &tmp_oct );

        // find all collisions with other characters and particles
        _coll_leaf_lst.top = 0;
        obj_BSP_collide( &( obj_BSP_root ), &tmp_aabb, &_coll_leaf_lst );

        // transfer valid _coll_leaf_lst entries to pchlst entries
        // and sort them by their initial times
        if ( _coll_leaf_lst.top > 0 )
        {
            size_t j;

            for ( j = 0; j < _coll_leaf_lst.top; j++ )
            {
                BSP_leaf_t * pleaf;
                size_t      coll_ref;
                CoNode_t    tmp_codata;
                bool_t      do_insert;
                BIT_FIELD   test_platform;

                pleaf = _coll_leaf_lst.ary[j];
                if ( NULL == pleaf ) continue;

                // assume the worst
                do_insert = bfalse;

                coll_ref = pleaf->index;
                if ( LEAF_CHR == pleaf->data_type )
                {
                    // collided with a character
                    CHR_REF ichr_b = ( CHR_REF )coll_ref;

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_chr_interaction_valid( ichr_a, ichr_b ) )
                    {
                        bool_t found = bfalse;

                        chr_t * pchr_b = ChrList.lst + ichr_b;

                        CoNode_ctor( &tmp_codata );

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
                            tmp_codata.chra = ichr_a;
                            tmp_codata.chrb = ichr_b;

                            do_insert = btrue;
                        }
                    }
                }
                else if ( LEAF_PRT == pleaf->data_type )
                {
                    // collided with a particle
                    PRT_REF iprt_b = ( PRT_REF )coll_ref;

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_prt_interaction_valid( ichr_a, iprt_b ) )
                    {
                        bool_t found = bfalse;

                        prt_t * pprt_b = PrtList.lst + iprt_b;

                        CoNode_ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the cap_t for these objects here
                        test_platform = PHYS_CLOSE_TOLERANCE_NONE;
                        test_platform |= pchr_a->platform ? PHYS_CLOSE_TOLERANCE_OBJ1 : 0;

                        // detect a when the possible collision occurred

                        // clear the collision volume
                        oct_bb_ctor( &( tmp_codata.cv ) );

                        // platform physics overrides normal interactions, so
                        // do the platform test first
                        if ( PHYS_CLOSE_TOLERANCE_NONE != test_platform && !found )
                        {
                            found = phys_intersect_oct_bb( pchr_a->chr_max_cv, pchr_a->pos, pchr_a->vel, pprt_b->prt_cv, prt_get_pos( pprt_b ), pprt_b->vel, test_platform, NULL, &( tmp_codata.tmin ), &( tmp_codata.tmax ) );
                        }

                        // try the normal test if the platform test fails
                        if ( !found )
                        {
                            found = phys_intersect_oct_bb( pchr_a->chr_min_cv, pchr_a->pos, pchr_a->vel, pprt_b->prt_cv, prt_get_pos( pprt_b ), pprt_b->vel, test_platform, &( tmp_codata.cv ), &( tmp_codata.tmin ), &( tmp_codata.tmax ) );
                        }

                        if ( found )
                        {
                            tmp_codata.chra = ichr_a;
                            tmp_codata.prtb = iprt_b;

                            do_insert = btrue;
                        }
                    }
                }

                if ( do_insert )
                {
                    if ( CHashList_insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst ) )
                    {
                        CHashList_inserted++;
                    }
                }
            }
        }
    }
    CHR_END_LOOP();

    //---- find the character and particle interactions with the mesh (not implemented)

    //// search through all characters. Use the ChrList.used_ref, for a change
    //_coll_leaf_lst.top = 0;
    //for ( i = 0; i < ChrList.used_count; i++ )
    //{
    //    CHR_REF ichra = ChrList.used_ref[i];
    //    if ( !INGAME_CHR( ichra ) ) continue;

    //    // find all character collisions with mesh tiles
    //    mesh_BSP_collide( &mesh_BSP_root, &( ChrList.lst[ichra].prt_cv ), &_coll_leaf_lst );
    //    if ( _coll_leaf_lst.top > 0 )
    //    {
    //        int j;

    //        for ( j = 0; j < _coll_leaf_lst.top; j++ )
    //        {
    //            int coll_ref;
    //            CoNode_t tmp_codata;

    //            coll_ref = _coll_leaf_lst.ary[j];

    //            CoNode_ctor( &tmp_codata );
    //            tmp_codata.chra  = ichra;
    //            tmp_codata.tileb = coll_ref;

    //            CHashList_insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst );
    //        }
    //    }
    //}

    //// search through all particles. Use the PrtList.used_ref, for a change
    //_coll_leaf_lst.top = 0;
    //for ( i = 0; i < PrtList.used_count; i++ )
    //{
    //    PRT_REF iprta = PrtList.used_ref[i];
    //    if ( !INGAME_PRT( iprta ) ) continue;

    //    // find all particle collisions with mesh tiles
    //    mesh_BSP_collide( &mesh_BSP_root, &( PrtList.lst[iprta].prt_cv ), &_coll_leaf_lst );
    //    if ( _coll_leaf_lst.top > 0 )
    //    {
    //        int j;

    //        for ( j = 0; j < _coll_leaf_lst.top; j++ )
    //        {
    //            int coll_ref;
    //            CoNode_t tmp_codata;

    //            coll_ref = _coll_leaf_lst.ary[j];

    //            CoNode_ctor( &tmp_codata );
    //            tmp_codata.prta  = iprta;
    //            tmp_codata.tileb = coll_ref;

    //            CHashList_insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst );
    //        }
    //    }
    //}

    // do this manually in C
    BSP_aabb_dtor( &tmp_aabb );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fill_bumplists( obj_BSP_t * pbsp )
{
    /// @details BB@> Fill in the obj_BSP_t for this frame
    ///
    /// @note do not use obj_BSP_empty every frame, because the number of pre-allocated nodes can be quite large.
    /// Instead, just remove the nodes from the tree, fill the tree, and then prune any empty leaves

    if ( NULL == pbsp ) return bfalse;

    // Remove any unused branches from the tree.
    // If you do this after BSP_tree_free_nodes() it will remove all branches
    if ( 7 == ( frame_all & 7 ) )
    {
        BSP_tree_prune( &( pbsp->tree ) );
    }

    // empty out the BSP node lists
    obj_BSP_empty( pbsp );

    // fill up the BSP list based on the current locations
    obj_BSP_fill( pbsp );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_mount_detection( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b )
{
    size_t cnt;

    fvec3_t grip_up, grip_origin, vdiff, pdiff;

    chr_t * pchr_a, * pchr_b;
    chr_t * pmount, * prider;
    chr_instance_t * pmount_inst;

    bool_t mount_updated = bfalse;
    bool_t mount_a, mount_b;

    float   vnrm;

    oct_bb_t  grip_cv, rider_cv;

    oct_vec_t odepth;
    float z_overlap;

    // how well does the position of the character's feet match the mount?
    float hrz_overlap1, hrz_overlap2, hrz_overlap, overlap;

    // make sure that A is valid
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // make sure that B is valid
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    // only check possible rider-mount interactions
    mount_a = chr_can_mount( ichr_b, ichr_a ) && !INGAME_CHR( pchr_b->attachedto );
    mount_b = chr_can_mount( ichr_a, ichr_b ) && !INGAME_CHR( pchr_a->attachedto );
    if ( !mount_a && !mount_b ) return bfalse;

    // if a rider is already mounted, don't do anything
    if ( mount_a ) mount_a = !INGAME_CHR( pchr_b->attachedto );
    if ( mount_b ) mount_b = !INGAME_CHR( pchr_a->attachedto );
    if ( !mount_a && !mount_b ) return bfalse;

    // determine whether any collision is possible
    z_overlap  = MIN( pchr_b->chr_min_cv.maxs[OCT_Z] + pchr_b->pos.z, pchr_a->chr_min_cv.maxs[OCT_Z] + pchr_a->pos.z ) -
                 MAX( pchr_b->chr_min_cv.mins[OCT_Z] + pchr_b->pos.z, pchr_a->chr_min_cv.mins[OCT_Z] + pchr_a->pos.z );

    if ( z_overlap <= 0.0f ) return bfalse;

    // if both riders are mounts determine how to arrange them
    if ( mount_a && mount_b )
    {
        bool_t chara_on_top;
        float depth_a, depth_b;
        float depth_max;

        depth_a = ( pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z] ) - pchr_a->pos.z;
        depth_b = ( pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] ) - pchr_b->pos.z;

        depth_max = MIN( pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z] + MOUNTTOLERANCE, pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] + MOUNTTOLERANCE ) -
                    MAX( pchr_b->pos.z, pchr_a->pos.z );

        chara_on_top = ABS( depth_max - depth_a ) < ABS( depth_max - depth_b );

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

    // alias this variable for notation simplicity
    pmount_inst = &( pmount->inst );

    // determine the grip properties
    {
        int       grip_count;
        Uint16    grip_verts[GRIP_VERTS];
        oct_bb_t  tmp_cv = OCT_BB_INIT_VALS;
        int       vertex;

        // use a standard size for the grip
        for ( cnt = 0; cnt < OCT_Z; cnt++ )
        {
            tmp_cv.mins[cnt] = -15 * pmount->fat * 2;
            tmp_cv.maxs[cnt] =  15 * pmount->fat * 2;
        }

        // make the vertical dimension something like 1/2 of a human character height
        tmp_cv.mins[OCT_Z] = 0;
        tmp_cv.maxs[OCT_Z] = 60 * pmount->fat;

        tmp_cv.mins[OCT_XY] *= SQRT_TWO;
        tmp_cv.maxs[OCT_XY] *= SQRT_TWO;
        tmp_cv.mins[OCT_YX] *= SQRT_TWO;
        tmp_cv.maxs[OCT_YX] *= SQRT_TWO;

        // do the automatic vertex update
        vertex = ( signed )( pmount_inst->vrt_count ) - ( signed )GRIP_LEFT;
        vertex = MAX( 0, vertex );
        chr_instance_update_vertices( pmount_inst, vertex, vertex + GRIP_LEFT, bfalse );

        // calculate the grip vertices
        for ( grip_count = 0, cnt = 0; cnt < GRIP_VERTS && vertex + cnt < pmount_inst->vrt_count; grip_count++, cnt++ )
        {
            grip_verts[cnt] = vertex + cnt;
        }
        for ( /* nothing */ ; cnt < GRIP_VERTS; cnt++ )
        {
            grip_verts[cnt] = 0xFFFF;
        }

        // assume the worst
        fvec3_self_clear( grip_origin.v );

        // calculate grip_origin and grip_up
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
            grip_up = fvec3_normalize( grip_vecs[2].v );

            // grab the origin of the grip
            grip_origin.x = grip_nupoints[0].x;
            grip_origin.y = grip_nupoints[0].y;
            grip_origin.z = grip_nupoints[0].z;
        }
        else if ( 0 == grip_count )
        {
            // grab the grip's "up" vector
            grip_up.x = grip_up.y = 0.0f;
            grip_up.z = -SGN( gravity );

            // grab the origin of the grip
            grip_origin = chr_get_pos( pmount );
        }
        else if ( grip_count > 0 )
        {
            fvec4_t grip_points[GRIP_VERTS], grip_nupoints[GRIP_VERTS];

            // Calculate grip point locations with linear interpolation and other silly things
            convert_grip_to_local_points( pmount, grip_verts, grip_points );

            // Do the transform the vertices for the grip's origin
            TransformVertices( &( pmount_inst->matrix ), grip_points, grip_nupoints, 1 );

            // grab the grip's "up" vector
            grip_up.x = grip_up.y = 0.0f;
            grip_up.z = -SGN( gravity );

            // grab the origin of the grip
            grip_origin.x = grip_nupoints[0].x;
            grip_origin.y = grip_nupoints[0].y;
            grip_origin.z = grip_nupoints[0].z;
        }

        // add in the "origin" of the grip
        oct_bb_add_vector( tmp_cv, grip_origin.v, &grip_cv );
    }

    // determine if the rider is falling on the grip
    // maybe not "falling", but just approaching?
    vdiff = fvec3_sub( prider->vel.v, pmount->vel.v );
    pdiff = fvec3_sub( prider->pos.v, grip_origin.v );
    vnrm = fvec3_dot_product( vdiff.v, pdiff.v );
    if ( vnrm >= 0.0f ) return bfalse;

    // convert the rider foot position to an oct vector
    oct_bb_add_vector( prider->chr_min_cv, prider->pos.v, &rider_cv );

    // initialize the overlap depths
    odepth[OCT_Z] = odepth[OCT_X] = odepth[OCT_Y] = odepth[OCT_XY] = odepth[OCT_YX] = 0.0f;

    // Calculate the interaction depths. The size of the rider doesn't matter.
    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        odepth[cnt] = MIN( grip_cv.maxs[cnt], rider_cv.maxs[cnt] ) - MAX( grip_cv.mins[cnt], rider_cv.mins[cnt] );
        if ( odepth[cnt] <= 0.0f ) return bfalse;
    }

    // fix the diagonal distances
    odepth[OCT_XY] *= INV_SQRT_TWO;
    odepth[OCT_YX] *= INV_SQRT_TWO;

    // estimate the amount of overlap area in the horizontal direction
    hrz_overlap1 = odepth[OCT_X ] * odepth[OCT_Y ];
    hrz_overlap2 = odepth[OCT_XY] * odepth[OCT_YX];
    hrz_overlap  = MIN( hrz_overlap1, hrz_overlap2 );
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
bool_t do_chr_platform_detection( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b )
{
    int cnt;

    chr_t * pchr_a, * pchr_b;

    bool_t platform_updated = bfalse;
    bool_t platform_a, platform_b;
    bool_t mount_a, mount_b;

    oct_vec_t opos_object;
    oct_bb_t  platform_min_cv, platform_max_cv;

    oct_vec_t odepth;
    float z_overlap;

    // how well does the position of the character's feet match the platform?
    float hrz_overlap1, hrz_overlap2, hrz_overlap, vrt_overlap, overlap;

    // make sure that A is valid
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // make sure that B is valid
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

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
    z_overlap  = MIN( pchr_b->chr_max_cv.maxs[OCT_Z] + pchr_b->pos.z, pchr_a->chr_max_cv.maxs[OCT_Z] + pchr_a->pos.z ) -
                 MAX( pchr_b->chr_max_cv.mins[OCT_Z] + pchr_b->pos.z, pchr_a->chr_max_cv.mins[OCT_Z] + pchr_a->pos.z );

    if ( z_overlap <= 0.0f ) return bfalse;

    // if both objects are platforms determine how to arrange them
    if ( platform_a && platform_b )
    {
        bool_t chara_on_top;
        float depth_a, depth_b;
        float depth_max;

        depth_a = ( pchr_b->pos.z + pchr_b->chr_max_cv.maxs[OCT_Z] ) - pchr_a->pos.z;
        depth_b = ( pchr_a->pos.z + pchr_a->chr_max_cv.maxs[OCT_Z] ) - pchr_b->pos.z;

        depth_max = MIN( pchr_b->pos.z + pchr_b->chr_max_cv.maxs[OCT_Z], pchr_a->pos.z + pchr_a->chr_max_cv.maxs[OCT_Z] ) -
                    MAX( pchr_b->pos.z, pchr_a->pos.z );

        chara_on_top = ABS( depth_max - depth_a ) < ABS( depth_max - depth_b );

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
    oct_bb_ctor( &platform_min_cv );
    if ( platform_a )
    {
        oct_vec_ctor( opos_object, pchr_b->pos );
        oct_bb_add_vector( pchr_a->chr_min_cv, pchr_a->pos.v, &platform_min_cv );
        oct_bb_add_vector( pchr_a->chr_max_cv, pchr_a->pos.v, &platform_max_cv );
    }
    else if ( platform_b )
    {
        oct_vec_ctor( opos_object, pchr_a->pos );
        oct_bb_add_vector( pchr_b->chr_min_cv, pchr_b->pos.v, &platform_min_cv );
        oct_bb_add_vector( pchr_b->chr_max_cv, pchr_b->pos.v, &platform_max_cv );
    }

    // initialize the overlap depths
    odepth[OCT_Z] = odepth[OCT_X] = odepth[OCT_Y] = odepth[OCT_XY] = odepth[OCT_YX] = 0.0f;

    // the size of the object doesn't matter
    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        odepth[cnt] = MIN( platform_max_cv.maxs[cnt] - opos_object[cnt], opos_object[cnt] - platform_max_cv.mins[cnt] );
        if ( odepth[cnt] <= 0.0f ) return bfalse;
    }

    // fix the diagonal distances
    odepth[OCT_XY] *= INV_SQRT_TWO;
    odepth[OCT_YX] *= INV_SQRT_TWO;

    // estimate the amount of overlap area in the horizontal direction
    hrz_overlap1 = odepth[OCT_X ] * odepth[OCT_Y ];
    hrz_overlap2 = odepth[OCT_XY] * odepth[OCT_YX];
    hrz_overlap  = MIN( hrz_overlap1, hrz_overlap2 );
    if ( hrz_overlap <= 0.0f ) return bfalse;

    // estimate the amount of overlap height
    vrt_overlap = PLATTOLERANCE - ABS( opos_object[OCT_Z] - platform_min_cv.maxs[OCT_Z] );
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
bool_t do_prt_platform_detection( const PRT_REF by_reference iprt_a, const CHR_REF by_reference ichr_b )
{
    int cnt;

    prt_t * pprt_a;
    chr_t * pchr_b;

    bool_t platform_updated = bfalse;
    bool_t platform_b;

    oct_vec_t opos_object;
    oct_bb_t  platform_min_cv, platform_max_cv;

    oct_vec_t odepth;
    float z_overlap;

    // how well does the position of the character's feet match the platform?
    float hrz_overlap1, hrz_overlap2, hrz_overlap, vrt_overlap, overlap;

    // make sure that A is valid
    if ( !DISPLAY_PRT( iprt_a ) ) return bfalse;
    pprt_a = PrtList.lst + iprt_a;

    // make sure that B is valid
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    // if you are mounted, only your mount is affected by platforms
    if ( DEFINED_CHR( pprt_a->attachedto_ref ) || IS_ATTACHED_PCHR( pchr_b ) ) return bfalse;

    // only check possible object-platform interactions
    platform_b = /* pprt_a->canuseplatforms && */ pchr_b->platform;
    if ( !platform_b ) return bfalse;

    // determine whether any collision is possible
    z_overlap  = MIN( pchr_b->chr_max_cv.maxs[OCT_Z] + pchr_b->pos.z, pprt_a->prt_min_cv.maxs[OCT_Z] + pprt_a->pos.z ) -
                 MAX( pchr_b->chr_max_cv.mins[OCT_Z] + pchr_b->pos.z, pprt_a->prt_min_cv.mins[OCT_Z] + pprt_a->pos.z );

    if ( z_overlap <= 0.0f ) return bfalse;

    // The surface that the object will rest on is given by chr_min_cv.maxs[OCT_Z],
    // not chr_max_cv.maxs[OCT_Z] or chr_cv.maxs[OCT_Z]
    oct_vec_ctor( opos_object, pprt_a->pos );
    oct_bb_add_vector( pchr_b->chr_min_cv, pchr_b->pos.v, &platform_min_cv );
    oct_bb_add_vector( pchr_b->chr_max_cv, pchr_b->pos.v, &platform_max_cv );

    // initialize the overlap depths
    odepth[OCT_Z] = odepth[OCT_X] = odepth[OCT_Y] = odepth[OCT_XY] = odepth[OCT_YX] = 0.0f;

    // the size of the object doesn't matter
    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        odepth[cnt] = MIN( platform_max_cv.maxs[cnt] - opos_object[cnt], opos_object[cnt] - platform_max_cv.mins[cnt] );
        if ( odepth[cnt] <= 0.0f ) return bfalse;
    }

    // fix the diagonal distances
    odepth[OCT_XY] *= INV_SQRT_TWO;
    odepth[OCT_YX] *= INV_SQRT_TWO;

    // estimate the amount of overlap area in the horizontal direction
    hrz_overlap1 = odepth[OCT_X ] * odepth[OCT_Y ];
    hrz_overlap2 = odepth[OCT_XY] * odepth[OCT_YX];
    hrz_overlap  = MIN( hrz_overlap1, hrz_overlap2 );
    if ( hrz_overlap <= 0.0f ) return bfalse;

    // estimate the amount of overlap height
    vrt_overlap = PLATTOLERANCE - ABS( opos_object[OCT_Z] - platform_min_cv.maxs[OCT_Z] );
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
bool_t attach_chr_to_platform( chr_t * pchr, chr_t * pplat )
{
    /// @details BB@> attach a character to a platform

    cap_t * pchr_cap;

    // verify that we do not have two dud pointers
    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PCHR( pplat ) ) return bfalse;

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
        if (( CHR_REF ) MAX_CHR != pchr->onwhichplatform_ref )
        {
            detach_character_from_platform( pchr );
        }

        // attach the character to this plaform
        pchr->onwhichplatform_ref = GET_REF_PCHR( pplat );

        // update the character-platform properties
        chr_get_environment( pchr );

        // tell the platform that we bumped into it
        // this is necessary for key buttons to work properly, for instance
        ai_state_set_bumplast( &( pplat->ai ), GET_REF_PCHR( pchr ) );
    }

    // blank out the targetplatform_ref, so we know we are done searching
    pchr->targetplatform_ref     = ( CHR_REF )MAX_CHR;
    pchr->targetplatform_overlap = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detach_character_from_platform( chr_t * pchr )
{
    /// @details BB@> detach a character from a platform

    chr_t * pplat;

    // verify that we do not have two dud pointers
    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    pchr->onwhichplatform_update   = 0;
    pchr->targetplatform_ref       = ( CHR_REF ) MAX_CHR;
    pchr->targetplatform_overlap   = 0.0f;

    // grab a pointer to the old platform
    pplat = NULL;
    if ( ACTIVE_CHR( pchr->onwhichplatform_ref ) )
    {
        pplat = ChrList.lst + pchr->onwhichplatform_ref;
    }

    // undo the attachment
    pchr->onwhichplatform_ref     = ( CHR_REF ) MAX_CHR;

    // do some platform adjustments
    if ( NULL != pplat )
    {
        // adjust the platform's weight, if necessary
        pplat->holdingweight -= pchr->onwhichplatform_weight;
        pplat->holdingweight = MAX( 0.0f, pplat->holdingweight );

        // update the character-platform properties
        chr_get_environment( pchr );
    }

    // fix some character variables
    pchr->onwhichplatform_weight  = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_prt_to_platform( prt_t * pprt, chr_t * pplat )
{
    /// @details BB@> attach a particle to a platform

    pip_t   * pprt_pip;

    // verify that we do not have two dud pointers
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;
    if ( !ACTIVE_PCHR( pplat ) ) return bfalse;

    pprt_pip = prt_get_ppip( GET_REF_PPRT( pprt ) );
    if ( NULL == pprt_pip ) return bfalse;

    // check if they can be connected
    if ( !pplat->platform ) return bfalse;

    // let the code know that the platform attachment was confirmed this update
    pprt->onwhichplatform_update = update_wld;

    // do the attachment
    if ( pprt->onwhichplatform_ref != GET_REF_PCHR( pplat ) )
    {
        prt_bundle_t bdl;

        pprt->onwhichplatform_ref    = GET_REF_PCHR( pplat );

        prt_get_environment( prt_bundle_set( &bdl, pprt ) );
    }

    // blank the targetplatform_* stuff so we know we are done searching
    pprt->targetplatform_ref     = ( CHR_REF )MAX_CHR;
    pprt->targetplatform_overlap = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detach_particle_from_platform( prt_t * pprt )
{
    /// @details BB@> attach a particle to a platform

    prt_bundle_t bdl_prt;

    // verify that we do not have two dud pointers
    if ( !DEFINED_PPRT( pprt ) ) return bfalse;

    // grab all of the particle info
    prt_bundle_set( &bdl_prt, pprt );

    // check if they can be connected
    if ( INGAME_CHR( pprt->onwhichplatform_ref ) ) return bfalse;

    // undo the attachment
    pprt->onwhichplatform_ref    = ( CHR_REF ) MAX_CHR;
    pprt->onwhichplatform_update = 0;
    pprt->targetplatform_ref     = ( CHR_REF ) MAX_CHR;
    pprt->targetplatform_overlap   = 0.0f;

    // get the correct particle environment
    prt_get_environment( &bdl_prt );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void bump_all_objects( obj_BSP_t * pbsp )
{
    /// @details ZZ@> This function handles characters hitting other characters or particles

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
bool_t bump_all_platforms( CoNode_ary_t * pcn_ary )
{
    /// @details BB@> Detect all character and particle interactions with platforms, then attach them.
    ///
    /// @note it is important to only attach the character to a platform once, so its
    ///  weight does not get applied to multiple platforms

    size_t     cnt;
    CoNode_t * d;

    if ( NULL == pcn_ary ) return bfalse;

    //---- Detect all platform attachments
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = pcn_ary->ary + cnt;

        // only look at character-platform or particle-platform interactions interactions
        if ( TOTAL_MAX_PRT != d->prta && TOTAL_MAX_PRT != d->prtb ) continue;

        if ( MAX_CHR != d->chra && MAX_CHR != d->chrb )
        {
            do_chr_platform_detection( d->chra, d->chrb );
        }
        else if ( MAX_CHR != d->chra && TOTAL_MAX_PRT != d->prtb )
        {
            do_prt_platform_detection( d->chra, d->prtb );
        }
        if ( TOTAL_MAX_PRT != d->prta && MAX_CHR != d->chrb )
        {
            do_prt_platform_detection( d->chrb, d->prta );
        }
    }

    //---- Do the actual platform attachments.

    // Doing the attachments after detecting the best platform
    // prevents an object from attaching it to multiple platforms as it
    // is still trying to find the best one
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = pcn_ary->ary + cnt;

        // only look at character-character interactions
        //if ( TOTAL_MAX_PRT != d->prta && TOTAL_MAX_PRT != d->prtb ) continue;

        if ( MAX_CHR != d->chra && MAX_CHR != d->chrb )
        {
            if ( INGAME_CHR( d->chra ) && INGAME_CHR( d->chrb ) )
            {
                if ( ChrList.lst[d->chra].targetplatform_overlap > 0.0f && ChrList.lst[d->chra].targetplatform_ref == d->chrb )
                {
                    attach_chr_to_platform( ChrList.lst + d->chra, ChrList.lst + d->chrb );
                }
                else if ( ChrList.lst[d->chrb].targetplatform_overlap > 0.0f && ChrList.lst[d->chrb].targetplatform_ref == d->chra )
                {
                    attach_chr_to_platform( ChrList.lst + d->chrb, ChrList.lst + d->chra );
                }

            }
        }
        else if ( MAX_CHR != d->chra && TOTAL_MAX_PRT != d->prtb )
        {
            if ( INGAME_CHR( d->chra ) && INGAME_PRT( d->prtb ) )
            {
                if ( PrtList.lst[d->prtb].targetplatform_overlap > 0.0f && PrtList.lst[d->prtb].targetplatform_ref == d->chra )
                {
                    attach_prt_to_platform( PrtList.lst + d->prtb, ChrList.lst + d->chra );
                }
            }
        }
        else if ( MAX_CHR != d->chrb && TOTAL_MAX_PRT != d->prta )
        {
            if ( INGAME_CHR( d->chrb ) && INGAME_PRT( d->prta ) )
            {
                if ( PrtList.lst[d->prta].targetplatform_overlap > 0.0f &&  PrtList.lst[d->prta].targetplatform_ref == d->chrb )
                {
                    attach_prt_to_platform( PrtList.lst + d->prta, ChrList.lst + d->chrb );
                }
            }
        }
    }

    //---- remove any bad platforms

    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        if ( MAX_CHR != pchr->onwhichplatform_ref && pchr->onwhichplatform_update < update_wld )
        {
            detach_character_from_platform( pchr );
        }
    }
    CHR_END_LOOP();

    PRT_BEGIN_LOOP_DISPLAY( iprt, bdl_prt )
    {
        if ( MAX_CHR != bdl_prt.prt_ptr->onwhichplatform_ref && bdl_prt.prt_ptr->onwhichplatform_update < update_wld )
        {
            detach_particle_from_platform( bdl_prt.prt_ptr );
        }
    }
    PRT_END_LOOP();

    return btrue;
}

//--------------------------------------------------------------------------------------------
egoboo_rv bump_prepare( obj_BSP_t * pbsp )
{
    CHashList_t * pchlst;
    size_t        co_node_count;

    // create a collision hash table that can keep track of 512
    // binary collisions per frame
    pchlst = CHashList_get_Instance( -1 );
    if ( NULL == pchlst )
    {
        log_error( "bump_all_objects() - cannot access the CHashList_t singleton" );
        return rv_error;
    }

    // set up the collision node array
    _co_ary.top = _co_ary.alloc;

    // set up the hash node array
    _hn_ary.top = _hn_ary.alloc;

    // fill up the BSP structures
    fill_bumplists( pbsp );

    // use the BSP structures to detect possible binary interactions
    fill_interaction_list( pchlst, &_co_ary, &_hn_ary );

    // convert the CHashList_t into a CoNode_ary_t and sort
    co_node_count = hash_list_count_nodes( pchlst );

    if ( co_node_count > 0 )
    {
        hash_list_iterator_t it;

        _coll_node_lst.top = 0;

        hash_list_iterator_ctor( &it );
        hash_list_iterator_set_begin( &it, pchlst );
        for ( /* nothing */; !hash_list_iterator_done( &it, pchlst ); hash_list_iterator_next( &it, pchlst ) )
        {
            CoNode_t * ptr = ( CoNode_t * )hash_list_iterator_ptr( &it );
            if ( NULL == ptr ) break;

            CoNode_ary_push_back( &_coll_node_lst, *ptr );
        }

        if ( _coll_node_lst.top > 1 )
        {
            // arrange the actual nodes by time order
            qsort( _coll_node_lst.ary, _coll_node_lst.top, sizeof( CoNode_t ), CoNode_cmp );
        }
    }

    return ( co_node_count > 0 ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
void bump_begin()
{
    // blank the accumulators
    //CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    //{
    //    phys_data_blank_accumulators( &(pchr->phys) );
    //}
    //CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
bool_t bump_all_mounts( CoNode_ary_t * pcn_ary )
{
    /// @details BB@> Detect all character interactions with mounts, then attach them.

    size_t     cnt;
    CoNode_t * d;

    if ( NULL == pcn_ary ) return bfalse;

    //---- Detect all mount attachments
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = pcn_ary->ary + cnt;

        // only look at character-mount or particle-mount interactions interactions
        if ( TOTAL_MAX_PRT != d->prta && TOTAL_MAX_PRT != d->prtb ) continue;

        if ( MAX_CHR != d->chra && MAX_CHR != d->chrb )
        {
            do_chr_mount_detection( d->chra, d->chrb );
        }
    }

    //---- Do the actual mount attachments.

    // Doing the attachments after detecting the best mount
    // prevents an object from attaching it to multiple mounts as it
    // is still trying to find the best one
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = pcn_ary->ary + cnt;

        // only look at character-character interactions
        if ( INGAME_CHR( d->chra ) && INGAME_CHR( d->chrb ) )
        {
            if ( ChrList.lst[d->chra].targetmount_overlap > 0.0f && ChrList.lst[d->chra].targetmount_ref == d->chrb )
            {
                attach_character_to_mount( d->chra, d->chrb, GRIP_ONLY );
            }
            else if ( ChrList.lst[d->chrb].targetmount_overlap > 0.0f && ChrList.lst[d->chrb].targetmount_ref == d->chra )
            {
                attach_character_to_mount( d->chrb, d->chra, GRIP_ONLY );
            }
        }
    }

    return btrue;
}

////--------------------------------------------------------------------------------------------
//bool_t bump_all_mounts( CoNode_ary_t * pcn_ary )
//{
//    /// @details BB@> Detect all character interactions with mounts, then attach them.
//
//    int        cnt;
//    CoNode_t * d;
//
//    if ( NULL == pcn_ary ) return bfalse;
//
//    // Do mounts
//    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
//    {
//        d = pcn_ary->ary + cnt;
//
//        // only look at character-character interactions
//        if ( TOTAL_MAX_PRT != d->prtb ) continue;
//
//        do_mounts( d->chra, d->chrb );
//    }
//
//    return btrue;
//}

//--------------------------------------------------------------------------------------------
bool_t bump_all_collisions( CoNode_ary_t * pcn_ary )
{
    /// @details BB@> Detect all character-character and character-particle collisions (with exclusions
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
            handled = do_chr_chr_collision( pcn_ary->ary + cnt );
        }

        if ( !handled )
        {
            handled = do_chr_prt_collision( pcn_ary->ary + cnt );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
egoboo_rv do_chr_platform_physics( CoNode_t * d, chr_bundle_t *pbdl_item, chr_bundle_t *pbdl_plat )
{
    // we know that ichr_a is a platform and ichr_b is on it
    egoboo_rv retval = rv_error;
    chr_t * pitem, * pplat;

    int     rot_a, rot_b;
    float   walk_lerp, plat_lerp, vel_lerp;
    fvec3_t platform_up, vdiff;
    float   vnrm;
    const float max_vnrm = 50.0f;

    if ( NULL == pbdl_item || NULL == pbdl_plat ) return rv_error;

    // aliases to make notation easier
    pitem = pbdl_item->chr_ptr;
    pplat = pbdl_plat->chr_ptr;

    // does the item exist?
    if ( !ACTIVE_PCHR( pitem ) ) return rv_error;

    // does the platform exist?
    if ( !ACTIVE_PCHR( pplat ) ) return rv_error;

    // wrong platform for this character
    if ( pitem->onwhichplatform_ref != GET_REF_PCHR( pplat ) ) return rv_error;

    // grab the "up" vector for the platform
    chr_getMatUp( pplat, &platform_up );
    fvec3_self_normalize( platform_up.v );

    // what is the "up" velocity relative to the platform?
    vdiff = fvec3_sub( pitem->vel.v, pplat->vel.v );
    vnrm = fvec3_dot_product( vdiff.v, platform_up.v );

    // if the character is moving too fast relative to the platform, then bounce instead of attaching
    if ( ABS( vnrm ) > max_vnrm )
    {
        goto do_chr_platform_physics_fail;
    }

    // re-compute the walk_lerp value, since we need a custom value here
    walk_lerp = ( pitem->pos.z - ( pplat->pos.z + pplat->chr_min_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;

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
        vel_lerp = ABS( vnrm ) / max_vnrm;
        vel_lerp = CLIP( vel_lerp, 0.0f, 1.0f );
    }

    // determine the rotation rates
    rot_b = pitem->ori.facing_z - pitem->ori_old.facing_z;
    rot_a = pplat->ori.facing_z - pplat->ori_old.facing_z;

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

        phys_data_accumulate_apos_plat_index( &( pitem->phys ), ( 1.0f + pitem->enviro.walk_level - pitem->pos.z ) * apos_plat_coeff, kZ );
        phys_data_accumulate_avel_index( &( pitem->phys ), ( pplat->vel.z  - pitem->vel.z ) * avel_coeff, kZ );
        pitem->ori.facing_z += ( float )( rot_a - rot_b ) * facing_coeff;
    }

    // rv_fail indicates that the platform did not attach.
    // in that case, we need to drop down to do_chr_platform_physics_fail
    if ( retval != rv_fail )
    {
        return retval;
    }

do_chr_platform_physics_fail:

    detach_character_from_platform( pitem );

    return rv_fail;
}

//--------------------------------------------------------------------------------------------
float estimate_chr_prt_normal( chr_t * pchr, prt_t * pprt, fvec3_base_t nrm, fvec3_base_t vdiff )
{
    fvec3_t collision_size;
    float dot;

    collision_size.x = MAX( pchr->chr_min_cv.maxs[OCT_X] - pchr->chr_min_cv.mins[OCT_X], 2.0f * pprt->bump_min.size );
    collision_size.y = MAX( pchr->chr_min_cv.maxs[OCT_Y] - pchr->chr_min_cv.mins[OCT_Y], 2.0f * pprt->bump_min.size );
    collision_size.z = MAX( pchr->chr_min_cv.maxs[OCT_Z] - pchr->chr_min_cv.mins[OCT_Z], 2.0f * pprt->bump_min.height );

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
        memcpy( nrm, ntmp.v, sizeof( fvec3_base_t ) );

        // determine the actual dot product
        dot = fvec3_dot_product( vdiff, nrm );
    }

    return dot;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision_test( chr_bundle_t *pbdl_a, chr_bundle_t *pbdl_b )
{
    /// BB@> break off the collision tests to this function

    CHR_REF ichr_a, ichr_b;
    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    if ( NULL == pbdl_a || NULL == pbdl_b ) return bfalse;

    // make some aliases for easier notation
    ichr_a = pbdl_a->chr_ref;
    pchr_a = pbdl_a->chr_ptr;
    pcap_a = pbdl_a->cap_ptr;

    ichr_b = pbdl_b->chr_ref;
    pchr_b = pbdl_b->chr_ptr;
    pcap_b = pbdl_b->cap_ptr;

    // items can interact with platforms but not with other characters/items
    if ( pchr_a->isitem && !pchr_b->platform ) return bfalse;
    if ( pchr_b->isitem && !pchr_a->platform ) return bfalse;

    // make sure that flying platforms fo not interact
    // I think this would mess up the platforms in the tourist starter
    if (( pchr_a->is_flying_platform && pchr_a->platform ) && ( pchr_b->is_flying_platform && pchr_b->platform ) ) return bfalse;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    // don't do anything if there is no bump size
    if ( 0.0f == pchr_a->bump_stt.size || 0.0f == pchr_b->bump_stt.size ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_plat_collision_test( chr_bundle_t *pbdl_item, chr_bundle_t *pbdl_plat )
{
    /// BB@> break off the collision tests to this function

    CHR_REF ichr_plat;
    chr_t * pchr_item, * pchr_plat;

    if ( NULL == pbdl_item || NULL == pbdl_plat ) return bfalse;

    // make some aliases for easier notation
    pchr_item = pbdl_item->chr_ptr;

    ichr_plat = pbdl_plat->chr_ref;
    pchr_plat = pbdl_plat->chr_ptr;

    // items can interact with platforms but not with other characters/items
    if ( !pchr_item->canuseplatforms || !pchr_plat->platform ) return bfalse;

    // are these two attached?
    if ( ichr_plat != pchr_item->onwhichplatform_ref ) return bfalse;

    // if the item is flying, there is no normal platform interaction
    if ( pchr_item->platform && pchr_item->is_flying_platform ) return bfalse;

    // don't interact if you are mounted or held by anything
    if ( IS_ATTACHED_PCHR( pchr_item ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision_interaction( CoNode_t * d, chr_bundle_t *pbdl_a, chr_bundle_t *pbdl_b )
{
    CHR_REF loc_ichr_a, loc_ichr_b;
    chr_t * loc_pchr_a, * loc_pchr_b;
    cap_t * loc_pcap_a, * loc_pcap_b;

    float interaction_strength = 1.0f;
    float wta, wtb;

    fvec3_t   nrm;
    int exponent = 1;

    oct_vec_t opos_a, opos_b, odepth;
    bool_t    collision = bfalse, bump = bfalse;

    // are the bundles valid?
    if ( NULL == pbdl_a || NULL == pbdl_b ) return bfalse;

    // make some aliases for easier notation
    loc_ichr_a = pbdl_a->chr_ref;
    loc_pchr_a = pbdl_a->chr_ptr;
    loc_pcap_a = pbdl_a->cap_ptr;

    loc_ichr_b = pbdl_b->chr_ref;
    loc_pchr_b = pbdl_b->chr_ptr;
    loc_pcap_b = pbdl_b->cap_ptr;

    // skip objects that are inside inventories
    if ( loc_pchr_a->pack.is_packed || loc_pchr_b->pack.is_packed ) return bfalse;

    interaction_strength = 1.0f;

    // ghosts don't interact much
    interaction_strength *= loc_pchr_a->inst.alpha * INV_FF;
    interaction_strength *= loc_pchr_b->inst.alpha * INV_FF;

    // are we interacting with a previous mount?
    interaction_strength *= calc_dismount_lerp( loc_pchr_a, loc_pchr_b );

    // ensure that we have a collision volume
    if ( oct_bb_empty( d->cv ) )
    {
        bool_t found;

        // If you got to this point and this bounding box is empty,
        // then you detected a platform interaction that was not used.
        // Recalculate everything assuming a normal (not platform) interaction

        found = phys_intersect_oct_bb( loc_pchr_a->chr_min_cv, loc_pchr_a->pos, loc_pchr_a->vel, loc_pchr_b->chr_min_cv, loc_pchr_b->pos, loc_pchr_b->vel, 0, &( d->cv ), &( d->tmin ), &( d->tmax ) );

        // if no valid collision was found, return with a fail code
        if ( !found || oct_bb_empty( d->cv ) || d->tmax < 0.0f || d->tmin > 1.0f )
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
        interaction_strength *= ABS( lerp_z );
    }

    if ( loc_pchr_b->platform && INGAME_CHR( loc_pchr_a->onwhichplatform_ref ) && loc_ichr_b != loc_pchr_a->onwhichplatform_ref )
    {
        float lerp_z = ( loc_pchr_a->pos.z - ( loc_pchr_b->pos.z + loc_pchr_b->chr_min_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        // if the platform is close to your level, ignore it
        interaction_strength *= ABS( lerp_z );
    }

    // estimate the collision volume and depth from a 10% overlap
    {
        int cnt;

        oct_bb_t src1, src2;

        oct_bb_t exp1, exp2, intersect;

        float tmp_min, tmp_max;

        tmp_min = d->tmin;
        tmp_max = d->tmin + ( d->tmax - d->tmin ) * 0.1f;

        // shift the source bounding boxes to be centered on the given positions
        oct_bb_add_vector( loc_pchr_a->chr_min_cv, loc_pchr_a->pos.v, &src1 );
        oct_bb_add_vector( loc_pchr_b->chr_min_cv, loc_pchr_b->pos.v, &src2 );

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb( src1, loc_pchr_a->vel, tmp_min, tmp_max, &exp1 );
        phys_expand_oct_bb( src2, loc_pchr_b->vel, tmp_min, tmp_max, &exp2 );

        // determine the intersection of these two volumes
        oct_bb_intersection( exp1, exp2, &intersect );

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
    oct_vec_ctor( opos_a, loc_pchr_a->pos );
    oct_vec_ctor( opos_b, loc_pchr_b->pos );

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
                tmin = MIN( tmin, odepth[OCT_X] / ABS( nrm.x ) );
            }
            if ( nrm.y != 0.0f )
            {
                tmin = MIN( tmin, odepth[OCT_Y] / ABS( nrm.y ) );
            }
            if ( nrm.z != 0.0f )
            {
                tmin = MIN( tmin, odepth[OCT_Z] / ABS( nrm.z ) );
            }

            if ( nrm.x + nrm.y != 0.0f )
            {
                tmin = MIN( tmin, odepth[OCT_XY] / ABS( nrm.x + nrm.y ) );
            }

            if ( -nrm.x + nrm.y != 0.0f )
            {
                tmin = MIN( tmin, odepth[OCT_YX] / ABS( -nrm.x + nrm.y ) );
            }

            if ( tmin < 1e6 )
            {
                const float pressure_strength = 0.125f;

                if ( wta >= 0.0f )
                {
                    float ratio = ( float )ABS( wtb ) / (( float )ABS( wta ) + ( float )ABS( wtb ) );

                    imp_a.x = tmin * nrm.x * ratio * pressure_strength;
                    imp_a.y = tmin * nrm.y * ratio * pressure_strength;
                    imp_a.z = tmin * nrm.z * ratio * pressure_strength;
                }

                if ( wtb >= 0.0f )
                {
                    float ratio = ( float )ABS( wta ) / (( float )ABS( wta ) + ( float )ABS( wtb ) );

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
        ai_state_set_bumplast( &( loc_pchr_a->ai ), loc_ichr_b );
        ai_state_set_bumplast( &( loc_pchr_b->ai ), loc_ichr_a );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision( CoNode_t * d )
{
    /// BB@> Take a collision node and actually do all of the collision physics

    bool_t retval = bfalse;                         // assume that the collision will fail

    CHR_REF ichr_a, ichr_b;
    chr_t * pchr_a, * pchr_b;
    chr_bundle_t bdl_a, bdl_b;

    chr_bundle_t * pbdl_item, * pbdl_plat;
    bool_t do_platform, do_collision;

    // make sure that the collision node exists
    if ( NULL == d ) return bfalse;

    // make sure it is the correct type of collision node
    if ( TOTAL_MAX_PRT != d->prta || TOTAL_MAX_PRT != d->prtb || FANOFF != d->tileb ) return bfalse;

    if ( !INGAME_CHR( d->chra ) ) return bfalse;
    if ( NULL == chr_bundle_set( &bdl_a, ChrList.lst + d->chra ) ) return bfalse;

    // make some aliases for easier notation
    ichr_a = bdl_a.chr_ref;
    pchr_a = bdl_a.chr_ptr;

    // make sure the item is not packed,
    // though it should never have been entered into the collision BSP if it was
    if ( pchr_a->pack.is_packed ) return bfalse;

    if ( !INGAME_CHR( d->chrb ) ) return bfalse;
    if ( NULL == chr_bundle_set( &bdl_b, ChrList.lst + d->chrb ) ) return bfalse;

    // make some aliases for easier notation
    ichr_b = bdl_b.chr_ref;
    pchr_b = bdl_b.chr_ptr;

    // make sure the item is not packed,
    // though it should never have been entered into the collision BSP if it was
    if ( pchr_b->pack.is_packed ) return bfalse;

    pbdl_item = NULL;
    pbdl_plat = NULL;

    do_collision = bfalse;
    do_platform  = bfalse;

    if ( ichr_a == pchr_b->onwhichplatform_ref )
    {
        // platform interaction. if the onwhichplatform_ref is set, then
        // all collision tests have been met

        do_platform = do_chr_plat_collision_test( &bdl_b, &bdl_a );

        if ( do_platform )
        {
            pbdl_item = &bdl_b;
            pbdl_plat = &bdl_a;
        }
    }

    if ( ichr_b == pchr_a->onwhichplatform_ref )
    {
        // platform interaction. if the onwhichplatform_ref is set, then
        // all collision tests have been met

        do_platform = do_chr_plat_collision_test( &bdl_a, &bdl_b );

        if ( do_platform )
        {
            pbdl_item = &bdl_a;
            pbdl_plat = &bdl_b;
        }
    }

    if ( !do_platform )
    {
        // make sure that the collision is allowed
        do_collision = do_chr_chr_collision_test( &bdl_a, &bdl_b );
    }

    // do we have a character-platform interaction?
    if ( do_platform )
    {
        // do the actual platform math
        egoboo_rv plat_retval = do_chr_platform_physics( d, pbdl_item, pbdl_plat );

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
        retval = do_chr_chr_collision_interaction( d, &bdl_a, &bdl_b );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t do_prt_platform_physics( prt_t * pprt, chr_t * pplat, chr_prt_collsion_data_t * pdata )
{
    /// @details BB@> handle the particle interaction with a platform it is not attached "on".
    ///               @note gravity is not handled here
    ///               @note use bump_min here since we are interested in the physical size of the particle

    bool_t plat_collision = bfalse;
    bool_t z_collide, was_z_collide;

    if ( NULL == pdata ) return bfalse;

    // is the platform a platform?
    if ( !ACTIVE_PCHR( pplat ) || !pplat->platform ) return bfalse;

    // can the particle interact with it?
    if ( !ACTIVE_PPRT( pprt ) || INGAME_CHR( pprt->attachedto_ref ) ) return bfalse;

    // this is handled elsewhere
    if ( GET_REF_PCHR( pplat ) == pprt->onwhichplatform_ref ) return bfalse;

    // Test to see whether the particle is in the right position to interact with the platform.
    // You have to be closer to a platform to interact with it than for a general object,
    // but the vertical distance is looser.
    plat_collision = test_interaction_close_1( pplat->chr_max_cv, pplat->pos, pprt->bump_min, prt_get_pos( pprt ), btrue );

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
bool_t do_chr_prt_collision_deflect( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    bool_t prt_deflected = bfalse;

    bool_t chr_is_invictus, chr_can_deflect;
    bool_t prt_wants_deflection;
    FACING_T direction;
    pip_t  *ppip;

    if ( NULL == pdata ) return bfalse;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack.lst + pprt->pip_ref;

    /// @note ZF@> Simply ignore characters with invictus for now, it causes some strange effects
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
            if ( chr_is_invictus && ACTION_IS_TYPE( pchr->inst.action_which, P ) )
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
                    chr_t *pshield = ChrList.lst + item;
                    chr_t *pattacker = ChrList.lst + pprt->owner_ref;
                    int total_block_rating;

                    // use the character block skill plus the base block rating of the shield and adjust for strength
                    total_block_rating = chr_get_skill( pchr, MAKE_IDSZ( 'B', 'L', 'O', 'C' ) );
                    total_block_rating += chr_get_skill( pshield, MAKE_IDSZ( 'B', 'L', 'O', 'C' ) );
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
bool_t do_chr_prt_collision_recoil( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    /// @details BB@> make the character and particle recoil from the collision
    float prt_mass;
    float attack_factor;

    if ( NULL == pdata ) return 0;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

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
        prt_vel2 = MAX( 100.0f, prt_vel2 );

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
        // reduces the damage below pdata->actual_damage == pchr->lifemax
        // and it doubles it if pdata->actual_damage is really huge
        //factor *= 2.0f * ( float )pdata->actual_damage / ( float )( ABS( pdata->actual_damage ) + pchr->lifemax );

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
        chr_t * ptarget;
        CHR_REF iholder;

        ptarget = NULL;

        // transmit the force of the blow back to the character that is
        // holding the weapon

        iholder = chr_get_lowest_attachment( pprt->attachedto_ref, bfalse );
        if ( INGAME_CHR( iholder ) )
        {
            ptarget = ChrList.lst + iholder;
        }
        else
        {
            iholder = chr_get_lowest_attachment( pprt->owner_ref, bfalse );
            if ( INGAME_CHR( iholder ) )
            {
                ptarget = ChrList.lst + iholder;
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
bool_t do_chr_prt_collision_damage( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    ENC_REF enchant, enc_next;
    bool_t prt_needs_impact;

    if ( NULL == pdata ) return bfalse;
    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pchr );

    // Check all enchants to see if they are removed
    enchant = pchr->firstenchant;
    while ( enchant != MAX_ENC )
    {
        enc_next = EncList.lst[enchant].nextenchant_ref;
        if ( enc_is_removed( enchant, pprt->profile_ref ) )
        {
            remove_enchant( enchant, NULL );
        }
        enchant = enc_next;
    }

    // Do confuse effects
    if ( pdata->ppip->grogtime > 0 && pdata->pcap->canbegrogged )
    {
        ADD_BITS( pchr->ai.alert, ALERTIF_CONFUSED );
        if ( pdata->ppip->grogtime > pchr->grogtime )
        {
            pchr->grogtime = MAX( 0, pchr->grogtime + pdata->ppip->grogtime );
        }
    }
    if ( pdata->ppip->dazetime > 0 && pdata->pcap->canbedazed )
    {
        ADD_BITS( pchr->ai.alert, ALERTIF_CONFUSED );
        if ( pdata->ppip->dazetime > pchr->dazetime )
        {
            pchr->dazetime = MAX( 0, pchr->dazetime + pdata->ppip->dazetime );
        }
    }

    //---- Damage the character, if necessary
    if (( pprt->damage.base + pprt->damage.base ) != 0 )
    {
        prt_needs_impact = pdata->ppip->rotatetoface || INGAME_CHR( pprt->attachedto_ref );
        if ( INGAME_CHR( pprt->owner_ref ) )
        {
            chr_t * powner = ChrList.lst + pprt->owner_ref;
            cap_t * powner_cap = pro_get_pcap( powner->profile_ref );

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
                chr_t * powner = ChrList.lst + pprt->owner_ref;

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
                    powner->life = MIN( powner->life + drain, powner->lifemax );
                }

                // Steal some mana
                if ( pprt->manadrain > 0 )
                {
                    drain = pchr->mana;
                    pchr->mana = CLIP( pchr->mana, 0, pchr->mana - pprt->manadrain );
                    drain -= pchr->mana;
                    powner->mana = MIN( powner->mana + drain, powner->manamax );
                }

                // Notify the attacker of a scored hit
                ADD_BITS( powner->ai.alert, ALERTIF_SCOREDAHIT );
                powner->ai.hitlast = GET_REF_PCHR( pchr );

                // Tell the weapons who the attacker hit last
                item = powner->holdingwhich[SLOT_LEFT];
                if ( INGAME_CHR( item ) )
                {
                    ChrList.lst[item].ai.hitlast = GET_REF_PCHR( pchr );
                }

                item = powner->holdingwhich[SLOT_RIGHT];
                if ( INGAME_CHR( item ) )
                {
                    ChrList.lst[item].ai.hitlast = GET_REF_PCHR( pchr );
                }
            }

            // handle vulnerabilities, double the damage
            if ( chr_has_vulnie( GET_REF_PCHR( pchr ), pprt->profile_ref ) )
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
        if ( 0 == ABS( pdata->max_damage ) || ( ABS( pdata->max_damage ) - ABS( pdata->actual_damage ) ) == 0 )
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
            recoil = ( float )ABS( ABS( pdata->max_damage ) - ABS( pdata->actual_damage ) ) / ( float )ABS( pdata->max_damage );

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
bool_t do_chr_prt_collision_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    bool_t prt_belongs_to_chr;
    bool_t prt_hates_chr, prt_attacks_chr, prt_hateonly;
    bool_t valid_onlydamagefriendly;
    bool_t valid_friendlyfire;
    bool_t valid_onlydamagehate;

    if ( NULL == pdata ) return bfalse;
    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // if the particle was deflected, then it can't bump the character
    if ( IS_INVICTUS_PCHR_RAW( pchr ) || pprt->attachedto_ref == GET_REF_PCHR( pchr ) ) return bfalse;

    prt_belongs_to_chr = ( GET_REF_PCHR( pchr ) == pprt->owner_ref );

    if ( !prt_belongs_to_chr )
    {
        // no simple owner relationship. Check for something deeper.
        CHR_REF prt_owner = prt_get_iowner( GET_REF_PPRT( pprt ), 0 );
        if ( INGAME_CHR( prt_owner ) )
        {
            CHR_REF chr_wielder = chr_get_lowest_attachment( GET_REF_PCHR( pchr ), btrue );
            CHR_REF prt_wielder = chr_get_lowest_attachment( prt_owner, btrue );

            if ( !INGAME_CHR( chr_wielder ) ) chr_wielder = GET_REF_PCHR( pchr );
            if ( !INGAME_CHR( prt_wielder ) ) prt_wielder = prt_owner;

            prt_belongs_to_chr = ( chr_wielder == prt_wielder );
        }
    }

    // does the particle team hate the character's team
    prt_hates_chr = team_hates_team( pprt->team, pchr->team );

    // Only bump into hated characters?
    prt_hateonly = PipStack.lst[pprt->pip_ref].hateonly;
    valid_onlydamagehate = prt_hates_chr && PipStack.lst[pprt->pip_ref].hateonly;

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
bool_t do_chr_prt_collision_handle_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    if ( NULL == pdata || !pdata->prt_bumps_chr ) return bfalse;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    if ( !pdata->prt_bumps_chr ) return bfalse;

    // Catch on fire
    spawn_bump_particles( GET_REF_PCHR( pchr ), GET_REF_PPRT( pprt ) );

    // handle some special particle interactions
    if ( pdata->ppip->end_bump )
    {
        if ( pdata->ppip->bump_money )
        {
            chr_t * pcollector = pchr;

            // Let mounts collect money for their riders
            if ( pchr->ismount && INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
            {
                pcollector = ChrList.lst + pchr->holdingwhich[SLOT_LEFT];

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
bool_t do_chr_prt_collision_init( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    /// must use bump_padded here since that is the range for character-particle interactions

    bool_t full_collision;

    if ( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof( *pdata ) );

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // initialize the collision data
    pdata->pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pdata->pcap ) return bfalse;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    pdata->ppip = PipStack.lst + pprt->pip_ref;

    // measure the collision depth
    full_collision = get_depth_1( pchr->chr_min_cv, pchr->pos, pprt->bump_padded, prt_get_pos( pprt ), btrue, pdata->odepth );

    //// measure the collision depth in the last update
    //// the objects were not touching last frame, so they must have collided this frame
    //collision = !get_depth_close_1( pchr_a->chr_min_cv, pchr_a->pos_old, pprt_b->bump, pprt_b->pos_old, btrue, odepth_old );

    // estimate the maximum possible "damage" from this particle
    // other effects can magnify this number, like vulnerabilities
    // or DAMFX_* bits
    pdata->max_damage = ABS( pprt->damage.base ) + ABS( pprt->damage.rand );

    return full_collision;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision( CoNode_t * d )
{
    /// @details BB@> this function goes through all of the steps to handle character-particle
    ///               interactions. A basic interaction has been detected. This needs to be refined
    ///               and then handled. The function returns bfalse if the basic interaction was wrong
    ///               or if the interaction had no effect.
    ///
    /// @note This function is a little more complicated than the character-character case because
    ///       of the friend-foe logic as well as the damage and other special effects that particles can do.

    bool_t retval = bfalse;

    CHR_REF ichr_a;
    PRT_REF iprt_b;

    chr_t * pchr_a;
    prt_t * pprt_b;

    bool_t prt_deflected;
    bool_t prt_can_hit_chr;

    bool_t plat_collision, full_collision;

    chr_prt_collsion_data_t cn_lst;

    if ( NULL == d || TOTAL_MAX_PRT != d->prta || MAX_CHR != d->chrb ) return bfalse;
    ichr_a = d->chra;
    iprt_b = d->prtb;

    // make sure that it is on
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // skip dead objects
    if ( !pchr_a->alive ) return bfalse;

    // skip objects that are inside inventories
    if ( pchr_a->pack.is_packed ) return bfalse;

    if ( !INGAME_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

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
            cap_t *pcap_a = chr_get_pcap( pchr_a->ai.index );

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
        prt_request_terminate_ref( iprt_b );
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t update_chr_platform_attachment( chr_t * pchr )
{
    chr_t * pplat;

    if ( !DEFINED_PCHR( pchr ) || !INGAME_CHR( pchr->onwhichplatform_ref ) ) return bfalse;

    pplat = ChrList.lst + pchr->onwhichplatform_ref;

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
    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        update_chr_platform_attachment( pchr );
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * update_prt_platform_attachment( prt_bundle_t * pbdl )
{
    chr_t * pplat;
    prt_t * loc_pprt;

    if ( NULL == pbdl || NULL == pbdl->prt_ptr ) return pbdl;
    loc_pprt = pbdl->prt_ptr;

    if ( !INGAME_CHR( loc_pprt->onwhichplatform_ref ) ) return pbdl;

    pplat = ChrList.lst + loc_pprt->onwhichplatform_ref;

    return pbdl;
}

//--------------------------------------------------------------------------------------------
void update_all_prt_platform_attachments()
{
    PRT_BEGIN_LOOP_DISPLAY( cnt, bdl )
    {
        update_prt_platform_attachment( &bdl );
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void update_all_platform_attachments()
{
    /// BB@> do all the platform-related stuff that must be done every update

    update_all_chr_platform_attachments();
    update_all_prt_platform_attachments();
}

//--------------------------------------------------------------------------------------------
//bool_t add_chr_chr_interaction( CHashList_t * pchlst, const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst )
//{
//    Uint32 hashval = 0;
//    int count;
//    bool_t found;
//
//    hash_node_t * n;
//    CoNode_t    * d;
//
//    if ( NULL == pchlst || NULL == pcn_lst || NULL == phn_lst ) return bfalse;
//
//    // there is no situation in the game where we allow characters to interact with themselves
//    if ( ichr_a == ichr_b ) return bfalse;
//
//    // create a hash that is order-independent
//    hashval = MAKE_HASH( REF_TO_INT( ichr_a ), REF_TO_INT( ichr_b ) );
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
//            d = ( CoNode_t * )( n->data );
//
//            // make sure to test both orders
//            if (( d->chra == ichr_a && d->chrb == ichr_b ) || ( d->chra == ichr_b && d->chrb == ichr_a ) )
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
//        EGOBOO_ASSERT( CoNode_ary_get_top( pcn_lst ) < CHR_MAX_COLLISIONS );
//        d = CoNode_ary_pop_back( pcn_lst );
//
//        // fill it in
//        CoNode_ctor( d );
//        d->chra = ichr_a;
//        d->chrb = ichr_b;
//
//        // generate a new hash node
//        EGOBOO_ASSERT( HashNode_ary_get_top( phn_lst ) < COLLISION_HASH_NODES );
//        n = HashNode_ary_pop_back( phn_lst );
//
//        hash_node_ctor( n, ( void* )d );
//
//        // insert the node
//        pchlst->subcount[hashval]++;
//        pchlst->sublist[hashval] = hash_node_insert_before( pchlst->sublist[hashval], n );
//    }
//
//    return !found;
//}

//--------------------------------------------------------------------------------------------
//bool_t add_chr_prt_interaction( CHashList_t * pchlst, const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst )
//{
//    bool_t found;
//    int    count;
//    Uint32 hashval = 0;
//
//    hash_node_t * n;
//    CoNode_t    * d;
//
//    if ( NULL == pchlst ) return bfalse;
//
//    // create a hash that is order-independent
//    hashval = MAKE_HASH( REF_TO_INT( ichr_a ), REF_TO_INT( iprt_b ) );
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
//            d = ( CoNode_t * )( n->data );
//            if ( d->chra == ichr_a && d->prtb == iprt_b )
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
//        EGOBOO_ASSERT( CoNode_ary_get_top( pcn_lst ) < CHR_MAX_COLLISIONS );
//        d = CoNode_ary_pop_back( pcn_lst );
//
//        // fill it in
//        CoNode_ctor( d );
//        d->chra = ichr_a;
//        d->prtb = iprt_b;
//
//        // generate a new hash node
//        EGOBOO_ASSERT( HashNode_ary_get_top( phn_lst ) < COLLISION_HASH_NODES );
//        n = HashNode_ary_pop_back( phn_lst );
//
//        hash_node_ctor( n, ( void* )d );
//
//        // insert the node
//        pchlst->subcount[hashval]++;
//        pchlst->sublist[hashval] = hash_node_insert_before( pchlst->sublist[hashval], n );
//    }
//
//    return !found;
//}

////--------------------------------------------------------------------------------------------
//bool_t do_mounts( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b )
//{
//    float xa, ya, za;
//    float xb, yb, zb;
//
//    chr_t * pchr_a, * pchr_b;
//    cap_t * pcap_a, * pcap_b;
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
//    pchr_a = ChrList.lst + ichr_a;
//
//    pcap_a = chr_get_pcap( ichr_a );
//    if ( NULL == pcap_a ) return bfalse;
//
//    // make sure that B is valid
//    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
//    pchr_b = ChrList.lst + ichr_b;
//
//    pcap_b = chr_get_pcap( ichr_b );
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
//            chr_instance_t * pinst = &( pchr_b->inst );
//
//            vertex = (( signed )pinst->vrt_count ) - GRIP_LEFT;
//
//            // do the automatic update
//            chr_instance_update_vertices( pinst, vertex, vertex, bfalse );
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
//        dx = ABS( xa - nupoint[0].x );
//        dy = ABS( ya - nupoint[0].y );
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
//            chr_instance_t * pinst = &( pchr_a->inst );
//
//            vertex = (( signed )pinst->vrt_count ) - GRIP_LEFT;
//
//            // do the automatic update
//            chr_instance_update_vertices( pinst, vertex, vertex, bfalse );
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
//        dx = ABS( xb - nupoint[0].x );
//        dy = ABS( yb - nupoint[0].y );
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