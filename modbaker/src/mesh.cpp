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

	this->vrt_x      = NULL;
	this->vrt_y      = NULL;
	this->vrt_z      = NULL;

	this->vrt_ar_fp8 = NULL;
	this->vrt_ag_fp8 = NULL;
	this->vrt_ab_fp8 = NULL;

	this->vrt_lr_fp8 = NULL;
	this->vrt_lg_fp8 = NULL;
	this->vrt_lb_fp8 = NULL;
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

	this->vrt_count = 0;
	this->base = calloc( vertcount, BYTESFOREACHVERTEX );

	if ( NULL == this->base )
		return false;

	this->vrt_count  = vertcount;

	this->vrt_x      = (float *)this->base;
	this->vrt_y      = this->vrt_x + vertcount;
	this->vrt_z      = this->vrt_y + vertcount;

	this->vrt_ar_fp8 = (Uint8*) (this->vrt_z + vertcount);
	this->vrt_ag_fp8 = this->vrt_ar_fp8 + vertcount;
	this->vrt_ab_fp8 = this->vrt_ag_fp8 + vertcount;

	this->vrt_lr_fp8 = (float *)(this->vrt_ab_fp8 + vertcount);
	this->vrt_lg_fp8 = this->vrt_lr_fp8 + vertcount;
	this->vrt_lb_fp8 = this->vrt_lg_fp8 + vertcount;

	return true;
}


//---------------------------------------------------------------------
//-   Deallocate the fan data
//---------------------------------------------------------------------
void c_mesh_mem::dealloc_fans()
{
	// TODO: This causes a segfault!
//	EGOBOO_DELETE ( this->tilelst );

	this->tile_count  = 0;
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

	this->tile_count = 0;
	this->tilelst = EGOBOO_NEW_ARY( MeshTile_t, fancount );

	if (this->tilelst == NULL)
		return false;

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
}


c_mesh::~c_mesh()
{
	delete spawn_manager;
	spawn_manager = NULL;

	delete this->mem;
	delete this->mi;
}


c_spawn_manager* c_mesh::get_spawn_manager()
{
	return spawn_manager;
}


void c_mesh::set_spawn_manager(c_spawn_manager* p_spawn_manager)
{
	spawn_manager = p_spawn_manager;
}


//---------------------------------------------------------------------
//-   Load the .mpd file
//---------------------------------------------------------------------
bool c_mesh::load_mesh_mpd(string p_modname)
{
	// Temp. variables
	int itmp;
	float ftmp;
	int cnt;
	int vert, vrt;

	string filename;
	ifstream file;

	filename = g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/level.mpd";

	// FAN data
	Uint32 fan = 0;

	MeshTile_t  *mf_list; // TODO: Replace with mem.tilelst


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

	delete this->mem;
	this->mem = new c_mesh_mem(this->mi->vert_count, this->mi->tile_count);

	if (this->mem == NULL)
	{
		cout << "mesh_load() - " << endl
		<< "  Unable to initialize Mesh Memory. MPD file "
		<< filename.c_str() << " has too many vertices." << endl;
		return -1;
	}

	// wait until fanlist is allocated!
	mf_list = this->mem->tilelst;

	// Get the right down edge
	this->mi->edge_x = this->mi->tiles_x * 128;
	this->mi->edge_y = this->mi->tiles_y * 128;

	//-----------------------------------------------------------------
	//- Allocate the size of mf_list based on the tile count
	//-----------------------------------------------------------------
	mf_list = (MeshTile_t *)malloc(this->mi->tile_count * sizeof(MeshTile_t));

	for (fan = 0; fan < this->mi->tile_count; fan++)
	{
		file.read((char*)&itmp, 4);
		itmp = SDL_SwapLE32( itmp );

		mf_list[fan].type = itmp >> 24;
		mf_list[fan].fx   = itmp >> 16;
		mf_list[fan].tile = itmp;
	}


	//-----------------------------------------------------------------
	//- Read the twist data from the FAN
	//-----------------------------------------------------------------
	for (fan = 0; fan < this->mi->tile_count; fan++)
	{
		file.read((char*)&itmp, 1);
		mf_list[fan].twist = itmp;
	}


	//-----------------------------------------------------------------
	//- Read vertex x,y and z data
	//-----------------------------------------------------------------
	// Load vertex fan_x data
	for (cnt = 0;  cnt < this->mi->vert_count; cnt++)
	{
		file.read((char*)&ftmp, 4);
		this->mem->vrt_x[cnt] = SwapLE_float(ftmp);
	}


	// Load vertex fan_y data
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		file.read((char*)&ftmp, 4);
		this->mem->vrt_y[cnt] = SwapLE_float(ftmp);
	}

	// Load vertex z data
	cnt = 0;
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		file.read((char*)&ftmp, 4);
		this->mem->vrt_z[cnt] = SwapLE_float(ftmp)  / 16.0;
		// Cartman uses 4 bit fixed point for Z
	}


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
	file.close();


	//-----------------------------------------------------------------
	//- Build the lookup table for each fan
	//-----------------------------------------------------------------
	mesh_make_fanstart();


	//-----------------------------------------------------------------
	//-
	//-----------------------------------------------------------------
	vert = 0;

	for (fan = 0; fan < mem->tile_count; fan++)
	{
		int vrtcount = c_mesh::ms_tiledict[mf_list[fan].type].vrt_count;
		int vrtstart = vert;

		mf_list[fan].vrt_start = vrtstart;

		mf_list[fan].bbox.mins.x = mf_list[fan].bbox.maxs.x = mem->vrt_x[vrtstart];
		mf_list[fan].bbox.mins.y = mf_list[fan].bbox.maxs.y = mem->vrt_y[vrtstart];
		mf_list[fan].bbox.mins.z = mf_list[fan].bbox.maxs.z = mem->vrt_z[vrtstart];


		for (vrt = vrtstart + 1; vrt < vrtstart + vrtcount; vrt++)
		{
			mf_list[fan].bbox.mins.x = MIN( mf_list[fan].bbox.mins.x, mem->vrt_x[vrt] );
			mf_list[fan].bbox.mins.y = MIN( mf_list[fan].bbox.mins.y, mem->vrt_y[vrt] );
			mf_list[fan].bbox.mins.z = MIN( mf_list[fan].bbox.mins.z, mem->vrt_z[vrt] );

			mf_list[fan].bbox.maxs.x = MAX( mf_list[fan].bbox.maxs.x, mem->vrt_x[vrt] );
			mf_list[fan].bbox.maxs.y = MAX( mf_list[fan].bbox.maxs.y, mem->vrt_y[vrt] );
			mf_list[fan].bbox.maxs.z = MAX( mf_list[fan].bbox.maxs.z, mem->vrt_z[vrt] );
		}

		vert += vrtcount;
	}

	// Store it in the tilelst again
	this->mem->tilelst = mf_list;

	spawn_manager = new c_spawn_manager();
	spawn_manager->load(p_modname);

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

		vertices                   =
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
int c_mesh::modify_verts_x(float x_modifier, int vert)
{
	int i;
	int num_modified;

	num_modified = 0;

	vect3 pos_old;

	pos_old.x = this->mem->vrt_x[vert];
	pos_old.y = this->mem->vrt_y[vert];
	pos_old.z = this->mem->vrt_z[vert];

	for (i = 0; i < this->mi->vert_count; i++)
	{
		if ((this->mem->vrt_x[i] == pos_old.x) &&
			(this->mem->vrt_y[i] == pos_old.y) &&
			(this->mem->vrt_z[i] == pos_old.z))
		{
			this->mem->vrt_x[i] += x_modifier;

			num_modified++;
		}
	}

	return num_modified;
}


//---------------------------------------------------------------------
//-   Modify all verts at the position of the selected vertex (Y axis)
//---------------------------------------------------------------------
int c_mesh::modify_verts_y(float y_modifier, int vert)
{
	int i;
	int num_modified;

	num_modified = 0;

	vect3 pos_old;

	pos_old.x = this->mem->vrt_x[vert];
	pos_old.y = this->mem->vrt_y[vert];
	pos_old.z = this->mem->vrt_z[vert];

	for (i = 0; i < this->mi->vert_count; i++)
	{
		if ((this->mem->vrt_x[i] == pos_old.x) &&
			(this->mem->vrt_y[i] == pos_old.y) &&
			(this->mem->vrt_z[i] == pos_old.z))
		{
			this->mem->vrt_y[i] += y_modifier;

			num_modified++;
		}
	}

	return num_modified;
}


//---------------------------------------------------------------------
//-   Modify all verts at the position of the selected vertex (Z axis)
//---------------------------------------------------------------------
int c_mesh::modify_verts_z(float z_modifier, int vert)
{
	int i;
	int num_modified;

	num_modified = 0;

	vect3 pos_old;

	pos_old.x = this->mem->vrt_x[vert];
	pos_old.y = this->mem->vrt_y[vert];
	pos_old.z = this->mem->vrt_z[vert];

	for (i = 0; i < this->mi->vert_count; i++)
	{
		if ((this->mem->vrt_x[i] == pos_old.x) &&
			(this->mem->vrt_y[i] == pos_old.y) &&
			(this->mem->vrt_z[i] == pos_old.z))
		{
			this->mem->vrt_z[i] += z_modifier;

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
	vect3 vertex;  // Used for each vertex

	ref.x = pos_x;
	ref.y = pos_y;
	ref.z = pos_z;

	dist_nearest = 999999.9f;

	for (i = 0; i < this->mi->vert_count; i++)
	{
		vertex.x = this->mem->vrt_x[i];
		vertex.y = this->mem->vrt_y[i];
		vertex.z = this->mem->vrt_z[i];

		dist_temp = calculate_distance(vertex, ref);

		if (dist_temp < dist_nearest)
		{
			dist_nearest = calculate_distance(vertex, ref);

			nearest_vertex = i;
		}
	}

	return nearest_vertex;
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

	ofstream file;

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
		itmp = (this->mem->tilelst[fan].type << 24) + (this->mem->tilelst[fan].fx << 16) + this->mem->tilelst[fan].tile;
		file.write((char *)&itmp, 4);
	}


	//-----------------------------------------------------------------
	//- Read the twist data from the FAN
	//-----------------------------------------------------------------
	for (fan = 0; fan < this->mi->tile_count; fan++)
	{
		itmp = this->mem->tilelst[fan].twist;
		file.write((char *)&itmp, 1);
	}


	//-----------------------------------------------------------------
	//- Read vertex x,y and z data
	//-----------------------------------------------------------------
	// Load vertex fan_x data
	for (cnt = 0;  cnt < this->mi->vert_count; cnt++)
	{
		ftmp = this->mem->vrt_x[cnt];
		file.write((char *)&ftmp, 4);
	}


	// Load vertex fan_y data
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		ftmp = this->mem->vrt_y[cnt];
		file.write((char *)&ftmp, 4);
	}

	// Load vertex z data
	cnt = 0;
	for (cnt = 0; cnt < this->mi->vert_count; cnt++)
	{
		ftmp = this->mem->vrt_z[cnt] * 16;
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


c_spawn_manager::c_spawn_manager()
{
	cout << "Creating the spawn manager" << endl;
}


c_spawn_manager::~c_spawn_manager()
{
	cout << "Deleting the spawn manager" << endl;
}


void c_spawn_manager::load(string p_modname)
{
	ifstream f_spawn;
	c_spawn temp_spawn;

	string filename;
	filename = g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/spawn.txt";

	f_spawn.open(filename.c_str());

	if (!f_spawn)
	{
		cout << "spawn.txt not found!" << endl;
		Quit();
	}

	string sTmp;
	int    iTmp;
	float  fTmp;
	char   cTmp;

	while (!f_spawn.eof())
	{
		if (fgoto_colon_yesno(f_spawn))
		{
			f_spawn >> sTmp;  temp_spawn.name        = sTmp;
			f_spawn >> iTmp;  temp_spawn.slot_number = iTmp;
			f_spawn >> fTmp;  temp_spawn.pos.x       = fTmp;
			f_spawn >> fTmp;  temp_spawn.pos.y       = fTmp;
			f_spawn >> fTmp;  temp_spawn.pos.z       = fTmp;
			f_spawn >> cTmp;  temp_spawn.direction   = cTmp;
			f_spawn >> iTmp;  temp_spawn.money       = iTmp;
			f_spawn >> iTmp;  temp_spawn.skin        = iTmp;
			f_spawn >> iTmp;  temp_spawn.passage     = iTmp;
			f_spawn >> iTmp;  temp_spawn.content     = iTmp;
			f_spawn >> iTmp;  temp_spawn.lvl         = iTmp;
			f_spawn >> cTmp;  temp_spawn.status_bar  = cTmp;
			f_spawn >> cTmp;  temp_spawn.ghost       = cTmp;
			f_spawn >> sTmp;  temp_spawn.team        = sTmp;

			this->spawns.push_back(temp_spawn);
		}
	}

	f_spawn.close();
}


bool c_spawn_manager::save(string p_modname)
{
	return false;
}


void c_spawn_manager::add(c_spawn)
{

}


void c_spawn_manager::remove(int p_slot_number)
{

}


unsigned int c_spawn_manager::get_spawns_size()
{
	return this->spawns.size();
}


c_spawn c_spawn_manager::get_spawn(int p_slot_number)
{
	return this->spawns[p_slot_number];
}


void c_spawn_manager::set_spawn(int p_slot_number, c_spawn p_spawn)
{
	this->spawns[p_slot_number] = p_spawn;
}
