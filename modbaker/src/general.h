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
#ifndef general_h
#define general_h
//---------------------------------------------------------------------
//-
//-   General definitions
//-
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//-   egoboo_types.h - Some helper functions
//---------------------------------------------------------------------
#define EGOBOO_NEW_ARY( TYPE, COUNT ) new TYPE [ COUNT ]
#define EGOBOO_DELETE_ARY(PTR) if(NULL != PTR) { delete [] PTR; PTR = NULL; }


/* Neither Linux nor Mac OS X seem to have MIN and MAX defined, so if they
 * haven't already been found, define them here. */
#ifndef MAX
#define MAX(a,b) ( ((a)>(b))? (a):(b) )
#endif

#ifndef MIN
#define MIN(a,b) ( ((a)>(b))? (b):(a) )
#endif

//Visual Studio specific defenitions
#if defined (_MSC_VER)
#define SPACE		Space
#define ESCAPE		Escape
#define UP			Up
#define DOWN		Down
#define LEFT		Left
#define RIGHT		Right
#define PAGE_UP		PageUp
#define PAGE_DOWN	PageDown
#define LEFT_SHIFT	LeftShift
#define RIGHT_SHIFT	RightShift
#endif


//---------------------------------------------------------------------
//-   egoboo_math.h - Vector definitions
//---------------------------------------------------------------------
typedef struct matrix_4x4_t
{
	float v[16];
} matrix_4x4;

typedef matrix_4x4 GLmatrix;

typedef union vector2_t
{
	float _v[2];
	struct { float x, y; };
	struct { float u, v; };
	struct { float s, t; };
} vect2;

typedef union vector3_t
{
	float v[3];
	struct { float x, y, z; };
} vect3;

typedef union vector4_t
{
	float v[4];
	struct { float x, y, z, w; };
	struct { float r, g, b, a; };
} vect4;


//---------------------------------------------------------------------
//-   egoboo_types.h - Swap the byteorder on OTHER systems
//---------------------------------------------------------------------
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
float SwapLE_float( float val );
#endif

#if SDL_BYTEORDER == SDL_LIL_ENDIAN

#    define SwapLE_float

#else

float SwapLE_float( float val )
{
	FCONVERT convert;

	convert.f = val;
	convert.i = SDL_SwapLE32(convert.i);

	return convert.f;
};

#endif // SDL_BYTEORDER
#endif // general_h
