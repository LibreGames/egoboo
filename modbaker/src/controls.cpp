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
#include "global.h"
#include "modbaker.h"
#include "mesh.h"
#include "renderer.h"
#include "edit.h"

#include "SDL_extensions.h"
#include "ogl_debug.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <iostream>
#include <sstream>

using namespace std;


c_input_handler::c_input_handler()
{
}

//---------------------------------------------------------------------
//-   Gets called at mouse events
//---------------------------------------------------------------------
void c_input_handler::mousePressed(gcn::MouseEvent& event)
{
	this->gui_clicked = false;

	if (event.isConsumed())
		this->gui_clicked = true;

	if (event.getButton() == LEFT) {
		if (this->gui_clicked)
		{
			// Get the string and convert it into an integer
			istringstream sid;
			int iid;
			sid.str(event.getSource()->getId());
			sid >> iid;
			do_something(iid);
		}
		else
		{
			if (!g_selection.get_add_mode())
			{
				g_selection.clear();
			}

			g_selection.add_vertex(g_nearest_vertex);
		}
	}
}


//---------------------------------------------------------------------
//-   Gets called at mouse events
//---------------------------------------------------------------------
void c_input_handler::do_something(int p_action)
{
	switch (p_action)
	{
		case ACTION_MESH_NEW:
			break;

		case ACTION_MESH_LOAD:
			break;

		case ACTION_MESH_SAVE:
			g_mesh.save_mesh_mpd(g_config.get_egoboo_path() + "modules/" + g_mesh.modname + "/gamedat/level.mpd");
			break;

		// Vertex editing
		case ACTION_VERTEX_UP:
			g_selection.modify_y(10.0f);
			break;

		case ACTION_VERTEX_LEFT:
			g_selection.modify_x(-10.0f);
			break;

		case ACTION_VERTEX_RIGHT:
			g_selection.modify_x(10.0f);
			break;

		case ACTION_VERTEX_DOWN:
			g_selection.modify_y(-10.0f);
			break;
	}
}


//---------------------------------------------------------------------
//-   Gets called at mouse events
//---------------------------------------------------------------------
void c_input_handler::mouseMoved(gcn::MouseEvent& event)
{
	g_mouse_x = event.getX();
	g_mouse_y = event.getY();
	g_renderer.m_wm.set_position(g_mouse_x, g_mouse_y);
}


//---------------------------------------------------------------------
//-   Handle input events
//---------------------------------------------------------------------
int c_modbaker::handle_events()
{
	// handle the events in the queue
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_ACTIVEEVENT:
				// Something's happend with our focus
				// If we lost focus or we are iconified, we
				// shouldn't draw the screen
				active = ((event.active.state == SDL_APPMOUSEFOCUS) ||
						  (event.active.state == SDL_APPINPUTFOCUS) ||
						  (event.active.state == SDL_APPACTIVE)) &&
						 event.active.gain == 1;
				break;

			case SDL_VIDEOEXPOSE:
				active = true;
				break;

				// Quit request
			case SDL_QUIT:
				done = true;
				break;

				// Keyboard input
				// handle key presses
			case SDL_KEYDOWN:
				handle_key_press(&event.key.keysym);
				break;

				// handle key releases
			case SDL_KEYUP:
				handle_key_release(&event.key.keysym);
				break;

			default:
				break;
		}

		// Guichan events
		g_renderer.m_wm.input->pushInput(event);
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

			// Camera control
		case SDLK_RIGHT:
			g_renderer.getPCam()->m_movex += 5.0f;
			break;

		case SDLK_LEFT:
			g_renderer.getPCam()->m_movex -= 5.0f;
			break;

		case SDLK_UP:
			g_renderer.getPCam()->m_movey += 5.0f;
			break;

		case SDLK_DOWN:
			g_renderer.getPCam()->m_movey -= 5.0f;
			break;

		case SDLK_PAGEUP:
			g_renderer.getPCam()->m_movez += 5.0f;
			break;

		case SDLK_PAGEDOWN:
			g_renderer.getPCam()->m_movez -= 5.0f;
			break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			g_renderer.getPCam()->m_factor = SCROLLFACTOR_FAST;
			g_selection.set_add_mode(true);
			break;

			// Editing
		case SDLK_w:
			g_input_handler.do_something(ACTION_VERTEX_UP);
			break;

		case SDLK_s:
			g_input_handler.do_something(ACTION_VERTEX_DOWN);
			break;

		case SDLK_a:
			g_input_handler.do_something(ACTION_VERTEX_LEFT);
			break;

		case SDLK_d:
			g_input_handler.do_something(ACTION_VERTEX_RIGHT);
			break;

		case SDLK_q:
			g_selection.modify_z(50.0f);
			break;

		case SDLK_e:
			g_selection.modify_z(-50.0f);
			break;

		case SDLK_o:
			g_selection.change_texture(0, 0);
			break;

		case SDLK_SPACE:
			g_selection.clear();
			break;

			// Save the file again
		case SDLK_F10:
			g_input_handler.do_something(ACTION_MESH_SAVE);
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
			g_renderer.getPCam()->m_movex = 0.0f;
			break;

		case SDLK_LEFT:
			g_renderer.getPCam()->m_movex = 0.0f;
			break;

		case SDLK_DOWN:
			g_renderer.getPCam()->m_movey = 0.0f;
			break;

		case SDLK_UP:
			g_renderer.getPCam()->m_movey = 0.0f;
			break;

		case SDLK_PAGEUP:
			g_renderer.getPCam()->m_movez = 0.0f;
			break;

		case SDLK_PAGEDOWN:
			g_renderer.getPCam()->m_movez = 0.0f;
			break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			g_renderer.getPCam()->m_factor = SCROLLFACTOR_SLOW;
			g_selection.set_add_mode(false);
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
	glReadPixels(x, viewport[3] - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);

	// get 3D coordinates based on window coordinates
	gluUnProject(x, viewport[3] - y, z, model_view, projection, viewport, &pos3D_x, &pos3D_y, &pos3D_z);

	g_mouse_gl_x = (float)pos3D_x;
	g_mouse_gl_y = (float)pos3D_y;
	g_mouse_gl_z = (float)pos3D_z;

	// Calculate the nearest vertex
	g_nearest_vertex = g_mesh.get_nearest_vertex(g_mouse_gl_x, g_mouse_gl_y, g_mouse_gl_z);
}
