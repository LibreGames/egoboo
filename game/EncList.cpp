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

/// @file EncList.c
/// @brief Implementation of the EncList_* functions
/// @details

#include "EncList.h"

#include "enchant.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
t_ego_obj_lst<ego_enc,MAX_ENC> EncList;

////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//
//INSTANTIATE_LIST( ACCESS_TYPE_NONE, ego_enc, EncList, MAX_ENC );
//
//static size_t  ego_enc_termination_count = 0;
//static ENC_REF ego_enc_termination_list[MAX_ENC];
//
//static size_t  ego_enc_activation_count = 0;
//static ENC_REF ego_enc_activation_list[MAX_ENC];
//
////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//int ego_enc_loop_depth = 0;
//
////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//
//static size_t  EncList_get_free();
//
//static bool_t EncList_remove_used( const ENC_REF & ienc );
//static bool_t EncList_remove_used_index( int index );
//static bool_t EncList_add_free( const ENC_REF & ienc );
//static bool_t EncList_remove_free( const ENC_REF & ienc );
//static bool_t EncList_remove_free_index( int index );
//
////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//void EncList_init()
//{
//    int cnt;
//
//    EncList.free_count = 0;
//    EncList.used_count = 0;
//    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
//    {
//        EncList.free_ref[cnt] = MAX_ENC;
//        EncList.used_ref[cnt] = MAX_ENC;
//    }
//
//    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
//    {
//        ENC_REF ienc = ( ENC_REF )(( MAX_ENC - 1 ) - cnt );
//        ego_enc * penc = EncList.get_valid_ptr(ienc);
//
//        // blank out all the data, including the obj_base data
//        memset( penc, 0, sizeof( *penc ) );
//
//        // enchant "initializer"
//        ego_object::ctor( POBJ_GET_PBASE( penc ), ienc );
//
//        EncList_add_free( ienc );
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void EncList_dtor()
//{
//    ENC_REF cnt;
//
//    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
//    {
//        ego_enc::run_object_deconstruct( EncList.get_valid_ptr(cnt), 100 );
//    }
//
//    EncList.free_count = 0;
//    EncList.used_count = 0;
//    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
//    {
//        EncList.free_ref[cnt] = MAX_ENC;
//        EncList.used_ref[cnt] = MAX_ENC;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void EncList_prune_used()
//{
//    // prune the used list
//
//    size_t cnt;
//    ENC_REF ienc;
//
//    for ( cnt = 0; cnt < EncList.used_count; cnt++ )
//    {
//        bool_t removed = bfalse;
//
//        ienc = EncList.used_ref[cnt];
//
//        if ( !VALID_ENC_REF( ienc ) || !DEFINED_ENC( ienc ) )
//        {
//            removed = EncList_remove_used_index( cnt );
//        }
//
//        if ( removed && !EncList.lst[ienc].obj_base.lst_state.in_free_list )
//        {
//            EncList_add_free( ienc );
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void EncList_prune_free()
//{
//    // prune the free list
//
//    size_t cnt;
//    ENC_REF ienc;
//
//    for ( cnt = 0; cnt < EncList.free_count; cnt++ )
//    {
//        bool_t removed = bfalse;
//
//        ienc = EncList.free_ref[cnt];
//
//        if ( VALID_ENC_REF( ienc ) && INGAME_PRT_BASE( ienc ) )
//        {
//            removed = EncList_remove_free_index( cnt );
//        }
//
//        if ( removed && !EncList.lst[ienc].obj_base.lst_state.in_free_list )
//        {
//            EncList_add_used( ienc );
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void EncList_update_used()
//{
//    size_t cnt;
//    ENC_REF ienc;
//
//    EncList_prune_used();
//
//    EncList_prune_free();
//
//    // go through the enchant list to see if there are any dangling enchants
//    for ( ienc = 0; ienc < MAX_ENC; ienc++ )
//    {
//        ego_enc               * penc;
//        list_object_state * lst_obj_ptr;
//
//        if ( !VALID_ENC( ienc ) ) continue;
//        penc        = EncList.get_valid_ptr(ienc);
//        lst_obj_ptr = &( penc->obj_base.lst_state );
//
//        if ( INGAME_ENC( ienc ) )
//        {
//            if ( !lst_obj_ptr->in_used_list )
//            {
//                EncList_add_used( ienc );
//            }
//        }
//        else if ( !DEFINED_ENC( ienc ) )
//        {
//            if ( !lst_obj_ptr->in_free_list )
//            {
//                EncList_add_free( ienc );
//            }
//        }
//    }
//
//    // blank out the unused elements of the used list
//    for ( cnt = EncList.used_count; cnt < MAX_ENC; cnt++ )
//    {
//        EncList.used_ref[cnt] = MAX_ENC;
//    }
//
//    // blank out the unused elements of the free list
//    for ( cnt = EncList.free_count; cnt < MAX_ENC; cnt++ )
//    {
//        EncList.free_ref[cnt] = MAX_ENC;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//bool_t EncList_free_one( const ENC_REF & ienc )
//{
//    /// @details ZZ@> This function sticks a enchant back on the free enchant stack
//    ///
//    /// @note Tying ALLOCATED_ENC() and POBJ_TERMINATE() to EncList_free_one()
//    /// should be enough to ensure that no enchant is freed more than once
//
//    bool_t retval;
//    ego_enc * penc;
//    ego_object * pbase;
//
//    if ( !ALLOCATED_ENC( ienc ) ) return bfalse;
//    penc = EncList.get_valid_ptr(ienc);
//
//    pbase = POBJ_GET_PBASE( penc );
//    if ( NULL == pbase ) return bfalse;
//
//#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && EGO_DEBUG
//    ego_enc::log_script_time( ienc );
//#endif
//
//    // if we are inside a EncList loop, do not actually change the length of the
//    // list. This will cause some problems later.
//    if ( ego_enc_loop_depth > 0 )
//    {
//        retval = EncList_add_termination( ienc );
//    }
//    else
//    {
//        // deallocate any dynamically allocated memory
//        penc = ego_enc::run_object_deinitialize( penc, 100 );
//        if ( NULL == penc ) return bfalse;
//
//        if ( pbase->lst_state.in_used_list )
//        {
//            EncList_remove_used( ienc );
//        }
//
//        if ( pbase->lst_state.in_free_list )
//        {
//            retval = btrue;
//        }
//        else
//        {
//            retval = EncList_add_free( ienc );
//        }
//
//        // enchant "destructor"
//        penc = ego_enc::dtor( penc );
//        if ( NULL == penc ) return bfalse;
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//size_t EncList_get_free()
//{
//    /// @details ZZ@> This function returns the next free enchant or MAX_ENC if there are none
//
//    size_t retval = MAX_ENC;
//
//    if ( EncList.free_count > 0 )
//    {
//        EncList.free_count--;
//        EncList.update_guid++;
//
//        retval = EncList.free_ref[EncList.free_count];
//
//        // completely remove it from the free list
//        EncList.free_ref[EncList.free_count] = MAX_ENC;
//
//        if ( VALID_ENC_REF( retval ) )
//        {
//            // let the object know it is not in the free list any more
//            list_object_state::set_free( &( EncList.lst[retval].obj_base.lst_state ), bfalse );
//        }
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//void EncList_free_all()
//{
//    ENC_REF cnt;
//
//    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
//    {
//        EncList_free_one( cnt );
//    }
//}
//
////--------------------------------------------------------------------------------------------
//int EncList_get_free_list_index( const ENC_REF & ienc )
//{
//    int retval = -1;
//    size_t cnt;
//
//    if ( !VALID_ENC_REF( ienc ) ) return retval;
//
//    for ( cnt = 0; cnt < EncList.free_count; cnt++ )
//    {
//        if ( ienc == EncList.free_ref[cnt] )
//        {
//            EGOBOO_ASSERT( EncList.lst[ienc].obj_base.lst_state.in_free_list );
//            retval = cnt;
//            break;
//        }
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t EncList_add_free( const ENC_REF & ienc )
//{
//    bool_t retval;
//
//    if ( !VALID_ENC_REF( ienc ) ) return bfalse;
//
//#if EGO_DEBUG && defined(DEBUG_ENC_LIST)
//    if ( EncList_get_free_list_index( ienc ) > 0 )
//    {
//        return bfalse;
//    }
//#endif
//
//    EGOBOO_ASSERT( !EncList.lst[ienc].obj_base.lst_state.in_free_list );
//
//    retval = bfalse;
//    if ( EncList.free_count < MAX_ENC )
//    {
//        EncList.free_ref[EncList.free_count] = ienc;
//
//        EncList.free_count++;
//        EncList.update_guid++;
//
//        list_object_state::set_free( &( EncList.lst[ienc].obj_base.lst_state ), btrue );
//
//        retval = btrue;
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t EncList_remove_free_index( int index )
//{
//    ENC_REF ienc;
//
//    // was it found?
//    if ( index < 0 || ( size_t )index >= EncList.free_count ) return bfalse;
//
//    ienc = EncList.free_ref[index];
//
//    // blank out the index in the list
//    EncList.free_ref[index] = MAX_ENC;
//
//    if ( VALID_ENC_REF( ienc ) )
//    {
//        // let the object know it is not in the list anymore
//        list_object_state::set_free( &( EncList.lst[ienc].obj_base.lst_state ), bfalse );
//    }
//
//    // shorten the list
//    EncList.free_count--;
//    EncList.update_guid++;
//
//    if ( EncList.free_count > 0 )
//    {
//        // swap the last element for the deleted element
//        SWAP( size_t, EncList.free_ref[index], EncList.free_ref[EncList.free_count] );
//    }
//
//    return btrue;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t EncList_remove_free( const ENC_REF & ienc )
//{
//    // find the object in the free list
//    int index = EncList_get_free_list_index( ienc );
//
//    return EncList_remove_free_index( index );
//}
//
////--------------------------------------------------------------------------------------------
//int EncList_get_used_list_index( const ENC_REF & ienc )
//{
//    int retval = -1;
//    size_t cnt;
//
//    if ( !VALID_ENC_REF( ienc ) ) return retval;
//
//    for ( cnt = 0; cnt < EncList.used_count; cnt++ )
//    {
//        if ( ienc == EncList.used_ref[cnt] )
//        {
//            EGOBOO_ASSERT( EncList.lst[ienc].obj_base.lst_state.in_used_list );
//            retval = cnt;
//            break;
//        }
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t EncList_add_used( const ENC_REF & ienc )
//{
//    bool_t retval;
//
//    if ( !VALID_ENC_REF( ienc ) ) return bfalse;
//
//#if EGO_DEBUG && defined(DEBUG_ENC_LIST)
//    if ( EncList_get_used_list_index( ienc ) > 0 )
//    {
//        return bfalse;
//    }
//#endif
//
//    EGOBOO_ASSERT( !EncList.lst[ienc].obj_base.lst_state.in_used_list );
//
//    retval = bfalse;
//    if ( EncList.used_count < MAX_ENC )
//    {
//        EncList.used_ref[EncList.used_count] = ienc;
//
//        EncList.used_count++;
//        EncList.update_guid++;
//
//        list_object_state::set_used( &( EncList.lst[ienc].obj_base.lst_state ), btrue );
//
//        retval = btrue;
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t EncList_remove_used_index( int index )
//{
//    ENC_REF ienc;
//
//    // was it found?
//    if ( index < 0 || ( size_t )index >= EncList.used_count ) return bfalse;
//
//    ienc = EncList.used_ref[index];
//
//    // blank out the index in the list
//    EncList.used_ref[index] = MAX_ENC;
//
//    if ( VALID_ENC_REF( ienc ) )
//    {
//        // let the object know it is not in the list anymore
//        list_object_state::set_used( &( EncList.lst[ienc].obj_base.lst_state ), bfalse );
//    }
//
//    // shorten the list
//    EncList.used_count--;
//    EncList.update_guid++;
//
//    if ( EncList.used_count > 0 )
//    {
//        // swap the last element for the deleted element
//        SWAP( size_t, EncList.used_ref[index], EncList.used_ref[EncList.used_count] );
//    }
//
//    return btrue;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t EncList_remove_used( const ENC_REF & ienc )
//{
//    // find the object in the used list
//    int index = EncList_get_used_list_index( ienc );
//
//    return EncList_remove_used_index( index );
//}
//
////--------------------------------------------------------------------------------------------
//ENC_REF EncList_allocate( const ENC_REF & override )
//{
//    ENC_REF ienc = ( ENC_REF )MAX_ENC;
//
//    if ( VALID_ENC_REF( override ) )
//    {
//        ienc = EncList_get_free();
//        if ( override != ienc )
//        {
//            int override_index = EncList_get_free_list_index( override );
//
//            if ( override_index < 0 || ( size_t )override_index >= EncList.free_count )
//            {
//                ienc = ( ENC_REF )MAX_ENC;
//            }
//            else
//            {
//                // store the "wrong" value in the override enchant's index
//                EncList.free_ref[override_index] = ienc;
//
//                // fix the in_free_list values
//                list_object_state::set_free( &( EncList.lst[ienc].obj_base.lst_state ), btrue );
//                list_object_state::set_free( &( EncList.lst[override].obj_base.lst_state ), bfalse );
//
//                ienc = override;
//            }
//        }
//
//        if ( MAX_ENC == ienc )
//        {
//            log_warning( "EncList_allocate() - failed to override a enchant? enchant %d already spawned? \n", REF_TO_INT( override ) );
//        }
//    }
//    else
//    {
//        ienc = EncList_get_free();
//        if ( MAX_ENC == ienc )
//        {
//            log_warning( "EncList_allocate() - failed to allocate a new enchant\n" );
//        }
//    }
//
//    if ( VALID_ENC_REF( ienc ) )
//    {
//        // if the enchant is already being used, make sure to destroy the old one
//        if ( DEFINED_ENC( ienc ) )
//        {
//            EncList_free_one( ienc );
//        }
//
//        // allocate the new one
//        POBJ_ALLOCATE( EncList.get_valid_ptr(ienc), ienc );
//    }
//
//    if ( VALID_ENC( ienc ) )
//    {
//        // construct the new structure
//        ego_enc::run_object_construct( EncList.get_valid_ptr(ienc), 100 );
//    }
//
//    return ienc;
//}
//
////--------------------------------------------------------------------------------------------
//void EncList_cleanup()
//{
//    size_t  cnt;
//    ego_enc * penc;
//
//    // go through the list and activate all the enchants that
//    // were created while the list was iterating
//    for ( cnt = 0; cnt < ego_enc_activation_count; cnt++ )
//    {
//        ENC_REF ienc = ego_enc_activation_list[cnt];
//
//        if ( !VALID_ENC( ienc ) ) continue;
//        penc = EncList.get_valid_ptr(ienc);
//
//        ego_object::grant_on( POBJ_GET_PBASE( penc ) );
//    }
//    ego_enc_activation_count = 0;
//
//    // go through and delete any enchants that were
//    // supposed to be deleted while the list was iterating
//    for ( cnt = 0; cnt < ego_enc_termination_count; cnt++ )
//    {
//        EncList_free_one( ego_enc_termination_list[cnt] );
//    }
//    ego_enc_termination_count = 0;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t EncList_add_activation( ENC_REF ienc )
//{
//    // put this enchant into the activation list so that it can be activated right after
//    // the EncList loop is completed
//
//    bool_t retval = bfalse;
//
//    if ( !VALID_ENC_REF( ienc ) ) return bfalse;
//
//    if ( ego_enc_activation_count < MAX_ENC )
//    {
//        ego_enc_activation_list[ego_enc_activation_count] = ienc;
//        ego_enc_activation_count++;
//
//        retval = btrue;
//    }
//
//    EncList.lst[ienc].obj_base.req.turn_me_on = btrue;
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t EncList_add_termination( ENC_REF ienc )
//{
//    bool_t retval = bfalse;
//
//    if ( !VALID_ENC_REF( ienc ) ) return bfalse;
//
//    if ( ego_enc_termination_count < MAX_ENC )
//    {
//        ego_enc_termination_list[ego_enc_termination_count] = ienc;
//        ego_enc_termination_count++;
//
//        retval = btrue;
//    }
//
//    // at least mark the object as "waiting to be terminated"
//    POBJ_REQUEST_TERMINATE( EncList.get_valid_ptr(ienc ));
//
//    return retval;
//}
//
