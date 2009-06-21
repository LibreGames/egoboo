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
//-   c_selection constructor
//---------------------------------------------------------------------
c_selection::c_selection()
{
	this->add_mode = false;
	this->selection_mode = SELECTION_MODE_VERTEX;
	this->texture = 0;
}


//---------------------------------------------------------------------
//-   Select a single vertex to the selection
//---------------------------------------------------------------------
void c_selection::add_vertex(int vertex_number)
{
	if (in_selection(vertex_number))
	{
		this->remove_vertex(vertex_number);
	}
	else
	{
		this->selection.push_back(vertex_number);
	}
}


//---------------------------------------------------------------------
//-   Remove a vertex from the selection
//---------------------------------------------------------------------
void c_selection::remove_vertex(int vertex_number)
{
	vector <int> new_selection;
	unsigned int i;

	for (i=0; i<this->selection.size(); i++)
	{
		if (vertex_number != this->selection[i])
		{
			new_selection.push_back(this->selection[i]);
		}
	}

	this->selection = new_selection;
}


//---------------------------------------------------------------------
//-   Select a tile to the selection
//---------------------------------------------------------------------
void c_selection::add_tile(int tile_number)
{
	if (in_selection(tile_number))
	{
		this->remove_tile(tile_number);
	}
	else
	{
		this->selection.push_back(tile_number);
	}
}


//---------------------------------------------------------------------
//-   Remove a tile from the selection
//---------------------------------------------------------------------
void c_selection::remove_tile(int tile_number)
{
	vector <int> new_selection;
	unsigned int i;

	for (i=0; i<this->selection.size(); i++)
	{
		if (tile_number != this->selection[i])
		{
			new_selection.push_back(this->selection[i]);
		}
	}

	this->selection = new_selection;
}


//---------------------------------------------------------------------
//-   Select a tile to the selection
//---------------------------------------------------------------------
void c_selection::add_object(int object_number)
{
	if (in_selection(object_number))
	{
		this->remove_object(object_number);
	}
	else
	{
		this->selection.push_back(object_number);
	}
}


//---------------------------------------------------------------------
//-   Remove an object from the selection
//---------------------------------------------------------------------
void c_selection::remove_object(int object_number)
{
	vector <int> new_selection;
	unsigned int i;

	for (i=0; i<this->selection.size(); i++)
	{
		if (object_number != this->selection[i])
		{
			new_selection.push_back(this->selection[i]);
		}
	}

	this->selection = new_selection;
}


//---------------------------------------------------------------------
//-   Clear the selection
//---------------------------------------------------------------------
void c_selection::clear()
{
	this->selection.clear();
}


//---------------------------------------------------------------------
//-   Weld several vertices together
//---------------------------------------------------------------------
bool c_selection::weld(c_mesh *p_mesh)
{
	unsigned int i;

	if (selection_mode != SELECTION_MODE_VERTEX)
		return false;

	for (i = 1; i < this->selection.size(); i++)
	{
		p_mesh->set_verts_x(p_mesh->get_verts_x(this->selection[0]), this->selection[i]);
		p_mesh->set_verts_y(p_mesh->get_verts_y(this->selection[0]), this->selection[i]);
		p_mesh->set_verts_z(p_mesh->get_verts_z(this->selection[0]), this->selection[i]);
	}

	return true;
}


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
// TODO
int c_selection::add_vertices_at_position(vect3 ref)
{
	int num_vertices;

	num_vertices = -1;

	return num_vertices;
}


//---------------------------------------------------------------------
//-   Modify all vertices in the selection (X direction)
//---------------------------------------------------------------------
void c_selection::modify_x(float off_x)
{
	unsigned int i;

	if (selection_mode != SELECTION_MODE_VERTEX)
		return;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_mesh->modify_verts_x(off_x, this->selection[i]);
	}
}


//---------------------------------------------------------------------
//-   Modify all vertices in the selection (Y direction)
//---------------------------------------------------------------------
void c_selection::modify_y(float off_y)
{
	unsigned int i;

	if (selection_mode != SELECTION_MODE_VERTEX)
		return;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_mesh->modify_verts_y(off_y, this->selection[i]);
	}
}


//---------------------------------------------------------------------
//-   Modify all vertices in the selection (Z direction)
//---------------------------------------------------------------------
void c_selection::modify_z(float off_z)
{
	unsigned int i;

	if (selection_mode != SELECTION_MODE_VERTEX)
		return;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_mesh->modify_verts_z(off_z, this->selection[i]);
	}
}


//---------------------------------------------------------------------
//-   Returns true if a vertex / object / tile is in the selection
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
//-   Change all textures in the selection
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
		g_mesh->mem->tilelst[fan].tile = this->texture;

		// Change the tile type (for big tiles)
		if (this->texture >= 64)
			g_mesh->mem->tilelst[fan].type = 32;
		else
			g_mesh->mem->tilelst[fan].type = 0;
	}
}


void c_selection::set_tile_flag(unsigned char p_flag)
{
	unsigned int i;
	int fan;

	for (i = 0; i < this->selection.size(); i++)
	{
		fan = this->selection[i];

		g_mesh->mem->tilelst[fan].fx = p_flag;
	}
}


void c_selection::set_tile_type(int p_type)
{
	unsigned int i;
	int fan;

	// TODO: Set fan FX to MESHFXWALL | MESHFXIMPASS | MESHFXSHA

	for (i = 0; i < this->selection.size(); i++)
	{
		fan = this->selection[i];

		g_mesh->mem->tilelst[fan].type = p_type;
	}
}


//---------------------------------------------------------------------
//-   Add one object
//---------------------------------------------------------------------
bool c_selection::add_mesh_object(int p_object_number, float p_x, float p_y, float p_z)
{
	return false;
}


//---------------------------------------------------------------------
//-   Remove all objects in the selection
//---------------------------------------------------------------------
bool c_selection::remove_objects()
{
	unsigned int i;

	if (this->selection.size() == 0)
		return false;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_object_manager->remove_spawn(this->selection[i]);
	}

	return true;
}


//---------------------------------------------------------------------
//-   Move one object
//---------------------------------------------------------------------
bool c_selection::move_mesh_object(int p_spawn_number, float p_new_x, float p_new_y, float p_new_z)
{
	return false;
}
