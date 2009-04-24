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

#include "general.h"

#include <vector>

using namespace std;

class c_selection
{
	private:
		vector<int> selection;
		bool add_mode;

	public:
		c_selection();
//		~c_selection();
		void add_vertex(int);
		void remove_vertex(int);
		int add_vertices_at_position(vect3);

		void set_add_mode(bool mode) { this->add_mode = mode; }
		bool get_add_mode()          { return this->add_mode; }

		void clear();

		void modify_x(float);
		void modify_y(float);
		void modify_z(float);

		void change_texture(int, int);

		bool in_selection(int);
};
#endif
