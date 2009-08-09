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
/// @brief Image definition
/// @details Definition of an image

#ifndef image_loader_h
#define image_loader_h

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#include <string>

using namespace std;

//---------------------------------------------------------------------
///   c_image definition
//---------------------------------------------------------------------
class c_image_loader
{
	private:
	protected:
		GLuint gl_image[1];      ///< an OpenGL image ID
		SDL_Surface *sdl_image; ///< a SDL surface
	public:
		bool load_image(string); ///< load the image
		c_image_loader();
		~c_image_loader();
};
#endif
