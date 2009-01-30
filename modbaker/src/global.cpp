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
#include "global.h"

#include <cmath>
#include <sstream>

using namespace std;

c_config        g_config;
c_selection     g_selection;
c_renderer      g_renderer;
c_mesh          g_mesh;
c_frustum       g_frustum;
c_spawn_manager g_spawn_manager;
vector<c_tile_definition> g_tiledict;


//---------------------------------------------------------------------
//-   Variables for mouse handling
//---------------------------------------------------------------------
int g_mouse_x;
int g_mouse_y;

float g_mouse_gl_x;
float g_mouse_gl_y;
float g_mouse_gl_z;


//---------------------------------------------------------------------
//-   Selection
//---------------------------------------------------------------------
int g_nearest_vertex;
extern c_selection g_selection;


//---------------------------------------------------------------------
//-   Calculate the distance between two points
//---------------------------------------------------------------------
float calculate_distance(vect3 start, vect3 end)
{
	float dist;
	dist = sqrtf( powf((start.x - end.x), 2.0f) +  powf((start.y - end.y), 2.0f) + powf((start.z - end.z), 2.0f) );

	return dist;
}


//---------------------------------------------------------------------
//-   Function to handle cleanup
//---------------------------------------------------------------------
void Quit()
{
	SDL_Quit( ); // clean up the window
	exit(-1);    // and exit appropriately
}


//---------------------------------------------------------------------
//-   Constructor: Read the config
//---------------------------------------------------------------------
c_config::c_config()
{
	ifstream file;

	// TODO: Also look in the home directories for a file called
	// ~/.egoboo/modbaker.cfg
	// C:\Documents and settings\egoboo\modbaker.cfg
	file.open("modbaker.cfg");

	if (!file)
	{
		cout << "Config file not found!" << endl;
		Quit();
	}

	string buffer;
	vector <string> tokens;

	while(!file.eof())
	{
		getline(file, buffer);

		tokens.clear();
		if(tokenize_colon(buffer, tokens))
		{
			if (tokens[0] == "egoboo_path")
				this->set_egoboo_path(tokens[1]);

			if (tokens[0] == "font_size")
			{
				// Convert the string to integer using stringstreams
				int itmp;
				stringstream sstr(tokens[1]);
				sstr >> itmp;
				this->set_font_size(itmp);
			}

			if (tokens[0] == "font_file")
				this->set_font_file(tokens[1]);
		}
	}

	file.close();
}


//---------------------------------------------------------------------
//-   Destructor
//---------------------------------------------------------------------
c_config::~c_config() {}


//---------------------------------------------------------------------
//-   Setter and Getter for m_egoboo_path
//---------------------------------------------------------------------
string c_config::get_egoboo_path()
{
	return this->m_egoboo_path;
}

void c_config::set_egoboo_path(string p_egoboo_path)
{
	this->m_egoboo_path = p_egoboo_path;
}


//---------------------------------------------------------------------
//-   Setter and Getter for m_font_size
//---------------------------------------------------------------------
int c_config::get_font_size()
{
	return this->m_font_size;
}

void c_config::set_font_size(int p_font_size)
{
	this->m_font_size = p_font_size;
}


//---------------------------------------------------------------------
//-   Setter and Getter for m_font_file
//---------------------------------------------------------------------
string c_config::get_font_file()
{
	return this->m_font_file;
}

void c_config::set_font_file(string p_font_file)
{
	this->m_font_file = p_font_file;
}


c_spawn_manager::c_spawn_manager()
{
	cout << "Creating the spawn manager" << endl;
}


c_spawn_manager::~c_spawn_manager()
{
	cout << "Deleting the spawn manager" << endl;
}


void c_spawn_manager::load(string p_modname)
{
	ifstream f_spawn;
	c_spawn temp_spawn;

	string filename;
	filename = g_config.get_egoboo_path() + "modules/" + p_modname + "/gamedat/spawn.txt";

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


bool c_spawn_manager::save(string p_modname)
{
	return false;
}


void c_spawn_manager::add(c_spawn)
{

}


void c_spawn_manager::remove(int p_slot_number)
{

}


c_spawn c_spawn_manager::get_spawn(int p_slot_number)
{
	return this->spawns[p_slot_number];
}


void c_spawn_manager::set_spawn(int p_slot_number, c_spawn p_spawn)
{
	this->spawns[p_slot_number] = p_spawn;
}


////         Name      Slot Xpos Ypos Zpos Dir Mon Skn Pas Con Lvl Stt Gho Team
//Player 0:  NONE      0    14.5 60.5 6.0  N   2   0   0   0   0   T   F   Good
