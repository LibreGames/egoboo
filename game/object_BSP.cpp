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

/// @file object_BSP.cpp
/// @brief Implementation of the BSP for objects
/// @details

#include "object_BSP.h"

#include "mesh_BSP.h"

#include "profile.inl"
#include "char.inl"
#include "particle.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

ego_obj_BSP ego_obj_BSP::root;

int ego_obj_BSP::chr_count = 0;
int ego_obj_BSP::prt_count = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::alloc( ego_obj_BSP * pbsp, int depth )
{
    ego_BSP_tree   * rv;

    if ( NULL == pbsp ) return bfalse;

    ego_obj_BSP::dealloc( pbsp );

    // make a 3D BSP tree, depth copied from the mesh depth
    rv = ego_BSP_tree::ctor_this( &( pbsp->tree ), 3, depth );

    return ( NULL != rv );
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::dealloc( ego_obj_BSP * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    ego_BSP_tree::dealloc( &( pbsp->tree ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::ctor_this( ego_obj_BSP * pbsp, mpd_BSP * pmesh_bsp )
{
    /// @details BB@> Create a new BSP tree for the mesh.
    //     These parameters duplicate the max resolution of the old system.

    int          cnt;
    float        bsp_size;
    ego_BSP_tree   * t;

    if ( NULL == pbsp || NULL == pmesh_bsp ) return bfalse;

    memset( pbsp, 0, sizeof( *pbsp ) );

    // allocate the data
    ego_obj_BSP::alloc( pbsp, pmesh_bsp->tree.depth );

    t = &( pbsp->tree );

    // copy the volume from the mesh
    t->bbox.mins[0] = pmesh_bsp->volume.mins[OCT_X];
    t->bbox.mins[1] = pmesh_bsp->volume.mins[OCT_Y];

    t->bbox.maxs[0] = pmesh_bsp->volume.maxs[OCT_X];
    t->bbox.maxs[1] = pmesh_bsp->volume.maxs[OCT_Y];

    // make some extra space in the z direction
    bsp_size = MAX( ABS( t->bbox.mins[kX] ), ABS( t->bbox.maxs[kX] ) );
    bsp_size = MAX( bsp_size, MAX( ABS( t->bbox.mins[kY] ), ABS( t->bbox.maxs[kY] ) ) );
    bsp_size = MAX( bsp_size, MAX( ABS( t->bbox.mins[kZ] ), ABS( t->bbox.maxs[kZ] ) ) );

    t->bbox.mins[2] = -bsp_size * 2;
    t->bbox.maxs[2] =  bsp_size * 2;

    // calculate the mid positions
    for ( cnt = 0; cnt < 3; cnt++ )
    {
        t->bbox.mids[cnt] = 0.5f * ( t->bbox.mins[cnt] + t->bbox.maxs[cnt] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::dtor_this( ego_obj_BSP * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    // deallocate everything
    ego_obj_BSP::dealloc( pbsp );

    // run the destructors on all of the sub-objects
    ego_BSP_tree::dtor_this( &( pbsp->tree ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::insert_chr( ego_obj_BSP * pbsp, ego_chr * pchr )
{
    /// @details BB@> insert a character's ego_BSP_leaf   into the ego_BSP_tree

    bool_t       retval;
    ego_BSP_leaf * pleaf;
    ego_BSP_tree   * ptree;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    // no interactions with hidden objects
    if ( pchr->is_hidden )
        return bfalse;

    // no interactions with packed objects
    if ( pchr->pack.is_packed )
        return bfalse;

    // no interaction with objects of zero size
    if ( 0 == pchr->bump_stt.size )
        return bfalse;

    if ( NULL == pbsp ) return bfalse;
    ptree = &( pbsp->tree );

    pleaf = &( pchr->bsp_leaf );
    if ( pchr != ( ego_chr * )( pleaf->data ) )
    {
        // some kind of error. re-initialize the data.
        pleaf->data      = pchr;
        pleaf->index     = GET_IDX_PCHR( pchr );
        pleaf->data_type = LEAF_CHR;
    };

    retval = bfalse;
    if ( !ego_oct_bb::empty( pchr->chr_max_cv ) )
    {
        ego_oct_bb   tmp_oct;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_chr_bb( pchr, 0.0f, 1.0f, &tmp_oct );

        // convert the bounding box
        ego_BSP_aabb::from_oct_bb( &( pleaf->bbox ), &tmp_oct );

        // insert the leaf
        retval = ego_BSP_tree::insert_leaf( ptree, pleaf );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::insert_prt( ego_obj_BSP * pbsp, ego_prt_bundle * pbdl_prt )
{
    /// @details BB@> insert a particle's ego_BSP_leaf   into the ego_BSP_tree

    bool_t       retval;
    ego_BSP_leaf * pleaf;
    ego_BSP_tree   * ptree;

    ego_prt *loc_pprt;
    ego_pip *loc_ppip;

    bool_t       has_enchant, has_bump_size;
    bool_t       does_damage, does_status_effect, does_special_effect;
    bool_t       needs_bump;

    if ( NULL == pbsp ) return bfalse;
    ptree = &( pbsp->tree );

    if ( NULL == pbdl_prt ) return bfalse;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    if ( !ACTIVE_PPRT( loc_pprt ) || loc_pprt->is_hidden ) return bfalse;

    // Make this optional? Is there any reason to fail if the particle has no profile reference?
    has_enchant = bfalse;
    if ( !LOADED_PRO( loc_pprt->profile_ref ) )
    {
        ego_pro * ppro = ProList.lst + loc_pprt->profile_ref;
        has_enchant = LOADED_EVE( ppro->ieve );
    }

    does_damage = ( ABS( loc_pprt->damage.base ) + ABS( loc_pprt->damage.rand ) ) > 0;

    does_status_effect = ( 0 != loc_ppip->grogtime ) || ( 0 != loc_ppip->dazetime );
    needs_bump     = loc_ppip->end_bump || loc_ppip->end_ground || ( loc_ppip->bumpspawn_amount > 0 ) || ( 0 != loc_ppip->bump_money );
    has_bump_size  = ( 0 != loc_ppip->bump_size ) && ( 0 != loc_ppip->bump_height );

    does_special_effect = loc_ppip->causepancake;

    if ( !has_bump_size && !needs_bump && !has_enchant && !does_damage && !does_status_effect && !does_special_effect )
        return bfalse;

    pleaf = &( loc_pprt->bsp_leaf );
    if ( loc_pprt != ( ego_prt * )( pleaf->data ) )
    {
        // some kind of error. re-initialize the data.
        pleaf->data      = loc_pprt;
        pleaf->index     = GET_IDX_PPRT( loc_pprt );
        pleaf->data_type = LEAF_PRT;
    };

    retval = bfalse;
    if ( ACTIVE_PPRT( loc_pprt ) )
    {
        ego_oct_bb   tmp_oct;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_prt_bb( loc_pprt, 0.0f, 1.0f, &tmp_oct );

        // convert the bounding box
        ego_BSP_aabb::from_oct_bb( &( pleaf->bbox ), &tmp_oct );

        retval = ego_BSP_tree::insert_leaf( ptree, pleaf );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::empty( ego_obj_BSP * pbsp )
{
    CHR_REF ichr;
    PRT_REF iprt;

    if ( NULL == pbsp ) return bfalse;

    // unlink all the BSP nodes
    ego_BSP_tree::clear_nodes( &( pbsp->tree ), btrue );

    // unlink all used character nodes
    ego_obj_BSP::chr_count = 0;
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        ego_chr * pchr = ChrObjList.get_pdata( ichr );
        if ( NULL == pchr ) continue;

        pchr->bsp_leaf.inserted = bfalse;
    }

    // unlink all used particle nodes
    ego_obj_BSP::prt_count = 0;
    for ( iprt = 0; iprt < MAX_PRT; iprt++ )
    {
        ego_prt * pprt = PrtObjList.get_pdata( iprt );
        if ( NULL == pprt ) continue;

        pprt->bsp_leaf.inserted = bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::fill( ego_obj_BSP * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    // insert the characters
    ego_obj_BSP::chr_count = 0;
    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        // reset a couple of things here
        pchr->holdingweight             = 0;
        pchr->targetplatform_ref     = CHR_REF( MAX_CHR );
        pchr->targetplatform_overlap = 0.0f;
        pchr->targetmount_ref        = CHR_REF( MAX_CHR );
        pchr->targetmount_overlap    = 0.0f;

        // try to insert the character
        if ( ego_obj_BSP::insert_chr( pbsp, pchr ) )
        {
            ego_obj_BSP::chr_count++;
        }
    }
    CHR_END_LOOP()

    // insert the particles
    ego_obj_BSP::prt_count = 0;
    PRT_BEGIN_LOOP_USED( iprt, prt_bdl )
    {
        // reset a couple of things here
        prt_bdl.prt_ptr->targetplatform_ref     = CHR_REF( MAX_CHR );
        prt_bdl.prt_ptr->targetplatform_overlap = 0.0f;

        // try to insert the particle
        if ( ego_obj_BSP::insert_prt( pbsp, &prt_bdl ) )
        {
            ego_obj_BSP::prt_count++;
        }
    }
    PRT_END_LOOP()

    return btrue;
}

//--------------------------------------------------------------------------------------------
int ego_obj_BSP::collide( ego_obj_BSP * pbsp, ego_BSP_aabb * paabb, leaf_child_list_t & colst )
{
    /// @details BB@> fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    if ( NULL == pbsp || NULL == paabb ) return 0;

    return ego_BSP_tree::collide( &( pbsp->tree ), paabb, colst );
}

////--------------------------------------------------------------------------------------------
//bool_t ego_obj_BSP::insert_leaf( ego_obj_BSP * pbsp, ego_BSP_leaf * pleaf, int depth, int address_x[], int address_y[], int address_z[] )
//{
//    int i;
//    bool_t retval;
//    Uint32 index;
//    ego_BSP_branch * pbranch, * pbranch_new;
//    ego_BSP_tree   * ptree = &( pbsp->tree );
//
//    retval = bfalse;
//    if ( depth < 0 )
//    {
//        // this can only happen if the node does not intersect the BSP bounding box
//        pleaf->next = ptree->infinite;
//        ptree->infinite = pleaf;
//        retval = btrue;
//    }
//    else if ( 0 == depth )
//    {
//        // this can only happen if the object should be in the root node list
//        pleaf->next = ptree->root->nodes;
//        ptree->root->nodes = pleaf;
//        retval = btrue;
//    }
//    else
//    {
//        // insert the node into the tree at this point
//        pbranch = ptree->root;
//        for ( i = 0; i < depth; i++ )
//        {
//            index = (( Uint32 )address_x[i] ) | ((( Uint32 )address_y[i] ) << 1 ) | ((( Uint32 )address_z[i] ) << 2 ) ;
//
//            pbranch_new = ego_BSP_tree::ensure_branch( ptree, pbranch, index );
//            if ( NULL == pbranch_new ) break;
//
//            pbranch = pbranch_new;
//        };
//
//        // insert the node in this branch
//        retval = ego_BSP_tree::insert( ptree, pbranch, pleaf, -1 );
//    }
//
//    return retval;
//}
//

////--------------------------------------------------------------------------------------------
//bool_t ego_obj_BSP::collide_branch( ego_BSP_branch * pbranch, ego_oct_bb   * pvbranch, ego_oct_bb   * pvobj, int_ary * colst )
//{
//    /// @details BB@> Recursively search the BSP tree for collisions with the pvobj
//    //      Return bfalse if we need to break out of the recursive search for any reason.
//
//    Uint32 i;
//    ego_oct_bb      int_ov, tmp_ov;
//    float x_mid, y_mid, z_mid;
//    int address_x, address_y, address_z;
//
//    if ( NULL == colst ) return bfalse;
//    if ( NULL == pvbranch || ego_oct_bb::empty( *pvbranch ) ) return bfalse;
//    if ( NULL == pvobj  || ego_oct_bb::empty( *pvobj ) ) return bfalse;
//
//    // return if the object does not intersect the branch
//    if ( !ego_oct_bb::do_intersection( *pvobj, *pvbranch, &int_ov ) )
//    {
//        return bfalse;
//    }
//
//    if ( !ego_obj_BSP::collide_nodes( pbranch->nodes, pvobj, colst ) )
//    {
//        return bfalse;
//    };
//
//    // check for collisions with any of the children
//    x_mid = ( pvbranch->maxs[OCT_X] + pvbranch->mins[OCT_X] ) * 0.5f;
//    y_mid = ( pvbranch->maxs[OCT_Y] + pvbranch->mins[OCT_Y] ) * 0.5f;
//    z_mid = ( pvbranch->maxs[OCT_Z] + pvbranch->mins[OCT_Z] ) * 0.5f;
//    for ( i = 0; i < pbranch->child_count; i++ )
//    {
//        // scan all the children
//        if ( NULL == pbranch->children[i] ) continue;
//
//        // create the volume of this node
//        address_x = i & ( 1 << 0 );
//        address_y = i & ( 1 << 1 );
//        address_z = i & ( 1 << 2 );
//
//        tmp_ov = *( pvbranch );
//
//        if ( 0 == address_x )
//        {
//            tmp_ov.maxs[OCT_X] = x_mid;
//        }
//        else
//        {
//            tmp_ov.mins[OCT_X] = x_mid;
//        }
//
//        if ( 0 == address_y )
//        {
//            tmp_ov.maxs[OCT_Y] = y_mid;
//        }
//        else
//        {
//            tmp_ov.mins[OCT_X] = y_mid;
//        }
//
//        if ( 0 == address_z )
//        {
//            tmp_ov.maxs[OCT_Z] = z_mid;
//        }
//        else
//        {
//            tmp_ov.mins[OCT_Z] = z_mid;
//        }
//
//        if ( ego_oct_bb::do_intersection( *pvobj, tmp_ov, &int_ov ) )
//        {
//            // potential interaction with the child. go recursive!
//            bool_t ret = ego_obj_BSP::collide_branch( pbranch->children[i], &( tmp_ov ), pvobj, colst );
//            if ( !ret ) return ret;
//        }
//    }
//
//    return btrue;
//}
//

////--------------------------------------------------------------------------------------------
//bool_t ego_obj_BSP::collide_nodes( ego_BSP_leaf   leaf_lst[], ego_oct_bb   * pvobj, int_ary * colst )
//{
//    /// @details BB@> check for collisions with the given node list
//
//    ego_BSP_leaf * pleaf;
//    ego_oct_bb      int_ov, * pnodevol;
//
//    if ( NULL == leaf_lst || NULL == pvobj ) return bfalse;
//
//    if ( 0 == int_ary_get_size( colst ) || int_ary_get_top( colst ) >= int_ary_get_size( colst ) ) return bfalse;
//
//    // check for collisions with any of the nodes of this branch
//    for ( pleaf = leaf_lst; NULL != pleaf; pleaf = pleaf->next )
//    {
//        if ( NULL == pleaf ) EGOBOO_ASSERT( bfalse );
//
//        // get the volume of the node
//        pnodevol = NULL;
//        if ( LEAF_CHR == pleaf->data_type )
//        {
//            ego_chr * pchr = ( ego_chr* )pleaf->data;
//            pnodevol = &( pchr->prt_cv );
//        }
//        else if ( LEAF_PRT == pleaf->data_type )
//        {
//            ego_prt * pprt = ( ego_prt* )pleaf->data;
//            pnodevol = &( pprt->prt_cv );
//        }
//        else
//        {
//            continue;
//        }
//
//        if ( ego_oct_bb::do_intersection( *pvobj, *pnodevol, &int_ov ) )
//        {
//            // we have a possible intersection
//            int_ary_push_back( colst, pleaf->index *(( LEAF_CHR == pleaf->data_type ) ? 1 : -1 ) );
//
//            if ( int_ary_get_top( colst ) >= int_ary_get_size( colst ) )
//            {
//                // too many nodes. break out of the search.
//                return bfalse;
//            };
//        }
