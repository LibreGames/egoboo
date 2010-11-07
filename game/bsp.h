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

/// \file bsp.h
/// \details

#include "egoboo_typedef_cpp.h"

#include "bbox.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_BSP_leaf;
struct ego_BSP_branch;

typedef std::vector< ego_BSP_branch   > branch_list_t;
typedef std::vector< ego_BSP_branch * > branch_child_list_t;

typedef std::vector< ego_BSP_leaf > leaf_list_t;

/// use a EGOBOO_SET to ensure only version of each node pointer gets inserted
#if defined(USE_HASH) && defined(__GNUC__)
typedef EGOBOO_SET< ego_BSP_leaf * , LargeIntHasher<ego_BSP_leaf> > leaf_child_list_t;
#else
typedef EGOBOO_SET< ego_BSP_leaf * > leaf_child_list_t;
#endif
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_BSP_aabb
{
    size_t             dim;
    std::vector<float> mins;
    std::vector<float> mids;
    std::vector<float> maxs;

    ego_BSP_aabb( const size_t _dim = 0 ) : mins( _dim ), maxs( _dim ), mids( _dim ) { clear( this ); init( this, _dim ); };
    ~ego_BSP_aabb()                { deinit( this ); };

    void invalidate();

    static ego_BSP_aabb * init( ego_BSP_aabb * pbb, const size_t dim );
    static ego_BSP_aabb * deinit( ego_BSP_aabb * pbb );

    static ego_BSP_aabb * alloc( ego_BSP_aabb * pbb, const size_t dim );
    static ego_BSP_aabb * dealloc( ego_BSP_aabb * pbb );

    static bool_t         reset( ego_BSP_aabb & src );
    static bool_t         empty( const ego_BSP_aabb & src1 );
    static bool_t         lhs_contains_rhs( const ego_BSP_aabb & lhs, const ego_BSP_aabb & rhs );
    static bool_t         lhs_intersects_rhs( const ego_BSP_aabb & lhs, const ego_BSP_aabb & rhs );

    static bool_t         from_oct_bb( ego_BSP_aabb & dst, const ego_oct_bb & src );

private:
    static ego_BSP_aabb * clear( ego_BSP_aabb * ptr )
    {
        if ( NULL == ptr ) return ptr;

        ptr->dim = 0;

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------
struct ego_BSP_leaf
{
    bool_t         inserted;

    int            data_type;
    void         * data;
    size_t         index;

    ego_BSP_aabb   bbox;

    ego_BSP_leaf( const int _dim = 0, void * _data = NULL, const int _type = -1 ) { clear( this ); init( this, _dim, _data, _type ); } ;
    ~ego_BSP_leaf() { deinit( this ); };

    //static ego_BSP_leaf * create( const int dim, void * data, const int type );
    //static bool_t         destroy( ego_BSP_leaf ** ppleaf );

    static ego_BSP_leaf * init( ego_BSP_leaf * t, const int dim, void * data, const int type );
    static ego_BSP_leaf * deinit( ego_BSP_leaf * t );

    static ego_BSP_leaf * alloc( ego_BSP_leaf * L, const int dim );
    static ego_BSP_leaf * dealloc( ego_BSP_leaf * L );

    void invalidate() { clear( this ); }

private:
    static ego_BSP_leaf * clear( ego_BSP_leaf * ptr )
    {
        if ( NULL == ptr ) return ptr;

        ptr->inserted = bfalse;

        ptr->data_type = -1;
        ptr->data = NULL;
        ptr->index = size_t( -1 );

        ptr->bbox.invalidate();

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------
struct ego_BSP_branch
{
    friend struct ego_BSP_tree;

    ego_BSP_branch  * parent;

    branch_child_list_t    child_lst;
    leaf_child_list_t      node_set;

    ego_BSP_aabb      bbox;
    int               depth;
    bool_t            do_insert;

    //static ego_BSP_branch * create( const size_t dim );
    //static bool_t           destroy( ego_BSP_branch ** ppbranch );

    static ego_BSP_branch * create_ary( const size_t ary_size, const size_t dim );
    static bool_t           destroy_ary( const size_t ary_size, ego_BSP_branch ** ppbranch );

    static bool_t           empty( ego_BSP_branch * pbranch );

    static ego_BSP_branch * alloc( ego_BSP_branch * B, const size_t dim );
    static ego_BSP_branch * dealloc( ego_BSP_branch * B );

    static bool_t           insert_leaf( ego_BSP_branch * B, ego_BSP_leaf * n );
    static bool_t           insert_branch( ego_BSP_branch * B, const int index, ego_BSP_branch * B2 );
    static bool_t           clear_nodes( ego_BSP_branch * B, const bool_t recursive = bfalse );
    static bool_t           free_nodes( ego_BSP_branch * B, const bool_t recursive = bfalse );
    static bool_t           unlink( ego_BSP_branch * B );
    static bool_t           add_all_nodes( ego_BSP_branch * pbranch, leaf_child_list_t & colst );

    ego_BSP_branch( const size_t dim = 0 ) { clear( this ); init( this, dim ); }
    ~ego_BSP_branch()                { deinit( this ); }

private:

    static ego_BSP_branch * init( ego_BSP_branch * B, const size_t dim );
    static ego_BSP_branch * deinit( ego_BSP_branch * B );

    static ego_BSP_branch * clear( ego_BSP_branch * ptr )
    {
        if ( NULL == ptr ) return ptr;

        ptr->parent = NULL;

        ptr->child_lst.resize( 0 );
        if ( !ptr->node_set.empty() )
        {
            ptr->node_set.clear();
        }

        ptr->depth = -1;

        return ptr;
    }

    static bool_t             collide( ego_BSP_branch * pbranch, const ego_BSP_aabb & bb, leaf_child_list_t & colst );
    static bool_t             dealloc_nodes( ego_BSP_branch * B, const bool_t recursive = bfalse );
};

//--------------------------------------------------------------------------------------------
struct ego_BSP_tree
{

/// use a EGOBOO_SET to ensure only version of each node pointer gets inserted
#if defined(USE_HASH) && defined(__GNUC__)
    typedef EGOBOO_SET< ego_BSP_branch * , LargeIntHasher<ego_BSP_branch> > branch_set_t;
#else
    typedef EGOBOO_SET< ego_BSP_branch * >   branch_set_t;
#endif

    typedef std::stack< ego_BSP_branch * > branch_stack_t;

    size_t dimensions;
    int    depth;

    ego_BSP_branch      * root;

    leaf_child_list_t     infinite;

    ego_BSP_aabb          bbox;

    ego_BSP_tree()  { clear( this ); };

    ego_BSP_tree( const Sint32 _dim, const Sint32 _depth ) { clear( this ); init( this, _dim, _depth ); }
    ~ego_BSP_tree()                            { deinit( this ); }

    //static ego_BSP_tree * create( const size_t count );
    //static bool_t           destroy( ego_BSP_tree   ** ptree );

    static ego_BSP_tree * init( ego_BSP_tree * t, const Sint32 dim, const Sint32 depth );
    static ego_BSP_tree * deinit( ego_BSP_tree * t );
    static bool_t           alloc( ego_BSP_tree * t, const Sint32 dim, const Sint32 depth );
    static bool_t           dealloc( ego_BSP_tree * t );

    static bool_t           init_0( ego_BSP_tree * t );

    static bool_t             clear_nodes( ego_BSP_tree * t, const bool_t recursive = bfalse );
    static bool_t             dealloc_nodes( ego_BSP_tree * t, const bool_t recursive = bfalse );
    static bool_t             dealloc_all( ego_BSP_tree * t );
    static bool_t             prune( ego_BSP_tree * t );
    static ego_BSP_branch * get_free( ego_BSP_tree * t );
    static bool_t             add_free( ego_BSP_tree * t, ego_BSP_branch * B );
    static ego_BSP_branch * ensure_root( ego_BSP_tree * t );
    static ego_BSP_branch * ensure_branch( ego_BSP_tree * t, ego_BSP_branch * B, const int index );
    static Sint32             count_nodes( const Sint32 dim, const Sint32 depth );
    static bool_t             insert( ego_BSP_tree * t, ego_BSP_branch * B, ego_BSP_leaf * n, const int index );
    static bool_t             insert_leaf( ego_BSP_tree * ptree, ego_BSP_leaf * pleaf );

    static bool_t             generate_aabb_child( const ego_BSP_aabb * psrc, const int index, ego_BSP_aabb * pdst );
    static int                collide( ego_BSP_tree * tree, const ego_BSP_aabb & bb, leaf_child_list_t & colst );

protected:

    static bool_t             prune_branch( ego_BSP_tree * t, ego_BSP_branch * B );
    static bool_t             prune_one_branch( ego_BSP_tree * t, ego_BSP_branch * B, const bool_t recursive = bfalse );

    static bool_t             dealloc_branch( ego_BSP_tree * t, ego_BSP_branch * B );
    static bool_t             remove_used( ego_BSP_tree * t, ego_BSP_branch * B );

    static bool_t             insert_leaf_rec( ego_BSP_tree * ptree, ego_BSP_branch * pbranch, ego_BSP_leaf * pleaf, const int depth );
    static bool_t             dealloc_branches( ego_BSP_tree * t );

    static bool_t             insert_infinite( ego_BSP_tree * ptree, ego_BSP_leaf * pleaf );

    static bool_t             free_allocation_lists( ego_BSP_tree * t );

private:

    //---- pre-allocated arrays of branches
    branch_list_t       branch_all;
    branch_set_t        branch_used;
    branch_stack_t      branch_free;

    static ego_BSP_tree * clear( ego_BSP_tree * ptr );

};

#define _bsp_h
