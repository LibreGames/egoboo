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

/// @file hash.h
/// @details Implementation of the "efficient" hash node storage.

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a hash type for "efficiently" storing data
struct ego_hash_node
{
    ego_hash_node * next;
    void          * data;

    static ego_hash_node * create( void * data );
    static bool_t          destroy( ego_hash_node ** );
    static ego_hash_node * ctor( ego_hash_node * n, void * data );
    static bool_t          dtor( ego_hash_node * n );
    static ego_hash_node * insert_after( ego_hash_node lst[], ego_hash_node * n );
    static ego_hash_node * insert_before( ego_hash_node lst[], ego_hash_node * n );
    static ego_hash_node * remove_after( ego_hash_node lst[] );
    static ego_hash_node * remove( ego_hash_node lst[] );
};

//--------------------------------------------------------------------------------------------
struct ego_hash_list
{
    int              allocated;
    int            * subcount;
    ego_hash_node ** sublist;

    ego_hash_list() { clear( this ); }
    ~ego_hash_list() { dtor( this ); }

    static ego_hash_list * create( int size );
    static bool_t          destroy( ego_hash_list ** );
    static ego_hash_list * ctor( ego_hash_list * lst, int size );
    static ego_hash_list * dtor( ego_hash_list * lst );
    static bool_t          dealloc( ego_hash_list * lst );
    static bool_t          alloc( ego_hash_list * lst, int size );
    static bool_t          renew( ego_hash_list * lst );

    static size_t          count_nodes( ego_hash_list *plst );
    static int             get_allocd( ego_hash_list *plst );
    static size_t          get_count( ego_hash_list *plst, int i );
    static ego_hash_node * get_node( ego_hash_list *plst, int i );

    static bool_t          set_allocd( ego_hash_list *plst,        int );
    static bool_t          set_count( ego_hash_list *plst, int i, int );
    static bool_t          set_node( ego_hash_list *plst, int i, ego_hash_node * );

    static bool_t          insert_unique( ego_hash_list * phash, ego_hash_node * pnode );

private:

    static ego_hash_list * clear( ego_hash_list * ptr )
    {
        if ( NULL == ptr ) return NULL;

        memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------

/// An iterator element for traversing the ego_hash_list
struct ego_hash_list_iterator
{
    int           hash;
    ego_hash_node * pnode;

    static ego_hash_list_iterator * ctor( ego_hash_list_iterator * it );
    static void                   * ptr( ego_hash_list_iterator * it );
    static bool_t                   set_begin( ego_hash_list_iterator * it, ego_hash_list * hlst );
    static bool_t                   done( ego_hash_list_iterator * it, ego_hash_list * hlst );
    static bool_t                   next( ego_hash_list_iterator * it, ego_hash_list * hlst );
};
