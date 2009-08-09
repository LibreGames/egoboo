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
/// @brief User interface definition
/// @details Definition of the user interface

#ifndef ui_h
#define ui_h

#include <iostream>
#include <string>
using namespace std;

#include "ui_windows.h"

#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
#include <guichan/opengl/openglsdlimageloader.hpp>
#include <guichan/opengl/openglimage.hpp>
#include <guichan/actionevent.hpp>
#include <guichan/actionlistener.hpp>


//---------------------------------------------------------------------
///   Definition of the window manager
//---------------------------------------------------------------------
class c_window_manager : public c_mb_action_listener
{
	private:
		// GUI
		gcn::OpenGLGraphics* graphics;
		c_mb_image_loader* imageLoader;

		gcn::ImageFont* font;

		int selected_object;
		int selected_tile_flag;
		int selected_tile_type;

	protected:
		gcn::Container* top;  ///< The top widget
		gcn::Gui* gui;

		// The windows
		c_palette_window* w_palette;
		c_object_window* w_object;
		c_tile_type_window* w_tile_type;
		c_tile_flag_window* w_tile_flag;
		c_info_window* w_info;
		c_mesh_save_window* w_mesh_save;
		c_mesh_load_window* w_mesh_load;
		c_mesh_new_window* w_mesh_new;
		c_mod_txt_window* w_mod_menu;
		c_texture_window* w_texture;
		c_mb_window* w_spawn;
		c_mb_window* w_minimap;
		c_mb_window* w_config;

	public:
		c_window_manager();
		~c_window_manager();

		void reload_textures(string);
		virtual void reload_object_window() = 0;

		void init_wm(int, int);
		void create_all_windows();

		virtual bool perform_gui_action(int) = 0;

		void set_info_fps(float);
		void set_info_position(float, float);

		bool create_window(string, string);
		bool destroy_window(string);
		bool set_visibility(string, bool);
		bool toggle_visibility(string);

		c_mb_window* get_window(string);

		int get_selected_object();
		virtual int get_selected_tile_flag() = 0;
		int get_selected_tile_type();

		void set_selected_object(int);
		void set_selected_tile_flag(int);
		void set_selected_tile_type(int);

		int  get_current_texture_size();
		void set_current_texture_size(int);

		/// Set the current tooltip
		/// \param p_tooltip new tooltip
		void set_tooltip(string p_tooltip)
		{
			this->w_palette->label_tooltip->setCaption(p_tooltip);
		}


		/// Get the current tooltip
		/// \return tooltip
		string get_tooltip()
		{
			return this->w_palette->label_tooltip->getCaption();
		}

		/// Get the menu.txt object
		/// \return menu.txt object
		c_menu_txt* get_menu_txt_content()
		{
			return this->w_mod_menu->get_menu_txt_content();
		}

		/// Set the menu.txt object
		/// \param p_menu_txt new menu.txt object
		void set_menu_txt_content(c_menu_txt* p_menu_txt)
		{
			this->w_mod_menu->set_menu_txt_content(p_menu_txt);
		}
};


//---------------------------------------------------------------------
///   Definition of the general input handler
//---------------------------------------------------------------------
class c_input_manager : public gcn::MouseListener, public gcn::KeyListener
{
	private:

	protected:
		/// \todo create an int vector for the mouse coordinates
		int mouse_x; ///< mouse x coordinate
		int mouse_y; ///< mouse y coordinate

		bool gui_event; ///< was the event on the GUI?
		bool active;    ///< is the window active?
		bool done;      ///< quit the application

	public:
		c_input_manager();
		~c_input_manager();

		virtual bool perform_nongui_action(int) = 0;

		virtual void mousePressed(gcn::MouseEvent&) = 0;
		void mouseMoved(gcn::MouseEvent&);
		void keyPressed(gcn::KeyEvent&);
		void keyReleased(gcn::KeyEvent&);
};


//---------------------------------------------------------------------
///   Definition of the UI class
//---------------------------------------------------------------------
class c_ui : public c_input_manager, public c_window_manager
{
	private:
	protected:
		gcn::SDLInput* input; ///< the input
		int handle_events();

	public:
		c_ui();
		~c_ui();
};
#endif
