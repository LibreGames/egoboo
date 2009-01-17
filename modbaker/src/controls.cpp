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
//---------------------------------------------------------------------
//-
//-   Everything regarding the input
//-
//---------------------------------------------------------------------
#include <iostream>

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "global.h"
#include "modbaker.h"
#include "mesh.h"
#include "renderer.h"
#include "edit.h"


//---------------------------------------------------------------------
//-   Handle window events (focus, resize, ...)
//---------------------------------------------------------------------
int c_modbaker::handle_window_events()
{
	SDL_Event event;

	// handle the events in the queue
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			/* Something's happend with our focus
			* If we lost focus or we are iconified, we
			* shouldn't draw the screen
			*/
			case SDL_ACTIVEEVENT:
				if (event.active.gain == 0)
					this->active = false;
				else
					this->active = true;
				break;			    


			// Window resize
			case SDL_VIDEORESIZE:
				g_renderer.m_screen = SDL_SetVideoMode(event.resize.w, event.resize.h, SCREEN_BPP, g_renderer.m_videoFlags);
				if (!g_renderer.m_screen)
				{
					cout << "Could not get a surface after resize: " << SDL_GetError() << endl;
					Quit();
				}
				g_renderer.resize_window(event.resize.w, event.resize.h);
				break;


			// Quit request
			case SDL_QUIT:
				this->done = true;
				break;

			default:
				break;
		}
	}

	return 0;
}


//---------------------------------------------------------------------
//-   Handle game events (movement, editing, ...)
//---------------------------------------------------------------------
int c_modbaker::handle_game_events()
{
	SDL_Event event;

	// handle the events in the queue
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			// Keyboard input
			// handle key presses
			case SDL_KEYDOWN:
				handle_key_press(&event.key.keysym);
				break;

			// handle key releases
			case SDL_KEYUP:
				handle_key_release(&event.key.keysym);
				break;


			// Mouse input
			case SDL_MOUSEMOTION:
				g_mouse_x = event.motion.x;
				g_mouse_y = event.motion.y;
				break;

			// TODO: Move to extra function (like key presses)
			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					if (!this->selection_add)
					{
						g_selection.clear();
					}

					g_selection.add_vertex(g_nearest_vertex);
				}
				break;


			default:
				break;
		}
	}

	return 0;
}


//---------------------------------------------------------------------
//-   Function to handle key press events                             -
//---------------------------------------------------------------------
void c_modbaker::handle_key_press(SDL_keysym *keysym)
{
	switch (keysym->sym)
	{
		// System controls
		case SDLK_ESCAPE:
			Quit();
			break;

		case SDLK_F1:
			SDL_WM_ToggleFullScreen(g_renderer.m_screen);
			break;

		// Camera control
		case SDLK_RIGHT:
			g_renderer.m_cam->m_movex += 5.0f;
			break;

		case SDLK_LEFT:
			g_renderer.m_cam->m_movex -= 5.0f;
			break;

		case SDLK_UP:
			g_renderer.m_cam->m_movey += 5.0f;
			break;

		case SDLK_DOWN:
			g_renderer.m_cam->m_movey -= 5.0f;
			break;

		case SDLK_PAGEUP:
			g_renderer.m_cam->m_movez += 5.0f;
			break;

		case SDLK_PAGEDOWN:
			g_renderer.m_cam->m_movez -= 5.0f;
			break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			g_renderer.m_cam->m_factor = SCROLLFACTOR_FAST;
			this->selection_add = true;
			break;

		// Editing
		case SDLK_w:
			g_selection.modify(50.0f);
			break;

		case SDLK_s:
			g_selection.modify(-50.0f);
			break;

		case SDLK_SPACE:
			g_selection.clear();
			break;

		// Save the file again
		case SDLK_F10:
			g_mesh.save_mesh_mpd("level.mpd");
			break;

		default:
			break;
	}
}


//---------------------------------------------------------------------
//-   Function to handle key release events
//---------------------------------------------------------------------
void c_modbaker::handle_key_release(SDL_keysym *keysym)
{
	switch (keysym->sym)
	{
		// Camera control
		case SDLK_RIGHT:
			g_renderer.m_cam->m_movex = 0.0f;
			break;

		case SDLK_LEFT:
			g_renderer.m_cam->m_movex = 0.0f;
			break;

		case SDLK_DOWN:
			g_renderer.m_cam->m_movey = 0.0f;
			break;

		case SDLK_UP:
			g_renderer.m_cam->m_movey = 0.0f;
			break;

		case SDLK_PAGEUP:
			g_renderer.m_cam->m_movez = 0.0f;
			break;

		case SDLK_PAGEDOWN:
			g_renderer.m_cam->m_movez = 0.0f;
			break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			g_renderer.m_cam->m_factor = SCROLLFACTOR_SLOW;
			this->selection_add = false;
			break;

		default:
			break;
	}
}


//---------------------------------------------------------------------
//-   Get the 3d coordinates related to the 2d position of the cursor
//---------------------------------------------------------------------
void c_modbaker::get_GL_pos(int x, int y)
{
	GLdouble pos3D_x, pos3D_y, pos3D_z;

	float z;

	// arrays to hold matrix information
	GLdouble model_view[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, model_view);

	GLdouble projection[16];
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Get the window z coorinate
	glReadPixels(x, viewport[3]-y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);

	// get 3D coordinates based on window coordinates
	gluUnProject(x, viewport[3]-y, z, model_view, projection, viewport, &pos3D_x, &pos3D_y, &pos3D_z);

	g_mouse_gl_x = (float)pos3D_x;
	g_mouse_gl_y = (float)pos3D_y;
	g_mouse_gl_z = (float)pos3D_z;

	// Calculate the nearest vertex
	g_nearest_vertex = g_mesh.get_nearest_vertex(g_mouse_gl_x, g_mouse_gl_y, g_mouse_gl_z);
}
