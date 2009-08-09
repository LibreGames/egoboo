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
/// @brief Basic renderer implementation
/// @details Implementation of the basic renderer

#include "SDL_GL_extensions.h"
#include "ogl_debug.h"

#include "renderer.h"
#include "global.h"

#include <iostream>
using namespace std;

static ogl_state_t tmp_ogl_state;


//---------------------------------------------------------------------
///   c_basic_renderer constructor
//---------------------------------------------------------------------
c_basic_renderer::c_basic_renderer()
{
	this->m_fps    = 0.0f;
	this->m_screen = NULL;
}


//---------------------------------------------------------------------
///   c_basic_renderer destructor
//---------------------------------------------------------------------
c_basic_renderer::~c_basic_renderer()
{
}


//---------------------------------------------------------------------
///   General SDL initialisation
//---------------------------------------------------------------------
void c_basic_renderer::initSDL(int p_width, int p_height, int p_bpp)
{
	cout << "Setting up SDL" << endl;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		cout << "Failed! Unable to initialize SDL: " << SDL_GetError() << endl;
		throw modbaker_exception("Unable to initialize SDL");
	}
	atexit( SDL_Quit );

	m_screen = SDL_SetVideoMode(p_width, p_height, p_bpp, SDL_HWSURFACE | SDL_OPENGL | SDL_HWACCEL);
	if (m_screen == NULL)
	{
		cout << "Failed! Unable to set video mode: " << SDL_GetError() << endl;
		throw modbaker_exception("Unable to set video mode");
	}

	if (SDL_EnableUNICODE(1) < 0)
	{
		cout << "Unable to set Unicode" << endl;
		throw modbaker_exception("Unable to set Unicode");
	}

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	//	SDL_WM_SetIcon(tmp_surface, NULL);
	SDL_WM_SetCaption("MODBaker", "MODBaker");
}


//---------------------------------------------------------------------
///   General OpenGL initialisation
//---------------------------------------------------------------------
void c_basic_renderer::initGL()
{
	// glClear() stuff
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Set the background black
	glClearDepth(1.0);

	// depth buffer stuff
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// alpha stuff
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);

	// backface culling
	// TODO: This prevents the mesh from getting rendered
	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW);
	//glCullFace(GL_BACK);

	// set up environment mapping
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

	gl_grab_state(&tmp_ogl_state);
}


//---------------------------------------------------------------------
///   Reset our viewport after a window resize
///   \param p_width new window width
///   \param p_height new window height
//---------------------------------------------------------------------
void c_basic_renderer::resize_window(int p_width, int p_height)
{
	// Protect against a divide by zero
	if (p_height == 0)
		p_height = 1;

	glViewport(0, 0, p_width, p_height); // Set our viewport

	glMatrixMode(GL_PROJECTION);     // Sets the projection matrix.
	glLoadIdentity();                // Reset the modelview matrix.

	// Calculate the aspect ratio of the window.
	gluPerspective(45.0f, (float)p_width / (float)p_height, 0.1f, 20000.0f);

	glMatrixMode(GL_MODELVIEW);      // Sets the projection matrix.
	glLoadIdentity();                // Reset the modelview matrix.
}


//---------------------------------------------------------------------
///   Load a single texture
///   \param imgname name of the image to load
///   \param tileset type where the image gets stored
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_basic_renderer::load_texture(string p_imgname, int p_tileset)
{
	SDL_Surface *picture;
	float tex_coords[4];

	picture = IMG_Load(p_imgname.c_str());

	cout <<  "INFO: Generating texture " << p_imgname << ": ";

	if (picture == NULL)
	{
		cout << "     WARNING: Cannot open file " << p_imgname << endl;
		return false;
	}

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, m_texture + p_tileset);

	cout << "Slot is m_texture[" << p_tileset <<  "]==" << m_texture[p_tileset] << endl;

	if (copySurfaceToTexture(picture, m_texture[p_tileset], tex_coords))
	{
		// Free the SDL_Surface
		SDL_FreeSurface(picture);
		picture = NULL;
	}

	return true;
}


//---------------------------------------------------------------------
///   Begin the frame: Reset the scene
//---------------------------------------------------------------------
void c_basic_renderer::begin_frame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, m_screen->w, m_screen->h);
}


//---------------------------------------------------------------------
///   Finally draw the scene to the screen and calculate the FPS
//---------------------------------------------------------------------
void c_basic_renderer::end_frame()
{
	static GLint T0     = 0;
	static GLint Frames = 0;

	// Draw it to the screen
	SDL_GL_SwapBuffers();

	// Calculate the frames per second
	Frames++;
	{
		GLint t = SDL_GetTicks();
		if (t - T0 >= 1000)
		{
			GLfloat seconds = (t - T0) / 1000.0;
			m_fps = Frames / seconds;
			T0 = t;
			Frames = 0;
		}
	}
}


//---------------------------------------------------------------------
///   Begin the 3D mode: set the GLview etc...
//---------------------------------------------------------------------
void c_basic_renderer::begin_3D_mode()
{
	// Clear the screen and reset the matrix
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	// Save the projection matrix to the stack
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(this->m_projection.v);

	// Save the view matrix to the stack
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(this->m_view.v);
}


//---------------------------------------------------------------------
///   End the 3D mode: cleanup
//---------------------------------------------------------------------
void c_basic_renderer::end_3D_mode()
{
	// Remove the projection matrix from the stack
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Remove the view matrix from the stack
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


//---------------------------------------------------------------------
///   Return the screen
///   \return the SDL screen object
//---------------------------------------------------------------------
SDL_Surface *c_basic_renderer::get_screen()
{
	if (m_screen == NULL)
	{
		cout << "WARNING: c_renderer::get_screen() returning an empty surface" << endl;
	}
	return m_screen;
}


//---------------------------------------------------------------------
///   Set the screen
///   \param p_screen new screen
//---------------------------------------------------------------------
void c_basic_renderer::set_screen(SDL_Surface *p_screen)
{
	m_screen = p_screen;
}


//---------------------------------------------------------------------
///   Get the current FPS
///   \param p_fps new FPS
//---------------------------------------------------------------------
void  c_basic_renderer::set_fps(float p_fps)
{
	this->m_fps = p_fps;
}


//---------------------------------------------------------------------
///   Get the FPS
///   \return current FPS
//---------------------------------------------------------------------
float c_basic_renderer::get_fps()
{
	return this->m_fps;
}
