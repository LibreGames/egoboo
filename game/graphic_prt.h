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

#include "egoboo_typedef.h"
#include "egoboo_math.h"

#include "file_formats/pip_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_camera;
struct ego_prt_data;
struct ego_prt_bundle;

//--------------------------------------------------------------------------------------------
// support for computing the particle texture coordinates on the fly.
// currently, there ate two texture types: TX_PARTICLE_TRANS and TX_PARTICLE_LIGHT

extern int   ptex_w[2];
extern int   ptex_h[2];
extern float ptex_wscale[2];
extern float ptex_hscale[2];

#define CALCULATE_PRT_U0(IDX,CNT)  (((.05f+((CNT)&15))/16.0f)*ptex_wscale[IDX])
#define CALCULATE_PRT_U1(IDX,CNT)  (((.95f+((CNT)&15))/16.0f)*ptex_wscale[IDX])
#define CALCULATE_PRT_V0(IDX,CNT)  (((.05f+((CNT)>>4))/16.0f) * ((float)ptex_w[IDX]/(float)ptex_h[IDX])*ptex_hscale[IDX])
#define CALCULATE_PRT_V1(IDX,CNT)  (((.95f+((CNT)>>4))/16.0f) * ((float)ptex_w[IDX]/(float)ptex_h[IDX])*ptex_hscale[IDX])

int prt_get_texture_style( const TX_REF by_reference itex );
void prt_set_texture_params( const TX_REF by_reference itex );

//--------------------------------------------------------------------------------------------
// Particle graphic data
//--------------------------------------------------------------------------------------------

/// All the data necessary to display a partile
struct prt_instance_t
{
    bool_t valid;

    // basic info
    Uint8    type;               ///< particle type
    TX_REF   texture_ref;        ///< which texture
    Uint32   image_ref;          ///< which sub image within the texture?
    float    alpha;              ///< base alpha
    Uint8    light;              ///< base self lighting

    // position info
    fvec3_t   pos;
    float     size;
    float     scale;

    // billboard info
    prt_ori_t orientation;
    fvec3_t   up;
    fvec3_t   right;
    fvec3_t   nrm;

    // lighting info
    float    famb;               ///< cached ambient light
    float    fdir;               ///< cached directional light

    float    fintens;            ///< current brightness
    float    falpha;             ///< current alpha

    // graphical optimizations
    bool_t         indolist;        ///< Has it been added yet?

    // pre-compute some values for the reflected particle posisions
    fvec3_t ref_up;
    fvec3_t ref_right;
    fvec3_t ref_pos;
};

//--------------------------------------------------------------------------------------------
bool_t render_one_prt_solid( const PRT_REF by_reference iprt );
bool_t render_one_prt_trans( const PRT_REF by_reference iprt );
bool_t render_one_prt_ref( const PRT_REF by_reference iprt );

void   render_all_particles( struct ego_camera * pcam );
void   render_all_prt_bbox();
void   render_prt_ref( struct ego_camera * pcam );

void   render_all_prt_attachment();

void prt_instance_update_all( struct ego_camera * pcam );

