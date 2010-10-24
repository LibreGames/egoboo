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

/// @file egoboo_object_list.h
/// @brief Routines for character list management

#include "egoboo_object.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

template <typename _ty, size_t _sz>
struct t_ego_obj_lst
{
    /// use this typedef to make iterating through the t_cpp_map<> easier
    typedef typename t_cpp_map<_ty, _sz>::iterator iterator;
    typedef          t_reference<_ty>              reference;

    int              loop_depth;

    t_ego_obj_lst( size_t len = _sz ) : _max_len( len ), loop_depth( 0 ) { init(); }
    ~t_ego_obj_lst() { deinit(); }

    INLINE bool_t validate_ref( const reference & ref ) { REF_T tmp = ref.get_value(); return tmp < _max_len; };

    void             update_used();

    void             deinit();
    void             init();

    egoboo_rv        free_one( const reference & ref );
    void             free_all();

    reference        allocate( const reference & override );
    void             cleanup();

    egoboo_rv        add_activation( const reference & ref );
    egoboo_rv        add_termination( const reference & ref );

    INLINE _ty &     get_obj( const reference & ref );
    INLINE _ty *     get_ptr( const reference & ref );
    INLINE _ty *     get_valid_ptr( const reference & ref );

    typename _ty::data_type &  get_data( const reference & ref );
    typename _ty::data_type *  get_pdata( const reference & ref );
    typename _ty::data_type *  get_valid_pdata( const reference & ref );

    iterator   used_begin()                    ; // { return used_map.iterator_begin(); }
    bool_t     used_end( iterator & it )       ; // { return used_map.iterator_end( it ); }
    iterator & used_increment( iterator & it ) ; // { return used_map.iterator_increment( it ); }

    size_t get_size()   { return _max_len; }
    signed free_count() { return signed( _max_len ) - used_map.size(); }
    size_t used_count() { return used_map.size();                      }

    unsigned update_guid() { return used_map.get_id(); };

    bool_t set_length( size_t len )
    {
        // technically, we might have to reinitialize the structure after this is changed?

        size_t old_len = _max_len;
        bool_t changed = bfalse;
        bool_t shrunk  = bfalse;

        _max_len = std::min( _sz, len );

        changed = old_len != _max_len;
        shrunk  = _max_len < old_len;

        if ( changed )
        {
            reinit();
        }

        return changed;

    }

protected:

    void             reinit();

    reference get_free();

    void      clear_free_list();

private:

    bool_t    free_raw( const reference & ref );
    reference activate_object( const reference & ref );

    /// the actual list of objects
    _ty ary[_sz];

    /// the map of used objects
    t_cpp_map<_ty, _sz> used_map;

    /// the map of free objects
    std::queue< REF_T > free_queue;

    /// a list of objects that need to be terminated outside all loops
    std::stack< REF_T > termination_stack;

    /// a list of objects that need to be activated outside all loops
    std::stack< REF_T > activation_stack;

    /// the current maximum size of the list
    size_t _max_len;
};

//--------------------------------------------------------------------------------------------
// Generic looping Macros
//--------------------------------------------------------------------------------------------

// Macros to automate looping through a t_ego_obj_lst<>.
//
// This looks a bit messy, but it will simplify the look of egoboo codebase by
// hiding the code that defers the the creation and deletion of objects until loop termination.
//
// This process is necessary so that o iterators will be invalidated during the loop
// because of operations occurring inside the loop that increase or decrease the length of the list.

/// the most basic loop beginning. scans through the used list.
#define OBJ_LIST_BEGIN_LOOP(OBJ_T, LIST, IT, POBJ)  \
    { \
        int internal__##LIST##_start_depth = LIST.loop_depth; \
        LIST.loop_depth++; \
        for( LIST##_t::iterator internal__##IT = LIST.used_begin(); !LIST.used_end(internal__##IT); LIST.used_increment(internal__##IT) ) \
        { \
            const OBJ_T * POBJ = internal__##IT->second; \
            if( NULL == POBJ ) continue;

//--------------------------------------------------------------------------------------------
/// A refinement OBJ_LIST_BEGIN_LOOP(), which picks out "defined"
#define OBJ_LIST_BEGIN_LOOP_DEFINED(OBJ_T, LIST, IT, POBJ)  OBJ_LIST_BEGIN_LOOP(OBJ_T, LIST, IT, POBJ) if( !DEFINED_PBASE(POBJ) ) continue;

//--------------------------------------------------------------------------------------------
/// The simplest refinement OBJ_LIST_BEGIN_LOOP(). Picks "active" objects out of the used list.
#define OBJ_LIST_BEGIN_LOOP_ACTIVE(OBJ_T, LIST, IT, POBJ) OBJ_LIST_BEGIN_LOOP(OBJ_T, LIST, IT, POBJ) if( !ACTIVE_PBASE(POBJ) ) continue;

//--------------------------------------------------------------------------------------------
/// The termination of all OBJ_LIST_BEGIN_LOOP()
#define OBJ_LIST_END_LOOP(LIST) \
    } \
    LIST.loop_depth--; \
    if(internal__##LIST##_start_depth != LIST.loop_depth) EGOBOO_ASSERT(bfalse); \
    LIST.cleanup(); \
    }

