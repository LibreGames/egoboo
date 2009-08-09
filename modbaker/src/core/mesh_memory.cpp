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
#include <SDL_types.h>
#include <SDL_endian.h>

#include "../general.h"
#include "../global.h"
#include "../utility.h"
#include "mesh_memory.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;






//---------------------------------------------------------------------
///   c_mesh_mem constructor
//---------------------------------------------------------------------
c_mesh_mem::c_mesh_mem()
{
}


//---------------------------------------------------------------------
///   c_mesh_mem destructor
//---------------------------------------------------------------------
c_mesh_mem::~c_mesh_mem()
{
	this->destroy_mesh_mem();
}


//---------------------------------------------------------------------
///   Setup the mesh memory
///   \param p_vertcount number of vertices
///   \param p_fancount number of fans
//---------------------------------------------------------------------
void c_mesh_mem::setup_mesh_mem(int p_vert_count, int p_fan_count)
{
	if (p_fan_count < 0)
		p_fan_count  = MAXMESHFAN;

	if (p_vert_count >= 0 || p_fan_count >= 0)
	{
		this->alloc_verts(p_vert_count);
		this->alloc_fans (p_fan_count );
	}
}


//---------------------------------------------------------------------
///   Destroy the mesh memory
//---------------------------------------------------------------------
void c_mesh_mem::destroy_mesh_mem()
{
	this->dealloc_verts();
	this->dealloc_fans ();
}


//---------------------------------------------------------------------
///   Deallocate the vertex data
//---------------------------------------------------------------------
void c_mesh_mem::dealloc_verts()
{
	this->vrt_count  = 0;
}


//---------------------------------------------------------------------
///   Allocate the vertex data
///   \param p_vertcount number of vertices
///   \return true on success, else false
///   \todo read the upper vert limit from config
//---------------------------------------------------------------------
bool c_mesh_mem::alloc_verts(int p_vertcount)
{
	if (p_vertcount == 0)
		return false;

	if (p_vertcount < 0)
		p_vertcount = 100;

	if (this->vrt_count > p_vertcount)
		return true;

	this->dealloc_verts();

	this->vrt_count  = p_vertcount;

	return true;
}


//---------------------------------------------------------------------
///   Deallocate the fan data
//---------------------------------------------------------------------
void c_mesh_mem::dealloc_fans()
{
	this->tile_count  = 0;
	this->tiles.clear();
}


//---------------------------------------------------------------------
///   Allocate the fan data
///   \param p_fancount number of fans
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_mesh_mem::alloc_fans(int p_fancount)
{
	if (p_fancount == 0)
		return false;

	if (p_fancount < 0)
		p_fancount = MAXMESHFAN;

	if ((int)this->tile_count > p_fancount)
		return true;

	this->dealloc_fans();

	this->tiles.reserve(p_fancount);

	this->tile_count  = p_fancount;

	return true;
}


//---------------------------------------------------------------------
///   Build a vertex => tile lookup vector
//---------------------------------------------------------------------
void c_mesh_mem::build_vert_lookup_table()
{
	unsigned int cnt, cnt_tile, cnt_vert;

	cnt_tile = 0;
	cnt_vert = 0;

	this->vert_lookup.clear();
	this->vert_lookup.reserve(this->vrt_count);

	for (cnt = 0; cnt < (unsigned int)this->vrt_count; cnt++)
	{
		if ((int)cnt_vert >= this->tiles[cnt_tile].vert_count)
		{
			cnt_vert = 0;
			cnt_tile++;
		}

		vert_lookup[cnt] = cnt_tile;

		cnt_vert++;
	}
}


//---------------------------------------------------------------------
///   Build a x,y => tile ID lookup table
//---------------------------------------------------------------------
void c_mesh_mem::build_tile_lookup_table()
{
	unsigned int cnty, cntx, cnt_tile;
	vector<unsigned int> tmp_vec;

	cnt_tile = 0;

	this->tile_lookup.clear();
	tmp_vec.clear();

	for (cnty = 0; cnty < (unsigned int)this->tiles_y; cnty++)
	{
		this->tile_lookup.push_back(tmp_vec);

		for (cntx = 0; cntx < (unsigned int)this->tiles_x; cntx++)
		{
			tile_lookup[cnty].push_back(cnt_tile);

			cnt_tile++;
		}
	}
}


//---------------------------------------------------------------------
///   Get the tile ID based on an Y/X pair
///   \param p_y Y position
///   \param p_x X position
///   \return tile ID
//---------------------------------------------------------------------
int c_mesh_mem::get_tile_by_pos(int p_y, int p_x)
{
	return this->tile_lookup[p_y][p_x];
}


//---------------------------------------------------------------------
///   Get the vertex data based on a big vertex
///   \param p_vert vertex number
///   \return vertex position
//---------------------------------------------------------------------
vect3 c_mesh_mem::get_vert(int p_vert)
{
	int tile;

	if (p_vert > this->vrt_count)
	{
		cout << "WARNING! get_vert() called with a too big vertex!" << endl;
		cout << "ABORTING" << endl;
		exit(1);
	}

	tile = this->vert_lookup[p_vert];

//	cout << "      get_vert() for tile: " << tile << "/" << this->tile_count<< endl;
//	cout << "         p_vert:  " << p_vert << "/" << this->vrt_count << endl;
//	cout << "         start: " << this->tiles[tile].vrt_start << endl;
//	cout << "         vert:  " << (p_vert-this->tiles[tile].vrt_start) << "/" << this->tiles[tile].vert_count << endl;

	return this->tiles[tile].vertices[(p_vert-this->tiles[tile].vrt_start)];
}


//---------------------------------------------------------------------
///   Modify the x position of one vertex
///   \param p_modifier x modifier
///   \param p_vertex vertex number to modify
//---------------------------------------------------------------------
void c_mesh_mem::modify_verts_x(float p_modifier, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->x += p_modifier;
}


//---------------------------------------------------------------------
///   Modify the y position of one vertex
///   \param p_modifier y modifier
///   \param p_vertex vertex number to modify
//---------------------------------------------------------------------
void c_mesh_mem::modify_verts_y(float p_modifier, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->y += p_modifier;
}


//---------------------------------------------------------------------
///   Modify the z position of one vertex
///   \param p_modifier z modifier
///   \param p_vertex vertex number to modify
//---------------------------------------------------------------------
void c_mesh_mem::modify_verts_z(float p_modifier, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->z += p_modifier;
}


//---------------------------------------------------------------------
///   Set the vertex x position
///   \param p_new new x position
///   \param p_vertex vertex number to modify
//---------------------------------------------------------------------
void c_mesh_mem::set_verts_x(float p_new, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->x = p_new;
}


//---------------------------------------------------------------------
///   Set the vertex y position
///   \param p_new new y position
///   \param p_vertex vertex number to modify
//---------------------------------------------------------------------
void c_mesh_mem::set_verts_y(float p_new, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->y = p_new;
}


//---------------------------------------------------------------------
///   Set the vertex z position
///   \param p_new new z position
///   \param p_vertex vertex number to modify
//---------------------------------------------------------------------
void c_mesh_mem::set_verts_z(float p_new, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->z = p_new;
}


//---------------------------------------------------------------------
///   Get the vertex x position
///   \param p_vertex vertex number
///   \return x position
//---------------------------------------------------------------------
float c_mesh_mem::get_verts_x(int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	return this->tiles[tile].get_vertex(vertex)->x;
}


//---------------------------------------------------------------------
///   Get the vertex y position
///   \param p_vertex vertex number
///   \return y position
//---------------------------------------------------------------------
float c_mesh_mem::get_verts_y(int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	return this->tiles[tile].get_vertex(vertex)->y;
}


//---------------------------------------------------------------------
///   Get the vertex z position
///   \param p_vertex vertex number
///   \return z position
//---------------------------------------------------------------------
float c_mesh_mem::get_verts_z(int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	return this->tiles[tile].get_vertex(vertex)->z;
}


//---------------------------------------------------------------------
///   Change the type of a tile
///   \param p_tile tile number
///   \param p_new_type new type
//---------------------------------------------------------------------
void c_mesh_mem::change_tile_type(int p_tile, int p_new_type)
{
	unsigned int cnt_tile;
	unsigned int old_number_of_vertices;
	unsigned int number_of_vertices;
	int diff;
	unsigned int cnt;

	number_of_vertices = this->tile_definitions[this->tiles[p_tile].type].vrt_count;

	// Set the type
	this->tiles[p_tile].type = TTYPE_PILLER_TEN;


	old_number_of_vertices = this->tiles[p_tile].vert_count;
	this->tiles[p_tile].vert_count = number_of_vertices;
	this->tiles[p_tile].vertices.resize(number_of_vertices);

	diff = number_of_vertices - old_number_of_vertices;


	if (number_of_vertices > old_number_of_vertices)
	{
		for (cnt = 0; cnt < number_of_vertices; cnt++)
		{
			vect3 tmp_vec;
//			tmp_vec = g_module->getTileDefinition(this->tiles[p_tile].type).vrt[cnt];
			tmp_vec.x = 0;
			tmp_vec.y = 0;
			tmp_vec.z = 0;

			this->tiles[p_tile].vertices[cnt] = tmp_vec;
		}
	}


	// Loop through all tiles, starting with the one to modify
	for (cnt_tile = (unsigned int)p_tile; cnt_tile < this->tile_count; cnt_tile++)
	{
		// Modify the vertex start position of the other tiles
		this->tiles[cnt_tile].vrt_start += diff;
	}

cout << "TD.vrt_count: " << (int)this->tile_definitions[this->tiles[p_tile].type].vrt_count << endl;
cout << "TD.cmd_count: " << (int)this->tile_definitions[this->tiles[p_tile].type].cmd_count << endl;

	this->vrt_count += diff;

	this->build_vert_lookup_table();
}


//---------------------------------------------------------------------
///   c_tile constructor
//---------------------------------------------------------------------
c_tile::c_tile()
{
	this->vertices.clear();
}


//---------------------------------------------------------------------
///   c_tile destructor
//---------------------------------------------------------------------
c_tile::~c_tile()
{
}


//---------------------------------------------------------------------
///   Get a vertex based on the vertex ID
///   \param p_vertex vertex ID
///   \return 3d vector
//---------------------------------------------------------------------
vect3* c_tile::get_vertex(int p_vertex)
{
	return &this->vertices[p_vertex];
}


//---------------------------------------------------------------------
///   Is the tile a wall?
///   \return true if tile is a wall
//---------------------------------------------------------------------
bool c_tile::get_wall()
{
	return this->wall;
}


//---------------------------------------------------------------------
///   Set it to wall or floor
///   \param p_wall true for wall, false for floor
//---------------------------------------------------------------------
void c_tile::set_wall(bool p_wall)
{
	this->wall = p_wall;
}


//---------------------------------------------------------------------
///   c_aa_box constructor
//---------------------------------------------------------------------
c_aa_box::c_aa_box()
{
	this->sub_used = 0;
	this->weight   = 0.0f;

	this->used    = false;
	this->level   = 0;
	this->address = 0;

	mins.x = 0;
	mins.y = 0;
	mins.z = 0;

	maxs.x = 0;
	maxs.y = 0;
	maxs.z = 0;
}


//---------------------------------------------------------------------
///   c_aa_box destructor
//---------------------------------------------------------------------
c_aa_box::~c_aa_box()
{
}


//---------------------------------------------------------------------
///   This function loads fan types for the terrain
///   \param p_filename path + filename of fans.txt
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_tile_dictionary::load_tiledict(string p_filename)
{
	int cnt, entry;
	int numfantype, fantype, bigfantype, vertices;
	int numcommand, command, commandsize;

	ifstream if_fan;


	// Used to initialize the vector
	c_tile_definition filled_td;
	filled_td.vrt_count = 0;
	filled_td.cmd_count = 0;

	// Initialize all mesh types to 0
	entry = 0;
	while ( entry < MAXMESHTYPE )
	{
		this->tile_definitions.push_back(filled_td);

		entry++;
	}

	// Open the file and go to it
	cout << "Loading fan types of the terrain... " << endl;
	if_fan.open(p_filename.c_str());

	if (!if_fan)
	{
		cout << "Unable to open the file" << p_filename << endl;
		exit(1);
	}

	fantype    = 0;               // 32 x 32 tiles
	bigfantype = MAXMESHTYPE / 2; // 64 x 64 tiles
	numfantype = fget_next_int(if_fan);

	for ( /* nothing */; fantype < numfantype; fantype++, bigfantype++ )
	{

		vertices =
			this->tile_definitions[   fantype].vrt_count =
			this->tile_definitions[bigfantype].vrt_count = fget_next_int(if_fan);  // Dupe

		for ( cnt = 0; cnt < vertices; cnt++ )
		{
			this->tile_definitions[   fantype].ref[cnt] =
			this->tile_definitions[bigfantype].ref[cnt] = fget_next_int(if_fan);

			this->tile_definitions[   fantype].tx[cnt].u =
			this->tile_definitions[bigfantype].tx[cnt].u = fget_next_float(if_fan);

			this->tile_definitions[   fantype].tx[cnt].v =
			this->tile_definitions[bigfantype].tx[cnt].v = fget_next_float(if_fan);
		}

		numcommand =
			this->tile_definitions[   fantype].cmd_count =
			this->tile_definitions[bigfantype].cmd_count = fget_next_int(if_fan);  // Dupe

		for ( entry = 0, command = 0; command < numcommand; command++ )
		{
			commandsize                            =
				this->tile_definitions[   fantype].cmd_size[command] =
				this->tile_definitions[bigfantype].cmd_size[command] = fget_next_int(if_fan);  // Dupe

			for ( cnt = 0; cnt < commandsize; cnt++ )
			{
				this->tile_definitions[   fantype].vrt[entry] =
				this->tile_definitions[bigfantype].vrt[entry] = fget_next_int(if_fan);  // Dupe
				entry++;
			}
		}

	}
	if_fan.close();


	// Correct silly Cartman 31-pixel-wide textures to Egoboo's 256 pixel wide textures
	for ( cnt = 0; cnt < MAXMESHTYPE / 2; cnt++ )
	{
		for ( entry = 0; entry < this->tile_definitions[cnt].vrt_count; entry++ )
		{
			this->tile_definitions[cnt].tx[entry].u = ( TX_FUDGE + this->tile_definitions[cnt].tx[entry].u * ( 31.0f - TX_FUDGE ) ) / 256.0f;
			this->tile_definitions[cnt].tx[entry].v = ( TX_FUDGE + this->tile_definitions[cnt].tx[entry].v * ( 31.0f - TX_FUDGE ) ) / 256.0f;
		}

		// blank the unused values
		for ( /* nothing */; entry < MAXMESHVERTICES; entry++ )
		{
			this->tile_definitions[cnt].tx[entry].u = -1.0f;
			this->tile_definitions[cnt].tx[entry].v = -1.0f;
		}
	}

	// Correct silly Cartman 63-pixel-wide textures to Egoboo's 256 pixel wide textures
	for ( cnt = MAXMESHTYPE / 2; cnt < MAXMESHTYPE; cnt++ )
	{
		for ( entry = 0; entry < this->tile_definitions[cnt].vrt_count; entry++ )
		{
			this->tile_definitions[cnt].tx[entry].u = ( TX_FUDGE  + this->tile_definitions[cnt].tx[entry].u * ( 63.0f - TX_FUDGE ) ) / 256.0f;
			this->tile_definitions[cnt].tx[entry].v = ( TX_FUDGE  + this->tile_definitions[cnt].tx[entry].v * ( 63.0f - TX_FUDGE ) ) / 256.0f;
		}

		// blank the unused values
		for ( /* nothing */; entry < MAXMESHVERTICES; entry++ )
		{
			this->tile_definitions[cnt].tx[entry].u = -1.0f;
			this->tile_definitions[cnt].tx[entry].v = -1.0f;
		}
	}

	// Make tile texture offsets
	// 64 tiles per texture, 4 textures
	for ( cnt = 0; cnt < MAXTILETYPE; cnt++ )
	{
		TxBox[cnt].u = ( ( cnt >> 0 ) & 7 ) / 8.0f;
		TxBox[cnt].v = ( ( cnt >> 3 ) & 7 ) / 8.0f;
	}

	return true;
}
