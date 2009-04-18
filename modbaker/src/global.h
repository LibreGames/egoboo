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


// Dummy classes
class c_config;
class c_spawn_manager;

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
extern c_config         g_config;
extern c_renderer       g_renderer;
extern c_mesh           g_mesh;
extern c_frustum        g_frustum;           // TODO: Move to g_renderer
extern c_spawn_manager  g_spawn_manager;

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


//---------------------------------------------------------------------
//-   This resembles one entry in spawn.txt
//---------------------------------------------------------------------
class c_spawn
{
	public:
		int id;               // ID for internal handling
		string name;          // Display name
		int slot_number;      // Egoboo slot number
		vect3 pos;            // Position
		char direction;       // Direction
		int money;            // Bonus money for this module
		int skin;             // The skin
		int passage;          // Reference passage
		int content;          // Content (for armor chests)
		int lvl;              // Character level
		char status_bar;      // Draw a status bar for this character?
		char ghost;           // Is the character a ghost?
		string team;          // Team
};


//---------------------------------------------------------------------
//-   spawn.txt handling
//---------------------------------------------------------------------
class c_spawn_manager
{
//	private:
	public:
		vector<c_spawn> spawns;
		c_spawn_manager();
		~c_spawn_manager();

		void load(string);
		bool save(string);

		void add(c_spawn);
		void remove(int);

		c_spawn get_spawn(int);
		void    set_spawn(int, c_spawn);
		void    render();
};
