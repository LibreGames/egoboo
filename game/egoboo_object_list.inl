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

#include "egoboo_object_list.h"

#include "log.h"
#include "egoboo_object.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
INLINE _ty & t_ego_obj_lst< _ty, _sz >::get_obj( const t_reference<_ty> & ref )
{
    if ( !validate_ref( ref ) )
    {
        // there is no coming back from this error
        EGOBOO_ASSERT( bfalse );
    }

    return ary[ref.get_value()];
}

//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
INLINE _ty * t_ego_obj_lst< _ty, _sz >::get_ptr( const t_reference<_ty> & ref )
{
    if ( !validate_ref( ref ) ) return NULL;

    return ary + ref.get_value();
}

//--------------------------------------------------------------------------------------------
template < typename _ty, size_t _sz >
INLINE _ty * t_ego_obj_lst< _ty, _sz >::get_valid_ptr( const t_reference<_ty> & ref )
{
    _ty * pobj = get_ptr( ref );
    if ( NULL == pobj ) return NULL;

    return !VALID_PBASE( pobj->get_pego_obj() ) ? NULL : pobj;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_ego_obj_lst< _ty, _sz >::clear_free_list()
{
    while ( !free_queue.empty() ) free_queue.pop();
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_ego_obj_lst< _ty, _sz >::init()
{
    int cnt;

    clear_free_list();
    used_map.clear();

    for ( cnt = 0; cnt < _sz; cnt++ )
    {
        _ty * pobj = ary + cnt;

        // destroy all dynamic data
        ego_obj::dtor_all( POBJ_GET_PBASE( pobj ) );

        // re-create all objects
        ego_obj::ctor_all( POBJ_GET_PBASE( pobj ), cnt );

        free_queue.push( cnt );
    }
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_ego_obj_lst< _ty, _sz >::dtor_this()
{
    reference ref;
    size_t           cnt;

    used_map.clear();
    clear_free_list();

    for ( cnt = 0; cnt < _sz; cnt++ )
    {
        _ty::run_deconstruct( ary + cnt, 100 );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_ego_obj_lst< _ty, _sz >::update_used()
{
    reference ref;
    std::stack< REF_T > tmp_stack;

    // iterate through the used_map do determine if any of them need to be removed
    // DO NOT delete an element from the list while you're looping through it
    for ( iterator it = used_map.iterator_begin(); !used_map.iterator_end( it ); used_map.iterator_increment( it ) )
    {
        const _ty * ptr = it->second;
        if ( NULL == ptr )
        {
            tmp_stack.push( it->first );
        }
        else
        {
            const ego_obj * pbase = ptr->cget_pego_obj();
            if ( NULL == ptr || !FLAG_ALLOCATED_PBASE( pbase ) )
            {
                tmp_stack.push( it->first );
            }
        }
    }

    // now iterate through our stack and remove the elements
    while ( !tmp_stack.empty() )
    {
        // grab the top element
        const REF_T idx = tmp_stack.top();
        tmp_stack.pop();

        // convert this back into a reference
        reference tmp_ref( idx );

        // grab some pointers
        _ty * ptr = get_ptr( tmp_ref );
        if ( NULL == ptr ) continue;

        ego_obj * pbase = ptr->get_pego_obj();
        if ( NULL == pbase ) continue;

        // move the element from the used_map...
        used_map.remove( tmp_ref );
        cpp_list_state::set_used( pbase->get_plist(), bfalse );

        // ...to the free_queue
        free_queue.push( tmp_ref.get_value() );
        cpp_list_state::set_free( pbase->get_plist(), btrue );
    }

    // go through the object array and see if (God forbid) there is an
    // object that is marked as allocated, but is not in the used map
    for ( size_t cnt = 0; cnt < _sz; cnt++ )
    {
        _ty     * ptr = ary + cnt;

        ego_obj * pbase = ptr->get_pego_obj();
        if ( NULL == pbase ) continue;

        if ( FLAG_ALLOCATED_PBASE( pbase ) && !pbase->in_used_list() )
        {
            used_map.add( ref, ptr );

            // tell the object that it is definitely in the used list
            cpp_list_state::set_used( pbase->get_plist(), btrue );
            cpp_list_state::set_free( pbase->get_plist(), bfalse );
        }
    }
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
egoboo_rv t_ego_obj_lst< _ty, _sz >::free_one( const t_reference<_ty> & ref )
{
    /// @details ZZ@> This function sticks a object back on the free object stack
    ///
    /// @note Tying ALLOCATED_*() and POBJ_TERMINATE() to t_ego_obj_lst<>::free_one()
    /// should be enough to ensure that no object is freed more than once

    egoboo_rv retval = rv_fail;
    _ty * pobj;
    ego_obj * pbase;

    pobj = ary + ref.get_value();
    if ( NULL == pobj )  return rv_error;

    pbase = POBJ_GET_PBASE( pobj );
    if ( NULL == pbase ) return rv_error;

    if ( !ALLOCATED_PBASE( pbase ) ) return rv_fail;

    // if we are inside a t_ego_obj_lst<> loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( t_ego_obj_lst< _ty, _sz >::loop_depth > 0 )
    {
        retval = t_ego_obj_lst< _ty, _sz >::add_termination( ref );
    }
    else
    {
        // deallocate any dynamically allocated memory
        pobj = _ty::run_deconstruct( pobj, 100 );
        if ( NULL == pobj ) return rv_error;

        // we are now done killing the object
        ego_obj::end_invalidating( pbase );

        // remove it from the used map
        used_map.remove( ref );
        cpp_list_state::set_used( pobj->get_plist(), bfalse );

        // add it to the free stack
        free_queue.push( ref.get_value() );
        cpp_list_state::set_free( pobj->get_plist(), btrue );

        // no longer allocated
        cpp_list_state::set_allocated( pobj->get_plist(), bfalse );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
t_reference<_ty> t_ego_obj_lst< _ty, _sz >::get_free()
{
    /// @details BB@> This function returns a reference to the next free object.
    ///               On failure, the reference will be blank/invalid.

    reference retval( _sz );

    while ( !free_queue.empty() )
    {
        // pop the top value off the stack
        retval = free_queue.front();
        free_queue.pop();

        // have we found a valid reference?
        if ( validate_ref( retval ) && !used_map.has_ref( retval ) ) break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_ego_obj_lst< _ty, _sz >::free_all()
{
    t_reference<_ty> cnt;

    for ( cnt = 0; cnt < _sz; cnt++ )
    {
        t_ego_obj_lst< _ty, _sz >::free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
t_reference<_ty> t_ego_obj_lst< _ty, _sz >::activate_object( const t_reference<_ty> & ref )
{
    if ( !validate_ref( ref ) ) return ref;

    _ty * pobj = ary + ref.get_value();
    ego_obj * pbase = POBJ_GET_PBASE( pobj );

    // if the object is already being used, make sure to destroy the old one
    if ( FLAG_ALLOCATED_PBASE( pbase ) && FLAG_VALID_PBASE( pbase ) )
    {
        t_ego_obj_lst< _ty, _sz >::free_one( ref );
    }

    // allocate the new one
    ego_obj::allocate( pbase, ( ref ).get_value() );

    _ty::run_construct( pobj, 100 );

    return ref;
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
t_reference<_ty> t_ego_obj_lst< _ty, _sz >::allocate( const t_reference<_ty> & override )
{
    reference ref( _sz );

    if ( !validate_ref( override ) )
    {
        // override is not a valid reference, so just get the next free value
        ref = get_free();
    }
    else if ( !used_map.has_ref( override ) )
    {
        // override is a valid reference and it does not already exist
        ref = override;
    }

    if ( !validate_ref( ref ) )
    {
        // we didn't get a valid reference
        log_warning( "t_ego_obj_lst<>::allocate() - failed to override a object? Object at index %d already spawned? \n", ( override ).get_value() );
    }
    else
    {
        // the reference is valid, so activate the object
        ref = t_ego_obj_lst< _ty, _sz >::activate_object( ref );

        _ty * pobj = ary + ref.get_value();

        // add the value to the used map
        used_map.add( ref, pobj );

        // tell the object that it is definitely in the used list
        cpp_list_state::set_used( pobj->get_plist(), btrue );
        cpp_list_state::set_free( pobj->get_plist(), bfalse );
    }

    return ref;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
void t_ego_obj_lst< _ty, _sz >::cleanup()
{
    // go through the list and activate all the characters that
    // were created while the list was iterating
    while ( !activation_stack.empty() )
    {
        reference ref( activation_stack.top() );
        activation_stack.pop();

        _ty * pobj = get_valid_ptr( ref );
        if ( NULL != pobj )
        {
            ego_obj::grant_on( pobj->get_pego_obj() );
        }
    }

    // go through and delete any characters that were
    // supposed to be deleted while the list was iterating
    while ( !termination_stack.empty() )
    {
        reference ref( termination_stack.top() );
        termination_stack.pop();

        free_one( ref );
    }

}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
egoboo_rv t_ego_obj_lst< _ty, _sz >::add_activation( const t_reference<_ty> & ref )
{
    // put this object into the activation list so that it can be activated right after
    // the t_ego_obj_lst<> loop is completed

    if ( !validate_ref( ref ) ) return rv_error;

    activation_stack.push( ref.get_value() );

    _ty     * pobj  = ary + ref.get_value();
    ego_obj * pbase = pobj->get_pego_obj();
    if ( NULL != pbase )
    {
        pbase->proc_req_on( btrue );
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
egoboo_rv t_ego_obj_lst< _ty, _sz >::add_termination( const t_reference<_ty> & ref )
{
    egoboo_rv retval = rv_fail;

    if ( !validate_ref( ref ) ) return rv_fail;

    termination_stack.push( ref.get_value() );

    ego_obj * pobj = ary + ref.get_value();
    if ( NULL != pobj )
    {
        // at least mark the object as "waiting to be terminated"
        ego_obj::req_terminate( pobj );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
typename _ty::data_type &  t_ego_obj_lst< _ty, _sz >::get_data( const t_reference<_ty> & ref )
{
    _ty * ptr = ary + ref.get_value();

    // there is no graceful recovery from this other than to have a special
    //  element that is returned on an error
    EGOBOO_ASSERT( NULL != ptr );

    return ptr->get_data();
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
typename _ty::data_type *  t_ego_obj_lst< _ty, _sz >::get_pdata( const t_reference<_ty> & ref )
{
    _ty * ptr = ary + ref.get_value();

    if ( NULL == ptr ) return NULL;

    return ( NULL == ptr ) ? NULL : ptr->get_pdata();
}

//--------------------------------------------------------------------------------------------
template <typename _ty, size_t _sz>
typename _ty::data_type *  t_ego_obj_lst< _ty, _sz >::get_valid_pdata( const t_reference<_ty> & ref )
{
    _ty * ptr = get_valid_ptr( ref );
    if ( NULL == ptr ) return NULL;

    return ( NULL == ptr ) ? NULL : ptr->get_pdata();
}
