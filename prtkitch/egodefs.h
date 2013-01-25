/*******************************************************************************
*  EGODEFS.H                                                                   *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Globally used definitions                                               *
*      (c) 2013 Paul Mueller <muellerp61@bluewin.ch>                           *
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
*   This program is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*   GNU Library General Public License for more details.                       *
*                                                                              *
*   You should have received a copy of the GNU General Public License          *
*   along with this program; if not, write to the Free Software                *
*   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
*******************************************************************************/

#ifndef _EGODEFS_H_
#define _EGODEFS_H_

/*******************************************************************************
* DEFINES 								                                       *
*******************************************************************************/

#define MPDFX_REF       0x00    /* MeshFX   */
#define MPDFX_SHA       0x01    
#define MPDFX_DRAWREF   0x02    
#define MPDFX_ANIM      0x04    
#define MPDFX_WATER     0x08    
#define MPDFX_WALL      0x10    
#define MPDFX_IMPASS    0x20    
#define MPDFX_DAMAGE    0x40    
#define MPDFX_SLIPPY    0x80 

// @todo: MPDFXG_             fan.fxg      /* GameFX */

// Types of objects (for drawing and handling) 
#define EGOMAP_OBJ_TILE   0x01      /* First always the tile            */
#define EGOMAP_OBJ_CHAR   0x02      /* Characters (MD2)                 */
#define EGOMAP_OBJ_PART   0x03      /* Particles (can be transparent)   */

// Types of environment collisions (object with map)
#define EGOMAP_HIT_NONE  0x00      // No collision detected
#define EGOMAP_HIT_WALL  0x01      // Collision with wall detected
#define EGOMAP_HIT_FLOOR 0x02      // Collision with bottom detected
#define EGOMAP_HIT_OTHER 0x03      // Collsiion with other object detected

#define EGOMAP_TILE_SIZE 128

/// Everything that is necessary to compute an objects interaction with the environment
typedef struct
{
    // floor stuff
    unsigned char twist;    // ego_mesh_get_twist( PMesh, itile );
    unsigned char fx;       // ego_mesh_get_fx()
    float  floor_level;     ///< Height of tile
                            // ego_mesh_get_level( PMesh, loc_obj->pos.x, loc_obj->pos.y );
    float  level;           ///< Height of a tile or a platform
                            // Init: penviro->floor_level @todo: Add height of 'fixed' object
                            // 'onwhichplatform_ref'    
    float  zlerp;

    float adj_level;        ///< The level for the particle to sit on the floor or a platform
    float adj_floor;        ///< The level for the particle to sit on the floor or a platform

    // friction stuff
    char is_slipping;
    char is_slippy, // !penviro->is_watery && ( 0 != ego_mesh_test_fx( PMesh, loc_obj->onwhichgrid, MAPFX_SLIPPY ) );
         is_watery; // object -> act_tile ==> MAPFX_WATER 'onwhichgrid', not set if !water.is_water
                    // water.is_water && penviro->inwater;
    float  noslipfriction;      // @todo: Handle this by map
    float  air_friction,
           ice_friction;
    float  fluid_friction_hrz, 
           fluid_friction_vrt; // penviro->is_watery ? waterfriction : penviro->air_friction
    float  friction_hrz;        // slippyfriction
    float  traction;            // Init = 1.0f; @todo: Adjust by tile angle 'hillslide'

    // misc states
    char   hit_env;             // 'hit_env: Hit wall or bottom from collision code or 'heavy' object
    float  water_surface_level; // @new 
    char   inwater; 
    float  waterdouselevel;     // @new      
    float  acc[3];

} MAPENV_T;

#endif  /* #define _EGODEFS_H_ */