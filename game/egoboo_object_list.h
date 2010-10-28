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

struct ego_obj_lst_state;

template < typename _data, size_t _sz > struct t_ego_obj_container;
template < typename _data, size_t _sz > struct t_ego_obj_lst;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a kind of an "interface" that should be inherited by every object that is
/// stored in t_ego_obj_lst

struct ego_obj_lst_state
{
    friend struct ego_obj_lst_client;

    ego_obj_lst_state( size_t idx = size_t( ~0L ) ) { ctor_this( this, idx ); }
    ~ego_obj_lst_state() { dtor_this( this ); }

    static ego_obj_lst_state * retor_this( ego_obj_lst_state * ptr, size_t idx )
    {
        ptr = dtor_this( ptr );
        ptr = ctor_this( ptr, idx );
        return ptr;
    }

    static const bool_t get_allocated( const ego_obj_lst_state * ptr ) { return ( NULL == ptr ) ? bfalse     : ptr->allocated; }
    static const size_t get_index( const ego_obj_lst_state * ptr ) { return ( NULL == ptr ) ? size_t( -1 ) : ptr->index; }
    static const bool_t in_free( const ego_obj_lst_state * ptr ) { return ( NULL == ptr ) ? bfalse     : ptr->in_free_list; }
    static const bool_t in_used( const ego_obj_lst_state * ptr ) { return ( NULL == ptr ) ? bfalse     : ptr->in_used_list; }
    static const Uint32 get_list_id( ego_obj_lst_state * ptr ) { return ( NULL == ptr ) ? INVALID_UPDATE_GUID     : ptr->update_guid; }

    static ego_obj_lst_state * set_allocated( ego_obj_lst_state *, bool_t val );
    static ego_obj_lst_state * set_used( ego_obj_lst_state *, bool_t val );
    static ego_obj_lst_state * set_free( ego_obj_lst_state *, bool_t val );
    static ego_obj_lst_state * set_list_id( ego_obj_lst_state *, unsigned val );

protected:

    static ego_obj_lst_state * ctor_this( ego_obj_lst_state *, size_t index = size_t( ~0L ) );
    static ego_obj_lst_state * dtor_this( ego_obj_lst_state * );

private:

    size_t    index;        ///< what is the index position in the object list?
    bool_t    allocated;    ///< The object has been allocated
    bool_t    in_free_list; ///< the object is currently in the free list
    bool_t    in_used_list; ///< the object is currently in the used list
    unsigned  update_guid;  ///< the value of t_ego_obj_lst::get_list_id() the last time this object was updated

    static ego_obj_lst_state * clear( ego_obj_lst_state *, size_t index = size_t( ~0L ) );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The actual container that will be stored in the t_cpp_list<>

template< typename _data, size_t _sz >
struct t_ego_obj_container : public ego_obj_lst_state
{
    friend struct t_ego_obj_lst<_data, _sz>;

    //---- typedefs

    /// a type definition so that list t_ego_obj_lst<> can automatically
    // route functions to the contained data using the scope
    // resolution ouperator ( i.e. data_type:: )
    typedef _data data_type;

    /// gcc is very particluar about defining dependent types... will this work?
    typedef typename t_ego_obj_container< _data, _sz > its_type;

    /// gcc is very particluar about defining dependent types... will this work?
    typedef typename t_ego_obj_lst< its_type, _sz > list_type;

    //---- constructors and destructors

    /// default constructor
    explicit t_ego_obj_container( size_t idx = size_t ( ~0 ) ) : ego_obj_lst_state( idx ), _container_data( this ) { ctor_this( this ); }

    /// default destructor
    ~t_ego_obj_container() { dtor_this( this ); }

    //---- implementation of accessors required by t_ego_obj_lst<>

    // This container "has a" data_type, so we need some way of accessing it
    // These have to have generic names to that t_ego_obj_lst<> can access the data
    // for all container types

    static       data_type & get_data_ref( its_type & rcont ) { return rcont._container_data; }
    static       data_type * get_data_ptr( its_type * pcont ) { return NULL == pcont ? NULL : &( pcont->_container_data ); }
    static const data_type * cget_data_ptr( const its_type * pcont ) { return NULL == pcont ? NULL : &( pcont->_container_data ); }

protected:

    //---- construction and destruction

    /// set/change the information about where this object is stored in t_ego_obj_lst<>.
    /// should only be called by t_ego_obj_lst<>
    static its_type * allocate( its_type * pobj, size_t index ) { if ( NULL == pobj ) return pobj; ego_obj_lst_state::retor_this( pobj, index ); return pobj; }

    /// tell this object that it is no longer stored in t_ego_obj_lst<>
    /// should only be called by t_ego_obj_lst<>
    static its_type * deallocate( its_type * pobj ) { ego_obj_lst_state::dtor_this( pobj, index ); }

    /// cause this struct to recycle its data
    /// should only be called by t_ego_obj_lst<>
    static its_type * retor_this( its_type * ptr, size_t idx = size_t ( ~0 ) )
    {
        dtor_this( ptr );
        ctor_this( ptr );
        return ptr;
    }

    /// cause this struct to completely deallocate and reallocate its data
    /// and the data of all its dependents
    /// should only be called by t_ego_obj_lst<>
    static its_type * retor_all( its_type * ptr, size_t idx = size_t ( ~0 ) )
    {
        dtor_all( ptr );
        ctor_all( ptr, idx );
        return ptr;
    }

    /// construct this struct, ONLY
    static its_type * ctor_this( its_type * pobj );
    /// destruct this struct, ONLY
    static its_type * dtor_this( its_type * pobj );

    /// construct this struct, and ALL dependent structs. use placement new
    static its_type * ctor_all( its_type * ptr ) { if ( NULL != ptr ) { /* puts( "\t" __FUNCTION__ ); */ new( ptr ) its_type(); } return ptr; }
    /// denstruct this struct, and ALL dependent structs. call the destructor
    static its_type * dtor_all( its_type * ptr )  { if ( NULL != ptr ) { ptr->~its_type(); /* puts( "\t" __FUNCTION__ ); */ } return ptr; }

private:
    data_type _container_data;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

template <typename _data, size_t _sz>
struct t_ego_obj_lst
{
    /// use this typedef to make iterating through the t_cpp_map<> easier
    typedef typename t_ego_obj_container<_data, _sz>                 container_type;
    typedef typename t_reference< t_ego_obj_container<_data, _sz> >  lst_reference;

    typedef typename t_cpp_map< t_ego_obj_container<_data, _sz>, _sz>           map_type;
    typedef typename t_cpp_map< t_ego_obj_container<_data, _sz>, _sz>::iterator map_iterator;

    //---- construction/destruction
    t_ego_obj_lst( size_t len = _sz ) : _max_len( len ), loop_depth( 0 )  { init(); }
    ~t_ego_obj_lst() { deinit(); }

    /// initialize the list from scratch
    void init();

    /// return the list to a completely deinitialized state
    void deinit();

    //---- public list management

    /// a "garbage collection" function that makes sure that there are no
    /// bad elements in the free_list
    void             update_used();

    /// completely deallocate a used object and move it from the used_list to the free_list
    egoboo_rv        free_one( const lst_reference & ref );

    /// deallocate all used objects and reinitialize the used_list and the free_list
    void             free_all();

    /// deallocate all used objects and reinitialize the used_list and the free_list
    lst_reference    allocate( const lst_reference & override );

    //---- public accessors

    /// what it the maximum length of the current list
    size_t   get_size()   { return _max_len; }

    /// how many free elements are left. might be negative if the list length was changed
    /// while some elements outside that range were still allocated
    signed   free_count() { return signed( _max_len ) - used_map.size(); }

    /// how many elements are allocated?
    size_t   used_count() { return used_map.size(); }

    /// get the current number of allocations/deallocations from the list. If this does not match
    /// with the corresponding number in a map_iterator, the iterator is invalid
    unsigned get_list_id() { return used_map.get_id(); };

    /// change the length of the list. this function can be called at any time and be set anywhere
    /// between 0 and _sz
    bool_t set_length( size_t len );

    /// is a given reference within the bounds of this list?
    INLINE bool_t in_range_ref( const lst_reference & ref ) { REF_T tmp = ref.get_value(); return tmp < _max_len; };

    //---- public iteration methods - iterate over the used map

    /// proper method of accessing the used_map::begin() function
    map_iterator   used_begin()                        { return used_map.iterator_begin(); }

    /// proper method of testing for the end of the iteration.
    /// will cause a loop to exit of the used_map's length changes during the iteration
    bool_t         used_end( map_iterator & it )       { return used_map.iterator_end( it ); }

    /// proper method of accessing incrementing a map_iterator
    map_iterator & used_increment( map_iterator & it ) { return used_map.iterator_increment( it ); }

    /// how many elements are allocated?
    int   get_loop_depth()       { return loop_depth; }
    int   increment_loop_depth() { return ++loop_depth; }
    int   decrement_loop_depth() { return --loop_depth; }

    //---- functions to access elements of the list

    // grab the container
    INLINE typename container_type & get_ref( const lst_reference & ref );
    INLINE typename container_type * get_ptr( const lst_reference & ref );
    INLINE typename container_type * get_allocated_ptr( const lst_reference & ref );

    // grab the container's data
    INLINE typename container_type::data_type &  get_data_ref( const lst_reference & ref );
    INLINE typename container_type::data_type *  get_data_ptr( const lst_reference & ref );
    INLINE typename container_type::data_type *  get_allocated_data_ptr( const lst_reference & ref );

    //---- deferred activation/termination

    /// add a deferred activation
    egoboo_rv         add_activation( const lst_reference & ref );

    /// add a deferred termination
    egoboo_rv         add_termination( const lst_reference & ref );

    /// perform all deferred actions after a list iteration has ended
    void             cleanup();

protected:

    /// enables structs that inherit from this list to re-initialize the index of a container
    void allocate_container( container_type & rcont, size_t index )
    {
        container_type::allocate( &rcont, index );
    }

    /// how many loops are currently going on?
    int              loop_depth;

    /// reinitialize the list
    void             reinit();

    /// internal function to get a valid element from the free_list
    /// does not allocate or initialize the element
    lst_reference get_free();

    /// a hack to get around the crash caused by calling free_queue.clear(), when the queue is empty
    void      clear_free_list();

private:

    /// internal function to return an element to the free_list.
    /// does not deallocate or deinitialize the element
    bool_t    free_raw( const lst_reference & ref );

    /// basic function to call to activate a list element
    lst_reference activate_element( const lst_reference & pcont );

    /// basic function to call to deactivate a list element
    lst_reference deactivate_element( const lst_reference & pcont );

    /// helper function to activate a given container
    container_type * activate_container_raw( container_type *, size_t index );

    /// helper function to deactivate a given container
    container_type * deactivate_container_raw( container_type * );

    /// helper function to activate a container data element
    _data * activate_data_raw( _data *, const container_type * pcont );

    /// helper function to deactivate a container data element
    _data * deactivate_data_raw( _data * );

    //---- this struct's data

    /// the actual list of objects
    container_type ary[_sz];

    /// the map of used objects
    map_type       used_map;

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
// CONTAINER STATE MACROS
//--------------------------------------------------------------------------------------------

/// What is the container's index?
#define INDEX_PCONT( TYPE, PCONT )          ( TYPE::get_index(PCONT) )

/// Is the container allocated?
#define FLAG_ALLOCATED_PCONT( TYPE, PCONT ) ( TYPE::get_allocated(PCONT) )

/// Is the container marked as being in the used list?
#define FLAG_USED_PCONT( TYPE, PCONT )      ( TYPE::in_used(PCONT) )

/// Is the container marked as being in the used list?
#define FLAG_FREE_PCONT( TYPE, PCONT )      ( TYPE::in_free(PCONT) )

//--------------------------------------------------------------------------------------------
// LOOPING MACROS
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
        int internal__##LIST##_start_depth = LIST.get_loop_depth(); \
        LIST.increment_loop_depth(); \
        for( LIST##_t::map_iterator internal__##IT = LIST.used_begin(); !LIST.used_end(internal__##IT); LIST.used_increment(internal__##IT) ) \
        { \
            if( NULL == internal__##IT->second ) continue; \
            const OBJ_T * POBJ = LIST##_t::container_type::cget_data_ptr( internal__##IT->second ); \
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
    LIST.decrement_loop_depth(); \
    if(internal__##LIST##_start_depth != LIST.get_loop_depth()) EGOBOO_ASSERT(bfalse); \
    LIST.cleanup(); \
    }

#define _egoboo_object_list_h

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egoboo_object_list.inl"