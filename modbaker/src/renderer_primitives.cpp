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


//---------------------------------------------------------------------
///   Render all models
///   \param p_obj_manager object manager containing the objects
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_renderer::render_models(c_object_manager *p_obj_manager)
{
	if (p_obj_manager == NULL)
		return false;

	unsigned int i;
	int slot;
	vect3 pos;

	for (i=0; i<p_obj_manager->get_spawns_size(); i++)
	{
		pos = p_obj_manager->get_spawn(i).pos;

		// Get the object slot number
		slot = p_obj_manager->get_spawn(i).slot_number;

		if (p_obj_manager->get_object_by_slot(slot) != NULL)
		{
			p_obj_manager->get_object_by_slot(slot)->render();

			if (p_obj_manager->get_object_by_slot(slot)->get_icon() == 0)
			{
				// No icon found: Render a blue box
				glDisable(GL_TEXTURE_2D);
				glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
			}
			else
			{
				// Load the icon
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, p_obj_manager->get_object_by_slot(slot)->get_icon());
			}
			pos.x =  pos.x * (1 << 7);
			pos.y =  pos.y * (1 << 7);
//			pos.z =  pos.z * (1 << 7);
			pos.z =  1.0f  * (1 << 7);

			// Highlight the object if it is in the selection
			if ((g_selection.get_selection_mode() == SELECTION_MODE_OBJECT) &&
				(g_selection.in_selection((int)i)))
			{
				glEnable(GL_TEXTURE_2D);
				glColor4f(0, 1, 0, 1);
			}

			glBegin(GL_QUADS);
				glTexCoord2f(0, 1);
				glVertex3f(pos.x-30, pos.y-30, pos.z); // Bottom left
				glTexCoord2f(1, 1);
				glVertex3f(pos.x+30, pos.y-30, pos.z); // Bottom right
				glTexCoord2f(1, 0);
				glVertex3f(pos.x+30, pos.y+30, pos.z); // Top    right
				glTexCoord2f(0, 0);
				glVertex3f(pos.x-30, pos.y+30, pos.z); // Top    left
			glEnd();

			// Reset the colors
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	glEnable(GL_TEXTURE_2D);

	return true;
}


//---------------------------------------------------------------------
///   Unused
///   \return false
//---------------------------------------------------------------------
bool c_object::render()
{
	return false;
}
