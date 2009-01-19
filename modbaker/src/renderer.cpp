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
#include "SDL_extensions.h"


#include <iostream>
#include <sstream>



//---------------------------------------------------------------------
//-   Global function stolen from Jonathan Fisher
//- who stole it from gl_font.c test program from SDL_ttf ;)
//---------------------------------------------------------------------
static int powerOfTwo( int input )
{
  int value = 1;

  while ( value < input )
  {
    value <<= 1;
  }
  return value;
}


//---------------------------------------------------------------------
//-   Global function stolen from Jonathan Fisher
//- who stole it from gl_font.c test program from SDL_ttf ;)
//---------------------------------------------------------------------
int copySurfaceToTexture( SDL_Surface *surface, GLuint texture, GLfloat *texCoords )
{
  int w, h;
  SDL_Surface *image;
  SDL_Rect area;
  Uint32  saved_flags;
  Uint8  saved_alpha;

  // Use the surface width & height expanded to the next powers of two
  w = powerOfTwo( surface->w );
  h = powerOfTwo( surface->h );
  texCoords[0] = 0.0f;
  texCoords[1] = 0.0f;
  texCoords[2] = ( GLfloat )surface->w / w;
  texCoords[3] = ( GLfloat )surface->h / h;

  image = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, amask );

  if ( image == NULL )
  {
    return 0;
  }

  // Save the alpha blending attributes
  saved_flags = surface->flags & ( SDL_SRCALPHA | SDL_RLEACCELOK );
  saved_alpha = surface->format->alpha;
  if ( ( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
  {
    SDL_SetAlpha( surface, 0, 0 );
  }

  // Copy the surface into the texture image
  area.x = 0;
  area.y = 0;
  area.w = surface->w;
  area.h = surface->h;
  SDL_BlitSurface( surface, &area, image, &area );

  // Restore the blending attributes
  if ( ( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
  {
    SDL_SetAlpha( surface, saved_flags, saved_alpha );
  }

  // Send the texture to OpenGL
  glBindTexture( GL_TEXTURE_2D, texture );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexImage2D( GL_TEXTURE_2D,  0, GL_RGBA,  w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,  image->pixels );

  // Don't need the extra image anymore
  SDL_FreeSurface( image );

  return 1;
}




//---------------------------------------------------------------------
//-   Global function stolen from Ben Birdsey ;)
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//-   Global function stolen from Ben Birdsey ;)
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//-   Constructor: Set the basic renderer values
//---------------------------------------------------------------------
c_renderer::c_renderer()
{
  m_fps = 0.0f;
  m_gfxState.ogl_vid.shading = GL_FLAT; // TODO: Read from game config

  initSDL();
  initGL();
  resize_window(WINDOW_WIDTH, WINDOW_HEIGHT);


  m_cam = new c_camera();
}


//---------------------------------------------------------------------
//-   Destructor: Cleanup
//---------------------------------------------------------------------
c_renderer::~c_renderer()
{
  glDeleteTextures(MAX_TEXTURES, m_texture);

  TTF_CloseFont(m_font);
}


//---------------------------------------------------------------------
//-   General SDL initialisation
//---------------------------------------------------------------------
void c_renderer::initSDL()
{
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
  if( TTF_Init() < 0)
  {
    cout << "ERROR: Unable to load the font handler: " << SDL_GetError() << endl;
    throw modbaker_exception("Unable to load the font handler");
  }
  else
  {
    atexit(TTF_Quit);

    char * pfname = "basicdat/Negatori.ttf";

    m_font = TTF_OpenFont(pfname, 16);
    if (NULL == m_font)
    {
      cout << "ERROR: Error loading font \"" << pfname << "\": " << TTF_GetError() << endl;
    }
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
  if( NULL == sdl_set_mode(NULL, &(m_gfxState.sdl_vid), NULL) )
  {
    cout << "I can't get SDL to set any video mode: " << SDL_GetError() << endl;
    throw modbaker_exception("I can't get SDL to set any video mode");
  }
}


//---------------------------------------------------------------------
//-   General OpenGL initialisation
//---------------------------------------------------------------------
void c_renderer::initGL()
{
  // Load the current graphical settings
  sdl_gl_set_mode(&(m_gfxState.ogl_vid));

  // glClear() stuff
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Set the background black
  glClearDepth( 1.0 );

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
  glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
  glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

}


//---------------------------------------------------------------------
//-   Reset our viewport after a window resize
//---------------------------------------------------------------------
void c_renderer::resize_window( int width, int height )
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
  load_texture("basicdat/globalparticles/particle.png",        TX_PARTICLE);
  load_texture("modules/" + modname + "/gamedat/tile0.bmp",    TX_TILE_0);
  load_texture("modules/" + modname + "/gamedat/tile1.bmp",    TX_TILE_1);
  load_texture("modules/" + modname + "/gamedat/tile2.bmp",    TX_TILE_2);
  load_texture("modules/" + modname + "/gamedat/tile3.bmp",    TX_TILE_3);
  load_texture("modules/" + modname + "/gamedat/watertop.bmp", TX_WATER_TOP);
  load_texture("modules/" + modname + "/gamedat/waterlow.bmp", TX_WATER_LOW);
}


//---------------------------------------------------------------------
//-   Load a single texture
//---------------------------------------------------------------------
bool c_renderer::load_texture(string imgname, int tileset)
{
  SDL_Surface *picture;
  SDL_Surface *img_temp;
  float tex_coords[4];

  picture = IMG_Load(imgname.c_str());

  glEnable( GL_TEXTURE_2D );
  glGenTextures(1, m_texture + tileset);
  if( copySurfaceToTexture(picture, m_texture[tileset], tex_coords) )
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

  glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex]-10),(g_mesh.mem->vrt_y[g_nearest_vertex]-10), g_mesh.mem->vrt_z[g_nearest_vertex]+1); // Bottom left
  glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex]+10),(g_mesh.mem->vrt_y[g_nearest_vertex]-10), g_mesh.mem->vrt_z[g_nearest_vertex]+1); // Bottom right
  glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex]+10),(g_mesh.mem->vrt_y[g_nearest_vertex]+10), g_mesh.mem->vrt_z[g_nearest_vertex]+1); // Top    right
  glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex]-10),(g_mesh.mem->vrt_y[g_nearest_vertex]+10), g_mesh.mem->vrt_z[g_nearest_vertex]+1); // Top    left
  glEnd();

  glBegin(GL_LINES);
  glLineWidth(5.0f);
  glColor4f(1.0f, 0.0f, 0.0f, 1);

  glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex]),(g_mesh.mem->vrt_y[g_nearest_vertex]), g_mesh.mem->vrt_z[g_nearest_vertex]-500);
  glVertex3f((g_mesh.mem->vrt_x[g_nearest_vertex]),(g_mesh.mem->vrt_y[g_nearest_vertex]), g_mesh.mem->vrt_z[g_nearest_vertex]+500);
  glEnd();

  int i;

  // Render all tiles in the selection
  glBegin(GL_QUADS);
  glColor4f(0.0f, 1.0f, 0.0f, 1);

  for (i = 0; i < g_mesh.mi->vert_count; i++)
  {
    if (g_selection.in_selection(i))
    {
      glVertex3f((g_mesh.mem->vrt_x[i]-10),(g_mesh.mem->vrt_y[i]-10), g_mesh.mem->vrt_z[i]+1); // Bottom left
      glVertex3f((g_mesh.mem->vrt_x[i]+10),(g_mesh.mem->vrt_y[i]-10), g_mesh.mem->vrt_z[i]+1); // Bottom right
      glVertex3f((g_mesh.mem->vrt_x[i]+10),(g_mesh.mem->vrt_y[i]+10), g_mesh.mem->vrt_z[i]+1); // Top    right
      glVertex3f((g_mesh.mem->vrt_x[i]-10),(g_mesh.mem->vrt_y[i]+10), g_mesh.mem->vrt_z[i]+1); // Top    left
    }
  }
  glEnd();

  // Now reset the colors
  glColor4f(1.0f,1.0f,1.0f,1.0f);
}

//---------------------------------------------------------------------
//-   Begin the frame
//---------------------------------------------------------------------
void c_renderer::begin_frame()
{
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glViewport( 0, 0, getPScreen()->w, getPScreen()->h );
}


//---------------------------------------------------------------------
//-   Finally draw the scene to the screen
//---------------------------------------------------------------------
void c_renderer::end_frame()
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
  int cnt, tnc;

  // Reset the last used texture, so we HAVE to load a new one
  // for the first tiles

  glEnable(GL_TEXTURE_2D);
  for (cnt = 0; cnt < 4; cnt++)
  {
    for (tnc = 0; tnc < m_renderlist.m_num_norm; tnc++)
    {
      Uint32 fan = m_renderlist.m_norm[tnc];
      Uint16 tile = g_mesh.mem->tilelst[fan].tile; // Tile

      // do not render invalid tiles
      if (tile == INVALID_TILE) continue;

      // keep one texture in memory as long as possible
      if((tile >> 6) == cnt)
      {
        g_renderer.render_fan(fan);
      }
    };
  }
}


//---------------------------------------------------------------------
//-   Render a single fan / tile
//---------------------------------------------------------------------
void c_renderer::render_fan(Uint32 fan)
{
  Uint16 commands;
  Uint16 vertices;
  Uint16 itexture;

  Uint16 cnt, tnc, entry, vertex;

  Uint32 badvertex;

  vect2 off;
  GLVertex v[MAXMESHVERTICES];

  Uint16 tile = g_mesh.mem->tilelst[fan].tile; // Tile
  Uint16 type = g_mesh.mem->tilelst[fan].type; // Command type ( index to points in fan )
  // type <= 32 => small tile
  // type >  32 => big tile

  if (tile == INVALID_TILE)
    return;

  //	mesh_calc_normal_fan( pmesh, &(gs->phys), fan, &nrm, &pos );

  off = g_mesh.mem->gTileTxBox[tile].off;          // Texture offsets

  // Detect which tileset to load (1, 2, 3 or 4 (0 = particles))
  itexture = (tile >> 6 ) + TX_TILE_0;    // (1 << 6) == 64 tiles in each 256x256 texture

  // Change the texture if we need to
  GLint icurrent_tx;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &icurrent_tx);
  if (icurrent_tx != m_texture[itexture])
  {
    glBindTexture(GL_TEXTURE_2D, m_texture[itexture]);
  }


  //	light_flat.r =
  //	light_flat.g =
  //	light_flat.b =
  //	light_flat.a = 0.0f;

  vertices  = g_tiledict[type].vrt_count;            // Number of vertices
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
  commands = g_tiledict[type].cmd_count;            // Number of commands
  glColor3f(1,1,1);
  for (cnt = 0, entry = 0; cnt < commands; cnt++)
  {
    glBegin(GL_TRIANGLE_FAN);

    for (tnc = 0; tnc < g_tiledict[type].cmd_size[cnt]; tnc++)
    {
      vertex = g_tiledict[type].vrt[entry] + badvertex;   // Get big reference value;

      glTexCoord2f(g_tiledict[type].tx[vertex].u + off.u, g_tiledict[type].tx[vertex].v + off.v);

      glVertex3f(g_mesh.mem->vrt_x[vertex], g_mesh.mem->vrt_y[vertex], g_mesh.mem->vrt_z[vertex]);

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
  glOrtho( 0, getPScreen()->w, getPScreen()->h, 0, -1, 1 );   // Set up an orthogonal projection

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();

  glDisable( GL_DEPTH_TEST );
  glDisable( GL_CULL_FACE );

  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glColor4f(1, 1, 1, 1);
}


void c_renderer::end_2D_mode()
{
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
}


void c_renderer::render_text()
{
  // Draw current FPS
  vect3 fps_pos;
  fps_pos.x = getPScreen()->w;
  fps_pos.y = 0;
  fps_pos.z = 0;

  ostringstream tmp_fps;
  tmp_fps << "FPS: " << (float)m_fps;

  glColor3f( 1, 1, 1 );
  render_text(tmp_fps.str(), fps_pos, JRIGHT);

  // Draw current position
  vect3 pos_pos;
  pos_pos.x = getPScreen()->w;
  pos_pos.y = 35;
  pos_pos.z = 0;

  ostringstream tmp_pos;
  tmp_pos << "Position: (" << g_mesh.mem->vrt_x[g_nearest_vertex] << ", " << g_mesh.mem->vrt_y[g_nearest_vertex] << ")";

  render_text(tmp_pos.str(), pos_pos, JRIGHT);
}


void c_renderer::render_text(string text, vect3 pos, c_renderer::TMODE mode)
{
  SDL_Surface *textSurf;
  SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };
  static GLuint font_tex = ~0;
  static float  font_coords[4];

  // Let TTF render the text
  textSurf = TTF_RenderText_Blended( m_font, text.c_str(), color );

  glEnable( GL_TEXTURE_2D );

  // Does this font already have a texture?  If not, allocate it here
  if(~0 == font_tex)
  {
    glGenTextures( 1, &font_tex );
  }

  // Copy the surface to the texture
  if ( copySurfaceToTexture( textSurf, font_tex, font_coords ) )
  {
    float xl, xr;
    float yt, yb;

    if(mode == JLEFT)
    {
      xl = pos.x;
      xr = pos.x + textSurf->w;
      yb = pos.y;
      yt = pos.y + textSurf->h;
    }
    else
    {
      xl = pos.x - textSurf->w;
      xr = pos.x;
      yb = pos.y;
      yt = pos.y + textSurf->h;
    }

    // And draw the darn thing
    glBegin( GL_TRIANGLE_STRIP );
    glTexCoord2f( font_coords[0], font_coords[1] ); glVertex2f( xl, yb );
    glTexCoord2f( font_coords[2], font_coords[1] ); glVertex2f( xr, yb );
    glTexCoord2f( font_coords[0], font_coords[3] ); glVertex2f( xl, yt );
    glTexCoord2f( font_coords[2], font_coords[3] ); glVertex2f( xr, yt );
    glEnd();
  }

  // Done with the surface
  SDL_FreeSurface( textSurf );
}
