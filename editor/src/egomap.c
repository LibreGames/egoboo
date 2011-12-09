/*******************************************************************************
*  EGOMAP.C                                                                    *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Managing the map and the objects on it                                  *
*      (c)2011 Paul Mueller <pmtech@swissonline.ch>                            *
*                                                                              *
*  This program is free software; you can redistribute it and/or modify        *
*  it under the terms of the GNU General Public License as published by        *
*  the Free Software Foundation; either version 2 of the License, or           *
*  (at your option) any later version.                                         *
*                                                                              *
*  This program is distributed in the hope that it will be useful,             *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
*  GNU Library General Public License for more details.                        *
*                                                                              *
*  You should have received a copy of the GNU General Public License           *
*  along with this program; if not, write to the Free Software                 *
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *
*******************************************************************************/


/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include <memory.h>


#include "sdlgl3d.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EGOMAP_MAXOBJ    1024

/*******************************************************************************
* TYPEDEFS								                                       *
*******************************************************************************/

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

static SDLGL3D_OBJECT TileObj;                      /* Tile for collision detection */
static SDLGL3D_OBJECT GameObj[EGOMAP_MAXOBJ + 2];   /* All objects              */

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/* ========================================================================== */
/* ============================= PUBLIC ROUTINE(S) ========================== */
/* ========================================================================== */

/*
 * Name:
 *     egomapInit
 * Description:
 *     Initializes the map data 
 * Input:
 *     None 
 */
 void  egomapInit(void)
 {
    
    memset(GameObj, (EGOMAP_MAXOBJ + 1) * sizeof(SDLGL3D_OBJECT), 0);

 }
