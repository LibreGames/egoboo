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

#include <exception>

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
        cpp_list_state::ctor( this, index );
    }

    ~cpp_list_state()
    {
        cpp_list_state::dtor( this );
    }

    static cpp_list_state * ctor( cpp_list_state *, size_t index = ( size_t )( ~0L ) );
    static cpp_list_state * dtor( cpp_list_state * );
    static cpp_list_state * clear( cpp_list_state *, size_t index = ( size_t )( ~0L ) );

    static cpp_list_state * set_allocated( cpp_list_state *, bool_t val );
    static cpp_list_state * set_used( cpp_list_state *, bool_t val );
    static cpp_list_state * set_free( cpp_list_state *, bool_t val );
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

#define CPP_DECLARE_REF( TYPE, NAME ) typedef t_reference<TYPE> NAME

#define CPP_REF_TO_INT(X) ((X).get_value())

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

#define CPP_DECLARE_T_ARY(TYPE, NAME, COUNT) t_cpp_ary<TYPE, COUNT> NAME

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// an "interface" that should be inherited by every object that is
/// stored in t_cpp_list
struct cpp_list_client
{
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
    size_t   used_count;
    size_t   free_count;
    size_t   used_ref[_sz];
    size_t   free_ref[_sz];

    CPP_DECLARE_T_ARY( _ty, lst, _sz );

    t_cpp_list() { used_count = free_count = 0; update_guid = 0; }

    _ty * get_ptr( const t_reference<_ty> & ref );

    size_t           count_free() { return free_count; }
    size_t           count_used() { return used_count; }

    bool_t validate_ref( const t_reference<_ty> & ref ) { REF_T tmp = ref.get_value(); return tmp > 0 && tmp < _sz; };

    egoboo_rv add_free( const t_reference<_ty> & ref );
    egoboo_rv add_used( const t_reference<_ty> & ref );

    bool_t    remove_free( const t_reference<_ty> & ref );
    bool_t    remove_used( const t_reference<_ty> & ref );

    egoboo_rv free_one( const t_reference<_ty> & ichr );
    egoboo_rv get_free( size_t index = ( ~0 ) );

protected:
    int get_used_list_index( const t_reference<_ty> & ref );
    int get_free_list_index( const t_reference<_ty> & ref );

    bool_t remove_free_index( int index );
    bool_t remove_used_index( int index );
};

#define CPP_DECLARE_LIST_EXTERN(TYPE, NAME, COUNT)    extern t_cpp_list<TYPE, COUNT> NAME
#define CPP_INSTANTIATE_LIST_STATIC(TYPE,NAME, COUNT) static t_cpp_list<TYPE, COUNT> NAME
#define CPP_INSTANTIATE_LIST(ACCESS,TYPE,NAME, COUNT) ACCESS t_cpp_list<TYPE, COUNT> NAME

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple stack template

template < typename _ty, size_t _sz >
struct t_cpp_stack
{
    unsigned update_guid;
    int      count;
    CPP_DECLARE_T_ARY( _ty, lst, _sz );

    t_cpp_stack() { count = 0; update_guid = 0; }
};

#define CPP_DECLARE_STACK_EXTERN(TYPE, NAME, COUNT)      extern t_cpp_stack<TYPE, COUNT> NAME
#define CPP_INSTANTIATE_STACK_STATIC(TYPE, NAME, COUNT)  static t_cpp_stack<TYPE, COUNT> NAME
#define CPP_INSTANTIATE_STACK(ACCESS, TYPE, NAME, COUNT) ACCESS t_cpp_stack<TYPE, COUNT> NAME

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define _egoboo_typedef_cpp_h

