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
}


//---------------------------------------------------------------------
//-   Select a single vertex to the selection
//---------------------------------------------------------------------
void c_selection::add_vertex(int vertex_number)
{
	this->selection.push_back(vertex_number);
}


//---------------------------------------------------------------------
//-   Clear the selection
//---------------------------------------------------------------------
void c_selection::clear()
{
	this->selection.clear();
}


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
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

	for (i = 0; i < this->selection.size(); i++)
	{
		g_mesh.modify_verts_x(off_x, this->selection[i]);
	}
}


//---------------------------------------------------------------------
//-   Modify all vertices in the selection (Z direction)
//---------------------------------------------------------------------
void c_selection::modify_y(float off_y)
{
	unsigned int i;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_mesh.modify_verts_y(off_y, this->selection[i]);
	}
}


//---------------------------------------------------------------------
//-   Modify all vertices in the selection (Z direction)
//---------------------------------------------------------------------
void c_selection::modify_z(float off_z)
{
	unsigned int i;

	for (i = 0; i < this->selection.size(); i++)
	{
		g_mesh.modify_verts_z(off_z, this->selection[i]);
	}
}


//---------------------------------------------------------------------
//-   Returns true if a vertex is in the selection
//---------------------------------------------------------------------
bool c_selection::in_selection(int vertex)
{
	unsigned int i;

	for (i = 0; i < this->selection.size(); i++)
	{
		if (this->selection[i] == vertex)
			return true;
	}

	return false;
}
