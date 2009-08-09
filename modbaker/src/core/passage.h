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
/// @brief Egoboo passage definition
/// @details Definition of Egoboo passages

#ifndef passage_h
#define passage_h

//---------------------------------------------------------------------
//-
//-   Everything related to Egoboo passages
//-
//---------------------------------------------------------------------
#include <string>
#include <vector>

using namespace std;


//---------------------------------------------------------------------
///   A passage (=one entry in passage.txt)
///  \todo does it really make sense to make everything private here
//---------------------------------------------------------------------
class c_passage
{
	private:
		unsigned int id;
		string name;
		vect2 pos_top;
		vect2 pos_bot;
		bool open;
		bool shoot_through;
		bool slippy_close;

	public:
		///   Get the ID
		///   \return ID
		unsigned int get_id()                  { return this->id; }
		///   Set the ID
		///   \param p_id new ID
		void         set_id(unsigned int p_id) { this->id = p_id; }

		///   Get the passage name
		///   \return passage name
		string get_name()              { return this->name; }
		///   Set the passage name
		///   \param p_name new passage name
		void   set_name(string p_name) { this->name = p_name; }

		///   Get the top position
		///   \return top position
		vect2 get_pos_top()                { return this->pos_top; }
		///   Set the top position
		///   \param p_pos_top new top position
		void  set_pos_top(vect2 p_pos_top) { this->pos_top = p_pos_top; }

		///   Get the bottom position
		///   \return bottom position
		vect2 get_pos_bot()                { return this->pos_bot; }
		///   Set the bottom position
		///   \param p_pos_bot new bottom position
		void  set_pos_bot(vect2 p_pos_bot) { this->pos_bot = p_pos_bot; }

		///   Get the "open" flag
		///   \return "open" flag
		bool get_open()            { return this->open; }
		///   Set the "open" flag
		///   \param p_open new "open" flag
		void set_open(bool p_open) { this->open = p_open; }

		///   Get the "shoot through" flag
		///   \return "shoot through" flag
		bool get_shoot_through()                     { return this->shoot_through; }
		///   Set the "shoot through" flag
		///   \param p_shoot_through new "shoot through" flag
		void set_shoot_through(bool p_shoot_through) { this->shoot_through = p_shoot_through; }

		///   Get the "slippy close" flag
		///   \return "slippy close" flag
		bool get_slippy_close()                    { return this->slippy_close; }
		///   Set the "slippy close" flag
		///   \param p_slippy_close new "slippy close" flag
		void set_slippy_close(bool p_slippy_close) { this->slippy_close = p_slippy_close; }

		///   c_passage constructor
		c_passage()
		{
			this->id            = 0;
			this->name          = "";
			this->pos_top.x     = 0;
			this->pos_top.y     = 0;
			this->pos_bot.x     = 0;
			this->pos_bot.y     = 0;
			this->open          = false;
			this->shoot_through = false;
			this->slippy_close  = false;
		}
		~c_passage()
		{
		}
};


//---------------------------------------------------------------------
///   The passage manager
//---------------------------------------------------------------------
class c_passage_manager
{
	private:
		vector<c_passage> passages;

	public:
		c_passage_manager();
		~c_passage_manager();

		///   Get a passage
		///   /param p_num passage number
		///   \return passage object
		c_passage get_passage(int p_num) {
			return this->passages[p_num];
		}
		///   Set a passage
		///   /param p_passage passage content
		///   /param p_num passage number
		void      set_passage(c_passage p_passage, int p_num) {
			this->passages[p_num] = p_passage;
		}

		bool load_passage_txt(string);
		bool save_passage_txt(string);
		void clear_passages();
		void add_passage(c_passage);
		void remove_passage_by_id(unsigned int);
		int get_passages_size();
};
#endif
