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
#define MAXTOTALMESHVERTICES            ((MAXMESHFAN*MAXMESHVERTICES) - 10)
#define MAXMESHCOMMAND                  4             // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32            // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32            // Max trigs in each command

#define FANOFF   0xFFFF         // Don't draw

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

#define COMMAND_TEXTUREHI_FLAG 0x20

#define DAMAGENULL          255

// Editor modes: How to draw the map
#define EDIT_MODE_SOLID     0x01        /* Draw solid, yes/no       */
#define EDIT_MODE_TEXTURED  0x02        /* Draw textured, yes/no    */
#define EDIT_MODE_LIGHTMAX  0x04        /* Is drawn all white       */    
                                            
/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    unsigned char ref;      /* Light reference      */
    float u, v;             /* Texture mapping info: Has to be multiplied by 'sub-uv' */    
    float x, y, z;          /* Default position of vertex at 0, 0, 0    */

} EDITOR_VTX;

typedef struct {

    char *name;                             // Name of this fan type
    char default_fx;                        // Default flags to set, if new fan (for walls)
    unsigned char default_tx_no;            // Default texture to set
    unsigned char numvertices;			    // meshcommandnumvertices
    int   count;			                // meshcommands
    int   size[MAXMESHCOMMAND];             // meshcommandsize
    int   vertexno[MAXMESHCOMMANDENTRIES];  // meshcommandvrt, number of vertex
    float uv[MAXMESHVERTICES * 2];          // meshcommandu, meshcommandv
    float biguv[MAXMESHVERTICES * 2];       // meshcommandu, meshcommandv
                                            // For big texture images
    EDITOR_VTX vtx[MAXMESHVERTICES];        // Holds it's u/v position and default extent x/y/z
    
} COMMAND_T;

typedef struct {
  
    unsigned char tx_no;    /* Number of texture:                           */
                            /* (tx_no >> 6) & 3: Number of wall texture     */
                            /* tx_no & 0x3F:     Number of part of texture  */ 
    unsigned char tx_flags; /* Special flags                                */
    unsigned char fx;		/* Tile special effects flags                   */
    char type;              /* Tile fan type (index into COMMAND_T)         */

} FANDATA_T;

typedef struct {

    int pt[3];              /* Point x / y / z                  */
    unsigned char a;        /* Ambient lighting                 */
    unsigned char l;        /* Light intensity (not used yet)   */

} MESH_VTXI_T;              /* Planned for later adjustement how to store vertices */

typedef struct {

    unsigned char map_loaded;   // A map is loaded  into this struct
    unsigned char draw_mode;    // Flags for display of map
    
    int numvert;                // Number of vertices in map
    int numfreevert;            // Number of free vertices for edit
    int tiles_x, tiles_y;       // Size of mesh in tiles          
    int numfan;                 // Size of map in 'fans' (tiles_x * tiles_y)
    int watershift;             // Depends on size of map

    float edgex;                // Borders of mesh
    float edgey;                
    float edgez;                

    FANDATA_T fan[MAXMESHFAN];                  // Fan desription            
    unsigned char twist[MAXMESHFAN];            // Surface normal
    
    float vrtx[MAXTOTALMESHVERTICES + 10];             // Vertex position
    float vrty[MAXTOTALMESHVERTICES + 10];             //
    float vrtz[MAXTOTALMESHVERTICES + 10];             // Vertex elevation
    
    unsigned char vrta[MAXTOTALMESHVERTICES + 10];   // Vertex base light, 0=unused
    
    int  vrtstart[MAXMESHFAN];                  // First vertex of given fan  
    char visible[MAXTOTALMESHVERTICES + 10];         // Is visible yes/no

} MESH_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

#endif /* _EDITOR_H_	*/

