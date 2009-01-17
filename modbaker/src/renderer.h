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

#include "general.h"

#include <string>

using namespace std;

//---------------------------------------------------------------------
//-   Texture types (used for giving crenderer.texture index names)
//---------------------------------------------------------------------
enum
{
	TX_PARTICLE  = 0,
	TX_TILE_0    = 1,
	TX_TILE_1    = 2,
	TX_TILE_2    = 3,
	TX_TILE_3    = 4,
	TX_WATER_TOP = 5,
	TX_WATER_LOW = 6
};


//---------------------------------------------------------------------
//-   Definitions for the renderer
//---------------------------------------------------------------------
#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   480
#define SCREEN_BPP       16

#define MAX_TEXTURES      8    // Number of tileset images (default: 8)
#define SCROLLFACTOR_FAST 200.0f // The default factor for scrolling
#define SCROLLFACTOR_SLOW 100.0f // The default factor for scrolling

// Camera stuff
#define DEG_TO_RAD       0.017453292519943295769236907684886f
#define RAD_TO_DEG       57.295779513082320876798154814105
#define FOV              40  // Field of view

#define INVALID_TILE      ((Uint16)(~(Uint16)0))   // Don't draw the fansquare if tile = this
#define MAXMESHRENDER    (1024*8)       // Max number of tiles to draw



#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	const Uint32 rmask = 0x000000ff;
	const Uint32 gmask = 0x0000ff00;
	const Uint32 bmask = 0x00ff0000;
	const Uint32 amask = 0xff000000;
#else
	const Uint32 rmask = 0xff000000;
	const Uint32 gmask = 0x00ff0000;
	const Uint32 bmask = 0x0000ff00;
	const Uint32 amask = 0x000000ff;
#endif


//---------------------------------------------------------------------
//-   
//---------------------------------------------------------------------
struct ogl_vertex_t
{
	vect4 pos;
	vect4 col;
	Uint32 color;  ///< should replace r,g,b,a and be called by glColor4ubv

	vect2 tx;      ///< u and v in D3D I guess
	vect3 nrm;
	vect3 up;
	vect3 rt;
};
typedef struct ogl_vertex_t GLVertex;


//---------------------------------------------------------------------
//-                                                                   -
//---------------------------------------------------------------------
struct sGraphics
{
	GLenum shading; // current shading type
};
typedef struct sGraphics Graphics_t;


//---------------------------------------------------------------------
//-   The camera
//---------------------------------------------------------------------
class c_camera
{
	private:
		void _cam_frustum_jitter_fov(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
		void make_matrix();

		GLmatrix make_view_matrix(const vect3, const vect3, const vect3, const float);
		GLmatrix make_projection_matrix(const float, const float, const float);

	public:
		vect3 m_pos;      // The camera position
		vect3 m_trackpos; // The camera target

		float m_movex;    // for x axis movement
		float m_movey;    // for y axis movement
		float m_movez;    // for z axis movement

		float m_roll;     // Camera roll angle
		float m_factor;   // scrolling factor

		GLmatrix m_projection;
		GLmatrix m_projection_big;
		GLmatrix m_view;

		void move();
		void reset();

		c_camera();
		~c_camera();
};


//---------------------------------------------------------------------
//-   The renderlist
//---------------------------------------------------------------------
class c_renderlist
{
	private:
	public:
		int     m_num_totl;                 // Number to render, total
		Uint32  m_totl[MAXMESHRENDER];      // List of which to render, total

		int     m_num_shine;                // ..., reflective
		Uint32  m_shine[MAXMESHRENDER];     // ..., reflective

		int     m_num_reflc;                // ..., has reflection
		Uint32  m_reflc[MAXMESHRENDER];     // ..., has reflection

		int     m_num_norm;                 // ..., not reflective, has no reflection
		Uint32  m_norm[MAXMESHRENDER];      // ..., not reflective, has no reflection

		int     m_num_watr;                 // ..., water
		Uint32  m_watr[MAXMESHRENDER];      // ..., water
		Uint32  m_watr_mode[MAXMESHRENDER];

		bool build();
};


//---------------------------------------------------------------------
//-   The renderer
//---------------------------------------------------------------------
class c_renderer
{
	private:
		Graphics_t m_gfxState;
		GLuint m_texture[MAX_TEXTURES];

		void initSDL();
		void initGL();

		void render_fan(Uint32);
		bool load_texture(string, int);

		void render_text(string, vect3);

		const SDL_VideoInfo *m_videoInfo; // this holds some info about our display

	public:
		c_renderer();
		~c_renderer();

		SDL_Surface *m_screen;
		TTF_Font *m_font;
		GLfloat m_fps;
		c_camera *m_cam;

		int m_videoFlags;                 // Flags to pass to SDL_SetVideoMode

		void resize_window(int, int);
		void render_positions();
		void draw_GL_Scene();

		void begin_3D_mode();
		void begin_2D_mode();
		void end_3D_mode();
		void end_2D_mode();

		void render_text();

		void load_basic_textures(string);
		void render_mesh();

		c_renderlist m_renderlist;
};
#endif
