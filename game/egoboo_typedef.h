#pragma once

//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egoboo_typedef.h
/// @details some basic types that are used throughout the game code.

#include "egoboo_config.h"

#include <SDL_types.h>

// include these before the memory stuff because the Fluid Studios Memory manager
// redefines the new operator in a way that ther STL doesn't like.
// And we do not want mmgr to be tracking internal allocation inside the STL, anyway!
#if defined(__cplusplus)

#if defined(_H_MMGR_INCLUDED)
#error If mmgr.h is included before this point, the remapping of new and delete will cause problems
#endif

#   if defined(USE_HASH)
#       include <hash_map>
#       define EGOBOO_MAP stdext::hash_map
#       include <hash_set>
#       define EGOBOO_SET stdext::hash_set
#   else
#       include <map>
#       define EGOBOO_MAP std::map
#       include <set>
#       define EGOBOO_SET std::set
#   endif

#   include <deque>
#   include <stack>
#   include <queue>
#   include <exception>
#   include <algorithm>
#   include <functional>
#   include <vector>

#endif

#include "egoboo_mem.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// portable definition of assert. the c++ version can be activated below.

#include <assert.h>

#define C_EGOBOO_ASSERT(X) assert(X)

//--------------------------------------------------------------------------------------------
// BOOLEAN

// we must have the same definition in c and c++ otherwise,
// structs with items of type bool will be different sizes
// in different modules!
    typedef unsigned char bool_t;
#define btrue  1
#define bfalse 0

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// special return values
    enum e_egoboo_rv
    {
        rv_error   = -1,
        rv_fail    = bfalse,
        rv_success = btrue
    };
    typedef enum e_egoboo_rv egoboo_rv;

//--------------------------------------------------------------------------------------------
// 24.8 fixed point types

    typedef Uint32 UFP8_T;
    typedef Sint32 SFP8_T;

// unsigned versions
#define UFP8_TO_FLOAT(V1)  ( ((float)((unsigned)(V1))) * INV_0100 )
#define FLOAT_TO_UFP8(V1)  ( (unsigned)( ((unsigned)(V1)) * (float)(0x0100) ) )
#define UFP8_TO_UINT(V1)   ( ((unsigned)(V1)) >> 8 )                           ///< fast version of V1 / 256
#define UINT_TO_UFP8(V1)   ( ((unsigned)(V1)) << 8 )                           ///< fast version of V1 * 256

// signed versions
#define SFP8_TO_FLOAT(V1)  ( ((float)((signed)(V1))) * INV_0100 )
#define FLOAT_TO_SFP8(V1)  ( (signed)( ((signed)(V1)) * (float)(0x0100) ) )
#define SFP8_TO_SINT(V1)   ( (V1) < 0 ? -((signed)UFP8_TO_UINT(-V1)) : (signed)UFP8_TO_UINT(V1) )
#define SINT_TO_SFP8(V1)   ( (V1) < 0 ? -((signed)UINT_TO_UFP8(-V1)) : (signed)UINT_TO_UFP8(V1) )

//--------------------------------------------------------------------------------------------
// the type for the 16-bit value used to stor angles
    typedef Uint16   FACING_T;
    typedef FACING_T TURN_T;

#define TO_FACING(X) ((FACING_T)(X))
#define TO_TURN(X)   ((TURN_T)((TO_FACING(X)>>2) & TRIG_TABLE_MASK))

//--------------------------------------------------------------------------------------------
// 16.16 fixed point types

    typedef Uint32 UFP16_T;
    typedef Sint32 SFP16_T;

#define FLOAT_TO_FP16( V1 )  ( (Uint32)((V1) * 0x00010000) )
#define FP16_TO_FLOAT( V1 )  ( (float )((V1) * 0.0000152587890625f ) )

//--------------------------------------------------------------------------------------------
// BIT FIELDS
    typedef Uint32 BIT_FIELD;                                ///< A big string supporting 32 bits
#define FULL_BIT_FIELD        ((BIT_FIELD)(~0))            ///< A bit string where all bits are flagged as 1
#define EMPTY_BIT_FIELD         0                            ///< A bit string where all bits are flagged as 0
#define FILL_BIT_FIELD(XX)    (XX) = FULL_BIT_FIELD        ///< Fills up all bits in a bit pattern
#define CLEAR_BIT_FIELD(XX) (XX) = 0                    ///< Resets all bits in a BIT_FIELD to 0

#if !defined(ADD_BITS)
#define ADD_BITS(XX, YY) (XX) |= (YY)
#endif

#if !defined(REMOVE_BITS)
#define REMOVE_BITS(XX, YY) (XX) &= ~(YY)
#endif

#if !defined(BOOL_TO_BIT)
#    define BOOL_TO_BIT(X)       ((X) ? 1 : 0 )
#endif

#if !defined(BIT_TO_BOOL)
#    define BIT_TO_BOOL(X)       ((1 == X) ? btrue : bfalse )
#endif

#if !defined(HAS_SOME_BITS)
#    define HAS_SOME_BITS(XX,YY) (0 != ((XX)&(YY)))
#endif

#if !defined(HAS_ALL_BITS)
#    define HAS_ALL_BITS(XX,YY)  ((YY) == ((XX)&(YY)))
#endif

#if !defined(HAS_NO_BITS)
#    define HAS_NO_BITS(XX,YY)   (0 == ((XX)&(YY)))
#endif

#if !defined(MISSING_BITS)
#    define MISSING_BITS(XX,YY)  (HAS_SOME_BITS(XX,YY) && !HAS_ALL_BITS(XX,YY))
#endif

#define CLIP_TO_08BITS( V1 )  ( (V1) & 0xFF       )
#define CLIP_TO_16BITS( V1 )  ( (V1) & 0xFFFF     )
#define CLIP_TO_24BITS( V1 )  ( (V1) & 0xFFFFFF   )
#define CLIP_TO_32BITS( V1 )  ( (V1) & 0xFFFFFFFF )

//--------------------------------------------------------------------------------------------
// RECTANGLE
    struct s_irect
    {
        float x, y;
        float w, h;
    };
    typedef struct s_irect irect_t;

    struct s_frect
    {
        float x, y;
        float w, h;
    };
    typedef struct s_frect frect_t;

    bool_t frect_union( frect_t * src1, frect_t * src2, frect_t * dst );

//--------------------------------------------------------------------------------------------
// Rectangle types

    struct s_ego_irect
    {
        int xmin, ymin;
        int xmax, ymax;
    };
    typedef struct s_ego_irect ego_irect_t;

    bool_t irect_point_inside( ego_irect_t * prect, int   ix, int   iy );

    struct s_ego_frect
    {
        float xmin, ymin;
        float xmax, ymax;
    };
    typedef struct s_ego_frect ego_frect_t;

    bool_t frect_point_inside( ego_frect_t * prect, float fx, float fy );

//--------------------------------------------------------------------------------------------
// PAIR AND RANGE

// Specifies a value between "base" and "base + rand"
    struct s_pair
    {
        int base, rand;
    };
    typedef struct s_pair IPair;

// Specifies a value from "from" to "to"
    struct s_range
    {
        float from, to;
    };
    typedef struct s_range FRange;

    void pair_to_range( IPair pair, FRange * prange );
    void range_to_pair( FRange range, IPair * ppair );

    void ints_to_range( int base, int rand, FRange * prange );
    void floats_to_pair( float vmin, float vmax, IPair * ppair );

//--------------------------------------------------------------------------------------------
// IDSZ
    typedef Uint32 IDSZ;

#define IDSZ_DEFINED

#if !defined(MAKE_IDSZ)
#define MAKE_IDSZ(C0,C1,C2,C3)     \
    ((IDSZ)(                       \
                                   ((((C0)-'A')&0x1F) << 15) |       \
                                   ((((C1)-'A')&0x1F) << 10) |       \
                                   ((((C2)-'A')&0x1F) <<  5) |       \
                                   ((((C3)-'A')&0x1F) <<  0)         \
           ))
#endif

#define IDSZ_NONE            MAKE_IDSZ('N','O','N','E')       ///< [NONE]
#define IDSZ_BOOK            MAKE_IDSZ('B','O','O','K')       ///< [BOOK]

    const char * undo_idsz( IDSZ idsz );

//--------------------------------------------------------------------------------------------
// STRING
    typedef char STRING[256];

//--------------------------------------------------------------------------------------------

/// the "base class" of Egoboo profiles
#define  EGO_PROFILE_STUFF \
    bool_t         loaded;      /* Does it exist? */ \
    STRING         name

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The basic 2d latch
    struct s_latch_2d
    {
        float     dir[2];
        BIT_FIELD b;         ///< the raw bits corresponding to various buttons
    };
    typedef struct s_latch_2d latch_2d_t;

#define LATCH_2D_INIT { {0.0f,0.0f}, EMPTY_BIT_FIELD }

/// The basic 3d latch
    struct s_latch_3d
    {
        float     dir[3];
        BIT_FIELD b;         ///< the raw bits corresponding to various buttons
    };
    typedef struct s_latch_3d latch_3d_t;

#define LATCH_3D_INIT { {0.0f,0.0f,0.0f}, EMPTY_BIT_FIELD }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The latch used by the input system
    struct s_latch_input
    {
        float     raw[2];    ///< the raw position of the "joystick axes"
        BIT_FIELD b;         ///< the raw state of the "joystick buttons"

        float     dir[3];    ///< the translated direction relative to the camera
    };
    typedef struct s_latch_input latch_input_t;

    void latch_input_init( latch_input_t * platch );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// References

/// base reference type
    typedef unsigned REF_T;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple list structure that tracks free elements

#define INVALID_UPDATE_GUID unsigned(~unsigned(0))

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// place this include here so that the REF_T is defined for egoboo_typedef_cpp.h

#if defined(__cplusplus)
}
#include "egoboo_typedef_cpp.h"

extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// definitions for the compiler environment

#if defined(__cplusplus)

#   define EGOBOO_ASSERT(X) CPP_EGOBOO_ASSERT(X)

#   define _EGOBOO_ASSERT(X) C_EGOBOO_ASSERT(X)

#else

#   define EGOBOO_ASSERT(X) C_EGOBOO_ASSERT(X)

#   define _EGOBOO_ASSERT(X) C_EGOBOO_ASSERT(X)

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// implementation of forward declaration of references

#if defined(__cplusplus)

    typedef struct s_oglx_texture oglx_texture_t;

    struct ego_cap;
    struct ego_obj_chr;
    struct ego_team;
    struct ego_eve;
    struct ego_obj_enc;
    struct ego_mad;
    struct ego_player;
    struct ego_pip;
    struct ego_obj_prt;
    struct ego_passage;
    struct ego_shop;
    struct ego_pro;
    struct s_oglx_texture;
    struct ego_billboard_data;
    struct snd_looped_sound_data;
    struct mnu_module;
    struct ego_tx_request;

    typedef t_reference<ego_cap>               CAP_REF;
    typedef t_reference<ego_obj_chr>           CHR_REF;
    typedef t_reference<ego_team>              TEAM_REF;
    typedef t_reference<ego_eve>               EVE_REF;
    typedef t_reference<ego_obj_enc>           ENC_REF;
    typedef t_reference<ego_mad>               MAD_REF;
    typedef t_reference<ego_player>            PLA_REF;
    typedef t_reference<ego_pip>               PIP_REF;
    typedef t_reference<ego_obj_prt>           PRT_REF;
    typedef t_reference<ego_passage>           PASS_REF;
    typedef t_reference<ego_shop>              SHOP_REF;
    typedef t_reference<ego_pro>               PRO_REF;
    typedef t_reference<oglx_texture_t>        TX_REF;
    typedef t_reference<ego_billboard_data>    BBOARD_REF;
    typedef t_reference<snd_looped_sound_data> LOOP_REF;
    typedef t_reference<mnu_module>            MOD_REF;
    typedef t_reference<MOD_REF>               MOD_REF_REF;
    typedef t_reference<ego_tx_request>        TREQ_REF;

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// forward declaration of standard dynamic array types
#if defined(__cplusplus)

    typedef t_dary<  char >  char_ary;
    typedef t_dary< short >  short_ary;
    typedef t_dary<   int >  int_ary;
    typedef t_dary< float >  float_ary;
    typedef t_dary<double >  double_ary;

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

#define _egoboo_typedef_h

