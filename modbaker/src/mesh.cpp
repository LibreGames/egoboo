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

#include "general.h"
#include "global.h"
#include "mesh.h"
#include "utility.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#define MAPID 0x4470614d

c_tile_dictionary c_mesh::ms_tiledict;

//---------------------------------------------------------------------
//-   c_mesh_mem constructor
//---------------------------------------------------------------------
c_mesh_mem::c_mesh_mem(int vertcount, int fancount)
{
	if (fancount < 0)
		fancount  = MAXMESHFAN;

	if (vertcount >= 0 || fancount >= 0)
	{
		this->alloc_verts(vertcount);
		this->alloc_fans (fancount );
	}
}


//---------------------------------------------------------------------
//-   c_mesh_mem destructor
//---------------------------------------------------------------------
c_mesh_mem::~c_mesh_mem()
{
	this->dealloc_verts();
	this->dealloc_fans ();
}


//---------------------------------------------------------------------
//-   Deallocate the vertex data
//---------------------------------------------------------------------
void c_mesh_mem::dealloc_verts()
{
	this->vrt_count  = 0;
}


//---------------------------------------------------------------------
//-   Allocate the vertex data
//---------------------------------------------------------------------
bool c_mesh_mem::alloc_verts(int vertcount)
{
	if (vertcount == 0)
		return false;

	if (vertcount < 0)
		vertcount = 100; // TODO: Read from config

	if (this->vrt_count > vertcount)
		return true;

	this->dealloc_verts();

	this->vrt_count  = vertcount;

	return true;
}


//---------------------------------------------------------------------
//-   Deallocate the fan data
//---------------------------------------------------------------------
void c_mesh_mem::dealloc_fans()
{
	this->tile_count  = 0;
	this->tiles.clear();
}


//---------------------------------------------------------------------
//-   Allocate the fan data
//---------------------------------------------------------------------
bool c_mesh_mem::alloc_fans(int fancount)
{
	if (fancount == 0)
		return false;

	if (fancount < 0)
		fancount = MAXMESHFAN;

	if ((int)this->tile_count > fancount)
		return true;

	this->dealloc_fans();

	this->tiles.reserve(fancount);

	this->tile_count  = fancount;

	return true;
}


//---------------------------------------------------------------------
//-   c_mesh constructor
//---------------------------------------------------------------------
c_mesh::c_mesh()
{
	this->mem = new c_mesh_mem(0, 0);
	this->mi  = new c_mesh_info();
	this->init_meshfx();
}


c_mesh::~c_mesh()
{
	delete this->mem;
	delete this->mi;
}


//---------------------------------------------------------------------
//-   Load the .mpd file
//---------------------------------------------------------------------
bool c_mesh::load_mesh_mpd(string p_modname)
{
	cout << "in load_mesh_mpd()" << endl;
	// Temp. variables
	int itmp;
	float ftmp;
	int cnt;
	int vert, vrt;

	vector<vect3> vertices;
	vect3 tmp_vec;

	string filename;
	ifstream file;

	filename = g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/level.mpd";

	// FAN data
	Uint32 fan = 0;

	cout << "opening the file" << endl;
	// Open the file
	file.open(filename.c_str(), ios::binary);

	if (!file)
	{
		cout << "File not found " << filename << endl;
		return false;
	}

	cout << "beginning the file read" << endl;
	//-----------------------------------------------------------------
	//- Read and check the map id
	//-----------------------------------------------------------------
	file.read( (char*)&itmp, sizeof(int) );
	if (SDL_SwapLE32( itmp ) != MAPID )
	{
		printf("Invalid map file %x %x %x\n", MAPID, itmp, SDL_SwapLE32( itmp ));
		return false;
	}


	file.read((char*)&itmp, 4); this->mi->vert_count = SDL_SwapLE32(itmp); // Read number of verts
	file.read((char*)&itmp, 4); this->mi->tiles_x    = SDL_SwapLE32(itmp); // Read map x data
	file.read((char*)&itmp, 4); this->mi->tiles_y    = SDL_SwapLE32(itmp); // Read map y data


	//-----------------------------------------------------------------
	//- Fill the mpd structure
	//-----------------------------------------------------------------
	// Store the number of tiles
	this->mi->tile_count = this->mi->tiles_x * this->mi->tiles_y;

	cout << "reallocating the mesh memory" << endl;
	delete this->mem;
	this->mem = new c_mesh_mem(this->mi->vert_count, this->mi->tile_count);
	vertices.reserve(this->mi->vert_count);

	cout << " ...done." << endl;


	if (this->mem == NULL)
	{
		cout << "mesh_load() - " << endl
		<< "  Unable to initialize Mesh Memory. MPD file "
		<< filename.c_str() << " has too many vertices." << endl;
		return -1;
	}

	// Get the right down edge
	this->mi->edge_x = this->mi->tiles_x * 128;
	this->mi->edge_y = this->mi->tiles_y * 128;


	cout << "getting the basic fan data" << endl;
	//-----------------------------------------------------------------
	//- Get the basic fan data
	//-----------------------------------------------------------------
	for (fan = 0; fan < this->mi->tile_count; fan++)
	{
		file.read((char*)&itmp, 4);
		itmp = SDL_SwapLE32( itmp );

		this->mem->tiles[fan].type = itmp >> 24;
		this->mem->tiles[fan].fx   = itmp >> 16;
		this->mem->tiles[fan].tile = itmp;
		this->mem->tiles[fan].vert_count = g_mesh->getTileDefinition(this->mem->tiles[fan].type).vrt_count;
		this->mem->tiles[fan].vertices.reserve(this->mem->tiles[fan].vert_count);
	}


	//-----------------------------------------------------------------
	//- Read the twist data from the FAN
	//-----------------------------------------------------------------
	for (fan = 0; fan < this->mi->tile_count; fan++)
	{
		file.read((char*)&itmp, 1);
		this->mem->tiles[fan].twist = itmp;
	}


	//-----------------------------------------------------------------
	//- Read vertex x, y and z data
	//-----------------------------------------------------------------
	// Init the vector
	for (cnt = 0;  cnt < this->mi->vert_count; cnt++)
	{
		vertices[cnt] = tmp_vec;
	}


	// Load vertex fan_x data
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		file.read((char*)&ftmp, 4);
		vertices[cnt].x = SwapLE_float(ftmp);
	}


	// Load vertex fan_y data
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		file.read((char*)&ftmp, 4);
		vertices[cnt].y = SwapLE_float(ftmp);
	}

	// Load vertex fan_z data
	cnt = 0;
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		file.read((char*)&ftmp, 4);
		vertices[cnt].z = SwapLE_float(ftmp)  / 16.0;
		// Cartman uses 4 bit fixed point for Z
	}


	cout << "...done." << endl;


	//-----------------------------------------------------------------
	//- Load vertex lighting data
	//-----------------------------------------------------------------
	/*
		for (cnt = 0; cnt < mi->vert_count; cnt++)
		{
			infile.read((char*)&itmp,1);

			mem->vrt_ar_fp8[cnt] = 0; // itmp;
			mem->vrt_ag_fp8[cnt] = 0; // itmp;
			mem->vrt_ab_fp8[cnt] = 0; // itmp;

			mem->vrt_lr_fp8[cnt] = 0;
			mem->vrt_lg_fp8[cnt] = 0;
			mem->vrt_lb_fp8[cnt] = 0;
		}
	*/
	cout << "closing the file" << endl;
	file.close();
	cout << "...done." << endl;


	//-----------------------------------------------------------------
	//- Split the big "vertices" vector into separate tiles
	//-----------------------------------------------------------------
	cout << "Splitting up the vertices" << endl;
	Uint32 cnt_tile = 0; // Number of the current tile
	Uint32 cnt_vert = 0; // Number of vertices already in the tile

	// Go through all vertices we found in the mesh
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		// Did we reach the vrt_count for the current tile?
		if (cnt_vert >= g_mesh->getTileDefinition(this->mem->tiles[cnt_tile].type).vrt_count)
		{
			cnt_tile++;
			cnt_vert = 0;
			this->mem->tiles[cnt_tile].vrt_start = cnt;
		}

		this->mem->tiles[cnt_tile].vertices[cnt_vert] = vertices[cnt];
//		cout << "tiles[" <<  cnt_tile<< "].vertices[" << cnt_vert << "] = (" << vertices[cnt].x << ", " << vertices[cnt].y << ", " << vertices[cnt].z << ")" << endl;
		cnt_vert++;
	}


	cout << "...done." << endl;


	cout << "mesh_make_fanstart()" << endl;
	//-----------------------------------------------------------------
	//- Build the lookup table for each fan
	//-----------------------------------------------------------------
	this->mesh_make_fanstart();
	cout << "...done." << endl;

	cout << "build_tile_lookup_table()" << endl;
	this->mem->build_tile_lookup_table();
	cout << "...done." << endl;


	//-----------------------------------------------------------------
	//-
	//-----------------------------------------------------------------
	vert = 0;

	cout << "setting up the tile dictionary" << endl;
	for (fan = 0; fan < mem->tile_count; fan++)
	{
		int vrtcount = c_mesh::ms_tiledict[this->mem->tiles[fan].type].vrt_count;
		int vrtstart = vert;

		this->mem->tiles[fan].vrt_start = vrtstart;

		this->mem->tiles[fan].bbox.mins.x = this->mem->tiles[fan].bbox.maxs.x = vertices[vrtstart].x;
		this->mem->tiles[fan].bbox.mins.y = this->mem->tiles[fan].bbox.maxs.y = vertices[vrtstart].y;
		this->mem->tiles[fan].bbox.mins.z = this->mem->tiles[fan].bbox.maxs.z = vertices[vrtstart].z;


		for (vrt = vrtstart + 1; vrt < vrtstart + vrtcount; vrt++)
		{
			this->mem->tiles[fan].bbox.mins.x = MIN( this->mem->tiles[fan].bbox.mins.x, vertices[vrt].x );
			this->mem->tiles[fan].bbox.mins.y = MIN( this->mem->tiles[fan].bbox.mins.y, vertices[vrt].y );
			this->mem->tiles[fan].bbox.mins.z = MIN( this->mem->tiles[fan].bbox.mins.z, vertices[vrt].z );

			this->mem->tiles[fan].bbox.maxs.x = MAX( this->mem->tiles[fan].bbox.maxs.x, vertices[vrt].x );
			this->mem->tiles[fan].bbox.maxs.y = MAX( this->mem->tiles[fan].bbox.maxs.y, vertices[vrt].y );
			this->mem->tiles[fan].bbox.maxs.z = MAX( this->mem->tiles[fan].bbox.maxs.z, vertices[vrt].z );
		}

		vert += vrtcount;
	}

	cout << "Leaving load_mesh_mpd()" << endl;

	return true;
}


//---------------------------------------------------------------------
//-   This function loads fan types for the terrain
//---------------------------------------------------------------------
bool c_tile_dictionary::load()
{
	int cnt, entry;
	int numfantype, fantype, bigfantype, vertices;
	int numcommand, command, commandsize;

	ifstream if_fan;

	my_vector & vect = getData();

	// Used to initialize the vector
	c_tile_definition filled_td;
	filled_td.vrt_count = 0;
	filled_td.cmd_count = 0;

	// Initialize all mesh types to 0
	entry = 0;
	while ( entry < MAXMESHTYPE )
	{
		this->push_back(filled_td);

		entry++;
	}

	string filename_fans;
	filename_fans = g_config->get_egoboo_path() + "basicdat/fans.txt";

	// Open the file and go to it
	cout << "Loading fan types of the terrain... " << endl;
	if_fan.open(filename_fans.c_str());

	if (!if_fan)
	{
		cout << "Unable to open the file" << endl;
		exit(1);
	}

	fantype    = 0;               // 32 x 32 tiles
	bigfantype = MAXMESHTYPE / 2; // 64 x 64 tiles
	numfantype = fget_next_int(if_fan);

	for ( /* nothing */; fantype < numfantype; fantype++, bigfantype++ )
	{

		vertices                       =
			vect[   fantype].vrt_count =
			vect[bigfantype].vrt_count = fget_next_int(if_fan);  // Dupe

		for ( cnt = 0; cnt < vertices; cnt++ )
		{
			vect[   fantype].ref[cnt] =
				vect[bigfantype].ref[cnt] = fget_next_int(if_fan);

			vect[   fantype].tx[cnt].u =
				vect[bigfantype].tx[cnt].u = fget_next_float(if_fan);

			vect[   fantype].tx[cnt].v =
				vect[bigfantype].tx[cnt].v = fget_next_float(if_fan);
		}

		numcommand                     =
			vect[   fantype].cmd_count =
			vect[bigfantype].cmd_count = fget_next_int(if_fan);  // Dupe

		for ( entry = 0, command = 0; command < numcommand; command++ )
		{
			commandsize                            =
				vect[   fantype].cmd_size[command] =
					vect[bigfantype].cmd_size[command] = fget_next_int(if_fan);  // Dupe

			for ( cnt = 0; cnt < commandsize; cnt++ )
			{
				vect[   fantype].vrt[entry] =
					vect[bigfantype].vrt[entry] = fget_next_int(if_fan);  // Dupe
				entry++;
			}
		}

	}
	if_fan.close();


	// Correct silly Cartman 31-pixel-wide textures to Egoboo's 256 pixel wide textures
	for ( cnt = 0; cnt < MAXMESHTYPE / 2; cnt++ )
	{
		for ( entry = 0; entry < vect[cnt].vrt_count; entry++ )
		{

			vect[cnt].tx[entry].u = ( TX_FUDGE + vect[cnt].tx[entry].u * ( 31.0f - TX_FUDGE ) ) / 256.0f;
			vect[cnt].tx[entry].v = ( TX_FUDGE + vect[cnt].tx[entry].v * ( 31.0f - TX_FUDGE ) ) / 256.0f;
		}

		// blank the unused values
		for ( /* nothing */; entry < MAXMESHVERTICES; entry++ )
		{
			vect[cnt].tx[entry].u = -1.0f;
			vect[cnt].tx[entry].v = -1.0f;
		}
	}

	// Correct silly Cartman 63-pixel-wide textures to Egoboo's 256 pixel wide textures
	for ( cnt = MAXMESHTYPE / 2; cnt < MAXMESHTYPE; cnt++ )
	{
		for ( entry = 0; entry < vect[cnt].vrt_count; entry++ )
		{
			vect[cnt].tx[entry].u = ( TX_FUDGE  + vect[cnt].tx[entry].u * ( 63.0f - TX_FUDGE ) ) / 256.0f;
			vect[cnt].tx[entry].v = ( TX_FUDGE  + vect[cnt].tx[entry].v * ( 63.0f - TX_FUDGE ) ) / 256.0f;
		}

		// blank the unused values
		for ( /* nothing */; entry < MAXMESHVERTICES; entry++ )
		{
			vect[cnt].tx[entry].u = -1.0f;
			vect[cnt].tx[entry].v = -1.0f;
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


//---------------------------------------------------------------------
//   This function builds a look up table to ease calculating the
//   fan number given an x,y pair
//---------------------------------------------------------------------
void c_mesh::mesh_make_fanstart()
{
	int cnt;

	for (cnt = 0; cnt < this->mi->tiles_y; cnt++)
	{
		this->mi->Tile_X[cnt] = this->mi->tiles_x * cnt;
	}

	for (cnt = 0; cnt < this->mi->blocks_y; cnt++)
	{
		this->mi->Block_X[cnt] = this->mi->blocks_x * cnt;
	}
}


//---------------------------------------------------------------------
//-   Modify all verts at the position of the selected vertex (X axis)
//---------------------------------------------------------------------
int c_mesh::modify_verts_x(float p_modifier, int p_vert)
{
	int i;
	int num_modified;

	num_modified = 0;

	vect3 pos_old, pos_test;

	pos_old = this->mem->get_vert(p_vert);

	for (i = 0; i < this->mi->vert_count; i++)
	{
		pos_test = this->mem->get_vert(i);
		if ((pos_test.x == pos_old.x) &&
			(pos_test.y == pos_old.y) &&
			(pos_test.z == pos_old.z))
		{
			this->mem->modify_verts_x(p_modifier, i);

			num_modified++;
		}
	}

	return num_modified;
}


//---------------------------------------------------------------------
//-   Modify all verts at the position of the selected vertex (Y axis)
//---------------------------------------------------------------------
int c_mesh::modify_verts_y(float p_modifier, int p_vert)
{
	int i;
	int num_modified;

	num_modified = 0;

	vect3 pos_old, pos_test;

	pos_old = this->mem->get_vert(p_vert);

	for (i = 0; i < this->mi->vert_count; i++)
	{
		pos_test = this->mem->get_vert(i);
		if ((pos_test.x == pos_old.x) &&
			(pos_test.y == pos_old.y) &&
			(pos_test.z == pos_old.z))
		{
			this->mem->modify_verts_y(p_modifier, i);

			num_modified++;
		}
	}

	return num_modified;
}


//---------------------------------------------------------------------
//-   Modify all verts at the position of the selected vertex (Z axis)
//---------------------------------------------------------------------
int c_mesh::modify_verts_z(float p_modifier, int p_vert)
{
	int i;
	int num_modified;

	num_modified = 0;

	vect3 pos_old, pos_test;

	pos_old = this->mem->get_vert(p_vert);

	for (i = 0; i < this->mi->vert_count; i++)
	{
		pos_test = this->mem->get_vert(i);
		if ((pos_test.x == pos_old.x) &&
			(pos_test.y == pos_old.y) &&
			(pos_test.z == pos_old.z))
		{
			this->mem->modify_verts_z(p_modifier, i);

			num_modified++;
		}
	}

	return num_modified;
}


//---------------------------------------------------------------------
//-   Get the nearest vertex
//---------------------------------------------------------------------
int c_mesh::get_nearest_vertex(float pos_x, float pos_y, float pos_z)
{
	int i;

	int nearest_vertex = 0;

	double dist_temp;
	double dist_nearest;

	vect3 ref;     // The reference point

	ref.x = pos_x;
	ref.y = pos_y;
	ref.z = pos_z;

	dist_nearest = 999999.9f;

	for (i = 0; i < this->mi->vert_count; i++)
	{
		dist_temp = calculate_distance(this->mem->get_vert(i), ref);

		if (dist_temp < dist_nearest)
		{
			dist_nearest = calculate_distance(this->mem->get_vert(i), ref);

			nearest_vertex = i;
		}
	}

	return nearest_vertex;
}


//---------------------------------------------------------------------
//-   Get the nearest tile
//---------------------------------------------------------------------
int c_mesh::get_nearest_tile(float pos_x, float pos_y, float pos_z)
{
	int i;
	int vert_start;

	int nearest_tile = 0;

	double dist_temp;
	double dist_nearest;

	vect3 ref;   // The reference point

	ref.x = pos_x;
	ref.y = pos_y;
	ref.z = pos_z;

	dist_nearest = 999999.9f;

	for (i = 0; i < (int)this->mem->tile_count; i++)
	{
		vert_start = g_mesh->mem->tiles[i].vrt_start;

		dist_temp = calculate_distance(this->mem->get_vert(vert_start), ref);

		if (dist_temp < dist_nearest)
		{
			dist_nearest = calculate_distance(this->mem->get_vert(vert_start), ref);

			nearest_tile = i;
		}
	}

	return nearest_tile;
}


//---------------------------------------------------------------------
//-   Get the nearest object
//---------------------------------------------------------------------
int c_mesh::get_nearest_object(float pos_x, float pos_y, float pos_z)
{
	int i;

	int nearest_object = 0;

	double dist_temp;
	double dist_nearest;

	vect3 ref;   // The reference point
	vect3 object;  // Used for each object

	ref.x = pos_x;
	ref.y = pos_y;
	ref.z = pos_z;

	dist_nearest = 999999.9f;

	for (i = 0; i < (int)g_object_manager->get_spawns_size(); i++)
	{

		object.x = g_object_manager->get_spawn(i).pos.x * (1 << 7);
		object.y = g_object_manager->get_spawn(i).pos.y * (1 << 7);
//		object.z = g_object_manager->get_spawn(i).pos.z * (1 << 7);
		object.z = 1.0f                                 * (1 << 7);

		dist_temp = calculate_distance(object, ref);

		if (dist_temp < dist_nearest)
		{
			dist_nearest = calculate_distance(object, ref);

			nearest_object = i;
		}
	}

	return nearest_object;
}


//---------------------------------------------------------------------
//-   Save the .mpd file
//---------------------------------------------------------------------
bool c_mesh::save_mesh_mpd(string filename)
{
	// Temp. variables
	int itmp;
	float ftmp;
	int cnt;
	vector<vect3> vertices;

	ofstream file;

	//-----------------------------------------------------------------
	//-   First unpack the tiles into one big vector
	//-----------------------------------------------------------------
	cout << "Unpacking the vertices" << endl;
	int cnt_tile = 0; // Number of the current tile
	int cnt_vert = 0; // Number of vertices already in the tile
	cnt = 0;

	vertices.reserve(this->mem->vrt_count);

	// Go through all tiles
	for (cnt_tile = 0; cnt_tile < (int)this->mi->tile_count; cnt_tile++)
	{
		// Go through all vertices
		for (cnt_vert = 0; cnt_vert < this->mem->tiles[cnt_tile].vert_count; cnt_vert++)
		{
			vertices[cnt] = this->mem->tiles[cnt_tile].vertices[cnt_vert];
			cnt++;
		}
	}
	cout << "...done." << endl;


	// FAN data
	Uint32 fan = 0;

	// Open the file
	file.open(filename.c_str(), ios::binary);

	if (!file)
	{
		cout << "File not found " << filename << endl;
		return false;
	}

	//-----------------------------------------------------------------
	//- Read and check the map id
	//-----------------------------------------------------------------
	itmp = MAPID;
	file.write((char*)&itmp, 4);

	itmp = this->mi->vert_count;
	file.write((char *)&itmp, 4);

	itmp = this->mi->tiles_x;
	file.write((char *)&itmp, 4);

	itmp = this->mi->tiles_y;
	file.write((char *)&itmp, 4);


	//-----------------------------------------------------------------
	//- Fill the mpd structure
	//-----------------------------------------------------------------
	for ( fan = 0; fan < this->mi->tile_count;  fan++)
	{
		itmp = (this->mem->tiles[fan].type << 24) + (this->mem->tiles[fan].fx << 16) + this->mem->tiles[fan].tile;
		file.write((char *)&itmp, 4);
	}


	//-----------------------------------------------------------------
	//- Read the twist data from the FAN
	//-----------------------------------------------------------------
	for (fan = 0; fan < this->mi->tile_count; fan++)
	{
		itmp = this->mem->tiles[fan].twist;
		file.write((char *)&itmp, 1);
	}


	//-----------------------------------------------------------------
	//- Read vertex x,y and z data
	//-----------------------------------------------------------------
	// Load vertex fan_x data
	for (cnt = 0;  cnt < this->mi->vert_count; cnt++)
	{
		ftmp = vertices[cnt].x;
		file.write((char *)&ftmp, 4);
	}


	// Load vertex fan_y data
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		ftmp = vertices[cnt].y;
		file.write((char *)&ftmp, 4);
	}

	// Load vertex z data
	cnt = 0;
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		ftmp = vertices[cnt].z * 16;
		file.write((char *)&ftmp, 4);
		// Cartman uses 4 bit fixed point for Z
	}


	//-----------------------------------------------------------------
	//- Load vertex lighting data
	//-----------------------------------------------------------------
	for (cnt = 0; cnt < mi->vert_count; cnt++)
	{
		itmp = 0;
		file.write((char *)&itmp, 1);
	}

	file.close();

	cout << "File saved: " << filename << endl;

	return true;
}



//---------------------------------------------------------------------
//-   Init the meshfx vector
//---------------------------------------------------------------------
void c_mesh::init_meshfx()
{
	int tmpi;

	tmpi = MESHFX_REF;
	this->m_meshfx.push_back(tmpi);

	tmpi = MESHFX_SHA;
	this->m_meshfx.push_back(tmpi);

	tmpi = MESHFX_DRAWREF;
	this->m_meshfx.push_back(tmpi);

	tmpi = MESHFX_ANIM;
	this->m_meshfx.push_back(tmpi);

	tmpi = MESHFX_WATER;
	this->m_meshfx.push_back(tmpi);

	tmpi = MESHFX_WALL;
	this->m_meshfx.push_back(tmpi);

	tmpi = MESHFX_IMPASS;
	this->m_meshfx.push_back(tmpi);

	tmpi = MESHFX_DAMAGE;
	this->m_meshfx.push_back(tmpi);

	tmpi = MESHFX_SLIPPY;
	this->m_meshfx.push_back(tmpi);
}


//---------------------------------------------------------------------
//-   Get the Mesh FX based on the vector entry
//---------------------------------------------------------------------
int c_mesh::get_meshfx(int p_entry)
{
	return this->m_meshfx[p_entry];
}


//---------------------------------------------------------------------
//-   Get all vertices of the fan
//---------------------------------------------------------------------
vector<int> c_mesh::get_fan_vertices(int p_fan)
{
	int cnt;
	vector<int> vec_vertices;

	Uint16 type      = g_mesh->mem->tiles[p_fan].type; // Command type ( index to points in fan )
	int vertices     = g_mesh->getTileDefinition(type).vrt_count;
	Uint32 badvertex = g_mesh->mem->tiles[p_fan].vrt_start;   // Actual vertex number

	for (cnt = 0; cnt < vertices; cnt++, badvertex++)
	{
		vec_vertices.push_back(badvertex);
	}
	return vec_vertices;
}


//---------------------------------------------------------------------
//-   Change the size of the mesh mem
//---------------------------------------------------------------------
bool c_mesh::change_mem_size(int p_new_vertices, int p_new_fans)
{
	if (this->mem == NULL)
	{
		cout << "change_mem_size: Aborting: mem is NULL" << endl;
		return false;
	}

	// TODO: Change the size of the mesh mem

	this->mem->vrt_count += 2;
	return true;
}


//---------------------------------------------------------------------
//-   Build a vertex => tile lookup vector
//---------------------------------------------------------------------
void c_mesh_mem::build_tile_lookup_table()
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
//-   Get the vertex start of the same tile based on a vertex number
//---------------------------------------------------------------------
// TODO: redo this function
int c_mesh_mem::get_vert_start(int p_vert)
{
	unsigned int cnt;

	for (cnt = 0; cnt < this->tile_count; cnt++)
	{
		if (this->tiles[cnt].vrt_start > (unsigned int)p_vert)
			return this->tiles[cnt-1].vrt_start;

		if (this->tiles[cnt].vrt_start == (unsigned int)p_vert)
			return this->tiles[cnt].vrt_start;
	}

	return -1;
}


//---------------------------------------------------------------------
//-   Get the vertex data based on a big vertex
//---------------------------------------------------------------------
vect3 c_mesh_mem::get_vert(int p_vert)
{
	int tile;

	tile = this->vert_lookup[p_vert];

//	cout << "      get_vert() for tile: " << tile << "/" << this->tile_count<< endl;
//	cout << "         p_vert:  " << p_vert << "/" << this->vrt_count << endl;
//	cout << "         start: " << this->tiles[tile].vrt_start << endl;
//	cout << "         vert:  " << (p_vert-this->tiles[tile].vrt_start) << "/" << this->tiles[tile].vert_count << endl;

	return this->tiles[tile].vertices[(p_vert-this->tiles[tile].vrt_start)];
}


c_tile::c_tile()
{
}

c_tile::~c_tile()
{
}


void c_mesh_mem::modify_verts_x(float p_modifier, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->x += p_modifier;
}


void c_mesh_mem::modify_verts_y(float p_modifier, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->y += p_modifier;
}


void c_mesh_mem::modify_verts_z(float p_modifier, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->z += p_modifier;
}


//---------------------------------------------------------------------
//-   Set the vertex x position
//---------------------------------------------------------------------
void c_mesh_mem::set_verts_x(float p_new, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->x = p_new;
}


//---------------------------------------------------------------------
//-   Set the vertex y position
//---------------------------------------------------------------------
void c_mesh_mem::set_verts_y(float p_new, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->y = p_new;
}


//---------------------------------------------------------------------
//-   Set the vertex z position
//---------------------------------------------------------------------
void c_mesh_mem::set_verts_z(float p_new, int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	this->tiles[tile].get_vertex(vertex)->z = p_new;
}


//---------------------------------------------------------------------
//-   Get the vertex x position
//---------------------------------------------------------------------
float c_mesh_mem::get_verts_x(int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	return this->tiles[tile].get_vertex(vertex)->x;
}


//---------------------------------------------------------------------
//-   Get the vertex y position
//---------------------------------------------------------------------
float c_mesh_mem::get_verts_y(int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	return this->tiles[tile].get_vertex(vertex)->y;
}


//---------------------------------------------------------------------
//-   Get the vertex z position
//---------------------------------------------------------------------
float c_mesh_mem::get_verts_z(int p_vertex)
{
	unsigned int tile, vertex;

	tile   = this->vert_lookup[p_vertex];
	vertex = p_vertex - this->tiles[tile].vrt_start;

	return this->tiles[tile].get_vertex(vertex)->z;
}


vect3* c_tile::get_vertex(int p_vertex)
{
	return &this->vertices[p_vertex];
}
