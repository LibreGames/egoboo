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
        pprt = PrtObjList.get_data_ptr( iprt );

        // does it have a valid profile?
        if ( !LOADED_PIP( pprt->pip_ref ) )
        {
            found = iprt;
            free_one_particle_in_game( iprt );
            break;
        }

        // do not bump another
        was_forced = ( PipStack[pprt->pip_ref].force );

        if ( WAITING_PRT( iprt ) )
        {
            // if the particle has been "terminated" but is still waiting around, bump it to the
            // front of the list

            size_t min_time  = SDL_min( pprt->lifetime_remaining, pprt->frames_remaining );

            if ( min_time < SDL_max( min_life, min_display ) )
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

    if ( PrtObjList.valid_index_range( found.get_value() ) )
    {
        // found a "bad" particle
        iprt = found;
    }
    else if ( PrtObjList.valid_index_range( min_display_idx.get_value() ) )
    {
        // found a "terminated" particle
        iprt = min_display_idx;
    }
    else if ( PrtObjList.valid_index_range( min_life_idx.get_value() ) )
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
    if ( PrtObjList.valid_index_range( iprt.get_value() ) )
    {
        // if the particle is already being used, make sure to destroy the old one
        if ( DEFINED_PRT( iprt ) )
        {
            free_one_particle_in_game( iprt );
        }

        // allocate the new one
        list_type::allocate_container( PrtObjList.get_ref( iprt ), iprt.get_value() );
    }

    if ( ALLOCATED_PRT( iprt ) )
    {
        // construct the new structure
        ego_object_engine::run_construct( PrtObjList.get_data_ptr( iprt ), 100 );
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
PRT_REF ego_particle_list::allocate( bool_t force, const PRT_REF & override )
{
    /// @details ZZ@> This function gets an unused particle.  If all particles are in use
    ///    and force is set, it grabs the first unimportant one.  The iprt
    ///    index is the return value

    ego_sint fake_free_count;
    ego_uint real_free_count = 0;

    // Return MAX_PRT if we can't find one
    PRT_REF iprt( MAX_PRT );

    // we might need to tread carefully here, since the "allocated" size of the PrtObjList could
    // change on-demand and wind up negative
    fake_free_count = PrtObjList.free_count();
    real_free_count = fake_free_count <= 0 ? 0 : fake_free_count;

    // do we need to override the base function?
    bool_t needs_search = ( force && 0 == real_free_count );

    if ( needs_search )
    {
        // search for a particle that needs to die and recycle it
        printf( "!!!!TEH SUXXORZ!!!!\n" );
        iprt = allocate_find();
        iprt = allocate_activate( iprt );
    }
    else if ( force || ( real_free_count > get_size() / 4 ) )
    {
        // just use the allocate function from the parent class
        iprt = t_obj_lst_deque<ego_obj_prt, MAX_PRT>::allocate( override );
    }

    return iprt;
}

