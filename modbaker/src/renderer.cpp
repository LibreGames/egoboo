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
#include "global.h"
#include "renderer.h"

#include <iostream>
#include <sstream>

//---------------------------------------------------------------------
//-   Constructor: Set the basic renderer values
//---------------------------------------------------------------------
c_renderer::c_renderer()
{
	m_fps = 0.0f;

	initSDL();
	initGL();
    resize_window(WINDOW_WIDTH, WINDOW_HEIGHT);

	this->m_gfxState.shading = GL_FLAT; // TODO: Read from game config

	m_cam = new c_camera();
}


//---------------------------------------------------------------------
//-   Destructor: Cleanup
//---------------------------------------------------------------------
c_renderer::~c_renderer()
{
	glDeleteTextures(MAX_TEXTURES, m_texture);

	TTF_CloseFont(this->m_font);
}


//---------------------------------------------------------------------
//-   General SDL initialisation
//---------------------------------------------------------------------
void c_renderer::initSDL()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		cout << "Video initialization failed: " << SDL_GetError() << endl;
		Quit();
	}

	m_videoInfo = SDL_GetVideoInfo(); // Get the video info

	if (!m_videoInfo)
	{
		cout << "Video query failed: " << SDL_GetError() << endl;
		Quit();
	}

	// the flags to pass to SDL_SetVideoMode
	m_videoFlags  = SDL_OPENGL;
	m_videoFlags |= SDL_GL_DOUBLEBUFFER;
	m_videoFlags |= SDL_HWPALETTE;
	m_videoFlags |= SDL_RESIZABLE;

	// This checks to see if surfaces can be stored in memory
	if (m_videoInfo->hw_available)
		m_videoFlags |= SDL_HWSURFACE;
	else
		m_videoFlags |= SDL_SWSURFACE;

	// This checks if hardware blits can be done
	if (m_videoInfo->blit_hw)
		m_videoFlags |= SDL_HWACCEL;

	// Sets up OpenGL double buffering
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// get a SDL surface
	this->m_screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, SCREEN_BPP, m_videoFlags);


//	SDL_WM_SetIcon(tmp_surface, NULL);
	SDL_WM_SetCaption("MODBaker", "MODBaker");

	// Verify there is a surface
	if (!this->m_screen)
	{
		cout << "Video mode set failed: " << SDL_GetError() << endl;
		Quit();
	}

	// Init SDL_TTF
	TTF_Init();
	atexit(TTF_Quit);
	this->m_font = TTF_OpenFont("basicdat/Negatori.ttf", 16);

	if (this->m_font == NULL)
		cout << "Error loading font: " << TTF_GetError() << endl;
}


//---------------------------------------------------------------------
//-   General OpenGL initialisation
//---------------------------------------------------------------------
void c_renderer::initGL()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Set the background black

	// depth buffer stuff
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// gfxState.shading stuff
	glShadeModel(m_gfxState.shading);

	// alpha stuff
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);

	// backface culling
	glEnable(GL_CULL_FACE);
//	glFrontFace(GL_CW); // TODO: This prevents the mesh from getting rendered
	glCullFace(GL_BACK);

	resize_window(WINDOW_WIDTH, WINDOW_HEIGHT);
}


//---------------------------------------------------------------------
//-   Reset our viewport after a window resize
//---------------------------------------------------------------------
void c_renderer::resize_window(int width, int height)
{
	/* Protect against a divide by zero */
	if (height == 0)
		height = 1;

	glViewport(0, 0, width, height); // Set our viewport

	glMatrixMode(GL_PROJECTION);     // Sets the projection matrix.
	glLoadIdentity();                // Reset the modelview matrix.

	// Calculate the aspect ratio of the window.
	gluPerspective(45.0f, (float)width/(float)height, 0.1f, 20000.0f);

	glMatrixMode(GL_MODELVIEW);      // Sets the projection matrix.
	glLoadIdentity();                // Reset the modelview matrix.
}


//---------------------------------------------------------------------
//-   Load all basic textures
//---------------------------------------------------------------------
void c_renderer::load_basic_textures(string modname)
{
	this->load_texture("basicdat/globalparticles/particle.png",        TX_PARTICLE);
	this->load_texture("modules/" + modname + "/gamedat/tile0.bmp",    TX_TILE_0);
	this->load_texture("modules/" + modname + "/gamedat/tile1.bmp",    TX_TILE_1);
	this->load_texture("modules/" + modname + "/gamedat/tile2.bmp",    TX_TILE_2);
	this->load_texture("modules/" + modname + "/gamedat/tile3.bmp",    TX_TILE_3);
	this->load_texture("modules/" + modname + "/gamedat/watertop.bmp", TX_WATER_TOP);
	this->load_texture("modules/" + modname + "/gamedat/waterlow.bmp", TX_WATER_LOW);
}


//---------------------------------------------------------------------
//-   Load a single texture
//---------------------------------------------------------------------
bool c_renderer::load_texture(string imgname, int tileset)
{
	SDL_Surface *picture;
	SDL_Surface *img_temp;

	picture = IMG_Load(imgname.c_str());

	if (picture == NULL)
	{
		cout << "Couldn't load " << imgname << ":" << SDL_GetError() << endl;
		return false;
	}

	// Check that the image's width is a power of 2
	if ((picture->w & (picture->w - 1)) != 0)
	{
		cout << "warning: "<< imgname << "'s width is not a power of 2" << endl;
		return false;
	}

	// Also check if the height is a power of 2
	if ((picture->h & (picture->h - 1)) != 0)
	{
		cout << "warning: " << imgname << "'s height is not a power of 2\n" << endl;
		return false;
	}

	// Convert it to RGB
	img_temp = SDL_CreateRGBSurface(SDL_SWSURFACE, picture->w, picture->h, 32, rmask, gmask, bmask, amask);
	SDL_BlitSurface(picture, NULL, img_temp, NULL);
	picture = img_temp;

	glEnable(GL_TEXTURE_2D); 

	// Set it to a valid OpenGL texture
	glGenTextures(1, &m_texture[tileset]);
	glBindTexture(GL_TEXTURE_2D, m_texture[tileset]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, picture->w, picture->h,
		0, GL_RGBA, GL_UNSIGNED_BYTE, picture->pixels);

	glDisable(GL_TEXTURE_2D);

	// Free the SDL_Surface only if it was successfully created
	if (picture)
	{ 
		SDL_FreeSurface(picture);
	}

	return true;
}


//---------------------------------------------------------------------
//-   Draw certain helping positions
//---------------------------------------------------------------------
void c_renderer::render_positions()
{
	// Render the origin (0,0,0)
	glBegin(GL_QUADS);
		glColor4f(1.0f, 0.0f, 0.0f, 1);

		glVertex3i(-10,-10, 250); // Bottom left
		glVertex3i( 10,-10, 250); // Bottom right
		glVertex3i( 10, 10, 250); // Top    right
		glVertex3i(-10, 10, 250); // Top    left
	glEnd();

	// TODO: Render a box and a line at the nearest vertex position
	glBegin(GL_QUADS);
		glColor4f(0.0f, 1.0f, 0.0f, 1);

		glVertex3i((g_mesh.mem->vrt_x[g_nearest_vertex]-10),(g_mesh.mem->vrt_y[g_nearest_vertex]-10), g_mesh.mem->vrt_z[g_nearest_vertex]+1); // Bottom left
		glVertex3i((g_mesh.mem->vrt_x[g_nearest_vertex]+10),(g_mesh.mem->vrt_y[g_nearest_vertex]-10), g_mesh.mem->vrt_z[g_nearest_vertex]+1); // Bottom right
		glVertex3i((g_mesh.mem->vrt_x[g_nearest_vertex]+10),(g_mesh.mem->vrt_y[g_nearest_vertex]+10), g_mesh.mem->vrt_z[g_nearest_vertex]+1); // Top    right
		glVertex3i((g_mesh.mem->vrt_x[g_nearest_vertex]-10),(g_mesh.mem->vrt_y[g_nearest_vertex]+10), g_mesh.mem->vrt_z[g_nearest_vertex]+1); // Top    left
	glEnd();

	glBegin(GL_LINES);
		glLineWidth(5.0f);
		glColor4f(1.0f, 0.0f, 0.0f, 1);

		glVertex3i((g_mesh.mem->vrt_x[g_nearest_vertex]),(g_mesh.mem->vrt_y[g_nearest_vertex]), g_mesh.mem->vrt_z[g_nearest_vertex]-500);
		glVertex3i((g_mesh.mem->vrt_x[g_nearest_vertex]),(g_mesh.mem->vrt_y[g_nearest_vertex]), g_mesh.mem->vrt_z[g_nearest_vertex]+500);
	glEnd();

	int i;

	// Render all tiles in the selection
	glBegin(GL_QUADS);
		glColor4f(0.0f, 1.0f, 0.0f, 1);

		for (i = 0; i < g_mesh.mi->vert_count; i++)
		{
			if (g_selection.in_selection(i))
			{
				glVertex3i((g_mesh.mem->vrt_x[i]-10),(g_mesh.mem->vrt_y[i]-10), g_mesh.mem->vrt_z[i]+1); // Bottom left
				glVertex3i((g_mesh.mem->vrt_x[i]+10),(g_mesh.mem->vrt_y[i]-10), g_mesh.mem->vrt_z[i]+1); // Bottom right
				glVertex3i((g_mesh.mem->vrt_x[i]+10),(g_mesh.mem->vrt_y[i]+10), g_mesh.mem->vrt_z[i]+1); // Top    right
				glVertex3i((g_mesh.mem->vrt_x[i]-10),(g_mesh.mem->vrt_y[i]+10), g_mesh.mem->vrt_z[i]+1); // Top    left
			}
		}
	glEnd();

	// Now reset the colors
	glColor4f(1.0f,1.0f,1.0f,1.0f);
}


//---------------------------------------------------------------------
//-   Finally draw the scene to the screen
//---------------------------------------------------------------------
void c_renderer::draw_GL_Scene()
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
			this->m_fps = Frames / seconds;
//			cout << Frames << " frames in " << seconds << " seconds = " << this->fps << " FPS" << endl;
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
	int cnt, tnc, fan;

	// Reset the last used texture, so we HAVE to load a new one
	// for the first tiles
	g_mesh.mi->last_texture = -1;

	glEnable(GL_TEXTURE_2D);
	for (cnt = 0; cnt < 4; cnt++)
	{
		for (tnc = 0; tnc < m_renderlist.m_num_norm; tnc++)
		{
			fan = this->m_renderlist.m_norm[tnc];

			g_renderer.render_fan(fan);
		};
	}
	glDisable(GL_TEXTURE_2D);
}


//---------------------------------------------------------------------
//-   Render a single fan / tile
//---------------------------------------------------------------------
void c_renderer::render_fan(Uint32 fan)
{
	Uint16 tile = g_mesh.mem->tilelst[fan].tile; // Tile
	Uint16 type = g_mesh.mem->tilelst[fan].type; // Command type ( index to points in fan )
	// type <= 32 => small tile
	// type >  32 => big tile

	Uint16 commands;
	Uint16 vertices;
	Uint16 itexture;

	Uint16 cnt, tnc, entry, vertex;

	Uint32 badvertex;

	vect2 off;
	GLVertex v[MAXMESHVERTICES];

	if (tile == INVALID_TILE)
		return;

//	mesh_calc_normal_fan( pmesh, &(gs->phys), fan, &nrm, &pos );


	off.u = g_mesh.mem->gTileTxBox[tile].off.u;          // Texture offsets
	off.v = g_mesh.mem->gTileTxBox[tile].off.v;          //


	// Detect which tileset to load (1, 2, 3 or 4 (0 = particles))
	itexture = (tile >> 6 ) + TX_TILE_0;    // (1 << 6) == 64 tiles in each 256x256 texture

	vertices = g_tiledict[type].vrt_count;   // Number of vertices
	commands = g_tiledict[type].cmd_count;   // Number of commands

	// Original points
	badvertex = g_mesh.mem->tilelst[fan].vrt_start;   // Get big reference value


	// Change the texture if we need to
	if (itexture != g_mesh.mi->last_texture)
	{
		glBindTexture(GL_TEXTURE_2D, m_texture[itexture]);
		g_mesh.mi->last_texture = itexture;
	}


//	light_flat.r =
//	light_flat.g =
//	light_flat.b =
//	light_flat.a = 0.0f;

	// Fill in the vertex data from the mesh memory
	for (cnt = 0; cnt < vertices; cnt++)
	{
		v[cnt].pos.x = g_mesh.mem->vrt_x[badvertex];
		v[cnt].pos.y = g_mesh.mem->vrt_y[badvertex];
		v[cnt].pos.z = g_mesh.mem->vrt_z[badvertex];

//		v[cnt].col.r = FP8_TO_FLOAT( mem->vrt_lr_fp8[badvertex] + mem->vrt_ar_fp8[badvertex] );
//		v[cnt].col.g = FP8_TO_FLOAT( mem->vrt_lg_fp8[badvertex] + mem->vrt_ag_fp8[badvertex] );
//		v[cnt].col.b = FP8_TO_FLOAT( mem->vrt_lb_fp8[badvertex] + mem->vrt_ab_fp8[badvertex] );
//		v[cnt].col.a = 1.0f;


//		light_flat.r += v[cnt].col.r;
//		light_flat.g += v[cnt].col.g;
//		light_flat.b += v[cnt].col.b;
//		light_flat.a += v[cnt].col.a;


		badvertex++;
	}


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

	entry = 0;

	// Render each command
	for (cnt = 0; cnt < commands; cnt++)
	{
		glBegin(GL_TRIANGLE_FAN);

		for (tnc = 0; tnc < g_tiledict[type].cmd_size[cnt]; tnc++)
		{
			vertex = g_tiledict[type].vrt[entry];

//			if (this->gfxState.shading != GL_FLAT)
//				glColor4fv( v[vertex].col.v );

			glTexCoord2f(g_tiledict[type].tx[vertex].u + off.u, g_tiledict[type].tx[vertex].v + off.v);

			glVertex3fv(v[vertex].pos.v);

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
		for ( tnc = 0; tnc < g_tiledict[type].cmd_size[cnt]; tnc++ )
		{
			vertex = g_tiledict[type].vrt[entry];

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
	glLoadMatrixf(this->m_cam->m_projection.v);

	// Save the view matrix to the stack
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(this->m_cam->m_view.v);
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
	glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, 1, -1);   // Set up an orthogonal projection

//	glMatrixMode( GL_MODELVIEW );
//	glPushMatrix();
//	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glColor4f(1, 1, 1, 1);
}


void c_renderer::end_2D_mode()
{
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();

//	glMatrixMode( GL_MODELVIEW );
//	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}


void c_renderer::render_text()
{
	// Draw current FPS
	vect3 fps_pos;
	fps_pos.x = this->m_screen->w;
	fps_pos.y = 0;
	fps_pos.z = 0;

	ostringstream tmp_fps;
	tmp_fps << "FPS: " << (float)this->m_fps;

	this->render_text(tmp_fps.str(), fps_pos);


	// Draw current position
	vect3 pos_pos;
	pos_pos.x = this->m_screen->w;
	pos_pos.y = 35;
	pos_pos.z = 0;

	ostringstream tmp_pos;
	tmp_pos << "Position: (" << g_mesh.mem->vrt_x[g_nearest_vertex] << ", " << g_mesh.mem->vrt_y[g_nearest_vertex] << ")";

	this->render_text(tmp_pos.str(), pos_pos);
}


void c_renderer::render_text(string text, vect3 pos)
{
	GLuint fonttex;

	SDL_Color fontcolor = { 255, 0, 0, 0 };
	SDL_Surface *sText = TTF_RenderText_Blended(this->m_font, text.c_str(), fontcolor);

	glGenTextures(1, &fonttex);
	glBindTexture(GL_TEXTURE_2D, fonttex);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, sText->w, sText->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, sText->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	

	// Do the alpha channel
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fonttex);
	glColor3f(1.0f, 1.0f, 1.0f);

	int w = sText->w;
	int h = sText->h;

	// If the position is off the screen, add the height or width
	if (pos.x >= this->m_screen->w)
		pos.x -= w;

	if (pos.y >= this->m_screen->h)
		pos.y -= h;

	if (pos.x <= 0)
		pos.x += w;

	if (pos.y <= 0)
		pos.y += h;

	// Those coordinates have to be inverted on the Y axis
	glBegin(GL_QUADS);
		glTexCoord2d(1, 1); glVertex2f(pos.x + w, pos.y - h);
		glTexCoord2d(0, 1); glVertex2f(pos.x, pos.y - h);
		glTexCoord2d(0, 0); glVertex2f(pos.x, pos.y);
		glTexCoord2d(1, 0); glVertex2f(pos.x + w, pos.y);
	glEnd();


	glDeleteTextures(1, &fonttex);
	SDL_FreeSurface(sText);
}
