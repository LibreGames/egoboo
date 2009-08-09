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
/// @brief mesh and module definition
/// @details Definition of all files in the module/gamedat/ folder

#ifndef mesh_h
#define mesh_h
//---------------------------------------------------------------------
//-
//-   Everything related to the .mpd mesh
//-
//---------------------------------------------------------------------
#include "../general.h"

#include "../SDL_extensions.h"
#include "../ogl_extensions.h"

#include "mesh_memory.h"

#include <fstream>
#include <string>
#include <vector>

using namespace std;


#define MAPID 0x4470614d


//---------------------------------------------------------------------
//-   Mesh FX bits
//---------------------------------------------------------------------
enum e_mpd_fx
{
	MESHFX_REF     =       0,  ///< 0 This tile is drawn 1st
	MESHFX_SHA     = (1 << 0), ///< 0 This tile is drawn 2nd
	MESHFX_DRAWREF = (1 << 1), ///< 1 Draw reflection of characters
	MESHFX_ANIM    = (1 << 2), ///< 2 Animated tile ( 4 frame )
	MESHFX_WATER   = (1 << 3), ///< 3 Render water above surface ( Water details are set per module )
	MESHFX_WALL    = (1 << 4), ///< 4 Wall ( Passable by ghosts, particles )
	MESHFX_IMPASS  = (1 << 5), ///< 5 Impassable
	MESHFX_DAMAGE  = (1 << 6), ///< 6 Damage
	MESHFX_SLIPPY  = (1 << 7)  ///< 7 Ice or normal
};


//---------------------------------------------------------------------
//-   Fan / tile types
//---------------------------------------------------------------------
enum e_fan_type
{
	TTYPE_TWO_FACED_GROUND1  =  0,
	TTYPE_TWO_FACED_GROUND2  =  1,
	TTYPE_FOUR_FACED_GROUND  =  2,
	TTYPE_EIGHT_FACED_GROUND =  3,
	TTYPE_PILLER_TEN         =  4,
	TTYPE_PILLER_EIGHTEEN    =  5,
	TTYPE_WALL_SIX_WE        =  9,
	TTYPE_WALL_SIX_NS        = 10,
	TTYPE_WALL_EIGHT_W       = 13,
	TTYPE_WALL_EIGHT_N       = 14,
	TTYPE_WALL_EIGHT_E       = 15,
	TTYPE_WALL_EIGHT_S       = 16,
	TTYPE_WALL_TEN_WS        = 17,
	TTYPE_WALL_TEN_NW        = 18,
	TTYPE_WALL_TEN_NE        = 19,
	TTYPE_WALL_TEN_ES        = 20,
	TTYPE_WALL_TWELVE_WSE    = 21,
	TTYPE_WALL_TWELVE_NWS    = 22,
	TTYPE_WALL_TWELVE_ENW    = 23,
	TTYPE_WALL_TWELVE_SEN    = 24,
	TTYPE_STAIR_TWELVE_WE    = 25,
	TTYPE_STAIR_TWELVE_NS    = 26
};


///   \def MAXMESHRENDER
///        Maximum number of tiles to draw
#define MAXMESHRENDER    (1024*8)


//---------------------------------------------------------------------
///   Definition of the renderlist
///   \todo use vectors for this
//---------------------------------------------------------------------
class c_renderlist
{
	private:
	public:
		int     m_num_totl;                 ///< Number to render, total
		Uint32  m_totl[MAXMESHRENDER];      ///< List of which to render, total

		int     m_num_shine;                ///< ..., reflective
		Uint32  m_shine[MAXMESHRENDER];     ///< ..., reflective

		int     m_num_reflc;                ///< ..., has reflection
		Uint32  m_reflc[MAXMESHRENDER];     ///< ..., has reflection

		int     m_num_norm;                 ///< ..., not reflective, has no reflection
		Uint32  m_norm[MAXMESHRENDER];      ///< ..., not reflective, has no reflection

		int     m_num_watr;                 ///< ..., water
		Uint32  m_watr[MAXMESHRENDER];      ///< ..., water
		Uint32  m_watr_mode[MAXMESHRENDER]; ///< the water mode

		virtual bool build_renderlist() = 0;
};


//---------------------------------------------------------------------
///   Container for all mesh related data
///   \todo the tile dictionary doesn't belong to the mesh. Move this to modbaker instead
///   \todo make the mesh mem private
//---------------------------------------------------------------------
class c_mesh : public c_mesh_mem, public c_renderlist
{
	private:
		void mesh_make_fanstart();
		void init_meshfx();

		vector<int> m_meshfx;

		vect2 new_size;

	protected:

	public:
		bool change_mem_size(int, int);

		void  set_mesh_size(vect2 p_size) { this->new_size = p_size; }
		vect2 get_mesh_size()             { return this->new_size; }

		c_mesh();
		~c_mesh();
		bool new_mesh_mpd(int, int);
		bool save_mesh_mpd(string);
		bool load_mesh_mpd(string);

		string modname;

		int get_meshfx(int);

		vector<int> get_fan_vertices(int);

		int modify_verts_x(float, int);
		int modify_verts_y(float, int);
		int modify_verts_z(float, int);

		int get_nearest_vertex(float, float, float);
		int get_nearest_tile(float, float, float);

		bool build_renderlist();
};
#endif
