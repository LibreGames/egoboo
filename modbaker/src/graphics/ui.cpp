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
/// @brief User interface implementation
/// @details Implementation of the user interface

#include "../global.h"
#include "ui.h"

#include <iostream>
#include <string>
#include <sstream>
using namespace std;




//---------------------------------------------------------------------
///   c_window_manager constructor
//---------------------------------------------------------------------
c_window_manager::c_window_manager()
{
	this->w_object    = NULL;
	this->w_texture   = NULL;
	this->w_palette   = NULL;
	this->w_tile_type = NULL;
	this->w_tile_flag = NULL;
	this->w_info      = NULL;
	this->w_mesh_save = NULL;
	this->w_mesh_load = NULL;
	this->w_mesh_new  = NULL;
	this->w_mod_menu  = NULL;
	this->w_spawn     = NULL;
	this->w_minimap   = NULL;
	this->w_config    = NULL;

	this->top         = NULL;
	this->gui         = NULL;

	this->selected_object    = -1;
	this->selected_tile_flag = -1;
	this->selected_tile_type = -1;
}


//---------------------------------------------------------------------
///   c_window_manager destructor
//---------------------------------------------------------------------
c_window_manager::~c_window_manager()
{
	delete this->gui;
	delete this->top;
}


//---------------------------------------------------------------------
///   Reload the textures for the current module
///   \param p_dirname directory name to load the textures from
//---------------------------------------------------------------------
void c_window_manager::reload_textures(string p_dirname)
{
	this->w_texture->build_tabbed_area(p_dirname);
}


//---------------------------------------------------------------------
///   create all general windows
///   \param p_width window width
///   \param p_height window height
//---------------------------------------------------------------------
void c_window_manager::create_all_windows()
{
	this->create_window("palette", "");
	this->create_window("info", "");
	this->create_window("mesh_load", "");
	this->create_window("mesh_save", "");
	this->create_window("mesh_new", "");
	this->create_window("mod_menu", "");
	this->create_window("tile_flag", "");
	this->create_window("tile_type", "");
}


//---------------------------------------------------------------------
///   init the window manager
///   \todo use the TTF font
//---------------------------------------------------------------------
void c_window_manager::init_wm(int p_width, int p_height)
{
	try
	{
		// Init the image loader
		imageLoader = new c_mb_image_loader();
		gcn::Image::setImageLoader(imageLoader);


		// Set up the OpenGL plane
		graphics = new gcn::OpenGLGraphics(p_width, p_height);
		graphics->setTargetPlane(p_width, p_height);


//		string pfname = g_config.get_egoboo_path() + "basicdat/" + g_config.get_font_file();
//		font = new gcn::contrib::OGLFTFont(pfname.c_str(), g_config.get_font_size());
		font = new gcn::ImageFont("data/rpgfont.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/():;%&``*#=[]'_");
		gcn::Widget::setGlobalFont(font);


		this->gui = new gcn::Gui();
		this->gui->setGraphics(graphics);

		// Create the top container
		this->top = new gcn::Container();
		this->top->setDimension(gcn::Rectangle(0, 0, p_width, p_height));
		this->top->setOpaque(false);
		this->top->setFocusable(true);
		this->gui->setTop(this->top);
	}
	catch (gcn::Exception e)
	{
		cout << "Guichan exception: " << endl;
		cout << e.getMessage() << endl;
	}
	catch (std::exception e)
	{
		cout << "Std exception: " << e.what() << endl;
	}
	catch (...)
	{
		cout << "Unknown exception" << endl;
	}
}



//---------------------------------------------------------------------
///   c_input_manager constuctor
//---------------------------------------------------------------------
c_input_manager::c_input_manager()
{
	this->mouse_x = 0;
	this->mouse_y = 0;

	this->gui_event = false;
	this->active    = true;
	this->done      = false;
}


//---------------------------------------------------------------------
///   c_input_manager destructor
//---------------------------------------------------------------------
c_input_manager::~c_input_manager()
{
}


//---------------------------------------------------------------------
///   Gets called when the mouse gets moved
///   \param event A mouse event
//---------------------------------------------------------------------
void c_input_manager::mouseMoved(gcn::MouseEvent& p_event)
{
	// clicked on a GUI element
	if (p_event.isConsumed())
		this->gui_event = true;
	else
		this->gui_event = false;

	// Set the mouse X and Y coordinates
	this->mouse_x = p_event.getX();
	this->mouse_y = p_event.getY();
}


//---------------------------------------------------------------------
///   Gets called at when a keyboard button gets pressed
///   \param p_event A keyboard event
///   \todo move to c_modbaker
//---------------------------------------------------------------------
void c_input_manager::keyPressed(gcn::KeyEvent& p_event)
{
	if (p_event.getKey() == gcn::Key::ESCAPE)
		perform_nongui_action(ACTION_EXIT);

	// Don't overwrite the input for the GUI
	if (this->gui_event)
	{
		cout << "GUI event" << endl;
		return;
	}

	if (p_event.getKey().getValue() == 'l')
		perform_nongui_action(ACTION_OBJECT_REMOVE);

	if (p_event.getKey().getValue() == 'o')
	{
		perform_nongui_action(ACTION_PAINT_TEXTURE);
	}

	if (p_event.getKey().getValue() == 'w')
		perform_nongui_action(ACTION_VERTEX_UP);

	if (p_event.getKey().getValue() == 's')
		perform_nongui_action(ACTION_VERTEX_DOWN);

	if (p_event.getKey().getValue() == 'a')
		perform_nongui_action(ACTION_VERTEX_LEFT);

	if (p_event.getKey().getValue() == 'd')
		perform_nongui_action(ACTION_VERTEX_RIGHT);

	if (p_event.getKey().getValue() == 'q')
		perform_nongui_action(ACTION_VERTEX_INC);

	if (p_event.getKey().getValue() == 'r')
		perform_nongui_action(ACTION_VERTEX_DEC);

	if (p_event.getKey().getValue() == 'b')
		perform_nongui_action(ACTION_SELMODE_TILE);

	if (p_event.getKey().getValue() == 'n')
		perform_nongui_action(ACTION_SELMODE_VERTEX);

	if (p_event.getKey() == gcn::Key::UP)
		perform_nongui_action(SCROLL_UP);

	if (p_event.getKey() == gcn::Key::LEFT)
		perform_nongui_action(SCROLL_LEFT);

	if (p_event.getKey() == gcn::Key::RIGHT)
		perform_nongui_action(SCROLL_RIGHT);

	if (p_event.getKey() == gcn::Key::DOWN)
		perform_nongui_action(SCROLL_DOWN);

	if (p_event.getKey() == gcn::Key::PAGE_UP)
		perform_nongui_action(SCROLL_INC);

	if (p_event.getKey() == gcn::Key::PAGE_DOWN)
		perform_nongui_action(SCROLL_DEC);

	if (p_event.getKey() == gcn::Key::LEFT_SHIFT)
		perform_nongui_action(ACTION_MODIFIER_ON);

	if (p_event.getKey() == gcn::Key::RIGHT_SHIFT)
		perform_nongui_action(ACTION_MODIFIER_ON);

	if (p_event.getKey() == gcn::Key::SPACE)
		perform_nongui_action(ACTION_SELECTION_CLEAR);
}


//---------------------------------------------------------------------
///   Gets called at when a keyboard button gets released
///   \param event A keyboard event
//---------------------------------------------------------------------
void c_input_manager::keyReleased(gcn::KeyEvent& p_event)
{
	// Don't overwrite the input for the GUI
	if (this->gui_event)
		return;


	if (p_event.getKey() == gcn::Key::UP)
		perform_nongui_action(SCROLL_Y_STOP);

	if (p_event.getKey() == gcn::Key::DOWN)
		perform_nongui_action(SCROLL_Y_STOP);

	if (p_event.getKey() == gcn::Key::LEFT)
		perform_nongui_action(SCROLL_X_STOP);

	if (p_event.getKey() == gcn::Key::RIGHT)
		perform_nongui_action(SCROLL_X_STOP);

	if (p_event.getKey() == gcn::Key::PAGE_UP)
		perform_nongui_action(SCROLL_Z_STOP);

	if (p_event.getKey() == gcn::Key::PAGE_DOWN)
		perform_nongui_action(SCROLL_Z_STOP);

	if (p_event.getKey() == gcn::Key::LEFT_SHIFT)
		perform_nongui_action(ACTION_MODIFIER_OFF);

	if (p_event.getKey() == gcn::Key::RIGHT_SHIFT)
		perform_nongui_action(ACTION_MODIFIER_OFF);
}


//---------------------------------------------------------------------
///   c_ui constructor
//---------------------------------------------------------------------
c_ui::c_ui()
{
	this->input = NULL;
}


//---------------------------------------------------------------------
///   c_ui destructor
//---------------------------------------------------------------------
c_ui::~c_ui()
{
}


//---------------------------------------------------------------------
///   Handle all ModBaker input events
///   \return error code or 0 on success
//---------------------------------------------------------------------
int c_ui::handle_events()
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
				this->active =
						((event.active.state == SDL_APPMOUSEFOCUS) ||
						 (event.active.state == SDL_APPINPUTFOCUS) ||
						 (event.active.state == SDL_APPACTIVE))    &&
						  event.active.gain == 1;
				break;

			case SDL_VIDEOEXPOSE:
				this->active = true;
				break;

				// Quit request
			case SDL_QUIT:
				this->done = true;
				break;

			default:
				break;
		}

		// Guichan events
		this->input->pushInput(event);
	}

	return 0;
}


//---------------------------------------------------------------------
///   Set the FPS for the "info" window
///   \param p_fps new FPS
//---------------------------------------------------------------------
void c_window_manager::set_info_fps(float p_fps)
{
	ostringstream tmp_str;
	tmp_str << "FPS: " << p_fps;

	if (this->w_info->label_fps == NULL)
		return;

	this->w_info->label_fps->setCaption(tmp_str.str());
}


//---------------------------------------------------------------------
///   Set the position for the "info" window
///   \param p_x cursor x position
///   \param p_y cursor y position
//---------------------------------------------------------------------
void c_window_manager::set_info_position(float p_x, float p_y)
{
	ostringstream tmp_str_x, tmp_str_y;
	tmp_str_x << "X Pos: " << p_x;

	if (this->w_info->label_position_x == NULL)
		return;

	this->w_info->label_position_x->setCaption(tmp_str_x.str());

	tmp_str_y << "Y Pos: " << p_y;

	if (this->w_info->label_position_y == NULL)
		return;

	this->w_info->label_position_y->setCaption(tmp_str_y.str());
}


//---------------------------------------------------------------------
///   Create a window
///   \param p_window_name name of the window
///   \param p_dirname optional directory to load
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_window_manager::create_window(string p_window_name, string p_dirname)
{
	if (this->top == NULL)
	{
		cout << "WARNING! The top widget is NULL" << endl;
		return false;
	}

	try
	{
		if (p_window_name == "texture")
		{
			this->w_texture = new c_texture_window(this, p_dirname);
			this->top->add(w_texture);
		}

		if (p_window_name == "info")
		{
			this->w_info = new c_info_window(this);
			this->top->add(w_info);
		}

		if (p_window_name == "object")
		{
			this->w_object = new c_object_window(this);
			this->top->add(w_object);
		}

		if (p_window_name == "tile_type")
		{
			this->w_tile_type = new c_tile_type_window(this);
			this->top->add(w_tile_type);
		}

		if (p_window_name == "tile_flag")
		{
			this->w_tile_flag = new c_tile_flag_window(this);
			this->top->add(w_tile_flag);
		}

//		if (p_window_name == "minimap")
//		{
//			this->w_minimap = new c_window(this);
//			this->top->add(w_minimap);
//		}

//		if (p_window_name == "config")
//		{
//			this->w_config = new c_window(this);
//			this->top->add(w_config);
//		}

		if (p_window_name == "palette")
		{
			this->w_palette = new c_palette_window(this);
			this->top->add(w_palette);
		}

		if (p_window_name == "mesh_save")
		{
			this->w_mesh_save = new c_mesh_save_window(this);
			this->top->add(w_mesh_save);
		}

		if (p_window_name == "mesh_load")
		{
			this->w_mesh_load = new c_mesh_load_window(this);
			this->top->add(w_mesh_load);
		}

		if (p_window_name == "mesh_new")
		{
			this->w_mesh_new  = new c_mesh_new_window(this);
			this->top->add(w_mesh_new);
		}

//		if (p_window_name == "spawn")
//		{
//			this->w_spawn = new c_window(this);
//			this->top->add(w_spawn);
//		}

		if (p_window_name == "mod_menu")
		{
			this->w_mod_menu  = new c_mod_txt_window(this);
			this->top->add(w_mod_menu);
		}
	}
	catch (gcn::Exception e)
	{
		cout << e.getMessage() << endl;
	}
	catch (std::exception e)
	{
		cout << "Std exception: " << e.what() << endl;
	}
	catch (...)
	{
		cout << "Unknown exception" << endl;
	}
	return true;
}


//---------------------------------------------------------------------
///   Destroy a window
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_window_manager::destroy_window(string p_window_name)
{
	c_mb_window* window;

	window = this->get_window(p_window_name);

	if (window == NULL)
		return false;

	delete window;

	return true;
}


//---------------------------------------------------------------------
///   Set the visibility of a window
///   \param p_window_name name of a window
///   \param p_visibility should it be visible?
///   \return false if window was not found, else true
//---------------------------------------------------------------------
bool c_window_manager::set_visibility(string p_window_name, bool p_visibility)
{
	c_mb_window* window;
	window = get_window(p_window_name);

	if (window == NULL)
	{
		cout << "WARNING: No window '" << p_window_name << "' defined!" << endl;
		return false;
	}

	get_window(p_window_name)->setVisible(p_visibility);
	return true;
}


//---------------------------------------------------------------------
///   Toggle the visibility of a window
///   \param p_window_name name of a window
///   \return false if window was not found, else true
//---------------------------------------------------------------------
bool c_window_manager::toggle_visibility(string p_window_name)
{
	c_mb_window* window;
	window = get_window(p_window_name);

	if (window == NULL)
	{
		cout << "WARNING: No window '" << p_window_name << "' defined!" << endl;
		return false;
	}

	if (window->isVisible())
		window->setVisible(false);
	else
		window->setVisible(true);

	return true;
}


//---------------------------------------------------------------------
///   Get the selected object
///   \return selected object
//---------------------------------------------------------------------
int c_window_manager::get_selected_object()
{
	return this->w_object->obj_list_box->getSelected();
}


//---------------------------------------------------------------------
///   Get the selected tile type
///   \return selected tile type
//---------------------------------------------------------------------
int c_window_manager::get_selected_tile_type()
{
	return this->w_tile_type->lb_tile_type->getSelected();
}


//---------------------------------------------------------------------
///   Set the selected object
///   \param p_object new object
//---------------------------------------------------------------------
void c_window_manager::set_selected_object(int p_object)
{
	this->selected_object = p_object;
}


//---------------------------------------------------------------------
///   Set the selected tile flag
///   \param p_tile_flag new tile flag
//---------------------------------------------------------------------
void c_window_manager::set_selected_tile_flag(int p_tile_flag)
{
	this->selected_tile_flag = p_tile_flag;
}


//---------------------------------------------------------------------
///   Set the selected tile type
///   \param p_tile_type new tile type
//---------------------------------------------------------------------
void c_window_manager::set_selected_tile_type(int p_tile_type)
{
	this->selected_tile_type = p_tile_type;
}


//---------------------------------------------------------------------
///   Return a window object based on a string
///   \param p_window window identifier
///   \return window object
//---------------------------------------------------------------------
c_mb_window* c_window_manager::get_window(string p_window)
{
	if (p_window == "texture")   return this->w_texture;
	if (p_window == "info")      return this->w_info;
	if (p_window == "object")    return this->w_object;
	if (p_window == "tile_type") return this->w_tile_type;
	if (p_window == "tile_flag") return this->w_tile_flag;
	if (p_window == "minimap")   return this->w_minimap;
	if (p_window == "config")    return this->w_config;
	if (p_window == "palette")   return this->w_palette;
	if (p_window == "mesh_save") return this->w_mesh_save;
	if (p_window == "mesh_load") return this->w_mesh_load;
	if (p_window == "mesh_new")  return this->w_mesh_new;
	if (p_window == "spawn")     return this->w_spawn;
	if (p_window == "mod_menu")  return this->w_mod_menu;

	return NULL;
}


int  c_window_manager::get_current_texture_size()
{
	return this->w_texture->current_size;
}


void c_window_manager::set_current_texture_size(int p_size)
{
	this->w_texture->current_size = p_size;
}
