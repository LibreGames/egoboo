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
#ifndef edit_h
#define edit_h

#include <SDL.h>
#include <SDL_types.h>
#include <SDL_endian.h>

#include "general.h"
#include "mesh.h"

#include <vector>

using namespace std;

enum
{
	SELECTION_MODE_VERTEX,
	SELECTION_MODE_TILE,
	SELECTION_MODE_OBJECT
};

class c_selection
{
	private:
		vector<int> selection;
		bool add_mode;
		int texture;
		int selection_mode;

	public:
		c_selection();
//		~c_selection(); // TODO: Clean up the selection vetor
		void add_vertex(int);
		void remove_vertex(int);

		void add_tile(int);
		void remove_tile(int);

		void add_object(int);
		void remove_object(int);

		int add_vertices_at_position(vect3);

		void set_add_mode(bool mode) { this->add_mode = mode; }
		bool get_add_mode()          { return this->add_mode; }

		void set_selection_mode(int mode) { this->selection_mode = mode; }
		int get_selection_mode()          { return this->selection_mode; }

		void set_texture(int p_texture) { this->texture = p_texture; }
		int  get_texture()              { return this->texture; }

		bool weld(c_mesh *);

		void clear();

		void modify_x(float);
		void modify_y(float);
		void modify_z(float);

		void change_texture();

		void set_tile_flag(unsigned char);
		void set_tile_type(int);

		void make_wall();
		void make_floor();

		bool in_selection(int);

		bool add_mesh_object(int, float, float, float);
		bool remove_objects();
		bool move_mesh_object(int, float, float, float);
};
#endif
