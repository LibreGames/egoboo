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
#ifndef modbaker_h
#define modbaker_h
//---------------------------------------------------------------------
//-
//-   The main class
//-
//---------------------------------------------------------------------
#include "global.h"
#include "mesh.h"
#include "renderer.h"

class c_modbaker 
{
	private:
		bool done;
		bool active;

		bool selection_add;

		// Keyboard input
		void handle_key_press(SDL_keysym*);
		void handle_key_release(SDL_keysym*);

		int handle_window_events();
		int handle_game_events();

	public:
		c_modbaker();
//		~c_modbaker();

		void init(string);
		void main_loop();
		void get_GL_pos(int, int);
};
#endif
