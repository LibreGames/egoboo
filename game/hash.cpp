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

/// @file hash.c
/// @brief Implementation of the support functions for hash tables
/// @details

#include "hash.h"

#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_hash_node::dtor_this( ego_hash_node * n )
{
    if ( NULL == n ) return bfalse;

    n->data = NULL;

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_hash_node * ego_hash_node::ctor_this( ego_hash_node * pn, void * data )
{
    if ( NULL == pn ) return pn;

    memset( pn, 0, sizeof( *pn ) );

    pn->data = data;

    return pn;
}

//--------------------------------------------------------------------------------------------
ego_hash_node * ego_hash_node::create( void * data )
{
    ego_hash_node * n = EGOBOO_NEW( ego_hash_node );

    return ego_hash_node::ctor_this( n, data );
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_node::destroy( ego_hash_node ** pn )
{
    bool_t retval = bfalse;

    if ( NULL == pn || NULL == *pn ) return bfalse;

    retval = ego_hash_node::dtor_this( *pn );

    EGOBOO_DELETE( *pn );

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_hash_node * ego_hash_node::insert_after( ego_hash_node lst[], ego_hash_node * n )
{
    if ( NULL == n ) return lst;
    n->next = NULL;

    if ( NULL == lst ) return n;

    n->next = n->next;
    lst->next = n;

    return lst;
}

//--------------------------------------------------------------------------------------------
ego_hash_node * ego_hash_node::insert_before( ego_hash_node lst[], ego_hash_node * n )
{
    if ( NULL == n ) return lst;
    n->next = NULL;

    if ( NULL == lst ) return n;

    n->next = lst;

    return n;
}

//--------------------------------------------------------------------------------------------
ego_hash_node * ego_hash_node::remove_after( ego_hash_node lst[] )
{
    ego_hash_node * n;

    if ( NULL == lst ) return NULL;

    n = lst->next;
    if ( NULL == n ) return lst;

    lst->next = n->next;
    n->next   = NULL;

    return lst;
}

//--------------------------------------------------------------------------------------------
ego_hash_node * ego_hash_node::remove( ego_hash_node lst[] )
{
    ego_hash_node * n;

    if ( NULL == lst ) return NULL;

    n = lst->next;
    if ( NULL == n ) return NULL;

    lst->next = NULL;

    return n;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_hash_list * ego_hash_list::ctor_this( ego_hash_list * lst, int hash_size )
{
    if ( NULL == lst ) return NULL;

    if ( hash_size < 0 ) hash_size = 256;

    ego_hash_list::alloc( lst, hash_size );

    return lst;
}

//--------------------------------------------------------------------------------------------
ego_hash_list * ego_hash_list::dtor_this( ego_hash_list * lst )
{
    if ( NULL == lst ) return NULL;

    ego_hash_list::dealloc( lst );

    return lst;
}

//--------------------------------------------------------------------------------------------
size_t ego_hash_list::count_nodes( ego_hash_list *plst )
{
    /// @details BB@> count the total number of nodes in the hash list

    int    i;
    size_t count = 0;

    if ( NULL == plst ) return 0;

    for ( i = 0; i < plst->allocated; i++ )
    {
        if ( NULL != plst->sublist[i] )
        {
            count += plst->subcount[i];
        }
    }

    return count;
}

//--------------------------------------------------------------------------------------------
ego_hash_list * ego_hash_list::create( int size )
{
    ego_hash_list * rv = EGOBOO_NEW( ego_hash_list );
    if ( NULL == rv ) return NULL;

    memset( rv, 0, sizeof( *rv ) );

    return ego_hash_list::ctor_this( rv, size );
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list::destroy( ego_hash_list ** plst )
{
    bool_t retval = bfalse;

    if ( NULL == plst || NULL == *plst ) return bfalse;

    retval = ( NULL != ego_hash_list::dtor_this( *plst ) );

    EGOBOO_DELETE( *plst );

    return retval;
}

//--------------------------------------------------------------------------------------------
int ego_hash_list::get_allocd( ego_hash_list *plst )
{
    if ( NULL == plst ) return 0;

    return plst->allocated;
}

//--------------------------------------------------------------------------------------------
size_t ego_hash_list::get_count( ego_hash_list *plst, int i )
{
    if ( NULL == plst || NULL == plst->subcount ) return 0;

    return plst->subcount[i];
}

//--------------------------------------------------------------------------------------------
ego_hash_node *  ego_hash_list::get_node( ego_hash_list *plst, int i )
{
    if ( NULL == plst || NULL == plst->sublist ) return NULL;

    return plst->sublist[i];
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list::set_allocd( ego_hash_list *plst, int ival )
{
    if ( NULL == plst ) return bfalse;

    plst->allocated = ival;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list::set_count( ego_hash_list *plst, int i, int count )
{
    if ( NULL == plst || NULL == plst->subcount ) return bfalse;

    if ( i >= plst->allocated ) return bfalse;

    plst->subcount[i] = count;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list::set_node( ego_hash_list *plst, int i, ego_hash_node * pnode )
{
    if ( NULL == plst || NULL == plst->sublist ) return bfalse;

    if ( i >= plst->allocated ) return bfalse;

    plst->sublist[i] = pnode;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list::dealloc( ego_hash_list * lst )
{
    if ( NULL == lst ) return bfalse;
    if ( 0 == lst->allocated ) return btrue;

    EGOBOO_DELETE_ARY( lst->subcount );
    EGOBOO_DELETE_ARY( lst->sublist );
    lst->allocated = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list::alloc( ego_hash_list * lst, int size )
{
    if ( NULL == lst ) return bfalse;

    ego_hash_list::dealloc( lst );

    lst->subcount = EGOBOO_NEW_ARY( int, size );
    if ( NULL == lst->subcount )
    {
        return bfalse;
    }

    lst->sublist = EGOBOO_NEW_ARY( ego_hash_node *, size );
    if ( NULL == lst->sublist )
    {
        EGOBOO_DELETE( lst->subcount );
        return bfalse;
    }
    else
    {
        int cnt;
        for ( cnt = 0; cnt < size; cnt++ ) lst->sublist[cnt] = NULL;
    }

    lst->allocated = size;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list::renew( ego_hash_list * lst )
{
    /// @details BB@> renew the ego_CoNode hash table.
    ///
    /// Since we are filling this list with pre-allocated ego_CoNode's,
    /// there is no need to delete any of the existing pchlst->sublist elements

    int cnt;

    if ( NULL == lst ) return bfalse;

    for ( cnt = 0; cnt < lst->allocated; cnt++ )
    {
        lst->subcount[cnt] = 0;
        lst->sublist[cnt]  = NULL;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_hash_list_iterator * ego_hash_list_iterator::ctor_this( ego_hash_list_iterator * it )
{
    if ( NULL == it ) return NULL;

    memset( it, 0, sizeof( *it ) );

    return it;
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list_iterator::set_begin( ego_hash_list_iterator * it, ego_hash_list * hlst )
{
    int i;

    it = ego_hash_list_iterator::ctor_this( it );

    if ( NULL == it || NULL == hlst ) return bfalse;

    // find the first non-null hash element
    for ( i = 0; i < hlst->allocated && NULL == it->pnode; i++ )
    {
        it->hash  = i;
        it->pnode = hlst->sublist[i];
    }

    return NULL != it->pnode;
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list_iterator::done( ego_hash_list_iterator * it, ego_hash_list * hlst )
{
    if ( NULL == it || NULL == hlst ) return btrue;

    // the end position
    if ( it->hash >= hlst->allocated ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t ego_hash_list_iterator::next( ego_hash_list_iterator * it, ego_hash_list * hlst )
{
    int i, inext;
    ego_hash_node * pnext;

    if ( NULL == it || NULL == hlst ) return bfalse;

    inext = it->hash;
    pnext = NULL;
    if ( NULL != it->pnode )
    {
        // try jumping to the next element
        pnext = it->pnode->next;
    }

    if ( NULL == pnext )
    {
        // find the next non-null hash element
        for ( i = it->hash + 1; i < hlst->allocated && NULL == pnext; i++ )
        {
            inext = i;
            pnext = hlst->sublist[i];
        }
    }

    if ( NULL == pnext )
    {
        // could not find one. set the iterator to the end condition.
        inext = hlst->allocated;
    }

    it->hash  = inext;
    it->pnode = pnext;

    return NULL != it->pnode;
}

//--------------------------------------------------------------------------------------------
void * ego_hash_list_iterator::ptr( ego_hash_list_iterator * it )
{
    if ( NULL == it || NULL == it->pnode ) return NULL;

    return it->pnode->data;
}

