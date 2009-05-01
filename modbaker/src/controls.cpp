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
#include <string>

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


// TODO: Move somewhere else (maybe utility.cpp or global.cpp)
bool tokenize_semicolon(const string str, vector<string>& tokens)
{
	string token;
	istringstream iss(str);

	if (str.length() == 0)
	{
		cout << "tokenizer: input string is empty" << endl;
		return false;
	}

	while (getline(iss, token, ';'))
	{
		tokens.push_back(token);
	}
	return true;
}


//---------------------------------------------------------------------
//-   Gets called at mouse events
//---------------------------------------------------------------------
void c_input_handler::mousePressed(gcn::MouseEvent& event)
{
	string sid;
	int iid;
	iid = -1;

	vector<string> tokens;

	if (event.getButton() == LEFT) {
		if (window_input)
		{
			sid = event.getSource()->getId();

			if (sid.substr(0, 3) == "tex")
			{
				if (!tokenize_semicolon(sid, tokens))
				{
					cout << "WARNING: Could not tokenize " << sid << endl;
				}

				istringstream iss_set(tokens[1]);
				istringstream iss_y(tokens[2]);
				istringstream iss_x(tokens[3]);
				int texture;
				int set, y, x;
				iss_set >> set;
				iss_y >> y;
				iss_x >> x;

				// TODO: calculate the texture ID
				texture = (set * 64) + (y * 8) + x;

				g_selection.set_texture(texture);
				return;
			}

			// Get the widget ID string and convert it to integer
			istringstream sid;
			sid.str(event.getSource()->getId());
			sid >> iid;
			do_something(iid);
		}
		else
		{
			// TODO: move get_nearest_vertex / get_nearest_tile here
			if (!g_selection.get_add_mode())
				do_something(ACTION_SELECTION_CLEAR);

			switch (g_selection.get_selection_mode())
			{
				case SELECTION_MODE_VERTEX:
					g_selection.add_vertex(g_nearest_vertex);
					break;

				case SELECTION_MODE_TILE:
					g_selection.add_tile(g_nearest_tile);
					break;

				default:
					cout << "Invalid selection mode" << endl;
					break;
			}
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

	if (event.getKey().getValue() == 'b')
		do_something(ACTION_SELMODE_TILE);

	if (event.getKey().getValue() == 'n')
		do_something(ACTION_SELMODE_VERTEX);

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
	string filename;

	switch (p_action)
	{
		case ACTION_MESH_NEW:
			delete g_mesh;
			g_mesh = NULL;
			break;

		case ACTION_MESH_LOAD:
			filename = g_renderer->get_wm()->tf_load->getText();
			cout << filename << endl;
			if(g_mesh->load_mesh_mpd(filename))
			{
				g_renderer->get_wm()->set_load_visibility(false);
				g_renderer->load_basic_textures(filename);

				g_renderer->m_renderlist.build();
			}
			break;

		case ACTION_MESH_SAVE:
			filename = g_renderer->get_wm()->tf_load->getText();
			cout << filename << endl;
			if (g_mesh->save_mesh_mpd(g_config->get_egoboo_path() + "modules/" + g_mesh->modname + "/gamedat/" + filename))
				g_renderer->get_wm()->set_save_visibility(false);

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

		case ACTION_SELMODE_VERTEX:
			if (g_selection.get_selection_mode() != SELECTION_MODE_VERTEX)
				g_selection.clear();

			g_selection.set_selection_mode(SELECTION_MODE_VERTEX);
			break;

		case ACTION_SELMODE_TILE:
			if (g_selection.get_selection_mode() != SELECTION_MODE_TILE)
				g_selection.clear();

			g_selection.set_selection_mode(SELECTION_MODE_TILE);
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

		// Texture painting
		case ACTION_PAINT_TEXTURE:
			g_selection.change_texture();
			break;

		// Scrolling
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

		case SCROLL_Z_STOP:
			g_renderer->getPCam()->m_movez = 0.0f;
			break;

		// Guichan window handling
		case WINDOW_TEXTURE_TOGGLE:
			g_renderer->get_wm()->toggle_texture_visibility();
			break;

		case WINDOW_LOAD_SHOW:
			g_renderer->get_wm()->set_load_visibility(true);
			break;

		case WINDOW_SAVE_SHOW:
			g_renderer->get_wm()->set_save_visibility(true);
			break;

		case WINDOW_LOAD_HIDE:
			g_renderer->get_wm()->set_load_visibility(false);
			break;

		case WINDOW_SAVE_HIDE:
			g_renderer->get_wm()->set_save_visibility(false);
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
	g_nearest_tile   = g_mesh->get_nearest_tile(g_mouse_gl_x, g_mouse_gl_y, g_mouse_gl_z);

	return true;
}
