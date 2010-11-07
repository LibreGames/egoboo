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

/// \file hash.h
/// \details Implementation of the "efficient" hash node storage.

#include "egoboo_typedef_cpp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a hash type for "efficiently" storing data
struct ego_hash_node
{
    ego_hash_node * next;
    void          * data;

    static ego_hash_node * create( void * data, ego_hash_node * src = NULL );
    static bool_t          destroy( ego_hash_node **, const bool_t own_ptr );
    static ego_hash_node * ctor_this( ego_hash_node * n, void * data );
    static bool_t          dtor_this( ego_hash_node * n );
    static ego_hash_node * insert_after( ego_hash_node lst[], ego_hash_node * n );
    static ego_hash_node * insert_before( ego_hash_node lst[], ego_hash_node * n );
    static ego_hash_node * remove_after( ego_hash_node lst[] );
    static ego_hash_node * remove( ego_hash_node lst[] );
};

//--------------------------------------------------------------------------------------------
struct ego_hash_list
{
    /// An iterator element for traversing the ego_hash_list
    struct iterator
    {
        int             hash;
        ego_hash_node * pnode;

        static iterator * ctor_this( iterator * it );
        static void     * ptr( iterator * it );
        static bool_t     set_begin( iterator * it, const ego_hash_list * hlst );
        static bool_t     done( const iterator * it, const ego_hash_list * hlst );
        static bool_t     next( iterator * it, const ego_hash_list * hlst );
    };

    static ego_hash_list * create( const int size, ego_hash_list * ptr = NULL );
    static bool_t          destroy( ego_hash_list **, const bool_t own_ptr = btrue );

    static bool_t          dealloc( ego_hash_list * lst );
    static bool_t          alloc( ego_hash_list * lst, const int size );
    static bool_t          renew( ego_hash_list * lst );

    static size_t          count_nodes( ego_hash_list *plst );
    static int             get_allocd( ego_hash_list *plst );
    static size_t          get_count( ego_hash_list *plst, const int i );
    static ego_hash_node * get_node( ego_hash_list *plst, const int i );

    static bool_t          set_allocd( ego_hash_list *plst, const int i );
    static bool_t          set_count( ego_hash_list *plst, const int i, const size_t );
    static bool_t          set_node( ego_hash_list *plst, const int i, ego_hash_node * );

    static bool_t          insert_unique( ego_hash_list * phash, const ego_hash_node * pnode );

protected:

    static ego_hash_list * ctor_this( ego_hash_list * lst, const int size );
    static ego_hash_list * dtor_this( ego_hash_list * lst );

    ego_hash_list( const int size = -1 ) { _init(); ctor_this( this, size ); }
    ~ego_hash_list() { dtor_this( this ); _init(); }

private:

    static ego_hash_list * clear( ego_hash_list * ptr )
    {
        if ( NULL == ptr ) return NULL;

        ego_hash_list::dtor_this( ptr );

        return ptr;
    }

    void _init()
    {
        if ( NULL != this )
        {
            SDL_memset( this, 0, sizeof( *this ) );
        }
    }

    int              allocated;
    size_t         * subcount;
    ego_hash_node ** sublist;

};

//--------------------------------------------------------------------------------------------