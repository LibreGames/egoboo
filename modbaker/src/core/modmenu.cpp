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
/// @brief Module helper file
/// @details Handling of the files in the module/gamedat/ folder

#include <SDL.h>
#include <SDL_types.h>
#include <SDL_endian.h>

#include "../general.h"
#include "../utility.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

//---------------------------------------------------------------------
///   c_menu_txt constructor
//---------------------------------------------------------------------
c_menu_txt::c_menu_txt()
{
}

//---------------------------------------------------------------------
///   c_menu_txt destructor
//---------------------------------------------------------------------
c_menu_txt::~c_menu_txt()
{
}

//---------------------------------------------------------------------
///   Load the menu.txt file
///   \param p_filename path + filename for the menu.txt file
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_menu_txt::load_menu_txt(string p_filename)
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
///   Save the menu.txt file
///   \param p_filename path + filename to the menu.txt file
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_menu_txt::save_menu_txt(string p_filename)
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
///   Set the content of the menu txt
///   \param p_menu_txt new menu txt
//---------------------------------------------------------------------
void c_menu_txt::set_menu_txt(c_menu_txt* p_menu_txt)
{
	this->set_name(p_menu_txt->get_name());
	this->set_reference_module(p_menu_txt->get_reference_module());
	this->set_required_idsz(p_menu_txt->get_required_idsz());
	this->set_number_of_imports(p_menu_txt->get_number_of_imports());
	this->set_allow_export(p_menu_txt->get_allow_export());
	this->set_min_players(p_menu_txt->get_min_players());
	this->set_max_players(p_menu_txt->get_max_players());
	this->set_allow_respawn(p_menu_txt->get_allow_respawn());
	this->set_not_used(p_menu_txt->get_not_used());
	this->set_rating(p_menu_txt->get_rating());
	this->set_summary(p_menu_txt->get_summary());
}


//---------------------------------------------------------------------
///   Get the content of the menu txt
///   \return menu txt content
//---------------------------------------------------------------------
c_menu_txt* c_menu_txt::get_menu_txt()
{
	c_menu_txt* tmp_menu_txt;

	tmp_menu_txt = new c_menu_txt();

	tmp_menu_txt->set_name(this->get_name());
	tmp_menu_txt->set_reference_module(this->get_reference_module());
	tmp_menu_txt->set_required_idsz(this->get_required_idsz());
	tmp_menu_txt->set_number_of_imports(this->get_number_of_imports());
	tmp_menu_txt->set_allow_export(this->get_allow_export());
	tmp_menu_txt->set_min_players(this->get_min_players());
	tmp_menu_txt->set_max_players(this->get_max_players());
	tmp_menu_txt->set_allow_respawn(this->get_allow_respawn());
	tmp_menu_txt->set_not_used(this->get_not_used());
	tmp_menu_txt->set_rating(this->get_rating());
	tmp_menu_txt->set_summary(this->get_summary());

	return tmp_menu_txt;
}

