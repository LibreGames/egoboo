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
/// @brief Egoboo passage implementation
/// @details Implementation of Egoboo passages

//---------------------------------------------------------------------
//-
//-   Everything related to Egoboo passages
//-
//---------------------------------------------------------------------

#include "../general.h"
#include "passage.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;


//---------------------------------------------------------------------
///   Load the passage.txt file
///   \param p_filename path + filename to the passage.txt file
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_passage_manager::load_passage_txt(string p_filename)
{
	ifstream file;

	// Open the file
	file.open(p_filename.c_str());

	if (!file)
	{
		cout << "There was an error opening the following file:" << endl;
		cout << p_filename << endl;
		return false;
	}

	int iTmp;
	string sTmp;
	c_passage tmp_passage;
	vect2 tmp_vec2;

	while (!file.eof())
	{
		// ID & name
		file >> iTmp; tmp_passage.set_id(iTmp);
		file >> sTmp; tmp_passage.set_name(sTmp);

		// Top position
		file >> iTmp; tmp_vec2.x = iTmp;
		file >> iTmp; tmp_vec2.y = iTmp;

		tmp_passage.set_pos_top(tmp_vec2);

		// Bottom position
		file >> iTmp; tmp_vec2.x = iTmp;
		file >> iTmp; tmp_vec2.y = iTmp;

		tmp_passage.set_pos_bot(tmp_vec2);

		// Flags
		file >> sTmp;                       tmp_passage.set_open(false);
		if ((sTmp == "T") || (sTmp == "t")) tmp_passage.set_open(true);

		file >> sTmp;                       tmp_passage.set_shoot_through(false);
		if ((sTmp == "T") || (sTmp == "t")) tmp_passage.set_shoot_through(true);

		file >> sTmp;                       tmp_passage.set_slippy_close(false);
		if ((sTmp == "T") || (sTmp == "t")) tmp_passage.set_slippy_close(true);
	}

	file.close();

	return true;
}


//---------------------------------------------------------------------
///   Save the passage.txt file
///   \param p_filename path + filename to the passage.txt file
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_passage_manager::save_passage_txt(string p_filename)
{
	ifstream file;
	// Open the file
	file.open(p_filename.c_str());

	if (!file)
	{
		cout << "There was an error opening the following file:" << endl;
		cout << p_filename << endl;
		return false;
	}

	file.close();

	return true;
}


//---------------------------------------------------------------------
///   Clear all passages
//---------------------------------------------------------------------
void c_passage_manager::clear_passages()
{
	this->passages.clear();
}


//---------------------------------------------------------------------
///   Add a passage
///   \param p_passage passage to add
//---------------------------------------------------------------------
void c_passage_manager::add_passage(c_passage p_passage)
{
	this->passages.push_back(p_passage);
}


//---------------------------------------------------------------------
///   Remove a passage by the passage id
///   \param p_id passage ID to remove
//---------------------------------------------------------------------
void c_passage_manager::remove_passage_by_id(unsigned int p_id)
{
	unsigned int cnt;

	for (cnt = 0; cnt < this->passages.size(); cnt++)
	{
		if (this->passages[cnt].get_id() == p_id)
			this->passages.erase(this->passages.begin()+cnt);
	}
}


//---------------------------------------------------------------------
///   Get the number of passages
///   \return number of passages
//---------------------------------------------------------------------
int c_passage_manager::get_passages_size()
{
	return this->passages.size();
}


//---------------------------------------------------------------------
///   c_passage_manager constructor
//---------------------------------------------------------------------
c_passage_manager::c_passage_manager()
{
	this->passages.clear();
}


//---------------------------------------------------------------------
///   c_passage_manager destructor
//---------------------------------------------------------------------
c_passage_manager::~c_passage_manager()
{
	this->passages.clear();
}
