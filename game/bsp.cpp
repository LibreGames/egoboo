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

/// \file bsp.c
/// \brief
/// \details

#include "bsp.h"
#include "log.h"

#include "egoboo.h"
#include "egoboo_mem.h"
#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define NODE_THRESHOLD 5

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// private functions

static bool_t ego_BSP_leaf_list_insert( leaf_child_list_t & lst, ego_BSP_leaf * n );
static bool_t ego_BSP_leaf_list_clear( leaf_child_list_t & lst );
static bool_t ego_BSP_leaf_list_collide( leaf_child_list_t & leaf_lst, ego_BSP_aabb & bbox, leaf_child_list_t & colst );

//--------------------------------------------------------------------------------------------
// ego_BSP_aabb
//--------------------------------------------------------------------------------------------

void ego_BSP_aabb::invalidate()
{
    /// \author BB
    /// \brief invert the bounding box to make it invalid

    if ( NULL == this || dim <= 0 ) return;

    for ( size_t cnt = 0; cnt < dim; cnt++ )
    {
        float min_val = std::min( mins[cnt], maxs[cnt] );
        float max_val = std::max( mins[cnt], maxs[cnt] );

        mins[cnt] = max_val;
        maxs[cnt] = min_val;
    }
}

//--------------------------------------------------------------------------------------------
ego_BSP_aabb * ego_BSP_aabb::init( ego_BSP_aabb * pbb, size_t dim )
{
    if ( NULL == pbb ) return NULL;

    ego_BSP_aabb::alloc( pbb, dim );

    if ( pbb->dim != pbb->mins.size() || pbb->dim != pbb->mids.size() || pbb->dim != pbb->maxs.size() )
    {
        ego_BSP_aabb::dealloc( pbb );
    }

    return pbb;
}

//--------------------------------------------------------------------------------------------
ego_BSP_aabb * ego_BSP_aabb::deinit( ego_BSP_aabb * pbb )
{
    if ( NULL == pbb ) return NULL;

    // deallocate everything
    pbb = dealloc( pbb );

    // wipe it
    pbb = clear( pbb );

    return pbb;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::empty( const ego_BSP_aabb & src )
{
    size_t cnt;

    if ( 0 == src.dim ) return btrue;

    for ( cnt = 0; cnt < src.dim; cnt++ )
    {
        if ( src.maxs[cnt] <= src.mins[cnt] )
            return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
ego_BSP_aabb * ego_BSP_aabb::alloc( ego_BSP_aabb * pbb, size_t dim )
{
    if ( NULL == pbb ) return pbb;

    dealloc( pbb );

    pbb->mins.resize( dim );
    pbb->mids.resize( dim );
    pbb->maxs.resize( dim );

    pbb->dim = dim;

    return pbb;
}

//--------------------------------------------------------------------------------------------
ego_BSP_aabb * ego_BSP_aabb::dealloc( ego_BSP_aabb * pbb )
{
    if ( NULL == pbb ) return pbb;

    pbb->mins.clear();
    pbb->mids.clear();
    pbb->maxs.clear();

    pbb->dim = 0;

    return pbb;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::reset( ego_BSP_aabb & src )
{
    /// \author BB
    /// \details  Return this bounding box to an empty state.

    size_t cnt;

    if ( 0 == src.dim ) return bfalse;

    if ( src.mins.empty() || src.mids.empty() || src.maxs.empty() )
    {
        alloc( &src, src.dim );
    }

    for ( cnt = 0; cnt < src.dim; cnt++ )
    {
        src.mins[cnt] = src.mids[cnt] = src.maxs[cnt] = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::lhs_contains_rhs( const ego_BSP_aabb & lhs, const ego_BSP_aabb & rhs )
{
    /// \author BB
    /// \details  Is rhs contained within lhs? If rhs has less dimensions
    ///               than lhs, just check the lowest common dimensions.

    size_t cnt, min_dim;

    min_dim = SDL_min( rhs.dim, lhs.dim );
    if ( min_dim <= 0 ) return bfalse;

    for ( cnt = 0; cnt < min_dim; cnt++ )
    {
        if ( lhs.maxs[cnt] < rhs.maxs[cnt] )
            return bfalse;

        if ( lhs.mins[cnt] > rhs.mins[cnt] )
            return bfalse;

        // inverted aabb?
        if ( lhs.maxs[cnt] < lhs.mins[cnt] )
            return bfalse;

        // inverted aabb?
        if ( rhs.maxs[cnt] < rhs.mins[cnt] )
            return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::lhs_intersects_rhs( const ego_BSP_aabb & lhs, const ego_BSP_aabb & rhs )
{
    /// \author BB
    /// \details  Do psrc1 and psrc2 overlap? If psrc2 has less dimensions
    ///               than psrc1, just check the lowest common dimensions.

    int cnt;
    int min_dim;

    min_dim = SDL_min( rhs.dim, lhs.dim );
    if ( min_dim <= 0 ) return bfalse;

    for ( cnt = 0; cnt < min_dim; cnt++ )
    {
        // are these two tests enough?
        if ( rhs.maxs[cnt] < lhs.mins[cnt] )
            return bfalse;

        if ( rhs.mins[cnt] > lhs.maxs[cnt] )
            return bfalse;

        // inverted aabb?
        if ( lhs.maxs[cnt] < lhs.mins[cnt] )
            return bfalse;

        // inverted aabb?
        if ( rhs.maxs[cnt] < rhs.mins[cnt] )
            return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::from_oct_bb( ego_BSP_aabb & dst, const ego_oct_bb & src )
{
    /// \author BB
    /// \details  do an automatic conversion from an ego_oct_bb   to a ego_BSP_aabb

    size_t cnt;

    if ( dst.dim <= 0 ) return bfalse;

    // this process is a little bit complicated because the
    // order to the OCT_* indices is optimized for a different test.
    if ( 1 == dst.dim )
    {
        dst.mins[0] = src.mins[OCT_X];

        dst.maxs[0] = src.maxs[OCT_X];
    }
    else if ( 2 == dst.dim )
    {
        dst.mins[0] = src.mins[OCT_X];
        dst.mins[1] = src.mins[OCT_Y];

        dst.maxs[0] = src.maxs[OCT_X];
        dst.maxs[1] = src.maxs[OCT_Y];
    }
    else if ( dst.dim >= 3 )
    {
        dst.mins[0] = src.mins[OCT_X];
        dst.mins[1] = src.mins[OCT_Y];
        dst.mins[2] = src.mins[OCT_Z];

        dst.maxs[0] = src.maxs[OCT_X];
        dst.maxs[1] = src.maxs[OCT_Y];
        dst.maxs[2] = src.maxs[OCT_Z];

        // blank any extended dimensions
        for ( cnt = 3; cnt < dst.dim; cnt++ )
        {
            dst.mins[cnt] = dst.maxs[cnt] = 0.0f;
        }
    }

    // find the mid values
    for ( cnt = 0; cnt < dst.dim; cnt++ )
    {
        dst.mids[cnt] = 0.5f * ( dst.mins[cnt] + dst.maxs[cnt] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// ego_BSP_leaf
//--------------------------------------------------------------------------------------------

//ego_BSP_leaf * ego_BSP_leaf::create( int dim, void * data, int type )
//{
//    ego_BSP_leaf * rv;
//
//    rv = new ego_BSP_leaf( dim, data, type );
//    if ( NULL == rv ) return rv;
//
//    return rv;
//}

//--------------------------------------------------------------------------------------------
//bool_t ego_BSP_leaf::destroy( ego_BSP_leaf ** ppleaf )
//{
//    if ( NULL == ppleaf || NULL == *ppleaf ) return bfalse;
//
//    ego_BSP_leaf::dealloc( *ppleaf );
//
//    EGOBOO_DELETE( *ppleaf );
//
//    return btrue;
//}
//
//--------------------------------------------------------------------------------------------
ego_BSP_leaf * ego_BSP_leaf::alloc( ego_BSP_leaf * L, int dim )
{
    if ( NULL == L ) return L;

    L = dealloc( L );
    if ( NULL == L ) return L;

    if ( dim > 0 )
    {
        ego_BSP_aabb::alloc( &( L->bbox ), dim );
    }

    return L;
}

//--------------------------------------------------------------------------------------------
ego_BSP_leaf * ego_BSP_leaf::init( ego_BSP_leaf * L, int dim, void * data, int type )
{
    if ( NULL == L ) return L;

    L->data_type = type;
    L->data      = data;

    dealloc( L );
    alloc( L, dim );

    return L;
}

//--------------------------------------------------------------------------------------------
ego_BSP_leaf * ego_BSP_leaf::dealloc( ego_BSP_leaf * L )
{
    if ( NULL == L ) return bfalse;

    ego_BSP_aabb::dealloc( &( L->bbox ) );

    return L;
}

//--------------------------------------------------------------------------------------------
ego_BSP_leaf * ego_BSP_leaf::deinit( ego_BSP_leaf * L )
{
    if ( NULL == L ) return bfalse;

    dealloc( L );

    return clear( L );
}

//--------------------------------------------------------------------------------------------
// ego_BSP_branch
//--------------------------------------------------------------------------------------------

//ego_BSP_branch * ego_BSP_branch::create( size_t dim )
//{
//    ego_BSP_branch * rv;
//
//    rv = new ego_BSP_branch( dim );
//    if ( NULL == rv ) return rv;
//
//    /* add something here */
//
//    return rv;
//}

//--------------------------------------------------------------------------------------------
//bool_t ego_BSP_branch::destroy( ego_BSP_branch ** ppbranch )
//{
//    if ( NULL == ppbranch || NULL == *ppbranch ) return bfalse;
//
//    ego_BSP_branch::dealloc( *ppbranch );
//
//    EGOBOO_DELETE( *ppbranch );
//
//    return btrue;
//}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_branch::alloc( ego_BSP_branch * B, size_t dim )
{

    ego_BSP_branch::dealloc( B );

    if ( 0 == dim ) return B;

    // determine the number of children from the number of dimensions
    size_t child_count = ( 0 == dim ) ? 0 : 2 << ( dim - 1 );

    // allocate the child list
    B->child_lst.resize( child_count );
    EGOBOO_ASSERT( child_count == B->child_lst.size() )

    // initialize the elements
    for ( size_t cnt = 0; cnt < child_count; cnt++ )
    {
        B->child_lst[cnt] = NULL;
    }

    // allocate the branch's bounding box
    ego_BSP_aabb::alloc( &( B->bbox ), dim );

    return B;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_branch::init( ego_BSP_branch * B, size_t dim )
{
    if ( NULL == B ) return B;

    B->parent    = NULL;
    B->depth     = -1;
    B->do_insert = bfalse;

    B = dealloc( B );
    B = alloc( B, dim );

    /* add something here */

    return B;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_branch::dealloc( ego_BSP_branch * B )
{
    if ( NULL == B ) return B;

    ego_BSP_aabb::dealloc( &( B->bbox ) );

    B->child_lst.clear();

    dealloc_nodes( B, bfalse );

    return B;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_branch::deinit( ego_BSP_branch * B )
{
    if ( NULL == B ) return B;

    B = dealloc( B );
    B = clear( B );

    return B;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::unlink( ego_BSP_branch * B )
{
    // remove any links to other leaves
    // assume the user knows what they are doing

    Uint32 i;

    if ( NULL == B ) return bfalse;

    // remove any nodes (from this branch only, not recursively)
    dealloc_nodes( B, bfalse );

    // completely unlink the branch
    B->parent = NULL;
    for ( i = 0; i < B->child_lst.size(); i++ )
    {
        if ( NULL != B->child_lst[i] )
        {
            B->child_lst[i]->parent = NULL;
        }
        B->child_lst[i] = NULL;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::insert_leaf( ego_BSP_branch * B, ego_BSP_leaf * n )
{
    std::pair <leaf_child_list_t::iterator, bool> rv = B->node_set.insert( n );

    n->inserted = rv.second;

    return rv.second;
}

//--------------------------------------------------------------------------------------------
bool_t  ego_BSP_branch::insert_branch( ego_BSP_branch * B, int index, ego_BSP_branch * B2 )
{
    if ( NULL == B || index < 0 || ( size_t )index >= B->child_lst.size() ) return bfalse;

    if ( NULL == B2 ) return bfalse;

    if ( NULL != B->child_lst[ index ] )
    {
        return bfalse;
    }

    EGOBOO_ASSERT( B->depth + 1 == B2->depth );

    B->child_lst[ index ] = B2;
    B2->parent            = B;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::clear_nodes( ego_BSP_branch * B, bool_t recursive )
{
    size_t cnt;

    if ( NULL == B ) return bfalse;

    if ( recursive )
    {
        // recursively clear out any nodes in the child_lst
        for ( cnt = 0; cnt < B->child_lst.size(); cnt++ )
        {
            if ( NULL == B->child_lst[cnt] ) continue;

            ego_BSP_branch::clear_nodes( B->child_lst[cnt], btrue );
        };
    }

    // clear the node list
    if ( !B->node_set.empty() )
    {
        B->node_set.clear();
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::dealloc_nodes( ego_BSP_branch * B, bool_t recursive )
{
    size_t cnt;

    if ( NULL == B ) return bfalse;

    // free all nodes of this branch
    ego_BSP_leaf_list_clear( B->node_set );

    // recursively clear out the children?
    if ( recursive )
    {
        for ( cnt = 0; cnt < B->child_lst.size(); cnt++ )
        {
            if ( NULL == B->child_lst[cnt] ) continue;

            dealloc_nodes( B->child_lst[cnt], btrue );
        };
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::prune_one_branch( ego_BSP_tree   * t, ego_BSP_branch * B, bool_t recursive )
{
    /// \author BB
    /// \details  remove all leaves with no child_lst. Do a depth first recursive search for efficiency

    size_t i;
    bool_t   retval;

    if ( NULL == B ) return bfalse;

    // prune all of the children 1st
    if ( recursive )
    {
        // prune all the children
        for ( i = 0; i < B->child_lst.size(); i++ )
        {
            ego_BSP_tree::prune_one_branch( t, B->child_lst[i], btrue );
        }
    }

    // do not remove the root node
    if ( B == t->root ) return btrue;

    retval = btrue;
    if ( ego_BSP_branch::empty( B ) )
    {
        if ( NULL != B->parent )
        {
            bool_t found = bfalse;

            // unlink the parent and return the node to the free list
            for ( i = 0; i < B->parent->child_lst.size(); i++ )
            {
                if ( B->parent->child_lst[i] == B )
                {
                    B->parent->child_lst[i] = NULL;
                    found = btrue;
                    break;
                }
            }
            EGOBOO_ASSERT( found );
        }

        t->branch_free.push( B );

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::add_all_nodes( ego_BSP_branch * pbranch, leaf_child_list_t & colst )
{
    size_t cnt;

    if ( NULL == pbranch ) return bfalse;

    // add any nodes in the node_set
    leaf_child_list_t::iterator it;
    for ( it = pbranch->node_set.begin(); it != pbranch->node_set.end(); it++ )
    {
        if ( NULL == *it ) continue;

        colst.insert( *it );
    }

    // add all nodes from all children
    for ( cnt = 0; cnt < pbranch->child_lst.size(); cnt++ )
    {
        ego_BSP_branch * pchild = pbranch->child_lst[cnt];
        if ( NULL == pchild ) continue;

        ego_BSP_branch::add_all_nodes( pchild, colst );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::empty( ego_BSP_branch * pbranch )
{
    size_t cnt;
    bool_t empty;

    if ( NULL == pbranch ) return bfalse;

    // look to see if all children are free
    empty = btrue;
    for ( cnt = 0; cnt < pbranch->child_lst.size(); cnt++ )
    {
        if ( NULL != pbranch->child_lst[cnt] )
        {
            empty = bfalse;
            break;
        }
    }

    // check to see if there are any nodes in this branch's node_set

    return pbranch->node_set.empty();
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::collide( ego_BSP_branch * pbranch, ego_BSP_aabb & bbox, leaf_child_list_t & colst )
{
    /// \author BB
    /// \details  Recursively search the BSP tree for collisions with the paabb
    //      Return bfalse if we need to break out of the recursive search for any reason.

    size_t       cnt;
    ego_BSP_aabb * pbranch_bb;

    // if the branch doesn't exist, stop
    if ( NULL == pbranch ) return bfalse;
    pbranch_bb = &( pbranch->bbox );

    // is the branch completely contained by the test aabb?
    if ( ego_BSP_aabb::lhs_contains_rhs( bbox, pbranch->bbox ) )
    {
        // add every single node under this branch
        return ego_BSP_branch::add_all_nodes( pbranch, colst );
    }

    // return if the object does not intersect the branch
    if ( !ego_BSP_aabb::lhs_intersects_rhs( bbox, pbranch->bbox ) )
    {
        // the branch and the object do not overlap at all.
        // do nothing.
        return bfalse;
    }

    // check the node_set
    ego_BSP_leaf_list_collide( pbranch->node_set, bbox, colst );

    // check for collisions with all child_lst branches
    for ( cnt = 0; cnt < pbranch->child_lst.size(); cnt++ )
    {
        ego_BSP_branch * pchild = pbranch->child_lst[cnt];

        // scan all the child_lst
        if ( NULL == pchild ) continue;

        ego_BSP_branch::collide( pchild, bbox, colst );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t ego_BSP_tree::free_allocation_lists( ego_BSP_tree   * t )
{
    if ( NULL == t ) return bfalse;

    // stop tracking all free elements
    while ( !t->branch_free.empty() )
    {
        t->branch_free.pop();
    }

    // stop tracking all used elements
    if ( !t->branch_used.empty() )
    {
        t->branch_used.clear();
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::init_0( ego_BSP_tree   * t )
{
    /// \author BB
    /// \details  reset the tree to the "empty" state. Assume we do not own the node_set or child_lst.

    size_t i;
    ego_BSP_branch * pbranch;

    // free any the nodes in the tree
    dealloc_nodes( t, bfalse );

    // initialize the leaves.
    free_allocation_lists( t );

    for ( i = 0; i < t->branch_all.size(); i++ )
    {
        // grab a branch off of the static list
        pbranch = &( t->branch_all[i] );

        if ( NULL == pbranch ) continue;

        // completely unlink the branch
        ego_BSP_branch::unlink( pbranch );

        // push it onto the "stack"
        t->branch_free.push( pbranch );
    };

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_BSP_tree   * ego_BSP_tree::init( ego_BSP_tree   * t, Sint32 dim, Sint32 depth )
{
    size_t cnt;

    if ( NULL == t ) return t;

    // reset the depth in case of an error
    t->depth = -1;
    free_allocation_lists( t );

    //---- allocate everything
    if ( !ego_BSP_tree::alloc( t, dim, depth ) ) return t;

    //---- initialize everything
    t->depth = depth;

    // initialize the free list
    free_allocation_lists( t );
    for ( cnt = 0; cnt < t->branch_all.size(); cnt++ )
    {
        t->branch_free.push( &( t->branch_all[cnt] ) );
    }

    return t;
}

//--------------------------------------------------------------------------------------------
ego_BSP_tree * ego_BSP_tree::deinit( ego_BSP_tree   * t )
{
    if ( NULL == t ) return NULL;

    ego_BSP_tree::dealloc( t );

    return clear( t );
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::alloc( ego_BSP_tree * t, Sint32 dim, Sint32 depth )
{
    int    fake_node_count;

    if ( NULL == t ) return bfalse;

    ego_BSP_tree::dealloc( t );

    fake_node_count = ego_BSP_tree::count_nodes( dim, depth );
    if ( fake_node_count < 0 ) return bfalse;

    size_t node_count = size_t( fake_node_count );

    // re-initialize the variables
    t->dimensions = 0;

    // allocate the branches
    t->branch_all.resize( node_count );
    if ( node_count != t->branch_all.size() ) return bfalse;

    // initialize the array branches
    for ( size_t cnt = 0; cnt < node_count; cnt++ )
    {
        ego_BSP_branch::alloc( &( t->branch_all[cnt] ), dim );
    }

    // initialize the root bounding box
    ego_BSP_aabb::alloc( &( t->bbox ), dim );

    // set the variables
    t->dimensions = dim;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::dealloc( ego_BSP_tree   * t )
{
    size_t i;

    if ( NULL == t ) return bfalse;

    if ( t->branch_all.empty() ) return btrue;

    // destruct the branches
    for ( i = 0; i < t->branch_all.size(); i++ )
    {
        ego_BSP_branch::dealloc( &( t->branch_all[i] ) );
    }

    // deallocate the branches
    t->branch_all.resize( 0 );

    // deallocate the aux arrays
    free_allocation_lists( t );

    // deallocate the root bounding box
    ego_BSP_aabb::dealloc( &( t->bbox ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Sint32 ego_BSP_tree::count_nodes( Sint32 dim, Sint32 depth )
{
    int itmp;
    Sint32 node_count;
    Uint32 numer, denom;

    itmp = dim * ( depth + 1 );
    if ( itmp > 31 ) return -1;

    numer = ( 1 << itmp ) - 1;
    denom = ( 1 <<  dim ) - 1;
    node_count    = numer / denom;

    return node_count;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::remove_used( ego_BSP_tree   * t, ego_BSP_branch * B )
{
    if ( NULL == t || NULL == B ) return bfalse;

    t->branch_used.erase( B );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::dealloc_branch( ego_BSP_tree   * t, ego_BSP_branch * B )
{
    if ( NULL == t || NULL == B ) return bfalse;

    t->branch_used.erase( B );
    t->branch_free.push( B );

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_tree::get_free( ego_BSP_tree   * t )
{
    ego_BSP_branch *  B, * tmp;

    if ( NULL == t ) return NULL;

    // grab a valid value
    B = tmp = NULL;
    while ( !t->branch_free.empty() )
    {
        tmp = t->branch_free.top();
        t->branch_free.pop();

        if ( t->branch_used.empty() || t->branch_used.end() == t->branch_used.find( tmp ) )
        {
            B = tmp;
            break;
        }
    }

    if ( NULL != B )
    {
        // make sure that this branch does not have data left over
        // from its last use
        EGOBOO_ASSERT( B->node_set.empty() );

        // make sure that the data is cleared out
        ego_BSP_branch::unlink( B );
    }

    return B;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::add_free( ego_BSP_tree   * t, ego_BSP_branch * B )
{
    if ( NULL == t || NULL == B ) return bfalse;

    // remove any links to other leaves
    ego_BSP_branch::unlink( B );

    return ego_BSP_tree::dealloc_branch( t, B );
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::dealloc_all( ego_BSP_tree * t )
{
    if ( !dealloc_nodes( t, btrue ) ) return bfalse;

    if ( !dealloc_branches( t ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::clear_nodes( ego_BSP_tree   * t, bool_t recursive )
{
    if ( NULL == t ) return bfalse;

    if ( recursive )
    {
        ego_BSP_branch::clear_nodes( t->root, btrue );
    }

    // free the infinite nodes of the tree
    ego_BSP_leaf_list_clear( t->infinite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::dealloc_nodes( ego_BSP_tree   * t, bool_t recursive )
{
    if ( NULL == t ) return bfalse;

    if ( recursive )
    {
        ego_BSP_branch::dealloc_nodes( t->root, btrue );
    }

    // free the infinite nodes of the tree
    ego_BSP_leaf_list_clear( t->infinite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::dealloc_branches( ego_BSP_tree   * t )
{
    if ( NULL == t ) return bfalse;

    // transfer all the "used" branches back to the "free" branches
    branch_set_t::iterator it;
    for ( it = t->branch_used.begin(); it != t->branch_used.end(); it++ )
    {
        // grab a used branch
        if ( NULL == *it ) continue;

        // completely unlink the branch
        ego_BSP_branch::unlink( *it );

        // return the branch to the free list
        t->branch_free.push( *it );
    }

    // reset the used list
    if ( !t->branch_used.empty() )
    {
        t->branch_used.clear();
    }

    // remove the tree root
    t->root = NULL;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t   ego_BSP_tree::prune( ego_BSP_tree   * t )
{
    /// \author BB
    /// \details  remove all leaves with no child_lst or node_set.

    if ( NULL == t || NULL == t->root ) return bfalse;

    if ( t->branch_used.empty() ) return btrue;

    // search through all allocated branches. This will not catch all of the
    // empty branches every time, but it should catch quite a few

    // defer all deletions until we are finished with scanning t->branch_used
    std::stack< ego_BSP_branch * > deferred;

    ego_BSP_tree::branch_set_t::iterator it;
    for ( it = t->branch_used.begin(); it != t->branch_used.end(); it++ )
    {
        if ( NULL == *it ) continue;

        if ( ego_BSP_tree::prune_branch( t, *it ) )
        {
            deferred.push( *it );
        }
    }

    // actually do the deletions
    while ( !deferred.empty() )
    {
        ego_BSP_tree::dealloc_branch( t, deferred.top() );
        deferred.pop();
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_tree::ensure_root( ego_BSP_tree   * t )
{
    size_t         cnt;
    ego_BSP_branch * proot;

    if ( NULL == t ) return NULL;

    if ( NULL != t->root ) return t->root;

    proot = ego_BSP_tree::get_free( t );
    if ( NULL == proot ) return NULL;

    // make sure that it is unlinked
    ego_BSP_branch::unlink( proot );

    // copy the tree bounding box to the root node
    for ( cnt = 0; cnt < t->dimensions; cnt++ )
    {
        proot->bbox.mins[cnt] = t->bbox.mins[cnt];
        proot->bbox.mids[cnt] = t->bbox.mids[cnt];
        proot->bbox.maxs[cnt] = t->bbox.maxs[cnt];
    }

    // fix the depth
    proot->depth = 0;

    // assign the root to the tree
    t->root = proot;

    return proot;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_tree::ensure_branch( ego_BSP_tree   * t, ego_BSP_branch * B, int index )
{
    ego_BSP_branch * pbranch;

    if (( NULL == t ) || ( NULL == B ) ) return NULL;
    if ( index < 0 || ego_sint( index ) > ego_sint( B->child_lst.size() ) ) return NULL;

    // grab any existing value
    pbranch = B->child_lst[index];

    // if this branch doesn't exist, create it and insert it properly.
    if ( NULL == pbranch )
    {
        // grab a free branch
        pbranch = ego_BSP_tree::get_free( t );

        if ( NULL != pbranch )
        {
            // make sure that it is unlinked
            ego_BSP_branch::unlink( pbranch );

            // generate its bounding box
            pbranch->depth = B->depth + 1;
            ego_BSP_tree::generate_aabb_child( &( B->bbox ), index, &( pbranch->bbox ) );

            // insert it in the correct position
            ego_BSP_branch::insert_branch( B, index, pbranch );
        }
    }

    return pbranch;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::insert( ego_BSP_tree   * t, ego_BSP_branch * B, ego_BSP_leaf * n, int index )
{
    bool_t retval;

    if (( NULL == t ) || ( NULL == B ) || ( NULL == n ) ) return bfalse;
    if ( ego_sint( index ) > ego_sint( B->child_lst.size() ) ) return bfalse;

    if ( index >= 0 && NULL != B->child_lst[index] )
    {
        // inserting a node into the child
        return ego_BSP_branch::insert_leaf( B->child_lst[index], n );
    }

    if ( index < 0 || t->branch_free.empty() )
    {
        // inserting a node into this branch node
        // this can either occur because someone requested it (index < 0)
        // OR because there are no more free nodes
        retval = ego_BSP_branch::insert_leaf( B, n );
    }
    else
    {
        // the requested B->child_lst[index] slot is empty. grab a pre-allocated
        // ego_BSP_branch   from the free list in the ego_BSP_tree   structure an insert it in
        // this child node

        ego_BSP_branch * pbranch = ego_BSP_tree::ensure_branch( t, B, index );

        retval = ego_BSP_branch::insert_leaf( pbranch, n );
    }

    // something went wrong ?
    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::insert_infinite( ego_BSP_tree   * ptree, ego_BSP_leaf * pleaf )
{
    return ego_BSP_leaf_list_insert( ptree->infinite, pleaf );
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::insert_leaf( ego_BSP_tree   * ptree, ego_BSP_leaf * pleaf )
{
    bool_t retval;

    if ( NULL == ptree || NULL == pleaf ) return bfalse;

    if ( !ego_BSP_aabb::lhs_contains_rhs( ptree->bbox, pleaf->bbox ) )
    {
        // put the leaf at the head of the infinite list
        retval = ego_BSP_tree::insert_infinite( ptree, pleaf );
    }
    else
    {
        ego_BSP_branch * proot = ego_BSP_tree::ensure_root( ptree );

        retval = ego_BSP_tree::insert_leaf_rec( ptree, proot, pleaf, 0 );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::insert_leaf_rec( ego_BSP_tree   * ptree, ego_BSP_branch * pbranch, ego_BSP_leaf * pleaf, int depth )
{
    /// \author BB
    /// \details  recursively insert a leaf in a tree of ego_BSP_branch  *. Get new branches using the
    ///              ego_BSP_tree::get_free() function to allocate any new branches that are needed.

    size_t cnt;
    size_t index;
    bool_t fail;

    ego_BSP_branch * pchild;

    std::stack<ego_BSP_leaf *> tmp_stack;

    if ( NULL == ptree || NULL == pbranch || NULL == pleaf ) return bfalse;

    // keep track of the tree depth
    depth++;

    // test whether we are going to begin splitting this node
    if ( !pbranch->do_insert && pbranch->node_set.size() > NODE_THRESHOLD )
    {
        pbranch->do_insert = btrue;

        // add all the nodes to tmp_stack
        leaf_child_list_t::iterator it;
        for ( it = pbranch->node_set.begin(); it != pbranch->node_set.end(); it++ )
        {
            if ( NULL == *it ) continue;
            tmp_stack.push( *it );
        }

        // clear out the node set
        ego_BSP_leaf_list_clear( pbranch->node_set );
    }

    // if we aren't going to split this leaf, just insert it in the node list
    if ( !pbranch->do_insert || depth > ptree->depth )
    {
        return ego_BSP_branch::insert_leaf( pbranch, pleaf );
    }

    // add our given node to the tmp_stack
    tmp_stack.push( pleaf );

    bool_t failure = bfalse;
    bool_t retval  = btrue;

    // for each leaf in the tmp_stack, recursively add it to the tree
    while ( !tmp_stack.empty() )
    {
        pleaf = tmp_stack.top();
        tmp_stack.pop();

        //---- determine which child the leaf needs to go under
        /// \note This function is not optimal, since we encode the comparisons
        /// in the 32-bit integer indices, and then may have to decimate index to construct
        /// the child  branch's bounding by calling ego_BSP_tree::generate_aabb_child().
        /// The reason that it is done this way is that we would have to be dynamically
        /// allocating and deallocating memory every time this function is called, otherwise. Big waste of time.
        fail  = bfalse;
        index = 0;
        for ( cnt = 0; cnt < ptree->dimensions; cnt++ )
        {
            if ( pleaf->bbox.mins[cnt] >= pbranch->bbox.mins[cnt] && pleaf->bbox.maxs[cnt] <= pbranch->bbox.mids[cnt] )
            {
                index <<= 1;
                index |= 0;
            }
            else if ( pleaf->bbox.mins[cnt] >= pbranch->bbox.mids[cnt] && pleaf->bbox.maxs[cnt] <= pbranch->bbox.maxs[cnt] )
            {
                index <<= 1;
                index |= 1;
            }
            else if ( pleaf->bbox.mins[cnt] >= pbranch->bbox.mins[cnt] && pleaf->bbox.maxs[cnt] <= pbranch->bbox.maxs[cnt] )
            {
                // this leaf belongs at this node
                break;
            }
            else
            {
                // this leaf is actually bigger than this branch
                fail = btrue;
                break;
            }
        }

        if ( fail )
        {
            // we cannot place this the node under this branch
            failure = btrue;
            continue;
        }

        //---- insert the leaf in the right place
        if ( cnt < ptree->dimensions )
        {
            // place this node at this index
            if ( !ego_BSP_branch::insert_leaf( pbranch, pleaf ) )
            {
                retval = bfalse;
            }
        }
        else if ( index < pbranch->child_lst.size() )
        {
            bool_t created;

            pchild = pbranch->child_lst[index];

            created = bfalse;
            if ( NULL == pchild )
            {
                pchild = ego_BSP_tree::ensure_branch( ptree, pbranch, index );

                created = btrue;
            }

            EGOBOO_ASSERT( pchild->depth == pbranch->depth + 1 );

            if ( NULL != pchild )
            {
                // insert the leaf
                if ( !ego_BSP_tree::insert_leaf_rec( ptree, pchild, pleaf, depth ) )
                {
                    retval = bfalse;
                }
            }
        }
    }

    // not necessary, but best to be thorough
    depth--;

    return retval && !failure;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::prune_branch( ego_BSP_tree   * t, ego_BSP_branch * B )
{
    /// \author BB
    /// \details  an optimized version of iterating through the t->branch_used list
    //                and then calling ego_BSP_tree::prune_branch() on the empty branch. In the old method,
    //                the t->branch_used list was searched twice to find each empty branch. This
    //                function does it only once.

    size_t i;
    bool_t remove;

    if ( NULL == B ) return bfalse;

    // do not remove the root node
    if ( B == t->root ) return btrue;

    remove = bfalse;
    if ( ego_BSP_branch::empty( B ) )
    {
        if ( NULL != B->parent )
        {
            bool_t found = bfalse;

            // unlink this node from its parent
            for ( i = 0; i < B->parent->child_lst.size(); i++ )
            {
                if ( B->parent->child_lst[i] == B )
                {
                    // remove it from the parent's list of children
                    B->parent->child_lst[i] = NULL;

                    // blank out the parent
                    B->parent = NULL;

                    // let them know that we found ourself
                    found = btrue;

                    break;
                }
            }

            // not finding yourself is an error
            EGOBOO_ASSERT( found );
        }

        remove = btrue;
    }

    if ( remove )
    {
        // set B's data to "safe" values
        B->parent = NULL;

        // clear the child list
        for ( i = 0; i < B->child_lst.size(); i++ )
        {
            B->child_lst[i] = NULL;
        }

        // reset the node set
        if ( !B->node_set.empty() )
        {
            B->node_set.clear();
        }

        B->depth = -1;

        ego_BSP_aabb::reset( B->bbox );
    }

    return remove;
}

//--------------------------------------------------------------------------------------------
int ego_BSP_tree::collide( ego_BSP_tree * tree, ego_BSP_aabb & bbox, leaf_child_list_t & colst )
{
    /// \author BB
    /// \details  fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    if ( NULL == tree ) return 0;

    // collide with any "infinite" nodes
    ego_BSP_leaf_list_collide( tree->infinite, bbox, colst );

    // collide with the rest of the tree
    ego_BSP_branch::collide( tree->root, bbox, colst );

    return colst.size();
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::generate_aabb_child( ego_BSP_aabb * psrc, int index, ego_BSP_aabb * pdst )
{
    size_t cnt;
    int    tnc;
    size_t child_lst_count;

    // valid source?
    if ( NULL == psrc || psrc->dim <= 0 ) return bfalse;

    // valid destination?
    if ( NULL == pdst ) return bfalse;

    // valid index?
    child_lst_count = ( 0 == psrc->dim ) ? 0 : 2 << ( psrc->dim - 1 );
    if ( index < 0 || size_t( index ) >= child_lst_count ) return bfalse;

    // make sure that the destination type matches the source type
    if ( pdst->dim != psrc->dim )
    {
        ego_BSP_aabb::alloc( pdst, psrc->dim );
    }

    // determine the bounds
    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        float maxval, minval;

        tnc = ego_sint( psrc->dim ) - 1 - cnt;

        if ( 0 == ( index & ( 1 << tnc ) ) )
        {
            minval = psrc->mins[cnt];
            maxval = psrc->mids[cnt];
        }
        else
        {
            minval = psrc->mids[cnt];
            maxval = psrc->maxs[cnt];
        }

        pdst->mins[cnt] = minval;
        pdst->maxs[cnt] = maxval;
        pdst->mids[cnt] = 0.5f * ( minval + maxval );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_BSP_tree * ego_BSP_tree::clear( ego_BSP_tree * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr->dimensions = 0;
    ptr->depth      = -1;
    ptr->root       = NULL;

    ego_BSP_leaf_list_clear( ptr->infinite );

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_BSP_leaf_list_insert( leaf_child_list_t & lst, ego_BSP_leaf * n )
{
    /// \author BB
    /// \details  Insert a leaf in the list, making sure there are no duplicates.
    ///               Duplicates will cause loops in the list and make it impossible to
    ///               traverse properly.

    if ( NULL == n ) return bfalse;

    if ( n->inserted )
    {
        // hmmm.... what to do?
        log_warning( "%s - trying to insert a ego_BSP_leaf that is claiming to be part of a list already\n", __FUNCTION__ );
    }

    std::pair <leaf_child_list_t::iterator, bool> rv = lst.insert( n );

    return rv.second;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_leaf_list_clear( leaf_child_list_t & lst )
{
    /// \author BB
    /// \details  Clear out the leaf list.

    if ( lst.empty() ) return btrue;

    leaf_child_list_t::iterator it;
    for ( it = lst.begin();  it != lst.end(); it++ )
    {
        if ( NULL == *it ) continue;

        // clean up the node
        ( *it )->inserted = bfalse;
    };

    lst.clear();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_leaf_list_collide( leaf_child_list_t & leaf_lst, ego_BSP_aabb & bbox, leaf_child_list_t & colst )
{
    /// \author BB
    /// \details  check for collisions with the given node list

    bool_t       retval;

    if ( leaf_lst.empty() ) return bfalse;

    for ( leaf_child_list_t::iterator it = leaf_lst.begin(); it != leaf_lst.end(); it++ )
    {
        if ( NULL == *it ) continue;

        if (( *it )->data_type < 0 )
        {
            // hmmm.... what to do?
            log_warning( "%s - a node in a leaf list has an invalid data type\n", __FUNCTION__ );
            continue;
        }

        if ( !( *it )->inserted )
        {
            // hmmm.... what to do?
            log_warning( "%s - a node in a leaf list is claiming to not be inserted\n", __FUNCTION__ );
            continue;
        }

        if ( ego_BSP_aabb::lhs_intersects_rhs( bbox, ( *it )->bbox ) )
        {
            // we have a possible intersection
            std::pair <leaf_child_list_t::iterator, bool> rv = colst.insert(( *it ) );
            if ( rv.second ) break;
        }
    }

    retval = btrue;

    return retval;
}
