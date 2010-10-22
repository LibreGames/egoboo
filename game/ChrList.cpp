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

/// @file ChrObjList.c
/// @brief Implementation of the ChrList_* functions
/// @details

#include "ChrList.h"

#include "char.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ChrObjList_t ChrObjList;

////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//
//t_cpp_list< ego_chr, MAX_CHR  > ChrObjList;
//
//static size_t  chr_termination_count = 0;
//static CHR_REF chr_termination_list[MAX_CHR];
//
//static size_t  chr_activation_count = 0;
//static CHR_REF chr_activation_list[MAX_CHR];
//
////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//int ChrObjList.loop_depth = 0;
//
////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//
//static size_t  ChrObjList.get_free();
//
//static bool_t ChrObjList.remove_used( const CHR_REF & ichr );
//static bool_t ChrObjList.remove_used_index( int index );
//static egoboo_rv ChrObjList.add_free( const CHR_REF & ichr );
//static bool_t ChrObjList.remove_free( const CHR_REF & ichr );
//static bool_t ChrObjList.remove_free_index( int index );
//
////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//void ChrObjList.init()
//{
//    int cnt;
//
//    ChrObjList.free_count() = 0;
//    ChrObjList.used_count = 0;
//    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
//    {
//        ChrObjList.free_ref[cnt] = MAX_CHR;
//        ChrObjList.used_ref[cnt] = MAX_CHR;
//    }
//
//    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
//    {
//        CHR_REF ichr = ( CHR_REF )(( MAX_CHR - 1 ) - cnt );
//        ego_chr * pchr = ChrObjList.get_pdata(ichr);
//
//        // blank out all the data, including the obj_base data
//        memset( pchr, 0, sizeof( *pchr ) );
//
//        // character "initializer"
//        ego_obj::ctor_this( POBJ_GET_PBASE( pchr ), (ichr).get_value() );
//
//        ChrObjList.add_free( ichr );
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void ChrObjList.dtor_this()
//{
//    CHR_REF ichr;
//    size_t  cnt;
//
//    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
//    {
//        ego_obj_chr::do_deconstruct( ChrObjList.get_pdata(ichr), 100 );
//    }
//
//    ChrObjList.free_count() = 0;
//    ChrObjList.used_count = 0;
//    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
//    {
//        ChrObjList.free_ref[cnt] = MAX_CHR;
//        ChrObjList.used_ref[cnt] = MAX_CHR;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void ChrObjList.prune_used()
//{
//    // prune the used list
//
//    size_t cnt;
//    CHR_REF ichr;
//
//    for ( cnt = 0; cnt < ChrObjList.used_count; cnt++ )
//    {
//        bool_t removed = bfalse;
//
//        ichr = ChrObjList.used_ref[cnt];
//
//        if ( !VALID_CHR_REF( ichr ) || !DEFINED_CHR( ichr ) )
//        {
//            removed = ChrObjList.remove_used_index( cnt );
//        }
//
//        if ( removed && !ChrObjList.get_data(ichr).get_proc().in_free_list )
//        {
//            ChrObjList.add_free( ichr );
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void ChrObjList.prune_free()
//{
//    // prune the free list
//
//    size_t cnt;
//    CHR_REF ichr;
//
//    for ( cnt = 0; cnt < ChrObjList.free_count(); cnt++ )
//    {
//        bool_t removed = bfalse;
//
//        ichr = ChrObjList.free_ref[cnt];
//
//        if ( VALID_CHR_REF( ichr ) && INGAME_CHR_BASE( ichr ) )
//        {
//            removed = ChrObjList.remove_free_index( cnt );
//        }
//
//        if ( removed && !ChrObjList.get_data(ichr).get_proc().in_free_list )
//        {
//            ChrObjList.add_used( ichr );
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void ChrObjList.update_used()
//{
//    size_t cnt;
//    CHR_REF ichr;
//
//    ChrObjList.prune_used();
//    ChrObjList.prune_free();
//
//    // go through the character list to see if there are any dangling characters
//    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
//    {
//        if ( !ALLOCATED_CHR( ichr ) ) continue;
//
//        if ( INGAME_CHR( ichr ) )
//        {
//            if ( !ChrObjList.get_data(ichr).get_proc().in_used_list )
//            {
//                ChrObjList.add_used( ichr );
//            }
//        }
//        else if ( !DEFINED_CHR( ichr ) )
//        {
//            if ( !ChrObjList.get_data(ichr).get_proc().in_free_list )
//            {
//                ChrObjList.add_free( ichr );
//            }
//        }
//    }
//
//    // blank out the unused elements of the used list
//    for ( cnt = ChrObjList.used_count; cnt < MAX_CHR; cnt++ )
//    {
//        ChrObjList.used_ref[cnt] = MAX_CHR;
//    }
//
//    // blank out the unused elements of the free list
//    for ( cnt = ChrObjList.free_count(); cnt < MAX_CHR; cnt++ )
//    {
//        ChrObjList.free_ref[cnt] = MAX_CHR;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//egoboo_rv ChrObjList.free_one( const CHR_REF & ichr )
//{
//    /// @details ZZ@> This function sticks a character back on the free character stack
//    ///
//    /// @note Tying ALLOCATED_CHR() and POBJ_TERMINATE() to ChrObjList.free_one()
//    /// should be enough to ensure that no character is freed more than once
//
//    egoboo_rv retval;
//    ego_chr * pchr;
//    ego_obj * pbase;
//
//    if ( !ALLOCATED_CHR( ichr ) ) return rv_fail;
//    pchr = ChrObjList.get_pdata(ichr);
//
//    pbase = POBJ_GET_PBASE( pchr );
//    if ( NULL == pbase ) return rv_error;
//
//#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && EGO_DEBUG
//    chr_log_script_time( ichr );
//#endif
//
//    // if we are inside a ChrObjList loop, do not actually change the length of the
//    // list. This will cause some problems later.
//    if ( ChrObjList.loop_depth > 0 )
//    {
//        retval = ChrObjList.add_termination( ichr );
//    }
//    else
//    {
//        // deallocate any dynamically allocated memory
//        pchr = ego_obj_chr::do_deinitialize( pchr, 100 );
//        if ( NULL == pchr ) return rv_error;
//
//        if ( pbase->in_used_list() )
//        {
//            ChrObjList.remove_used( ichr );
//        }
//
//        if ( pbase->in_free_list() )
//        {
//            retval = rv_success;
//        }
//        else
//        {
//            retval = ChrObjList.add_free( ichr );
//        }
//
//        // character "destructor"
//        pchr = ego_chr::dtor_this( pchr );
//        if ( NULL == pchr ) return rv_error;
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//size_t ChrObjList.get_free()
//{
//    /// @details ZZ@> This function returns the next free character or MAX_CHR if there are none
//
//    size_t retval = MAX_CHR;
//
//    if ( ChrObjList.free_count() > 0 )
//    {
//        ChrObjList.free_count()--;
//        ChrObjList.update_guid()++;
//
//        retval = ChrObjList.free_ref[ChrObjList.free_count()];
//
//        // completely remove it from the free list
//        ChrObjList.free_ref[ChrObjList.free_count()] = MAX_CHR;
//
//        if ( VALID_CHR_IDX( retval ) )
//        {
//            ego_obj * pobj = POBJ_GET_PBASE( ChrObjList.lst + (CHR_REF)retval );
//
//            // let the object know it is not in the free list any more
//            cpp_list_state::set_free( &( pobj->lst_state ), bfalse );
//        }
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//void ChrObjList.free_all()
//{
//    CHR_REF cnt;
//
//    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
//    {
//        ChrObjList.free_one( cnt );
//    }
//}
//
////--------------------------------------------------------------------------------------------
//int ChrObjList.get_free_list_index( const CHR_REF & ichr )
//{
//    int    retval = -1;
//    size_t cnt;
//
//    if ( !VALID_CHR_REF( ichr ) ) return retval;
//
//    for ( cnt = 0; cnt < ChrObjList.free_count(); cnt++ )
//    {
//        if ( ichr == ChrObjList.free_ref[cnt] )
//        {
//            EGOBOO_ASSERT( ChrObjList.get_data(ichr).get_proc().in_free_list );
//            retval = cnt;
//            break;
//        }
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//egoboo_rv ChrObjList.add_free( const CHR_REF & ichr )
//{
//    egoboo_rv retval;
//
//    if ( !VALID_CHR_REF( ichr ) ) return rv_error;
//
//#if EGO_DEBUG && defined(DEBUG_CHR_LIST)
//    if ( ChrObjList.get_free_list_index( ichr ) > 0 )
//    {
//        return rv_error;
//    }
//#endif
//
//    EGOBOO_ASSERT( !ChrObjList.get_data(ichr).get_proc().in_free_list );
//
//    retval = rv_fail;
//    if ( ChrObjList.free_count() < MAX_CHR )
//    {
//        ChrObjList.free_ref[ChrObjList.free_count()] = (ichr).get_value();
//
//        ChrObjList.free_count()++;
//        ChrObjList.update_guid()++;
//
//        cpp_list_state::set_free( &( ChrObjList.get_data(ichr).get_proc() ), btrue );
//
//        retval = rv_success;
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t ChrObjList.remove_free_index( int index )
//{
//    CHR_REF ichr;
//
//    // was it found?
//    if ( index < 0 || ( size_t )index >= ChrObjList.free_count() ) return bfalse;
//
//    ichr = ChrObjList.free_ref[index];
//
//    // blank out the index in the list
//    ChrObjList.free_ref[index] = MAX_CHR;
//
//    if ( VALID_CHR_REF( ichr ) )
//    {
//        // let the object know it is not in the list anymore
//        cpp_list_state::set_free( &( ChrObjList.get_data(ichr).get_proc() ), bfalse );
//    }
//
//    // shorten the list
//    ChrObjList.free_count()--;
//    ChrObjList.update_guid()++;
//
//    if ( ChrObjList.free_count() > 0 )
//    {
//        // swap the last element for the deleted element
//        SWAP( size_t, ChrObjList.free_ref[index], ChrObjList.free_ref[ChrObjList.free_count()] );
//    }
//
//    return btrue;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t ChrObjList.remove_free( const CHR_REF & ichr )
//{
//    // find the object in the free list
//    int index = ChrObjList.get_free_list_index( ichr );
//
//    return ChrObjList.remove_free_index( index );
//}
//
////--------------------------------------------------------------------------------------------
//int ChrObjList.get_used_list_index( const CHR_REF & ichr )
//{
//    int   retval = -1;
//    size_t cnt;
//
//    if ( !VALID_CHR_REF( ichr ) ) return retval;
//
//    for ( cnt = 0; cnt < ChrObjList.used_count; cnt++ )
//    {
//        if ( ichr == ChrObjList.used_ref[cnt] )
//        {
//            EGOBOO_ASSERT( ChrObjList.get_data(ichr).get_proc().in_used_list );
//            retval = cnt;
//            break;
//        }
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t ChrObjList.add_used( const CHR_REF & ichr )
//{
//    bool_t retval;
//
//    if ( !VALID_CHR_REF( ichr ) ) return bfalse;
//
//#if EGO_DEBUG && defined(DEBUG_CHR_LIST)
//    if ( ChrObjList.get_used_list_index( ichr ) > 0 )
//    {
//        return bfalse;
//    }
//#endif
//
//    EGOBOO_ASSERT( !ChrObjList.get_data(ichr).get_proc().in_used_list );
//
//    retval = bfalse;
//    if ( ChrObjList.used_count < MAX_CHR )
//    {
//        ChrObjList.used_ref[ChrObjList.used_count] = (ichr).get_value();
//
//        ChrObjList.used_count++;
//        ChrObjList.update_guid()++;
//
//        cpp_list_state::set_used( &( ChrObjList.get_data(ichr).get_proc() ), btrue );
//
//        retval = btrue;
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t ChrObjList.remove_used_index( int index )
//{
//    CHR_REF ichr;
//
//    // was it found?
//    if ( index < 0 || ( size_t )index >= ChrObjList.used_count ) return bfalse;
//
//    ichr = ChrObjList.used_ref[index];
//
//    // blank out the index in the list
//    ChrObjList.used_ref[index] = MAX_CHR;
//
//    if ( VALID_CHR_REF( ichr ) )
//    {
//        // let the object know it is not in the list anymore
//        cpp_list_state::set_used( &( ChrObjList.get_data(ichr).get_proc() ), bfalse );
//    }
//
//    // shorten the list
//    ChrObjList.used_count--;
//    ChrObjList.update_guid()++;
//
//    if ( ChrObjList.used_count > 0 )
//    {
//        // swap the last element for the deleted element
//        SWAP( size_t, ChrObjList.used_ref[index], ChrObjList.used_ref[ChrObjList.used_count] );
//    }
//
//    return btrue;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t ChrObjList.remove_used( const CHR_REF & ichr )
//{
//    // find the object in the used list
//    int index = ChrObjList.get_used_list_index( ichr );
//
//    return ChrObjList.remove_used_index( index );
//}
//
////--------------------------------------------------------------------------------------------
//CHR_REF ChrObjList.allocate( const CHR_REF & override )
//{
//    CHR_REF ichr = ( CHR_REF )MAX_CHR;
//
//    if ( VALID_CHR_REF( override ) )
//    {
//        ichr = ChrObjList.get_free();
//        if ( override != ichr )
//        {
//            int override_index = ChrObjList.get_free_list_index( override );
//
//            if ( override_index < 0 || ( size_t )override_index >= ChrObjList.free_count() )
//            {
//                ichr = ( CHR_REF )MAX_CHR;
//            }
//            else
//            {
//                // store the "wrong" value in the override character's index
//                ChrObjList.free_ref[override_index] = (ichr).get_value();
//
//                // fix the in_free_list values
//                cpp_list_state::set_free( &( ChrObjList.get_data(ichr).get_proc() ), btrue );
//                cpp_list_state::set_free( &( ChrObjList.get_data(override).get_proc() ), bfalse );
//
//                ichr = override;
//            }
//        }
//
//        if ( MAX_CHR == ichr )
//        {
//            log_warning( "ChrObjList.allocate() - failed to override a character? character %d already spawned? \n", (override ).get_value() );
//        }
//    }
//    else
//    {
//        ichr = ChrObjList.get_free();
//        if ( MAX_CHR == ichr )
//        {
//            log_warning( "ChrObjList.allocate() - failed to allocate a new character\n" );
//        }
//    }
//
//    // if the character is already being used, make sure to destroy the old one
//    if ( DEFINED_CHR( ichr ) )
//    {
//        ChrObjList.free_one( ichr );
//    }
//
//    if ( VALID_CHR_REF( ichr ) )
//    {
//        // allocate the new one
//        POBJ_ALLOCATE( ChrObjList.get_pdata(ichr), (ichr).get_value() );
//    }
//
//    if ( VALID_CHR( ichr ) )
//    {
//        // construct the new structure
//        ego_obj_chr::do_construct( ChrObjList.get_pdata(ichr), 100 );
//    }
//
//    return ichr;
//}
//
////--------------------------------------------------------------------------------------------
//void ChrObjList.cleanup()
//{
//    size_t  cnt;
//    ego_chr * pchr;
//
//    // go through the list and activate all the characters that
//    // were created while the list was iterating
//    for ( cnt = 0; cnt < chr_activation_count; cnt++ )
//    {
//        CHR_REF ichr = chr_activation_list[cnt];
//
//        if ( !VALID_CHR( ichr ) ) continue;
//        pchr = ChrObjList.get_pdata(ichr);
//
//        ego_obj::grant_on( POBJ_GET_PBASE( pchr ) );
//    }
//    chr_activation_count = 0;
//
//    // go through and delete any characters that were
//    // supposed to be deleted while the list was iterating
//    for ( cnt = 0; cnt < chr_termination_count; cnt++ )
//    {
//        ChrObjList.free_one( chr_termination_list[cnt] );
//    }
//    chr_termination_count = 0;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t ChrObjList.add_activation( CHR_REF ichr )
//{
//    // put this character into the activation list so that it can be activated right after
//    // the ChrObjList loop is completed
//
//    bool_t retval = bfalse;
//
//    if ( !VALID_CHR_REF( ichr ) ) return bfalse;
//
//    if ( chr_activation_count < MAX_CHR )
//    {
//        chr_activation_list[chr_activation_count] = ichr;
//        chr_activation_count++;
//
//        retval = btrue;
//    }
//
//    ChrObjList.get_obj(ichr).proc_req_on( btrue );
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//egoboo_rv ChrObjList.add_termination( CHR_REF ichr )
//{
//    egoboo_rv retval = rv_fail;
//
//    if ( !VALID_CHR_REF( ichr ) ) return rv_fail;
//
//    if ( chr_termination_count < MAX_CHR )
//    {
//        chr_termination_list[chr_termination_count] = ichr;
//        chr_termination_count++;
//
//        retval = rv_success;
//    }
//
//    // at least mark the object as "waiting to be terminated"
//    POBJ_REQUEST_TERMINATE( ChrObjList.get_pdata(ichr ));
//
//    return retval;
//}
//
