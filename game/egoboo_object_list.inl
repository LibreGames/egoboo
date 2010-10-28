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

/// @file egoboo_object_list.inl
/// @brief Implementation of the t_ego_obj_lst< _ty, _sz >::* functions
/// @details

#if !defined(__cplusplus)
#    error egoboo_typedef_cpp.inl should only be included if you are compling as c++
#endif

#if !defined(_egoboo_object_list_h)
#   error this file should not be included directly. Access it through egoboo_object_list.h
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "log.h"

//--------------------------------------------------------------------------------------------
// grab a container
//--------------------------------------------------------------------------------------------
template < typename _d, size_t _sz >
INLINE t_ego_obj_container<_d, _sz> & t_ego_obj_lst<_d, _sz>::get_ref( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    if ( !in_range_ref( ref ) )
    {
        // there is no coming back from this error
        EGOBOO_ASSERT( bfalse );
    }

    return ary[ref.get_value()];
}

//--------------------------------------------------------------------------------------------
template < typename _d, size_t _sz >
INLINE t_ego_obj_container<_d, _sz> * t_ego_obj_lst<_d, _sz>::get_ptr( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    if ( !in_range_ref( ref ) ) return NULL;

    return ary + ref.get_value();
}

//--------------------------------------------------------------------------------------------
template < typename _d, size_t _sz >
INLINE t_ego_obj_container<_d, _sz> * t_ego_obj_lst<_d, _sz>::get_allocated_ptr( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    t_ego_obj_container<_d, _sz> * pobj = get_ptr( ref );
    if ( NULL == pobj ) return NULL;

    return !ego_obj_lst_state::get_allocated( pobj ) ? NULL : pobj;
}

//--------------------------------------------------------------------------------------------
// grab a container's data
//--------------------------------------------------------------------------------------------
template < typename _d, size_t _sz >
INLINE typename t_ego_obj_container<_d, _sz>::data_type & t_ego_obj_lst<_d, _sz>::get_data_ref( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    return container_type::get_data_ref( get_ref( ref ) );
}

//--------------------------------------------------------------------------------------------
template < typename _d, size_t _sz >
INLINE typename t_ego_obj_container<_d, _sz>::data_type * t_ego_obj_lst<_d, _sz>::get_data_ptr( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    container_type * ptr = get_ptr( ref );

    if ( NULL == ptr ) return NULL;

    return container_type::get_data_ptr( ptr );
}

//--------------------------------------------------------------------------------------------
template < typename _d, size_t _sz >
INLINE typename t_ego_obj_container<_d, _sz>::data_type * t_ego_obj_lst<_d, _sz>::get_allocated_data_ptr( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    container_type * ptr = get_allocated_ptr( ref );
    if ( NULL == ptr ) return NULL;

    return container_type::get_data_ptr( ptr );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
void t_ego_obj_lst<_d, _sz>::clear_free_list()
{
    while ( !free_queue.empty() ) free_queue.pop();
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
void t_ego_obj_lst<_d, _sz>::reinit()
{
    // reinitialize the free list so that no more objects beyond _max_len
    // will be allocated

    // clear the free list, but do nothing to the used list
    clear_free_list();

    for ( size_t cnt = 0; cnt < _max_len; cnt++ )
    {
        if ( !container_type::get_allocated( ary + cnt ) )
        {
            free_queue.push( cnt );
        }
    }
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
void t_ego_obj_lst<_d, _sz>::init()
{
    clear_free_list();
    used_map.clear();

    for ( size_t cnt = 0; cnt < _max_len; cnt++ )
    {
        container_type * pobj = ary + cnt;

        // create/re-create the link to this list
        ego_obj_lst_state::retor_this( pobj, cnt );

        free_queue.push( cnt );
    }
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
void t_ego_obj_lst<_d, _sz>::deinit()
{
    size_t cnt;

    used_map.clear();
    clear_free_list();

    for ( cnt = 0; cnt < _sz; cnt++ )
    {
        _d * pobj = get_data_ptr( lst_reference( cnt ) );
        if ( NULL == pobj ) continue;

        ego_object_engine::run_deconstruct( pobj, 100 );
    }
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
bool_t t_ego_obj_lst<_d, _sz>::free_raw( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    // grab a pointer to the container, if the reference is in a valid range
    container_type * pcont = get_ptr( ref );
    if ( NULL == pcont ) return bfalse;

    // move the element from the used_map...
    used_map.remove( ref );
    ego_obj_lst_state::set_used( pcont, bfalse );

    // ...to the free_queue
    if ( ref.get_value() < _max_len )
    {
        // but only if the index is less than out current limit
        free_queue.push( ref.get_value() );
    }
    ego_obj_lst_state::set_free( pcont, btrue );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
void t_ego_obj_lst<_d, _sz>::update_used()
{
    lst_reference ref;
    std::stack< REF_T > tmp_stack;

    // iterate through the used_map do determine if any of them need to be removed
    // DO NOT delete an element from the list while you're looping through it
    for ( map_iterator it = used_map.iterator_begin(); !used_map.iterator_end( it ); used_map.iterator_increment( it ) )
    {
        const container_type * pcont = it->second;

        if ( container_type::get_allocated( pcont ) ) continue;

        tmp_stack.push( it->first );
    }

    // now iterate through our stack and remove the elements
    while ( !tmp_stack.empty() )
    {
        free_raw( lst_reference( tmp_stack.top() ) );
        tmp_stack.pop();
    }

#if defined(DEBUG_LIST) && EGO_DEBUG
    // go through the object array and see if (God forbid) there is an
    // object that is marked as allocated, but is not in the used map
    for ( size_t cnt = 0; cnt < _max_len; cnt++ )
    {
        container_type * pcont = ary + cnt;

        if ( !container_type::get_allocated( pcont ) ) continue;

        if ( !ego_obj_lst_state::in_used( pcont ) ) continue;

        // add it to the used map
        used_map.add( ref, pcont );

        // tell the object that it is definitely in the used list
        ego_obj_lst_state::set_used( pcont, btrue );
        ego_obj_lst_state::set_free( pcont, bfalse );
    }
#endif
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
egoboo_rv t_ego_obj_lst<_d, _sz>::free_one( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    /// @details ZZ@> This function sticks a object back on the free object stack
    ///
    /// @note Tying ALLOCATED_*() and POBJ_TERMINATE() to t_ego_obj_lst<>::free_one()
    /// should be enough to ensure that no object is freed more than once

    egoboo_rv retval = rv_fail;

    container_type * pcont = ary + ref.get_value();
    if ( !container_type::get_allocated( pcont ) ) return rv_fail;

    _d * pdata = container_type::get_data_ptr( pcont );
    if ( NULL == pdata ) return rv_error;

    // if we are inside a t_ego_obj_lst<> loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( loop_depth > 0 )
    {
        retval = add_termination( ref );
    }
    else
    {
        // deactivate the list element
        deactivate_element( ref );

        // return the object reference
        free_raw( ref );

    }

    return retval;
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
typename t_ego_obj_lst<_d, _sz>::lst_reference t_ego_obj_lst<_d, _sz>::get_free()
{
    /// @details BB@> This function returns a lst_reference to the next free object.
    ///               On failure, the lst_reference will be blank/invalid.

    lst_reference retval( _sz );

    while ( !free_queue.empty() )
    {
        // pop the top value off the stack
        retval = free_queue.front();
        free_queue.pop();

        // have we found a valid lst_reference?
        if ( in_range_ref( retval ) && !used_map.has_ref( retval ) ) break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
void t_ego_obj_lst<_d, _sz>::free_all()
{
    typename t_ego_obj_lst<_d, _sz>::lst_reference cnt;

    for ( cnt = 0; cnt < _sz; cnt++ )
    {
        free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
typename t_ego_obj_lst<_d, _sz>::lst_reference t_ego_obj_lst<_d, _sz>::allocate( const typename t_ego_obj_lst<_d, _sz>::lst_reference & override )
{
    lst_reference ref( _sz );

    if ( !in_range_ref( override ) )
    {
        // override is not a valid lst_reference, so just get the next free value
        ref = get_free();
    }
    else if ( !used_map.has_ref( override ) )
    {
        // override is a valid lst_reference and it does not already exist
        ref = override;
    }

    if ( !in_range_ref( ref ) )
    {
        // we didn't get a valid lst_reference
        log_warning( "t_ego_obj_lst<>::allocate() - failed to override a object? Object at index %d already spawned? \n", ( override ).get_value() );
    }
    else
    {
        container_type * pobj = ary + ref.get_value();

        // the lst_reference is valid, so activate the object
        ref = activate_element( ref );

        if ( in_range_ref( ref ) )
        {
            // add the value to the used map
            used_map.add( ref, pobj );

            // tell the object that it is definitely in the used list
            ego_obj_lst_state::set_used( pobj, btrue );
            ego_obj_lst_state::set_free( pobj, bfalse );
        }
    }

    return ref;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
void t_ego_obj_lst<_d, _sz>::cleanup()
{
    // go through the list and activate all the characters that
    // were created while the list was iterating
    while ( !activation_stack.empty() )
    {
        lst_reference ref( activation_stack.top() );
        activation_stack.pop();

        _d * pobj = get_allocated_data_ptr( ref );
        if ( NULL != pobj )
        {
            ego_obj::grant_on( ego_obj::get_ego_obj_ptr( pobj ) );
        }
    }

    // go through and delete any characters that were
    // supposed to be deleted while the list was iterating
    while ( !termination_stack.empty() )
    {
        lst_reference ref( termination_stack.top() );
        termination_stack.pop();

        free_one( ref );
    }
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
egoboo_rv t_ego_obj_lst<_d, _sz>::add_activation( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    // put this object into the activation list so that it can be activated right after
    // the t_ego_obj_lst<> loop is completed

    if ( !in_range_ref( ref ) ) return rv_error;

    activation_stack.push( ref.get_value() );

    ego_obj * pdata = get_data_ptr( ref );
    if ( NULL != pdata )
    {
        pdata->proc_req_on( btrue );
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
egoboo_rv t_ego_obj_lst<_d, _sz>::add_termination( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    egoboo_rv retval = rv_fail;

    if ( !in_range_ref( ref ) ) return rv_fail;

    termination_stack.push( ref.get_value() );

    _d * pobj = get_data_ptr( ref );
    if ( NULL != pobj )
    {
        // at least mark the object as "waiting to be terminated"
        ego_obj::req_terminate( pobj );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
template <typename _data, size_t _sz>
bool_t t_ego_obj_lst<_data, _sz>::set_length( size_t len )
{
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

//--------------------------------------------------------------------------------------------
// t_ego_obj_container<> -
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// struct ego_obj_chr - memory management
//--------------------------------------------------------------------------------------------
template< typename _d, size_t _sz >
t_ego_obj_container<_d, _sz> * t_ego_obj_container<_d, _sz>::ctor_this( t_ego_obj_container<_d, _sz> * pobj )
{
    // construct this struct, ONLY

    if ( NULL == pobj ) return NULL;

    /* puts( "\t\t" __FUNCTION__ ); */

    //pobj = dealloc( pobj );
    // pobj = alloc( pobj );

    return pobj;
}

//--------------------------------------------------------------------------------------------
template< typename _d, size_t _sz >
t_ego_obj_container<_d, _sz> * t_ego_obj_container<_d, _sz>::dtor_this( t_ego_obj_container<_d, _sz> * pcont )
{
    // destruct this struct, ONLY

    if ( NULL == pcont || !get_allocated( pcont ) ) return pcont;

    //data_type * pobj = get_data_ptr(pcont);

    /* puts( "\t\t" __FUNCTION__ ); */

    //pcont = dealloc( pcont );

    return pcont;
}

////--------------------------------------------------------------------------------------------
//template< typename _d, size_t _sz >
//bool_t t_ego_obj_container<_d,_sz>::request_terminate( t_ego_obj_container<_d,_sz> * pcont )
//{
//    /// @details BB@> Mark this data for deletion
//
//    if ( !ALLOCATED_PBASE( pcont ) ) return bfalse;
//
//    POBJ_REQUEST_TERMINATE( pcont->get_data_ptr() );
//
//    return btrue;
//}

////--------------------------------------------------------------------------------------------
//template< typename _d, size_t _sz >
//bool_t t_ego_obj_container<_d,_sz>::request_terminate( _d * pdata )
//{
//    /// @details BB@> Mark this data for deletion
//
//    if ( !DEFINED_PBASE( _d::get_obj_ptr(pdata)  ) ) return bfalse;
//
//    POBJ_REQUEST_TERMINATE( _d::get_obj_ptr(pdata)  );
//
//    return btrue;
//}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
typename t_ego_obj_lst<_d, _sz>::lst_reference t_ego_obj_lst<_d, _sz>::activate_element( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{
    bool_t needs_free = bfalse;

    container_type * pcont = get_ptr( ref );
    if ( NULL == pcont ) return lst_reference( _sz );

    // if it is already allocated, there we need to get rid of the old data
    if ( container_type::get_allocated( pcont ) )
    {
        needs_free = btrue;
    }

    _d * pdata = container_type::get_data_ptr( pcont );

    // if it is already initialized, we need to deinitialize it
    if ( _d::get_valid( pdata ) )
    {
        needs_free = btrue;
    }

    // if the object is already being used, make sure to destroy the old one
    if ( needs_free )
    {
        free_one( ref );
        needs_free = bfalse;
    }

    // handle the actual activation(s)
    pcont = activate_container_raw( pcont, ref.get_value() );
    pdata = activate_data_raw( pdata, pcont );

    return ref;
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
typename t_ego_obj_lst<_d, _sz>::lst_reference t_ego_obj_lst<_d, _sz>::deactivate_element( const typename t_ego_obj_lst<_d, _sz>::lst_reference & ref )
{

    container_type * pcont = get_ptr( ref );
    if ( NULL == pcont ) return lst_reference( _sz );

    _d * pdata = container_type::get_data_ptr( pcont );
    if ( NULL == pcont ) return lst_reference( _sz );

    // "free" the container
    pcont = deactivate_container_raw( pcont );

    // "free" the data
    pdata = deactivate_data_raw( pdata );

    return ref;
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
_d * t_ego_obj_lst<_d, _sz>::activate_data_raw( _d * pdata, const typename t_ego_obj_lst<_d, _sz>::container_type * pcont )
{
    if ( NULL == pdata ) return pdata;

    // use the placement new operator to make the object construct itself
    pdata = new( pdata ) _d( pcont );

    // tell the object process that it is valid
    ego_object_process::set_valid( pdata, btrue );

    // construct the object
    ego_object_engine::run_construct( pdata, 100 );

    return pdata;
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
typename t_ego_obj_lst<_d, _sz>::container_type * t_ego_obj_lst<_d, _sz>::activate_container_raw( typename t_ego_obj_lst<_d, _sz>::container_type * pcont, size_t index )
{
    if ( NULL == pcont ) return pcont;

    // reset the container's list state to match the given value
    ego_obj_lst_state::retor_this( pcont, index );

    // tell the state that it is allocated
    ego_obj_lst_state::set_allocated( pcont, btrue );

    return pcont;
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
typename t_ego_obj_lst<_d, _sz>::container_type * t_ego_obj_lst<_d, _sz>::deactivate_container_raw( typename t_ego_obj_lst<_d, _sz>::container_type * pcont )
{
    if ( NULL == pcont ) return pcont;

    // turn off the allocation bit
    ego_obj_lst_state::set_allocated( pcont, bfalse );

    return pcont;
}

//--------------------------------------------------------------------------------------------
template <typename _d, size_t _sz>
_d * t_ego_obj_lst<_d, _sz>::deactivate_data_raw( _d * pdata )
{
    if ( NULL == pdata ) return pdata;

    // deallocate any dynamically allocated memory
    if ( NULL == ego_object_engine::run_deconstruct( pdata, 100 ) ) return pdata;

    // let the object process know that it is invalidated
    pdata->end_invalidating();

    // call the destructor directly
    pdata->~_d();

    return pdata;
}

