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
/// @brief mesh memory definition
/// @details Definition of the mesh memory

#include <string>
#include <vector>

using namespace std;


#ifndef meesh_memory_h
#define meesh_memory_h
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

		c_tile_definition() {}
		~c_tile_definition() {}
};


//---------------------------------------------------------------------
///   The tile dictionary, compiling all tile definitions
//---------------------------------------------------------------------
class c_tile_dictionary
{
	protected:
		vect2 TxBox[MAXTILETYPE]; ///< The texture box
		vector<c_tile_definition> tile_definitions; ///< vector with all tile definitions

	public:
		bool load_tiledict(string);
};



//---------------------------------------------------------------------
///   the aa box
///   \todo find out what the aa_bbox actually is
//---------------------------------------------------------------------
class c_aa_box
{
	public:
		int    sub_used;
		float  weight;

		bool   used;
		int    level;
		int    address;

		vect3  mins;
		vect3  maxs;

		c_aa_box();
		~c_aa_box();
};


//---------------------------------------------------------------------
///   Definition of one single tile
//---------------------------------------------------------------------
class c_tile
{
	protected:
		bool wall;                     ///< Is it a wall?

	public:
		int vert_count;                ///< Number of vertices
		int pos_x;                     ///< The X position of the tile
		int pos_y;                     ///< The Y position of the tile
		vector<vect3> vertices;        ///< x, y, z values
		vector<vect3> vert_base_light; ///< r, g, b values
		vector<vect3> vert_light;      ///< r, g, b values

		Uint8    type;         ///< Command type
		Uint8    fx;           ///< Special effects flags
		Uint8    twist;        //
		bool     inrenderlist; ///< Is it in the renderlist?
		Uint16   tile;         ///< Get texture from this
		Uint32   vrt_start;    ///< Which vertex to start at
		c_aa_box bbox;         ///< what is the minimum extent of the fan

		c_tile();
		~c_tile();

		bool get_wall();
		void set_wall(bool);

		vect3* get_vertex(int);
};


//---------------------------------------------------------------------
///   mesh memory - this stores the vertex data
//---------------------------------------------------------------------
class c_mesh_mem : public c_tile_dictionary
{
	protected:
		bool alloc_verts(int);
		bool alloc_fans(int);

		void dealloc_verts();
		void dealloc_fans();

		void setup_mesh_mem(int, int);
		void destroy_mesh_mem();

	public:
		int     vrt_count;    ///< Number of vertices in the mesh mem

		vector<c_tile> tiles; ///< Vector holding all tiles

		int     tiles_x;      ///< Mesh X size
		int     tiles_y;      ///< Mesh Y size
		Uint32 tile_count;    ///< Number of tiles

		vector<unsigned int> vert_lookup;          ///< Vertex lookup table
		vector<vector<unsigned int> > tile_lookup; ///< Tile loopup table

		void build_vert_lookup_table();
		void build_tile_lookup_table();

		void change_tile_type(int, int);

		c_mesh_mem();
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
#endif
