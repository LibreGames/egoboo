/*******************************************************************************
*  MISC.H                                                                  *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - [...]                                                                   *
*      (c)2012 Paul Mueller <muellerp61@bluewin.ch>                            *
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

#ifndef _MISC_H_
#define _MISC_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/


/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

// Random values
void miscSetRandSeed(int num);
int  miscRand(int min, int max);
int  miscRandom(void);
// Random treasure
void miscLoadRandTreasure(void);
char *miscGetRandTreasure(char* pstr);


#endif  /* #define _MISC_H_ */