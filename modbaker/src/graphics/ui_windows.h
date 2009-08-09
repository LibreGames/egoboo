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
/// @brief Window definitions
/// @details Definition of the GUI windows

#ifndef ui_helper_h
#define ui_helper_h

#include <iostream>
#include <string>
using namespace std;

#include "../core/modmenu.h"
#include "../SDL_extensions.h"
#include "ui_widgets.h"

#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
#include <guichan/opengl/openglsdlimageloader.hpp>
#include <guichan/opengl/openglimage.hpp>


//---------------------------------------------------------------------
///   Action types for the input system
//---------------------------------------------------------------------
enum input_actions
{
	ACTION_MESH_NEW         =  0,
	ACTION_MESH_LOAD        =  1,
	ACTION_MESH_SAVE        =  2,
	ACTION_VERTEX_UP        =  3,
	ACTION_VERTEX_LEFT      =  4,
	ACTION_VERTEX_RIGHT     =  5,
	ACTION_VERTEX_DOWN      =  6,
	ACTION_VERTEX_INC       =  7,
	ACTION_VERTEX_DEC       =  8,
	ACTION_SELECTION_CLEAR  =  9,
	ACTION_EXIT             = 10,
	SCROLL_UP               = 11,
	SCROLL_LEFT             = 12,
	SCROLL_RIGHT            = 13,
	SCROLL_DOWN             = 14,
	SCROLL_INC              = 15,
	SCROLL_DEC              = 16,
	SCROLL_X_STOP           = 17,
	SCROLL_Y_STOP           = 18,
	SCROLL_Z_STOP           = 19,
	ACTION_MODIFIER_ON      = 20,
	ACTION_MODIFIER_OFF     = 21,
	ACTION_PAINT_TEXTURE    = 22,
	ACTION_SET_TEXTURE      = 23,
	ACTION_SELMODE_VERTEX   = 24,
	ACTION_SELMODE_TILE     = 25,
	ACTION_SELMODE_OBJECT   = 26,
	ACTION_QUIT             = 27,
	ACTION_WELD_VERTICES    = 28,
	ACTION_SHOW_TILEFLAGS   = 29,
	ACTION_HIDE_TILEFLAGS   = 30,
	ACTION_TOGGLE_TILEFLAGS = 31,
	ACTION_TILE_FLAG_PAINT  = 32,
	ACTION_TILE_TYPE_PAINT  = 33,
	ACTION_OBJECT_REMOVE    = 34,
	ACTION_MAKE_WALL        = 35,
	ACTION_MAKE_FLOOR       = 36,
	ACTION_SAVE_MOD_MENU    = 37,
	ACTION_LOAD_MOD_MENU    = 38,
	WINDOW_TEXTURE_TOGGLE   = 39,
	WINDOW_OBJECT_TOGGLE    = 40,
	WINDOW_INFO_TOGGLE      = 41,
	WINDOW_MESH_SAVE_SHOW   = 42,
	WINDOW_MESH_SAVE_HIDE   = 43,
	WINDOW_MESH_LOAD_SHOW   = 44,
	WINDOW_MESH_LOAD_HIDE   = 45,
	WINDOW_TILE_FLAG_SHOW   = 46,
	WINDOW_TILE_FLAG_HIDE   = 47,
	WINDOW_TILE_FLAG_TOGGLE = 48,
	WINDOW_TILE_TYPE_SHOW   = 49,
	WINDOW_TILE_TYPE_HIDE   = 50,
	WINDOW_TILE_TYPE_TOGGLE = 51,
	WINDOW_MESH_NEW_SHOW    = 52,
	WINDOW_MESH_NEW_HIDE    = 53,
	WINDOW_MESH_NEW_TOGGLE  = 54,
	WINDOW_MOD_MENU_SHOW    = 55,
	WINDOW_MOD_MENU_HIDE    = 56,
	WINDOW_MOD_MENU_TOGGLE  = 57,
	WINDOW_TEXTURE_SMALL    = 58,
	WINDOW_TEXTURE_LARGE    = 59,
	WINDOW_TEXTURE_RELOAD   = 60
};


//---------------------------------------------------------------------
///   Definition of the window for editing menu.txt
//---------------------------------------------------------------------
class c_mod_txt_window : public c_mb_window
{
	private:
		c_mb_label*     label_name;
		c_mb_textfield* tf_name;
		c_mb_label*     label_ref;
		c_mb_textfield* tf_ref;
		c_mb_label*     label_req_idsz;
		c_mb_textfield* tf_req_idsz;
		c_mb_label*     label_num_imp;
		c_mb_textfield* tf_num_imp;
		c_mb_label*     label_export;
		c_mb_textfield* tf_export;
		c_mb_label*     label_pla_min;
		c_mb_textfield* tf_pla_min;
		c_mb_label*     label_pla_max;
		c_mb_textfield* tf_pla_max;
		c_mb_label*     label_respawn;
		c_mb_textfield* tf_respawn;
		c_mb_label*     label_not_used;
		c_mb_textfield* tf_not_used;
		c_mb_label*     label_rating;
		c_mb_textfield* tf_rating;
		// Elements for the summary
		c_mb_label*     label_summary;
		c_mb_textfield* tf_summary1;
		c_mb_textfield* tf_summary2;
		c_mb_textfield* tf_summary3;
		c_mb_textfield* tf_summary4;
		c_mb_textfield* tf_summary5;
		c_mb_textfield* tf_summary6;
		c_mb_textfield* tf_summary7;
		c_mb_textfield* tf_summary8;
		// The buttons
		c_mb_button* button_ok;
		c_mb_button* button_cancel;

	public:
		c_mod_txt_window(c_mb_action_listener*);

		c_menu_txt* get_menu_txt_content();
		void set_menu_txt_content(c_menu_txt*);
};


//---------------------------------------------------------------------
///   Implementation of c_texture_window
//---------------------------------------------------------------------
class c_texture_window : public c_mb_window
{
	private:
		c_mb_action_listener* m_action_listener;
	protected:
	public:
		c_texture_window(c_mb_action_listener*, string);
		~c_texture_window();

		int current_size;

		void set_action_listener(c_mb_action_listener *);

		c_mb_tabbedarea* t_texture;
		c_mb_button* b_texsize;
		c_mb_button* b_reload;
		void build_tabbed_area(string);
		void toggle_texsize();
};


//---------------------------------------------------------------------
///   Implementation of c__window
//---------------------------------------------------------------------
class c_mesh_load_window : public c_mb_window
{
	private:
	protected:
	public:
		c_mesh_load_window(c_mb_action_listener*);
		~c_mesh_load_window();

		c_mb_textfield* tf_load; ///< "Load mesh" textfield
		c_mb_button* button_ok;
		c_mb_button* button_cancel;
};


//---------------------------------------------------------------------
///   Implementation of c_info_window
//---------------------------------------------------------------------
class c_info_window : public c_mb_window
{
	private:
	protected:
	public:
		c_info_window(c_mb_action_listener*);
		~c_info_window();

		c_mb_label* label_fps;
		c_mb_label* label_position_x;
		c_mb_label* label_position_y;
};


//---------------------------------------------------------------------
///   Implementation of c__window
//---------------------------------------------------------------------
class c_tile_type_window : public c_mb_window
{
	private:
	protected:
	public:
		c_tile_type_window(c_mb_action_listener*);
		~c_tile_type_window();

		c_mb_list_box*    lb_tile_type;
		c_mb_list_model*  lm_tile_type;
		c_mb_container*   c_tile_type;
		c_mb_scroll_area* sc_tile_type;
};


//---------------------------------------------------------------------
///   Implementation of c__window
//---------------------------------------------------------------------
class c_tile_flag_window : public c_mb_window
{
	private:
	protected:
	public:
		c_tile_flag_window(c_mb_action_listener*);
		~c_tile_flag_window();

		c_mb_list_box*    lb_tile_flag;
		c_mb_list_model*  lm_tile_flag;
		c_mb_container*   c_tile_flag;
		c_mb_scroll_area* sc_tile_flag;
};


//---------------------------------------------------------------------
///   Implementation of c__window
//---------------------------------------------------------------------
class c_object_window : public c_mb_window
{
	private:
	protected:
	public:
		c_object_window(c_mb_action_listener*);
		~c_object_window();

		c_mb_list_box*    obj_list_box;
		c_mb_list_model*  obj_list_model;
		c_mb_container*   c_object;
		c_mb_scroll_area* sc_object;
};


//---------------------------------------------------------------------
///   Implementation of c__window
//---------------------------------------------------------------------
class c_palette_window : public c_mb_window
{
	private:
	protected:
	public:
		c_palette_window(c_mb_action_listener*);
		~c_palette_window();

		c_mb_container* c_mesh;
		c_mb_label* label_tooltip;
};


//---------------------------------------------------------------------
///   Implementation of c__window
//---------------------------------------------------------------------
class c_mesh_save_window : public c_mb_window
{
	private:
	protected:
	public:
		c_mesh_save_window(c_mb_action_listener*);
		~c_mesh_save_window();

		c_mb_textfield* tf_save; ///< "Save mesh" textfield
};


//---------------------------------------------------------------------
///   Implementation of c__window
//---------------------------------------------------------------------
class c_mesh_new_window : public c_mb_window
{
	private:
	protected:
	public:
		c_mesh_new_window(c_mb_action_listener*);
		~c_mesh_new_window();

		c_mb_textfield* tf_name;   // New mesh: module name
		c_mb_textfield* tf_size_x; // New mesh: x size
		c_mb_textfield* tf_size_y; // New mesh: y size
};
#endif
