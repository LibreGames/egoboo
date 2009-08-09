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
/// @brief ModBaker implementation
/// @details Implementation of the logic between all core elements


#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_types.h>
#include <SDL_endian.h>

#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
#include <guichan/opengl/openglsdlimageloader.hpp>
#include <guichan/opengl/openglimage.hpp>

#include "general.h"
#include "modbaker.h"
#include "global.h"

#include <iostream>
#include <stdlib.h>
#include <string>

using namespace std;

//---------------------------------------------------------------------
///   The main function
///   \param argc the parameters
///   \param argv number of parameters
///   \return error code or 0 on success
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
	cout << "entering main" << endl;
	c_modbaker modbaker;

	string modname;

	modname = "advent.mod";

	// Read the module name from the command line
	if (argc >= 2)
	{
		modname = argv[1];
	}

	modbaker.init(modname);
	modbaker.main_loop();

	return 0;
}


//---------------------------------------------------------------------
///   Class constructor
//---------------------------------------------------------------------
c_modbaker::c_modbaker()
{
	// Reset the global objects
	g_config            = NULL;
//	g_frustum           = NULL;
}


//---------------------------------------------------------------------
///   Class destructor
//---------------------------------------------------------------------
c_modbaker::~c_modbaker()
{
}


//---------------------------------------------------------------------
///   Initializes everything
///   \param p_modname module name to start with
//---------------------------------------------------------------------
void c_modbaker::init(string p_modname)
{
	g_config   = new c_config();

	// Load the renderer
	this->load_basic_textures(p_modname);

	this->init_renderer(g_config->get_width(), g_config->get_height(), 32);

	// Setup the GUI
	this->input = new gcn::SDLInput();

	this->init_wm(g_config->get_width(), g_config->get_height());

	this->top->addMouseListener(this);
	this->top->addKeyListener(this);

	this->gui->setInput(this->input);

	this->create_all_windows();
	this->load_complete_module(g_config->get_egoboo_path(), p_modname);

	this->build_renderlist();

	// Set the module name
	this->modname = p_modname;
}


//---------------------------------------------------------------------
///   Main loop
//---------------------------------------------------------------------
void c_modbaker::main_loop()
{
	this->reset_camera();

	while ( !done )
	{
		if ( active )
		{
			this->begin_frame();

			// 3D mode
			this->begin_3D_mode();
			this->move_camera(this->get_fps());
			this->render_mesh();
			this->render_positions();
			this->render_models(); // Render spawn.txt
			get_GL_pos(this->mouse_x, this->mouse_y);
			this->end_3D_mode();

			// Update the position in the info window
			this->set_info_position(g_mouse_gl_x, g_mouse_gl_y);

			// Draw the GUI
			gui->logic();
			gui->draw();

			this->end_frame();  // Finally render the scene
			this->set_info_fps(this->get_fps());
		}

		// Handle all events
		this->handle_events();
	}
}


//---------------------------------------------------------------------
///   This function controls all actions
///   \param p_action Action-ID (see window.h for all IDs)
//---------------------------------------------------------------------
bool c_modbaker::perform_nongui_action(int p_action)
{
	string filename;

	switch (p_action)
	{
		case ACTION_MESH_NEW:
//			this->perform_gui_action(WINDOW_MESH_NEW_HIDE);
			g_nearest_vertex = 0;
			this->new_mesh_mpd(this->get_mesh_size().x, this->get_mesh_size().y);
			this->build_renderlist();
			/// \todo Remove objects / spawns
			break;

		case ACTION_MESH_LOAD:
			filename = this->w_mesh_load->tf_load->getText();
			this->load_complete_module(g_config->get_egoboo_path(), filename);

			break;

		case ACTION_MESH_SAVE:
			filename = this->w_mesh_save->tf_save->getText();
			if (this->save_mesh_mpd(g_config->get_egoboo_path() + "modules/" + this->modname + "/gamedat/" + filename))
				this->set_visibility("mesh_save", false);

			break;

		// Vertex editing
		case ACTION_VERTEX_UP:
			this->modify_y(10.0f);
			break;

		case ACTION_VERTEX_LEFT:
			this->modify_x(-10.0f);
			break;

		case ACTION_VERTEX_RIGHT:
			this->modify_x(10.0f);
			break;

		case ACTION_VERTEX_DOWN:
			this->modify_y(-10.0f);
			break;

		case ACTION_VERTEX_INC:
			this->modify_z(50.0f);
			break;

		case ACTION_VERTEX_DEC:
			this->modify_z(-50.0f);
			break;

		case ACTION_SELECTION_CLEAR:
			this->clear_selection();
			break;

		case ACTION_MAKE_WALL:
			this->make_wall();
			break;

		case ACTION_MAKE_FLOOR:
			this->make_floor();
			break;

		case ACTION_SELMODE_VERTEX:
			if (this->get_selection_mode() != SELECTION_MODE_VERTEX)
				this->clear_selection();

			this->set_selection_mode(SELECTION_MODE_VERTEX);
			break;

		case ACTION_SELMODE_TILE:
			if (this->get_selection_mode() != SELECTION_MODE_TILE)
				this->clear_selection();

			this->set_selection_mode(SELECTION_MODE_TILE);
			break;

		case ACTION_SELMODE_OBJECT:
			if (this->get_selection_mode() != SELECTION_MODE_OBJECT)
				this->clear_selection();

			this->set_selection_mode(SELECTION_MODE_OBJECT);
			break;

		case ACTION_EXIT:
			this->done = true;
			break;

		case ACTION_MODIFIER_ON:
			this->m_factor = SCROLLFACTOR_FAST;

			if (this->get_add_mode() == MODE_OVERWRITE)
				this->set_add_mode(MODE_ADD);
			break;

		case ACTION_MODIFIER_OFF:
			this->m_factor = SCROLLFACTOR_SLOW;

			if (this->get_add_mode() == MODE_ADD)
				this->set_add_mode(MODE_OVERWRITE);
			break;

		// Texture painting
		case ACTION_PAINT_TEXTURE:
			this->change_texture(this->get_current_texture_size());
			break;

		// Weld the selected vertices together
		case ACTION_WELD_VERTICES:
			this->weld();
			break;

		// Scrolling
		case SCROLL_RIGHT:
			this->m_movex += 5.0f;
			break;

		case SCROLL_LEFT:
			this->m_movex -= 5.0f;
			break;

		case SCROLL_UP:
			this->m_movey += 5.0f;
			break;

		case SCROLL_DOWN:
			this->m_movey -= 5.0f;
			break;

		case SCROLL_INC:
			this->m_movez += 5.0f;
			break;

		case SCROLL_DEC:
			this->m_movez -= 5.0f;
			break;

		case SCROLL_X_STOP:
			this->m_movex = 0.0f;
			break;

		case SCROLL_Y_STOP:
			this->m_movey = 0.0f;
			break;

		case SCROLL_Z_STOP:
			this->m_movez = 0.0f;
			break;

		// Tile flag rendering
		case ACTION_SHOW_TILEFLAGS:
			this->set_render_mode(RENDER_TILE_FLAGS);
			break;

		case ACTION_HIDE_TILEFLAGS:
			this->set_render_mode(RENDER_NORMAL);
			break;

		case ACTION_TOGGLE_TILEFLAGS:
			if (this->get_render_mode() == RENDER_TILE_FLAGS)
				this->set_render_mode(RENDER_NORMAL);
			else
				this->set_render_mode(RENDER_TILE_FLAGS);
			break;

		// Tile flags
		case ACTION_TILE_FLAG_PAINT:
//			this->set_tile_flag(g_renderer->get_wm()->get_selected_tile_flag());
			break;

		// Tile types
		case ACTION_TILE_TYPE_PAINT:
//			this->set_tile_type(g_renderer->get_wm()->get_selected_tile_type());
			break;

		// Remove an object
		case ACTION_OBJECT_REMOVE:
			this->delete_mesh_objects();
			this->clear_selection();
			break;

		// Save the file menu.txt
		case ACTION_SAVE_MOD_MENU:
//			this->set_menu_txt(this->get_menu_txt_content());
//			this->save_menu_txt(g_config->get_egoboo_path() + "modules/" + this->modname + "/gamedat/menu.txt");
			break;

		default:
			return false;
			break;
	}

	return true;
}


//---------------------------------------------------------------------
///   Perform a GUI action
///   \param p_action a GUI action
///   \return true if action was found, else false
//---------------------------------------------------------------------
bool c_modbaker::perform_gui_action(int p_action)
{
	bool found;

	found = true;

	switch (p_action)
	{
		// Guichan window handling
		case WINDOW_TEXTURE_TOGGLE:
			// Reset direct painting mode
			if (this->get_window("texture")->isVisible())
				this->set_add_mode(MODE_OVERWRITE);
				this->set_selection_mode(SELECTION_MODE_TILE);

			this->toggle_visibility("texture");
			break;

		case WINDOW_OBJECT_TOGGLE:
			this->toggle_visibility("object");
			break;

		case WINDOW_INFO_TOGGLE:
			this->toggle_visibility("info");
			break;

		case WINDOW_MESH_LOAD_SHOW:
			this->set_visibility("mesh_load", true);
			break;

		case WINDOW_MESH_SAVE_SHOW:
			this->set_visibility("mesh_save", true);
			break;

		case WINDOW_MESH_LOAD_HIDE:
			this->set_visibility("mesh_load", false);
			break;

		case WINDOW_MESH_SAVE_HIDE:
			this->set_visibility("mesh_save", false);
			break;

		case WINDOW_MESH_NEW_HIDE:
			this->set_visibility("mesh_new", false);
			break;

		case WINDOW_MESH_NEW_SHOW:
			this->set_visibility("mesh_new", true);
			break;

		case WINDOW_MOD_MENU_HIDE:
			this->set_visibility("mod_menu", false);
			break;

		case WINDOW_MOD_MENU_SHOW:
			this->set_menu_txt_content(this->get_menu_txt());
			this->set_visibility("mod_menu", true);
			break;

		case WINDOW_TILE_FLAG_HIDE:
			this->set_visibility("tile_flag", false);
			break;

		case WINDOW_TILE_TYPE_HIDE:
			this->set_visibility("tile_type", false);
			break;

		case WINDOW_TILE_FLAG_SHOW:
			this->set_visibility("tile_flag", true);
			break;

		case WINDOW_TILE_TYPE_SHOW:
			this->set_visibility("tile_type", true);
			break;

		case WINDOW_TILE_FLAG_TOGGLE:
			this->toggle_visibility("tile_flag");
			break;

		case WINDOW_TILE_TYPE_TOGGLE:
			this->toggle_visibility("tile_type");
			break;

		case WINDOW_TEXTURE_SMALL:
		case WINDOW_TEXTURE_LARGE:
			this->w_texture->toggle_texsize();
			this->reload_textures(g_config->get_egoboo_path() + "modules/" + this->modname + "/gamedat/");
			break;

		case WINDOW_TEXTURE_RELOAD:
			this->reload_textures(g_config->get_egoboo_path() + "modules/" + this->modname + "/gamedat/");
			break;

		default:
			found = false;
			break;
	}

	return found;
}


//---------------------------------------------------------------------
///   Get the 3d coordinates depending on a 2d screen position
///   \param p_x screen x coordicate
///   \param p_y screen y coordicate
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_modbaker::get_GL_pos(int p_x, int p_y)
{
	if (this == NULL)
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
	glReadPixels(p_x, viewport[3] - p_y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);

	// get 3D coordinates based on window coordinates
	gluUnProject(p_x, viewport[3] - p_y, z, model_view, projection, viewport, &pos3D_x, &pos3D_y, &pos3D_z);

	g_mouse_gl_x = (float)pos3D_x;
	g_mouse_gl_y = (float)pos3D_y;
	g_mouse_gl_z = (float)pos3D_z;

	// Calculate the nearest vertex
	g_nearest_vertex = this->get_nearest_vertex(g_mouse_gl_x, g_mouse_gl_y, g_mouse_gl_z);
	g_nearest_tile   = this->get_nearest_tile(  g_mouse_gl_x, g_mouse_gl_y, g_mouse_gl_z);
	g_nearest_object = this->get_nearest_object(g_mouse_gl_x, g_mouse_gl_y, g_mouse_gl_z);

	return true;
}


//---------------------------------------------------------------------
///   The action function from ActionListener
///   \param actionEvent the action event
///   \todo use the action enum instead of strings
//---------------------------------------------------------------------
void c_modbaker::action(const gcn::ActionEvent& actionEvent)
{
	int iTmp;

	string action_id;

	vector<string> tokens;

	action_id = actionEvent.getId();

	if ((action_id.length() >= 3) && (action_id.substr(0, 4) == "tex;"))
	{
		// Tokenize the tex;xxx string
		if (tokenize_semicolon(action_id, tokens) <= 0)
		{
			cout << "WARNING: Could not tokenize " << action_id << endl;
			cout << "ABORTING" << endl;
			return;
		}

		istringstream iss_size(tokens[1]);
		istringstream iss_texture(tokens[2]);
		int texture, size;

		iss_size    >> size;
		iss_texture >> texture;

		this->set_current_item(texture);

		// If we have some tiles selected, change their texture
		if (this->get_selection_size() > 0)
			this->perform_nongui_action(ACTION_PAINT_TEXTURE);

		// Switch to direct texture painting
		this->set_selection_mode(SELECTION_MODE_TEXTURE);
		this->set_add_mode(MODE_PAINT);
	}
	else if (action_id == "save_mesh_ok")
	{
		this->perform_nongui_action(ACTION_MESH_SAVE);
		this->perform_gui_action(WINDOW_MESH_SAVE_HIDE);
	}
	else if (action_id == "load_mesh_ok")
	{
		this->perform_nongui_action(ACTION_MESH_LOAD);
		this->perform_gui_action(WINDOW_MESH_LOAD_HIDE);
	}
	else if (action_id == "new_mesh_ok")
	{
		vect2 new_size;
		new_size.x = translate_into_uint(this->w_mesh_new->tf_size_x->getText());
		new_size.y = translate_into_uint(this->w_mesh_new->tf_size_y->getText());
		this->set_mesh_size(new_size);

		this->perform_nongui_action(ACTION_MESH_NEW);
		this->perform_gui_action(WINDOW_MESH_NEW_HIDE);
	}
	else if (action_id == "mod_menu_ok")
	{
		this->perform_nongui_action(ACTION_SAVE_MOD_MENU);
		this->perform_gui_action(WINDOW_MOD_MENU_HIDE);
	}
	else if (action_id == "save_mesh_cancel")
	{
		this->perform_gui_action(WINDOW_MESH_SAVE_HIDE);
	}
	else if (action_id == "load_mesh_cancel")
	{
		this->perform_gui_action(WINDOW_MESH_LOAD_HIDE);
	}
	else if (action_id == "new_mesh_cancel")
	{
		this->perform_gui_action(WINDOW_MESH_NEW_HIDE);
	}
	else if (action_id == "mod_menu_cancel")
	{
		this->perform_gui_action(WINDOW_MOD_MENU_HIDE);
	}
	else if (action_id == "texture_large")
	{
		this->perform_gui_action(WINDOW_TEXTURE_SMALL);
	}
	else if (action_id == "texture_small")
	{
		this->perform_gui_action(WINDOW_TEXTURE_LARGE);
	}
	else if (action_id == "reload_textures")
	{
		this->perform_gui_action(WINDOW_TEXTURE_RELOAD);
	}
	else
	{
		iTmp = translate_into_uint(actionEvent.getId());

		this->perform_gui_action(iTmp);
		this->perform_nongui_action(iTmp);
	}
/*	else if (action_id == "")
	{
	}
*/
}


//---------------------------------------------------------------------
///   load a module
///   \param p_modname module name
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_modbaker::load_complete_module(string p_basedir, string p_modname)
{
	g_nearest_vertex = 0;

	this->load_module(p_basedir, p_modname);
//	g_renderer->get_wm()->set_visibility("load", false);
	this->load_basic_textures(p_modname);

	this->build_renderlist();

	// Reset module specific windows
	this->destroy_window("texture");
	this->destroy_window("object");
	this->create_window("texture", p_basedir + "modules/" + p_modname + "/gamedat/");
	this->create_window("object", "");

	return true;
}


//---------------------------------------------------------------------
///   Gets called at mouse button presses
///   \param p_event A mouse event
///   \todo this should be an action listener
//---------------------------------------------------------------------
void c_modbaker::mousePressed(gcn::MouseEvent& p_event)
{
	// Abort if it was a GUI event
	if (this->gui_event)
		return;

	if (p_event.getButton() == LEFT) {

		/// \todo move get_nearest_vertex / get_nearest_tile here
		if (!this->get_add_mode())
			this->perform_nongui_action(ACTION_SELECTION_CLEAR);

		switch (this->get_selection_mode())
		{
			case SELECTION_MODE_VERTEX:
				this->add_item(g_nearest_vertex);
				break;

			case SELECTION_MODE_TILE:
				this->add_item(g_nearest_tile);
				break;

			case SELECTION_MODE_TEXTURE:
				this->change_texture(this->get_current_texture_size());
				break;

			case SELECTION_MODE_OBJECT:
				/// \todo Different action if we want to place an object
				// (but also add the object to the selection)
				this->add_item(g_nearest_object);
				break;

			default:
				cout << "Invalid selection mode" << endl;
				break;
		}
	}
}


//---------------------------------------------------------------------
///   Draw certain helping positions
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_modbaker::render_positions()
{
	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// Render the origin (0,0,0)
	glBegin(GL_QUADS);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

	glVertex3i(-10, -10, 250); // Bottom left
	glVertex3i( 10, -10, 250); // Bottom right
	glVertex3i( 10, 10, 250); // Top    right
	glVertex3i(-10, 10, 250); // Top    left
	glEnd();

	// Render a box and a line at the nearest vertex position
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	glBegin(GL_QUADS);
	glVertex3f((this->get_vert(g_nearest_vertex).x - 10), (this->get_vert(g_nearest_vertex).y - 10), this->get_vert(g_nearest_vertex).z + 1); // Bottom left
	glVertex3f((this->get_vert(g_nearest_vertex).x + 10), (this->get_vert(g_nearest_vertex).y - 10), this->get_vert(g_nearest_vertex).z + 1); // Bottom right
	glVertex3f((this->get_vert(g_nearest_vertex).x + 10), (this->get_vert(g_nearest_vertex).y + 10), this->get_vert(g_nearest_vertex).z + 1); // Top    right
	glVertex3f((this->get_vert(g_nearest_vertex).x - 10), (this->get_vert(g_nearest_vertex).y + 10), this->get_vert(g_nearest_vertex).z + 1); // Top    left
	glEnd();

	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	glLineWidth(5.0f);
	glBegin(GL_LINES);
	glVertex3f((this->get_vert(g_nearest_vertex).x), (this->get_vert(g_nearest_vertex).y), this->get_vert(g_nearest_vertex).z - 500);
	glVertex3f((this->get_vert(g_nearest_vertex).x), (this->get_vert(g_nearest_vertex).y), this->get_vert(g_nearest_vertex).z + 500);
	glEnd();

	int i;

	if (this->get_selection_mode() == SELECTION_MODE_VERTEX)
	{
		// Render all tiles in the selection
		glBegin(GL_QUADS);
		glColor4f(0.0f, 1.0f, 0.0f, 1);

		for (i = 0; i < this->vrt_count; i++)
		{
			if (this->in_selection(i))
			{
				glVertex3f((this->get_vert(i).x - 10), (this->get_vert(i).y - 10), this->get_vert(i).z + 1); // Bottom left
				glVertex3f((this->get_vert(i).x + 10), (this->get_vert(i).y - 10), this->get_vert(i).z + 1); // Bottom right
				glVertex3f((this->get_vert(i).x + 10), (this->get_vert(i).y + 10), this->get_vert(i).z + 1); // Top    right
				glVertex3f((this->get_vert(i).x - 10), (this->get_vert(i).y + 10), this->get_vert(i).z + 1); // Top    left
			}
		}
		glEnd();
	}

	glEnable(GL_TEXTURE_2D);
	// Now reset the colors
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	return true;
}


//---------------------------------------------------------------------
///   Call render_fan() for every fan in the mesh
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_modbaker::render_mesh()
{
	// Reset the last used texture, so we HAVE to load a new one
	// for the first tiles
//	ogl_state_t      loc_ogl_state;
//	ogl_state_comp_t loc_ogl_state_comp;

//	gl_grab_state(&loc_ogl_state);
//	gl_comp_state(&loc_ogl_state_comp, &loc_ogl_state, &tmp_ogl_state);

	glEnable(GL_TEXTURE_2D);

	for (Uint8 tileset = 0; tileset < 4; tileset++)
	{
		// Change the texture
		GLint itexture, icurrent_tx;
		itexture = m_texture[tileset + TX_TILE_0];

		glGetIntegerv(GL_TEXTURE_BINDING_2D, &icurrent_tx);
		if (icurrent_tx != itexture)
		{
			glBindTexture(GL_TEXTURE_2D, itexture);
		}

		for (int inorm = 0; inorm < this->m_num_norm; inorm++)
		{
			// grab a tile from the renderlist
			Uint32 this_fan  = this->m_norm[inorm];

			// do not render invalid tiles
			Uint16 this_tile = this->tiles[this_fan].tile;
			if (INVALID_TILE == this_tile) continue;

			// keep one texture in memory as long as possible
			Uint8 this_tileset = this_tile >> 6;
			if (this_tileset == tileset)
			{
				render_fan(this_fan, false);
			}
		};
	}

	return true;
}


//---------------------------------------------------------------------
///   Render a single fan / tile
///   \param fan the fan to render
///   \param set_texture do we need to change the texture?
//---------------------------------------------------------------------
void c_modbaker::render_fan(Uint32 fan, bool set_texture = true)
{
	Uint16 commands;
	Uint16 vertices;
	Uint16 itexture;

	Uint16 cnt, tnc, entry;

	Uint32 badvertex;

	vect2 off;
	vect3 tmp_vertex;

	Uint16 tile = this->tiles[fan].tile; // Tile
	Uint16 fx   = this->tiles[fan].fx;   // Tile FX
	Uint16 type = this->tiles[fan].type; // Command type ( index to points in fan )
	// type <= 32 => small tile
	// type >  32 => big tile

	if (tile == INVALID_TILE)
		return;

	//	mesh_calc_normal_fan( pmesh, &(gs->phys), fan, &nrm, &pos );
	tile &= 0xFF;
	off = this->TxBox[tile];

	// Change the texture if we need to
	if (set_texture)
	{
		// Detect which tileset to load (1, 2, 3 or 4 (0 = particles))
		itexture = (tile >> 6 ) + TX_TILE_0;    // (1 << 6) == 64 tiles in each 256x256 texture

		GLint icurrent_tx;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &icurrent_tx);
		if ((GLuint)icurrent_tx != m_texture[itexture])
		{
			glBindTexture(GL_TEXTURE_2D, m_texture[itexture]);
		}
	}


	//	light_flat.r =
	//	light_flat.g =
	//	light_flat.b =
	//	light_flat.a = 0.0f;

	vertices  = this->tile_definitions[type].vrt_count;            // Number of vertices
	badvertex = this->tiles[fan].vrt_start;   // Get big reference value

	// Fill in the vertex data from the mesh memory
	//	for (cnt = 0; cnt < vertices; cnt++, badvertex++)
	//	{
	//		v[cnt].pos.x = this->vrt_x[badvertex];
	//		v[cnt].pos.y = this->vrt_y[badvertex];
	//		v[cnt].pos.z = this->vrt_z[badvertex];
	//
	//		v[cnt].col.r = FP8_TO_FLOAT( this->vrt_lr_fp8[badvertex] + this->vrt_ar_fp8[badvertex] );
	//		v[cnt].col.g = FP8_TO_FLOAT( this->vrt_lg_fp8[badvertex] + this->vrt_ag_fp8[badvertex] );
	//		v[cnt].col.b = FP8_TO_FLOAT( this->vrt_lb_fp8[badvertex] + this->vrt_ab_fp8[badvertex] );
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
	commands = this->tile_definitions[type].cmd_count;            // Number of commands
	glColor4f(1, 1, 1, 1);

	// Render the FX bits
	this->render_tile_flag(fx);

	// Highlight the nearest tile and tiles in the selection
	if (((this->get_selection_mode() == SELECTION_MODE_TILE) ||
	    (this->get_selection_mode() == SELECTION_MODE_TEXTURE)) &&
	    (this->in_selection((int)fan) || (int)fan == g_nearest_tile))
	{
		glEnable(GL_TEXTURE_2D);
		glColor4f(0, 1, 0, 1);
	}


	for (cnt = 0, entry = 0; cnt < commands; cnt++)
	{
		c_tile_definition & tile_def = this->tile_definitions[type];
		Uint8 size = tile_def.cmd_size[cnt];

		glBegin(GL_TRIANGLE_FAN);

		for (tnc = 0; tnc < size; tnc++)
		{
			Uint16 loc_vrt = tile_def.vrt[entry];
			Uint16 glb_vrt = loc_vrt + badvertex;

			tmp_vertex = this->get_vert(glb_vrt);

			glTexCoord2f(tile_def.tx[loc_vrt].u + off.u, tile_def.tx[loc_vrt].v + off.v);
			glVertex3f(tmp_vertex.x, tmp_vertex.y, tmp_vertex.z);

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
	for ( tnc = 0; tnc < this->getTileDefinition(type).cmd_size[cnt]; tnc++ )
	{
	vertex = this->getTileDefinition(type).vrt[entry];

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


//---------------------------------------------------------------------
///   Render all models
///   \param p_obj_manager object manager containing the objects
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_modbaker::render_models()
{
	unsigned int i;
	int slot;
	vect3 pos;

	for (i=0; i<this->get_spawns_size(); i++)
	{
		pos = this->get_spawn(i).pos;

		// Get the object slot number
		slot = this->get_spawn(i).slot_number;

		if (this->get_object_by_slot(slot) != NULL)
		{
//			this->get_object_by_slot(slot)->render();

			if (this->get_object_by_slot(slot)->get_icon() == 0)
			{
				// No icon found: Render a blue box
				glDisable(GL_TEXTURE_2D);
				glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
			}
			else
			{
				// Load the icon
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, this->get_object_by_slot(slot)->get_icon());
			}
			pos.x =  pos.x * (1 << 7);
			pos.y =  pos.y * (1 << 7);
//			pos.z =  pos.z * (1 << 7);
			pos.z =  1.0f  * (1 << 7);

			// Highlight the object if it is in the selection
			if ((this->get_selection_mode() == SELECTION_MODE_OBJECT) &&
				(this->in_selection((int)i)))
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
///   Load all basic textures
///   \param p_modname module to load
//---------------------------------------------------------------------
void c_modbaker::load_basic_textures(string p_modname)
{
	load_texture(g_config->get_egoboo_path() + "basicdat/globalparticles/particle.png",        TX_PARTICLE);
/*
	if (!load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/tile0.bmp",    TX_TILE_0))
		load_texture("data/tiles/tile0.bmp", TX_TILE_0);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/tile1.bmp",    TX_TILE_1))
		load_texture("data/tiles/tile1.bmp", TX_TILE_1);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/tile2.bmp",    TX_TILE_2))
		load_texture("data/tiles/tile2.bmp", TX_TILE_2);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/tile3.bmp",    TX_TILE_3))
		load_texture("data/tiles/tile3.bmp", TX_TILE_3);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/watertop.bmp", TX_WATER_TOP))
		load_texture("data/tiles/watertop.bmp", TX_WATER_TOP);

	if (!load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/waterlow.bmp", TX_WATER_LOW))
		load_texture("data/tiles/waterlow.bmp", TX_WATER_LOW);
*/

	load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/tile0.bmp",    TX_TILE_0);
	load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/tile1.bmp",    TX_TILE_1);
	load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/tile2.bmp",    TX_TILE_2);
//	load_texture("data/tiles/tile2.bmp",    TX_TILE_2);
	load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/tile3.bmp",    TX_TILE_3);
	load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/watertop.bmp", TX_WATER_TOP);
	load_texture(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/waterlow.bmp", TX_WATER_LOW);
}


//---------------------------------------------------------------------
///   Get the selected tile flag
///   \return selected tile flag
//---------------------------------------------------------------------
int c_modbaker::get_selected_tile_flag()
{
	return this->get_meshfx(this->w_tile_flag->lb_tile_flag->getSelected());
}



//---------------------------------------------------------------------
///   rebuild the object window
//---------------------------------------------------------------------
void c_modbaker::reload_object_window()
{
	unsigned int i;
	string obj_showname;

	// Append all objects to the area
	for (i=0; i<this->get_objects_size(); i++)
	{
		obj_showname = this->get_object(i)->get_name();

		if (this->get_object(i)->get_gor())
			obj_showname += " (GOR)";

		this->w_object->obj_list_model->add_element(obj_showname);
	}
}
