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

#if !defined(__cplusplus)
#    error egoboo_typedef_cpp.h should only be included if you are compling as c++
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct cpp_list_state
{
    size_t index;        ///< what is the index position in the object list?
    bool_t allocated;    ///< The object has been allocated
    bool_t in_free_list; ///< the object is currently in the free list
    bool_t in_used_list; ///< the object is currently in the used list

    cpp_list_state( size_t index = ( size_t )( ~0L ) )
    {
        cpp_list_state::ctor_this( this, index );
    }

    ~cpp_list_state()
    {
        cpp_list_state::dtor_this( this );
    }

    static cpp_list_state * ctor_this( cpp_list_state *, size_t index = ( size_t )( ~0L ) );
    static cpp_list_state * dtor_this( cpp_list_state * );

    static cpp_list_state * set_allocated( cpp_list_state *, bool_t val );
    static cpp_list_state * set_used( cpp_list_state *, bool_t val );
    static cpp_list_state * set_free( cpp_list_state *, bool_t val );

private:

    static cpp_list_state * clear( cpp_list_state *, size_t index = ( size_t )( ~0L ) );
};

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
// definition of the reference template

template <typename _ty> class t_reference
{
    protected:
        REF_T    ref;          ///< the reference
        _ty    * ptr;          ///< not yet used, needed to allow for template specilization?
        unsigned update_guid;  ///< the list guid at the time the reference was generated

    public:

        explicit t_reference( REF_T v = 0xFFFF, _ty * p = NULL ) : ref( v ), ptr( p ) {};

        t_reference<_ty> & operator = ( REF_T rhs )
        {
            ref = rhs;
            ptr = NULL;
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
template < typename _ty, size_t _sz >
struct t_cpp_map
{
private:

    unsigned                             _id;
    std::map< const REF_T, const _ty * > _map;

public:

    /// a custom iterator that tracks additions and removals to the
    /// t_cpp_map::_map
    struct iterator
    {
        friend struct t_cpp_map<_ty, _sz>;

        explicit iterator() { _id = unsigned( -1 ); _valid = bfalse; }

        typedef typename std::map< const REF_T, const _ty * >           _map_t;
        typedef typename std::map< const REF_T, const _ty * >::iterator _it_t;
        typedef typename std::pair< const REF_T, const _ty * >          _it_val_t;

        /// overload this operator to pass it on to the base iterator
        const _it_val_t& operator*() const
        {
            return _i.operator*();
        }

        /// overload this operator to pass it on to the base iterator
        const _it_val_t *operator->() const
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

    bool_t ref_validate( const t_reference<_ty> & ref )
    {
        return ref < _sz;
    }

    unsigned get_id() { return _id; }
    _ty    * get_ptr( const t_reference<_ty> & ref );
    bool_t   has_ref( const t_reference<_ty> & ref );
    bool_t   add( const t_reference<_ty> & ref, const _ty * val );
    bool_t   remove( const t_reference<_ty> & ref );

    void     clear() { _map.clear(); _id = unsigned( -1 ); };
    size_t   size()  { return _map.size(); }

    iterator iterator_begin()
    {
        return iterator( _map.begin(), _id );
    }

    bool_t iterator_end( iterator & it )
    {
        // if the iterator is not valid, we ARE at the end
        if ( !iterator_validate( it ) ) btrue;

        return it._i == _map.end();
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

/// an "interface" that should be inherited by every object that is
/// stored in t_cpp_list
struct cpp_list_client
{
    cpp_list_client( size_t index = size_t( -1 ) ) : _cpp_list_state_data( index ) {};

    cpp_list_state * get_plist() { return &_cpp_list_state_data; }
    cpp_list_state & get_list()  { return _cpp_list_state_data;  }

    const bool_t get_allocated() const { return _cpp_list_state_data.allocated; }
    const size_t get_index()     const { return _cpp_list_state_data.index;     }

    const bool_t in_free_list() const { return _cpp_list_state_data.in_free_list; }
    const bool_t in_used_list() const { return _cpp_list_state_data.in_used_list; }

protected:
    cpp_list_state _cpp_list_state_data;
};

// a simple list template that tracks free elements
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

    size_t free_full() { return.free_count() >= _sz; }
    size_t used_full() { return used_count >= _sz; }

    size_t count_free() { return.free_count(); }
    size_t count_used() { return used_count; }

    bool_t validate_ref( const t_reference<_ty> & ref ) { REF_T tmp = ref.get_value(); return tmp > 0 && tmp < _sz; };

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
    t_cpp_ary<_ty, _sz> lst;

    t_cpp_stack() { count = 0; update_guid = 0; }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a template-like declaration of a dynamically allocated array

template<typename _ty>
struct t_dary : public std::vector< _ty >
{
    size_t top;

    t_dary( size_t size = 0 )
    {
        top = 0;
        if ( 0 != size ) std::vector< _ty >::resize( size );
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

#define DECLARE_STATIC_ARY_TYPE(ARY_T, ELEM_T, SIZE) \
    struct s_STATIC_ARY_##ARY_T \
    { \
        int    count;     \
        ELEM_T ary[SIZE]; \
    }; \
    typedef struct s_STATIC_ARY_##ARY_T ARY_T##_t

#define DECLARE_EXTERN_STATIC_ARY(ARY_T, NAME) extern ARY_T##_t NAME;
#define STATIC_ARY_INIT_VALS {0}
#define INSTANTIATE_STATIC_ARY(ARY_T, NAME) ARY_T##_t NAME = STATIC_ARY_INIT_VALS;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define _egoboo_typedef_cpp_h

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "egoboo_typedef_cpp.inl"


