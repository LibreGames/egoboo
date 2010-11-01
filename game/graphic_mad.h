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

struct ego_MD2_Model;

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
enum e_matrix_data_type
{
    MAT_UNKNOWN   = 0,
    MAT_CHARACTER = ( 1 << 0 ),
    MAT_WEAPON    = ( 1 << 1 )
};

typedef enum e_matrix_data_type matrix_data_type_t;

/// the data necessary to create the matrix for a mad instance
struct gfx_mad_matrix_data
{
    friend struct ego_chr;

    // is the cache data valid?
    bool_t valid;

    // is the matrix data valid?
    bool_t matrix_valid;

    // how was the matrix made?
    BIT_FIELD type_bits;

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

    gfx_mad_matrix_data() { init( this ); }

    static gfx_mad_matrix_data * init( gfx_mad_matrix_data * mcache );

    static bool_t download( gfx_mad_matrix_data & mc_tmp, ego_chr * pchr );
    static bool_t generate_character_matrix( gfx_mad_matrix_data & mcache, ego_chr * pchr );
    static bool_t generate_weapon_matrix( gfx_mad_matrix_data & mcache, ego_chr * pweap );
    static bool_t generate_matrix( gfx_mad_matrix_data & mc_tmp, ego_chr * pchr );

};

// definition that is consistent with using it as a callback in SDL_qsort() or some similar function
int  cmp_matrix_data( const void * vlhs, const void * vrhs );

//--------------------------------------------------------------------------------------------

/// some pre-computed parameters for reflection
struct gfx_chr_reflection_info
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

    gfx_chr_reflection_info() { clear( this ); }

    static gfx_chr_reflection_info * init( gfx_chr_reflection_info * pcache );

private:
    static gfx_chr_reflection_info * clear( gfx_chr_reflection_info * ptr )
    {
        if ( NULL == ptr ) return ptr;

        SDL_memset( ptr, 0, sizeof( *ptr ) );

        ptr->alpha = 127;
        ptr->light = 255;

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------

struct gfx_range
{
    int      vmin, vmax;

    gfx_range( int mi = -1, int ma = -2 ) { init(); vmin = mi; vmax = ma; }

    void init()
    {
        // make a invalid range
        vmin = -1;
        vmax = vmin - 1;
    }

    // accessors so that inheritors of this struct can pick out its data
    gfx_range &  get_range()       { return *this; }
    const gfx_range & cget_range() const { return *this; }

    // is the range valid?
    bool is_null() const { return vmax < vmin; }

    // are two ranges "identical"?
    friend bool operator == ( const gfx_range & lhs, const gfx_range & rhs )
    {
        bool match = ( lhs.vmin == rhs.vmin ) && ( lhs.vmax == rhs.vmax );

        if ( !match )
        {
            // all null ranges match all other null ranges
            if ( lhs.is_null() && rhs.is_null() )
            {
                match = true;
            }
        }

        return match;
    }

    static bool_t rhs_inside_lhs( const gfx_range & lhs, const gfx_range & rhs )
    {
        if ( lhs.is_null() || rhs.is_null() ) return bfalse;

        return ( rhs.vmin <= lhs.vmax ) && ( rhs.vmin >= lhs.vmin ) &&
               ( rhs.vmax <= lhs.vmax ) && ( rhs.vmax >= lhs.vmin );
    }

    static gfx_range intersection( const gfx_range & lhs, const gfx_range & rhs )
    {
        gfx_range rv;

        if ( lhs.is_null() || rhs.is_null() ) return rv;

        rv.vmin = std::max( lhs.vmin, rhs.vmin );
        rv.vmax = std::min( lhs.vmax, rhs.vmax );

        return rv;
    }

};

/// the data to determine whether re-alculation of vlst is necessary
struct gfx_vlst_range : public gfx_range
{
    bool_t   valid;             ///< has the range been declared invalid?
    unsigned pose_id;           ///< the id value of the pose that this range was last calculated for
    unsigned frame_wld;         ///< the last FRAME  where the vertices were calculated
    unsigned update_wld;        ///< the last UPDATE where the vertices were calculated

    gfx_vlst_range() { init(); };

    void init()
    {
        valid = bfalse;
        pose_id = unsigned( -1 );
        gfx_range::init();
    };

    friend bool operator == ( const gfx_vlst_range & lhs, const gfx_vlst_range & rhs )
    {
        return ( lhs.pose_id == rhs.pose_id ) && ( lhs.cget_range() == rhs.cget_range() );
    }

};

//--------------------------------------------------------------------------------------------

/// All the data that the renderer needs to draw the character
struct gfx_mad_instance
{
    friend struct ego_chr_data;
    friend struct ego_chr;

    // model info
    ego_MD2_Model * md2_ptr;

    // position info
    fmat_4x4_t       matrix;           ///< Character's matrix
    gfx_mad_matrix_data mcache;     ///< Did we make one yet?

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

    // lighting info
    Sint32         color_amb;
    fvec4_t        col_amb;
    int            max_light, min_light;
    unsigned       lighting_update_wld;            ///< update some lighting info no more than once an update
    unsigned       lighting_frame_all;             ///< update some lighting info no more than once a frame

    // linear interpolated frame vertices
    size_t         vrt_count;
    ego_GLvertex * vrt_lst;
    ego_oct_bb     bbox;                           ///< the bounding box for this frame

    // graphical optimizations
    bool_t                   indolist;               ///< Has it been added yet?
    pose_data                pose;                   ///< what is the state
    gfx_vlst_range           vrange;                 ///< The range of currently valid vertices
    gfx_chr_reflection_info  ref;                    ///< pre-computing some reflection parameters

    // OBSOLETE
    // lighting
    // FACING_T       light_turn_z;    ///< Character's light rotation 0 to 0xFFFF
    // Uint8          lightlevel_amb;  ///< 0-255, terrain light
    // Uint8          lightlevel_dir;  ///< 0-255, terrain light

    gfx_mad_instance() { clear( this ); }
    ~gfx_mad_instance() { dtor_this( this ); }

    static egoboo_rv update_bbox( gfx_mad_instance * pinst );
    static egoboo_rv needs_update( gfx_mad_instance * pgfx_inst, const pose_data & p_new, const gfx_range & r_new, bool_t *verts_match, bool_t *frames_match );
    static egoboo_rv update_vertices( gfx_mad_instance * pgfx_inst, const pose_data & p_new, const gfx_range & r_new, const bool_t force = bfalse );
    static egoboo_rv update_grip_verts( gfx_mad_instance * pinst, Uint16 vrt_lst[], const size_t vrt_count );

    static void      get_tint( gfx_mad_instance * pinst, GLfloat tint[], Uint32 bits );

    static void      clear_cache( gfx_mad_instance * pinst );

    static egoboo_rv  test_pose( const pose_data & p_old, const pose_data & p_new );
    static egoboo_rv  test_vertices( const gfx_range & v_old, const gfx_range & r_new );

    static egoboo_rv validate_vertices( gfx_mad_instance * pinst, const gfx_range & r_new );
    static int       validate_pose( gfx_mad_instance * pinst, const mad_instance * pmad_inst );

protected:
    static gfx_mad_instance * ctor_this( gfx_mad_instance * pinst );
    static gfx_mad_instance * dtor_this( gfx_mad_instance * pinst );
    static bool_t             alloc( gfx_mad_instance * pinst, size_t vlst_size );
    static bool_t             dealloc( gfx_mad_instance * pinst );
    static bool_t             spawn( gfx_mad_instance * pinst, const PRO_REF & profile, Uint8 skin );
    static bool_t             set_mad( gfx_mad_instance * pinst, const MAD_REF & imad );

    static void               update_lighting_base( gfx_mad_instance * pinst, ego_chr * pchr, bool_t force );

    static egoboo_rv          update_vlst_cache( gfx_mad_instance * pinst, const pose_data & p_new, const gfx_range & r_new, bool_t force, bool_t vertices_match, bool_t frames_match );
    static bool_t             update_ref( gfx_mad_instance * pinst, float grid_level, bool_t need_matrix );

private:
    static gfx_mad_instance * clear( gfx_mad_instance * ptr );
};

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_enviro( const CHR_REF & character, GLXvector4f tint, Uint32 bits );
bool_t render_one_mad_tex( const CHR_REF & character, GLXvector4f tint, Uint32 bits );
bool_t render_one_mad( const CHR_REF & character, GLXvector4f tint, Uint32 bits );
bool_t render_one_mad_ref( const CHR_REF & tnc );

void      update_all_chr_instance();
