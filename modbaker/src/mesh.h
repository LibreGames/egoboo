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
#ifndef mesh_h
#define mesh_h
//---------------------------------------------------------------------
//-
//-   Everything related to the .mpd mesh
//-
//---------------------------------------------------------------------

#include "general.h"

#include <fstream>
#include <string>
#include <vector>

using namespace std;

//---------------------------------------------------------------------
//-   mesh.h - definitions for mesh handling
//---------------------------------------------------------------------
#define MAXMESHFAN          (512*512) // Terrain mesh size
#define MAXMESHCOMMAND           4    // Draw up to 4 fans
#define MAXMESHSIZEY          1024    // Max fans in y direction
#define MAXMESHTYPE             64    // Number of fansquare command types
#define MAXMESHCOMMANDENTRIES   32    // Fansquare command list size
#define MAXMESHVERTICES         16    // Fansquare vertices
#define BYTESFOREACHVERTEX      (3*sizeof(float) + 3*sizeof(Uint8) + 3*sizeof(float))

#define MAXTILETYPE          (64*4)   // Max number of tile images
#define TX_FUDGE              0.5f


//---------------------------------------------------------------------
//-   This resembles one entry in spawn.txt
//---------------------------------------------------------------------
class c_spawn
{
	public:
		int id;               // ID for internal handling
		string name;          // Display name
		int slot_number;      // Egoboo slot number
		vect3 pos;            // Position
		char direction;       // Direction
		int money;            // Bonus money for this module
		int skin;             // The skin
		int passage;          // Reference passage
		int content;          // Content (for armor chests)
		int lvl;              // Character level
		char status_bar;      // Draw a status bar for this character?
		char ghost;           // Is the character a ghost?
		string team;          // Team
};


//---------------------------------------------------------------------
//-   spawn.txt handling
//---------------------------------------------------------------------
class c_spawn_manager
{
	private:
		vector<c_spawn> spawns;

	public:
		c_spawn_manager();
		~c_spawn_manager();

		void load(string);
		bool save(string);

		void add(c_spawn);
		void remove(int);

		unsigned int get_spawns_size();
		c_spawn get_spawn(int);
		void    set_spawn(int, c_spawn);
		void    render();
};


//---------------------------------------------------------------------
//-   Texture box?
//---------------------------------------------------------------------
typedef vect2 TILE_TXBOX;

//---------------------------------------------------------------------
//-   the definition of a single tile
//---------------------------------------------------------------------
class c_tile_definition
{
	private:
	public:
		Uint8   cmd_count;                  // Number of commands
		Uint8   cmd_size[MAXMESHCOMMAND];   // Entries in each command
		Uint16  vrt[MAXMESHCOMMANDENTRIES]; // Fansquare vertex list

		Uint8   vrt_count;                  // Number of vertices
		Uint8   ref[MAXMESHVERTICES];       // Lighting references
		vect2   tx[MAXMESHVERTICES];        // Relative texture coordinates

//		c_tile_definition();
//		~c_tile_definition()
};

//---------------------------------------------------------------------
//-   The dictionary, compiling all of the tile definitions
//---------------------------------------------------------------------
struct c_tile_dictionary : public vector<c_tile_definition>
{
	typedef vector<c_tile_definition> my_vector;

	TILE_TXBOX TxBox[MAXTILETYPE];

	my_vector & getData() { return *(static_cast<my_vector*>(this)); }

	bool load();
};

//---------------------------------------------------------------------
//-   The mesh info
//---------------------------------------------------------------------
class c_mesh_info
{
	private:
	public:
		int     vert_count;                 // Total number of vertices

		int     tiles_x;                    // Mesh size in tiles
		int     tiles_y;
		size_t  tile_count;

		int     blocks_x;
		int     blocks_y;
		size_t  block_count;

		float   edge_x;                     // floating point size of mesh
		float   edge_y;                     //

		Uint32  Block_X[(MAXMESHSIZEY >> 2) +1];
		Uint32  Tile_X[MAXMESHSIZEY];       // Which fan to start a row with

//		c_mesh_info();
//		~c_mesh_info();
};


//---------------------------------------------------------------------
//-   TODO: What is this?
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
//-   mesh tile - this stores information about a single tile (fan)
//---------------------------------------------------------------------
struct sMeshTile
{
	Uint8   type;         // Command type
	Uint8   fx;           // Special effects flags
	Uint8   twist;        //
	bool    inrenderlist; //
	Uint16  tile;         // Get texture from this
	Uint32  vrt_start;    // Which vertex to start at
	AA_BBOX bbox;         // what is the minimum extent of the fan
};
typedef struct sMeshTile MeshTile_t;


//---------------------------------------------------------------------
//-   mesh memory - this stores the vertex data
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
		void *  base;             // For malloc

		float*  vrt_x;            // Vertex position
		float*  vrt_y;            //
		float*  vrt_z;            // Vertex elevation

		Uint8*  vrt_ar_fp8;       // Vertex base light
		Uint8*  vrt_ag_fp8;       // Vertex base light
		Uint8*  vrt_ab_fp8;       // Vertex base light

		float*  vrt_lr_fp8;       // Vertex light
		float*  vrt_lg_fp8;       // Vertex light
		float*  vrt_lb_fp8;       // Vertex light

		Uint32        tile_count;
		MeshTile_t  * tilelst;

		c_mesh_mem(int, int);
		~c_mesh_mem();
};


//---------------------------------------------------------------------
//-   container for all mesh related data                             -
//---------------------------------------------------------------------
class c_mesh
{
	private:
		void mesh_make_fanstart();
		c_spawn_manager *spawn_manager;

	protected:
		// TODO: The tile dictionary doesn't really belong to the mesh
		//       It's more a general Egoboo thing
		static c_tile_dictionary ms_tiledict;

	public:
		c_mesh_info  *mi;
		c_mesh_mem   *mem;

		c_spawn_manager* get_spawn_manager();
		void set_spawn_manager(c_spawn_manager*);

		c_mesh();
		~c_mesh();
		bool save_mesh_mpd(string);
		bool load_mesh_mpd(string);

		string modname;

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
