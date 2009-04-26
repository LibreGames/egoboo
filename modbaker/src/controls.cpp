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
	window_input = true;
}

//---------------------------------------------------------------------
//-   Gets called at mouse events
//---------------------------------------------------------------------
void c_input_handler::mouseMoved(gcn::MouseEvent& event)
{
	if (event.isConsumed())
		window_input = true;
	else
		window_input = false;

	g_mouse_x = event.getX();
	g_mouse_y = event.getY();
	g_renderer->get_wm()->set_position(g_mouse_x, g_mouse_y);
}


//---------------------------------------------------------------------
//-   Gets called at mouse events
//---------------------------------------------------------------------
void c_input_handler::mousePressed(gcn::MouseEvent& event)
{
	int iid;
	iid = -1;

	if (event.getButton() == LEFT) {
		if (window_input)
		{
			// Get the widget ID string and convert it to integer
			istringstream sid;
			sid.str(event.getSource()->getId());
			sid >> iid;
			do_something(iid);
		}
		else
		{
			if (!g_selection.get_add_mode())
				do_something(ACTION_SELECTION_CLEAR);

			g_selection.add_vertex(g_nearest_vertex);
		}
	}
}


//---------------------------------------------------------------------
//-   Gets called at key events
//---------------------------------------------------------------------
void c_input_handler::keyPressed(gcn::KeyEvent& event)
{
	if (event.getKey() == gcn::Key::ESCAPE)
		do_something(ACTION_EXIT);

	// Don't overwrite the input for the GUI
	if (window_input)
		return;

	// TODO: A switch ... case should do it, too
	if (event.getKey().getValue() == 'o')
		do_something(ACTION_PAINT_TEXTURE);

	if (event.getKey().getValue() == 'w')
		do_something(ACTION_VERTEX_UP);

	if (event.getKey().getValue() == 's')
		do_something(ACTION_VERTEX_DOWN);

	if (event.getKey().getValue() == 'a')
		do_something(ACTION_VERTEX_LEFT);

	if (event.getKey().getValue() == 'd')
		do_something(ACTION_VERTEX_RIGHT);

	if (event.getKey().getValue() == 'q')
		do_something(ACTION_VERTEX_INC);

	if (event.getKey().getValue() == 'r')
		do_something(ACTION_VERTEX_DEC);

	if (event.getKey() == gcn::Key::UP)
		do_something(SCROLL_UP);

	if (event.getKey() == gcn::Key::LEFT)
		do_something(SCROLL_LEFT);

	if (event.getKey() == gcn::Key::RIGHT)
		do_something(SCROLL_RIGHT);

	if (event.getKey() == gcn::Key::DOWN)
		do_something(SCROLL_DOWN);

	if (event.getKey() == gcn::Key::PAGE_UP)
		do_something(SCROLL_INC);

	if (event.getKey() == gcn::Key::PAGE_DOWN)
		do_something(SCROLL_DEC);

	if (event.getKey() == gcn::Key::LEFT_SHIFT)
		do_something(ACTION_MODIFIER_ON);

	if (event.getKey() == gcn::Key::RIGHT_SHIFT)
		do_something(ACTION_MODIFIER_ON);

	if (event.getKey() == gcn::Key::SPACE)
		do_something(ACTION_SELECTION_CLEAR);
}


//---------------------------------------------------------------------
//-   Gets called at key events
//---------------------------------------------------------------------
void c_input_handler::keyReleased(gcn::KeyEvent& event)
{

	if (event.getKey() == gcn::Key::UP)
		do_something(SCROLL_Y_STOP);

	if (event.getKey() == gcn::Key::DOWN)
		do_something(SCROLL_Y_STOP);

	if (event.getKey() == gcn::Key::LEFT)
		do_something(SCROLL_X_STOP);

	if (event.getKey() == gcn::Key::RIGHT)
		do_something(SCROLL_X_STOP);

	if (event.getKey() == gcn::Key::PAGE_UP)
		do_something(SCROLL_Z_STOP);

	if (event.getKey() == gcn::Key::PAGE_DOWN)
		do_something(SCROLL_Z_STOP);


	// Don't overwrite the input for the GUI
	if (window_input)
		return;


	// TODO: A switch ... case should do it, too
	if (event.getKey() == gcn::Key::LEFT_SHIFT)
		do_something(ACTION_MODIFIER_OFF);

	if (event.getKey() == gcn::Key::RIGHT_SHIFT)
		do_something(ACTION_MODIFIER_OFF);
}


//---------------------------------------------------------------------
//-   This function controls all actions
//---------------------------------------------------------------------
void c_input_handler::do_something(int p_action)
{
	switch (p_action)
	{
		case ACTION_MESH_NEW:
			delete g_mesh;
			g_mesh = NULL;
			break;

		case ACTION_MESH_LOAD:
			g_renderer->get_wm()->set_filename_visibility(true);
			g_mesh->load_mesh_mpd("palice.mod");
			break;

		case ACTION_MESH_SAVE:
			g_mesh->save_mesh_mpd(g_config->get_egoboo_path() + "modules/" + g_mesh->modname + "/gamedat/level.mpd");
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

		case ACTION_VERTEX_INC:
			g_selection.modify_z(50.0f);
			break;

		case ACTION_VERTEX_DEC:
			g_selection.modify_z(-50.0f);
			break;

		case ACTION_SELECTION_CLEAR:
			g_selection.clear();
			break;

		case ACTION_EXIT:
			// TODO: Set c_modbaker->done to true
			Quit();
			break;

		case ACTION_MODIFIER_ON:
			g_renderer->getPCam()->m_factor = SCROLLFACTOR_FAST;
			g_selection.set_add_mode(true);
			break;

		case ACTION_MODIFIER_OFF:
			g_renderer->getPCam()->m_factor = SCROLLFACTOR_SLOW;
			g_selection.set_add_mode(false);
			break;

		case ACTION_PAINT_TEXTURE:
			g_selection.change_texture(0, 0);
			break;

		case SCROLL_RIGHT:
			g_renderer->getPCam()->m_movex += 5.0f;
			break;

		case SCROLL_LEFT:
			g_renderer->getPCam()->m_movex -= 5.0f;
			break;

		case SCROLL_UP:
			g_renderer->getPCam()->m_movey += 5.0f;
			break;

		case SCROLL_DOWN:
			g_renderer->getPCam()->m_movey -= 5.0f;
			break;

		case SCROLL_INC:
			g_renderer->getPCam()->m_movez += 5.0f;
			break;

		case SCROLL_DEC:
			g_renderer->getPCam()->m_movez -= 5.0f;
			break;

		case SCROLL_X_STOP:
			g_renderer->getPCam()->m_movex = 0.0f;
			break;

		case SCROLL_Y_STOP:
			g_renderer->getPCam()->m_movey = 0.0f;
			break;

		case WINDOW_TEXTURE_TOGGLE:
			g_renderer->get_wm()->toggle_texture_visibility();
			break;

		case WINDOW_LOAD_SHOW:
			g_renderer->get_wm()->set_filename_visibility(true);
			break;

		case WINDOW_SAVE_SHOW:
			g_renderer->get_wm()->set_filename_visibility(true);
			break;
	}
}


//---------------------------------------------------------------------
//-   Handle input events
//---------------------------------------------------------------------
int c_modbaker::handle_events()
{
	SDL_Event event;

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

			default:
				break;
		}

		// Guichan events
		g_renderer->get_wm()->input->pushInput(event);
	}

	return 0;
}


//---------------------------------------------------------------------
//-   Get the 3d coordinates related to the 2d position of the cursor
//---------------------------------------------------------------------
bool c_modbaker::get_GL_pos(int x, int y)
{
	if (g_mesh == NULL)
		return false;

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
	g_nearest_vertex = g_mesh->get_nearest_vertex(g_mouse_gl_x, g_mouse_gl_y, g_mouse_gl_z);

	return true;
}
