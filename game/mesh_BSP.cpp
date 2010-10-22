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

/// @file mesh_BSP.cpp
/// @brief
/// @details

#include "mesh_BSP.h"

#include "mesh.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

mpd_BSP mpd_BSP::root;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_BSP * mpd_BSP::ctor_this( mpd_BSP * pbsp, ego_mpd   * pmesh )
{
    /// @details BB@> Create a new BSP tree for the mesh.
    //     These parameters duplicate the max resolution of the old system.

    int size_x, size_y;
    int depth;

    if ( NULL == pbsp ) return NULL;

    memset( pbsp, 0, sizeof( *pbsp ) );

    if ( NULL == pmesh ) return pbsp;
    size_x = pmesh->gmem.grids_x;
    size_y = pmesh->gmem.grids_y;

    // determine the number of bifurcations necessary to get cells the size of the "blocks"
    depth = ceil( log( 0.5f * MAX( size_x, size_y ) ) / log( 2.0f ) );

    // make a 2D BSP tree with "max depth" depth
    ego_BSP_tree::ctor_this( &( pbsp->tree ), 2, depth );

    mpd_BSP::alloc( pbsp, pmesh );

    return pbsp;
}

//--------------------------------------------------------------------------------------------
mpd_BSP * mpd_BSP::dtor_this( mpd_BSP * pbsp )
{
    if ( NULL == pbsp ) return NULL;

    // free all allocated memory
    mpd_BSP::dealloc( pbsp );

    // set the volume to zero
    pbsp->volume.mins[OCT_X] = pbsp->volume.maxs[OCT_X] = 0.0f;

    return pbsp;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_BSP::alloc( mpd_BSP * pbsp, ego_mpd * pmesh )
{
    Uint32 i;

    if ( NULL == pbsp || NULL == pmesh ) return bfalse;

    if ( 0 == pmesh->gmem.grid_count ) return bfalse;

    // allocate the ego_BSP_leaf   list, the containers for the actual tiles
    ego_BSP_leaf_ary::alloc( &( pbsp->nodes ), pmesh->gmem.grid_count );
    if ( 0 == pbsp->nodes.size() ) return bfalse;

    // set up the initial bounding volume size
    pbsp->volume = pmesh->tmem.tile_list[0].oct;

    // construct the ego_BSP_leaf   list
    for ( i = 0; i < pmesh->gmem.grid_count; i++ )
    {
        ego_BSP_leaf        * pleaf = pbsp->nodes + i;
        ego_tile_info * ptile = pmesh->tmem.tile_list + i;

        // add the bounding volume for this tile to the bounding volume for the mesh
        ego_oct_bb::do_union( pbsp->volume, ptile->oct, &( pbsp->volume ) );

        // let data type 1 stand for a tile, -1 is uninitialized
        ego_BSP_leaf::ctor_this( pleaf, 2, pmesh->tmem.tile_list + i, 1 );
        pleaf->index = i;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_BSP::dealloc( mpd_BSP * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    // deallocate the tree
    ego_BSP_tree::dealloc( &( pbsp->tree ) );

    // deallocate the nodes
    ego_BSP_leaf_ary::dealloc( &( pbsp->nodes ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_BSP::fill( mpd_BSP * pbsp )
{
    size_t tile;

    for ( tile = 0; tile < pbsp->nodes.top; tile++ )
    {
        ego_tile_info * pdata;
        ego_BSP_leaf        * pleaf = pbsp->nodes + tile;

        // do not deal with uninitialized nodes
        if ( pleaf->data_type <= LEAF_UNKNOWN ) continue;

        // grab the leaf data, assume that it points to the correct data structure
        pdata = ( ego_tile_info* ) pleaf->data;
        if ( NULL == pdata ) continue;

        // calculate the leaf's ego_BSP_aabb
        ego_BSP_aabb::from_oct_bb( &( pleaf->bbox ), &( pdata->oct ) );

        // insert the leaf
        ego_BSP_tree::insert_leaf( &( pbsp->tree ), pleaf );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int mpd_BSP::collide( mpd_BSP * pbsp, ego_BSP_aabb * paabb, ego_BSP_leaf_pary * colst )
{
    /// @details BB@> fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    if ( NULL == pbsp || NULL == paabb ) return 0;

    if ( NULL == colst ) return 0;

    colst->top = 0;

    if ( 0 == colst->size() ) return 0;

    return ego_BSP_tree::collide( &( pbsp->tree ), paabb, colst );
}

////--------------------------------------------------------------------------------------------
//bool_t mpd_BSP::insert_node( mpd_BSP * pbsp, ego_BSP_leaf * pnode, int depth, int address_x[], int address_y[] )
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
//        pnode->next = ptree->infinite;
//        ptree->infinite = pnode;
//        retval = btrue;
//    }
//    else if ( 0 == depth )
//    {
//        // this can only happen if the tile should be in the root node list
//        pnode->next = ptree->root->nodes;
//        ptree->root->nodes = pnode;
//        retval = btrue;
//    }
//    else
//    {
//        // insert the node into the tree at this point
//        pbranch = ptree->root;
//        for ( i = 0; i < depth; i++ )
//        {
//            index = (( Uint32 )address_x[i] ) + ((( Uint32 )address_y[i] ) << 1 );
//
//            pbranch_new = ego_BSP_tree::ensure_branch( ptree, pbranch, index );
//            if ( NULL == pbranch_new ) break;
//
//            pbranch = pbranch_new;
//        };
//
//        // insert the node in this branch
//        retval = ego_BSP_tree::insert( ptree, pbranch, pnode, -1 );
//    };
//
//    return retval;
//}
//
