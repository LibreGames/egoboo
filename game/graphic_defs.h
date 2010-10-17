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

#include "egoboo_typedef.h"
#include "extensions/ogl_include.h"

#if defined(__cplusplus)
extern "C" {
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_ego_config_data;
struct s_oglx_texture_parameters;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define DOLIST_SIZE (MAX_CHR + TOTAL_MAX_PRT)

#define MAXMESHRENDER             1024                       ///< Max number of tiles to draw

#define MAPSIZE 96

#define TABX                            32// 16      ///< Size of little name tag on the bar
#define BARX                            112// 216         ///< Size of bar
#define BARY                            16// 8
#define NUMTICK                         10// 50          ///< Number of ticks per row
#define TICKX                           8// 4           ///< X size of each tick
#define MAXTICK                         (NUMTICK*10) ///< Max number of ticks to draw
#define XPTICK                          6

#define NUMBAR                          6               ///< Number of status bars
#define NUMXPBAR                        2               ///< Number of xp bars

#define MAXLIGHTLEVEL                   16          ///< Number of premade light intensities
#define MAXSPEKLEVEL                    16          ///< Number of premade specularities
#define MAXLIGHTROTATION                256         ///< Number of premade light maps

#define DONTFLASH                       255
#define SEEKURSEAND                     31          ///< Blacking flash

#define ICON_SIZE                       32

#define SHADOWRAISE                       5

/// The supported colors of bars and blips
enum e_color
{
    COLOR_WHITE = 0,
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_PURPLE,
    COLOR_MAX,
    NOSPARKLE = COLOR_MAX
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// encapsulation of all graphics options

struct s_gfx_config_data
{
    GLuint shading;
    bool_t refon;
    Uint8  reffadeor;
    bool_t antialiasing;
    bool_t dither;
    bool_t perspective;
    bool_t phongon;
    bool_t shaon;
    bool_t shasprite;

    bool_t clearson;          ///< Do we clear every time?
    bool_t draw_background;   ///< Do we draw the background image?
    bool_t draw_overlay;      ///< Draw overlay?
    bool_t draw_water_0;      ///< Do we draw water layer 1 (TX_WATER_LOW)
    bool_t draw_water_1;      ///< Do we draw water layer 2 (TX_WATER_TOP)

    int    dyna_list_max;     ///< Max number of dynamic lights to draw
    bool_t exploremode;       ///< fog of war mode for mesh display
    bool_t usefaredge;        ///< Far edge maps? (Outdoor)

    // virtual window parameters
    float vw, vh;
    float vdw, vdh;
};
typedef struct s_gfx_config_data gfx_config_data_t;

bool_t gfx_config_data_init(gfx_config_data_t *);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// handle access to the gfx singleton for c modules
gfx_config_data_t * gfx_get_config();

bool_t gfx_synch_config( gfx_config_data_t * pgfx, struct s_ego_config_data * pcfg );
bool_t gfx_synch_oglx_texture_parameters( struct s_oglx_texture_parameters * ptex, struct s_ego_config_data * pcfg );
bool_t gfx_set_virtual_screen( gfx_config_data_t * pgfx );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
};
#endif

#define graphics_defs_h