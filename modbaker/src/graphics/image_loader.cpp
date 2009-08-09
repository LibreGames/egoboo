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
/// @brief Image implementation
/// @details Implementation of an image

#include "image_loader.h"

#include "../SDL_extensions.h"
#include "../ogl_extensions.h"
#include "../SDL_GL_extensions.h"
#include "../ogl_debug.h"

#include <iostream>
using namespace std;

//---------------------------------------------------------------------
///   Load a single icon for a game object
///   \param imgname name of the image
///   \return true on success, else false
///   \todo Do we need GL_TEXTURE_2D here?
//---------------------------------------------------------------------
bool c_image_loader::load_image(string p_filename)
{
	float tex_coords[4];

	this->sdl_image = IMG_Load(p_filename.c_str());

	// Generate an OpenGL texture
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, this->gl_image);

	cout <<  "INFO: icon: " << this->gl_image << " (\"" << p_filename << "\")" << endl;

	if (this->sdl_image == NULL)
	{
		this->gl_image[0] = 0;
		return false;
	}

	// Copy the surface to a texture
	copySurfaceToTexture(this->sdl_image, this->gl_image[0], tex_coords);

	return true;
}


//---------------------------------------------------------------------
///   c_image_loader constructor
//---------------------------------------------------------------------
c_image_loader::c_image_loader()
{
	this->sdl_image = NULL;
	this->gl_image[0] = 0;
}


//---------------------------------------------------------------------
///   c_image_loader destructor
//---------------------------------------------------------------------
c_image_loader::~c_image_loader()
{
	if (this->sdl_image == NULL)
	{
		// Free the SDL_Surface
		SDL_FreeSurface(this->sdl_image);
		this->sdl_image = NULL;
	}

	this->gl_image[0] = 0;
}
