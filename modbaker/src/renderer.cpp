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

#include "renderer.h"
#include "global.h"

#include "SDL_GL_extensions.h"
#include "ogl_debug.h"

#include <iostream>
#include <sstream>

static ogl_state_t tmp_ogl_state;

//---------------------------------------------------------------------
//-   Constructor: Set the basic renderer values
//---------------------------------------------------------------------
c_renderer::c_renderer()
{
	this->m_fps = 0.0f;

	this->initSDL();
	this->initGUI();
	this->initGL();
	this->resize_window(WINDOW_WIDTH, WINDOW_HEIGHT);

	this->m_cam = new c_camera();
}


//---------------------------------------------------------------------
//-   Destructor: Cleanup
//---------------------------------------------------------------------
c_renderer::~c_renderer()
{
/*	glDeleteTextures(MAX_TEXTURES, m_texture);

	if (NULL != m_font)
	{
		TTF_CloseFont(m_font);
	}
*/
}


void c_renderer::initGUI()
{
	m_wm.init();
}


//---------------------------------------------------------------------
//-   General SDL initialisation
//---------------------------------------------------------------------
void c_renderer::initSDL()
{
	SDL_Init(SDL_INIT_VIDEO);
	m_wm.screen = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE | SDL_OPENGL | SDL_HWACCEL);
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

/*
	cout << "INFO: Initializing SDL version " << SDL_MAJOR_VERSION << "." << SDL_MINOR_VERSION << "." << SDL_PATCHLEVEL << "... ";
	if ( SDL_Init( 0 ) < 0 )
	{
		cout << "Failed! Unable to initialize SDL:" << SDL_GetError() << endl;
		throw modbaker_exception("Unable to initialize SDL");
	}
	else
	{
		cout << "Success!" << endl;
	}
	atexit( SDL_Quit );

	if (SDL_EnableUNICODE(1) < 0)
	{
		cout << "Unable to set Unicode" << endl;
		throw modbaker_exception("Unable to set Unicode");
	}

	cout << "INFO: Initializing SDL video... ";
	if (SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0)
	{
		cout << "Failed! Unable to initialize SDL video:" << SDL_GetError() << endl;
		throw modbaker_exception("Unable to initialize SDL video");
	}
	else
	{
		cout << "Success!"  << endl;
	}

	if (SDL_InitSubSystem( SDL_INIT_TIMER ) < 0)
	{
		cout << "WARN: SDL Timer initialization failed: " << SDL_GetError() << endl;
	}

	// start the font handler
	m_font = NULL;
	if (TTF_Init() < 0)
	{
		cout << "ERROR: Unable to load the font handler: " << SDL_GetError() << endl;
		throw modbaker_exception("Unable to load the font handler");
	}
	else
	{
		atexit(TTF_Quit);

//		string pfname = g_config.get_egoboo_path() + "basicdat/" + g_config.get_font_file();

//		m_font = TTF_OpenFont(pfname.c_str(), g_config.get_font_size());
//		if (NULL == m_font)
//		{
//			cout << "ERROR: Error loading font \"" << pfname << "\": " << TTF_GetError() << endl;
//		}
	}

	//	SDL_WM_SetIcon(tmp_surface, NULL);
	SDL_WM_SetCaption("MODBaker", "MODBaker");

	// the flags to pass to SDL_SetVideoMode
	m_gfxState.sdl_vid.flags          = SDL_RESIZABLE;
	m_gfxState.sdl_vid.opengl         = SDL_TRUE;
	m_gfxState.sdl_vid.doublebuffer   = SDL_TRUE;
	m_gfxState.sdl_vid.glacceleration = GL_TRUE;
	m_gfxState.sdl_vid.width          = WINDOW_WIDTH;
	m_gfxState.sdl_vid.height         = WINDOW_HEIGHT;
	m_gfxState.sdl_vid.depth          = SCREEN_BPP;

	// Get us a video mode
	if (NULL == sdl_set_mode(NULL, &(m_gfxState.sdl_vid), NULL))
	{
		cout << "I can't get SDL to set any video mode: " << SDL_GetError() << endl;
		throw modbaker_exception("I can't get SDL to set any video mode");
	}
*/
}


//---------------------------------------------------------------------
//-   General OpenGL initialisation
//---------------------------------------------------------------------
void c_renderer::initGL()
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
	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW); // TODO: This prevents the mesh from getting rendered
	//glCullFace(GL_BACK);

	// set up environment mapping
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

	gl_grab_state(&tmp_ogl_state);
}


//---------------------------------------------------------------------
//-   Reset our viewport after a window resize
//---------------------------------------------------------------------
void c_renderer::resize_window( int width, int height )
{
	// Protect against a divide by zero
	if (height == 0)
		height = 1;

	glViewport(0, 0, width, height); // Set our viewport

	glMatrixMode(GL_PROJECTION);     // Sets the projection matrix.
	glLoadIdentity();                // Reset the modelview matrix.

	// Calculate the aspect ratio of the window.
	gluPerspective(45.0f, (float)width / (float)height, 0.1f, 20000.0f);

	glMatrixMode(GL_MODELVIEW);      // Sets the projection matrix.
	glLoadIdentity();                // Reset the modelview matrix.
}


//---------------------------------------------------------------------
//-   Load all basic textures
//---------------------------------------------------------------------
void c_renderer::load_basic_textures(string modname)
{
	load_texture(g_config.get_egoboo_path() + "basicdat/globalparticles/particle.png",        TX_PARTICLE);
	load_texture(g_config.get_egoboo_path() + "modules/" + modname + "/gamedat/tile0.bmp",    TX_TILE_0);
	load_texture(g_config.get_egoboo_path() + "modules/" + modname + "/gamedat/tile1.bmp",    TX_TILE_1);
	load_texture(g_config.get_egoboo_path() + "modules/" + modname + "/gamedat/tile2.bmp",    TX_TILE_2);
	load_texture(g_config.get_egoboo_path() + "modules/" + modname + "/gamedat/tile3.bmp",    TX_TILE_3);
	load_texture(g_config.get_egoboo_path() + "modules/" + modname + "/gamedat/watertop.bmp", TX_WATER_TOP);
	load_texture(g_config.get_egoboo_path() + "modules/" + modname + "/gamedat/waterlow.bmp", TX_WATER_LOW);
}


//---------------------------------------------------------------------
//-   Load a single texture
//---------------------------------------------------------------------
bool c_renderer::load_texture(string imgname, int tileset)
{
	SDL_Surface *picture;
	float tex_coords[4];

	picture = IMG_Load(imgname.c_str());

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, m_texture + tileset);

	cout <<  "INFO: m_texture[" << tileset <<  "]==" << m_texture[tileset] << " (\"" << imgname << "\")" << endl;

	if (copySurfaceToTexture(picture, m_texture[tileset], tex_coords))
	{
		// Free the SDL_Surface
		SDL_FreeSurface(picture);
		picture = NULL;
	}

	return true;
}


//---------------------------------------------------------------------
//-   Draw certain helping positions
//---------------------------------------------------------------------
void c_renderer::render_positions()
{
	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// Render the origin (0,0,0)
	glBegin(GL_QUADS);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

	glVertex3i(-10, -10, 250); // Bottom left
	glVertex3i( 10, -10, 250); // Bottom right
	glVertex3i( 10, 10, 250); // Top    right
	glVertex3i(-10, 10, 250); // Top    left
	glEnd();

	// Render a box and a line at the nearest vertex position
	glBegin(GL_QUADS);
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex] - 10), (g_mesh.mem->vrt_y[g_nearest_vertex] - 10), g_mesh.mem->vrt_z[g_nearest_vertex] + 1); // Bottom left
	glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex] + 10), (g_mesh.mem->vrt_y[g_nearest_vertex] - 10), g_mesh.mem->vrt_z[g_nearest_vertex] + 1); // Bottom right
	glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex] + 10), (g_mesh.mem->vrt_y[g_nearest_vertex] + 10), g_mesh.mem->vrt_z[g_nearest_vertex] + 1); // Top    right
	glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex] - 10), (g_mesh.mem->vrt_y[g_nearest_vertex] + 10), g_mesh.mem->vrt_z[g_nearest_vertex] + 1); // Top    left
	glEnd();

	glBegin(GL_LINES);
	glLineWidth(5.0f);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

	glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex]), (g_mesh.mem->vrt_y[g_nearest_vertex]), g_mesh.mem->vrt_z[g_nearest_vertex] - 500);
	glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex]), (g_mesh.mem->vrt_y[g_nearest_vertex]), g_mesh.mem->vrt_z[g_nearest_vertex] + 500);
	glEnd();

	int i;

	// Render all tiles in the selection
	glBegin(GL_QUADS);
	glColor4f(0.0f, 1.0f, 0.0f, 1);

	for (i = 0; i < g_mesh.mi->vert_count; i++)
	{
		if (g_selection.in_selection(i))
		{
			glVertex3f((g_mesh.mem->vrt_x[i] - 10), (g_mesh.mem->vrt_y[i] - 10), g_mesh.mem->vrt_z[i] + 1); // Bottom left
			glVertex3f((g_mesh.mem->vrt_x[i] + 10), (g_mesh.mem->vrt_y[i] - 10), g_mesh.mem->vrt_z[i] + 1); // Bottom right
			glVertex3f((g_mesh.mem->vrt_x[i] + 10), (g_mesh.mem->vrt_y[i] + 10), g_mesh.mem->vrt_z[i] + 1); // Top    right
			glVertex3f((g_mesh.mem->vrt_x[i] - 10), (g_mesh.mem->vrt_y[i] + 10), g_mesh.mem->vrt_z[i] + 1); // Top    left
		}
	}
	glEnd();

	glEnable(GL_TEXTURE_2D);
	// Now reset the colors
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}


//---------------------------------------------------------------------
//-   Begin the frame
//---------------------------------------------------------------------
void c_renderer::begin_frame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, m_wm.screen->w, m_wm.screen->h);
}


//---------------------------------------------------------------------
//-   Finally draw the scene to the screen
//---------------------------------------------------------------------
void c_renderer::end_frame()
{
	static GLint T0     = 0;
	static GLint Frames = 0;

	// Draw it to the screen
	m_wm.gui->logic();
	m_wm.gui->draw();

	SDL_GL_SwapBuffers();

	// Calculate the frames per second
	Frames++;
	{
		GLint t = SDL_GetTicks();
		if (t - T0 >= 1000)
		{
			GLfloat seconds = (t - T0) / 1000.0;
			m_fps = Frames / seconds;
			m_wm.set_fps(m_fps);
			T0 = t;
			Frames = 0;
		}
	}
}


//---------------------------------------------------------------------
//-   Call render_fan() for every fan in the mesh
//---------------------------------------------------------------------
void c_renderer::render_mesh()
{
	// Reset the last used texture, so we HAVE to load a new one
	// for the first tiles
	ogl_state_t      loc_ogl_state;
	ogl_state_comp_t loc_ogl_state_comp;

	gl_grab_state(&loc_ogl_state);
	gl_comp_state(&loc_ogl_state_comp, &loc_ogl_state, &tmp_ogl_state);

	glEnable(GL_TEXTURE_2D);

	for (Uint8 tileset = 0; tileset < 4; tileset++)
	{
		// Change the texture
		GLint itexture, icurrent_tx;
		itexture = m_texture[tileset + TX_TILE_0];

		glGetIntegerv(GL_TEXTURE_BINDING_2D, &icurrent_tx);
		if (icurrent_tx != itexture)
		{
			glBindTexture(GL_TEXTURE_2D, itexture);
		}

		for (int inorm = 0; inorm < m_renderlist.m_num_norm; inorm++)
		{
			// grab a tile from the renderlist
			Uint32 this_fan  = m_renderlist.m_norm[inorm];

			// do not render invalid tiles
			Uint16 this_tile = g_mesh.mem->tilelst[this_fan].tile;
			if (INVALID_TILE == this_tile) continue;

			// keep one texture in memory as long as possible
			Uint8 this_tileset = this_tile >> 6;
			if (this_tileset == tileset)
			{
				render_fan(this_fan, false);
			}
		};
	}
}


//---------------------------------------------------------------------
//-   Render a single fan / tile
//---------------------------------------------------------------------
void c_renderer::render_fan(Uint32 fan, bool set_texture)
{
	Uint16 commands;
	Uint16 vertices;
	Uint16 itexture;

	Uint16 cnt, tnc, entry;

	Uint32 badvertex;

	vect2 off;

	Uint16 tile = g_mesh.mem->tilelst[fan].tile; // Tile
	Uint16 type = g_mesh.mem->tilelst[fan].type; // Command type ( index to points in fan )
	// type <= 32 => small tile
	// type >  32 => big tile

	if (tile == INVALID_TILE) return;

	//	mesh_calc_normal_fan( pmesh, &(gs->phys), fan, &nrm, &pos );
	tile &= 0xFF;
	off = g_mesh.getTileOffset(tile);

	// Change the texture if we need to
	if (set_texture)
	{
		// Detect which tileset to load (1, 2, 3 or 4 (0 = particles))
		itexture = (tile >> 6 ) + TX_TILE_0;    // (1 << 6) == 64 tiles in each 256x256 texture

		GLint icurrent_tx;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &icurrent_tx);
		if ((GLuint)icurrent_tx != m_texture[itexture])
		{
			glBindTexture(GL_TEXTURE_2D, m_texture[itexture]);
		}
	}


	//	light_flat.r =
	//	light_flat.g =
	//	light_flat.b =
	//	light_flat.a = 0.0f;

	vertices  = g_mesh.getTileDefinition(type).vrt_count;            // Number of vertices
	badvertex = g_mesh.mem->tilelst[fan].vrt_start;   // Get big reference value

	// Fill in the vertex data from the mesh memory
	//	for (cnt = 0; cnt < vertices; cnt++, badvertex++)
	//	{
	//		v[cnt].pos.x = g_mesh.mem->vrt_x[badvertex];
	//		v[cnt].pos.y = g_mesh.mem->vrt_y[badvertex];
	//		v[cnt].pos.z = g_mesh.mem->vrt_z[badvertex];
	//
	//		v[cnt].col.r = FP8_TO_FLOAT( mem->vrt_lr_fp8[badvertex] + mem->vrt_ar_fp8[badvertex] );
	//		v[cnt].col.g = FP8_TO_FLOAT( mem->vrt_lg_fp8[badvertex] + mem->vrt_ag_fp8[badvertex] );
	//		v[cnt].col.b = FP8_TO_FLOAT( mem->vrt_lb_fp8[badvertex] + mem->vrt_ab_fp8[badvertex] );
	//		v[cnt].col.a = 1.0f;
	//
	//		light_flat.r += v[cnt].col.r;
	//		light_flat.g += v[cnt].col.g;
	//		light_flat.b += v[cnt].col.b;
	//		light_flat.a += v[cnt].col.a;
	//
	//	}


	//	if ( vertices > 1 )
	//	{
	//		light_flat.r /= vertices;
	//		light_flat.g /= vertices;
	//		light_flat.b /= vertices;
	//		light_flat.a /= vertices;
	//	};

	// use the average lighting
	//	if (this->gfxState.shading == GL_FLAT)
	//		glColor4fv( light_flat.v );

	// Render each command
	commands = g_mesh.getTileDefinition(type).cmd_count;            // Number of commands
	glColor4f(1, 1, 1, 1);
	for (cnt = 0, entry = 0; cnt < commands; cnt++)
	{
		c_tile_definition & tile_def = g_mesh.getTileDefinition(type);
		Uint8 size = tile_def.cmd_size[cnt];

		glBegin(GL_TRIANGLE_FAN);

		for (tnc = 0; tnc < size; tnc++)
		{
			Uint16 loc_vrt = tile_def.vrt[entry];
			Uint16 glb_vrt = loc_vrt + badvertex;

			glTexCoord2f(tile_def.tx[loc_vrt].u + off.u, tile_def.tx[loc_vrt].v + off.v);
			glVertex3f(g_mesh.mem->vrt_x[glb_vrt], g_mesh.mem->vrt_y[glb_vrt], g_mesh.mem->vrt_z[glb_vrt]);

			entry++;
		}
		glEnd();
	}

	/*
	// Devmode begin: draw the grid
	entry = 0;

	glBegin(GL_LINES);

	glLineWidth(5.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1);

	for ( cnt = 0; cnt < commands; cnt++ )
	{
	for ( tnc = 0; tnc < g_mesh.getTileDefinition(type).cmd_size[cnt]; tnc++ )
	{
	vertex = g_mesh.getTileDefinition(type).vrt[entry];

	//			glVertex3fv(v[vertex].pos.v);
	glVertex3f(v[vertex].pos.x, v[vertex].pos.y, (v[vertex].pos.z-5.0f));
	glVertex3f(v[vertex].pos.x, v[vertex].pos.y, (v[vertex].pos.z+5.0f));

	entry++;
	}
	}

	glEnd();
	// Devmode end
	*/
}


void c_renderer::begin_3D_mode()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity(); // Reset the matrix

	// Save the projection matrix to the stack
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(m_cam->m_projection.v);

	// Save the view matrix to the stack
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(m_cam->m_view.v);
}


void c_renderer::end_3D_mode()
{
	// Remove the projection matrix from the stack
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Remove the view matrix from the stack
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


void c_renderer::begin_2D_mode()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();         // Reset The Projection Matrix
	glOrtho(0, m_wm.screen->w, m_wm.screen->h, 0, -1, 1);   // Set up an orthogonal projection

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glColor4f(1, 1, 1, 1);
}


void c_renderer::end_2D_mode()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}


void c_renderer::render_text()
{
	return;
	// Draw current position
	vect3 pos_pos;
	pos_pos.x = m_wm.screen->w;
	pos_pos.y = 35;
	pos_pos.z = 0;

	ostringstream tmp_pos;
	tmp_pos << "Position: (" << g_mesh.mem->vrt_x[g_nearest_vertex] << ", " << g_mesh.mem->vrt_y[g_nearest_vertex] << ")";

	render_text(tmp_pos.str(), pos_pos, JRIGHT);
}
