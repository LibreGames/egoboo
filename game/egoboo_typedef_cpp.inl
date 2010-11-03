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

/// @file egoboo_typedef_cpp.inl
/// @details cpp-only inline and template implementations

#if !defined(__cplusplus)
#    error egoboo_typedef_cpp.inl should only be included if you are compling as c++
#endif

#if !defined(_egoboo_typedef_cpp_h)
#   error this file should not be included directly. Access it through egoboo_typedef.h
#endif

//--------------------------------------------------------------------------------------------
// template t_cpp_list<>
//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
_ty * t_cpp_list< _ty, _sz >::get_ptr( const t_reference<_ty> & ref )
{
    if ( !in_range_ref( ref ) ) return NULL;

    return lst + ref;
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
size_t t_cpp_list< _ty, _sz >::get_free()
{
    /// @details ZZ@> This function returns the next free object or _sz if there are none

    size_t retval = _sz;

    if ( free_count > _sz ) free_count = _sz;
    if ( used_count > _sz ) used_count = _sz;

    while ( free_count > 0 )
    {
        free_count--;

        size_t tmp_val = free_ref[free_count];

        // completely remove it from the free list
        free_ref[free_count] = _sz;

        if ( tmp_val < _sz )
        {
            retval = tmp_val;
            update_guid++;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
egoboo_rv t_cpp_list< _ty, _sz >::add_free( const t_reference<_ty> & ref )
{
    if ( !in_range_ref( ref ) ) return rv_error;
    _ty * ptr   = lst + ref;

    if ( free_count > _sz ) free_count = _sz;
    if ( used_count > _sz ) used_count = _sz;

    //assume that the data is not corrupt, at the moment
    if ( ptr->in_free_list() ) return rv_fail;

    //EGOBOO_ASSERT( !pbase->in_free_list() );

#if EGO_DEBUG
    if ( get_free_list_index( ref ) > 0 )
    {
        return rv_error;
    }
#endif

    if ( free_full() )
    {
        shrink_free();
    }

    if ( free_full() )
    {
        compact_free();
    }

    if ( free_full() )
    {
        return rv_error;
    }

    free_ref[free_count] = ref.get_value();

    free_count++;
    update_guid++;

    ego_obj_lst_state * plst = ptr->get_plist();

    ego_obj_lst_state::set_free( plst, btrue );
    ego_obj_lst_state::set_used( plst, bfalse );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
int t_cpp_list< _ty, _sz >::get_free_list_index( const t_reference<_ty> & ref )
{
    int    retval = -1;
    size_t cnt;
    _ty  * ptr;

    if ( !in_range_ref( ref ) ) return retval;
    ptr   = lst + ref;

    // shorten the list to remove any bad values on the top
    shrink_free();

    for ( cnt = 0; cnt < free_count; cnt++ )
    {
        if ( ref == free_ref[cnt] )
        {
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
bool_t t_cpp_list< _ty, _sz >::remove_free_index( int idx )
{
    t_reference<_ty> ref;

    if ( free_count > _sz ) free_count = _sz;
    if ( used_count > _sz ) used_count = _sz;

    // was it found?
    if ( idx < 0 || ( size_t )idx >= free_count ) return bfalse;

    // make sure that there is a valid value on the top of the list
    shrink_free();

    // if the list is empty, there's nothing to do
    if ( 0 == free_count ) return bfalse;

    // grab the reference value to the element
    ref = free_ref[idx];

    // blank out the idx in the list
    free_ref[idx] = _sz;

    // let the object know it is not in the list anymore
    if ( in_range_ref( ref ) )
    {
        ego_obj_lst_state::set_free( lst[ref].get_plist(), bfalse );
    }

    // shorten the list
    free_count--;

    // swap the blank element with the highest
    if (( size_t )idx < free_count )
    {
        // swap the last element for the deleted element
        //SWAP( size_t, free_ref[idx], free_ref[free_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
bool_t    t_cpp_list< _ty, _sz >::remove_free( const t_reference<_ty> & ref )
{
    // find the object in the free list
    int index = get_free_list_index( ref );

    return remove_free_index( index );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
egoboo_rv t_cpp_list< _ty, _sz >::add_used( const t_reference<_ty> & ref )
{
    egoboo_rv retval;
    _ty * ptr;

    if ( !in_range_ref( ref ) ) return rv_error;
    ptr  = lst + ref;

    if ( free_count > _sz ) free_count = _sz;
    if ( used_count > _sz ) used_count = _sz;

    //assume that the data is not corrupt, at the moment
    if ( ptr->in_used_list() ) return rv_fail;

    //EGOBOO_ASSERT( !ptr->in_used_list() );

#if EGO_DEBUG
    if ( get_used_list_index( ref ) > 0 )
    {
        return rv_error;
    }
#endif

    if ( used_full() )
    {
        shrink_used();
    }

    if ( used_full() )
    {
        compact_used();
    }

    if ( used_full() )
    {
        return rv_error;
    }

    retval = rv_fail;

    used_ref[used_count] = ref.get_value();

    used_count++;
    update_guid++;

    ego_obj_lst_state * plst = ptr->get_plist();

    ego_obj_lst_state::set_free( plst, bfalse );
    ego_obj_lst_state::set_used( plst, btrue );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
int t_cpp_list< _ty, _sz >::get_used_list_index( const t_reference<_ty> & ref )
{
    int    retval = -1;
    size_t cnt;
    _ty * ptr;

    if ( !in_range_ref( ref ) ) return retval;
    ptr = lst + ref;

    // shorten the list to remove any bad values on the top
    shrink_used();

    for ( cnt = 0; cnt < used_count; cnt++ )
    {
        if ( ref == used_ref[cnt] )
        {
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
bool_t t_cpp_list< _ty, _sz >::remove_used_index( int index )
{
    t_reference<_ty> ref;

    if ( free_count > _sz ) free_count = _sz;
    if ( used_count > _sz ) used_count = _sz;

    // was it found?
    if ( index < 0 || ( size_t )index >= used_count ) return bfalse;

    // make sure that there is a valid value on the top of the list
    shrink_used();

    // if the list is empty, there's nothing to do
    if ( 0 == used_count ) return bfalse;

    // grab the reference value to the element
    ref = used_ref[index];

    // blank out the index in the list
    used_ref[index] = _sz;

    // let the object know it is not in the list anymore
    if ( in_range_ref( ref ) )
    {
        ego_obj_lst_state::set_used( lst[ref].get_plist(), bfalse );
    }

    // shorten the list
    used_count--;

    // swap the blank element with the highest
    if (( size_t )index < used_count )
    {
        // swap the last element for the deleted element
        //SWAP( size_t, used_ref[index], used_ref[used_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
bool_t    t_cpp_list< _ty, _sz >::remove_used( const t_reference<_ty> & ref )
{
    // find the object in the used list
    int index = get_used_list_index( ref );

    return remove_used_index( index );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
egoboo_rv t_cpp_list< _ty, _sz >::free_one( const t_reference<_ty> & ref )
{
    egoboo_rv retval = rv_error;
    _ty     * ptr;

    if ( !in_range_ref( ref ) ) return rv_error;
    ptr  = lst + ref;

    if ( ptr->in_used_list() )
    {
        remove_used( ref );
    }

    if ( ptr->in_free_list() )
    {
        retval = rv_success;
    }
    else
    {
        retval = add_free( ref );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_cpp_list< _ty, _sz >::shrink_used()
{
    t_reference<_ty> ref;

    if ( free_count > _sz ) free_count = _sz;
    if ( used_count > _sz ) used_count = _sz;

    while ( used_count > 0 )
    {
        size_t index = used_count - 1;

        if ( used_ref[index] < _sz ) break;

        used_ref[index] = _sz;

        used_count = index;
    }
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_cpp_list< _ty, _sz >::shrink_free()
{
    t_reference<_ty> ref;

    if ( free_count > _sz ) free_count = _sz;
    if ( used_count > _sz ) used_count = _sz;

    while ( free_count > 0 )
    {
        size_t index = free_count - 1;

        if ( free_ref[index] < _sz ) break;

        free_ref[index] = _sz;

        free_count = index;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_cpp_list< _ty, _sz >::compact_used()
{
    t_reference<_ty> ref;
    size_t           src, dst;

    if ( free_count > _sz ) free_count = _sz;
    if ( used_count > _sz ) used_count = _sz;

    for ( src = 0, dst = 0; dst < used_count; src++ )
    {
        ref = used_ref[src];
        used_ref[src] = _sz;

        if ( ref < _sz )
        {
            used_ref[dst] = ref.get_value();
            dst++;
        }
    }

    used_count = dst;
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_cpp_list< _ty, _sz >::compact_free()
{
    t_reference<_ty> ref;
    size_t           src, dst;

    if ( free_count > _sz ) free_count = _sz;
    if ( used_count > _sz ) used_count = _sz;

    for ( src = 0, dst = 0; dst < free_count; src++ )
    {
        ref = free_ref[src];
        free_ref[src] = _sz;

        if ( ref < _sz )
        {
            free_ref[dst] = ref.get_value();
            dst++;
        }
    }

    free_count = dst;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
_ty * t_cpp_map<_ty, _ity>::get_ptr( const t_reference<_ty> & ref )
{
    if ( !ref_validate( ref ) ) return NULL;

    t_cpp_map<_ty, _ity>::map_iterator it = _map.find( ref.get_value() );
    if ( it == _map.end() ) return NULL;

    return it->second;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
bool_t t_cpp_map<_ty, _ity>::has_ref( const t_reference<_ty> & ref )
{
    return _map.end() != _map.find( ref.get_value() );
}

//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
bool_t t_cpp_map<_ty, _ity>::add( const t_reference<_ty> & ref, const _ty * val )
{
    // is it a valid pointer?
    if ( NULL == val )
    {
        remove( ref );
        return bfalse;
    }
    else
    {
#if defined DEBUG_CPP_LISTS
        // is the slot already occupied?
        if ( has_ref( ref ) ) return bfalse;
#endif

        // add the value
        _map[ ref.get_value()] = val;
        increment_id();
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
bool_t t_cpp_map<_ty, _ity>::remove( const t_reference<_ty> & ref )
{
    // add the value
    bool_t rv = bfalse;
    if ( 0 != _map.erase( ref.get_value() ) )
    {
        increment_id();
        rv = btrue;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
typename t_cpp_deque<_ty, _ity>::deque_iterator t_cpp_deque<_ty, _ity>::find_ref( const t_reference<_ty> & ref )
{
    const _ity ref_val = ref.get_value();

    if ( _deque.empty() ) return _deque.end();

    for ( deque_iterator it = _deque.begin(); it != _deque.end(); it++ )
    {
        if ( *it == ref_val )
        {
            break;
        }
    }

    return it;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
bool_t t_cpp_deque<_ty, _ity>::has_ref( const t_reference<_ty> & ref )
{
    return find_ref( ref ) != _deque.end();
}

//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
bool_t t_cpp_deque<_ty, _ity>::add( const t_reference<_ty> & ref )
{
    // is it in the valid range?
    if ( has_ref( ref ) ) return bfalse;

    // add the value
    _deque.push_back( ref.get_value() );
    increment_id();

    return btrue;
}

//--------------------------------------------------------------------------------------------
template < typename _ty, typename _ity >
bool_t t_cpp_deque<_ty, _ity>::remove( const t_reference<_ty> & ref )
{
    deque_iterator it = find_ref( ref );

    bool_t rv = bfalse;
    if ( it != _deque.end() )
    {
        _deque.erase( it );
        increment_id();
        rv = btrue;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template<typename _ty>
bool_t   t_dary<_ty>::alloc( t_dary<_ty> * pary, size_t sz )
{
    if ( NULL == pary ) return bfalse;

    if ( sz > 0 )
    {
        pary->resize( sz );
    }
    else
    {
        dealloc( pary );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
bool_t t_dary<_ty>::dealloc( t_dary<_ty> * pary )
{
    if ( NULL == pary ) return bfalse;

    //pary->std::vector<_ty>::clear();
    pary->std::vector<_ty>::resize( 0 );
    pary->top = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
void     t_dary<_ty>::clear( t_dary<_ty> * pary )
{
    if ( NULL != pary ) pary->top = 0;
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
size_t   t_dary<_ty>::get_top( t_dary<_ty> * pary )
{
    if ( NULL == pary ) return 0;

    pary->top = std::min( pary->top, pary->size() );

    return pary->top;
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
size_t   t_dary<_ty>::get_size( t_dary<_ty> * pary )
{
    return ( NULL == pary ) ? 0 : pary->size();
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
_ty *    t_dary<_ty>::pop_back( t_dary<_ty> * pary )
{
    if ( NULL == pary || 0 == pary->top ) return NULL;
    --pary->top;
    return &( pary->operator []( pary->top ) );
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
bool_t   t_dary<_ty>::push_back( t_dary<_ty> * pary, _ty val )
{
    bool_t retval = bfalse;

    if ( NULL == pary ) return bfalse;

    if ( pary->top < pary->size() )
    {
        pary->operator []( pary->top ) = val;
        pary->top++;
        retval = btrue;
    }

    return retval;
}
