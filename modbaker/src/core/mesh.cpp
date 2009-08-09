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
#include "mesh.h"
#include "../utility.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#define MAPID 0x4470614d


//---------------------------------------------------------------------
///   Load the .mpd file
///   \param p_filename path + filename of the file to load
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_mesh::load_mesh_mpd(string p_filename)
{
	cout << "in load_mesh_mpd()" << endl;
	// Temp. variables
	int itmp;
	float ftmp;
	int cnt;
	int vert, vrt;

	vector<vect3> vertices;
	vect3 tmp_vec;
	tmp_vec.x = 0;
	tmp_vec.y = 0;
	tmp_vec.z = 0;

	c_tile tmp_tile;

	ifstream file;

	// FAN data
	Uint32 fan = 0;

	cout << "opening the file" << endl;
	// Open the file
	file.open(p_filename.c_str(), ios::binary);

	if (!file)
	{
		cout << "File not found " << p_filename << endl;
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

	int tmp_vert_count, tmp_tiles_x, tmp_tiles_y, tmp_tile_count;

	file.read((char*)&itmp, 4); tmp_vert_count = SDL_SwapLE32(itmp); // Read number of verts
	file.read((char*)&itmp, 4); tmp_tiles_x    = SDL_SwapLE32(itmp); // Read map x data
	file.read((char*)&itmp, 4); tmp_tiles_y    = SDL_SwapLE32(itmp); // Read map y data


	//-----------------------------------------------------------------
	//- Fill the mpd structure
	//-----------------------------------------------------------------
	// Store the number of tiles
	tmp_tile_count = tmp_tiles_x * tmp_tiles_y;

	cout << "reallocating the mesh memory" << endl;
	this->setup_mesh_mem(tmp_vert_count, tmp_tile_count);

	this->vrt_count  = tmp_vert_count;
	this->tiles_x    = tmp_tiles_x;
	this->tiles_y    = tmp_tiles_y;
	this->tile_count = tmp_tile_count;

	vertices.resize(tmp_vert_count, tmp_vec);
	cout << "...done." << endl;


	/// \todo Segfault on Windows
	cout << "getting the basic fan data" << endl;
	//-----------------------------------------------------------------
	//- Get the basic fan data
	//-----------------------------------------------------------------
	for (fan = 0; fan < this->tile_count; fan++)
	{
		file.read((char*)&itmp, 4);
		itmp = SDL_SwapLE32( itmp );

		tmp_tile.type = itmp >> 24;
		tmp_tile.fx   = itmp >> 16;
		tmp_tile.tile = itmp;
		tmp_tile.vert_count = this->tile_definitions[tmp_tile.type].vrt_count;

		this->tiles.push_back(tmp_tile);

		this->tiles[fan].vertices.reserve(tmp_tile.vert_count);
	}
	cout << "...done." << endl;


	cout << "Reading the twist" << endl;
	//-----------------------------------------------------------------
	//- Read the twist data from the FAN
	//-----------------------------------------------------------------
	for (fan = 0; fan < this->tile_count; fan++)
	{
		file.read((char*)&itmp, 1);
		this->tiles[fan].twist = itmp;
	}
	cout << "...done." << endl;


	cout << "Reading the X, Y, Z data" << endl;
	//-----------------------------------------------------------------
	//- Read vertex x, y and z data
	//-----------------------------------------------------------------
	// Load vertex fan_x data
	for (cnt = 0; cnt < this->vrt_count; cnt++)
	{
		file.read((char*)&ftmp, 4);
		vertices[cnt].x = SwapLE_float(ftmp);
	}


	// Load vertex fan_y data
	for (cnt = 0; cnt < this->vrt_count; cnt++)
	{
		file.read((char*)&ftmp, 4);
		vertices[cnt].y = SwapLE_float(ftmp);
	}

	// Load vertex fan_z data
	cnt = 0;
	for (cnt = 0; cnt < this->vrt_count; cnt++)
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
		for (cnt = 0; cnt < this->vrt_count; cnt++)
		{
			file.read((char*)&itmp, 1);

			this->vrt_ar_fp8[cnt] = 0; // itmp;
			this->vrt_ag_fp8[cnt] = 0; // itmp;
			this->vrt_ab_fp8[cnt] = 0; // itmp;

			this->vrt_lr_fp8[cnt] = 0;
			this->vrt_lg_fp8[cnt] = 0;
			this->vrt_lb_fp8[cnt] = 0;
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
	for (cnt = 0; cnt < this->vrt_count; cnt++)
	{
		// Did we reach the vrt_count for the current tile?
		if (cnt_vert >= this->tile_definitions[this->tiles[cnt_tile].type].vrt_count)
		{
			cnt_tile++;
			cnt_vert = 0;
			this->tiles[cnt_tile].vrt_start = cnt;
		}

		this->tiles[cnt_tile].vertices.push_back(vertices[cnt]);
		cnt_vert++;
	}


	cout << "...done." << endl;


	cout << "build_vert_lookup_table()" << endl;
	this->build_vert_lookup_table();
	cout << "...done." << endl;

	cout << "build_tile_lookup_table()" << endl;
	this->build_tile_lookup_table();
	cout << "...done." << endl;


	//-----------------------------------------------------------------
	//-
	//-----------------------------------------------------------------
	vert = 0;

	cout << "setting up the tile dictionary" << endl;
	for (fan = 0; fan < this->tile_count; fan++)
	{
		int vrtcount = this->tile_definitions[this->tiles[fan].type].vrt_count;
		int vrtstart = vert;

		this->tiles[fan].vrt_start = vrtstart;

		this->tiles[fan].bbox.mins.x = this->tiles[fan].bbox.maxs.x = vertices[vrtstart].x;
		this->tiles[fan].bbox.mins.y = this->tiles[fan].bbox.maxs.y = vertices[vrtstart].y;
		this->tiles[fan].bbox.mins.z = this->tiles[fan].bbox.maxs.z = vertices[vrtstart].z;


		for (vrt = vrtstart + 1; vrt < vrtstart + vrtcount; vrt++)
		{
			this->tiles[fan].bbox.mins.x = MIN( this->tiles[fan].bbox.mins.x, vertices[vrt].x );
			this->tiles[fan].bbox.mins.y = MIN( this->tiles[fan].bbox.mins.y, vertices[vrt].y );
			this->tiles[fan].bbox.mins.z = MIN( this->tiles[fan].bbox.mins.z, vertices[vrt].z );

			this->tiles[fan].bbox.maxs.x = MAX( this->tiles[fan].bbox.maxs.x, vertices[vrt].x );
			this->tiles[fan].bbox.maxs.y = MAX( this->tiles[fan].bbox.maxs.y, vertices[vrt].y );
			this->tiles[fan].bbox.maxs.z = MAX( this->tiles[fan].bbox.maxs.z, vertices[vrt].z );
		}

		vert += vrtcount;
	}

	cout << "Leaving load_mesh_mpd()" << endl;

	return true;
}


//---------------------------------------------------------------------
///   Modify all verts at the position of the selected vertex (X axis)
///   \param p_modifier x modifier
///   \param p_vert vertex to modify
///   \return number of modified vertices
//---------------------------------------------------------------------
int c_mesh::modify_verts_x(float p_modifier, int p_vert)
{
	int i;
	int num_modified;

	num_modified = 0;

	vect3 pos_old, pos_test;

	pos_old = this->get_vert(p_vert);

	for (i = 0; i < this->vrt_count; i++)
	{
		pos_test = this->get_vert(i);
		if ((pos_test.x == pos_old.x) &&
			(pos_test.y == pos_old.y) &&
			(pos_test.z == pos_old.z))
		{
			this->modify_verts_x(p_modifier, i);

			num_modified++;
		}
	}

	return num_modified;
}


//---------------------------------------------------------------------
///   Modify all verts at the position of the selected vertex (Y axis)
///   \param p_modifier y modifier
///   \param p_vert vertex to modify
///   \return number of modified vertices
//---------------------------------------------------------------------
int c_mesh::modify_verts_y(float p_modifier, int p_vert)
{
	int i;
	int num_modified;

	num_modified = 0;

	vect3 pos_old, pos_test;

	pos_old = this->get_vert(p_vert);

	for (i = 0; i < this->vrt_count; i++)
	{
		pos_test = this->get_vert(i);
		if ((pos_test.x == pos_old.x) &&
			(pos_test.y == pos_old.y) &&
			(pos_test.z == pos_old.z))
		{
			this->modify_verts_y(p_modifier, i);

			num_modified++;
		}
	}

	return num_modified;
}


//---------------------------------------------------------------------
///   Modify all verts at the position of the selected vertex (Z axis)
///   \param p_modifier z modifier
///   \param p_vert vertex to modify
///   \return number of modified vertices
//---------------------------------------------------------------------
int c_mesh::modify_verts_z(float p_modifier, int p_vert)
{
	int i;
	int num_modified;

	num_modified = 0;

	vect3 pos_old, pos_test;

	pos_old = this->get_vert(p_vert);

	for (i = 0; i < this->vrt_count; i++)
	{
		pos_test = this->get_vert(i);
		if ((pos_test.x == pos_old.x) &&
			(pos_test.y == pos_old.y) &&
			(pos_test.z == pos_old.z))
		{
			this->modify_verts_z(p_modifier, i);

			num_modified++;
		}
	}

	return num_modified;
}


//---------------------------------------------------------------------
///   Get the nearest vertex
///   \param pos_x reference x position
///   \param pos_y reference y position
///   \param pos_z reference z position
///   \return number of the nearest vertex
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

	for (i = 0; i < this->vrt_count; i++)
	{
		dist_temp = calculate_distance(this->get_vert(i), ref);

		if (dist_temp < dist_nearest)
		{
			dist_nearest = calculate_distance(this->get_vert(i), ref);

			nearest_vertex = i;
		}
	}

	return nearest_vertex;
}


//---------------------------------------------------------------------
///   Get the nearest tile
///   \param pos_x reference x position
///   \param pos_y reference y position
///   \param pos_z reference z position
///   \return number of the nearest tile
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

	for (i = 0; i < (int)this->tile_count; i++)
	{
		vert_start = this->tiles[i].vrt_start;

		dist_temp = calculate_distance(this->get_vert(vert_start), ref);

		if (dist_temp < dist_nearest)
		{
			dist_nearest = calculate_distance(this->get_vert(vert_start), ref);

			nearest_tile = i;
		}
	}

	return nearest_tile;
}


//---------------------------------------------------------------------
///   Save the .mpd file
///   \param filename path + filename to save
///   \return true on success, else false
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

	vertices.reserve(this->vrt_count);

	// Go through all tiles
	for (cnt_tile = 0; cnt_tile < (int)this->tile_count; cnt_tile++)
	{
		// Go through all vertices
		for (cnt_vert = 0; cnt_vert < this->tiles[cnt_tile].vert_count; cnt_vert++)
		{
			vertices[cnt] = this->tiles[cnt_tile].vertices[cnt_vert];
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

	itmp = this->vrt_count;
	file.write((char *)&itmp, 4);

	itmp = this->tiles_x;
	file.write((char *)&itmp, 4);

	itmp = this->tiles_y;
	file.write((char *)&itmp, 4);


	//-----------------------------------------------------------------
	//- Fill the mpd structure
	//-----------------------------------------------------------------
	for ( fan = 0; fan < this->tile_count;  fan++)
	{
		itmp = (this->tiles[fan].type << 24) + (this->tiles[fan].fx << 16) + this->tiles[fan].tile;
		file.write((char *)&itmp, 4);
	}


	//-----------------------------------------------------------------
	//- Read the twist data from the FAN
	//-----------------------------------------------------------------
	for (fan = 0; fan < this->tile_count; fan++)
	{
		itmp = this->tiles[fan].twist;
		file.write((char *)&itmp, 1);
	}


	//-----------------------------------------------------------------
	//- Read vertex x,y and z data
	//-----------------------------------------------------------------
	// Load vertex fan_x data
	for (cnt = 0;  cnt < this->vrt_count; cnt++)
	{
		ftmp = vertices[cnt].x;
		file.write((char *)&ftmp, 4);
	}


	// Load vertex fan_y data
	for (cnt = 0; cnt < this->vrt_count; cnt++)
	{
		ftmp = vertices[cnt].y;
		file.write((char *)&ftmp, 4);
	}

	// Load vertex z data
	cnt = 0;
	for (cnt = 0; cnt < this->vrt_count; cnt++)
	{
		ftmp = vertices[cnt].z * 16;
		file.write((char *)&ftmp, 4);
		// Cartman uses 4 bit fixed point for Z
	}


	//-----------------------------------------------------------------
	//- Load vertex lighting data
	//-----------------------------------------------------------------
	for (cnt = 0; cnt < this->vrt_count; cnt++)
	{
		itmp = 0;
		file.write((char *)&itmp, 1);
	}

	file.close();

	cout << "File saved: " << filename << endl;

	return true;
}


//---------------------------------------------------------------------
///   Init the meshfx vector
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
///   Get the Mesh FX based on the vector entry
///   \param p_entry entry to get
///   \return mesh FX
//---------------------------------------------------------------------
int c_mesh::get_meshfx(int p_entry)
{
	return this->m_meshfx[p_entry];
}


//---------------------------------------------------------------------
///   Get all vertices of the fan
///   \param p_fan fan to look for
///   \return vector of vertices found
//---------------------------------------------------------------------
vector<int> c_mesh::get_fan_vertices(int p_fan)
{
	int cnt;
	vector<int> vec_vertices;

	Uint16 type      = this->tiles[p_fan].type; // Command type ( index to points in fan )
	int vertices     = this->tile_definitions[type].vrt_count;
	Uint32 badvertex = this->tiles[p_fan].vrt_start;   // Actual vertex number

	for (cnt = 0; cnt < vertices; cnt++, badvertex++)
	{
		vec_vertices.push_back(badvertex);
	}
	return vec_vertices;
}


//---------------------------------------------------------------------
///   Change the size of the mesh mem
///   \param p_new_vertices new number of vertices
///   \param p_new_fans new number of fans
///   \return true on success, else false
///   \todo implement everything
//---------------------------------------------------------------------
bool c_mesh::change_mem_size(int p_new_vertices, int p_new_fans)
{
	// TODO: Change the size of the mesh mem

	this->vrt_count += 2;
	return true;
}


//---------------------------------------------------------------------
///   c_mesh constructor
//---------------------------------------------------------------------
c_mesh::c_mesh()
{
	this->setup_mesh_mem(0, 0);
	this->init_meshfx();
}


//---------------------------------------------------------------------
///   c_mesh destructor
//---------------------------------------------------------------------
c_mesh::~c_mesh()
{
}


//---------------------------------------------------------------------
///   Create a new mesh
///   \param p_tile_x mesh width in tiles (x axis)
///   \param p_tile_y mesh height in tiles (y axis)
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_mesh::new_mesh_mpd(int p_tiles_x, int p_tiles_y)
{
	cout << "Creating a new mesh with size " << p_tiles_x << " * " << p_tiles_y << endl;

	this->destroy_mesh_mem();

	// Temp. variables
//	int cnt;
//	int vert, vrt;

	c_tile tmp_tile;


	// FAN data
	Uint32 fan = 0;




	this->tiles_x    = p_tiles_x;
	this->tiles_y    = p_tiles_y;
	this->tile_count = this->tiles_x * this->tiles_y;

	// Every tile is a floor
	int tmp_vert_count;
	tmp_vert_count = this->tile_count * this->tile_definitions[TTYPE_TWO_FACED_GROUND1].vrt_count;

	this->setup_mesh_mem(tmp_vert_count, this->tile_count);
	this->vrt_count = this->vrt_count;

	this->tiles.clear();

	cout << "getting the basic fan data" << endl;
	//-----------------------------------------------------------------
	//- Get the basic fan data
	//-----------------------------------------------------------------
	for (fan = 0; fan < this->tile_count; fan++)
	{
		tmp_tile.type  = TTYPE_TWO_FACED_GROUND1;
		tmp_tile.fx    = MESHFX_REF;
		tmp_tile.tile  = 0; /// \todo replace the .tile dummy values
		tmp_tile.twist = 0; /// \todo replace the .twist dummy value

		tmp_tile.vert_count = this->tile_definitions[tmp_tile.type].vrt_count;

		this->tiles.push_back(tmp_tile);
	}
	cout << "all tiles set up" << endl;


	cout << "Setting up the vertices" << endl;
	//-----------------------------------------------------------------
	//- Read vertex x, y and z data
	//-----------------------------------------------------------------

	unsigned int tmp_x, tmp_y, cnt_tile, cnt_vert;

	vect3 tmp_vert;

	cnt_vert = 0;

	// Loop through all tiles
	for (tmp_y = 0; tmp_y < (unsigned int)this->tiles_y; tmp_y++)
	{
		for (tmp_x = 0; tmp_x < (unsigned int)this->tiles_x; tmp_x++)
		{
			cnt_tile = (tmp_y * this->tiles_x) + tmp_x;

			tmp_tile = this->tiles[cnt_tile];

			tmp_tile.vrt_start = cnt_vert;

			/// \todo calculate wall / floor etc.
			// Loop trough all vertices
/*			for (tmp_v = 0; tmp_v < this->tiles[tmp_file].vert_count; tmp_v++)
			{
				this->tiles[tmp_file].vertices[tmp_v].x = tmp_x;
				this->tiles[tmp_file].vertices[tmp_v].y = tmp_y;
				cnt_vert++;
			}
*/

			tmp_vert.z = 270.0;
			tmp_vert.x = 0.0;
			tmp_vert.y = 0.0;

			int new_size;
			new_size = (int)tmp_tile.vert_count;

			tmp_tile.vertices.clear();

			tmp_vert.x = (float)tmp_x     * 128.0;
			tmp_vert.y = (float)tmp_y     * 128.0;

			tmp_tile.vertices.push_back(tmp_vert);

			tmp_vert.x = (float)(tmp_x+1) * 128.0;
			tmp_vert.y = (float)tmp_y     * 128.0;

			tmp_tile.vertices.push_back(tmp_vert);

			tmp_vert.x = (float)(tmp_x+1) * 128.0;
			tmp_vert.y = (float)(tmp_y+1) * 128.0;

			tmp_tile.vertices.push_back(tmp_vert);

			tmp_vert.x = (float)tmp_x     * 128.0;
			tmp_vert.y = (float)(tmp_y+1) * 128.0;

			tmp_tile.vertices.push_back(tmp_vert);

			this->tiles[cnt_tile] = tmp_tile;

			cnt_vert += 4;
		}
	}
	cout << "...done." << endl;


	//-----------------------------------------------------------------
	//- Load vertex lighting data
	//-----------------------------------------------------------------
	/*
		for (cnt = 0; cnt < this->vrt_count; cnt++)
		{
			infile.read((char*)&itmp,1);

			this->vrt_ar_fp8[cnt] = 0; // itmp;
			this->vrt_ag_fp8[cnt] = 0; // itmp;
			this->vrt_ab_fp8[cnt] = 0; // itmp;

			this->vrt_lr_fp8[cnt] = 0;
			this->vrt_lg_fp8[cnt] = 0;
			this->vrt_lb_fp8[cnt] = 0;
		}
	*/


	cout << "build_vert_lookup_table()" << endl;
	this->build_vert_lookup_table();
	cout << "...done." << endl;

	cout << "build_tile_lookup_table()" << endl;
	this->build_tile_lookup_table();
	cout << "...done." << endl;

/*
	//-----------------------------------------------------------------
	//-
	//-----------------------------------------------------------------
	vert = 0;

	cout << "setting up the tile dictionary" << endl;
	for (fan = 0; fan < mem->tile_count; fan++)
	{
		int vrtcount = c_mesh::ms_tiledict[this->tiles[fan].type].vrt_count;
		int vrtstart = vert;

		this->tiles[fan].vrt_start = vrtstart;

		this->tiles[fan].bbox.mins.x = this->tiles[fan].bbox.maxs.x = vertices[vrtstart].x;
		this->tiles[fan].bbox.mins.y = this->tiles[fan].bbox.maxs.y = vertices[vrtstart].y;
		this->tiles[fan].bbox.mins.z = this->tiles[fan].bbox.maxs.z = vertices[vrtstart].z;


		for (vrt = vrtstart + 1; vrt < vrtstart + vrtcount; vrt++)
		{
			this->tiles[fan].bbox.mins.x = MIN( this->tiles[fan].bbox.mins.x, vertices[vrt].x );
			this->tiles[fan].bbox.mins.y = MIN( this->tiles[fan].bbox.mins.y, vertices[vrt].y );
			this->tiles[fan].bbox.mins.z = MIN( this->tiles[fan].bbox.mins.z, vertices[vrt].z );

			this->tiles[fan].bbox.maxs.x = MAX( this->tiles[fan].bbox.maxs.x, vertices[vrt].x );
			this->tiles[fan].bbox.maxs.y = MAX( this->tiles[fan].bbox.maxs.y, vertices[vrt].y );
			this->tiles[fan].bbox.maxs.z = MAX( this->tiles[fan].bbox.maxs.z, vertices[vrt].z );
		}

		vert += vrtcount;
	}
*/

	cout << "Leaving new_mesh_mpd()" << endl;

	return true;
}



//---------------------------------------------------------------------
///   Build the renderlist
///   \return true on success, else false
//---------------------------------------------------------------------
bool c_mesh::build_renderlist()
{
	Uint32 fan;

	// Reset the values
	this->m_num_totl  = 0;
	this->m_num_shine = 0;
	this->m_num_reflc = 0;
	this->m_num_norm  = 0;
	this->m_num_watr  = 0;

	for (fan = 0; fan < this->tile_count; fan++)
	{
		this->tiles[fan].inrenderlist = false;

		if (this->m_num_totl < MAXMESHRENDER)
		{
			bool is_shine, is_noref, is_norml, is_water;

			this->tiles[fan].inrenderlist = true;

//			is_shine = mesh_has_all_bits(mm->tilelst, fan, MPDFX_SHINY);
//			is_noref = mesh_has_all_bits(mm->tilelst, fan, MPDFX_NOREFLECT);
//			is_norml = !is_shine;
//			is_water = mesh_has_some_bits(mm->tilelst, fan, MPDFX_WATER);

			is_norml = true; // TODO: Added by xenom
			is_noref = false;
			is_shine = false;
			is_water = false;

			// Put each tile in basic list
			this->m_totl[m_num_totl] = fan;
			this->m_num_totl++;
//			mesh_fan_add_renderlist(this->tilelst, fan);

			// Put each tile
			if (!is_noref)
			{
				this->m_reflc[m_num_reflc] = fan;
				this->m_num_reflc++;
			}

			if (is_shine)
			{
				this->m_shine[m_num_shine] = fan;
				this->m_num_shine++;
			}

			if (is_norml)
			{
				this->m_norm[m_num_norm] = fan;
				this->m_num_norm++;
			}

			if (is_water)
			{
				// precalculate the "mode" variable so that we don't waste time rendering the waves
				int tx, ty;

				ty = fan / this->tiles_x;
				tx = fan % this->tiles_x;

				this->m_watr_mode[m_num_watr] = ((ty & 1) << 1) + (tx & 1);
				this->m_watr[m_num_watr]      = fan;

				this->m_num_watr++;
			}
		}
	}

	return true;
}
