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

#include "PrtList.h"

#include "char.inl"

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
INLINE PIP_REF ego_prt::get_ipip( const PRT_REF & iprt )
{
    ego_prt * pprt;

    if ( !DEFINED_PRT( iprt ) ) return PIP_REF( MAX_PIP );
    pprt = PrtObjList.get_data_ptr( iprt );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return PIP_REF( MAX_PIP );

    return pprt->pip_ref;
}

//--------------------------------------------------------------------------------------------
INLINE ego_pip * ego_prt::get_ppip( const PRT_REF & iprt )
{
    ego_prt * pprt;

    if ( !DEFINED_PRT( iprt ) ) return NULL;
    pprt = PrtObjList.get_data_ptr( iprt );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return NULL;

    return PipStack + pprt->pip_ref;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_prt::set_size( ego_prt * pprt, int size )
{
    ego_pip *ppip;

    if ( !DEFINED_PPRT( pprt ) ) return bfalse;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack + pprt->pip_ref;

    // set the graphical size
    pprt->size = size;

    // set the bumper size, if available
    if ( 0 == pprt->bump_size_stt )
    {
        // make the particle non-interacting if the initial bumper size was 0
        pprt->bump_real.size   = 0;
        pprt->bump_padded.size = 0;
        pprt->bump_min.size    = 0;
    }
    else
    {
        float real_size  = SFP8_TO_FLOAT( size ) * ego_prt::get_scale( pprt );

        if ( 0.0f == pprt->bump_real.size || 0.0f == size )
        {
            // just set the size, assuming a spherical particle
            pprt->bump_real.size     = real_size;
            pprt->bump_real.size_big = real_size * SQRT_TWO;
            pprt->bump_real.height   = real_size;
        }
        else
        {
            float mag = real_size / pprt->bump_real.size;

            // resize all dimensions equally
            pprt->bump_real.size     *= mag;
            pprt->bump_real.size_big *= mag;
            pprt->bump_real.height   *= mag;
        }

        // make sure that the virtual bumper size is at least as big as what is in the pip file
        pprt->bump_padded.size     = SDL_max( pprt->bump_real.size,     ppip->bump_size );
        pprt->bump_padded.size_big = SDL_max( pprt->bump_real.size_big, ppip->bump_size * SQRT_TWO );
        pprt->bump_padded.height   = SDL_max( pprt->bump_real.height,   ppip->bump_height );

        pprt->bump_min.size        = SDL_min( pprt->bump_real.size,     ppip->bump_size );
        pprt->bump_min.size_big    = SDL_min( pprt->bump_real.size_big, ppip->bump_size * SQRT_TWO );
        pprt->bump_min.height      = SDL_min( pprt->bump_real.height,   ppip->bump_height );
    }

    // use the padded bumper to figure out the prt_cv
    bumper_to_oct_bb_0( pprt->bump_padded, &( pprt->prt_cv ) );
    bumper_to_oct_bb_0( pprt->bump_min,    &( pprt->prt_min_cv ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF ego_prt::get_iowner( const PRT_REF & iprt, int depth )
{
    /// BB@> A helper function for determining the owner of a particle
    ///
    ///      @details There could be a possibility that a particle exists that was spawned by
    ///      another particle, but has lost contact with its original spawner. For instance
    ///      If an explosion particle bounces off of something with MISSILE_DEFLECT or
    ///      MISSILE_REFLECT, which subsequently dies before the particle...
    ///
    ///      That is actually pretty far fetched, but at some point it might make sense to
    ///      spawn particles just keeping track of the spawner (whether particle or character)
    ///      and working backward to any potential owner using this function. ;)
    ///
    ///      @note this function should be completely trivial for anything other than
    ///       namage particles created by an explosion

    CHR_REF iowner = CHR_REF( MAX_CHR );

    ego_prt * pprt;

    // be careful because this can be recursive
    if ( depth > ego_sint(maxparticles) - ego_sint(PrtObjList.free_count()) ) return CHR_REF( MAX_CHR );

    if ( !DEFINED_PRT( iprt ) ) return CHR_REF( MAX_CHR );
    pprt = PrtObjList.get_data_ptr( iprt );

    if ( DEFINED_CHR( pprt->owner_ref ) )
    {
        iowner = pprt->owner_ref;
    }
    else
    {
        // make a check for a stupid looping structure...
        // cannot be sure you could never get a loop, though

        if ( !VALID_PRT( pprt->parent_ref ) )
        {
            // make sure that a non valid parent_ref is marked as non-valid
            pprt->parent_ref = MAX_PRT;
            pprt->parent_guid = 0xFFFFFFFF;
        }
        else
        {
            // if a particle has been poofed, and another particle lives at that address,
            // it is possible that the pprt->parent_ref points to a valid particle that is
            // not the parent. Depending on how scrambled the list gets, there could actually
            // be looping structures. I have actually seen this, so don't laugh :)

            if ( PrtObjList.get_data_ref( pprt->parent_ref ).guid == pprt->parent_guid )
            {
                if ( iprt != pprt->parent_ref )
                {
                    iowner = ego_prt::get_iowner( pprt->parent_ref, depth + 1 );
                }
            }
            else
            {
                // the parent particle doesn't exist anymore
                // fix the reference
                pprt->parent_ref = MAX_PRT;
                pprt->parent_guid = 0xFFFFFFFF;
            }
        }
    }

    return iowner;
}

//--------------------------------------------------------------------------------------------
INLINE float ego_prt::get_scale( ego_prt * pprt )
{
    /// @details BB@> get the scale factor between the "graphical size" of the particle and the actual
    ///               display size

    float scale = 0.25f;

    if ( !DEFINED_PPRT( pprt ) ) return scale;

    // set some particle dependent properties
    switch ( pprt->type )
    {
        case SPRITE_SOLID: scale *= 0.9384f; break;
        case SPRITE_ALPHA: scale *= 0.9353f; break;
        case SPRITE_LIGHT: scale *= 1.5912f; break;
    }

    return scale;
}
