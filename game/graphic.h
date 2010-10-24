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
#include "graphic_defs.h"
#include "egoboo_setup.h"

#include "mesh.h"
#include "mad.h"
#include "font_ttf.h"

#include "extensions/ogl_texture.h"
#include "file_formats/module_file.h"

#include "egoboo.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_chr;
struct ego_camera;
struct config_data;
struct s_chr_instance;
struct s_oglx_texture_parameters;
struct s_display_item;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// An element of the do-list, an all encompassing list of all objects to be drawn by the renderer
struct ego_do_list_data
{
    float   dist;
    CHR_REF chr;
};

//--------------------------------------------------------------------------------------------

/// Structure for sorting both particles and characters based on their position from the camera
struct ego_obj_registry_entity
{
    CHR_REF ichr;
    PRT_REF iprt;
    float   dist;
};

int obj_registry_entity_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// OPENGL VERTEX
struct ego_GLvertex
{
    GLXvector4f pos;
    GLXvector3f nrm;
    GLXvector2f env;

    GLXvector2f tex;
    GLXvector4f col_dir;
    GLint       color_dir;   ///< the vertex-dependent, directional lighting

    GLXvector4f col;      ///< the total vertex-dependent lighting (ambient + directional)

    ego_GLvertex() { clear( this ); }

    ego_GLvertex * ctor_this( ego_GLvertex * ptr ) { return clear( ptr ); }

private:

    static ego_GLvertex * clear( ego_GLvertex * ptr )
    {
        if ( NULL == ptr ) return NULL;

        memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Which tiles are to be drawn, arranged by MPDFX_* bits
struct ego_renderlist
{
    ego_mpd   * pmesh;

    int     all_count;                               ///< Number to render, total
    int     ref_count;                               ///< ..., is reflected in the floor
    int     sha_count;                               ///< ..., is not reflected in the floor
    int     drf_count;                               ///< ..., draws character reflections
    int     ndr_count;                               ///< ..., draws no character reflections
    int     wat_count;                               ///< ..., draws no character reflections

    Uint32  all[MAXMESHRENDER];                      ///< List of which to render, total

    Uint32  ref[MAXMESHRENDER];                      ///< ..., is reflected in the floor
    Uint32  sha[MAXMESHRENDER];                      ///< ..., is not reflected in the floor

    Uint32  drf[MAXMESHRENDER];                      ///< ..., draws character reflections
    Uint32  ndr[MAXMESHRENDER];                      ///< ..., draws no character reflections

    Uint32  wat[MAXMESHRENDER];                      ///< ..., draws a water tile
};

extern ego_renderlist renderlist;

//--------------------------------------------------------------------------------------------
//extern Uint8           lightdirectionlookup[65536];                        ///< For lighting characters
//extern float           lighttable_local[MAXLIGHTROTATION][EGO_NORMAL_COUNT];
//extern float           lighttable_global[MAXLIGHTROTATION][EGO_NORMAL_COUNT];
extern float           indextoenvirox[EGO_NORMAL_COUNT];                    ///< Environment map
extern float           lighttoenviroy[256];                                ///< Environment map
//extern Uint32          lighttospek[MAXSPEKLEVEL][256];

//--------------------------------------------------------------------------------------------
// Display messages
extern int    msgtimechange;

/// A display messages
struct ego_msg
{
    int             time;                            ///< The time for this message
    char            textdisplay[MESSAGESIZE];        ///< The displayed text
};

DECLARE_STATIC_ARY_TYPE( DisplayMsgAry, ego_msg, MAX_MESSAGE );

DECLARE_EXTERN_STATIC_ARY( DisplayMsgAry, DisplayMsg )

//--------------------------------------------------------------------------------------------
// camera optimization

// For figuring out what to draw
#define CAM_ROTMESH_TOPSIDE                  50
#define CAM_ROTMESH_BOTTOMSIDE               50
#define CAM_ROTMESH_UP                       30
#define CAM_ROTMESH_DOWN                     30

// The ones that get used
extern int rotmeshtopside;
extern int rotmeshbottomside;
extern int rotmeshup;
extern int rotmeshdown;

//--------------------------------------------------------------------------------------------
// encapsulation of all graphics options

struct ego_gfx_config : public s_gfx_config_data
{
    static bool_t init( ego_gfx_config * pgfx );
};

extern ego_gfx_config gfx;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern ego_obj_registry_entity dolist[DOLIST_SIZE];             ///< List of which characters to draw
extern size_t                dolist_count;                  ///< How many in the list

// Minimap stuff
#define MAXBLIP        128                          ///< Max blips on the screen
extern Uint8           mapon;
extern Uint8           mapvalid;
extern Uint8           youarehereon;

extern size_t          blip_count;
extern float           blip_x[MAXBLIP];
extern float           blip_y[MAXBLIP];
extern Uint8           blip_c[MAXBLIP];

#define BILLBOARD_COUNT     (2 * MAX_CHR)
#define INVALID_BILLBOARD   BILLBOARD_COUNT

enum e_bb_opt
{
    bb_opt_none          = EMPTY_BIT_FIELD,
    bb_opt_randomize_pos = ( 1 << 0 ),      // Randomize the position of the bb to witin 1 grid
    bb_opt_randomize_vel = ( 1 << 1 ),      // Randomize the velocity of the bb. Enough to move it by 2 tiles within its lifetime.
    bb_opt_fade          = ( 1 << 2 ),      // Make the billboard fade out
    bb_opt_burn          = ( 1 << 3 ),      // Make the tint fully saturate over time.
    bb_opt_all           = FULL_BIT_FIELD    //(size_t)(~0)   Enum doesn't support unsigned integers, size_t is also unsigned.
};

/// Description of a generic bilboarded object.
/// Any graphics that can be composited onto a SDL_surface can be used

struct ego_billboard_data
{
    bool_t      valid;        ///< has the billboard data been initialized?

    Uint32      time;         ///< the time when the billboard will expire
    TX_REF      tex_ref;      ///< our texture index
    fvec3_t     pos;          ///< the position of the bottom-missile of the box

    CHR_REF     ichr;         ///< the character we are attached to

    GLXvector4f tint;         ///< a color to modulate the billboard's r,g,b, and a channels
    GLXvector4f tint_add;     ///< the change in tint per update

    GLXvector4f offset;       ///< an offset to the billboard's position in world coordinates
    GLXvector4f offset_add;   ///<

    float       size;         ///< the size of the billboard
    float       size_add;     ///< the size change per update

    static ego_billboard_data * init( ego_billboard_data * pbb );
    static bool_t               dealloc( ego_billboard_data * pbb );
    static bool_t               update( ego_billboard_data * pbb );
    static bool_t               printf_ttf( ego_billboard_data * pbb, TTF_Font *font, SDL_Color color, const char * format, ... );

private:

    static ego_billboard_data * clear( ego_billboard_data * pbb );
};

typedef ego_billboard_data ego_billboard_data_t;

extern t_cpp_list< ego_billboard_data, BILLBOARD_COUNT  > BillboardList;

void               BillboardList_init_all();
void               BillboardList_update_all();
void               BillboardList_free_all();
size_t             BillboardList_get_free( Uint32 lifetime_secs );
bool_t             BillboardList_free_one( size_t ibb );
ego_billboard_data * BillboardList_get_ptr( const BBOARD_REF &  ibb );

#define VALID_BILLBOARD_RANGE( IBB ) ( ( (IBB) >= 0 ) && ( (IBB) < BILLBOARD_COUNT ) )
#define VALID_BILLBOARD( IBB )       ( VALID_BILLBOARD_RANGE( IBB ) && BillboardList.lst[IBB].valid )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// some lines to be drawn in the display

#define LINE_COUNT 100

struct ego_line_data
{
    fvec3_t   dst;
    fvec4_t   src, color;
    int time;
};

extern ego_line_data line_list[LINE_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern float time_render_scene_init;
extern float time_render_scene_mesh;
extern float time_render_scene_solid;
extern float time_render_scene_water;
extern float time_render_scene_trans;

extern float time_render_scene_init_renderlist_make;
extern float time_render_scene_init_dolist_make;
extern float time_render_scene_init_do_grid_dynalight;
extern float time_render_scene_init_light_fans;
extern float time_render_scene_init_update_all_chr_instance;
extern float time_render_scene_init_update_all_prt_instance;

extern float time_render_scene_mesh_dolist_sort;
extern float time_render_scene_mesh_ndr;
extern float time_render_scene_mesh_drf_back;
extern float time_render_scene_mesh_ref;
extern float time_render_scene_mesh_ref_chr;
extern float time_render_scene_mesh_drf_solid;
extern float time_render_scene_mesh_render_shadows;

extern int GFX_WIDTH;
extern int GFX_HEIGHT;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes

void   gfx_system_begin();
void   gfx_system_end();

int    ogl_init();
void   gfx_main();
void   gfx_begin_3d( struct ego_camera * pcam );
void   gfx_end_3d();
void   gfx_reload_all_textures();

void   request_clear_screen();
void   do_clear_screen();
bool_t flip_pages_requested();
void   request_flip_pages();
void   do_flip_pages();

void   dolist_sort( struct ego_camera * pcam, bool_t do_reflect );
void   dolist_make( ego_mpd   * pmesh );
bool_t dolist_add_chr( ego_mpd   * pmesh, const CHR_REF & ichr );
bool_t dolist_add_prt( ego_mpd   * pmesh, const PRT_REF & iprt );

void draw_one_icon( const TX_REF & icontype, float x, float y, Uint8 sparkle );
void draw_one_font( int fonttype, float x, float y );
void draw_map_texture( float x, float y );
int  draw_one_bar( Uint8 bartype, float x, float y, int ticks, int maxticks );
int  draw_string( float x, float y, const char *format, ... );
int  draw_wrap_string( const char *szText, float x, float y, int maxx );
int  draw_status( const CHR_REF & character, float x, float y );
void draw_text();
void draw_one_character_icon( const CHR_REF & item, float x, float y, bool_t draw_ammo );
void draw_blip( float sizeFactor, Uint8 color, float x, float y, bool_t mini_map );
void draw_all_lines( struct ego_camera * pcam );

void   render_world( struct ego_camera * pcam );
void   render_shadow( const CHR_REF & character );
void   render_bad_shadow( const CHR_REF & character );
void   render_scene( ego_mpd   * pmesh, struct ego_camera * pcam );
bool_t render_oct_bb( ego_oct_bb   * bb, bool_t draw_square, bool_t draw_diamond );
bool_t render_aabb( ego_aabb * pbbox );
void   render_all_billboards( struct ego_camera * pcam );

void   make_enviro();
void   clear_messages();
bool_t dump_screenshot();
void   make_lightdirectionlookup();

int  DisplayMsg_get_free();

int debug_printf( const char *format, ... );

void renderlist_reset();
void renderlist_make( ego_mpd   * pmesh, struct ego_camera * pcam );

bool_t grid_lighting_interpolate( ego_mpd   * pmesh, ego_lighting_cache * dst, float fx, float fy );
float  grid_lighting_test( ego_mpd   * pmesh, GLXvector3f pos, float * low_diff, float * hgh_diff );

int  get_free_line();

void init_all_graphics();
void release_all_graphics();
void delete_all_graphics();

void release_all_profile_textures();

void   load_graphics();
bool_t load_cursor();
bool_t load_blips();
void   load_bars();
void   load_map();
bool_t load_all_global_icons();
void   load_basic_textures( );

float  get_ambient_level();
