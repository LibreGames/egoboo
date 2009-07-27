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
/// @brief module menu definition
/// @details Definition of the module menu

#ifndef modmenu_h
#define modmenu_h
//---------------------------------------------------------------------
//-
//-   Everything related to spawns and objects
//-
//---------------------------------------------------------------------
#include "../general.h"

#include <string>
#include <vector>

using namespace std;


//---------------------------------------------------------------------
///   Definition of the module menu file (menu.txt)
//---------------------------------------------------------------------
class c_menu_txt
{
	private:
		string name;
		string reference_module;
		string required_idsz;
		unsigned int number_of_imports;
		bool allow_export;
		unsigned int min_players;
		unsigned int max_players;
		bool allow_respawn;
		bool not_used;
		string rating;
		vector<string> summary;

	public:
		c_menu_txt();
//		~c_menu_txt();
		bool load_menu_txt(string);
		bool save_menu_txt(string);

		void set_menu_txt(c_menu_txt*);
		c_menu_txt* get_menu_txt();

		///   Get the module name
		///   \return module name
		string get_name()            { return this->name; }
		///   Set the module name
		///   \param p_name new module name
		void set_name(string p_name) { this->name = p_name; }

		///   Get the reference module
		///   \return reference module
		string get_reference_module()                        { return this->reference_module; }
		///   Set the reference module
		///   \param p_reference_module new reference module
		void set_reference_module(string p_reference_module) { this->reference_module = p_reference_module; }

		///   Get the required IDSZ
		///   \return required IDSZ
		string get_required_idsz()                     { return this->required_idsz; }
		///   Set the required IDSZ
		///   \param p_required_idsz new required IDSZ
		void set_required_idsz(string p_required_idsz) { this->required_idsz = p_required_idsz; }

		///   Get the number of imports
		///   \return number of imports
		unsigned int get_number_of_imports()                         { return this->number_of_imports; }
		///   Set the number of imports
		///   \param p_number_of_imports new number of imports
		void set_number_of_imports(unsigned int p_number_of_imports) { this->number_of_imports = p_number_of_imports; }

		///   Get the "allow export" flag
		///   \return "allow export" flag
		bool get_allow_export()                    { return this->allow_export; }
		///   Set the "allow export" flag
		///   \param p_allow_export new "allow export" flag
		void set_allow_export(bool p_allow_export) { this->allow_export = p_allow_export; }

		///   Get the min. player number
		///   \return min. player number
		unsigned int get_min_players()                   { return this->min_players; }
		///   Set the min. player number
		///   \param p_min_players new min. player number
		void set_min_players(unsigned int p_min_players) { this->min_players = p_min_players; }

		///   Get the max. player number
		///   \return max. player number
		unsigned int get_max_players()                   { return this->max_players; }
		///   Set the max. player number
		///   \param p_max_players new max. player number
		void set_max_players(unsigned int p_max_players) { this->max_players = p_max_players; }

		///   Get the "allow respawn" flag
		///   \return "allow respawn" flag
		bool get_allow_respawn()                     { return this->allow_respawn; }
		///   Set the "allow respawn" flag
		///   \param p_allow_respawn new "allow respawn" flag
		void set_allow_respawn(bool p_allow_respawn) { this->allow_respawn = p_allow_respawn; }

		///   Get the "not used" flag
		///   \return "not used" flag
		bool get_not_used()                { return this->not_used; }
		///   Set the "not used" flag
		///   \param p_not_used new "not used" flag
		void set_not_used(bool p_not_used) { this->not_used = p_not_used; }

		///   Get the module rating
		///   \return module rating
		string get_rating()              { return this->rating; }
		///   Set the module rating
		///   \param p_rating new module rating
		void set_rating(string p_rating) { this->rating = p_rating; }

		///   Get the module summary
		///   \return module summary
		vector<string> get_summary()               { return this->summary; }
		///   Set the module summary
		///   \param p_summary new module summary
		void set_summary(vector<string> p_summary) { this->summary = p_summary; }
};
#endif
