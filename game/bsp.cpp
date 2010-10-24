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

/// @file bsp.c
/// @brief
/// @details

#include "bsp.h"
#include "log.h"

#include "egoboo.h"
#include "egoboo_mem.h"
#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// private functions

static bool_t ego_BSP_leaf_list_insert( leaf_child_list_t & lst, ego_BSP_leaf * n );
static bool_t ego_BSP_leaf_list_clear( leaf_child_list_t & lst );
static bool_t ego_BSP_leaf_list_collide( leaf_child_list_t & leaf_lst, ego_BSP_aabb * paabb, leaf_child_list_t & colst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_BSP_aabb * ego_BSP_aabb::ctor_this( ego_BSP_aabb * pbb, size_t dim )
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
ego_BSP_aabb * ego_BSP_aabb::dtor_this( ego_BSP_aabb * pbb )
{
    if ( NULL == pbb ) return NULL;

    // deallocate everything
    float_ary::dealloc( &( pbb->mins ) );
    float_ary::dealloc( &( pbb->mids ) );
    float_ary::dealloc( &( pbb->maxs ) );

    // wipe it
    return clear( pbb );
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::empty( ego_BSP_aabb * psrc )
{
    size_t cnt;

    if ( NULL == psrc || 0 == psrc->dim ) return btrue;

    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        if ( psrc->maxs[cnt] <= psrc->mins[cnt] )
            return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
ego_BSP_aabb * ego_BSP_aabb::alloc( ego_BSP_aabb * pbb, size_t dim )
{
    if ( NULL == pbb ) return pbb;

    ego_BSP_aabb::dealloc( pbb );

    float_ary::alloc( &( pbb->mins ), dim );
    float_ary::alloc( &( pbb->mids ), dim );
    float_ary::alloc( &( pbb->maxs ), dim );

    pbb->dim = dim;

    return pbb;
}

//--------------------------------------------------------------------------------------------
ego_BSP_aabb * ego_BSP_aabb::dealloc( ego_BSP_aabb * pbb )
{
    if ( NULL == pbb ) return pbb;

    float_ary::dealloc( &( pbb->mins ) );
    float_ary::dealloc( &( pbb->mids ) );
    float_ary::dealloc( &( pbb->maxs ) );

    pbb->dim = 0;

    return pbb;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::reset( ego_BSP_aabb * psrc )
{
    /// @details BB@> Return this bounding box to an empty state.

    size_t cnt;

    if ( NULL == psrc ) return bfalse;

    if ( psrc->mins.empty() || psrc->mids.empty() || psrc->maxs.empty() ) return bfalse;

    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        psrc->mins[cnt] = psrc->mids[cnt] = psrc->maxs[cnt] = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::lhs_contains_rhs( ego_BSP_aabb * psrc1, ego_BSP_aabb * psrc2 )
{
    /// @details BB@> Is psrc2 contained within psrc1? If psrc2 has less dimensions
    ///               than psrc1, just check the lowest common dimensions.

    size_t cnt, min_dim;

    if ( NULL == psrc1 || NULL == psrc2 ) return bfalse;

    min_dim = MIN( psrc2->dim, psrc1->dim );
    if ( min_dim <= 0 ) return bfalse;

    for ( cnt = 0; cnt < min_dim; cnt++ )
    {
        // inverted aabb?
        if ( psrc1->maxs[cnt] < psrc1->mins[cnt] )
            return bfalse;

        // inverted aabb?
        if ( psrc2->maxs[cnt] < psrc2->mins[cnt] )
            return bfalse;

        if ( psrc2->maxs[cnt] > psrc1->maxs[cnt] )
            return bfalse;

        if ( psrc2->mins[cnt] < psrc1->mins[cnt] )
            return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::do_overlap( ego_BSP_aabb * psrc1, ego_BSP_aabb * psrc2 )
{
    /// @details BB@> Do psrc1 and psrc2 overlap? If psrc2 has less dimensions
    ///               than psrc1, just check the lowest common dimensions.

    int cnt;
    int min_dim;

    if ( NULL == psrc1 || NULL == psrc2 ) return bfalse;

    min_dim = MIN( psrc2->dim, psrc1->dim );
    if ( min_dim <= 0 ) return bfalse;

    for ( cnt = 0; cnt < min_dim; cnt++ )
    {
        float minval, maxval;

        // inverted aabb?
        if ( psrc1->maxs[cnt] < psrc1->mins[cnt] )
            return bfalse;

        // inverted aabb?
        if ( psrc2->maxs[cnt] < psrc2->mins[cnt] )
            return bfalse;

        minval = MAX( psrc1->mins[cnt], psrc2->mins[cnt] );
        maxval = MIN( psrc1->maxs[cnt], psrc2->maxs[cnt] );

        if ( maxval < minval ) return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_aabb::from_oct_bb( ego_BSP_aabb * pdst, ego_oct_bb   * psrc )
{
    /// @details BB@> do an automatic conversion from an ego_oct_bb   to a ego_BSP_aabb

    size_t cnt;

    if ( NULL == pdst || NULL == psrc ) return bfalse;

    if ( pdst->dim <= 0 ) return bfalse;

    // this process is a little bit complicated because the
    // order to the OCT_* indices is optimized for a different test.
    if ( 1 == pdst->dim )
    {
        pdst->mins[0] = psrc->mins[OCT_X];

        pdst->maxs[0] = psrc->maxs[OCT_X];
    }
    else if ( 2 == pdst->dim )
    {
        pdst->mins[0] = psrc->mins[OCT_X];
        pdst->mins[1] = psrc->mins[OCT_Y];

        pdst->maxs[0] = psrc->maxs[OCT_X];
        pdst->maxs[1] = psrc->maxs[OCT_Y];
    }
    else if ( pdst->dim >= 3 )
    {
        pdst->mins[0] = psrc->mins[OCT_X];
        pdst->mins[1] = psrc->mins[OCT_Y];
        pdst->mins[2] = psrc->mins[OCT_Z];

        pdst->maxs[0] = psrc->maxs[OCT_X];
        pdst->maxs[1] = psrc->maxs[OCT_Y];
        pdst->maxs[2] = psrc->maxs[OCT_Z];

        // blank any extended dimensions
        for ( cnt = 3; cnt < pdst->dim; cnt++ )
        {
            pdst->mins[cnt] = pdst->maxs[cnt] = 0.0f;
        }
    }

    // find the mid values
    for ( cnt = 0; cnt < pdst->dim; cnt++ )
    {
        pdst->mids[cnt] = 0.5f * ( pdst->mins[cnt] + pdst->maxs[cnt] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_BSP_leaf * ego_BSP_leaf::create( int dim, void * data, int type )
{
    ego_BSP_leaf * rv;

    rv = new ego_BSP_leaf( dim, data, type );
    if ( NULL == rv ) return rv;

    return rv;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_leaf::destroy( ego_BSP_leaf ** ppleaf )
{
    if ( NULL == ppleaf || NULL == *ppleaf ) return bfalse;

    ego_BSP_leaf::dealloc( *ppleaf );

    EGOBOO_DELETE( *ppleaf );

    return btrue;
}

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
ego_BSP_leaf * ego_BSP_leaf::ctor_this( ego_BSP_leaf * L, int dim, void * data, int type )
{
    if ( NULL == L ) return L;

    L->data_type = type;
    L->data      = data;

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
ego_BSP_leaf * ego_BSP_leaf::dtor_this( ego_BSP_leaf * L )
{
    if ( NULL == L ) return bfalse;

    L = ego_BSP_leaf::dealloc( L );

    return clear( L );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

ego_BSP_branch * ego_BSP_branch::create( size_t dim )
{
    ego_BSP_branch * rv;

    rv = new ego_BSP_branch( dim );
    if ( NULL == rv ) return rv;

    /* add something here */

    return rv;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::destroy( ego_BSP_branch ** ppbranch )
{
    if ( NULL == ppbranch || NULL == *ppbranch ) return bfalse;

    ego_BSP_branch::dealloc( *ppbranch );

    EGOBOO_DELETE( *ppbranch );

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_branch::alloc( ego_BSP_branch * B, size_t dim )
{

    ego_BSP_branch::dealloc( B );

    if ( 0 == dim ) return B;

    int child_count;

    // determine the number of children from the number of dimensions
    child_count = ( 0 == dim ) ? 0 : 2 << ( dim - 1 );

    // allocate the child list
    B->child_lst.resize( child_count );
    EGOBOO_ASSERT( child_count == B->child_lst.size() )

    // initialize the elements
    for ( int cnt = 0; cnt < child_count; cnt++ )
    {
        B->child_lst[cnt] = NULL;
    }

    // allocate the branch's bounding box
    ego_BSP_aabb::alloc( &( B->bbox ), dim );

    return B;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_branch::ctor_this( ego_BSP_branch * B, size_t dim )
{
    if ( NULL == B ) return B;

    B = ego_BSP_branch::alloc( B, dim );

    /* add something here */

    return B;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_branch::dealloc( ego_BSP_branch * B )
{
    if ( NULL == B ) return B;

    B->child_lst.resize( 0 );

    ego_BSP_aabb::dealloc( &( B->bbox ) );

    B->child_lst.resize( 0 );

    dealloc_nodes( B, btrue );

    return B;
}

//--------------------------------------------------------------------------------------------
ego_BSP_branch * ego_BSP_branch::dtor_this( ego_BSP_branch * B )
{
    if ( NULL == B ) return B;

    B = ego_BSP_branch::dealloc( B );
    if ( NULL == B ) return B;

    return clear( B );
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::unlink( ego_BSP_branch * B )
{
    // remove any links to other leaves
    // assume the user knows what they are doing

    Uint32 i;

    if ( NULL == B ) return bfalse;

    // remove any nodes (from this branch only, not recursively)
    ego_BSP_branch::dealloc_nodes( B, bfalse );

    // completely unlink the branch
    B->parent   = NULL;
    for ( i = 0; i < B->child_lst.size(); i++ )
    {
        if ( NULL != B->child_lst[i] )
        {
            B->child_lst[i]->parent = NULL;
        }
        B->child_lst[i] = NULL;
    }

    if ( !B->node_set.empty() )
    {
        B->node_set.clear();
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_branch::insert_leaf( ego_BSP_branch * B, ego_BSP_leaf * n )
{
    std::pair <leaf_child_list_t::iterator, bool> rv = B->node_set.insert( n );

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

    if ( recursive )
    {
        // recursively clear out any nodes in the child_lst
        for ( cnt = 0; cnt < B->child_lst.size(); cnt++ )
        {
            if ( NULL == B->child_lst[cnt] ) continue;

            ego_BSP_branch::dealloc_nodes( B->child_lst[cnt], btrue );
        };
    }

    // free all nodes of this branch
    ego_BSP_leaf_list_clear( B->node_set );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::prune_one_branch( ego_BSP_tree   * t, ego_BSP_branch * B, bool_t recursive )
{
    /// @details BB@> remove all leaves with no child_lst. Do a depth first recursive search for efficiency

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
    size_t cnt, colst_size;

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
bool_t ego_BSP_branch::collide( ego_BSP_branch * pbranch, ego_BSP_aabb * paabb, leaf_child_list_t & colst )
{
    /// @details BB@> Recursively search the BSP tree for collisions with the paabb
    //      Return bfalse if we need to break out of the recursive search for any reason.

    size_t       cnt;
    ego_BSP_aabb * pbranch_bb;

    // if the branch doesn't exist, stop
    if ( NULL == pbranch ) return bfalse;
    pbranch_bb = &( pbranch->bbox );

    // is the branch completely contained by the test aabb?
    if ( ego_BSP_aabb::lhs_contains_rhs( paabb, pbranch_bb ) )
    {
        // add every single node under this branch
        return ego_BSP_branch::add_all_nodes( pbranch, colst );
    }

    // return if the object does not intersect the branch
    if ( !ego_BSP_aabb::do_overlap( paabb, pbranch_bb ) )
    {
        // the branch and the object do not overlap at all.
        // do nothing.
        return bfalse;
    }

    // check the node_set
    ego_BSP_leaf_list_collide( pbranch->node_set, paabb, colst );

    // check for collisions with all child_lst branches
    for ( cnt = 0; cnt < pbranch->child_lst.size(); cnt++ )
    {
        ego_BSP_branch * pchild = pbranch->child_lst[cnt];

        // scan all the child_lst
        if ( NULL == pchild ) continue;

        ego_BSP_branch::collide( pchild, paabb, colst );
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
    /// @details BB@> reset the tree to the "empty" state. Assume we do not own the node_set or child_lst.

    size_t i;
    ego_BSP_branch * pbranch;

    // free any the nodes in the tree
    ego_BSP_tree::dealloc_nodes( t, bfalse );

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
ego_BSP_tree   * ego_BSP_tree::ctor_this( ego_BSP_tree   * t, Sint32 dim, Sint32 depth )
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
ego_BSP_tree * ego_BSP_tree::dtor_this( ego_BSP_tree   * t )
{
    if ( NULL == t ) return NULL;

    ego_BSP_tree::dealloc( t );

    return clear( t );
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::alloc( ego_BSP_tree * t, Sint32 dim, Sint32 depth )
{
    int    node_count;

    if ( NULL == t ) return bfalse;

    ego_BSP_tree::dealloc( t );

    node_count = ego_BSP_tree::count_nodes( dim, depth );
    if ( node_count < 0 ) return bfalse;

    // re-initialize the variables
    t->dimensions = 0;

    // allocate the branches
    t->branch_all.resize( node_count );
    if ( node_count != t->branch_all.size() ) return bfalse;

    // initialize the array branches
    for ( int cnt = 0; cnt < node_count; cnt++ )
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
bool_t ego_BSP_tree::dealloc_all( ego_BSP_tree   * t )
{
    if ( !ego_BSP_tree::dealloc_nodes( t, bfalse ) ) return bfalse;

    if ( !ego_BSP_tree::dealloc_branches( t ) ) return bfalse;

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
    if ( !t->infinite.empty() )
    {
        t->infinite.clear();
    }

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
    /// @details BB@> remove all leaves with no child_lst or node_set.

    if ( NULL == t || NULL == t->root ) return bfalse;

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

    while ( !deferred.empty() )
    {
        ego_BSP_tree::dealloc_branch( t, deferred.top() );
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
    if ( index < 0 || ( signed )index > ( signed )B->child_lst.size() ) return NULL;

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
    if (( signed )index > ( signed )B->child_lst.size() ) return bfalse;

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

    if ( !ego_BSP_aabb::lhs_contains_rhs( &( ptree->bbox ), &( pleaf->bbox ) ) )
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
    /// @details BB@> recursively insert a leaf in a tree of ego_BSP_branch  *. Get new branches using the
    ///              ego_BSP_tree::get_free() function to allocate any new branches that are needed.

    size_t cnt;
    size_t index;
    bool_t retval, fail;

    ego_BSP_branch * pchild;

    if ( NULL == ptree || NULL == pbranch || NULL == pleaf ) return bfalse;

    // keep track of the tree depth
    depth++;

    // don't go too deep
    if ( depth > ptree->depth )
    {
        // insert the node under this branch
        return ego_BSP_branch::insert_leaf( pbranch, pleaf );
    }

    //---- determine which child the leaf needs to go under
    /// @note This function is not optimal, since we encode the comparisons
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
        return bfalse;
    }

    //---- insert the leaf in the right place
    retval = bfalse;
    if ( cnt < ptree->dimensions )
    {
        // place this node at this index
        retval = ego_BSP_branch::insert_leaf( pbranch, pleaf );
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
            retval = ego_BSP_tree::insert_leaf_rec( ptree, pchild, pleaf, depth );
        }
    }

    // not necessary, but best to be thorough
    depth--;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_tree::prune_branch( ego_BSP_tree   * t, ego_BSP_branch * B )
{
    /// @details BB@> an optimized version of iterating through the t->branch_used list
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

        ego_BSP_aabb::reset( &( B->bbox ) );
    }

    return remove;
}

//--------------------------------------------------------------------------------------------
int ego_BSP_tree::collide( ego_BSP_tree * tree, ego_BSP_aabb * paabb, leaf_child_list_t & colst )
{
    /// @details BB@> fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    if ( NULL == tree || NULL == paabb ) return 0;

    // collide with any "infinite" nodes
    ego_BSP_leaf_list_collide( tree->infinite, paabb, colst );

    // collide with the rest of the tree
    ego_BSP_branch::collide( tree->root, paabb, colst );

    return colst.size();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_BSP_leaf_list_insert( leaf_child_list_t & lst, ego_BSP_leaf * n )
{
    /// @details BB@> Insert a leaf in the list, making sure there are no duplicates.
    ///               Duplicates will cause loops in the list and make it impossible to
    ///               traverse properly.

    if ( NULL == n ) return bfalse;

    if ( n->inserted )
    {
        // hmmm.... what to do?
        log_warning( "ego_BSP_leaf_list_insert() - trying to insert a ego_BSP_leaf that is claiming to be part of a list already\n" );
    }

    std::pair <leaf_child_list_t::iterator, bool> rv = lst.insert( n );

    return rv.second;
}

//--------------------------------------------------------------------------------------------
bool_t ego_BSP_leaf_list_clear( leaf_child_list_t & lst )
{
    /// @details BB@> Clear out the leaf list.

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
bool_t ego_BSP_leaf_list_collide( leaf_child_list_t & leaf_lst, ego_BSP_aabb * paabb, leaf_child_list_t & colst )
{
    /// @details BB@> check for collisions with the given node list

    bool_t       retval;
    size_t       colst_size;

    if ( leaf_lst.empty() ) return bfalse;
    if ( NULL == paabb ) return bfalse;

    leaf_child_list_t::iterator it;
    for ( it = leaf_lst.begin(); it != leaf_lst.end(); it++ )
    {
        if ( NULL == *it ) continue;

        ego_BSP_aabb * pleaf_bb = &(( *it )->bbox );

        EGOBOO_ASSERT(( *it )->data_type > -1 );

        if ( !( *it )->inserted )
        {
            // hmmm.... what to do?
            log_warning( "ego_BSP_leaf_list_collide() - a node in a leaf list is claiming to not be inserted\n" );
        }

        if ( ego_BSP_aabb::do_overlap( paabb, pleaf_bb ) )
        {
            // we have a possible intersection
            std::pair <leaf_child_list_t::iterator, bool> rv = colst.insert(( *it ) );
            if ( rv.second ) break;
        }
    }

    retval = btrue;

    return retval;
}

//--------------------------------------------------------------------------------------------
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

        tnc = (( signed )psrc->dim ) - 1 - cnt;

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

