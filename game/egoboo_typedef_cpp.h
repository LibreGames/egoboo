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

/// @file egoboo_typedef_cpp.h
/// @details cpp-only definitions

#include "clock.h"

#if !defined(__cplusplus)
#    error egoboo_typedef_cpp.h should only be included if you are compling as c++
#endif

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
#define PROFILE_KEEP  0.9
#define PROFILE_NEW  (1.0 - PROFILE_KEEP)

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
        new_amt  = 1.0 - keep_amt;
    }

    static debug_profiler * reset( debug_profiler * ptr )
    {
        if ( NULL != ptr )
        {
            ptr->count = ptr->time = 0.0;
            ptr->dt = ptr->dt_max = ptr->dt_min = 0.0;
            ptr->rate = ptr->rate_min = ptr->rate_max = 0.0;
        }
        return ptr;
    }

    static double query( debug_profiler * ptr )
    {
        if ( NULL == ptr ) return 0.0;

        return ptr->on ? ptr->calc() : 1.0;
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
            ptr->count = ptr->count * ptr->keep_amt + ptr->new_amt * 1.0;
            ptr->time  = ptr->time * ptr->keep_amt + ptr->new_amt * ptr->dt;
        }
        else
        {
            ptr->count += 1.0;
        }

        return ptr;
    }

    static debug_profiler * end2( debug_profiler * ptr )
    {
        if ( NULL == ptr ) return ptr;

        if ( ptr->on )
        {
            ptr->update();

            ptr->count += 1.0;
            ptr->time  += ptr->dt;
        }
        else
        {
            ptr->count += 1.0;
        }

        return ptr;
    }

private:

    void update()
    {
        clk_frameStep( state );
        dt = clk_getFrameDuration( state );

        if ( 0.0 == count )
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

        if ( NULL == this || 0.0 == count )
        {
            rate = 0.0;
        }
        else
        {
            rate = time / count;
        }

        if ( 0.0 == old_rate && 0.0 != rate )
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

#if defined(DEBUG_PROFILE) && EGO_DEBUG
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
    //_ty    * ptr;          ///< not yet used, needed to allow for template specilization?
    //unsigned update_guid;  ///< the list guid at the time the reference was generated

public:

    explicit t_reference( REF_T v = REF_T( -1 ) /*, _ty * p = NULL*/ ) : ref( v ) /* , ptr( p ) */ {};

    t_reference<_ty> & operator = ( const REF_T & rhs )
    {
        ref = rhs;
        //ptr = NULL;
        return *this;
    }

    const REF_T get_value() const { return ref; }

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
struct t_cpp_ary
{
private:
    _ty    lst[_sz];

public:
    _ty & operator []( const t_reference<_ty> & ref ) { const REF_T val = ref.get_value(); /* if ( val > _sz ) CPP_EGOBOO_ASSERT( NULL == "t_cpp_ary::operator[] - index out of range" ); */ return lst[val]; }
    _ty * operator + ( const t_reference<_ty> & ref ) { const REF_T val = ref.get_value(); /* if ( val > _sz ) CPP_EGOBOO_ASSERT( NULL == "t_cpp_ary::operator + - index out of range" ); */ return lst + val; }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
struct t_cpp_map
{
private:

    typedef typename EGOBOO_MAP< const _ity, const _ty * > map_type;
    typedef typename map_type::iterator                    map_iterator;
    typedef typename t_reference<_ty>                      reference_type;

    unsigned _id;
    map_type _map;

public:

    /// a custom iterator that tracks additions and removals to the
    /// t_cpp_map::_map
    struct iterator
    {
        friend struct t_cpp_map<_ty, _ity>;

        explicit iterator() { _id = unsigned( -1 ); _valid = bfalse; }

        typedef typename std::pair< const _ity, const _ty * >  _val_t;
        typedef typename EGOBOO_MAP< const _ity, const _ty * > _map_t;
        typedef typename _map_t::iterator                      _it_t;

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

private:
        explicit iterator( _it_t it, unsigned id )
        {
            _i     = it;
            _id    = id;
            _valid = btrue;
        }

        _it_t     _i;
        unsigned  _id;
        bool_t    _valid;
    };

    t_cpp_map() { _id = unsigned( -1 ); }

    unsigned get_id() { return _id; }

    _ty *    get_ptr( const reference_type & ref );
    bool_t   has_ref( const reference_type & ref );
    bool_t   add( const reference_type & ref, const _ty * val );
    bool_t   remove( const reference_type & ref );

    void     clear() { _map.clear(); _id = unsigned( -1 ); };
    size_t   size()  { return _map.size(); }

    iterator iterator_begin()
    {
        iterator rv;

        if ( !_map.empty() )
        {
            rv = iterator( _map.begin(), _id );
        }

        return rv;
    }

    bool_t iterator_end( iterator & it )
    {
        // if the iterator is not valid, we ARE at the end
        // this function already tests for the
        // it._i == _map.end() condition, so no need to repeat ourselves
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
        it._id    = unsigned( -1 );
        it._valid = bfalse;
        it._i     = _map.end();

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

        if ( _map.empty() )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        if ( _map.end() == it._i )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        return btrue;
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
struct t_cpp_deque
{
private:
    typedef typename std::deque< _ity >              deque_type;
    typedef typename std::deque< _ity >::iterator    deque_iterator;
    typedef typename t_reference<_ty>                reference_type;

    unsigned   _id;
    deque_type _deque;

public:

    /// a custom iterator that tracks additions and removals to the
    /// t_cpp_deque::_deque
    struct iterator
    {
        friend struct t_cpp_deque<_ty, _ity>;

        explicit iterator() { _id = unsigned( -1 ); _valid = bfalse; }

        typedef typename _ity                           _val_t;
        typedef typename std::deque< _val_t >           _deque_t;
        typedef typename std::deque< _val_t >::iterator _it_t;

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

private:
        explicit iterator( _it_t it, unsigned id )
        {
            _i     = it;
            _id    = id;
            _valid = btrue;
        }

        _it_t     _i;
        unsigned  _id;
        bool_t    _valid;
    };

    t_cpp_deque() { _id = unsigned( -1 ); }

    unsigned get_id() { return _id; }

    deque_iterator find_ref( const reference_type & ref );
    bool_t         has_ref( const reference_type & ref );
    bool_t         add( const reference_type & ref );
    bool_t         remove( const reference_type & ref );

    void     clear() { _deque.clear(); _id = unsigned( -1 ); };
    size_t   size()  { return _deque.size(); }

    iterator iterator_begin()
    {
        iterator rv;

        if ( !_deque.empty() )
        {
            rv = iterator( _deque.begin(), _id );
        }

        return rv;
    }

    bool_t iterator_end( iterator & it )
    {
        // if the iterator is not valid, we ARE at the end
        // this function already tests for the
        // it._i == _deque.end() condition, so no need to repeat ourselves
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
        it._id    = unsigned( -1 );
        it._valid = bfalse;
        it._i     = _deque.end();

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

        if ( _deque.empty() )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        if ( _deque.end() == it._i )
        {
            iterator_invalidate( it );
            return bfalse;
        }

        return btrue;
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

template < typename _ty, size_t _sz >
struct t_cpp_list
{
    unsigned update_guid;

    size_t   _used_count;
    size_t   _free_count;
    size_t   used_ref[_sz];
    size_t   free_ref[_sz];

    t_cpp_ary<_ty, _sz> lst;

    size_t   used_count() { return _used_count; }
    size_t   free_count() { return _free_count; }

    t_cpp_list() { _used_count = _free_count = 0; update_guid = 0; }

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
struct t_cpp_stack
{
    unsigned update_guid;
    int      count;

    bool_t valid_idx( const signed index ) const { return index > 0 && index < _sz; }
    bool_t in_range_ref( const t_reference<_ty> & ref ) const { return ref.get_value() < _sz; }

    t_cpp_stack() { count = 0; update_guid = 0; }

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
    t_cpp_ary<_ty, _sz> lst;

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
#define _egoboo_typedef_cpp_h

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egoboo_typedef_cpp.inl"

