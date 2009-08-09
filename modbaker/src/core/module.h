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
/// @brief module and module editor definition
/// @details Defines the module and module editor class

#ifndef module_h
#define module_h

#include <vector>
#include <string>
using namespace std;

#include "mesh.h"
#include "passage.h"
#include "modmenu.h"
#include "object.h"


//---------------------------------------------------------------------
///   \enum selectopm_modes
///         Selection modes
//---------------------------------------------------------------------
enum selection_modes
{
	SELECTION_MODE_VERTEX,  ///< Vertex selection mode
	SELECTION_MODE_TEXTURE, ///< Texture selection mode
	SELECTION_MODE_TTYPE,   ///< Tile type selection mode
	SELECTION_MODE_TFLAG,   ///< Tile flag selection mode
	SELECTION_MODE_OBJECT,  ///< Object selection mode
	SELECTION_MODE_TILE     ///< Tile selection mode
};


//---------------------------------------------------------------------
///   \enum add_modes
///         Add modes
//---------------------------------------------------------------------
enum add_modes
{
	MODE_OVERWRITE, ///< Replace selection with the item
	MODE_ADD,       ///< Add item to the selection
	MODE_PAINT      ///< Directly draw item to the mesh
};


//---------------------------------------------------------------------
///   container for one module
//---------------------------------------------------------------------
class c_module : public c_mesh, public c_passage_manager,
	public c_menu_txt, public c_object_manager
{
	private:
	public:
		void load_module(string, string);
		bool save_module(string);
		bool new_module(string);
		c_module() {}
		~c_module() {}
};


//---------------------------------------------------------------------
///   container for all module editing functions
//---------------------------------------------------------------------
class c_module_editor : public c_module
{
	private:
		int current_item;      ///< contains the current texture, ... id
		vector<int> selection; ///< the selection
		int add_mode;          ///< the add mode

	protected:
		int selection_mode;    ///< the selection mode

	public:
		c_module_editor();
		~c_module_editor();


		void add_item(int);
		void remove_item(int);
		int add_item_at_position(vect3);
		void clear_selection();

		bool in_selection(int);

		void set_add_mode(int);
		int  get_add_mode();

		void set_selection_mode(int);
		int  get_selection_mode();

		void set_current_item(int);
		int  get_current_item();

		int get_selection_size();

		// Vertex functions
		bool weld();

		// Object functions
		bool add_mesh_object(int, float, float, float);
		bool delete_mesh_objects();
		bool move_mesh_object(int, float, float, float);

		// Vertex and object functions
		void modify_x(float);
		void modify_y(float);
		void modify_z(float);


		// Texture functions
		void change_texture(int);

		void set_tile_flag(unsigned char);
		void set_tile_type(int);

		void make_wall();
		void make_floor();
};
#endif
