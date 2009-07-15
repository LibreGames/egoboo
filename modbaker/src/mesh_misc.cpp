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

#include <SDL.h>
#include <SDL_types.h>
#include <SDL_endian.h>

#include "general.h"
#include "global.h"
#include "mesh.h"
#include "utility.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

c_object_manager::c_object_manager()
{
	cout << "Creating the spawn manager" << endl;
}


c_object_manager::~c_object_manager()
{
	cout << "Deleting the spawn manager" << endl;
}


void c_object_manager::load_spawns(string p_modname)
{
	ifstream f_spawn;
	c_spawn temp_spawn;

	string filename;
	filename = g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/spawn.txt";

	f_spawn.open(filename.c_str());

	if (!f_spawn)
	{
		cout << "spawn.txt not found!" << endl;
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


bool c_object_manager::save_spawns(string p_modname)
{
	return false;
}


void c_object_manager::add_spawn(c_spawn)
{

}


void c_object_manager::remove_spawn(int p_slot_number)
{
	this->spawns.erase(this->spawns.begin()+p_slot_number);
}


unsigned int c_object_manager::get_spawns_size()
{
	return this->spawns.size();
}


c_spawn c_object_manager::get_spawn(int p_slot_number)
{
	return this->spawns[p_slot_number];
}


void c_object_manager::set_spawn(int p_slot_number, c_spawn p_spawn)
{
	this->spawns[p_slot_number] = p_spawn;
}



unsigned int c_object_manager::get_objects_size()
{
	return this->objects.size();
}


c_object *c_object_manager::get_object(int p_slot_number)
{
	return this->objects[p_slot_number];
}


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


void c_object_manager::set_object(int p_slot_number, c_object *p_object)
{
	this->objects[p_slot_number] = p_object;
}


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
		temp_object->set_slot(0);     // TODO: Read slot from data.txt
		temp_object->set_icon(0);

		data_file = p_dirname + temp_object->get_name() + "/data.txt";
		temp_object->read_data_file(data_file);

		if (g_renderer == NULL)
		{
			cout << "WARNING: No renderer!" << endl;
		}

		if (!g_renderer->load_icon(p_dirname + objnames[j] + "/icon0.bmp", temp_object))
		{
			// icon0.bmp not found, so try to load the tris0.bmp instead
			if(!g_renderer->load_icon(p_dirname + objnames[j] + "/tris0.bmp", temp_object))
			{
				temp_object->set_icon(0);
			}
		}

		this->objects.push_back(temp_object);
	}
	cout << "Imported " << this->get_objects_size() << " objects" << endl;
}


bool c_object_manager::save_objects(string p_modname)
{
	return false;
}

c_object::c_object()
{
	this->name    = "";
	this->icon[0] = (GLuint)0;
}

c_object::~c_object()
{
}


//---------------------------------------------------------------------
//-   Read file data.txt file to get the slot number
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
//-   Return all objects
//---------------------------------------------------------------------
vector<c_object*> c_object_manager::get_objects()
{
	return this->objects;
}


//---------------------------------------------------------------------
//-   Return all spawns
//---------------------------------------------------------------------
vector<c_spawn> c_object_manager::get_spawns()
{
	return this->spawns;
}


//---------------------------------------------------------------------
//-   Constructor for c_menu_txt
//---------------------------------------------------------------------
c_menu_txt::c_menu_txt()
{
}

//---------------------------------------------------------------------
//-   Load the menu.txt file
//---------------------------------------------------------------------
bool c_menu_txt::load(string p_filename)
{
	ifstream file;
	vector <string> tokens;

	// Open the file
	file.open(p_filename.c_str());

	if (!file)
	{
		cout << "File not found " << p_filename << endl;
		return false;
	}

	string sTmp;
	int    iTmp;
	vector<string> tmp_summary;

	fgoto_colon_yesno(file);
	file >> sTmp;  this->name              = sTmp;
	fgoto_colon_yesno(file);
	file >> sTmp;  this->reference_module  = sTmp;
	fgoto_colon_yesno(file);
	file >> sTmp;  this->required_idsz     = sTmp;
	fgoto_colon_yesno(file);
	file >> iTmp;  this->number_of_imports = iTmp;
	fgoto_colon_yesno(file);
	file >> sTmp;  this->allow_export      = translate_into_bool(sTmp);
	fgoto_colon_yesno(file);
	file >> iTmp;  this->min_players       = iTmp;
	fgoto_colon_yesno(file);
	file >> iTmp;  this->max_players       = iTmp;
	fgoto_colon_yesno(file);
	file >> sTmp;  this->allow_respawn     = translate_into_bool(sTmp);
	fgoto_colon_yesno(file);
	file >> sTmp;  this->not_used          = translate_into_bool(sTmp);
	fgoto_colon_yesno(file);
	file >> sTmp;  this->rating            = sTmp;
	fgoto_colon_yesno(file);

	// Read the summary
	file >> sTmp;  tmp_summary.push_back(sTmp);
	fgoto_colon_yesno(file);
	file >> sTmp;  tmp_summary.push_back(sTmp);
	fgoto_colon_yesno(file);
	file >> sTmp;  tmp_summary.push_back(sTmp);
	fgoto_colon_yesno(file);
	file >> sTmp;  tmp_summary.push_back(sTmp);
	fgoto_colon_yesno(file);
	file >> sTmp;  tmp_summary.push_back(sTmp);
	fgoto_colon_yesno(file);
	file >> sTmp;  tmp_summary.push_back(sTmp);
	fgoto_colon_yesno(file);
	file >> sTmp;  tmp_summary.push_back(sTmp);
	fgoto_colon_yesno(file);
	file >> sTmp;  tmp_summary.push_back(sTmp);
	this->summary = tmp_summary;

	file.close();

	return true;
}


//---------------------------------------------------------------------
//-   Save the menu.txt file
//---------------------------------------------------------------------
bool c_menu_txt::save(string p_filename)
{
	ofstream file;
	string buffer;
	vector <string> tokens;

	// Open the file
	file.open(p_filename.c_str());

	if (!file)
	{
		cout << "There was an error opening the file" << endl;
		return false;
	}

	vector<string> tmp_summary;

	buffer = "// This file contains info for the module's menu description";
	file << buffer << endl;

	buffer = "Name ( with underscores for spaces )		: " + this->name;
	file << buffer << endl;

	buffer = "Reference module ( Directory name or NONE )     	: " + this->reference_module;
	file << buffer << endl;

	buffer = "Required reference IDSZ ( or [NONE] )           	: " + this->required_idsz;
	file << buffer << endl;

	buffer = "Number of imports ( 0 to 4 )                    		: " + translate_into_string(this->number_of_imports);
	file << buffer << endl;

	buffer = "Allow exporting of characters ( TRUE or FALSE ) 	: " + translate_into_egobool(this->allow_export);
	file << buffer << endl;

	buffer = "Minimum number of players ( 1 to 4 )		: " + translate_into_string(this->min_players);
	file << buffer << endl;

	buffer = "Maximum number of players ( 1 to 4 )		: " + translate_into_string(this->max_players);
	file << buffer << endl;

	buffer = "Allow respawning ( TRUE or FALSE )		: " + translate_into_egobool(this->allow_respawn);
	file << buffer << endl;

	buffer = "NOT USED				: " + translate_into_egobool(this->not_used);
	file << buffer << endl;

	buffer = "Level rating ( *, **, ***, ****, or ***** )    		 : " + this->rating;
	file << buffer << endl;

	buffer = "";
	file << buffer << endl;

	buffer = "";
	file << buffer << endl;

	buffer = "// Module summary ( Must be 8 lines...  Each line mush have at least an _ )";
	file << buffer << endl;

	buffer = ":" + this->summary[0];
	file << buffer << endl;

	buffer = ":" + this->summary[1];
	file << buffer << endl;

	buffer = ":" + this->summary[2];
	file << buffer << endl;

	buffer = ":" + this->summary[3];
	file << buffer << endl;

	buffer = ":" + this->summary[4];
	file << buffer << endl;

	buffer = ":" + this->summary[5];
	file << buffer << endl;

	buffer = ":" + this->summary[6];
	file << buffer << endl;

	buffer = ":" + this->summary[7];
	file << buffer << endl;

	buffer = "";
	file << buffer << endl;

	buffer = "";
	file << buffer << endl;

	buffer = "// Module expansion IDSZs ( with a colon in front )";
	file << buffer << endl;

	buffer = "";
	file << buffer << endl;

	file.close();

	return true;
}


//---------------------------------------------------------------------
//-   Load the menu.txt file
//---------------------------------------------------------------------
void c_mesh::load_menu_txt(string p_modname)
{
	this->m_menu_txt = new c_menu_txt();
	this->m_menu_txt->load(g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/menu.txt");
}
