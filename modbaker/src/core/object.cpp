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
/// @brief spawn and object implementation
/// @details Implementation of the spawns and objects

//---------------------------------------------------------------------
//-
//-   Everything related to spawns and objects
//-
//---------------------------------------------------------------------
#include "../general.h"
#include "../global.h"
#include "object.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;


//---------------------------------------------------------------------
///   Constructor
//---------------------------------------------------------------------
c_object_manager::c_object_manager()
{
	cout << "Creating the spawn manager" << endl;
}


//---------------------------------------------------------------------
///   Destructor
//---------------------------------------------------------------------
c_object_manager::~c_object_manager()
{
	cout << "Deleting the spawn manager" << endl;
}


//---------------------------------------------------------------------
///   Load all objects & spawns for a module
///   \param p_egoboo_dir path to the Egoboo directory
///   \param p_modname module name to load
//---------------------------------------------------------------------
void c_object_manager::load_all_for_module(string p_egoboo_dir, string p_modname)
{
	string filename;

	this->clear_objects();
	this->clear_spawns();

	filename = p_egoboo_dir + "modules/" + p_modname + "/objects/";
	this->load_objects(filename, false);

	filename = p_egoboo_dir + "basicdat/globalobjects/";
	this->load_objects(filename, true);

	filename = p_egoboo_dir + "modules/" + p_modname + "/gamedat/spawn.txt";
	this->load_spawns(filename);
}


//---------------------------------------------------------------------
///   Load all spawns from the provided file
///   \param p_filename filename to load
//---------------------------------------------------------------------
void c_object_manager::load_spawns(string p_filename)
{
	ifstream f_spawn;
	c_spawn temp_spawn;

	f_spawn.open(p_filename.c_str());

	if (!f_spawn)
	{
		cout << "File " << p_filename << " not found!" << endl;
		Quit();
	}

	string sTmp;
	int    iTmp;
	float  fTmp;
	char   cTmp;

	while (!f_spawn.eof())
	{
		if (fgoto_colon_yesno(f_spawn))
		{
			f_spawn >> sTmp;  temp_spawn.name        = sTmp;
			f_spawn >> iTmp;  temp_spawn.slot_number = iTmp;
			f_spawn >> fTmp;  temp_spawn.pos.x       = fTmp;
			f_spawn >> fTmp;  temp_spawn.pos.y       = fTmp;
			f_spawn >> fTmp;  temp_spawn.pos.z       = fTmp;
			f_spawn >> cTmp;  temp_spawn.direction   = cTmp;
			f_spawn >> iTmp;  temp_spawn.money       = iTmp;
			f_spawn >> iTmp;  temp_spawn.skin        = iTmp;
			f_spawn >> iTmp;  temp_spawn.passage     = iTmp;
			f_spawn >> iTmp;  temp_spawn.content     = iTmp;
			f_spawn >> iTmp;  temp_spawn.lvl         = iTmp;
			f_spawn >> cTmp;  temp_spawn.status_bar  = cTmp;
			f_spawn >> cTmp;  temp_spawn.ghost       = cTmp;
			f_spawn >> sTmp;  temp_spawn.team        = sTmp;

			this->spawns.push_back(temp_spawn);
		}
	}

	f_spawn.close();
}


//---------------------------------------------------------------------
///   Save spawns.txt
///   Currently unused
///   \param p_modname module name
///   \return false
//---------------------------------------------------------------------
bool c_object_manager::save_spawns(string p_modname)
{
	return false;
}


//---------------------------------------------------------------------
///   Add a spawn
///   \param p_spawn spawn entry to add
//---------------------------------------------------------------------
void c_object_manager::add_spawn(c_spawn p_spawn)
{

}


//---------------------------------------------------------------------
///   Remove a spawn
///   \param p_slot_number spawn slot number to remove
//---------------------------------------------------------------------
void c_object_manager::remove_spawn(int p_slot_number)
{
	this->spawns.erase(this->spawns.begin()+p_slot_number);
}


//---------------------------------------------------------------------
///   Get the number of spawns
///   \return number of spawns
//---------------------------------------------------------------------
unsigned int c_object_manager::get_spawns_size()
{
	return this->spawns.size();
}


//---------------------------------------------------------------------
///   Get a spawn based on the spawn number in the vector
///   \param p_slot_number spawn number
///   \return spawn object
//---------------------------------------------------------------------
c_spawn c_object_manager::get_spawn(int p_slot_number)
{
	return this->spawns[p_slot_number];
}


//---------------------------------------------------------------------
///   Sets the content of a spawn
///   \param p_slot_number spawn number
///   \param p_spawn new spawn content
//---------------------------------------------------------------------
void c_object_manager::set_spawn(int p_slot_number, c_spawn p_spawn)
{
	this->spawns[p_slot_number] = p_spawn;
}



//---------------------------------------------------------------------
///   Get the number of objects
///   \return number of objects
//---------------------------------------------------------------------
unsigned int c_object_manager::get_objects_size()
{
	return this->objects.size();
}


//---------------------------------------------------------------------
///   Get an object based on the object number in the vector
///   \param p_slot_number number of the object
///   \return object
//---------------------------------------------------------------------
c_object *c_object_manager::get_object(int p_slot_number)
{
	return this->objects[p_slot_number];
}


//---------------------------------------------------------------------
///   Get an object based on the slot number
///   \param p_slot object slot number
///   \return object
//---------------------------------------------------------------------
c_object *c_object_manager::get_object_by_slot(int p_slot)
{
	unsigned int i;

	for (i=0; i<objects.size(); i++)
	{
		if (this->objects[i]->get_slot() == p_slot)
		{
			return this->objects[i];
		}
	}

	return NULL;
}


//---------------------------------------------------------------------
///   Sets the content of an object
///   \param p_slot_number number of the object
///   \param p_object new content
//---------------------------------------------------------------------
void c_object_manager::set_object(int p_slot_number, c_object *p_object)
{
	this->objects[p_slot_number] = p_object;
}


//---------------------------------------------------------------------
///   Load all objects from the provided directory
///   \param p_dirname directory name to search in
///   \param p_gor GOR flag for the objects loaded
//---------------------------------------------------------------------
void c_object_manager::load_objects(string p_dirname, bool p_gor)
{
	unsigned int i, j, start, end;

	c_object *temp_object;
	temp_object = new c_object();

	vector<string> objnames;
	string data_file;

	read_files(p_dirname, ".obj", objnames);

	start = this->get_objects_size();
	end   = this->get_objects_size() + objnames.size();

	for (i=start; i<end; i++)
	{
		j = i - start;

		temp_object = new c_object();
		temp_object->set_name(objnames[j]);
		temp_object->set_gor(p_gor);
		temp_object->set_slot(0);     ///< \todo read slot from data.txt
		temp_object->load_icon(p_dirname);

		data_file = p_dirname + temp_object->get_name() + "/data.txt";
		temp_object->read_data_file(data_file);

		this->objects.push_back(temp_object);
	}
	cout << "Imported " << this->get_objects_size() << " objects" << endl;
}


//---------------------------------------------------------------------
///   Save all objects: UNUSED
///   \param p_modname module name
//---------------------------------------------------------------------
bool c_object_manager::save_objects(string p_modname)
{
	return false;
}


//---------------------------------------------------------------------
///   Get the nearest object
///   \param pos_x reference x position
///   \param pos_y reference y position
///   \param pos_z reference z position
///   \return object number
//---------------------------------------------------------------------
int c_object_manager::get_nearest_object(float pos_x, float pos_y, float pos_z)
{
	int i;

	int nearest_object = 0;

	double dist_temp;
	double dist_nearest;

	vect3 ref;   // The reference point
	vect3 object;  // Used for each object

	ref.x = pos_x;
	ref.y = pos_y;
	ref.z = pos_z;

	dist_nearest = 999999.9f;

	for (i = 0; i < (int)this->get_spawns_size(); i++)
	{

		object.x = this->get_spawn(i).pos.x * (1 << 7);
		object.y = this->get_spawn(i).pos.y * (1 << 7);
//		object.z = this->get_spawn(i).pos.z * (1 << 7);
		object.z = 1.0f                                 * (1 << 7);

		dist_temp = calculate_distance(object, ref);

		if (dist_temp < dist_nearest)
		{
			dist_nearest = calculate_distance(object, ref);

			nearest_object = i;
		}
	}

	return nearest_object;
}


//---------------------------------------------------------------------
///   c_object constructor
//---------------------------------------------------------------------
c_object::c_object()
{
	this->name    = "";
	this->set_icon((GLuint)0);
}


//---------------------------------------------------------------------
///   c_object destructor
//---------------------------------------------------------------------
c_object::~c_object()
{
}


//---------------------------------------------------------------------
///   Function to allow debuggin of the object content
//---------------------------------------------------------------------
void c_object::debug_data()
{
	cout << "=====================" << endl;
	cout << "= Object debug info =" << endl;
	cout << "=====================" << endl;
	cout << ".name:     " << this->name << endl;
	if (this->gor)
		cout << ".in GOR?: yes" << endl;
	else
		cout << ".in GOR?:  no" << endl;
	cout << ".slot:     " << this->slot << endl;
	cout << ".icon:     " << (int)this->get_icon() << endl;
}



//---------------------------------------------------------------------
///   Load the icon for this object
///   \param p_dirname directory name for the objects folder
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_object::load_icon(string p_dirname)
{
	if (this->load_image(p_dirname + this->name + "/icon0.bmp"))
		return true;

	// icon0.bmp not found, so try to load the tris0.bmp instead
	if(this->load_image(p_dirname + this->name + "/tris0.bmp"))
		return true;

	// Else set the icon to 0
	this->set_icon(0);
	return false;
}


//---------------------------------------------------------------------
///   Read file data.txt file to get the slot number
///   \param p_filename path + filename of the data.txt file
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_object::read_data_file(string p_filename)
{
	ifstream file;
	string buffer;
	vector <string> tokens;

	// Open the file
	file.open(p_filename.c_str());

	if (!file)
	{
		cout << "File not found " << p_filename << endl;
		this->slot = 0;
		return false;
	}

	buffer = fread_skip_comments(file);

	// Tokenize the buffer on colons
	tokens.clear();
	if(tokenize_colon(buffer, tokens))
	{
		int itmp;
		stringstream sstr(tokens[1]);
		sstr >> itmp;
		this->slot = itmp;
	}

	return true;
}


//---------------------------------------------------------------------
///   Return all objects
///   \return vector of all objects
//---------------------------------------------------------------------
vector<c_object*> c_object_manager::get_objects()
{
	return this->objects;
}


//---------------------------------------------------------------------
///   Return all spawns
///   \return vector of all spawns
//---------------------------------------------------------------------
vector<c_spawn> c_object_manager::get_spawns()
{
	return this->spawns;
}


//---------------------------------------------------------------------
///   Clear all objects
//---------------------------------------------------------------------
void c_object_manager::clear_objects()
{
	unsigned int cnt;

	for (cnt = 0; cnt < this->objects.size(); cnt++)
	{
		glDeleteTextures(1, this->objects[cnt]->get_icon_array());
	}
	this->objects.clear();
}
