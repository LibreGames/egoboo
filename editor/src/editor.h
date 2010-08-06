/*******************************************************************************
*  EDITOR.H                                                                    *
*	- Entrypoint for the library, starts the menu-loop.		                   *
*									                                           *
*   Copyright (C) 2001  Paul Mueller <pmtech@swissonline.ch>                   *
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
*                                                                              *
*                                                                              *
* Last change: 2008-06-21                                                      *
*******************************************************************************/

#ifndef _EDITOR_H_
#define _EDITOR_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define MAXMESSAGE          6                       // Number of messages
#define TOTALMAXDYNA                    64          // Absolute max number of dynamic lights
#define TOTALMAXPRT             2048                // True max number of particles


#define MAXLIGHT 100
#define MAXRADIUS 500*FOURNUM
#define MINRADIUS 50*FOURNUM
#define MAXLEVEL 255
#define MINLEVEL 50

#define VERSION 005             // Version number
#define YEAR 1999               // Year
#define NAME "Cartman"          // Program name
#define KEYDELAY 12             // Delay for keyboard
#define MAXTILE 256             //
#define TINYXY   4              // Plan tiles
#define SMALLXY 32              // Small tiles
#define BIGXY   64              // Big tiles
#define CAMRATE 8               // Arrow key movement rate
#define MAXSELECT 2560          // Max points that can be select_vertsed
#define FOURNUM   ((1<<7)/((float)(SMALLXY)))          // Magic number
#define FIXNUM    4 // 4.129           // 4.150
#define TILEDIV   SMALLXY
#define MAPID     0x4470614d        // The string... MapD

#define FADEBORDER 64           // Darkness at the edge of map
#define SLOPE 50            // Twist stuff

#define ONSIZE 264          // Max size of raise mesh

#define MAXMESHTYPE                     64          // Number of mesh types
#define MAXMESHLINE                     64          // Number of lines in a fan schematic
#define MAXMESHVERTICES                 16      // Max number of vertices in a fan
#define MAXMESHFAN                      (512*512)        // Size of map in fans
#define MAXTOTALMESHVERTICES            (MAXMESHFAN*MAXMESHVERTICES)
#define MAXMESHTILEY                    1024       // Max fans in y direction
#define MAXMESHBLOCKY                   (( MAXMESHTILEY >> 2 )+1)  // max blocks in the y direction
#define MAXMESHCOMMAND                  4             // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32            // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32            // Max trigs in each command

#define FANOFF   0xFFFF         // Don't draw
#define CHAINEND 0xFFFFFFFF     // End of vertex chain

#define VERTEXUNUSED 0          // Check mesh.vrta to see if used
#define MAXPOINTS 20480         // Max number of points to draw

#define MPDFX_REF 0            // MeshFX
#define MPDFX_SHA 1            //
#define MPDFX_DRAWREF 2      //
#define MPDFX_ANIM 4             //
#define MPDFX_WATER 8            //
#define MPDFX_WALL 16            //
#define MPDFX_IMPASS 32         //
#define MPDFX_DAMAGE 64         //
#define MPDFX_SLIPPY 128        //

#define INVALID_BLOCK ((unsigned int )(~0))
#define INVALID_TILE  ((unsigned int )(~0))

#define DAMAGENULL          255

// handle the upper and lower bits for the tile image
#define TILE_UPPER_SHIFT                8
#define TILE_LOWER_MASK                 ((1 << TILE_UPPER_SHIFT)-1)
#define TILE_UPPER_MASK                 (~TILE_LOWER_MASK)

#define TILE_GET_LOWER_BITS(XX)         ( TILE_LOWER_MASK & (XX) )

#define TILE_GET_UPPER_BITS(XX)         (( TILE_UPPER_MASK & (XX) ) >> TILE_UPPER_SHIFT )
#define TILE_SET_UPPER_BITS(XX)         (( (XX) << TILE_UPPER_SHIFT ) & TILE_UPPER_MASK )
#define TILE_SET_BITS(HI,LO)            (TILE_SET_UPPER_BITS(HI) | TILE_GET_LOWER_BITS(LO))

#define TILE_IS_FANOFF(XX)              ( FANOFF == (XX) )

#define TILE_HAS_INVALID_IMAGE(XX)      HAS_SOME_BITS( TILE_UPPER_MASK, (XX).img )
                                            
/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {
    unsigned char numvertices;                // Number of vertices

    unsigned char ref[MAXMESHVERTICES];       // Lighting references

    int     x[MAXMESHVERTICES];         // Vertex texture posi
    int     y[MAXMESHVERTICES];         //

    float   u[MAXMESHVERTICES];         // Vertex texture posi
    float   v[MAXMESHVERTICES];         //

    int     count;                      // how many commands
    int     size[MAXMESHCOMMAND];      // how many command entries
    int     vrt[MAXMESHCOMMANDENTRIES];       // which vertex for each command entry
} COMMAND_T;

typedef struct {
    unsigned char start[MAXMESHTYPE];
    unsigned char end[MAXMESHTYPE];
} LINE_DATA_T;

typedef struct {
    unsigned char exploremode;

    int               tiles_x;           // Size of mesh
    int               tiles_y;           //
    unsigned int fanstart[MAXMESHTILEY];           // Y to fan number

    int               blocksx;          // Size of mesh
    int               blocksy;          //
    unsigned int blockstart[(MAXMESHTILEY >> 4) + 1];

    int               edgex;            // Borders of mesh
    int               edgey;            //
    int               edgez;            //

    unsigned char fantype[MAXMESHFAN];  // Tile fan type
    unsigned char fx[MAXMESHFAN];       // Tile special effects flags
    unsigned short tx_bits[MAXMESHFAN]; // Tile texture bits and special tile bits
    unsigned char twist[MAXMESHFAN];    // Surface normal

    unsigned int vrtstart[MAXMESHFAN];              // Which vertex to start at
    unsigned int vrtnext[MAXTOTALMESHVERTICES];     // Next vertex in fan

    unsigned int vrtx[MAXTOTALMESHVERTICES];        // Vertex position
    unsigned int vrty[MAXTOTALMESHVERTICES];        //
    short int vrtz[MAXTOTALMESHVERTICES];           // Vertex elevation
    unsigned char vrta[MAXTOTALMESHVERTICES];       // Vertex base light, 0=unused

    unsigned int numline[MAXMESHTYPE];              // Number of lines to draw
    LINE_DATA_T  line[MAXMESHLINE];
    COMMAND_T    command[MAXMESHTYPE];
} MESH_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/


#endif /* _EDITOR_H_	*/

