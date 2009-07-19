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
/// @brief General stuff
/// @details Definition of stuff that doesn't fit anywhere else

#ifndef general_h
#define general_h
//---------------------------------------------------------------------
//-
//-   General definitions
//-
//---------------------------------------------------------------------


///   \def MAX
///        Helper function that retruns the bigger one of two values
///   \def MIN
///        Helper function that retruns the smaller one of two values
/* Neither Linux nor Mac OS X seem to have MIN and MAX defined, so if they
 * haven't already been found, define them here. */
#ifndef MAX
#define MAX(a,b) ( ((a)>(b))? (a):(b) )
#endif

#ifndef MIN
#define MIN(a,b) ( ((a)>(b))? (b):(a) )
#endif


//Visual Studio specific definitions
///   \def SPACE
///        Definition of the SPACE key for Visual Studio (req. MixedCase)
///   \def ESCAPE
///        Definition of the ESCAPE key for Visual Studio (req. MixedCase)
///   \def UP
///        Definition of the UP arrow key for Visual Studio (req. MixedCase)
///   \def DOWN
///        Definition of the DOWN arrow key for Visual Studio (req. MixedCase)
///   \def LEFT
///        Definition of the LEFT arrow key for Visual Studio (req. MixedCase)
///   \def RIGHT
///        Definition of the RIGHT arrow key for Visual Studio (req. MixedCase)
///   \def PAGE_UP
///        Definition of the PAGE_UP key for Visual Studio (req. MixedCase)
///   \def PAGE_DOWN
///        Definition of the PAGE_DOWN key for Visual Studio (req. MixedCase)
///   \def LEFT_SHIFT
///        Definition of the LEFT_SHIFT key for Visual Studio (req. MixedCase)
///   \def RIGHT_SHIFT
///        Definition of the RIGHT_SHIFT key for Visual Studio (req. MixedCase)
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
///   Vector and matrix definitions (stolen from egoboo_math.h)
///   \struct matrix_4x4
///           Matrix with a size 4x4
///   \union vector2_t
///           A vector with 2 coordinates
///   \union vector3_t
///           A vector with 3 coordinates
///   \union vector4_t
///           A vector with 4 coordinates
//---------------------------------------------------------------------
typedef struct matrix_4x4_t
{
	float v[16]; ///< Space
} matrix_4x4;

typedef matrix_4x4 GLmatrix;

typedef union vector2_t
{
	float _v[2]; ///< Space
	struct { float x, y; };
	struct { float u, v; };
	struct { float s, t; };
} vect2;

typedef union vector3_t
{
	float v[3]; ///< Space
	struct { float x, y, z; };
} vect3;

typedef union vector4_t
{
	float v[4]; ///< Space
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


//---------------------------------------------------------------------
///   Function to spawn left / right endian based on the OS
///   \param val Float value
///   \return (swapped) float value
//---------------------------------------------------------------------
float SwapLE_float( float val )
{
	FCONVERT convert;

	convert.f = val;
	convert.i = SDL_SwapLE32(convert.i);

	return convert.f;
};

#endif // SDL_BYTEORDER
#endif // general_h
