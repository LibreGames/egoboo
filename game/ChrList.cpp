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

/// @file ChrList.c
/// @brief Implementation of the ChrList_* functions
/// @details

#include "ChrList.h"
#include "log.h"
#include "egoboo_object.h"

#include "char.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST( ACCESS_TYPE_NONE, ego_chr, ChrList, MAX_CHR );

static size_t  chr_termination_count = 0;
static CHR_REF chr_termination_list[MAX_CHR];

static size_t  chr_activation_count = 0;
static CHR_REF chr_activation_list[MAX_CHR];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_loop_depth = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static size_t  ChrList_get_free();

static bool_t ChrList_remove_used( const CHR_REF by_reference ichr );
static bool_t ChrList_remove_used_index( int index );
static egoboo_rv ChrList_add_free( const CHR_REF by_reference ichr );
static bool_t ChrList_remove_free( const CHR_REF by_reference ichr );
static bool_t ChrList_remove_free_index( int index );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ChrList_init()
{
    int cnt;

    ChrList.free_count = 0;
    ChrList.used_count = 0;
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList.free_ref[cnt] = MAX_CHR;
        ChrList.used_ref[cnt] = MAX_CHR;
    }

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        CHR_REF ichr = ( CHR_REF )(( MAX_CHR - 1 ) - cnt );
        ego_chr * pchr = ChrList.lst + ichr;

        // blank out all the data, including the obj_base data
        memset( pchr, 0, sizeof( *pchr ) );

        // character "initializer"
        ego_object::ctor( POBJ_GET_PBASE( pchr ), ichr );

        ChrList_add_free( ichr );
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_dtor()
{
    CHR_REF cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ego_chr::run_object_deconstruct( ChrList.lst + cnt, 100 );
    }

    ChrList.free_count = 0;
    ChrList.used_count = 0;
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList.free_ref[cnt] = MAX_CHR;
        ChrList.used_ref[cnt] = MAX_CHR;
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_prune_used()
{
    // prune the used list

    size_t cnt;
    CHR_REF ichr;

    for ( cnt = 0; cnt < ChrList.used_count; cnt++ )
    {
        bool_t removed = bfalse;

        ichr = ChrList.used_ref[cnt];

        if ( !VALID_CHR_RANGE( ichr ) || !DEFINED_CHR( ichr ) )
        {
            removed = ChrList_remove_used_index( cnt );
        }

        if ( removed && !ChrList.lst[ichr].obj_base.lst_state.in_free_list )
        {
            ChrList_add_free( ichr );
        }
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_prune_free()
{
    // prune the free list

    size_t cnt;
    CHR_REF ichr;

    for ( cnt = 0; cnt < ChrList.free_count; cnt++ )
    {
        bool_t removed = bfalse;

        ichr = ChrList.free_ref[cnt];

        if ( VALID_CHR_RANGE( ichr ) && INGAME_PRT_BASE( ichr ) )
        {
            removed = ChrList_remove_free_index( cnt );
        }

        if ( removed && !ChrList.lst[ichr].obj_base.lst_state.in_free_list )
        {
            ChrList_add_used( ichr );
        }
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_update_used()
{
    size_t cnt;
    CHR_REF ichr;

    ChrList_prune_used();
    ChrList_prune_free();

    // go through the character list to see if there are any dangling characters
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        if ( !ALLOCATED_CHR( ichr ) ) continue;

        if ( INGAME_CHR( ichr ) )
        {
            if ( !ChrList.lst[ichr].obj_base.lst_state.in_used_list )
            {
                ChrList_add_used( ichr );
            }
        }
        else if ( !DEFINED_CHR( ichr ) )
        {
            if ( !ChrList.lst[ichr].obj_base.lst_state.in_free_list )
            {
                ChrList_add_free( ichr );
            }
        }
    }

    // blank out the unused elements of the used list
    for ( cnt = ChrList.used_count; cnt < MAX_CHR; cnt++ )
    {
        ChrList.used_ref[cnt] = MAX_CHR;
    }

    // blank out the unused elements of the free list
    for ( cnt = ChrList.free_count; cnt < MAX_CHR; cnt++ )
    {
        ChrList.free_ref[cnt] = MAX_CHR;
    }
}

//--------------------------------------------------------------------------------------------
egoboo_rv ChrList_free_one( const CHR_REF by_reference ichr )
{
    /// @details ZZ@> This function sticks a character back on the free character stack
    ///
    /// @note Tying ALLOCATED_CHR() and POBJ_TERMINATE() to ChrList_free_one()
    /// should be enough to ensure that no character is freed more than once

    egoboo_rv retval;
    ego_chr * pchr;
    ego_object * pbase;

    if ( !ALLOCATED_CHR( ichr ) ) return rv_fail;
    pchr = ChrList.lst + ichr;

    pbase = POBJ_GET_PBASE( pchr );
    if ( NULL == pbase ) return rv_error;

#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && EGO_DEBUG
    chr_log_script_time( ichr );
#endif

    // if we are inside a ChrList loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( chr_loop_depth > 0 )
    {
        retval = ChrList_add_termination( ichr );
    }
    else
    {
        // deallocate any dynamically allocated memory
        pchr = ego_chr::run_object_deinitialize( pchr, 100 );
        if ( NULL == pchr ) return rv_error;

        if ( pbase->lst_state.in_used_list )
        {
            ChrList_remove_used( ichr );
        }

        if ( pbase->lst_state.in_free_list )
        {
            retval = rv_success;
        }
        else
        {
            retval = ChrList_add_free( ichr );
        }

        // character "destructor"
        pchr = ego_chr::dtor( pchr );
        if ( NULL == pchr ) return rv_error;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t ChrList_get_free()
{
    /// @details ZZ@> This function returns the next free character or MAX_CHR if there are none

    size_t retval = MAX_CHR;

    if ( ChrList.free_count > 0 )
    {
        ChrList.free_count--;
        ChrList.update_guid++;

        retval = ChrList.free_ref[ChrList.free_count];

        // completely remove it from the free list
        ChrList.free_ref[ChrList.free_count] = MAX_CHR;

        if ( VALID_CHR_RANGE( retval ) )
        {
            // let the object know it is not in the free list any more
            list_object_set_free( &( ChrList.lst[retval].obj_base.lst_state ), bfalse );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void ChrList_free_all()
{
    CHR_REF cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
int ChrList_get_free_list_index( const CHR_REF by_reference ichr )
{
    int    retval = -1;
    size_t cnt;

    if ( !VALID_CHR_RANGE( ichr ) ) return retval;

    for ( cnt = 0; cnt < ChrList.free_count; cnt++ )
    {
        if ( ichr == ChrList.free_ref[cnt] )
        {
            EGOBOO_ASSERT( ChrList.lst[ichr].obj_base.lst_state.in_free_list );
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ChrList_add_free( const CHR_REF by_reference ichr )
{
    egoboo_rv retval;

    if ( !VALID_CHR_RANGE( ichr ) ) return rv_error;

#if EGO_DEBUG && defined(DEBUG_CHR_LIST)
    if ( ChrList_get_free_list_index( ichr ) > 0 )
    {
        return rv_error;
    }
#endif

    EGOBOO_ASSERT( !ChrList.lst[ichr].obj_base.lst_state.in_free_list );

    retval = rv_fail;
    if ( ChrList.free_count < MAX_CHR )
    {
        ChrList.free_ref[ChrList.free_count] = ichr;

        ChrList.free_count++;
        ChrList.update_guid++;

        list_object_set_free( &( ChrList.lst[ichr].obj_base.lst_state ), btrue );

        retval = rv_success;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_remove_free_index( int index )
{
    CHR_REF ichr;

    // was it found?
    if ( index < 0 || ( size_t )index >= ChrList.free_count ) return bfalse;

    ichr = ChrList.free_ref[index];

    // blank out the index in the list
    ChrList.free_ref[index] = MAX_CHR;

    if ( VALID_CHR_RANGE( ichr ) )
    {
        // let the object know it is not in the list anymore
        list_object_set_free( &( ChrList.lst[ichr].obj_base.lst_state ), bfalse );
    }

    // shorten the list
    ChrList.free_count--;
    ChrList.update_guid++;

    if ( ChrList.free_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, ChrList.free_ref[index], ChrList.free_ref[ChrList.free_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_remove_free( const CHR_REF by_reference ichr )
{
    // find the object in the free list
    int index = ChrList_get_free_list_index( ichr );

    return ChrList_remove_free_index( index );
}

//--------------------------------------------------------------------------------------------
int ChrList_get_used_list_index( const CHR_REF by_reference ichr )
{
    int   retval = -1;
    size_t cnt;

    if ( !VALID_CHR_RANGE( ichr ) ) return retval;

    for ( cnt = 0; cnt < ChrList.used_count; cnt++ )
    {
        if ( ichr == ChrList.used_ref[cnt] )
        {
            EGOBOO_ASSERT( ChrList.lst[ichr].obj_base.lst_state.in_used_list );
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_add_used( const CHR_REF by_reference ichr )
{
    bool_t retval;

    if ( !VALID_CHR_RANGE( ichr ) ) return bfalse;

#if EGO_DEBUG && defined(DEBUG_CHR_LIST)
    if ( ChrList_get_used_list_index( ichr ) > 0 )
    {
        return bfalse;
    }
#endif

    EGOBOO_ASSERT( !ChrList.lst[ichr].obj_base.lst_state.in_used_list );

    retval = bfalse;
    if ( ChrList.used_count < MAX_CHR )
    {
        ChrList.used_ref[ChrList.used_count] = ichr;

        ChrList.used_count++;
        ChrList.update_guid++;

        list_object_set_used( &( ChrList.lst[ichr].obj_base.lst_state ), btrue );

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_remove_used_index( int index )
{
    CHR_REF ichr;

    // was it found?
    if ( index < 0 || ( size_t )index >= ChrList.used_count ) return bfalse;

    ichr = ChrList.used_ref[index];

    // blank out the index in the list
    ChrList.used_ref[index] = MAX_CHR;

    if ( VALID_CHR_RANGE( ichr ) )
    {
        // let the object know it is not in the list anymore
        list_object_set_used( &( ChrList.lst[ichr].obj_base.lst_state ), bfalse );
    }

    // shorten the list
    ChrList.used_count--;
    ChrList.update_guid++;

    if ( ChrList.used_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, ChrList.used_ref[index], ChrList.used_ref[ChrList.used_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_remove_used( const CHR_REF by_reference ichr )
{
    // find the object in the used list
    int index = ChrList_get_used_list_index( ichr );

    return ChrList_remove_used_index( index );
}

//--------------------------------------------------------------------------------------------
CHR_REF ChrList_allocate( const CHR_REF by_reference override )
{
    CHR_REF ichr = ( CHR_REF )MAX_CHR;

    if ( VALID_CHR_RANGE( override ) )
    {
        ichr = ChrList_get_free();
        if ( override != ichr )
        {
            int override_index = ChrList_get_free_list_index( override );

            if ( override_index < 0 || ( size_t )override_index >= ChrList.free_count )
            {
                ichr = ( CHR_REF )MAX_CHR;
            }
            else
            {
                // store the "wrong" value in the override character's index
                ChrList.free_ref[override_index] = ichr;

                // fix the in_free_list values
                list_object_set_free( &( ChrList.lst[ichr].obj_base.lst_state ), btrue );
                list_object_set_free( &( ChrList.lst[override].obj_base.lst_state ), bfalse );

                ichr = override;
            }
        }

        if ( MAX_CHR == ichr )
        {
            log_warning( "ChrList_allocate() - failed to override a character? character %d already spawned? \n", REF_TO_INT( override ) );
        }
    }
    else
    {
        ichr = ChrList_get_free();
        if ( MAX_CHR == ichr )
        {
            log_warning( "ChrList_allocate() - failed to allocate a new character\n" );
        }
    }

    // if the character is already being used, make sure to destroy the old one
    if ( DEFINED_CHR( ichr ) )
    {
        ChrList_free_one( ichr );
    }

    if ( VALID_CHR_RANGE( ichr ) )
    {
        // allocate the new one
        POBJ_ALLOCATE( ChrList.lst + ichr, ichr );
    }

    if ( VALID_CHR( ichr ) )
    {
        // construct the new structure
        ego_chr::run_object_construct( ChrList.lst + ichr, 100 );
    }

    return ichr;
}

//--------------------------------------------------------------------------------------------
void ChrList_cleanup()
{
    size_t  cnt;
    ego_chr * pchr;

    // go through the list and activate all the characters that
    // were created while the list was iterating
    for ( cnt = 0; cnt < chr_activation_count; cnt++ )
    {
        CHR_REF ichr = chr_activation_list[cnt];

        if ( !VALID_CHR( ichr ) ) continue;
        pchr = ChrList.lst + ichr;

        ego_object::grant_on( POBJ_GET_PBASE( pchr ) );
    }
    chr_activation_count = 0;

    // go through and delete any characters that were
    // supposed to be deleted while the list was iterating
    for ( cnt = 0; cnt < chr_termination_count; cnt++ )
    {
        ChrList_free_one( chr_termination_list[cnt] );
    }
    chr_termination_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_add_activation( CHR_REF ichr )
{
    // put this character into the activation list so that it can be activated right after
    // the ChrList loop is completed

    bool_t retval = bfalse;

    if ( !VALID_CHR_RANGE( ichr ) ) return bfalse;

    if ( chr_activation_count < MAX_CHR )
    {
        chr_activation_list[chr_activation_count] = ichr;
        chr_activation_count++;

        retval = btrue;
    }

    ChrList.lst[ichr].obj_base.req.turn_me_on = btrue;

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ChrList_add_termination( CHR_REF ichr )
{
    egoboo_rv retval = rv_fail;

    if ( !VALID_CHR_RANGE( ichr ) ) return rv_fail;

    if ( chr_termination_count < MAX_CHR )
    {
        chr_termination_list[chr_termination_count] = ichr;
        chr_termination_count++;

        retval = rv_success;
    }

    // at least mark the object as "waiting to be terminated"
    POBJ_REQUEST_TERMINATE( ChrList.lst + ichr );

    return retval;
}

