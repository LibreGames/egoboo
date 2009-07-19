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
	this->m_screen = NULL;
	this->render_mode = RENDER_NORMAL;

	this->initSDL();
	this->initGL();
	this->resize_window(g_config->get_width(), g_config->get_height());

	this->m_wm  = new c_window_manager(this);
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


//---------------------------------------------------------------------
//-   General SDL initialisation
//---------------------------------------------------------------------
void c_renderer::initSDL()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		cout << "Failed! Unable to initialize SDL: " << SDL_GetError() << endl;
		throw modbaker_exception("Unable to initialize SDL");
	}
	atexit( SDL_Quit );

	// TODO: Get BPP from config
	m_screen = SDL_SetVideoMode(g_config->get_width(), g_config->get_height(), 32, SDL_HWSURFACE | SDL_OPENGL | SDL_HWACCEL);
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

/*
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

//		string pfname = g_config->get_egoboo_path() + "basicdat/" + g_config->get_font_file();

//		m_font = TTF_OpenFont(pfname.c_str(), g_config->get_font_size());
//		if (NULL == m_font)
//		{
//			cout << "ERROR: Error loading font \"" << pfname << "\": " << TTF_GetError() << endl;
//		}
	}

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
	load_texture(g_config->get_egoboo_path() + "basicdat/globalparticles/particle.png",        TX_PARTICLE);
/*
	if (!load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/tile0.bmp",    TX_TILE_0))
		load_texture("data/tiles/tile0.bmp", TX_TILE_0);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/tile1.bmp",    TX_TILE_1))
		load_texture("data/tiles/tile1.bmp", TX_TILE_1);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/tile2.bmp",    TX_TILE_2))
		load_texture("data/tiles/tile2.bmp", TX_TILE_2);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/tile3.bmp",    TX_TILE_3))
		load_texture("data/tiles/tile3.bmp", TX_TILE_3);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/watertop.bmp", TX_WATER_TOP))
		load_texture("data/tiles/watertop.bmp", TX_WATER_TOP);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/waterlow.bmp", TX_WATER_LOW))
		load_texture("data/tiles/waterlow.bmp", TX_WATER_LOW);
*/

	load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/tile0.bmp",    TX_TILE_0);
	load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/tile1.bmp",    TX_TILE_1);
//	load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/tile2.bmp",    TX_TILE_2);
	load_texture("data/tiles/tile2.bmp",    TX_TILE_2);
	load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/tile3.bmp",    TX_TILE_3);
	load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/watertop.bmp", TX_WATER_TOP);
	load_texture(g_config->get_egoboo_path() + "modules/" + modname + "/gamedat/waterlow.bmp", TX_WATER_LOW);
}


//---------------------------------------------------------------------
//-   Load a single texture
//---------------------------------------------------------------------
bool c_renderer::load_texture(string imgname, int tileset)
{
	SDL_Surface *picture;
	float tex_coords[4];

	picture = IMG_Load(imgname.c_str());

	cout <<  "INFO: Generating texture " << imgname << ": ";

	if (picture == NULL)
	{
		cout << "     WARNING: Cannot open file " << imgname << endl;
		return false;
	}

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, m_texture + tileset);

	cout << "Slot is m_texture[" << tileset <<  "]==" << m_texture[tileset] << endl;

	if (copySurfaceToTexture(picture, m_texture[tileset], tex_coords))
	{
		// Free the SDL_Surface
		SDL_FreeSurface(picture);
		picture = NULL;
	}

	return true;
}


//---------------------------------------------------------------------
//-   Load a single icon
//---------------------------------------------------------------------
bool c_renderer::load_icon(string imgname, c_object *p_object)
{
	SDL_Surface *picture;
	float tex_coords[4];

	picture = IMG_Load(imgname.c_str());

	glEnable(GL_TEXTURE_2D); // TODO: Do we need this?
	glGenTextures(1, p_object->get_icon_array());

	cout <<  "INFO: icon: " << p_object->get_icon() << " (\"" << imgname << "\")" << endl;

	if (picture == NULL)
	{
		p_object->set_icon(0);
		return false;
	}

	if (copySurfaceToTexture(picture, p_object->get_icon(), tex_coords))
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
bool c_renderer::render_positions()
{
	if (g_module == NULL)
		return false;

	if (g_module->mem == NULL)
		return false;

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
	glVertex3f((g_module->mem->get_vert(g_nearest_vertex).x - 10), (g_module->mem->get_vert(g_nearest_vertex).y - 10), g_module->mem->get_vert(g_nearest_vertex).z + 1); // Bottom left
	glVertex3f((g_module->mem->get_vert(g_nearest_vertex).x + 10), (g_module->mem->get_vert(g_nearest_vertex).y - 10), g_module->mem->get_vert(g_nearest_vertex).z + 1); // Bottom right
	glVertex3f((g_module->mem->get_vert(g_nearest_vertex).x + 10), (g_module->mem->get_vert(g_nearest_vertex).y + 10), g_module->mem->get_vert(g_nearest_vertex).z + 1); // Top    right
	glVertex3f((g_module->mem->get_vert(g_nearest_vertex).x - 10), (g_module->mem->get_vert(g_nearest_vertex).y + 10), g_module->mem->get_vert(g_nearest_vertex).z + 1); // Top    left
	glEnd();

	glBegin(GL_LINES);
	glLineWidth(5.0f);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

	glVertex3f((g_module->mem->get_vert(g_nearest_vertex).x), (g_module->mem->get_vert(g_nearest_vertex).y), g_module->mem->get_vert(g_nearest_vertex).z - 500);
	glVertex3f((g_module->mem->get_vert(g_nearest_vertex).x), (g_module->mem->get_vert(g_nearest_vertex).y), g_module->mem->get_vert(g_nearest_vertex).z + 500);
	glEnd();

	int i;

	if (g_selection.get_selection_mode() == SELECTION_MODE_VERTEX)
	{
		// Render all tiles in the selection
		glBegin(GL_QUADS);
		glColor4f(0.0f, 1.0f, 0.0f, 1);

		for (i = 0; i < g_module->mem->vrt_count; i++)
		{
			if (g_selection.in_selection(i))
			{
				glVertex3f((g_module->mem->get_vert(i).x - 10), (g_module->mem->get_vert(i).y - 10), g_module->mem->get_vert(i).z + 1); // Bottom left
				glVertex3f((g_module->mem->get_vert(i).x + 10), (g_module->mem->get_vert(i).y - 10), g_module->mem->get_vert(i).z + 1); // Bottom right
				glVertex3f((g_module->mem->get_vert(i).x + 10), (g_module->mem->get_vert(i).y + 10), g_module->mem->get_vert(i).z + 1); // Top    right
				glVertex3f((g_module->mem->get_vert(i).x - 10), (g_module->mem->get_vert(i).y + 10), g_module->mem->get_vert(i).z + 1); // Top    left
			}
		}
		glEnd();
	}

	glEnable(GL_TEXTURE_2D);
	// Now reset the colors
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	return true;
}


//---------------------------------------------------------------------
//-   Begin the frame
//---------------------------------------------------------------------
void c_renderer::begin_frame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, m_screen->w, m_screen->h);
}


//---------------------------------------------------------------------
//-   Finally draw the scene to the screen
//---------------------------------------------------------------------
void c_renderer::end_frame()
{
	static GLint T0     = 0;
	static GLint Frames = 0;

	// Draw it to the screen
	m_wm->get_gui()->logic();
	m_wm->get_gui()->draw();

	SDL_GL_SwapBuffers();

	// Calculate the frames per second
	Frames++;
	{
		GLint t = SDL_GetTicks();
		if (t - T0 >= 1000)
		{
			GLfloat seconds = (t - T0) / 1000.0;
			m_fps = Frames / seconds;
			m_wm->set_fps(m_fps);
			T0 = t;
			Frames = 0;
		}
	}
}


//---------------------------------------------------------------------
//-   Call render_fan() for every fan in the mesh
//---------------------------------------------------------------------
bool c_renderer::render_mesh()
{
	if (g_module == NULL)
		return false;

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
			Uint16 this_tile = g_module->mem->tiles[this_fan].tile;
			if (INVALID_TILE == this_tile) continue;

			// keep one texture in memory as long as possible
			Uint8 this_tileset = this_tile >> 6;
			if (this_tileset == tileset)
			{
				render_fan(this_fan, false);
			}
		};
	}

	return true;
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
	vect3 tmp_vertex;

	Uint16 tile = g_module->mem->tiles[fan].tile; // Tile
	Uint16 fx   = g_module->mem->tiles[fan].fx;   // Tile FX
	Uint16 type = g_module->mem->tiles[fan].type; // Command type ( index to points in fan )
	// type <= 32 => small tile
	// type >  32 => big tile

	if (tile == INVALID_TILE) return;

	//	mesh_calc_normal_fan( pmesh, &(gs->phys), fan, &nrm, &pos );
	tile &= 0xFF;
	off = g_module->getTileOffset(tile);

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

	vertices  = g_module->getTileDefinition(type).vrt_count;            // Number of vertices
	badvertex = g_module->mem->tiles[fan].vrt_start;   // Get big reference value

	// Fill in the vertex data from the mesh memory
	//	for (cnt = 0; cnt < vertices; cnt++, badvertex++)
	//	{
	//		v[cnt].pos.x = g_module->mem->vrt_x[badvertex];
	//		v[cnt].pos.y = g_module->mem->vrt_y[badvertex];
	//		v[cnt].pos.z = g_module->mem->vrt_z[badvertex];
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
	commands = g_module->getTileDefinition(type).cmd_count;            // Number of commands
	glColor4f(1, 1, 1, 1);

	// Render the FX bits
	this->render_tile_flag(fx);

	// Highlight the nearest tile and tiles in the selection
	if ((g_selection.get_selection_mode() == SELECTION_MODE_TILE) &&
		(g_selection.in_selection((int)fan) || (int)fan == g_nearest_tile))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(0, 1, 0, 1);
	}


	for (cnt = 0, entry = 0; cnt < commands; cnt++)
	{
		c_tile_definition & tile_def = g_module->getTileDefinition(type);
		Uint8 size = tile_def.cmd_size[cnt];

		glBegin(GL_TRIANGLE_FAN);

		for (tnc = 0; tnc < size; tnc++)
		{
			Uint16 loc_vrt = tile_def.vrt[entry];
			Uint16 glb_vrt = loc_vrt + badvertex;

			tmp_vertex = g_module->mem->get_vert(glb_vrt);

			glTexCoord2f(tile_def.tx[loc_vrt].u + off.u, tile_def.tx[loc_vrt].v + off.v);
			glVertex3f(tmp_vertex.x, tmp_vertex.y, tmp_vertex.z);

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
	for ( tnc = 0; tnc < g_module->getTileDefinition(type).cmd_size[cnt]; tnc++ )
	{
	vertex = g_module->getTileDefinition(type).vrt[entry];

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
	glOrtho(0, m_screen->w, m_screen->h, 0, -1, 1);   // Set up an orthogonal projection

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


c_window_manager* c_renderer::get_wm()
{
	return m_wm;
}


void c_renderer::set_wm(c_window_manager* p_wm)
{
	m_wm = p_wm;
}


SDL_Surface *c_renderer::get_screen()
{
	if (m_screen == NULL)
	{
		cout << "WARNING: c_renderer::get_screen() returning an empty surface" << endl;
	}
	return m_screen;
}


void c_renderer::set_screen(SDL_Surface *p_screen)
{
	m_screen = p_screen;
}


// Mesh render mode
int c_renderer::get_render_mode()
{
	return this->render_mode;
}

void c_renderer::set_render_mode(int p_render_mode)
{
	this->render_mode = p_render_mode;
}


//---------------------------------------------------------------------
//-   Render the tile flags
//---------------------------------------------------------------------
void c_renderer::render_tile_flag(Uint16 p_fx)
{
	// Render 1st is rendered in pink
	if ((g_renderer->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_REF)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(1, 0.5, 1, 1);
	}

	// Reflecting is rendered in bright blue
	if ((g_renderer->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_DRAWREF)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(0.5, 1, 0.5, 1);
	}

	// Slippy is rendered in green
	if ((g_renderer->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_SLIPPY)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(0, 1, 0, 1);
	}

	// Water is rendered in blue
	if ((g_renderer->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_WATER)))
	{
		glEnable(GL_BLEND);
		glColor4f(0, 0, 1, 1);
	}

	// Damage is rendered in red
	if ((g_renderer->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_DAMAGE)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(1, 0, 0, 1);
	}

	// Wall is rendered in violet
	if ((g_renderer->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_WALL)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(1, 0, 1, 1);
	}

	// Impassable is rendered in orange
	if ((g_renderer->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_IMPASS)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(1, 0.5, 0, 1);
	}
}
