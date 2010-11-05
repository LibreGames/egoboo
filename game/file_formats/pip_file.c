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

/// \file file_formats/pip_file.c
/// \brief Routines for reading and writing the particle profile file "part*.txt"
/// \details

#include "pip_file.h"

#include "../sound_defs.h"

#include "../egoboo_vfs.h"
#include "../egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
particle_direction_t prt_direction[256] =
{
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_l, prt_v, prt_v, prt_v, prt_v, prt_l, prt_l, prt_l, prt_r, prt_r, prt_r, prt_r, prt_r,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_l, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_l, prt_l, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_l, prt_l, prt_l, prt_l,
    prt_u, prt_u, prt_u, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_l, prt_u, prt_u, prt_u, prt_u,
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_u, prt_u, prt_u, prt_u
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
pip_data_t * pip_init( pip_data_t * ppip )
{
    if ( NULL == ppip ) return ppip;

    // clear the pip
    SDL_memset( ppip, 0, sizeof( *ppip ) );

    ppip->end_sound          = INVALID_SOUND;
    ppip->end_sound_floor = INVALID_SOUND;
    ppip->end_sound_wall  = INVALID_SOUND;
    ppip->damfx           = DAMFX_TURN;

    ppip->allowpush = btrue;

    ppip->orientation = ORIENTATION_B;  // make the orientation the normal billboarded orientation
    ppip->type        = SPRITE_SOLID;

    return ppip;
}

//--------------------------------------------------------------------------------------------
pip_data_t * load_one_pip_file_vfs( const char *szLoadName, pip_data_t * ppip )
{
    /// \author ZZ
    /// \details  This function loads a particle template, returning bfalse if the file wasn't
    ///    found

    vfs_FILE* fileread;
    IDSZ idsz;
    char cTmp;

    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread )
    {
        return NULL;
    }

    pip_init( ppip );

    // set up the EGO_PROFILE_STUFF
    strncpy( ppip->name, szLoadName, SDL_arraysize( ppip->name ) );
    ppip->loaded = btrue;

    // read the 1 line comment at the top of the file
    vfs_gets( ppip->comment, SDL_arraysize( ppip->comment ) - 1, fileread );

    // rewind the file
    vfs_seek( fileread, 0 );

    // General data
    ppip->force = fget_next_bool( fileread );

    cTmp = fget_next_char( fileread );
    if ( 'L' == SDL_toupper( cTmp ) )  ppip->type = SPRITE_LIGHT;
    else if ( 'S' == SDL_toupper( cTmp ) )  ppip->type = SPRITE_SOLID;
    else if ( 'T' == SDL_toupper( cTmp ) )  ppip->type = SPRITE_ALPHA;

    ppip->image_base = fget_next_int( fileread );
    ppip->numframes = fget_next_int( fileread );
    ppip->image_add.base = fget_next_int( fileread );
    ppip->image_add.rand = fget_next_int( fileread );
    ppip->rotate_pair.base = fget_next_int( fileread );
    ppip->rotate_pair.rand = fget_next_int( fileread );
    ppip->rotate_add = fget_next_int( fileread );
    ppip->size_base = fget_next_int( fileread );
    ppip->size_add = fget_next_int( fileread );
    ppip->spdlimit = fget_next_float( fileread );
    ppip->facingadd = fget_next_int( fileread );

    // override the base rotation
    if ( ppip->image_base < 256 && prt_u != prt_direction[ ppip->image_base ] )
    {
        ppip->rotate_pair.base = prt_direction[ ppip->image_base ];
    };

    // Ending conditions
    ppip->end_water     = fget_next_bool( fileread );
    ppip->end_bump      = fget_next_bool( fileread );
    ppip->end_ground    = fget_next_bool( fileread );
    ppip->end_lastframe = fget_next_bool( fileread );
    ppip->time         = fget_next_int( fileread );

    // Collision data
    ppip->dampen     = fget_next_float( fileread );
    ppip->bump_money  = fget_next_int( fileread );
    ppip->bump_size   = fget_next_int( fileread );
    ppip->bump_height = fget_next_int( fileread );

    fget_next_range( fileread, &( ppip->damage ) );
    ppip->damagetype = fget_next_damage_type( fileread );

    // Lighting data
    cTmp = fget_next_char( fileread );
    if ( 'T' == SDL_toupper( cTmp ) ) ppip->dynalight.mode = DYNA_MODE_ON;
    else if ( 'L' == SDL_toupper( cTmp ) ) ppip->dynalight.mode = DYNA_MODE_LOCAL;
    else                             ppip->dynalight.mode = DYNA_MODE_OFF;

    ppip->dynalight.level   = fget_next_float( fileread );
    ppip->dynalight.falloff = fget_next_int( fileread );

    // Initial spawning of this particle
    ppip->facing_pair.base    = fget_next_int( fileread );
    ppip->facing_pair.rand    = fget_next_int( fileread );
    ppip->spacing_hrz_pair.base = fget_next_int( fileread );
    ppip->spacing_hrz_pair.rand = fget_next_int( fileread );
    ppip->spacing_vrt_pair.base  = fget_next_int( fileread );
    ppip->spacing_vrt_pair.rand  = fget_next_int( fileread );
    ppip->vel_hrz_pair.base     = fget_next_int( fileread );
    ppip->vel_hrz_pair.rand     = fget_next_int( fileread );
    ppip->vel_vrt_pair.base      = fget_next_int( fileread );
    ppip->vel_vrt_pair.rand      = fget_next_int( fileread );

    // Continuous spawning of other particles
    ppip->contspawn_delay      = fget_next_int( fileread );
    ppip->contspawn_amount    = fget_next_int( fileread );
    ppip->contspawn_facingadd = fget_next_int( fileread );
    ppip->contspawn_pip       = fget_next_int( fileread );

    // End spawning of other particles
    ppip->end_spawn_amount    = fget_next_int( fileread );
    ppip->end_spawn_facingadd = fget_next_int( fileread );
    ppip->end_spawn_pip       = fget_next_int( fileread );

    // Bump spawning of attached particles
    ppip->bumpspawn_amount = fget_next_int( fileread );
    ppip->bumpspawn_pip    = fget_next_int( fileread );

    // Random stuff  !!!BAD!!! Not complete
    ppip->dazetime     = fget_next_int( fileread );
    ppip->grogtime     = fget_next_int( fileread );
    ppip->spawnenchant = fget_next_bool( fileread );

    goto_colon( NULL, fileread, bfalse );  // !!Cause roll

    ppip->causepancake = fget_next_bool( fileread );

    ppip->needtarget         = fget_next_bool( fileread );
    ppip->targetcaster       = fget_next_bool( fileread );
    ppip->startontarget      = fget_next_bool( fileread );
    ppip->onlydamagefriendly = fget_next_bool( fileread );

    ppip->soundspawn = fget_next_int( fileread );

    ppip->end_sound = fget_next_int( fileread );

    ppip->friendlyfire = fget_next_bool( fileread );

    ppip->hateonly = fget_next_bool( fileread );

    ppip->newtargetonspawn = fget_next_bool( fileread );

    ppip->targetangle = fget_next_int( fileread ) >> 1;
    ppip->homing      = fget_next_bool( fileread );

    ppip->homingfriction = fget_next_float( fileread );
    ppip->homingaccel    = fget_next_float( fileread );
    ppip->rotatetoface   = fget_next_bool( fileread );

    goto_colon( NULL, fileread, bfalse );  // !!Respawn on hit is unused

    ppip->manadrain         = fget_next_int( fileread ) * 256;
    ppip->lifedrain         = fget_next_int( fileread ) * 256;

    // assume default end_wall
    ppip->end_wall = ppip->end_ground;

    // assume default damfx
    if ( ppip->homing )  ppip->damfx = DAMFX_NONE;

    // Read expansions
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        idsz = fget_idsz( fileread );

        /// \note ZF@> Does this line doesn't do anything?
        /// \note BB@> Should it be "ppip->damfx = DAMFX_NONE"?
        if ( MAKE_IDSZ( 'T', 'U', 'R', 'N' ) == idsz )       ADD_BITS( ppip->damfx, DAMFX_NONE );

        else if ( MAKE_IDSZ( 'A', 'R', 'M', 'O' ) == idsz )  ADD_BITS( ppip->damfx, DAMFX_ARMO );
        else if ( MAKE_IDSZ( 'B', 'L', 'O', 'C' ) == idsz )  ADD_BITS( ppip->damfx, DAMFX_NBLOC );
        else if ( MAKE_IDSZ( 'A', 'R', 'R', 'O' ) == idsz )  ADD_BITS( ppip->damfx, DAMFX_ARRO );
        else if ( MAKE_IDSZ( 'T', 'I', 'M', 'E' ) == idsz )  ADD_BITS( ppip->damfx, DAMFX_TIME );
        else if ( MAKE_IDSZ( 'Z', 'S', 'P', 'D' ) == idsz )  ppip->zaimspd = fget_int( fileread );
        else if ( MAKE_IDSZ( 'F', 'S', 'N', 'D' ) == idsz )  ppip->end_sound_floor = fget_int( fileread );
        else if ( MAKE_IDSZ( 'W', 'S', 'N', 'D' ) == idsz )  ppip->end_sound_wall = fget_int( fileread );
        else if ( MAKE_IDSZ( 'W', 'E', 'N', 'D' ) == idsz )  ppip->end_wall = ( 0 != fget_int( fileread ) );
        else if ( MAKE_IDSZ( 'P', 'U', 'S', 'H' ) == idsz )  ppip->allowpush = ( 0 != fget_int( fileread ) );
        else if ( MAKE_IDSZ( 'D', 'L', 'E', 'V' ) == idsz )  ppip->dynalight.level_add = fget_int( fileread ) / 1000.0f;
        else if ( MAKE_IDSZ( 'D', 'R', 'A', 'D' ) == idsz )  ppip->dynalight.falloff_add = fget_int( fileread ) / 1000.0f;
        else if ( MAKE_IDSZ( 'I', 'D', 'A', 'M' ) == idsz )  ppip->intdamagebonus = ( 0 != fget_int( fileread ) );
        else if ( MAKE_IDSZ( 'W', 'D', 'A', 'M' ) == idsz )  ppip->wisdamagebonus = ( 0 != fget_int( fileread ) );
        else if ( MAKE_IDSZ( 'O', 'R', 'N', 'T' ) == idsz )
        {
            char ornt_val = fget_first_letter( fileread );
            switch ( SDL_toupper( ornt_val ) )
            {
                case 'X': ppip->orientation = ORIENTATION_X; break;  // put particle up along the world or body-fixed x-axis
                case 'Y': ppip->orientation = ORIENTATION_Y; break;  // put particle up along the world or body-fixed y-axis
                case 'Z': ppip->orientation = ORIENTATION_Z; break;  // put particle up along the world or body-fixed z-axis
                case 'V': ppip->orientation = ORIENTATION_V; break;  // vertical, like a candle
                case 'H': ppip->orientation = ORIENTATION_H; break;  // horizontal, like a plate
                case 'B': ppip->orientation = ORIENTATION_B; break;  // billboard
            }
        }
    }

    vfs_close( fileread );

    return ppip;
}
