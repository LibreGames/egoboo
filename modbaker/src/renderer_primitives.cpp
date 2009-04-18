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

#include "SDL_GL_extensions.h"
#include "ogl_debug.h"

#include <iostream>
#include <sstream>

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
	if (~0 == (int)font_tex)
	{
		glGenTextures( 1, &font_tex );
	}

	// Copy the surface to the texture
	if ( copySurfaceToTexture( textSurf, font_tex, font_coords ) )
	{
		float xl, xr;
		float yt, yb;

		if (mode == JLEFT)
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


//---------------------------------------------------------------------
//-   Render one model
//---------------------------------------------------------------------
void c_renderer::render_models()
{
	unsigned int i;
	vect3 pos;

	glDisable(GL_TEXTURE_2D);

	for (i=0; i<g_spawn_manager.spawns.size(); i++)
	{
		pos = g_spawn_manager.get_spawn(i).pos;

		pos.x =  pos.x * (1 << 7);
		pos.y =  pos.y * (1 << 7);
//		pos.z =  pos.z * (1 << 7);
		pos.z =  1.0f  * (1 << 7);

		glBegin(GL_QUADS);
		glColor4f(0.0f, 0.0f, 1.0f, 1.0f);

		glVertex3i(pos.x-10, pos.y-10, pos.z); // Bottom left
		glVertex3i(pos.x+10, pos.y-10, pos.z); // Bottom right
		glVertex3i(pos.x+10, pos.y+10, pos.z); // Top    right
		glVertex3i(pos.x-10, pos.y+10, pos.z); // Top    left
		glEnd();

		// Reset the colors
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	glEnable(GL_TEXTURE_2D);
}
