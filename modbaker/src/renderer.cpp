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

//---------------------------------------------------------------------
///   c_renderer constructor
//---------------------------------------------------------------------
c_renderer::c_renderer()
{
	this->render_mode = RENDER_NORMAL;
}


//---------------------------------------------------------------------
///   c_renderer destructor
//---------------------------------------------------------------------
c_renderer::~c_renderer()
{
//	glDeleteTextures(MAX_TEXTURES, m_texture);
}


//---------------------------------------------------------------------
///   Initialize the renderer
///   \param p_width screen width
///   \param p_height screen height
///   \param p_bpp screen color depth (bpp)
//---------------------------------------------------------------------
void c_renderer::init_renderer(int p_width, int p_height, int p_bpp)
{
	this->initSDL(p_width, p_height, p_bpp);
	this->initGL();
	this->resize_window(p_width, p_height);
}


//---------------------------------------------------------------------
///   Get the mesh render mode
///   \return render mode
//---------------------------------------------------------------------
int c_renderer::get_render_mode()
{
	return this->render_mode;
}

//---------------------------------------------------------------------
///   Set the mesh render mode
///   \param p_render_mode new render mode
//---------------------------------------------------------------------
void c_renderer::set_render_mode(int p_render_mode)
{
	this->render_mode = p_render_mode;
}


//---------------------------------------------------------------------
///   Render the tile flags
///   This gets called for every fan
///   \param p_fx flag to render
//---------------------------------------------------------------------
void c_renderer::render_tile_flag(Uint16 p_fx)
{
	// Render 1st is rendered in pink
	if ((this->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_REF)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(1, 0.5, 1, 1);
	}

	// Reflecting is rendered in bright blue
	if ((this->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_DRAWREF)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(0.5, 1, 0.5, 1);
	}

	// Slippy is rendered in green
	if ((this->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_SLIPPY)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(0, 1, 0, 1);
	}

	// Water is rendered in blue
	if ((this->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_WATER)))
	{
		glEnable(GL_BLEND);
		glColor4f(0, 0, 1, 1);
	}

	// Damage is rendered in red
	if ((this->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_DAMAGE)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(1, 0, 0, 1);
	}

	// Wall is rendered in violet
	if ((this->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_WALL)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(1, 0, 1, 1);
	}

	// Impassable is rendered in orange
	if ((this->get_render_mode() == RENDER_TILE_FLAGS) && (0 != (p_fx & MESHFX_IMPASS)))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(1, 0.5, 0, 1);
	}
}
