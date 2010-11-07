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

/// \file egoboo_typedef_cpp.h
/// \details cpp-only definitions

#include "egoboo_typedef.h"

#include "clock.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// include these before the memory stuff because the Fluid Studios Memory manager
// redefines the new operator in a way that ther STL doesn't like.
// And we do not want mmgr to be tracking internal allocation inside the STL, anyway!

#if defined(_H_MMGR_INCLUDED)
#error If mmgr.h is included before this point, the remapping of new and delete will cause problems
#endif

#   if defined(USE_HASH)
#       if defined(__GNUC__)
#           include <backward/hash_map>
#           include <backward/hash_set>

// this is getting bloody ugly
template< typename _ty >
class LargeIntHasher
{
    public:
        size_t operator()( _ty * ptr ) const
        {
            return h(( size_t ) ptr );
        };

    private:
        __gnu_cxx::hash<size_t> h;
};

#           define EGOBOO_MAP __gnu_cxx::hash_map
#           define EGOBOO_SET __gnu_cxx::hash_set
#       elif defined(_MSC_VER)
#           include <hash_map>
#           include <hash_set>
#           define EGOBOO_MAP stdext::hash_map
#           define EGOBOO_SET stdext::hash_set
#       else
#           error I do not know the namespace containing hash_map and hash_set...
#       endif
#   else
#       include <map>
#       define EGOBOO_MAP std::map
#       include <set>
#       define EGOBOO_SET std::set
#   endif

#   include <deque>
#   include <stack>
#   include <queue>
#   include <exception>
#   include <algorithm>
#   include <functional>
#   include <vector>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

template < typename _ty, typename _ity > struct t_map;
template < typename _ty, typename _ity > struct t_deque;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// forward declaration of references

typedef struct s_oglx_texture oglx_texture_t;

struct ego_cap;
struct ego_obj_chr;
struct ego_team;
struct ego_eve;
struct ego_obj_enc;
struct ego_mad;
struct ego_player;
struct ego_pip;
struct ego_obj_prt;
struct ego_passage;
struct ego_shop;
struct ego_pro;
struct s_oglx_texture;
struct ego_billboard_data;
struct snd_looped_sound_data;
struct mnu_module;
struct ego_tx_request;

// forward declaration of the template
template< typename _data, size_t _sz > struct t_ego_obj_container;

struct ego_obj_chr;
struct ego_obj_enc;
struct ego_obj_prt;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// fix for the fact that assert is technically not supported in c++
class egoboo_exception : public std::exception
{
    protected:
        const char * what;
    public:
        egoboo_exception( const char * str ) : what( str ) {};
};

#define CPP_EGOBOO_ASSERT(X) if( !(X) ) { throw (std::exception*)(new egoboo_exception( #X )); }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// macros to use the high resolution timer for profiling
#define PROFILE_KEEP  0.9F
#define PROFILE_NEW  (1.0F - PROFILE_KEEP)

struct debug_profiler
{
    bool_t         on;
    ClockState_t * state;
    double         count;
    double         time, dt, dt_max, dt_min;

    double         rate, rate_min, rate_max;

    double         keep_amt, new_amt;

    debug_profiler( bool_t used, const char * name = "DEFAULT_NAME" )
    {
        on    = used;
        state = clk_create( name, -1 );
        reset( this );
        set_keep( PROFILE_KEEP );
    }

    ~debug_profiler() { clk_destroy( &state ); }

    void set_keep( double keep )
    {
        keep_amt = std::min( std::max( 0.0, keep ), 1.0 );
        new_amt  = 1.0F - keep_amt;
    }

    static debug_profiler * reset( debug_profiler * ptr )
    {
        if ( NULL != ptr )
        {
            ptr->count = ptr->time = 0.0F;
            ptr->dt = ptr->dt_max = ptr->dt_min = 0.0F;
            ptr->rate = ptr->rate_min = ptr->rate_max = 0.0F;
        }
        return ptr;
    }

    static double query( debug_profiler * ptr )
    {
        if ( NULL == ptr ) return 0.0F;

        return ptr->on ? ptr->calc() : 1.0F;
    }

    static debug_profiler * begin( debug_profiler * ptr )
    {
        if ( NULL != ptr && ptr->on )
        {
            clk_frameStep( ptr->state );
        }

        return ptr;
    }

    static debug_profiler * end( debug_profiler * ptr )
    {
        if ( NULL == ptr ) return NULL;

        if ( ptr->on )
        {
            ptr->update();
            ptr->count = ptr->count * ptr->keep_amt + ptr->new_amt * 1.0F;
            ptr->time  = ptr->time * ptr->keep_amt + ptr->new_amt * ptr->dt;
        }
        else
        {
            ptr->count += 1.0F;
        }

        return ptr;
    }

    static debug_profiler * end2( debug_profiler * ptr )
    {
        if ( NULL == ptr ) return ptr;

        if ( ptr->on )
        {
            ptr->update();

            ptr->count += 1.0F;
            ptr->time  += ptr->dt;
        }
        else
        {
            ptr->count += 1.0F;
        }

        return ptr;
    }

private:

    void update()
    {
        clk_frameStep( state );
        dt = clk_getFrameDuration( state );

        if ( 0.0F == count )
        {
            dt_max = dt_min = dt;
        }
        else
        {
            dt_min = std::min( dt, dt_min );
            dt_max = std::max( dt, dt_max );
        }
    }

    double calc()
    {
        double old_rate = rate;

        if ( NULL == this || 0.0F == count )
        {
            rate = 0.0F;
        }
        else
        {
            rate = time / count;
        }

        if ( 0.0F == old_rate && 0.0F != rate )
        {
            rate_min = rate;
            rate_max = rate;
        }
        else
        {
            rate_min = std::min( rate, rate_min );
            rate_max = std::min( rate, rate_max );
        }

        return rate;
    }
};

#define PROFILE_QUERY_STRUCT(PTR)  debug_profiler::query( &((PTR)->dbg_pro) )
#define PROFILE_BEGIN_STRUCT(PTR)  debug_profiler::begin( &((PTR)->dbg_pro) );
#define PROFILE_END_STRUCT(PTR)    debug_profiler::end( &((PTR)->dbg_pro) );
#define PROFILE_END2_STRUCT(PTR)   debug_profiler::end2( &((PTR)->dbg_pro) );

#define PROFILE_QUERY(NAME)  debug_profiler::query( &(dbg_pro_##NAME) )
#define PROFILE_BEGIN(NAME)  debug_profiler::begin( &(dbg_pro_##NAME) );
#define PROFILE_END(NAME)    debug_profiler::end( &(dbg_pro_##NAME) );
#define PROFILE_END2(NAME)   debug_profiler::end2( &(dbg_pro_##NAME) );

#if (PROFILE_LEVEL > 0)
#   define PROFILE_INIT_STRUCT(NAME)    dbg_pro(btrue, #NAME)
#   define PROFILE_DECLARE(NAME)        debug_profiler dbg_pro_##NAME(btrue, #NAME);
#else
#   define PROFILE_INIT_STRUCT(NAME)    dbg_pro(bfalse, #NAME)
#   define PROFILE_DECLARE(NAME)        debug_profiler dbg_pro_##NAME(bfalse, #NAME);
#endif

#define PROFILE_FREE(NAME)
#define PROFILE_INIT(NAME)           debug_profiler::reset( &(dbg_pro_##NAME) );
#define PROFILE_DECLARE_STRUCT(NAME) debug_profiler dbg_pro;
#define PROFILE_RESET(NAME)          debug_profiler::reset( &(dbg_pro_##NAME) );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// definition of the reference template

template <typename _ty>
struct t_reference
{
protected:
    REF_T    ref;          ///< the reference

public:

    explicit t_reference( REF_T v = REF_T( -1 ) /*, _ty * p = NULL*/ ) : ref( v ) /* , ptr( p ) */ {};

    t_reference<_ty> & operator = ( const REF_T & rhs )
    {
        ref = rhs;
        return *this;
    }

    REF_T get_value() const { return ref; }

    friend bool operator == ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref == rhs; }
    friend bool operator != ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref != rhs; }
    friend bool operator >= ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref >= rhs; }
    friend bool operator <= ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref <= rhs; }
    friend bool operator < ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref <  rhs; }
    friend bool operator > ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref >  rhs; }

    friend bool operator == ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs == rhs.ref; }
    friend bool operator != ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs != rhs.ref; }
    friend bool operator >= ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs >= rhs.ref; }
    friend bool operator <= ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs <= rhs.ref; }
    friend bool operator < ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs <  rhs.ref; }
    friend bool operator > ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs >  rhs.ref; }

    friend bool operator == ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref == rhs.ref; }
    friend bool operator != ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref != rhs.ref; }
    friend bool operator >= ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref >= rhs.ref; }
    friend bool operator <= ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref <= rhs.ref; }
    friend bool operator < ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref <  rhs.ref; }
    friend bool operator > ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref >  rhs.ref; }

    t_reference<_ty> & operator ++ ( int ) { ref++; return *this; }
    t_reference<_ty> & operator ++ ()      { ++ref; return *this; }

    t_reference<_ty> & operator -- ( int ) { if ( 0 == ref ) CPP_EGOBOO_ASSERT( NULL == "t_reference()::operator -- underflow" ); ref--; return *this; }
    t_reference<_ty> & operator -- ()      { if ( 0 == ref ) CPP_EGOBOO_ASSERT( NULL == "t_reference()::operator -- underflow" ); --ref; return *this; }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple array template

template < typename _ty, size_t _sz >
struct t_ary
{
private:
    _ty    lst[_sz];

public:
    _ty & operator []( const t_reference<_ty> & ref ) { const REF_T val = ref.get_value(); /* if ( val > _sz ) CPP_EGOBOO_ASSERT( NULL == "t_ary::operator[] - index out of range" ); */ return lst[val]; }
    _ty * operator + ( const t_reference<_ty> & ref ) { const REF_T val = ref.get_value(); /* if ( val > _sz ) CPP_EGOBOO_ASSERT( NULL == "t_ary::operator + - index out of range" ); */ return lst + val; }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// this struct must be inherited by any class that wants to use t_allocator_dynamic<>.
///
/// \note The compiler will complain that it could not generate an assignment operator,
/// but this is to be expected, since we want no two objects that interact with the
/// allocator to have the same id

struct allocator_client
{
    static const ego_uint invalid_value = ego_uint( ~0L );

    explicit allocator_client( const ego_uint guid = invalid_value, const ego_uint index = invalid_value ) :
            _id( guid ),
            _ix( index )
    {};

    ~allocator_client()
    {
        // set the _id to a special value on deletion so we could verify that
        // an element of the "free store" has been deleted
        *(( ego_uint* )( &_id ) ) = invalid_value;
        *(( ego_uint* )( &_ix ) ) = invalid_value;
    };

    ego_uint get_id()          const { return _id; };
    bool     has_valid_id()    const { return _id != invalid_value; };

    ego_uint get_index()       const { return _ix; };
    bool     has_valid_index( const ego_uint max_index = ego_uint( ~0L ) ) const { return _ix < max_index; };

    //---- static accessors
    // make these functions static so we can don't have to worry whether the pointer is NULL

    static bool has_valid_id( const allocator_client * ptr )
    {
        return NULL == ptr ? bfalse : ptr->has_valid_id();
    }

    static bool has_valid_index( const allocator_client * ptr, const ego_uint max_index = invalid_value )
    {
        return NULL == ptr ? bfalse : ptr->has_valid_index( max_index );
    }

    static bool is_valid( const allocator_client * ptr, const ego_uint max_index = invalid_value )
    {
        return NULL == ptr ? bfalse : ptr->has_valid_id() && ptr->has_valid_index( max_index );
    }

private:
    const ego_uint _id;
    const ego_uint _ix;
};

//--------------------------------------------------------------------------------------------

/// an allocator that will buffer element creation and deletion using
/// a private heap

template < typename _ty >
struct t_allocator_dynamic
{
#if defined(USE_HASH) && defined(__GNUC__)
    typedef typename EGOBOO_SET<_ty *, LargeIntHasher<_ty> >                heap_type;
    typedef typename EGOBOO_SET<_ty *, LargeIntHasher<_ty> >::iterator      heap_iterator;
#else
    typedef typename EGOBOO_SET<_ty *>                heap_type;
    typedef typename EGOBOO_SET<_ty *>::iterator      heap_iterator;
#endif

    typedef typename std::pair< heap_iterator, bool > heap_pair;

    //---- construction and destruction

    t_allocator_dynamic( size_t creation_size, size_t heap_size ) :
            creation_limit( creation_size ),
            heap_limit( heap_size )
    {
        creation_count = 0;
        guid           = 0;
    }

    ~t_allocator_dynamic() { garbage_collect( 0 ); }

    //---- create and destroy elements of type _ty

    /// Generate a _ty.
    /// Returns NULL if the creation_limit has been met or exceeded.
    _ty * create( _ty * ptr = NULL )
    {
        return heap_new( ptr );
    }

    /// destroy or recycle a _ty
    /// returns NULL if it was successfully dealt with
    _ty * destroy( _ty * delete_ptr )
    {
        if ( NULL == delete_ptr ) return delete_ptr;

        return heap_delete( delete_ptr );
    }

    //---- accessors

    /// do not store more than this amount of element on the heap
    void set_heap_limit( size_t limit )
    {
        garbage_collect( limit );
        heap_limit = limit;
    }

    /// do not create more than this number of elements
    void set_creation_limit( size_t limit )
    {
        creation_limit = limit;
    }

    /// How many elements can we create at this time?
    /// This value could be negative if set_creaton_limit() was used
    /// to reduce the limit below the number of already allocated units
    int get_free_count()
    {
        return creation_limit - creation_count;
    }

    /// How many elements have been created?
    size_t get_used_count()
    {
        return creation_count;
    }

private:

    /// a specialization of the new command
    _ty * heap_new( _ty * new_ptr = NULL )
    {

        if ( NULL != new_ptr )
        {
            // a specialzation of placement-new
            new_ptr = new( new_ptr ) _ty( guid, guid );
        }
        else
        {
            // try to allocate a pointer from the heap
            if ( NULL == new_ptr )
            {
                new_ptr = heap_allocate();

                if ( NULL != new_ptr )
                {
                    // we have an pre-existing pointer
                    // call its constructor using placement-new
                    new_ptr = new( new_ptr ) _ty( guid, guid );
                    creation_count++;
                }
            }

            if ( NULL == new_ptr )
            {
                // there is no cached pointer,
                // create a new element in the normal way
                new_ptr = new _ty( guid, guid );

                if ( NULL != new_ptr )
                {
                    creation_count++;
                }
            }
        }

        // update the guid for any non-NULL pointer
        if ( NULL != new_ptr )
        {
            guid++;
        }

        return new_ptr;
    }

    /// a specialization of the delete command
    _ty * heap_delete( _ty * delete_ptr )
    {
        // NULL pointers can't be deleted
        if ( NULL == delete_ptr ) return NULL;

        if ( heap_free( delete_ptr ) )
        {
            // call the destructor (if it exists) directly
            delete_ptr->~_ty();
            delete_ptr = NULL;
        }
        else
        {
            // delete the pointer directly
            delete delete_ptr;
            delete_ptr = NULL;
        }

        if (( NULL == delete_ptr ) && ( creation_count > 0 ) )
        {
            creation_count--;
        }

        return delete_ptr;
    }

    /// return a free element from the heap
    _ty * heap_allocate()
    {
        _ty * element_ptr = NULL;
        heap_iterator it;

        if ( creation_count >= creation_limit ) return NULL;

        // just in case there is a NULL pointer in the EGOBOO_SET
        while ( !_heap.empty() && NULL == element_ptr )
        {
            it = _heap.begin();
            element_ptr = *it;
            _heap.erase( it );
        }

        return element_ptr;
    }

    /// try to place an element onto the heap
    bool_t heap_free( _ty * element_ptr )
    {
        // if there is a duplicate of this pointer in the _heap, remove it
        heap_remove( element_ptr );

        // this is OK to put here. allow kill_element() to remove a NULL pointer
        if ( NULL == element_ptr ) return bfalse;

        // if there is too much element, don't add it
        if ( _heap.size() + 1 > heap_limit ) return bfalse;

        // try to insert the pointer
        heap_pair rv = _heap.insert( element_ptr );

        return rv.second;
    }

    /// remove an element from the heap
    bool_t  heap_remove( _ty * element_ptr )
    {
        bool_t found = bfalse;

        // remove the goven pointer from the _heap
        if ( !_heap.empty() )
        {
            typename heap_type::iterator it = _heap.find( element_ptr );
            if ( it != _heap.end() )
            {
                _heap.erase( it );
                found = btrue;
            }
        }

        return found;
    }

    /// get rid of "useless" data
    void garbage_collect( size_t size )
    {
        heap_iterator it;
        _ty * tmp_ptr = NULL;

        // erase all excess elements from the heap
        // and delete them
        while ( _heap.size() > size )
        {
            // erase
            it = _heap.begin();
            tmp_ptr = *it;
            _heap.erase( it );

            if ( NULL != tmp_ptr ) { delete tmp_ptr; tmp_ptr = NULL; }
        }
    }

    ego_sint creation_count;

    size_t   creation_limit;
    size_t   heap_limit;

    ego_uint guid;

    heap_type _heap;
};

//--------------------------------------------------------------------------------------------

/// an allocator that will simulate buffered creation and deletion using
/// a statically allocated array

template < typename _ty, size_t _sz >
struct t_allocator_static
{
#if defined(USE_HASH) && defined(__GNUC__)
    typedef typename EGOBOO_SET<_ty *, LargeIntHasher<_ty> >                heap_type;
    typedef typename EGOBOO_SET<_ty *, LargeIntHasher<_ty> >::iterator      heap_iterator;
#else
    typedef typename EGOBOO_SET<_ty *>                heap_type;
    typedef typename EGOBOO_SET<_ty *>::iterator      heap_iterator;
#endif

    typedef typename std::pair< heap_iterator, bool > heap_pair;

    static const size_t invalid_index = size_t( -1 );

    //---- construction and destruction

    t_allocator_static( size_t creation_size = _sz ) :
            creation_limit( creation_size )
    { init(); }

    ~t_allocator_static() { deinit(); }

    void deinit()
    {
        // eliminate the _heap
        garbage_collect( 0 );

        // destroy any other elements that haven't been destroyed
        for ( int cnt = 0; cnt < _sz; cnt++ )
        {
            if ( _static_ary[cnt].allocator_client::get_id() != ego_uint( ~0L ) )
            {
                _static_ary[cnt].~_ty();
            }
        }

        creation_count = 0;
        guid           = 0;
    }

    void init()
    {
        deinit();

        for ( int cnt = 0; cnt < _sz; cnt++ )
        {
            const typename heap_type::value_type dummy_ptr = _static_ary + cnt;
            _heap.insert( dummy_ptr );
        }
    }

    //---- create and destroy elements of type _ty

    /// Generate a _ty.
    /// Returns NULL if the creation_limit has been met or exceeded.
    _ty * create( _ty * ptr = NULL )
    {
        return heap_new( ptr );
    }

    /// destroy or recycle a _ty
    /// returns NULL if it was successfully dealt with
    _ty * destroy( _ty * delete_ptr )
    {
        if ( NULL == delete_ptr ) return delete_ptr;

        return heap_delete( delete_ptr );
    }

    //---- accessors

    /// do not create more than this number of elements
    void set_creation_limit( size_t limit )
    {
        limit = std::min( _sz, limit );
        creation_limit = limit;
    }

    /// How many elements can we create at this time?
    /// This value could be negative if set_creaton_limit() was used
    /// to reduce the limit below the number of already allocated units
    int get_free_count()
    {
        return creation_limit - creation_count;
    }

    /// How many elements have been created?
    size_t get_used_count()
    {
        return creation_count;
    }

    _ty * get_element_ptr_raw( const size_t index )
    {
        if ( !valid_index_range( index ) ) return NULL;

        return _static_ary + index;
    }

    const _ty * get_element_ptr_raw( const size_t index ) const
    {
        if ( !valid_index_range( index ) ) return NULL;

        return _static_ary + index;
    }

    _ty * get_element_ptr( const size_t index )
    {
        _ty * ptr = get_element_ptr_raw( index );

        return !element_allocated( ptr ) ? NULL : ptr;
    }

    const _ty * get_element_ptr( const size_t index ) const
    {
        const _ty * ptr = get_element_ptr_raw( index );

        return !element_allocated( ptr ) ? NULL : ptr;
    }

    size_t get_element_idx( const _ty * pelement )
    {
        if ( !element_allocated( pelement ) ) return allocator_client::invalid_value;

        return static_cast<const allocator_client*>( pelement )->get_index();
    }

    size_t get_element_idx( const _ty * pelement ) const
    {
        if ( !element_allocated( pelement ) ) return allocator_client::invalid_value;

        return static_cast<const allocator_client*>( pelement )->get_index();
    }

    bool_t valid_index_range( const size_t index ) const { return index < _sz; }
    bool_t element_allocated( const _ty * pelement ) const
    {
        return NULL == pelement ? bfalse : allocator_client::is_valid( pelement, _sz );
    }

private:

    /// a kludge to get the "index" to _static_ary of the pointer, ptr
    size_t find_element_index( const _ty * ptr ) const
    {
        if ( ptr < _static_ary ) return invalid_index;

        const char * test_ptr = ( const char * )ptr;
        const char * base_ptr = ( const char * )_static_ary;

        // determine the offset in bytes
        size_t offset = test_ptr - base_ptr;

        // determine the nearest index of the value
        size_t index = offset / sizeof( _ty );

        // alignmet error
        ego_sint err = ego_sint( test_ptr ) - ego_sint(( const char * )( _static_ary + index ) );

        return ( 0 == err ) ? index : invalid_index;
    }

    bool_t valid_element_ptr( const _ty * ptr ) const
    {
        return invalid_index != find_element_index( ptr );
    }

    /// a specialization of the new command
    ///
    /// \note using both find_element_index() and valid_element_ptr() calls the
    /// function find_element_index() twice
    _ty * heap_new( _ty * new_ptr = NULL )
    {
        // assume that the optional new_ptr does not have a valid index
        size_t index = invalid_index;

        // handle optional parameters
        if ( NULL != new_ptr )
        {
            // verify that any element pointer we are passed is a inside of,
            // and properly aligned with, our heap

            // grab the element's index, if it exists
            index = find_element_index( new_ptr );

            // if not, blank out the pointer
            if ( invalid_index == index ) new_ptr = NULL;
        }

        if ( invalid_index != index )
        {
            // re-initialize the old element using
            // a specialzation of placement-new
            new_ptr = new( new_ptr ) _ty( guid, index );
        }
        else
        {
            // allocate a new element from teh haep
            new_ptr = heap_allocate();

            // find its index
            index = find_element_index( new_ptr );

            // if the element pointer is valid, construct it using placement-new
            if ( invalid_index != index )
            {
                new_ptr = new( new_ptr ) _ty( guid, index );
                creation_count++;
            }
        }

        // update the guid for any non-NULL pointer
        if ( NULL != new_ptr )
        {
            guid++;
        }

        return new_ptr;
    }

    /// a specialization of the delete command
    _ty * heap_delete( _ty * delete_ptr )
    {
        // NULL pointers can't be deleted
        if ( !valid_element_ptr( delete_ptr ) ) return delete_ptr;

        if ( heap_free( delete_ptr ) )
        {
            // call the destructor (if it exists) directly
            delete_ptr->~_ty();
            delete_ptr = NULL;
        }

        if (( NULL == delete_ptr ) && ( creation_count > 0 ) )
        {
            creation_count--;
        }

        return delete_ptr;
    }

    /// return a free element from the heap
    _ty * heap_allocate()
    {
        _ty * element_ptr = NULL;
        heap_iterator it;

        if ( size_t( std::max( 0, creation_count ) ) >= creation_limit ) return NULL;

        // just in case there is a NULL pointer in the EGOBOO_SET
        while ( !_heap.empty() && NULL == element_ptr )
        {
            it = _heap.begin();

            element_ptr = *it;

            _heap.erase( it );
        }

        return element_ptr;
    }

    /// try to place an element onto the heap
    bool_t heap_free( _ty * element_ptr )
    {
        if ( !valid_element_ptr( element_ptr ) ) return bfalse;

        // if there is a duplicate of this pointer in the _heap, remove it
        heap_remove( element_ptr );

        // this is OK to put here. allow kill_element() to remove a NULL pointer
        if ( NULL == element_ptr ) return bfalse;

        // if there is too much element, don't add it
        if ( _heap.size() + 1 > _sz ) return bfalse;

        // try to insert the pointer
        heap_pair rv = _heap.insert( element_ptr );

        return rv.second;
    }

    /// remove an element from the heap
    bool_t  heap_remove( _ty * element_ptr )
    {
        bool_t found = bfalse;

        if ( !valid_element_ptr( element_ptr ) ) return bfalse;

        // remove the given pointer from the _heap
        if ( !_heap.empty() )
        {
            typename heap_type::iterator it = _heap.find( element_ptr );
            if ( it != _heap.end() )
            {
                _heap.erase( it );
                found = btrue;
            }
        }

        return found;
    }

    /// get rid of "useless" data
    void garbage_collect( size_t size )
    {
        heap_iterator it;
        _ty *         tmp_ptr = NULL;

        // erase all excess elements from the heap
        // and delete them
        while ( _heap.size() > size )
        {
            it = _heap.begin();

            tmp_ptr = *it;

            _heap.erase( it );

            if ( NULL != tmp_ptr )
            {
                tmp_ptr = NULL;
            }
        }
    }

    ego_sint creation_count;

    size_t  creation_limit;

    ego_uint guid;

    _ty       _static_ary[_sz];
    heap_type _heap;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
struct t_map
{
    /// a custom iterator that tracks additions and removals to the
    /// t_map::_map
    struct iterator
    {
        friend struct t_map<_ty, _ity>;

private:

        // some internal typedefs
        typedef typename std::pair< const _ity, const _ty * >  _val_t;

#if defined(USE_HASH) && defined(__GNUC__)
        typedef typename EGOBOO_MAP< const _ity, const _ty *, LargeIntHasher<const _ity> > _map_t;
#else
        typedef typename EGOBOO_MAP< const _ity, const _ty * > _map_t;
#endif

        typedef typename _map_t::iterator                      _it_t;

public:

        explicit iterator() { _id = ego_uint( ~0L ); _valid = bfalse; }

        /// overload this operator to pass it on to the base iterator
        const _val_t& operator*() const
        {
            return _i.operator*();
        }

        /// overload this operator to pass it on to the base iterator
        const _val_t *operator->() const
        {
            return _i.operator->();
        }

        iterator & get_map_iterator() { return *this; }
        const iterator & get_map_iterator() const { return *this; }

private:
        explicit iterator( _it_t it, ego_uint id )
        {
            _i     = it;
            _id    = id;
            _valid = btrue;
        }

        _it_t     _i;
        ego_uint  _id;
        bool_t    _valid;
    };

private:

    // some private typedefs
#if defined(USE_HASH) && defined(__GNUC__)
    typedef typename EGOBOO_MAP< const _ity, const _ty *, LargeIntHasher<_ity> > cache_type;
#else
    typedef typename EGOBOO_MAP< const _ity, const _ty * > cache_type;
#endif

    typedef t_reference<_ty>                               reference_type;

public:

    t_map() { _id = ego_uint( ~0L ); }

    ego_uint get_id() { return _id; }

    _ty *    get_ptr( const reference_type & ref );
    bool_t   has_ref( const reference_type & ref );
    bool_t   add( const reference_type & ref, const _ty * val );
    bool_t   remove( const reference_type & ref );

    void     clear() { _cache.clear(); _id = ego_uint( ~0L ); };
    size_t   size()  { return _cache.size(); }

    iterator iterator_begin()
    {
        iterator rv;

        if ( !_cache.empty() )
        {
            rv = iterator( _cache.begin(), _id );
        }

        return rv;
    }

    bool_t iterator_end( iterator & it )
    {
        // if the iterator is not valid, we ARE at the end
        // this function already tests for the
        // it._i == _cache.end() condition, so no need to repeat ourselves
        return !iterator_validate( it );
    }

    iterator & iterator_increment( iterator & it )
    {
        // if the iterator is not valid, this call will invalidate it.
        // then just send back the invalid iterator
        if ( !iterator_validate( it ) ) return it;

        it._i++;

        return it;
    }

protected:

    iterator & iterator_invalidate( iterator & it )
    {
        it._id    = ego_uint( ~0L );
        it._valid = bfalse;
        it._i     = _cache.end();

        return it;
    }

    bool_t iterator_validate( iterator & it )
    {
        if ( !it._valid )
        {
            return bfalse;
        }

        if ( _id != it._id )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        if ( _cache.empty() )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        if ( _cache.end() == it._i )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        return btrue;
    }

private:
    ego_uint   _id;
    cache_type _cache;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
struct t_deque
{
    static const ego_uint invalid_id = ego_uint( ~0L );

    /// a custom iterator that tracks additions and removals to the
    /// t_deque::_deque
    struct iterator
    {
        friend struct t_deque<_ty, _ity>;

private:

        // internal typedefs
        typedef          _ity                           _val_t;
        typedef typename std::deque< _val_t >           _deque_t;
        typedef typename std::deque< _val_t >::iterator _it_t;

public:

        explicit iterator() { _id = invalid_id; _valid = bfalse; }

        /// overload this operator to pass it on to the base iterator
        const _val_t& operator*() const
        {
            return _i.operator*();
        }

        /// overload this operator to pass it on to the base iterator
        const _val_t *operator->() const
        {
            return _i.operator->();
        }

        iterator & deque_iterator() { return *this; }
        const iterator & deque_iterator() const { return *this; }

private:
        explicit iterator( _it_t it, ego_uint id )
        {
            _i     = it;
            _id    = id;
            _valid = btrue;
        }

        _it_t     _i;
        ego_uint  _id;
        bool_t    _valid;
    };

private:

    // internal typedefs
    typedef typename std::deque< _ity >              cache_type;
    typedef typename std::deque< _ity >::iterator    cache_iterator;
    typedef          t_reference<_ty>                reference_type;

public:

    t_deque() : _id( invalid_id ) {}

    ego_uint get_id() { return _id; }

    cache_iterator find_ref( const reference_type & ref );
    bool_t         has_ref( const reference_type & ref );
    bool_t         add( const reference_type & ref );
    bool_t         remove( const reference_type & ref );

    void     clear() { _cache.clear(); _id = invalid_id; };
    size_t   size()  { return _cache.size(); }

    iterator iterator_begin()
    {
        iterator rv;

        if ( !_cache.empty() )
        {
            rv = iterator( _cache.begin(), _id );
        }

        return rv;
    }

    bool_t iterator_end( iterator & it )
    {
        // if the iterator is not valid, we ARE at the end
        // this function already tests for the
        // it._i == _cache.end() condition, so no need to repeat ourselves
        return !iterator_validate( it );
    }

    iterator & iterator_increment( iterator & it )
    {
        // if the iterator is not valid, this call will invalidate it.
        // then just send back the invalid iterator
        if ( !iterator_validate( it ) ) return it;

        it._i++;

        return it;
    }

protected:

    iterator & iterator_invalidate( iterator & it )
    {
        it._id    = invalid_id;
        it._valid = bfalse;
        it._i     = _cache.end();

        return it;
    }

    bool_t iterator_validate( iterator & it )
    {
        if ( !it._valid )
        {
            return bfalse;
        }

        if ( _id != it._id )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        if ( _cache.empty() )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        if ( _cache.end() == it._i )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        return btrue;
    }

private:

    ego_uint   _id;
    cache_type _cache;

    void increment_id()
    {
        if ( invalid_id == _id )
        {
            _id = 1;
        }
        else
        {
            _id++;
        }
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

template < typename _ty, size_t _sz >
struct t_list
{
    ego_uint update_guid;

    size_t   _used_count;
    size_t   _free_count;
    size_t   used_ref[_sz];
    size_t   free_ref[_sz];

    t_ary<_ty, _sz> lst;

    size_t   used_count() { return _used_count; }
    size_t   free_count() { return _free_count; }

    t_list() { _used_count = _free_count = 0; update_guid = 0; }

    _ty * get_ptr( const t_reference<_ty> & ref );

    size_t free_full() { return free_count() >= _sz; }
    size_t used_full() { return used_count >= _sz; }

    size_t count_free() { return free_count(); }
    size_t count_used() { return used_count; }

    bool_t in_range_ref( const t_reference<_ty> & ref ) { REF_T tmp = ref.get_value(); return tmp > 0 && tmp < _sz; };

    egoboo_rv add_free( const t_reference<_ty> & ref );
    egoboo_rv add_used( const t_reference<_ty> & ref );

    bool_t    remove_free( const t_reference<_ty> & ref );
    bool_t    remove_used( const t_reference<_ty> & ref );

    egoboo_rv free_one( const t_reference<_ty> & ref );
    size_t    get_free();

    void shrink_free();
    void shrink_used();

    void compact_free();
    void compact_used();

protected:
    int get_used_list_index( const t_reference<_ty> & ref );
    int get_free_list_index( const t_reference<_ty> & ref );

    bool_t remove_free_index( int index );
    bool_t remove_used_index( int index );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple stack template

template < typename _ty, size_t _sz >
struct t_stack
{
    ego_uint update_guid;
    ego_sint count;

    bool_t valid_idx( const ego_sint index ) const { return index > 0 && index < _sz; }
    bool_t in_range_ref( const t_reference<_ty> & ref ) const { return ref.get_value() < _sz; }

    t_stack() { count = 0; update_guid = 0; }

    _ty & operator []( const t_reference<_ty> & ref )
    {
        CPP_EGOBOO_ASSERT( in_range_ref( ref ) );
        return lst[ref];
    }

    const _ty & operator []( const t_reference<_ty> & ref ) const
    {
        CPP_EGOBOO_ASSERT( in_range_ref( ref ) );
        return lst[ref];
    }

    _ty * operator + ( const t_reference<_ty> & ref )
    {
        CPP_EGOBOO_ASSERT( in_range_ref( ref ) );
        return lst + ref;
    }

    const _ty * operator + ( const t_reference<_ty> & ref ) const
    {
        CPP_EGOBOO_ASSERT( in_range_ref( ref ) );
        return lst + ref;
    }

private:
    t_ary<_ty, _sz> lst;

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a template-like declaration of a dynamically allocated array

template<typename _ty>
struct t_dary : public std::vector< _ty >
{
    using std::vector< _ty >::size;

    size_t top;

    t_dary( size_t sz = 0 )
    {
        top = 0;
        if ( 0 != sz ) std::vector< _ty >::resize( sz );
    }

    static bool_t   alloc( t_dary * pary, size_t sz );
    static bool_t   dealloc( t_dary * pary );

    static size_t   get_top( t_dary * pary );
    static size_t   get_size( t_dary * pary );

    static _ty *    pop_back( t_dary * pary );
    static bool_t   push_back( t_dary * pary, _ty val );
    static void     clear( t_dary * pary );

    _ty & operator []( size_t offset )
    {
        CPP_EGOBOO_ASSERT( offset < size() );
        return std::vector<_ty>::operator []( offset );
    }

    _ty * operator + ( size_t offset )
    {
        if ( offset > size() ) return NULL;

        return &( operator []( offset ) );
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a template-like declaration of a statically allocated array
template < typename _ty, size_t _sz >
struct t_sary
{
    int count;
    _ty ary[_sz];
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// forward declaration of the containers
typedef t_ego_obj_container< ego_obj_chr, MAX_CHR >  ego_chr_container;
typedef t_ego_obj_container< ego_obj_enc, MAX_ENC >  ego_enc_container;
typedef t_ego_obj_container< ego_obj_prt, MAX_PRT >  ego_prt_container;

typedef t_reference< ego_chr_container >   CHR_REF;
typedef t_reference< ego_enc_container >   ENC_REF;
typedef t_reference< ego_prt_container >   PRT_REF;

typedef t_reference<ego_cap>               CAP_REF;
typedef t_reference<ego_team>              TEAM_REF;
typedef t_reference<ego_eve>               EVE_REF;
typedef t_reference<ego_mad>               MAD_REF;
typedef t_reference<ego_player>            PLA_REF;
typedef t_reference<ego_pip>               PIP_REF;
typedef t_reference<ego_passage>           PASS_REF;
typedef t_reference<ego_shop>              SHOP_REF;
typedef t_reference<ego_pro>               PRO_REF;
typedef t_reference<oglx_texture_t>        TX_REF;
typedef t_reference<ego_billboard_data>    BBOARD_REF;
typedef t_reference<snd_looped_sound_data> LOOP_REF;
typedef t_reference<mnu_module>            MOD_REF;
typedef t_reference<MOD_REF>               MOD_REF_REF;
typedef t_reference<ego_tx_request>        TREQ_REF;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// forward declaration of dynamic arrays of common types

typedef t_dary<  char >  char_ary;
typedef t_dary< short >  short_ary;
typedef t_dary<   int >  int_ary;
typedef t_dary< float >  float_ary;
typedef t_dary<double >  double_ary;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define _egoboo_typedef_cpp_h
