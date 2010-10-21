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
#include "graphic.h"

#include "file_formats/cap_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Bits used to request a character tint
enum e_chr_render_bits
{
    CHR_UNKNOWN  = 0,
    CHR_SOLID    = ( 1 << 0 ),
    CHR_ALPHA    = ( 1 << 1 ),
    CHR_LIGHT    = ( 1 << 2 ),
    CHR_PHONG    = ( 1 << 3 ),
    CHR_REFLECT  = ( 1 << 4 )
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Bits that tell you which variables to look at
enum e_matrix_cache_type
{
    MAT_UNKNOWN   = 0,
    MAT_CHARACTER = ( 1 << 0 ),
    MAT_WEAPON    = ( 1 << 1 )
};

typedef enum e_matrix_cache_type matrix_cache_type_t;

/// the data necessary to cache the last values required to create the character matrix
struct ego_matrix_cache
{
    friend struct ego_chr;

    // is the cache data valid?
    bool_t valid;

    // is the matrix data valid?
    bool_t matrix_valid;

    // how was the matrix made?
    int type_bits;

    //---- MAT_CHARACTER data

    // the "Euler" rotation angles in 16-bit form
    fvec3_t   rotate;

    // the translate vector
    fvec3_t   pos;

    //---- MAT_WEAPON data

    CHR_REF grip_chr;                   ///< !=MAX_CHR if character is a held weapon
    slot_t  grip_slot;                  ///< SLOT_LEFT or SLOT_RIGHT
    Uint16  grip_verts[GRIP_VERTS];     ///< Vertices which describe the weapon grip
    fvec3_t grip_scale;

    //---- data used for both

    // the body fixed scaling
    fvec3_t  self_scale;

    static ego_matrix_cache * init( ego_matrix_cache * mcache );
};


//--------------------------------------------------------------------------------------------

/// some pre-computed parameters for reflection
struct ego_chr_reflection_cache
{
    fmat_4x4_t matrix;
    bool_t     matrix_valid;
    Uint8      alpha;
    Uint8      light;
    Uint8      sheen;
    Uint8      redshift;
    Uint8      grnshift;
    Uint8      blushift;

    Uint32     update_wld;

    ego_chr_reflection_cache() { clear(this); }

    static ego_chr_reflection_cache * init( ego_chr_reflection_cache * pcache );

private:
    static ego_chr_reflection_cache * clear( ego_chr_reflection_cache * ptr )
    {
        if( NULL == ptr ) return ptr;

        memset( ptr, 0, sizeof(*ptr) );

        ptr->alpha = 127;
        ptr->light = 255;

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------

/// the data to determine whether re-calculation of vlst is necessary
struct ego_vlst_cache
{
    bool_t valid;             ///< do we know whether this cache is valid or not?

    float  flip;              ///< the in-betweening  the last time the animation was updated
    Uint16 frame_nxt;         ///< the initial frame  the last time the animation was updated
    Uint16 frame_lst;         ///< the final frame  the last time the animation was updated
    Uint32 frame_wld;         ///< the update_wld the last time the animation was updated

    int    vmin;              ///< the minimum clean vertex the last time the vertices were updated
    int    vmax;              ///< the maximum clean vertex the last time the vertices were updated
    Uint32 vert_wld;          ///< the update_wld the last time the vertices were updated
};

ego_vlst_cache * vlst_cache_init( ego_vlst_cache * );

//--------------------------------------------------------------------------------------------

/// All the data that the renderer needs to draw the character
struct ego_chr_instance
{
    friend struct ego_chr_data;
    friend struct ego_chr;

    // position info
    fmat_4x4_t     matrix;           ///< Character's matrix
    ego_matrix_cache matrix_cache;     ///< Did we make one yet?

    FACING_T       facing_z;

    // render mode info
    Uint8          alpha;           ///< 255 = Solid, 0 = Invisible
    Uint8          light;           ///< 1 = Light, 0 = Normal
    Uint8          sheen;           ///< 0-15, how shiny it is
    bool_t         enviro;          ///< Environment map?

    // color info
    Uint8          redshift;        ///< Color channel shifting
    Uint8          grnshift;
    Uint8          blushift;

    // texture info
    TX_REF         texture;         ///< The texture id of the character's skin
    SFP8_T         uoffset;         ///< For moving textures
    SFP8_T         voffset;

    // model info
    MAD_REF        imad;            ///< Character's model

    // animation info
    Uint16         frame_nxt;       ///< Character's frame
    Uint16         frame_lst;       ///< Character's last frame
    Uint8          ilip;            ///< Character's frame inbetweening
    float          flip;            ///< Character's frame inbetweening
    float          rate;

    // action info
    Uint8          action_ready;                   ///< Ready to play a new one
    Uint8          action_which;                   ///< Character's action
    bool_t         action_keep;                    ///< Keep the action playing
    bool_t         action_loop;                    ///< Loop it too
    Uint8          action_next;                    ///< Character's action to play next

    // lighting info
    Sint32         color_amb;
    fvec4_t        col_amb;
    int            max_light, min_light;
    unsigned       lighting_update_wld;            ///< update some lighting info no more than once an update
    unsigned       lighting_frame_all;             ///< update some lighting info no more than once a frame

    // linear interpolated frame vertices
    size_t         vrt_count;
    ego_GLvertex * vrt_lst;
    ego_oct_bb         bbox;                           ///< the bounding box for this frame

    // graphical optimizations
    bool_t                 indolist;               ///< Has it been added yet?
    ego_vlst_cache           save;                   ///< Do we need to re-calculate all or part of the vertex list
    ego_chr_reflection_cache ref;                    ///< pre-computing some reflection parameters

    // OBSOLETE
    // lighting
    // FACING_T       light_turn_z;    ///< Character's light rotation 0 to 0xFFFF
    // Uint8          lightlevel_amb;  ///< 0-255, terrain light
    // Uint8          lightlevel_dir;  ///< 0-255, terrain light

    ego_chr_instance() { clear(this); }
    ~ego_chr_instance() { dtor(this); }

    static egoboo_rv update_bbox( ego_chr_instance * pinst );
    static egoboo_rv needs_update( ego_chr_instance * pinst, int vmin, int vmax, bool_t *verts_match, bool_t *frames_match );
    static egoboo_rv update_vertices( ego_chr_instance * pinst, int vmin, int vmax, bool_t force );
    static egoboo_rv update_grip_verts( ego_chr_instance * pinst, Uint16 vrt_lst[], size_t vrt_count );

    static egoboo_rv set_action( ego_chr_instance * pinst, int action, bool_t action_ready, bool_t override_action );
    static egoboo_rv set_frame( ego_chr_instance * pinst, int frame );
    static egoboo_rv start_anim( ego_chr_instance * pinst, int action, bool_t action_ready, bool_t override_action );
    static egoboo_rv set_anim( ego_chr_instance * pinst, int action, int frame, bool_t action_ready, bool_t override_action );

    static egoboo_rv increment_action( ego_chr_instance * pinst );
    static egoboo_rv increment_frame( ego_chr_instance * pinst, ego_mad * pmad, const CHR_REF & imount );
    static egoboo_rv play_action( ego_chr_instance * pinst, int action, bool_t actionready );

    static void      get_tint( ego_chr_instance * pinst, GLfloat * tint, Uint32 bits );

    static void      clear_cache( ego_chr_instance * pinst );

protected:
    static ego_chr_instance * ctor( ego_chr_instance * pinst );
    static ego_chr_instance * dtor( ego_chr_instance * pinst );
    static bool_t             alloc( ego_chr_instance * pinst, size_t vlst_size );
    static bool_t             dealloc( ego_chr_instance * pinst );
    static bool_t             spawn( ego_chr_instance * pinst, const PRO_REF & profile, Uint8 skin );
    static bool_t             set_mad( ego_chr_instance * pinst, const MAD_REF & imad );

    static void               update_lighting_base( ego_chr_instance * pinst, ego_chr * pchr, bool_t force );

    static egoboo_rv          update_vlst_cache( ego_chr_instance * pinst, int vmax, int vmin, bool_t force, bool_t vertices_match, bool_t frames_match );
    static bool_t             update_ref( ego_chr_instance * pinst, float grid_level, bool_t need_matrix );


private:
    static ego_chr_instance * clear( ego_chr_instance * ptr );
};

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_enviro( const CHR_REF & character, GLXvector4f tint, Uint32 bits );
bool_t render_one_mad_tex( const CHR_REF & character, GLXvector4f tint, Uint32 bits );
bool_t render_one_mad( const CHR_REF & character, GLXvector4f tint, Uint32 bits );
bool_t render_one_mad_ref( const CHR_REF & tnc );

void      update_all_chr_instance();
