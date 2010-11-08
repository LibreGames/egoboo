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

/// \file object_BSP.cpp
/// \brief Implementation of the BSP for objects
/// \details

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
bool_t ego_obj_BSP::alloc( ego_obj_BSP * pbsp, const int depth )
{
    ego_BSP_tree * rv;

    if ( NULL == pbsp ) return bfalse;

    ego_obj_BSP::dealloc( pbsp );

    // make a 3D BSP tree, depth copied from the mesh depth
    rv = ego_BSP_tree::init( &( pbsp->tree ), 3, depth );

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
    /// \author BB
    /// \details  Create a new BSP tree for the mesh.
    //     These parameters duplicate the max resolution of the old system.

    int          cnt;
    float        bsp_size;
    ego_BSP_tree * t;

    if ( NULL == pbsp || NULL == pmesh_bsp ) return bfalse;

    // allocate the data
    ego_obj_BSP::alloc( pbsp, pmesh_bsp->tree.depth );

    t = &( pbsp->tree );

    // copy the volume from the mesh
    t->bbox.mins[0] = pmesh_bsp->volume.mins[OCT_X];
    t->bbox.mins[1] = pmesh_bsp->volume.mins[OCT_Y];

    t->bbox.maxs[0] = pmesh_bsp->volume.maxs[OCT_X];
    t->bbox.maxs[1] = pmesh_bsp->volume.maxs[OCT_Y];

    // make some extra space in the z direction
    bsp_size = SDL_max( SDL_abs( t->bbox.mins[kX] ), SDL_abs( t->bbox.maxs[kX] ) );
    bsp_size = SDL_max( bsp_size, SDL_max( SDL_abs( t->bbox.mins[kY] ), SDL_abs( t->bbox.maxs[kY] ) ) );
    bsp_size = SDL_max( bsp_size, SDL_max( SDL_abs( t->bbox.mins[kZ] ), SDL_abs( t->bbox.maxs[kZ] ) ) );

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
    ego_BSP_tree::deinit( &( pbsp->tree ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::insert_chr( ego_obj_BSP * pbsp, ego_chr * pchr )
{
    /// \author BB
    /// \details  insert a character's ego_BSP_leaf   into the ego_BSP_tree

    bool_t           retval;
    ego_BSP_leaf   * pleaf;
    ego_BSP_tree * ptree;

    if ( NULL == pbsp ) return bfalse;
    ptree = &( pbsp->tree );

    ego_obj_chr * loc_pobj = ego_chr::get_obj_ptr( pchr );
    if ( !PROCESSING_PBASE( loc_pobj ) ) return bfalse;

    pleaf = &( loc_pobj->bsp_leaf );

    // no interactions with hidden objects
    if ( pchr->is_hidden )
        return bfalse;

    // no interactions with packed objects
    if ( pchr->pack.is_packed )
        return bfalse;

    // no interaction with objects of zero size
    if ( 0 == pchr->bump_stt.size )
        return bfalse;

    // update the object's bsp data
    loc_pobj->update_bsp();

    retval = bfalse;
    if ( !ego_BSP_aabb::empty( pleaf->bbox ) )
    {
        // insert the leaf
        retval = ego_BSP_tree::insert_leaf( ptree, pleaf );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::insert_prt( ego_obj_BSP * pbsp, const ego_bundle_prt & bdl_prt )
{
    /// \author BB
    /// \details  insert a particle's ego_BSP_leaf   into the ego_BSP_tree

    bool_t       retval;
    ego_BSP_leaf * pleaf;
    ego_BSP_tree * ptree;

    ego_prt *loc_pprt;
    ego_pip *loc_ppip;

    bool_t       has_enchant, has_bump_size;
    bool_t       does_damage, does_status_effect, does_special_effect;
    bool_t       needs_bump;

    loc_pprt = bdl_prt.prt_ptr();
    loc_ppip = bdl_prt.pip_ptr();
    if ( NULL == loc_pprt ) return bfalse;

    ego_obj_prt * loc_pobj = ego_prt::get_obj_ptr( loc_pprt );
    if ( NULL == loc_pobj ) return bfalse;
    pleaf = &( loc_pobj->bsp_leaf );

    if ( NULL == pbsp ) return bfalse;
    ptree = &( pbsp->tree );

    if ( !PROCESSING_PPRT( loc_pprt ) || loc_pprt->is_hidden ) return bfalse;

    // Make this optional? Is there any reason to fail if the particle has no profile reference?
    has_enchant = bfalse;
    if ( !LOADED_PRO( loc_pprt->profile_ref ) )
    {
        ego_pro * ppro = ProList.lst + loc_pprt->profile_ref;
        has_enchant = LOADED_EVE( ppro->ieve );
    }

    does_damage = ( SDL_abs( loc_pprt->damage.base ) + SDL_abs( loc_pprt->damage.rand ) ) > 0;

    does_status_effect = ( 0 != loc_ppip->grogtime ) || ( 0 != loc_ppip->dazetime );
    needs_bump     = loc_ppip->end_bump || loc_ppip->end_ground || ( loc_ppip->bumpspawn_amount > 0 ) || ( 0 != loc_ppip->bump_money );
    has_bump_size  = ( 0 != loc_ppip->bump_size ) && ( 0 != loc_ppip->bump_height );

    does_special_effect = loc_ppip->causepancake;

    if ( !has_bump_size && !needs_bump && !has_enchant && !does_damage && !does_status_effect && !does_special_effect )
        return bfalse;

    // update the object's bsp data
    loc_pobj->ego_obj_prt::update_bsp();

    retval = bfalse;
    if ( !ego_BSP_aabb::empty( pleaf->bbox ) )
    {
        // insert the leaf
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
    CHR_BEGIN_LOOP_DEFINED( ichr, pchr )
    {
        pchr_obj->bsp_leaf.inserted = bfalse;
    }
    CHR_END_LOOP();

    // unlink all used particle nodes
    ego_obj_BSP::prt_count = 0;
    PRT_BEGIN_LOOP_DEFINED_BDL( iprt, bdl )
    {
        bdl_ptr_obj->bsp_leaf.inserted = bfalse;
    }
    PRT_END_LOOP();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_BSP::fill( ego_obj_BSP * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    // insert the characters
    ego_obj_BSP::chr_count = 0;
    CHR_BEGIN_LOOP_PROCESSING( ichr, pchr )
    {
        // reset a couple of things here
        pchr->holdingweight             = 0;
        pchr->targetplatform_ref     = CHR_REF( MAX_CHR );
        pchr->targetplatform_overlap = 0.0f;
        pchr->targetmount_ref        = CHR_REF( MAX_CHR );
        pchr->targetmount_overlap    = 0.0f;

        // make sure the leaf is not marked as inserted
        pchr_obj->bsp_leaf.inserted = bfalse;

        // try to insert the character
        ego_obj_BSP::insert_chr( pbsp, pchr );

        // did we succeed?
        if ( pchr_obj->bsp_leaf.inserted )
        {
            ego_obj_BSP::chr_count++;
        }
    }
    CHR_END_LOOP()

    // insert the particles
    ego_obj_BSP::prt_count = 0;
    PRT_BEGIN_LOOP_ALLOCATED_BDL( iprt, prt_bdl )
    {
        // reset a couple of things here
        prt_bdl_ptr->targetplatform_ref     = CHR_REF( MAX_CHR );
        prt_bdl_ptr->targetplatform_overlap = 0.0f;

        // make sure the leaf is not marked as inserted
        prt_bdl_ptr_obj->bsp_leaf.inserted = bfalse;

        // try to insert the particle
        insert_prt( pbsp, prt_bdl );

        if ( prt_bdl_ptr_obj->bsp_leaf.inserted )
        {
            ego_obj_BSP::prt_count++;
        }
    }
    PRT_END_LOOP()

    return btrue;
}

//--------------------------------------------------------------------------------------------
int ego_obj_BSP::collide( ego_obj_BSP * pbsp, ego_BSP_aabb & bbox, leaf_child_list_t & colst )
{
    /// \author BB
    /// \details  fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    if ( NULL == pbsp ) return 0;

    return ego_BSP_tree::collide( &( pbsp->tree ), bbox, colst );
}

////--------------------------------------------------------------------------------------------
//bool_t ego_obj_BSP::insert_leaf( ego_obj_BSP * pbsp, ego_BSP_leaf * pleaf, const int depth, const int address_x[], const int address_y[], const int address_z[] )
//{
//    int i;
//    bool_t retval;
//    Uint32 index;
//    ego_BSP_branch * pbranch, * pbranch_new;
//    ego_BSP_tree * ptree = &( pbsp->tree );
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
//            index = (( const Uint32 )address_x[i] ) | ((( const Uint32 )address_y[i] ) << 1 ) | ((( const Uint32 )address_z[i] ) << 2 ) ;
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
//    /// \author BB
//    /// \details  Recursively search the BSP tree for collisions with the pvobj
//    ///      Return bfalse if we need to break out of the recursive search for any reason.
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
//    /// \author BB
//    /// \details  check for collisions with the given node list
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
