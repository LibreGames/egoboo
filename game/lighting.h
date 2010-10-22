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

/// @file lighting.h

#include "physics.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum
{
    LVEC_PX,               ///< light from +x
    LVEC_MX,               ///< light from -x

    LVEC_PY,               ///< light from +y
    LVEC_MY,               ///< light from -y

    LVEC_PZ,               ///< light from +z
    LVEC_MZ,               ///< light from -z

    LVEC_AMB,             ///< light from ambient

    LIGHTING_VEC_SIZE
};

typedef float lighting_vector_t[LIGHTING_VEC_SIZE];

void lighting_vector_evaluate( lighting_vector_t lvec, fvec3_base_t nrm, float * direct, float * amb );
void lighting_vector_sum( lighting_vector_t lvec, fvec3_base_t nrm, float direct, float ambient );

//--------------------------------------------------------------------------------------------
struct ego_lighting_cache_base
{
    float             max_light;  ///< max amplitude of direct light
    float             max_delta;  ///< max change in the light amplitude
    lighting_vector_t lighting;   ///< light from +x,-x, +y,-y, +z,-z, ambient

    ego_lighting_cache_base() { clear( this ); }

    ego_lighting_cache_base * ctor_this( ego_lighting_cache_base * ptr ) { return clear( ptr ); }

private:

    static ego_lighting_cache_base * clear( ego_lighting_cache_base * ptr )
    {
        if ( NULL == ptr ) return ptr;

        memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

ego_lighting_cache_base * lighting_cache_base_init( ego_lighting_cache_base * pdata );
bool_t                  lighting_cache_base_max_light( ego_lighting_cache_base * cache );
bool_t                  lighting_cache_base_blend( ego_lighting_cache_base * cache, ego_lighting_cache_base * cnew, float keep );

//--------------------------------------------------------------------------------------------
struct ego_lighting_cache
{
    float                 max_light;              ///< max amplitude of direct light
    float                 max_delta;              ///< max change in amplitude of all light

    ego_lighting_cache_base low;
    ego_lighting_cache_base hgh;

    ego_lighting_cache() { max_light = max_light = 0.0f; }
};

ego_lighting_cache * lighting_cache_init( ego_lighting_cache * pdata );
bool_t             lighting_cache_max_light( ego_lighting_cache * cache );
bool_t             lighting_cache_blend( ego_lighting_cache * cache, ego_lighting_cache * cnew, float keep );
//--------------------------------------------------------------------------------------------
#define MAXDYNADIST                     2700        // Leeway for offscreen lights
#define MAX_DYNA                    64          // Absolute max number of dynamic lights

/// A definition of a single in-game dynamic light
struct ego_dynalight
{
    float   distance;      ///< The distance from the center of the camera view
    fvec3_t pos;           ///< Light position
    float   level;         ///< Light intensity
    float   falloff;       ///< Light radius
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern float  light_a, light_d, light_nrm[3];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t lighting_project_cache( ego_lighting_cache * dst, ego_lighting_cache * src, fmat_4x4_t mat );
bool_t lighting_cache_interpolate( ego_lighting_cache * dst, ego_lighting_cache * src[], float u, float v );
float lighting_cache_test( ego_lighting_cache * src[], float u, float v, float * low_max_diff, float * hgh_max_diff );

float lighting_evaluate_cache( ego_lighting_cache * src, fvec3_base_t nrm, float z, ego_aabb bbox, float * light_amb, float * light_dir );

bool_t sum_dyna_lighting( ego_dynalight * pdyna, lighting_vector_t lighting, fvec3_base_t nrm );
