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
#include "math.h"
#include "global.h"

c_selection g_selection;
c_renderer  g_renderer;
c_mesh      g_mesh;
c_frustum   g_frustum;
vector<c_tile_definition> g_tiledict;


//---------------------------------------------------------------------
//-   Variables for mouse handling
//---------------------------------------------------------------------
int g_mouse_x;
int g_mouse_y;

float g_mouse_gl_x;
float g_mouse_gl_y;
float g_mouse_gl_z;


//---------------------------------------------------------------------
//-   Selection
//---------------------------------------------------------------------
int g_nearest_vertex;
extern c_selection g_selection;


//---------------------------------------------------------------------
//-   Calculate the distance between two points
//---------------------------------------------------------------------
float calculate_distance(vect3 start, vect3 end)
{
	float dist;
	dist = sqrtf( powf((start.x - end.x), 2.0f) +  powf((start.y - end.y), 2.0f) + powf((start.z - end.z), 2.0f) );

	return dist;
}


//---------------------------------------------------------------------
//-   Function to handle cleanup
//---------------------------------------------------------------------
void Quit()
{
	SDL_Quit( ); // clean up the window
	exit(-1);    // and exit appropriately
}


