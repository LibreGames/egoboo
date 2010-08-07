/*******************************************************************************
*  EDITOR.H                                                                    *
*	- General definitions for the editor                	                   *
*									                                           *
*   Copyright (C) 2010  Paul Mueller <pmtech@swissonline.ch>                   *
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
*******************************************************************************/

#ifndef _EDITOR_H_
#define _EDITOR_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define MAXMESSAGE         6    // Number of messages
#define TOTALMAXDYNA      64    // Absolute max number of dynamic lights
#define TOTALMAXPRT     2048    // True max number of particles


#define MAXLIGHT 100
#define MAXRADIUS 500*FOURNUM
#define MINRADIUS 50*FOURNUM
#define MAXLEVEL 255
#define MINLEVEL 50

#define VERSION 005             // Version number
#define YEAR 1999               // Year
#define KEYDELAY 12             // Delay for keyboard
#define MAXTILE 256             //
#define TINYXY   4              // Plan tiles
#define SMALLXY 32              // Small tiles
#define BIGXY   64              // Big tiles
#define CAMRATE 8               // Arrow key movement rate
#define MAXSELECT 2560          // Max points that can be select_vertsed
#define FOURNUM   ((1<<7)/((float)(SMALLXY)))          // Magic number

#define FADEBORDER 64           // Darkness at the edge of map
#define ONSIZE 264              // Max size of raise mesh

#define MAXMESHTYPE                     64          // Number of mesh types
#define MAXMESHLINE                     64          // Number of lines in a fan schematic
#define MAXMESHVERTICES                 16          // Max number of vertices in a fan
#define MAXMESHFAN                      (128*128)   // Size of map in fans
#define MAXTOTALMESHVERTICES            (MAXMESHFAN*MAXMESHVERTICES)
#define MAXMESHCOMMAND                  4             // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32            // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32            // Max trigs in each command

#define FANOFF   0xFFFF         // Don't draw
#define CHAINEND 0xFFFFFFFF     // End of vertex chain

#define VERTEXUNUSED 0          // Check mesh -> vrta to see if used
#define MAXPOINTS 20480         // Max number of points to draw

#define MPDFX_REF       0x00    /* MeshFX   */
#define MPDFX_SHA       0x01    
#define MPDFX_DRAWREF   0x02    
#define MPDFX_ANIM      0x04    
#define MPDFX_WATER     0x08    
#define MPDFX_WALL      0x10    
#define MPDFX_IMPASS    0x20    
#define MPDFX_DAMAGE    0x40    
#define MPDFX_SLIPPY    0x80    

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

    int   numvertices;			            // meshcommandnumvertices
    int   count;			                // meshcommands
    int   size[MAXMESHCOMMAND];             // meshcommandsize
    int   vertexno[MAXMESHCOMMANDENTRIES];  // meshcommandvrt, number of vertex
    float uv[MAXMESHVERTICES * 2];          // meshcommandu, meshcommandv
    float biguv[MAXMESHVERTICES * 2];       // meshcommandu, meshcommandv
                                            // For big texture images
} COMMAND_T;

typedef struct {

    unsigned short tx_bits; /* Tile texture bits and special tile bits      */
                            /* The upper byte could be abused for some	    */
                            /* "decoration" - index.			            */
    unsigned char  fx;		/* Tile special effects flags                   */
    unsigned char  type;	/* Tile fan type            			        */

} FANDATA_T;

typedef struct {

    unsigned char exploremode;
    unsigned char map_loaded;   // A map is loaded  into this struct
    
    int numvert;        // Number of vertices
    int tiles_x;        // Size of mesh in tiles
    int tiles_y;        //
    int numfan;
    int watershift;     // Depends on size of map
    char wireframe;     // Display in wireframe mode yes/no

    int blocksx;        // Size of mesh
    int blocksy;        //

    float edgex;        // Borders of mesh
    float edgey;        //
    float edgez;        //
    
    int textures[4];    // OpenGL-Number of Textures used for map

    FANDATA_T fan[MAXMESHFAN];             
    unsigned char twist[MAXMESHFAN];            // Surface normal
    
    int vrtx[MAXTOTALMESHVERTICES];             // Vertex position
    int vrty[MAXTOTALMESHVERTICES];             //
    int vrtz[MAXTOTALMESHVERTICES];             // Vertex elevation
    unsigned char vrta[MAXTOTALMESHVERTICES];   // Vertex base light, 0=unused
    
    int  vrtstart[MAXMESHFAN];                  // First vertex of given fan  
    char visible[MAXTOTALMESHVERTICES];         // Is visible yes/no

} MESH_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

#endif /* _EDITOR_H_	*/

