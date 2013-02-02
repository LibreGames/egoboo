/*******************************************************************************
*  IDSZ.H                                                                      *
*    - EGOBOO-Game                                                             *     
*                                                                              *
*    - IDSZ-Map and management for editor and game                             *
*      (c) 2013 The Egoboo-Team                                                *
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

#ifndef _IDSZ_H_
#define _IDSZ_H_

/*******************************************************************************
* INCLUDES								                                   *
*******************************************************************************/

/*******************************************************************************
* DEFINES								                                   *
*******************************************************************************/

#define IDSZ_NOT_FOUND  -1
#define MAX_IDSZ_SIZE   64
#define MAX_IDSZ_MAP_SIZE 32

/*******************************************************************************
* TYPEDEFS 								                                   *
*******************************************************************************/

/// The definition of a single IDSZ element in a IDSZ map
typedef struct
{
    unsigned int idsz;      // The IDSZ itself
    int  level;             // Or value

} IDSZ_T;

/*******************************************************************************
* CODE 								                                       *
*******************************************************************************/

/* ============ Tools for loading and saving ========== */
void idszIDSZtoString(IDSZ_T *idsz, char *idsz_str);
void idszStringtoIDSZ(char *idsz_str, IDSZ_T *idsz);

/* =========== Handling IDSZ-Lists ========================== */
void idszMapAdd(IDSZ_T *idsz_list, unsigned int idsz, int list_len);
IDSZ_T *idszMapGet(IDSZ_T *idsz_list, unsigned int idsz, int list_len);

#endif  /* #define _IDSZ_H_ */