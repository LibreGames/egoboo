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
/// @brief spawn and object definition
/// @details Definition of the spawns and objects

#ifndef object_h
#define object_h
//---------------------------------------------------------------------
//-
//-   Everything related to spawns and objects
//-
//---------------------------------------------------------------------
#include "../general.h"
#include "../SDL_extensions.h"
#include "../ogl_extensions.h"

#include <string>
#include <vector>

using namespace std;


//---------------------------------------------------------------------
///   Definition of one spawn (= one entry in spawn.txt)
//---------------------------------------------------------------------
class c_spawn
{
	public:
		int id;               ///< ID for internal handling
		string name;          ///< Display name
		int slot_number;      ///< Egoboo slot number
		vect3 pos;            ///< Position
		char direction;       ///< Direction
		int money;            ///< Bonus money for this module
		int skin;             ///< The skin
		int passage;          ///< Reference passage
		int content;          ///< Content (for armor chests)
		int lvl;              ///< Character level
		char status_bar;      ///< Draw a status bar for this character?
		char ghost;           ///< Is the character a ghost?
		string team;          ///< Team
};


//---------------------------------------------------------------------
///   Definition of one object in the module/objects/ folder
//---------------------------------------------------------------------
class c_object
{
	private:
		string name;    ///< the name, like object.obj
		bool gor;       ///< Is it in the Global Object Repository?
		int slot;       ///< The slot number
		GLuint icon[1]; ///< The icon texture

	public:
		c_object();
		~c_object();

		///   Get the object name
		///   \return object name
		string get_name()              { return this->name; }
		///   Set the object name
		///   \param p_name new object name
		void   set_name(string p_name) { this->name = p_name; }

		///   Get the GOR flag
		///   \return GOR flag
		bool   get_gor()           { return this->gor; }
		///   Set the GOR flag
		///   \param p_gor new GOR flag
		void   set_gor(bool p_gor) { this->gor = p_gor; }

		///   Get the slot number
		///   \return slot number
		int    get_slot()           { return this->slot; }
		///   Set the slot number
		///   \param p_slot new slot number
		void   set_slot(int p_slot) { this->slot = p_slot; }

		///   Get the OpenGL icon
		///   \return OpenGL icon
		GLuint get_icon()            { return this->icon[0]; }
		///   Get the whole OpenGL icon array
		///   \return OpenGL icon array
		GLuint *get_icon_array()     { return this->icon; }
		///   Set the OpenGL icon
		///   \param p_icon new OpenGL icon
		void set_icon(GLuint p_icon) { this->icon[0] = p_icon; }

		bool read_data_file(string);
		bool render();
		void debug_data();
};


//---------------------------------------------------------------------
///   Object and spawn manager
//---------------------------------------------------------------------
class c_object_manager
{
	private:
		vector<c_spawn>  spawns;
		vector<c_object*> objects;

	public:
		c_object_manager();
		~c_object_manager();

		void clear_objects()
		{
			this->objects.clear();
		}
		void clear_spawns()
		{
			this->spawns.clear();
		}

		void load_spawns(string);
		bool save_spawns(string);

		void load_objects(string, bool);
		bool save_objects(string);

		void add_spawn(c_spawn);
		void remove_spawn(int);

		unsigned int get_spawns_size();
		c_spawn get_spawn(int);
		void    set_spawn(int, c_spawn);

		unsigned int get_objects_size();
		c_object* get_object(int);
		c_object* get_object_by_slot(int);
		void      set_object(int, c_object*);

		vector<c_spawn>  get_spawns();
		vector<c_object*> get_objects();

		void load_all_for_module(string, string);

		int get_nearest_object(float, float, float);

		void    render();
};
#endif
