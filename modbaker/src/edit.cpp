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
#include "edit.h"
#include "global.h"

#include <iostream>
using namespace std;

//---------------------------------------------------------------------
///   c_selection constructor
//---------------------------------------------------------------------
c_selection::c_selection()
{
	this->add_mode = false;
	this->selection_mode = SELECTION_MODE_VERTEX;
	this->texture = 0;
}


//---------------------------------------------------------------------
///   Add a single vertex to the selection
///   \param p_vertex_number a vertex number
//---------------------------------------------------------------------
void c_selection::add_vertex(int p_vertex_number)
{
	if (in_selection(p_vertex_number))
	{
		this->remove_vertex(p_vertex_number);
	}
	else
	{
		this->selection.push_back(p_vertex_number);
	}
}


//---------------------------------------------------------------------
///   Remove a vertex from the selection
///   \param p_vertex_number a vertex number
//---------------------------------------------------------------------
void c_selection::remove_vertex(int p_vertex_number)
{
	vector <int> new_selection;
	unsigned int i;

	for (i=0; i<this->selection.size(); i++)
	{
		if (p_vertex_number != this->selection[i])
		{
			new_selection.push_back(this->selection[i]);
		}
	}

	this->selection = new_selection;
}


//---------------------------------------------------------------------
///   Add a tile to the selection
///   \param p_tile_number a tile number
//---------------------------------------------------------------------
void c_selection::add_tile(int p_tile_number)
{
	if (in_selection(p_tile_number))
	{
		this->remove_tile(p_tile_number);
	}
	else
	{
		this->selection.push_back(p_tile_number);
	}
}


//---------------------------------------------------------------------
///   Remove a tile from the selection
///   \param p_tile_number a tile number
//---------------------------------------------------------------------
void c_selection::remove_tile(int p_tile_number)
{
	vector <int> new_selection;
	unsigned int i;

	for (i=0; i<this->selection.size(); i++)
	{
		if (p_tile_number != this->selection[i])
		{
			new_selection.push_back(this->selection[i]);
		}
	}

	this->selection = new_selection;
}


//---------------------------------------------------------------------
///   Add a tile to the selection
//    \param p_object_number an object number
//---------------------------------------------------------------------
void c_selection::add_object(int p_object_number)
{
	if (in_selection(p_object_number))
	{
		this->remove_object(p_object_number);
	}
	else
	{
		this->selection.push_back(p_object_number);
	}
}


//---------------------------------------------------------------------
///   Remove an object from the selection
///   \param p_object_number an object number
//---------------------------------------------------------------------
void c_selection::remove_object(int p_object_number)
{
	vector <int> new_selection;
	unsigned int i;

	for (i=0; i<this->selection.size(); i++)
	{
		if (p_object_number != this->selection[i])
		{
			new_selection.push_back(this->selection[i]);
		}
	}

	this->selection = new_selection;
}


//---------------------------------------------------------------------
///   Clear the selection
//---------------------------------------------------------------------
void c_selection::clear()
{
	this->selection.clear();
}


//---------------------------------------------------------------------
///   Weld several vertices together
///   \param p_mesh a mesh object
///   \return true is everything is ok, else false
//---------------------------------------------------------------------
bool c_selection::weld(c_mesh *p_mesh)
{
	unsigned int i;

	if (selection_mode != SELECTION_MODE_VERTEX)
		return false;

	for (i = 1; i < this->selection.size(); i++)
	{
		p_mesh->mem->set_verts_x(p_mesh->mem->get_verts_x(this->selection[0]), this->selection[i]);
		p_mesh->mem->set_verts_y(p_mesh->mem->get_verts_y(this->selection[0]), this->selection[i]);
		p_mesh->mem->set_verts_z(p_mesh->mem->get_verts_z(this->selection[0]), this->selection[i]);
	}

	return true;
}


//---------------------------------------------------------------------
///   Add vertices at a vertain position to the selection
///   TODO: Currently this function does nothing
///   \param p_ref reference point
///   \return number of vertices added to the function
//---------------------------------------------------------------------
int c_selection::add_vertices_at_position(vect3 p_ref)
{
	int num_vertices;

	num_vertices = -1;

	return num_vertices;
}


//---------------------------------------------------------------------
///   Modify all vertices in the selection (X direction)
///   \param p_off_x x offset
//---------------------------------------------------------------------
void c_selection::modify_x(float p_off_x)
{
	unsigned int i;

	if (selection_mode != SELECTION_MODE_VERTEX)
		return;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_module->modify_verts_x(p_off_x, this->selection[i]);
	}
}


//---------------------------------------------------------------------
///   Modify all vertices in the selection (Y direction)
///   \param p_off_y y offset
//---------------------------------------------------------------------
void c_selection::modify_y(float p_off_y)
{
	unsigned int i;

	if (selection_mode != SELECTION_MODE_VERTEX)
		return;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_module->modify_verts_y(p_off_y, this->selection[i]);
	}
}


//---------------------------------------------------------------------
///   Modify all vertices in the selection (Z direction)
///   \param p_off_z z offset
//---------------------------------------------------------------------
void c_selection::modify_z(float p_off_z)
{
	unsigned int i;

	if (selection_mode != SELECTION_MODE_VERTEX)
		return;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_module->modify_verts_z(p_off_z, this->selection[i]);
	}
}


//---------------------------------------------------------------------
///   Returns true if a vertex / object / tile is in the selection
///   \param p_needle item to search
///   \return true if item is in the selection, else false
//---------------------------------------------------------------------
bool c_selection::in_selection(int p_needle)
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
///   Change all textures in the selection
//---------------------------------------------------------------------
void c_selection::change_texture()
{
	unsigned int i;
	int fan;

	if ((selection_mode != SELECTION_MODE_TILE) &&
		(selection_mode != SELECTION_MODE_VERTEX))
		return;

	for (i = 0; i < this->selection.size(); i++)
	{
		fan = this->selection[i];
		g_module->mem->tiles[fan].tile = this->texture;

		// Change the tile type (for big tiles)
		if (this->texture >= 64)
			g_module->mem->tiles[fan].type = 32;
		else
			g_module->mem->tiles[fan].type = 0;
	}
}


//---------------------------------------------------------------------
///   Change the tile flag (water, reflection, ...)
///   \param p_flag new flag
//---------------------------------------------------------------------
void c_selection::set_tile_flag(unsigned char p_flag)
{
	unsigned int i;
	int fan;

	for (i = 0; i < this->selection.size(); i++)
	{
		fan = this->selection[i];

		g_module->mem->tiles[fan].fx = p_flag;
	}
}


//---------------------------------------------------------------------
///   Change the tile type (wall, floor, ...)
///   \param p_type new type
//---------------------------------------------------------------------
void c_selection::set_tile_type(int p_type)
{
	unsigned int i;
	int fan;

	for (i = 0; i < this->selection.size(); i++)
	{
		fan = this->selection[i];

		g_module->mem->tiles[fan].type = p_type;
		g_module->change_mem_size(g_module->mem->vrt_count + 2, g_module->mem->tile_count);
	}
}


//---------------------------------------------------------------------
///   Transform the selection into wall tiles
///   TODO: This function has to be done yet
//---------------------------------------------------------------------
void c_selection::make_wall()
{
	unsigned int i;
	int fan;
	vector<int> vec_vertices;

	for (i = 0; i < this->selection.size(); i++)
	{
		fan = this->selection[i];

		// Is it a wall already?
		if (0 != (g_module->mem->tiles[fan].fx & MESHFX_WALL))
		{
			cout << "Tile already is a wall" << endl;
			// Nothing to do
			break;
		}

		// Set the tile flags to "wall"
		g_module->mem->tiles[fan].fx = MESHFX_WALL | MESHFX_IMPASS | MESHFX_SHA;

		// Set the tile type
		// TODO: make type dependent on the surrounding tiles
		// TODO: randomize floor tiles for a nicer look

		// Change the number of vertices in the tile
		g_module->mem->change_tile_type(fan, TTYPE_PILLER_TEN);

/*
		vec_vertices = g_module->get_fan_vertices(fan);

		// Get the height of the first vertex
		newheight = g_module->mem->get_verts_z(vec_vertices[0]) + 192;

		for (vec = 0; vec < (int)vec_vertices.size(); vec++)
		{
			// Set the height
			g_module->mem->set_verts_z(newheight, vec_vertices[vec]);
		}
*/

		// TODO: Change the texture
		// TODO: Autoweld
	}
}


//---------------------------------------------------------------------
///   Transform the selection into floor tiles
///   TODO: This function has to be done yet
//---------------------------------------------------------------------
void c_selection::make_floor()
{
}


//---------------------------------------------------------------------
///   Add one object
///   TODO: This function has to be done yet
///   \param p_object_number object number
///   \param p_x x position
///   \param p_y y position
///   \param p_z z position
///   \return true if it was successful, else false
//---------------------------------------------------------------------
bool c_selection::add_mesh_object(int p_object_number, float p_x, float p_y, float p_z)
{
	return false;
}


//---------------------------------------------------------------------
///   Remove all objects in the selection
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_selection::remove_objects()
{
	unsigned int i;

	if (this->selection.size() == 0)
		return false;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_module->remove_spawn(this->selection[i]);
	}

	return true;
}


//---------------------------------------------------------------------
///   Move one object
///   TODO: This function has to be done yet
///   \param p_spawn_number spawn number
///   \param p_new_x new x position
///   \param p_new_y new y position
///   \param p_new_z new z position
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_selection::move_mesh_object(int p_spawn_number, float p_new_x, float p_new_y, float p_new_z)
{
	return false;
}
