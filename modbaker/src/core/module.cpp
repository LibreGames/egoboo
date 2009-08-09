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
/// @brief module implementation
/// @details Implementation of the module class

#include "module.h"
#include "../utility.h"

#include <iostream>
using namespace std;


//---------------------------------------------------------------------
///   Load a module
///   \param p_basedir egoboo base directory
///   \param p_modname the module name to load
///   \return true on success
//---------------------------------------------------------------------
void c_module::load_module(string p_basedir, string p_modname)
{
	vector<string> files;
	if (!read_files(p_basedir + "modules/" + p_modname, "*", files))
	{
		cout << "Module " << p_modname << " not found" << endl;
		return;
	}

	this->load_tiledict(p_basedir + "basicdat/fans.txt");
	this->load_mesh_mpd(p_basedir + "modules/" + p_modname + "/gamedat/level.mpd");

	// Read the object and spawn.txt information
	this->load_all_for_module(p_basedir, p_modname);
	this->load_menu_txt(p_basedir + "modules/" + p_modname + "/gamedat/menu.txt");
}


//---------------------------------------------------------------------
///   Save a module
///   \param p_modname the module name to save
///   \return true on success
///   \todo do this function
//---------------------------------------------------------------------
bool c_module::save_module(string p_modname)
{
	return false;
}


//---------------------------------------------------------------------
///   Create a new module
///   \param p_modname the module name to create
///   \return true on success
///   \todo do this function
//---------------------------------------------------------------------
bool c_module::new_module(string p_modname)
{
	return false;
}


//---------------------------------------------------------------------
///   c_module_editor constructor
//---------------------------------------------------------------------
c_module_editor::c_module_editor()
{
	this->add_mode       = false;
	this->selection_mode = SELECTION_MODE_VERTEX;
	this->current_item   = 0;
	this->selection.clear();
}


//---------------------------------------------------------------------
///   c_module_editor destructor
//---------------------------------------------------------------------
c_module_editor::~c_module_editor()
{
	this->selection.clear();
}


//---------------------------------------------------------------------
///   Add an item to the selection
///   \param p_item_number an item number
//---------------------------------------------------------------------
void c_module_editor::add_item(int p_item_number)
{
	if (in_selection(p_item_number))
	{
		this->remove_item(p_item_number);
	}
	else
	{
		this->selection.push_back(p_item_number);
	}
}


//---------------------------------------------------------------------
///   Remove an item from the selection
///   \param p_item_number the item number to remove
//---------------------------------------------------------------------
void c_module_editor::remove_item(int p_item_number)
{
	vector <int> new_selection;
	unsigned int i;

	for (i=0; i<this->selection.size(); i++)
	{
		if (p_item_number != this->selection[i])
		{
			new_selection.push_back(this->selection[i]);
		}
	}

	this->selection = new_selection;
}


//---------------------------------------------------------------------
///   Clear the selection
//---------------------------------------------------------------------
void c_module_editor::clear_selection()
{
	this->selection.clear();
}


//---------------------------------------------------------------------
///   Add an item at a certain position to the selection
///   \todo implement this function
///   \param p_ref reference point
///   \return number of items added to the function
//---------------------------------------------------------------------
int c_module_editor::add_item_at_position(vect3 p_ref)
{
	int num_vertices;

	num_vertices = -1;

	return num_vertices;
}


//---------------------------------------------------------------------
///   Returns true if an item is in the selection
///   \param p_needle item to search
///   \return true if item is in the selection, else false
//---------------------------------------------------------------------
bool c_module_editor::in_selection(int p_needle)
{
	unsigned int i;

	for (i = 0; i < this->selection.size(); i++)
	{
		if (this->selection[i] == p_needle)
			return true;
	}

	return false;
}


//---------------------------------------------------------------------
///   Weld several vertices together
///   \param p_mesh a mesh object
///   \return true is everything is ok, else false
//---------------------------------------------------------------------
bool c_module_editor::weld()
{
	unsigned int i;

	if (selection_mode != SELECTION_MODE_VERTEX)
		return false;

	for (i = 1; i < this->selection.size(); i++)
	{
		this->set_verts_x(this->get_verts_x(this->selection[0]), this->selection[i]);
		this->set_verts_y(this->get_verts_y(this->selection[0]), this->selection[i]);
		this->set_verts_z(this->get_verts_z(this->selection[0]), this->selection[i]);
	}

	return true;
}


//---------------------------------------------------------------------
///   Add one object
///   \todo This function has to be done yet
///   \param p_object_number object number
///   \param p_x x position
///   \param p_y y position
///   \param p_z z position
///   \return true if it was successful, else false
//---------------------------------------------------------------------
bool c_module_editor::add_mesh_object(int p_object_number, float p_x, float p_y, float p_z)
{
	return false;
}


//---------------------------------------------------------------------
///   Remove all objects in the selection
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_module_editor::delete_mesh_objects()
{
	unsigned int i;

	if (this->selection.size() == 0)
		return false;

	for (i = 0; i < this->selection.size(); i++)
	{
		this->remove_spawn(this->selection[i]);
	}

	return true;
}


//---------------------------------------------------------------------
///   Move one object
///   \todo This function has to be done yet
///   \param p_spawn_number spawn number
///   \param p_new_x new x position
///   \param p_new_y new y position
///   \param p_new_z new z position
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_module_editor::move_mesh_object(int p_spawn_number, float p_new_x, float p_new_y, float p_new_z)
{
	return false;
}


//---------------------------------------------------------------------
///   Modify the x position of the items in the selection
///   \param p_off_x x offset
///   \todo object mode
//---------------------------------------------------------------------
void c_module_editor::modify_x(float p_off_x)
{
	unsigned int i;

	switch (this->selection_mode)
	{
		case SELECTION_MODE_VERTEX:
			for (i = 0; i < this->selection.size(); i++)
			{
				this->modify_verts_x(p_off_x, this->selection[i]);
			}
			break;

		case SELECTION_MODE_OBJECT:
			break;

		default:
			break;
	}
}


//---------------------------------------------------------------------
///   Modify the y position of the items in the selection
///   \param p_off_y y offset
///   \todo object mode
//---------------------------------------------------------------------
void c_module_editor::modify_y(float p_off_y)
{
	unsigned int i;

	switch (this->selection_mode)
	{
		case SELECTION_MODE_VERTEX:
			for (i = 0; i < this->selection.size(); i++)
			{
				this->modify_verts_y(p_off_y, this->selection[i]);
			}
			break;

		case SELECTION_MODE_OBJECT:
			break;

		default:
			break;
	}
}


//---------------------------------------------------------------------
///   Modify the z position of the items in the selection
///   \param p_off_z z offset
///   \todo object mode
//---------------------------------------------------------------------
void c_module_editor::modify_z(float p_off_z)
{
	unsigned int i;

	switch (this->selection_mode)
	{
		case SELECTION_MODE_VERTEX:
			for (i = 0; i < this->selection.size(); i++)
			{
				this->modify_verts_z(p_off_z, this->selection[i]);
			}
			break;

		case SELECTION_MODE_OBJECT:
			break;

		default:
			break;
	}
}


//---------------------------------------------------------------------
///   Change all textures in the selection
///   \param p_size texture size (small / big textures)
///   \todo do MODE_PAINT
//---------------------------------------------------------------------
void c_module_editor::change_texture(int p_size)
{
	unsigned int i;
	int fan;

	if ((this->selection_mode != SELECTION_MODE_TEXTURE) &&
	    (this->selection_mode != SELECTION_MODE_TILE))
		return;

	switch (this->add_mode)
	{
		// Direct painting
		case MODE_PAINT:
			this->clear_selection();

			this->tiles[g_nearest_tile].tile = this->current_item;

			// Change the tile type (for big tiles)
			if (p_size == 64)
				this->tiles[g_nearest_tile].type = 32;
			else
				this->tiles[g_nearest_tile].type = 0;
			break;

		case MODE_OVERWRITE:
		case MODE_ADD:
			for (i = 0; i < this->selection.size(); i++)
			{
				fan = this->selection[i];
				this->tiles[fan].tile = this->current_item;

				// Change the tile type (for big tiles)
				if (p_size == 64)
					this->tiles[fan].type = 32;
				else
					this->tiles[fan].type = 0;
			}
			break;

		default:
			break;
	}
}


//---------------------------------------------------------------------
///   Change the tile flag (water, reflection, ...)
///   \param p_flag new flag
//---------------------------------------------------------------------
void c_module_editor::set_tile_flag(unsigned char p_flag)
{
	unsigned int i;
	int fan;

	if (this->selection_mode != SELECTION_MODE_TFLAG)
		return;

	for (i = 0; i < this->selection.size(); i++)
	{
		fan = this->selection[i];

		this->tiles[fan].fx = p_flag;
	}
}


//---------------------------------------------------------------------
///   Change the tile type (wall, floor, ...)
///   \param p_type new type
//---------------------------------------------------------------------
void c_module_editor::set_tile_type(int p_type)
{
	unsigned int i;
	int fan;

	if (this->selection_mode != SELECTION_MODE_TTYPE)
		return;

	for (i = 0; i < this->selection.size(); i++)
	{
		fan = this->selection[i];

		this->tiles[fan].type = p_type;
		this->change_mem_size(this->vrt_count + 2, this->tile_count);
	}
}


//---------------------------------------------------------------------
///   Transform the selection into wall tiles
///   \todo This function has to be done yet
//---------------------------------------------------------------------
void c_module_editor::make_wall()
{
	unsigned int i;
	int fan;
	vector<int> vec_vertices;

	for (i = 0; i < this->selection.size(); i++)
	{
		fan = this->selection[i];

		// Is it a wall already?
		if (0 != (this->tiles[fan].fx & MESHFX_WALL))
		{
			cout << "Tile already is a wall" << endl;
			// Nothing to do
			break;
		}

		// Set the tile flags to "wall"
		this->tiles[fan].fx = MESHFX_WALL | MESHFX_IMPASS | MESHFX_SHA;

		// Set the tile type
		// TODO: make type dependent on the surrounding tiles
		// TODO: randomize floor tiles for a nicer look

		// Change the number of vertices in the tile
		this->change_tile_type(fan, TTYPE_PILLER_TEN);


//		vec_vertices = this->get_fan_vertices(fan);

//		// Get the height of the first vertex
//		newheight = this->get_verts_z(vec_vertices[0]) + 192;

//		for (vec = 0; vec < (int)vec_vertices.size(); vec++)
//		{
//			// Set the height
//			this->set_verts_z(newheight, vec_vertices[vec]);
//		}


		// TODO: Change the texture
		// TODO: Autoweld
	}
}


//---------------------------------------------------------------------
///   Transform the selection into floor tiles
///   \todo This function has to be done yet
//---------------------------------------------------------------------
void c_module_editor::make_floor()
{
}


//---------------------------------------------------------------------
///   Set add mode
///   \param p_mode true for "add", false for "overwrite current selection"
//---------------------------------------------------------------------
void c_module_editor::set_add_mode(int p_mode)
{
	this->add_mode = p_mode;
}


//---------------------------------------------------------------------
///   Get add mode
///   \return current add mode
//---------------------------------------------------------------------
int c_module_editor::get_add_mode()
{
	return this->add_mode;
}


//---------------------------------------------------------------------
///   Set the selction mode
///   \param p_mode selection mode
//---------------------------------------------------------------------
void c_module_editor::set_selection_mode(int p_mode)
{
	this->selection_mode = p_mode;
}


//---------------------------------------------------------------------
///   Get the selction mode
///   \return selection mode
//---------------------------------------------------------------------
int c_module_editor::get_selection_mode()
{
	return this->selection_mode;
}


//---------------------------------------------------------------------
///   Set current texture
///   \param p_texture new current texture
//---------------------------------------------------------------------
void c_module_editor::set_current_item(int p_item)
{
	this->current_item = p_item;
}


//---------------------------------------------------------------------
///   Get current texture
///   \return current texture
//---------------------------------------------------------------------
int c_module_editor::get_current_item()
{
	return this->current_item;
}


//---------------------------------------------------------------------
///   Get selection size
///   \return current texture
//---------------------------------------------------------------------
int c_module_editor::get_selection_size()
{
	return this->selection.size();
}
