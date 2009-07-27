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

#include "general.h"

#include "SDL_extensions.h"
#include "ogl_extensions.h"

#include <fstream>
#include <string>
#include <vector>

using namespace std;

//---------------------------------------------------------------------
//-   mesh.h - definitions for mesh handling
//---------------------------------------------------------------------
#define MAXMESHFAN          (512*512) ///< Terrain mesh size
#define MAXMESHCOMMAND           4    ///< Draw up to 4 fans
#define MAXMESHSIZEY          1024    ///< Max fans in y direction
#define MAXMESHTYPE             64    ///< Number of fansquare command types
#define MAXMESHCOMMANDENTRIES   32    ///< Fansquare command list size
#define MAXMESHVERTICES         16    ///< Fansquare vertices
#define BYTESFOREACHVERTEX      (3*sizeof(float) + 3*sizeof(Uint8) + 3*sizeof(float))

#define MAXTILETYPE          (64*4)   ///< Max number of tile images
#define TX_FUDGE              0.5f


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


//---------------------------------------------------------------------
///   Texture box?
//---------------------------------------------------------------------
typedef vect2 TILE_TXBOX;


//---------------------------------------------------------------------
///   Definition of a single tile definition
//---------------------------------------------------------------------
class c_tile_definition
{
	private:
	public:
		Uint8   cmd_count;                  ///< Number of commands
		Uint8   cmd_size[MAXMESHCOMMAND];   ///< Entries in each command
		Uint16  vrt[MAXMESHCOMMANDENTRIES]; ///< Fansquare vertex list

		Uint8   vrt_count;                  ///< Number of vertices
		Uint8   ref[MAXMESHVERTICES];       ///< Lighting references
		vect2   tx[MAXMESHVERTICES];        ///< Relative texture coordinates

//		c_tile_definition();
//		~c_tile_definition()
};


//---------------------------------------------------------------------
///   The tile dictionary, compiling all tile definitions
///   \todo make the tile dictionary to a class
//---------------------------------------------------------------------
struct c_tile_dictionary : public vector<c_tile_definition>
{
	typedef vector<c_tile_definition> my_vector; ///< vector with all tile definitions

	TILE_TXBOX TxBox[MAXTILETYPE]; ///< The texture box

	/// Get the vector data
	/// \return vector
	my_vector & getData() { return *(static_cast<my_vector*>(this)); }

	bool load();
};


//---------------------------------------------------------------------
///   \todo find out what the aa_bbox is
//---------------------------------------------------------------------
struct s_aa_bbox
{
	int    sub_used;
	float  weight;

	bool   used;
	int    level;
	int    address;

	vect3  mins;
	vect3  maxs;
};
typedef struct s_aa_bbox AA_BBOX;


//---------------------------------------------------------------------
///   Definition of one single tile
//---------------------------------------------------------------------
class c_tile
{
	private:
		bool wall;                     ///< Is it a wall?
	public:
		int vert_count;                ///< Number of vertices
		int pos_x;                     ///< The X position of the tile
		int pos_y;                     ///< The Y position of the tile
		vector<vect3> vertices;        ///< x, y, z values
		vector<vect3> vert_base_light; ///< r, g, b values
		vector<vect3> vert_light;      ///< r, g, b values

		Uint8   type;         ///< Command type
		Uint8   fx;           ///< Special effects flags
		Uint8   twist;        //
		bool    inrenderlist; ///< Is it in the renderlist?
		Uint16  tile;         ///< Get texture from this
		Uint32  vrt_start;    ///< Which vertex to start at
		AA_BBOX bbox;         ///< what is the minimum extent of the fan

		c_tile();
		~c_tile();

		/// Is it a wall?
		/// \return true if tile is a wall
		bool get_wall() { return this->wall; }

		/// Set it to wall or floor
		/// \param p_wall true for wall, false for floor
		void set_wall(bool p_wall) { this->wall = p_wall; }

		vect3* get_vertex(int);
};


//---------------------------------------------------------------------
///   mesh memory - this stores the vertex data
//---------------------------------------------------------------------
class c_mesh_mem
{
	private:
		bool alloc_verts(int);
		bool alloc_fans(int);

		void dealloc_verts();
		void dealloc_fans();

	public:

		int     vrt_count;

		vector<c_tile> tiles;

		int     tiles_x;                    // Mesh size in tiles
		int     tiles_y;
		Uint32 tile_count;

		// Vertex lookup table
		vector<unsigned int> vert_lookup;
		vector<vector<unsigned int> > tile_lookup;

		void build_vert_lookup_table();
		void build_tile_lookup_table();

		void change_tile_type(int, int);

		c_mesh_mem(int, int);
		~c_mesh_mem();
		vect3 get_vert(int);
		int get_tile_by_pos(int, int);

		void modify_verts_x(float, int);
		void modify_verts_y(float, int);
		void modify_verts_z(float, int);

		void set_verts_x(float, int);
		void set_verts_y(float, int);
		void set_verts_z(float, int);

		float get_verts_x(int);
		float get_verts_y(int);
		float get_verts_z(int);
};


//---------------------------------------------------------------------
///   Container for all mesh related data
///   \todo the tile dictionary doesn't belong to the mesh. Move this to modbaker instead
///   \todo make the mesh mem private
//---------------------------------------------------------------------
class c_mesh
{
	private:
		void mesh_make_fanstart();
		void init_meshfx();

		vector<int> m_meshfx;

		vect2 new_size;

	protected:
		static c_tile_dictionary ms_tiledict;

	public:
		c_mesh_mem   *mem;

		bool change_mem_size(int, int);

		void  set_size(vect2 p_size) { this->new_size = p_size; }
		vect2 get_size()             { return this->new_size; }

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

		c_tile_dictionary & getTileDictioary()          { return ms_tiledict;             }
		c_tile_definition & getTileDefinition(int tile) { return ms_tiledict[tile];       }
		vect2             & getTileOffset(int tile)     { return ms_tiledict.TxBox[tile]; }
};
#endif
