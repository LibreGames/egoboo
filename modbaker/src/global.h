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
#pragma once

#define global_h

//---------------------------------------------------------------------
//-
//-   This file contains global defined stuff
//-
//---------------------------------------------------------------------
#include "utility.h"
#include "edit.h"
#include "renderer.h"
#include "mesh.h"
#include "frustum.h"

#include <vector>
#include <stdexcept>

using namespace std;


// Action types for the input system
enum
{
	ACTION_MESH_NEW        =  0,
	ACTION_MESH_LOAD       =  1,
	ACTION_MESH_SAVE       =  2,
	ACTION_VERTEX_UP       =  3,
	ACTION_VERTEX_LEFT     =  4,
	ACTION_VERTEX_RIGHT    =  5,
	ACTION_VERTEX_DOWN     =  6,
	ACTION_VERTEX_INC      =  7,
	ACTION_VERTEX_DEC      =  8,
	ACTION_SELECTION_CLEAR =  9,
	ACTION_EXIT            = 10,
	SCROLL_UP              = 11,
	SCROLL_LEFT            = 12,
	SCROLL_RIGHT           = 13,
	SCROLL_DOWN            = 14,
	SCROLL_INC             = 15,
	SCROLL_DEC             = 16,
	SCROLL_X_STOP          = 17,
	SCROLL_Y_STOP          = 18,
	SCROLL_Z_STOP          = 19,
	ACTION_MODIFIER_ON     = 20,
	ACTION_MODIFIER_OFF    = 21,
	ACTION_PAINT_TEXTURE   = 22,
	WINDOW_TEXTURE_TOGGLE  = 23,
	WINDOW_SAVE_SHOW       = 24,
	WINDOW_SAVE_HIDE       = 25,
	WINDOW_LOAD_SHOW       = 26,
	WINDOW_LOAD_HIDE       = 27
};


// Dummy classes
class c_config;

//---------------------------------------------------------------------
//-   Class definition for the configuration file
//---------------------------------------------------------------------
// Mouse / input stuff
// Implemented in controls.cpp
extern int g_mouse_x;
extern int g_mouse_y;

extern float g_mouse_gl_x;
extern float g_mouse_gl_y;
extern float g_mouse_gl_z;

// Selection
// Implemented in edit.cpp
extern int g_nearest_vertex;
extern c_selection g_selection;

// Global subsystems
// Implmented in renderer.cpp, renderer_misc.cpp, mesh.cpp, frustum.cpp
extern c_input_handler* g_input_handler;
extern c_config*        g_config;
extern c_renderer*      g_renderer;
extern c_mesh*          g_mesh;
extern c_frustum        g_frustum;           // TODO: Move to g_renderer

// Misc stuff
// Implmented in global.cpp
extern float calculate_distance(vect3, vect3); // TODO: Move to graphic stuff
extern void Quit();

class modbaker_exception : public runtime_error
{
	public:
		// Constructors
		modbaker_exception(const string &error)
				: std::runtime_error(error) { };

		modbaker_exception(const string &error, const string &filename)
				: std::runtime_error(error), _which(filename) { };

		virtual ~modbaker_exception() throw() { }

		// Public interace
		virtual const char *which() const throw()
		{
			return _which.c_str();
		}

	private:
		// Member variables
		string _which;
};


//---------------------------------------------------------------------
//-   Class definition for the configuration file
//---------------------------------------------------------------------
class c_config
{
	private:
		string m_egoboo_path;
		int    m_font_size;
		string m_font_file;

	public:
		c_config();
		~c_config();

		string get_egoboo_path();
		void   set_egoboo_path(string);

		int    get_font_size();
		void   set_font_size(int);

		string get_font_file();
		void   set_font_file(string);
};
