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

/// \file egoboo_object_list.h
/// \brief Routines for character list management

#include "egoboo_object.h"
#include "egoboo_typedef_cpp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_obj_lst_state;

template < typename _data, const size_t _sz > struct t_ego_obj_container;
template < typename _data, const size_t _sz > struct t_obj_lst_map;
template < typename _data, const size_t _sz > struct t_obj_lst_deque;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the data stored in ego_obj_lst_state, separated to make initialization easier

struct ego_obj_lst_state_data
{
    friend struct ego_obj_lst_state;

    ego_obj_lst_state_data() { clear( this ); }
    ~ego_obj_lst_state_data() { clear( this ); }

    //---- accessor

    static bool_t in_used( const ego_obj_lst_state_data * ptr )
    { return ( NULL == ptr ) ? bfalse     : ptr->in_used_list; }

    static Uint32 get_list_id( ego_obj_lst_state_data * ptr )
    { return ( NULL == ptr ) ? INVALID_UPDATE_GUID     : ptr->update_guid; }

    static ego_obj_lst_state_data * set_used( ego_obj_lst_state_data *, const bool_t val );
    static ego_obj_lst_state_data * set_list_id( ego_obj_lst_state_data *, const ego_uint val );

private:

    bool_t    in_used_list; ///< the object is currently in the used list
    ego_uint  update_guid;  ///< the value of t_obj_lst_map::get_list_id() the last time this object was updated

    static ego_obj_lst_state_data * clear( ego_obj_lst_state_data * );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a kind of an "interface" that should be inherited by every object that is
/// stored in t_obj_lst_*<>

struct ego_obj_lst_state : public ego_obj_lst_state_data, public allocator_client
{
    friend struct ego_obj_lst_client;

    ego_obj_lst_state( const ego_uint guid = invalid_value, const ego_uint index = invalid_value ) :
            allocator_client( guid, index )
    { ctor_this( this ); }

    ~ego_obj_lst_state() { dtor_this( this ); }

    static ego_obj_lst_state * retor_this( ego_obj_lst_state * ptr, const ego_uint guid = invalid_value, const ego_uint index = invalid_value )
    {
        if ( NULL == ptr ) return ptr;

        ptr->~ego_obj_lst_state();
        ptr = new( ptr ) ego_obj_lst_state( guid, index );

        return ptr;
    }

    static ego_uint get_index( const ego_obj_lst_state * ptr, const REF_T fail_value = ego_uint( ~0L ) )
    { return ( NULL == ptr || !ptr->allocator_client::has_valid_id() ) ? fail_value : ptr->allocator_client::get_index(); }

    static Uint32 get_id( ego_obj_lst_state * ptr )
    { return ( NULL == ptr ) ? invalid_value : ptr->allocator_client::get_id(); }

protected:

    static ego_obj_lst_state * ctor_this( ego_obj_lst_state * );
    static ego_obj_lst_state * dtor_this( ego_obj_lst_state * );

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The actual container that will be stored in the t_list<>

template< typename _data, const size_t _sz >
struct t_ego_obj_container : public ego_obj_lst_state
{
    friend struct t_obj_lst_map<_data, _sz>;
    friend struct t_obj_lst_deque<_data, _sz>;

    //---- typedefs

    /// a type definition so that list t_obj_lst_map<> can automatically
    // route functions to the contained data using the scope
    // resolution ouperator ( i.e. data_type:: )
    typedef _data data_type;

    /// gcc is very particluar about defining dependent types... will this work?
    typedef t_ego_obj_container< _data, _sz > its_type;

    /// gcc is very particluar about defining dependent types... will this work?
    typedef t_obj_lst_map< its_type, _sz > list_type;

    //---- constructors and destructors

    /// default constructor
    explicit t_ego_obj_container( const ego_uint guid = invalid_value, const ego_uint index = invalid_value ) :
            ego_obj_lst_state( guid, index ),
            _container_data( this )
    { ctor_this( this ); }

    /// default destructor
    ~t_ego_obj_container() { dtor_this( this ); }

    //---- implementation of accessors required by t_obj_lst_map<>

    // This container "has a" data_type, so we need some way of accessing it
    // These have to have generic names to that t_obj_lst_map<> can access the data
    // for all container types

    static       data_type & get_data_ref( its_type & rcont ) { return rcont._container_data; }
    static       data_type * get_data_ptr( its_type * pcont ) { return NULL == pcont ? NULL : &( pcont->_container_data ); }
    static const data_type * cget_data_ptr( const its_type * pcont ) { return NULL == pcont ? NULL : &( pcont->_container_data ); }

    //---- automatic type conversions
    operator data_type & () { return _container_data; }
    operator const data_type & () const { return _container_data; }

protected:

    //---- construction and destruction

    /// set/change the information about where this object is stored in t_obj_lst_map<>.
    /// should only be called by t_obj_lst_map<>
    static its_type * allocate( its_type * pobj, const ego_uint index ) { if ( NULL == pobj ) return pobj; ego_obj_lst_state::retor_this( pobj, index ); return pobj; }

    /// tell this object that it is no longer stored in t_obj_lst_map<>
    /// should only be called by t_obj_lst_map<>
    static its_type * deallocate( its_type * pobj ) { ego_obj_lst_state::dtor_this( pobj, index ); }

    /// cause this struct to completely deallocate and reallocate its data
    /// and the data of all its dependents
    /// should only be called by t_obj_lst_map<>
    static its_type * retor_all( its_type * ptr, const ego_uint new_idx = invalid_value )
    {
        if ( NULL == ptr ) return ptr;

        // grab some info from the current allocator_client
        allocator_client * palloc = static_cast<allocator_client>( ptr );
        const ego_uint current_id  = palloc->get_id();
        const ego_uint current_idx = palloc->get_index();

        // if we are not given a new index, reuse the current one
        if ( invalid_value == new_idx ) new_idx = current_idx;

        dtor_all( ptr );
        ctor_all( ptr, current_id, new_idx );

        return ptr;
    }

private:

    /// construct this struct, and ALL dependent structs. use placement new
    static its_type * ctor_all( its_type * ptr, const ego_uint id, const ego_uint index )
    {
        if ( NULL == ptr ) return NULL;

        return new( ptr ) its_type( id, index );
    }

    /// destruct this struct, and ALL dependent structs. call the destructor
    static its_type * dtor_all( its_type * ptr ) { if ( NULL != ptr ) { ptr->~its_type(); } return ptr; }

    data_type _container_data;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

template <typename _data, const size_t _sz>
struct t_obj_lst_map :  public t_allocator_static< t_ego_obj_container<_data, _sz>, _sz >
{
    /// use this typedef to make iterating through the t_map<> easier
    typedef t_ego_obj_container<_data, _sz>                 container_type;
    typedef t_reference< t_ego_obj_container<_data, _sz> >  lst_reference;

    typedef t_map< t_ego_obj_container<_data, _sz>, REF_T>  cache_type;
    typedef typename cache_type::iterator                   iterator;

    //---- construction/destruction
    t_obj_lst_map( const size_t len = _sz ) : _max_len( len ), loop_depth( 0 )  { init(); }
    ~t_obj_lst_map() { deinit(); }

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
    ego_sint free_count() { return ego_sint( _max_len ) - ego_sint( used_map.size() ); }

    /// how many elements are allocated?
    size_t   used_count() { return used_map.size(); }

    /// get the current number of allocations/deallocations from the list. If this does not match
    /// with the corresponding number in a iterator_type, the iterator is invalid
    ego_uint get_list_id() { return used_map.get_id(); };

    /// change the length of the list. this function can be called at any time and be set anywhere
    /// between 0 and _sz
    bool_t set_length( const size_t len );

    /// is a given reference within the bounds of this list?
    INLINE bool_t in_range_ref( const lst_reference & ref ) { REF_T tmp = ref.get_value(); return tmp < _max_len; };

    //---- public iteration methods - iterate over the used map

    /// proper method of accessing the used_map::begin() function
    iterator       iterator_begin_allocated()                        { return used_map.iterator_begin(); }

    /// proper method of testing for the end of the iteration.
    /// will cause a loop to exit of the used_map's length changes during the iteration
    bool_t         iterator_finished( iterator & it )       { return used_map.iterator_end( it ); }

    /// proper method of accessing incrementing a iterator_type
    iterator & iterator_increment_allocated( iterator & it ) { return used_map.iterator_increment( it ); }

    /// how many elements are allocated?
    int   get_loop_depth()       { return loop_depth; }
    int   increment_loop_depth()
    {
        return ++loop_depth;
    }
    int   decrement_loop_depth()
    {
        return --loop_depth;
    }

    //---- functions to access elements of the list

    // grab the container
    INLINE container_type & get_ref( const lst_reference & ref );
    INLINE container_type * get_ptr( const lst_reference & ref );
    INLINE container_type * get_allocated_ptr( const lst_reference & ref );

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
    void allocate_container( container_type & rcont, const size_t index )
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
    //void      clear_free_list();

private:

    /// internal function to return an element to the free_list.
    /// does not deallocate or deinitialize the element
    bool_t    free_raw( const lst_reference & ref );

    /// basic function to call to activate a list element
    lst_reference activate_element( const lst_reference & pcont );

    /// basic function to call to deactivate a list element
    lst_reference deactivate_element( const lst_reference & pcont );

    /// helper function to activate a given container
    container_type * activate_container_raw( container_type *, const size_t index );

    /// helper function to deactivate a given container
    container_type * deactivate_container_raw( container_type * );

    /// helper function to activate a container data element
    _data * activate_data_raw( _data *, const container_type * pcont );

    /// helper function to deactivate a container data element
    _data * deactivate_data_raw( _data * );

    //---- this struct's data

    /// the map of used objects
    cache_type       used_map;

    /// a list of objects that need to be terminated outside all loops
    std::stack< REF_T > termination_stack;

    /// a list of objects that need to be activated outside all loops
    std::stack< REF_T > activation_stack;

    /// the current maximum size of the list
    size_t _max_len;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

template <typename _data, const size_t _sz>
struct t_obj_lst_deque : public t_allocator_static< t_ego_obj_container<_data, _sz>, _sz >
{
    typedef t_ego_obj_container<_data, _sz>                 container_type;
    typedef t_reference< t_ego_obj_container<_data, _sz> >  lst_reference;
    typedef t_allocator_static< container_type, _sz >       allocator_type;
    typedef t_deque< container_type, REF_T >                cache_type;

    /// make a specialization to the t_deque<>::iterator that will allow us to
    /// cache some pointers, which should save time later
    struct iterator : public cache_type::iterator
    {
        container_type * pcont;
        _data          * pdata;

        iterator( typename cache_type::iterator it ) : pcont( NULL ), pdata( NULL ) { this->deque_iterator() = it; };

        void invalidate() { cache_type::iterator::invalidate(); pcont = NULL; pdata = NULL; }
    };

    //---- construction/destruction
    t_obj_lst_deque( const size_t len = _sz ) : _max_len( len ), loop_depth( 0 )  { init(); }
    ~t_obj_lst_deque() { deinit(); }

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
    ego_sint free_count() { return ego_sint( _max_len ) - ego_sint( used_deque.size() ); }

    /// how many elements are allocated?
    size_t   used_count() { return used_deque.size(); }

    /// get the current number of allocations/deallocations from the list. If this does not match
    /// with the corresponding number in a iterator, the iterator is invalid
    ego_uint get_list_id() { return used_deque.get_id(); };

    /// change the length of the list. this function can be called at any time and be set anywhere
    /// between 0 and _sz
    bool_t set_length( const size_t len );

    //---- public iteration methods - iterate over the used deque

    /// begins the iterator through all ALLOCATED data
    iterator iterator_begin_allocated();

    /// begins the iterator through all DEFINED data == (ALLOCATED and VALID)
    iterator iterator_begin_defined();

    /// begins the iterator through all PROCESSING data == (ALLOCATED and VALID and state is processing)
    iterator iterator_begin_processing();

    /// begins the iterator through all INGAME data
    iterator iterator_begin_ingame();

    // begins the iterator through all BSP data
    // must be implemented in the list whose container has bsp data
    //iterator iterator_begin_bsp() { return used_deque.iterator_begin( it ); }

    // begins the iterator through all LIMBO data
    // implemented for particles
    //iterator iterator_begin_limbo() { return used_deque.iterator_begin( it ); }

    /// proper method of testing for the end of the iteration.
    /// will cause a loop to exit of the used_deque's length changes during the iteration
    bool_t         iterator_finished( iterator & it );

    /// increments the iterator through all ALLOCATED data
    iterator & iterator_increment_allocated( iterator & it );

    /// increments the iterator through all DEFINED data == (ALLOCATED and VALID)
    iterator & iterator_increment_defined( iterator & it );

    /// increments the iterator through all PROCESSING data == (ALLOCATED and VALID and state is processing)
    iterator & iterator_increment_processing( iterator & it );

    /// increments the iterator through all INGAME data
    iterator & iterator_increment_ingame( iterator & it );

    /// how many elements are allocated?
    int   get_loop_depth()       { return loop_depth; }
    int   increment_loop_depth()
    {
        return ++loop_depth;
    }
    int   decrement_loop_depth()
    {
        return --loop_depth;
    }

    //---- functions to access elements of the list

    // grab the container
    INLINE container_type & get_ref( const lst_reference & ref );
    INLINE container_type * get_ptr( const lst_reference & ref );
    INLINE container_type * get_allocated_ptr( const lst_reference & ref );

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
    void allocate_container( container_type & rcont, const size_t index )
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
    //void      clear_free_list();

private:

    /// internal function to return an element to the free_list.
    /// does not deallocate or deinitialize the element
    bool_t    free_raw( const lst_reference & ref );

    /// basic function to call to activate a list element
    lst_reference activate_element( const lst_reference & pcont );

    /// basic function to call to deactivate a list element
    lst_reference deactivate_element( const lst_reference & pcont );

    //---- this struct's data

    /// the deque of used objects
    cache_type       used_deque;

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

/// What is the container index?
#define GET_INDEX_PCONT( TYPE, PCONT, FAIL_VALUE ) ( TYPE::get_index(PCONT,FAIL_VALUE) )

/// Is the container allocated?
#define FLAG_ALLOCATED_PCONT( TYPE, PCONT ) ( allocator_client::has_valid_id(PCONT) )

/// Is the container marked as being in the used list?
#define FLAG_USED_PCONT( TYPE, PCONT )      ( TYPE::in_used(PCONT) )

/// Is the container marked as being in the used list?
#define FLAG_FREE_PCONT( TYPE, PCONT )      ( TYPE::in_free(PCONT) )

//--------------------------------------------------------------------------------------------
// LOOPING MACROS
//--------------------------------------------------------------------------------------------

// Macros to automate looping through a t_obj_lst_map<>.
//
// This looks a bit messy, but it will simplify the look of egoboo codebase by
// hiding the code that defers the the creation and deletion of objects until loop termination.
//
// This process is necessary so that o iterators will be invalidated during the loop
// because of operations occurring inside the loop that increase or decrease the length of the list.

#define OBJ_LIST_BEGIN_LOOP_ALLOCATED(OBJ_T, LIST, IT, POBJ)  \
    { \
        int internal__##LIST##_start_depth = LIST.get_loop_depth(); \
        LIST.increment_loop_depth(); \
        for( LIST##_t::iterator internal__##IT = LIST.iterator_begin_allocated(); !LIST.iterator_finished(internal__##IT); LIST.iterator_increment_allocated(internal__##IT) ) \
        { \
            LIST##_t::lst_reference IT( *internal__##IT );\
            LIST##_t::container_type * POBJ##_cont = LIST.get_ptr( IT );\
            if( NULL == POBJ##_cont ) continue;\
            OBJ_T * POBJ = LIST##_t::container_type::get_data_ptr( POBJ##_cont ); \
            if( NULL == POBJ ) continue;\
            int internal__##LIST##_internal_depth = LIST.get_loop_depth();

#define OBJ_LIST_BEGIN_LOOP_DEFINED(OBJ_T, LIST, IT, POBJ) \
    { \
        int internal__##LIST##_start_depth = LIST.get_loop_depth(); \
        LIST.increment_loop_depth(); \
        for( LIST##_t::iterator internal__##IT = LIST.iterator_begin_defined(); !LIST.iterator_finished(internal__##IT); LIST.iterator_increment_defined(internal__##IT) ) \
        { \
            LIST##_t::container_type * POBJ##_cont = internal__##IT.pcont;\
            if( NULL == POBJ##_cont ) continue;\
            OBJ_T * POBJ = internal__##IT.pdata; \
            if( NULL == POBJ ) continue;\
            LIST##_t::lst_reference IT( *internal__##IT );\
            int internal__##LIST##_internal_depth = LIST.get_loop_depth();

#define OBJ_LIST_BEGIN_LOOP_PROCESSING(OBJ_T, LIST, IT, POBJ) \
    { \
        int internal__##LIST##_start_depth = LIST.get_loop_depth(); \
        LIST.increment_loop_depth(); \
        for( LIST##_t::iterator internal__##IT = LIST.iterator_begin_processing(); !LIST.iterator_finished(internal__##IT); LIST.iterator_increment_processing(internal__##IT) ) \
        { \
            LIST##_t::container_type * POBJ##_cont = internal__##IT.pcont;\
            if( NULL == POBJ##_cont ) continue;\
            OBJ_T * POBJ = internal__##IT.pdata; \
            if( NULL == POBJ ) continue;\
            LIST##_t::lst_reference IT( *internal__##IT );\
            int internal__##LIST##_internal_depth = LIST.get_loop_depth();

#define OBJ_LIST_BEGIN_LOOP_INGAME(OBJ_T, LIST, IT, POBJ) \
    { \
        int internal__##LIST##_start_depth = LIST.get_loop_depth(); \
        LIST.increment_loop_depth(); \
        for( LIST##_t::iterator internal__##IT = LIST.iterator_begin_ingame(); !LIST.iterator_finished(internal__##IT); LIST.iterator_increment_ingame(internal__##IT) ) \
        { \
            LIST##_t::container_type * POBJ##_cont = internal__##IT.pcont;\
            if( NULL == POBJ##_cont ) continue;\
            OBJ_T * POBJ = internal__##IT.pdata; \
            if( NULL == POBJ ) continue;\
            LIST##_t::lst_reference IT( *internal__##IT );\
            int internal__##LIST##_internal_depth = LIST.get_loop_depth();

//--------------------------------------------------------------------------------------------
/// The termination of all OBJ_LIST_BEGIN_LOOP_ALLOCATED()
#define OBJ_LIST_END_LOOP(LIST) \
    if(internal__##LIST##_internal_depth != LIST.get_loop_depth()) EGOBOO_ASSERT(bfalse); \
    } \
    LIST.decrement_loop_depth(); \
    if(internal__##LIST##_start_depth != LIST.get_loop_depth()) EGOBOO_ASSERT(bfalse); \
    LIST.cleanup(); \
    }

#define _egoboo_object_list_h
