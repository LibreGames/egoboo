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

/// @file PrtObjList.c
/// @brief Implementation of the PrtList_* functions
/// @details

#include "PrtList.h"
#include "egoboo_setup.h"

//#include "particle.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PrtObjList_t PrtObjList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PRT_REF ego_particle_list::allocate_find()
{
    PRT_REF iprt( MAX_PRT );

    PRT_REF found           = PRT_REF( MAX_PRT );
    size_t  min_life        = ( size_t )( ~0 );
    PRT_REF min_life_idx    = PRT_REF( MAX_PRT );
    size_t  min_display     = ( size_t )( ~0 );
    PRT_REF min_display_idx = PRT_REF( MAX_PRT );

    // Gotta find one, so go through the list and replace a unimportant one
    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
        bool_t was_forced = bfalse;
        ego_prt * pprt;

        // Is this an invalid particle? The particle allocation count is messed up! :(
        if ( !DEFINED_PRT( iprt ) )
        {
            found = iprt;
            break;
        }
        pprt = PrtObjList.get_pdata( iprt );

        // does it have a valid profile?
        if ( !LOADED_PIP( pprt->pip_ref ) )
        {
            found = iprt;
            free_one_particle_in_game( iprt );
            break;
        }

        // do not bump another
        was_forced = ( PipStack.lst[pprt->pip_ref].force );

        if ( WAITING_PRT( iprt ) )
        {
            // if the particle has been "terminated" but is still waiting around, bump it to the
            // front of the list

            size_t min_time  = MIN( pprt->lifetime_remaining, pprt->frames_remaining );

            if ( min_time < MAX( min_life, min_display ) )
            {
                min_life     = pprt->lifetime_remaining;
                min_life_idx = iprt;

                min_display     = pprt->frames_remaining;
                min_display_idx = iprt;
            }
        }
        else if ( !was_forced )
        {
            // if the particle has not yet died, let choose the worst one

            if ( pprt->lifetime_remaining < min_life )
            {
                min_life     = pprt->lifetime_remaining;
                min_life_idx = iprt;
            }

            if ( pprt->frames_remaining < min_display )
            {
                min_display     = pprt->frames_remaining;
                min_display_idx = iprt;
            }
        }
    }

    if ( VALID_PRT_REF( found ) )
    {
        // found a "bad" particle
        iprt = found;
    }
    else if ( VALID_PRT_REF( min_display_idx ) )
    {
        // found a "terminated" particle
        iprt = min_display_idx;
    }
    else if ( VALID_PRT_REF( min_life_idx ) )
    {
        // found a particle that closest to death
        iprt = min_life_idx;
    }
    else
    {
        // found nothing. this should only happen if all the
        // particles are forced
        iprt = PRT_REF( MAX_PRT );
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
PRT_REF ego_particle_list::allocate_activate( const PRT_REF & iprt )
{
    if ( VALID_PRT_REF( iprt ) )
    {
        // if the particle is already being used, make sure to destroy the old one
        if ( DEFINED_PRT( iprt ) )
        {
            free_one_particle_in_game( iprt );
        }

        // allocate the new one
        POBJ_ALLOCATE( PrtObjList.get_ptr( iprt ), ( iprt ).get_value() );
    }

    if ( ALLOCATED_PRT( iprt ) )
    {
        // construct the new structure
        ego_obj_prt::run_construct( PrtObjList.get_ptr( iprt ), 100 );
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
PRT_REF ego_particle_list::allocate( bool_t force, const PRT_REF & override )
{
    /// @details ZZ@> This function gets an unused particle.  If all particles are in use
    ///    and force is set, it grabs the first unimportant one.  The iprt
    ///    index is the return value

    // Return MAX_PRT if we can't find one
    PRT_REF iprt( MAX_PRT );

    // do we need to override the base function?
    bool_t needs_search = ( force && 0 == PrtObjList.free_count() );

    if ( needs_search )
    {
        iprt = allocate_find();
        iprt = allocate_activate( iprt );
    }
    else if ( force || PrtObjList.free_count() > ( maxparticles / 4 ) )
    {
        iprt = t_ego_obj_lst<ego_obj_prt, MAX_PRT>::allocate( override );
    }

    return iprt;
}

////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//
//t_cpp_list< ego_prt, MAX_PRT  > PrtObjList;
//
//static size_t  prt_termination_count = 0;
//static PRT_REF prt_termination_list[MAX_PRT];
//
//static size_t  prt_activation_count = 0;
//static PRT_REF prt_activation_list[MAX_PRT];
//
////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//
//int PrtObjList.loop_depth = 0;
//
////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//
//static size_t  PrtList_get_free();
//
//static bool_t PrtList_remove_used( const PRT_REF & iprt );
//static bool_t PrtList_remove_used_index( int index );
//static bool_t PrtList_add_free( const PRT_REF & iprt );
//static bool_t PrtList_remove_free( const PRT_REF & iprt );
//static bool_t PrtList_remove_free_index( int index );
//
//static size_t  PrtList_get_free();
//
////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//void PrtList_init()
//{
//    int cnt;
//
//    // fix any problems with maxparticles
//    if ( maxparticles > MAX_PRT ) maxparticles = MAX_PRT;
//
//    // free all the particles
//    PrtObjList.free_count() = 0;
//    PrtObjList.used_count = 0;
//    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
//    {
//        PrtObjList.free_ref[cnt] = MAX_PRT;
//        PrtObjList.used_ref[cnt] = MAX_PRT;
//    }
//
//    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
//    {
//        PRT_REF iprt = ( PRT_REF )(( MAX_PRT - 1 ) - cnt );
//        ego_prt * pprt = PrtObjList.get_valid_pdata(iprt);
//
//        // blank out all the data, including the obj_base data
//        memset( pprt, 0, sizeof( *pprt ) );
//
//        // particle "initializer"
//        ego_obj::ctor( PDATA_GET_PBASE( pprt ), iprt );
//
//        PrtList_add_free( iprt );
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void PrtList_dtor()
//{
//    PRT_REF cnt;
//
//    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
//    {
//        ego_obj_prt::run_deconstruct( PrtObjList.get_valid_pdata(cnt), 100 );
//    }
//
//    PrtObjList.free_count() = 0;
//    PrtObjList.used_count = 0;
//    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
//    {
//        PrtObjList.free_ref[cnt] = MAX_PRT;
//        PrtObjList.used_ref[cnt] = MAX_PRT;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void PrtList_prune_used()
//{
//    // prune the used list
//
//    size_t  cnt;
//    PRT_REF iprt;
//
//    for ( cnt = 0; cnt < PrtObjList.used_count; cnt++ )
//    {
//        bool_t removed = bfalse;
//
//        iprt = PrtObjList.used_ref[cnt];
//
//        if ( !VALID_PRT_REF( iprt ) || !DEFINED_PRT( iprt ) )
//        {
//            removed = PrtList_remove_used_index( cnt );
//        }
//
//        if ( removed && !PrtObjList.get_data(iprt).get_proc().in_free_list )
//        {
//            PrtList_add_free( iprt );
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void PrtList_prune_free()
//{
//    // prune the free list
//
//    size_t  cnt;
//    PRT_REF iprt;
//
//    for ( cnt = 0; cnt < PrtObjList.free_count(); cnt++ )
//    {
//        bool_t removed = bfalse;
//
//        iprt = PrtObjList.free_ref[cnt];
//
//        if ( VALID_PRT_REF( iprt ) && INGAME_PRT_BASE( iprt ) )
//        {
//            removed = PrtList_remove_free_index( cnt );
//        }
//
//        if ( removed && !PrtObjList.get_data(iprt).get_proc().in_free_list )
//        {
//            PrtList_add_used( iprt );
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void PrtList_update_used()
//{
//    PRT_REF iprt;
//
//    PrtList_prune_used();
//    PrtList_prune_free();
//
//    // go through the particle list to see if there are any dangling particles
//    for ( iprt = 0; iprt < MAX_PRT; iprt++ )
//    {
//        if ( !ALLOCATED_PRT( iprt ) ) continue;
//
//        if ( INGAME_PRT_BASE( iprt ) )
//        {
//            if ( !PrtObjList.get_data(iprt).get_proc().in_used_list )
//            {
//                PrtList_add_used( iprt );
//            }
//        }
//        else if ( !DEFINED_PRT( iprt ) )
//        {
//            if ( !PrtObjList.get_data(iprt).get_proc().in_free_list )
//            {
//                PrtList_add_free( iprt );
//            }
//        }
//    }
//
//    // blank out the unused elements of the used list
//    for ( iprt = PrtObjList.used_count; iprt < MAX_PRT; iprt++ )
//    {
//        PrtObjList.used_ref[iprt] = MAX_PRT;
//    }
//
//    // blank out the unused elements of the free list
//    for ( iprt = PrtObjList.free_count(); iprt < MAX_PRT; iprt++ )
//    {
//        PrtObjList.free_ref[iprt] = MAX_PRT;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//bool_t PrtList_free_one( const PRT_REF & iprt )
//{
//    /// @details ZZ@> This function sticks a particle back on the free particle stack
//    ///
//    /// @note Tying ALLOCATED_PRT() and POBJ_TERMINATE() to PrtList_free_one()
//    /// should be enough to ensure that no particle is freed more than once
//
//    bool_t retval;
//    ego_prt * pprt;
//    ego_obj * pbase;
//
//    if ( !ALLOCATED_PRT( iprt ) ) return bfalse;
//    pprt = PrtObjList.get_valid_pdata(iprt);
//
//    pbase = PDATA_GET_PBASE( pprt );
//    if ( NULL == pbase ) return bfalse;
//
//    // if we are inside a PrtObjList loop, do not actually change the length of the
//    // list. This will cause some problems later.
//    if ( PrtObjList.loop_depth > 0 )
//    {
//        retval = PrtList_add_termination( iprt );
//
//        // invalidate the egoboo_object_state, so that module will
//        // ignore this object until we can actually remove the object
//        // from the list
//        if ( FLAG_VALID_PBASE( pbase ) )
//        {
//            ego_obj_proc::invalidate( pbase->get_pproc() );
//        }
//    }
//    else
//    {
//        // was the egoboo_object_state invalidated before being completely killed?
//        if ( !FLAG_KILLED_PBASE( pbase ) )
//        {
//            // turn it on again so that we can call the ego_obj_prt::run_*() functions
//            // properly
//            pbase->get_proc().valid = btrue;
//        }
//
//        // deallocate any dynamically allocated memory
//        pprt = ego_obj_prt::run_deinitialize( pprt, 100 );
//        if ( NULL == pprt ) return bfalse;
//
//        if ( pbase->in_used_list() )
//        {
//            PrtList_remove_used( iprt );
//        }
//
//        if ( pbase->in_free_list() )
//        {
//            retval = btrue;
//        }
//        else
//        {
//            retval = PrtList_add_free( iprt );
//        }
//
//        // particle "destructor"
//        pprt = ego_obj_prt::run_deconstruct( pprt, 100 );
//        if ( NULL == pprt ) return bfalse;
//
//        // let everyone know that that the object is completely gone
//        ego_obj::invalidate( pbase );
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//size_t PrtList_get_free()
//{
//    /// @details ZZ@> This function returns the next free particle or MAX_PRT if there are none
//
//    size_t retval = MAX_PRT;
//
//    if ( PrtObjList.free_count() > 0 )
//    {
//        PrtObjList.free_count()--;
//        PrtObjList.update_guid()++;
//
//        retval = PrtObjList.free_ref[PrtObjList.free_count()];
//
//        // completely remove it from the free list
//        PrtObjList.free_ref[PrtObjList.free_count()] = MAX_PRT;
//
//        if ( VALID_PRT_REF( retval ) )
//        {
//            // let the object know it is not in the free list any more
//            cpp_list_state::set_free( &( PrtObjList.get_data(retval).get_proc() ), bfalse );
//        }
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//PRT_REF PrtList_allocate( bool_t force )
//{
//    /// @details ZZ@> This function gets an unused particle.  If all particles are in use
//    ///    and force is set, it grabs the first unimportant one.  The iprt
//    ///    index is the return value
//
//    PRT_REF iprt;
//
//    // Return MAX_PRT if we can't find one
//    iprt = PRT_REF(MAX_PRT);
//
//    if ( 0 == PrtObjList.free_count() )
//    {
//        if ( force )
//        {
//            PRT_REF found           = PRT_REF(MAX_PRT);
//            size_t  min_life        = ( size_t )( ~0 );
//            PRT_REF min_life_idx    = PRT_REF(MAX_PRT);
//            size_t  min_display     = ( size_t )( ~0 );
//            PRT_REF min_display_idx = PRT_REF(MAX_PRT);
//
//            // Gotta find one, so go through the list and replace a unimportant one
//            for ( iprt = 0; iprt < maxparticles; iprt++ )
//            {
//                bool_t was_forced = bfalse;
//                ego_prt * pprt;
//
//                // Is this an invalid particle? The particle allocation count is messed up! :(
//                if ( !DEFINED_PRT( iprt ) )
//                {
//                    found = iprt;
//                    break;
//                }
//                pprt =  PrtObjList.get_valid_pdata( iprt);
//
//                // does it have a valid profile?
//                if ( !LOADED_PIP( pprt->pip_ref ) )
//                {
//                    found = iprt;
//                    free_one_particle_in_game( iprt );
//                    break;
//                }
//
//                // do not bump another
//                was_forced = ( PipStack.lst[pprt->pip_ref].force );
//
//                if ( WAITING_PRT( iprt ) )
//                {
//                    // if the particle has been "terminated" but is still waiting around, bump it to the
//                    // front of the list
//
//                    size_t min_time  = MIN( pprt->lifetime_remaining, pprt->frames_remaining );
//
//                    if ( min_time < MAX( min_life, min_display ) )
//                    {
//                        min_life     = pprt->lifetime_remaining;
//                        min_life_idx = iprt;
//
//                        min_display     = pprt->frames_remaining;
//                        min_display_idx = iprt;
//                    }
//                }
//                else if ( !was_forced )
//                {
//                    // if the particle has not yet died, let choose the worst one
//
//                    if ( pprt->lifetime_remaining < min_life )
//                    {
//                        min_life     = pprt->lifetime_remaining;
//                        min_life_idx = iprt;
//                    }
//
//                    if ( pprt->frames_remaining < min_display )
//                    {
//                        min_display     = pprt->frames_remaining;
//                        min_display_idx = iprt;
//                    }
//                }
//            }
//
//            if ( VALID_PRT_REF( found ) )
//            {
//                // found a "bad" particle
//                iprt = found;
//            }
//            else if ( VALID_PRT_REF( min_display_idx ) )
//            {
//                // found a "terminated" particle
//                iprt = min_display_idx;
//            }
//            else if ( VALID_PRT_REF( min_life_idx ) )
//            {
//                // found a particle that closest to death
//                iprt = min_life_idx;
//            }
//            else
//            {
//                // found nothing. this should only happen if all the
//                // particles are forced
//                iprt = PRT_REF(MAX_PRT);
//            }
//        }
//    }
//    else
//    {
//        if ( PrtObjList.free_count() > ( maxparticles / 4 ) )
//        {
//            // Just grab the next one
//            iprt = PrtList_get_free();
//        }
//        else if ( force )
//        {
//            iprt = PrtList_get_free();
//        }
//    }
//
//    // return a proper value
//    iprt = ( iprt >= maxparticles ) ? PRT_REF(MAX_PRT) : iprt;
//
//    if ( VALID_PRT_REF( iprt ) )
//    {
//        // if the particle is already being used, make sure to destroy the old one
//        if ( DEFINED_PRT( iprt ) )
//        {
//            free_one_particle_in_game( iprt );
//        }
//
//        // allocate the new one
//        POBJ_ALLOCATE( PrtObjList.get_valid_pdata(iprt), iprt );
//    }
//
//    if ( ALLOCATED_PRT( iprt ) )
//    {
//        // construct the new structure
//        ego_obj_prt::run_construct( PrtObjList.get_valid_pdata( iprt), 100 );
//    }
//
//    return iprt;
//}
//
////--------------------------------------------------------------------------------------------
//void PrtList_free_all()
//{
//    /// @details ZZ@> This function resets the particle allocation lists
//
//    PRT_REF cnt;
//
//    // free all the particles
//    for ( cnt = 0; cnt < maxparticles; cnt++ )
//    {
//        PrtList_free_one( cnt );
//    }
//}
//
////--------------------------------------------------------------------------------------------
//int PrtList_get_free_list_index( const PRT_REF & iprt )
//{
//    int    retval = -1;
//    size_t cnt;
//
//    if ( !VALID_PRT_REF( iprt ) ) return retval;
//
//    for ( cnt = 0; cnt < PrtObjList.free_count(); cnt++ )
//    {
//        if ( iprt == PrtObjList.free_ref[cnt] )
//        {
//            EGOBOO_ASSERT( PrtObjList.get_data(iprt).get_proc().in_free_list );
//            retval = cnt;
//            break;
//        }
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t PrtList_add_free( const PRT_REF & iprt )
//{
//    bool_t retval;
//
//    if ( !VALID_PRT_REF( iprt ) ) return bfalse;
//
//#if EGO_DEBUG && defined(DEBUG_PRT_LIST)
//    if ( PrtList_get_free_list_index( iprt ) > 0 )
//    {
//        return bfalse;
//    }
//#endif
//
//    EGOBOO_ASSERT( !PrtObjList.get_data(iprt).get_proc().in_free_list );
//
//    retval = bfalse;
//    if ( PrtObjList.free_count() < maxparticles )
//    {
//        PrtObjList.free_ref[PrtObjList.free_count()] = iprt;
//
//        PrtObjList.free_count()++;
//        PrtObjList.update_guid()++;
//
//        cpp_list_state::set_free( &( PrtObjList.get_data(iprt).get_proc() ), btrue );
//
//        retval = btrue;
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t PrtList_remove_free_index( int index )
//{
//    PRT_REF iprt;
//
//    // was it found?
//    if ( index < 0 || ( size_t )index >= PrtObjList.free_count() ) return bfalse;
//
//    iprt = PrtObjList.free_ref[index];
//
//    // blank out the index in the list
//    PrtObjList.free_ref[index] = MAX_PRT;
//
//    if ( VALID_PRT_REF( iprt ) )
//    {
//        // let the object know it is not in the list anymore
//        cpp_list_state::set_free( &( PrtObjList.get_data(iprt).get_proc() ), bfalse );
//    }
//
//    // shorten the list
//    PrtObjList.free_count()--;
//    PrtObjList.update_guid()++;
//
//    if ( PrtObjList.free_count() > 0 )
//    {
//        // swap the last element for the deleted element
//        SWAP( size_t, PrtObjList.free_ref[index], PrtObjList.free_ref[PrtObjList.free_count()] );
//    }
//
//    return btrue;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t PrtList_remove_free( const PRT_REF & iprt )
//{
//    // find the object in the free list
//    int index = PrtList_get_free_list_index( iprt );
//
//    return PrtList_remove_free_index( index );
//}
//
////--------------------------------------------------------------------------------------------
//int PrtList_get_used_list_index( const PRT_REF & iprt )
//{
//    int retval = -1, cnt;
//
//    if ( !VALID_PRT_REF( iprt ) ) return retval;
//
//    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
//    {
//        if ( iprt == PrtObjList.used_ref[cnt] )
//        {
//            EGOBOO_ASSERT( PrtObjList.get_data(iprt).get_proc().in_used_list );
//            retval = cnt;
//            break;
//        }
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t PrtList_add_used( const PRT_REF & iprt )
//{
//    bool_t retval;
//
//    if ( !VALID_PRT_REF( iprt ) ) return bfalse;
//
//#if EGO_DEBUG && defined(DEBUG_PRT_LIST)
//    if ( PrtList_get_used_list_index( iprt ) > 0 )
//    {
//        return bfalse;
//    }
//#endif
//
//    EGOBOO_ASSERT( !PrtObjList.get_data(iprt).get_proc().in_used_list );
//
//    retval = bfalse;
//    if ( PrtObjList.used_count < maxparticles )
//    {
//        PrtObjList.used_ref[PrtObjList.used_count] = iprt;
//
//        PrtObjList.used_count++;
//        PrtObjList.update_guid()++;
//
//        cpp_list_state::set_used( &( PrtObjList.get_data(iprt).get_proc() ), btrue );
//
//        retval = btrue;
//    }
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t PrtList_remove_used_index( int index )
//{
//    PRT_REF iprt;
//
//    // was it found?
//    if ( index < 0 || ( size_t )index >= PrtObjList.used_count ) return bfalse;
//
//    iprt = PrtObjList.used_ref[index];
//
//    // blank out the index in the list
//    PrtObjList.used_ref[index] = MAX_PRT;
//
//    if ( VALID_PRT_REF( iprt ) )
//    {
//        // let the object know it is not in the list anymore
//        cpp_list_state::set_used( &( PrtObjList.get_data(iprt).get_proc() ), bfalse );
//    }
//
//    // shorten the list
//    PrtObjList.used_count--;
//    PrtObjList.update_guid()++;
//
//    if ( PrtObjList.used_count > 0 )
//    {
//        // swap the last element for the deleted element
//        SWAP( size_t, PrtObjList.used_ref[index], PrtObjList.used_ref[PrtObjList.used_count] );
//    }
//
//    return btrue;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t PrtList_remove_used( const PRT_REF & iprt )
//{
//    // find the object in the used list
//    int index = PrtList_get_used_list_index( iprt );
//
//    return PrtList_remove_used_index( index );
//}
//
////--------------------------------------------------------------------------------------------
//void PrtList_cleanup()
//{
//    size_t  cnt;
//    ego_prt * pprt;
//
//    // go through the list and activate all the particles that
//    // were created while the list was iterating
//    for ( cnt = 0; cnt < prt_activation_count; cnt++ )
//    {
//        PRT_REF iprt = prt_activation_list[cnt];
//
//        if ( !ALLOCATED_PRT( iprt ) ) continue;
//        pprt = PrtObjList.get_valid_pdata(iprt);
//
//        ego_obj::grant_on( PDATA_GET_PBASE( pprt ) );
//    }
//    prt_activation_count = 0;
//
//    // go through and delete any particles that were
//    // supposed to be deleted while the list was iterating
//    for ( cnt = 0; cnt < prt_termination_count; cnt++ )
//    {
//        PrtList_free_one( prt_termination_list[cnt] );
//    }
//    prt_termination_count = 0;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t PrtList_add_activation( PRT_REF iprt )
//{
//    // put this particle into the activation list so that it can be activated right after
//    // the PrtObjList loop is completed
//
//    bool_t retval = bfalse;
//
//    if ( !VALID_PRT_REF( iprt ) ) return bfalse;
//
//    if ( prt_activation_count < MAX_PRT )
//    {
//        prt_activation_list[prt_activation_count] = iprt;
//        prt_activation_count++;
//
//        retval = btrue;
//    }
//
//    PrtObjList.get_obj(iprt).proc_req_on( btrue );
//
//    return retval;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t PrtList_add_termination( PRT_REF iprt )
//{
//    bool_t retval = bfalse;
//
//    if ( !VALID_PRT_REF( iprt ) ) return bfalse;
//
//    if ( prt_termination_count < MAX_PRT )
//    {
//        prt_termination_list[prt_termination_count] = iprt;
//        prt_termination_count++;
//
//        retval = btrue;
//    }
//
//    // at least mark the object as "waiting to be terminated"
//    POBJ_REQUEST_TERMINATE( PrtObjList.get_valid_pdata(iprt ));
//
//    return retval;
//}
//
