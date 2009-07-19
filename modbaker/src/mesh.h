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
//-   Mesh FX bits
//---------------------------------------------------------------------
enum e_mpd_fx
{
	MESHFX_REF     =       0,  // 0 This tile is drawn 1st
	MESHFX_SHA     = (1 << 0), // 0 This tile is drawn 2nd
	MESHFX_DRAWREF = (1 << 1), // 1 Draw reflection of characters
	MESHFX_ANIM    = (1 << 2), // 2 Animated tile ( 4 frame )
	MESHFX_WATER   = (1 << 3), // 3 Render water above surface ( Water details are set per module )
	MESHFX_WALL    = (1 << 4), // 4 Wall ( Passable by ghosts, particles )
	MESHFX_IMPASS  = (1 << 5), // 5 Impassable
	MESHFX_DAMAGE  = (1 << 6), // 6 Damage
	MESHFX_SLIPPY  = (1 << 7)  // 7 Ice or normal
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
///   Definition of one spawn (= one entry in spawn.txt)
//---------------------------------------------------------------------
class c_spawn
{
	public:
		int id;               ///< ID for internal handling
		string name;          ///< Display name
		int slot_number;      ///< Egoboo slot number
		vect3 pos;            ///< Position
		char direction;       ///< Direction
		int money;            ///< Bonus money for this module
		int skin;             ///< The skin
		int passage;          ///< Reference passage
		int content;          ///< Content (for armor chests)
		int lvl;              ///< Character level
		char status_bar;      ///< Draw a status bar for this character?
		char ghost;           ///< Is the character a ghost?
		string team;          ///< Team
};


//---------------------------------------------------------------------
///   Definition of one object in the module/objects/ folder
//---------------------------------------------------------------------
class c_object
{
	private:
		string name;    // the name, like object.obj
		bool gor;       // Is it in the Global Object Repository?
		int slot;       // The slot number
		GLuint icon[1]; // The icon texture

	public:
		c_object();
		~c_object();

		string get_name()              { return this->name; }
		void   set_name(string p_name) { this->name = p_name; }

		bool   get_gor()           { return this->gor; }
		void   set_gor(bool p_gor) { this->gor = p_gor; }

		int    get_slot()           { return this->slot; }
		void   set_slot(int p_slot) { this->slot = p_slot; }

		GLuint get_icon()            { return this->icon[0]; }
		GLuint *get_icon_array()     { return this->icon; }
		void set_icon(GLuint p_icon) { this->icon[0] = p_icon; }

		bool read_data_file(string);
		bool render();
		void debug_data();
};


//---------------------------------------------------------------------
///   Object and spawn manager
//---------------------------------------------------------------------
class c_object_manager
{
	private:
		vector<c_spawn>  spawns;
		vector<c_object*> objects;

	public:
		c_object_manager();
		~c_object_manager();

		void clear_objects()
		{
			this->objects.clear();
		}
		void clear_spawns()
		{
			this->spawns.clear();
		}

		void load_spawns(string);
		bool save_spawns(string);

		void load_objects(string, bool);
		bool save_objects(string);

		void add_spawn(c_spawn);
		void remove_spawn(int);

		unsigned int get_spawns_size();
		c_spawn get_spawn(int);
		void    set_spawn(int, c_spawn);

		unsigned int get_objects_size();
		c_object* get_object(int);
		c_object* get_object_by_slot(int);
		void      set_object(int, c_object*);

		vector<c_spawn>  get_spawns();
		vector<c_object*> get_objects();

		void load_all_for_module(string);

		int get_nearest_object(float, float, float);

		void    render();
};


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
		bool load(string);
		bool save(string);

		string get_name()            { return this->name; }
		void set_name(string p_name) { this->name = p_name; }

		string get_reference_module()                        { return this->reference_module; }
		void set_reference_module(string p_reference_module) { this->reference_module = p_reference_module; }

		string get_required_idsz()                     { return this->required_idsz; }
		void set_required_idsz(string p_required_idsz) { this->required_idsz = p_required_idsz; }

		unsigned int get_number_of_imports()                         { return this->number_of_imports; }
		void set_number_of_imports(unsigned int p_number_of_imports) { this->number_of_imports = p_number_of_imports; }

		bool get_allow_export()                    { return this->allow_export; }
		void set_allow_export(bool p_allow_export) { this->allow_export = p_allow_export; }

		unsigned int get_min_players()                   { return this->min_players; }
		void set_min_players(unsigned int p_min_players) { this->min_players = min_players; }

		unsigned int get_max_players()                   { return this->max_players; }
		void set_max_players(unsigned int p_max_players) { this->max_players = max_players; }

		bool get_allow_respawn()                     { return this->allow_respawn; }
		void set_allow_respawn(bool p_allow_respawn) { this->allow_respawn = p_allow_respawn; }

		bool get_not_used()                { return this->not_used; }
		void set_not_used(bool p_not_used) { this->not_used = p_not_used; }

		string get_rating()              { return this->rating; }
		void set_rating(string p_rating) { this->rating = p_rating; }

		vector<string> get_summary()               { return this->summary; }
		void set_summary(vector<string> p_summary) { this->summary = p_summary; }
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
///   TODO: Make this a class
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
///   The mesh info
//---------------------------------------------------------------------
class c_mesh_info
{
	private:
	public:
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
///   \struct TODO: What is this?
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

		Uint32 tile_count;

		// Vertex lookup table
		vector<unsigned int> vert_lookup;

		void build_tile_lookup_table();
		void change_tile_type(int, int);

		c_mesh_mem(int, int);
		~c_mesh_mem();
		vect3 get_vert(int);

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
//---------------------------------------------------------------------
class c_mesh
{
	private:
		void mesh_make_fanstart();
		void init_meshfx();

		vector<int> m_meshfx;

		c_menu_txt  *m_menu_txt;

		vect2 new_size;

	protected:
		// TODO: The tile dictionary doesn't really belong to the mesh
		//       It's more a general Egoboo thing
		static c_tile_dictionary ms_tiledict;

	public:
		// TODO: Make private
		c_mesh_info  *mi;
		c_mesh_mem   *mem;

		bool change_mem_size(int, int);

		void  set_size(vect2 p_size) { this->new_size = p_size; }
		vect2 get_size()             { return this->new_size; }

		c_mesh();
		~c_mesh();
		bool new_mesh_mpd(int, int);
		bool save_mesh_mpd(string);
		bool load_mesh_mpd(string);

		void load_menu_txt(string);
		c_menu_txt* get_menu_txt()                { return this->m_menu_txt; }
		void set_menu_txt(c_menu_txt* p_menu_txt) { this->m_menu_txt = p_menu_txt; }

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


//---------------------------------------------------------------------
///   A passage (=one entry in passage.txt)
//---------------------------------------------------------------------
// TODO: Does it really make sense to make those private?
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
		unsigned int get_id()                  { return this->id; }
		void         set_id(unsigned int p_id) { this->id = p_id; }

		string get_name()              { return this->name; }
		void   set_name(string p_name) { this->name = p_name; }

		vect2 get_pos_top()                { return this->pos_top; }
		void  set_pos_top(vect2 p_pos_top) { this->pos_top = p_pos_top; }

		vect2 get_pos_bot()                { return this->pos_bot; }
		void  set_pos_bot(vect2 p_pos_bot) { this->pos_bot = p_pos_bot; }

		bool get_open()            { return this->open; }
		void set_open(bool p_open) { this->open = p_open; }

		bool get_shoot_through()                     { return this->shoot_through; }
		void set_shoot_through(bool p_shoot_through) { this->shoot_through = p_shoot_through; }

		bool get_slippy_close()                    { return this->slippy_close; }
		void set_slippy_close(bool p_slippy_close) { this->slippy_close = p_slippy_close; }

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

		c_passage get_passage(int p_num)                      { return this->passages[p_num]; }
		void      set_passage(c_passage p_passage, int p_num) { this->passages[p_num] = p_passage; };

		bool load_passage_txt(string);
		bool save_passage_txt(string);
		void clear_passages();
		void add_passage(c_passage);
		void remove_passage_by_id(unsigned int);
		int get_passages_size();
};
#endif
