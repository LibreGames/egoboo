//---------------------------------------------------------------------
// ModBaker - a module editor for Egoboo
//---------------------------------------------------------------------
// Copyright (C) 2009 Tobias Gall
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------

/// @file
/// @brief Renderer definition
/// @details Definition of the rendering system

#ifndef graphic_h
#define graphic_h

//---------------------------------------------------------------------
//-
//-   Everything related to OpenGL, SDL or rendering goes in here
//-
//---------------------------------------------------------------------

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "window.h"
#include "general.h"
#include "core/object.h"
#include "SDL_extensions.h"
#include "ogl_extensions.h"

#include <string>

using namespace std;


//---------------------------------------------------------------------
///   \def IMG_TILE0
///        relative filename for tile0.bmp
///   \def IMG_TILE1
///        relative filename for tile1.bmp
///   \def IMG_TILE2
///        relative filename for tile2.bmp
///   \def IMG_TILE3
///        relative filename for tile3.bmp
//---------------------------------------------------------------------
#define IMG_TILE0    "[EGOBOO_PATH]/modules/[MODULENAME]/basicdat/tile0.bmp"
#define IMG_TILE1    "[EGOBOO_PATH]/modules/[MODULENAME]/basicdat/tile1.bmp"
#define IMG_TILE2    "[EGOBOO_PATH]/modules/[MODULENAME]/basicdat/tile2.bmp"
#define IMG_TILE3    "[EGOBOO_PATH]/modules/[MODULENAME]/basicdat/tile3.bmp"
#define IMG_WATERTOP "[EGOBOO_PATH]/modules/[MODULENAME]/basicdat/watertop.bmp"
#define IMG_WATERLOW "[EGOBOO_PATH]/modules/[MODULENAME]/basicdat/waterlow.bmp"
// Fallback textures
#define IMG_TILE0_F    "data/tiles/tile0.bmp"
#define IMG_TILE1_F    "data/tiles/tile1.bmp"
#define IMG_TILE2_F    "data/tiles/tile2.bmp"
#define IMG_TILE3_F    "data/tiles/tile3.bmp"
#define IMG_WATERTOP_F "data/tiles/watertop.bmp"
#define IMG_WATERLOW_F "data/tiles/waterlow.bmp"


//---------------------------------------------------------------------
///   \enum Texture types (used for giving crenderer.texture index names)
//---------------------------------------------------------------------
enum
{
	TX_PARTICLE  = 0, ///< particles
	TX_TILE_0    = 1, ///< tile0.bmp
	TX_TILE_1    = 2, ///< tile1.bmp
	TX_TILE_2    = 3, ///< tile2.bmp
	TX_TILE_3    = 4, ///< tile3.bmp
	TX_WATER_TOP = 5, ///< overlay
	TX_WATER_LOW = 6  ///< the water image
};


//---------------------------------------------------------------------
///   \enum Mesh render modes
//---------------------------------------------------------------------
enum
{
	RENDER_NORMAL,     ///< standard render mode
	RENDER_TILE_FLAGS, ///< render the tile flags
	RENDER_TILE_TYPES  ///< render the tile types (unused)
};


//---------------------------------------------------------------------
///   Definitions for the renderer
///   \def SCREEN_BPP
///        video depth
///   \def MAX_TEXTURES
///        maximum number of tileset images
///   \def SCROLLFACTOR_FAST
///        factor for fast scrolling
///   \def SCROLLFACTOR_SLOW
///        factor for slow scrolling
///   \def DEG_TO_RAD
///        Camera factor
///   \def RAD_TO_DEG
///        Camera factor
///   \def FOV
///        Camera field of view
///   \def INVALID_TILE
///        Don't draw the fansquare if tile = this
///   \def MAXMESHRENDER
///        Maximum number of tiles to draw
//---------------------------------------------------------------------
#define SCREEN_BPP       24
#define MAX_TEXTURES      8
#define SCROLLFACTOR_FAST 200.0f
#define SCROLLFACTOR_SLOW 100.0f
// Camera stuff
#define DEG_TO_RAD       0.017453292519943295769236907684886f
#define RAD_TO_DEG       57.295779513082320876798154814105
#define FOV              40
#define INVALID_TILE      ((Uint16)(~(Uint16)0))
#define MAXMESHRENDER    (1024*8)

struct Graphics_t;


//---------------------------------------------------------------------
///   \struct GLVertex Definition of an OpenGL vertex
//---------------------------------------------------------------------
struct GLVertex
{
	vect4 pos;     ///< Position
	vect4 col;
	Uint32 color;  ///< should replace r,g,b,a and be called by glColor4ubv

	vect2 tx;      ///< u and v in D3D I guess
	vect3 nrm;
	vect3 up;      ///< Up
	vect3 rt;      ///< Right
};


//---------------------------------------------------------------------
///   Definition of the camera
//---------------------------------------------------------------------
class c_camera
{
	private:
		void _cam_frustum_jitter_fov(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
		void make_matrix();

		GLmatrix make_view_matrix(const vect3, const vect3, const vect3, const float);
		GLmatrix make_projection_matrix(const float, const float, const float);

	public:
		vect3 m_pos;      ///< The camera position
		vect3 m_trackpos; ///< The camera target

		float m_movex;    ///< for x axis movement
		float m_movey;    ///< for y axis movement
		float m_movez;    ///< for z axis movement

		float m_roll;     ///< Camera roll angle
		float m_factor;   ///< scrolling factor

		GLmatrix m_projection;
		GLmatrix m_projection_big;
		GLmatrix m_view;

		void move();
		void reset();

		c_camera();
		~c_camera();
};


//---------------------------------------------------------------------
///   Definition of the renderlist
//---------------------------------------------------------------------
class c_renderlist
{
	private:
	public:
		int     m_num_totl;                 ///< Number to render, total
		Uint32  m_totl[MAXMESHRENDER];      ///< List of which to render, total

		int     m_num_shine;                ///< ..., reflective
		Uint32  m_shine[MAXMESHRENDER];     ///< ..., reflective

		int     m_num_reflc;                ///< ..., has reflection
		Uint32  m_reflc[MAXMESHRENDER];     ///< ..., has reflection

		int     m_num_norm;                 ///< ..., not reflective, has no reflection
		Uint32  m_norm[MAXMESHRENDER];      ///< ..., not reflective, has no reflection

		int     m_num_watr;                 ///< ..., water
		Uint32  m_watr[MAXMESHRENDER];      ///< ..., water
		Uint32  m_watr_mode[MAXMESHRENDER]; ///< the water mode

		bool build();
};


//---------------------------------------------------------------------
///   Definition of the rendering system
//---------------------------------------------------------------------
class c_renderer
{
		enum TMODE {JLEFT, JRIGHT};

		friend class c_camera;

		SDL_Surface *m_screen;

	protected:
		GLuint m_texture[MAX_TEXTURES]; ///< Used to store all OpenGL textures

		void initSDL();
		void initGL();

		void render_fan(Uint32, bool set_texture = true);
		void render_tile_flag(Uint16);
		bool load_texture(string, int);

		GLfloat           m_fps;  ///< Number of frames per second (FPS)
		TTF_Font*         m_font; ///< SDL TTF object (currently unused)
		c_camera*         m_cam;  ///< Camera object
		c_window_manager* m_wm;   ///< Window manager object

		int render_mode; ///< The current render mode

	public:
		c_renderer();
		~c_renderer();

		c_window_manager* get_wm();
		void set_wm(c_window_manager*);

		SDL_Surface *get_screen();
		void set_screen(SDL_Surface *);

		// Load an object icon
		bool load_icon(string, c_object*);

		// Window stuff
		void resize_window(int, int);
		bool render_positions();
		void begin_frame();
		void end_frame();

		// 2D / 3D modes
		void begin_3D_mode();
		void begin_2D_mode();
		void end_3D_mode();
		void end_2D_mode();

		// Rendering containers
		bool render_models(c_object_manager*);

		// Mesh render mode
		int get_render_mode();
		void set_render_mode(int);

		void load_basic_textures(string);
		bool render_mesh();

		///   Get the camera object
		///   \return camera object
		c_camera * getPCam() { return m_cam; }

		c_renderlist m_renderlist; ///< the render list
};
#endif
