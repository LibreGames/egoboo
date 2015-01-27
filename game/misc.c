/*******************************************************************************
*  MISC.C                                                                  *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Functions for the random treausre table                                 *
*      (c)2012 Paul Mueller <muellerp61@bluewin.ch>                            *
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

// #include <stdio.h>
#include <stdlib.h>     /* rand()       */
#include <string.h>
#include <time.h>


// Own header
#include "misc.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/


/*******************************************************************************
* TYPEDEFS 								                                       *
*******************************************************************************/


/*******************************************************************************
* DATA									                                       *
*******************************************************************************/


/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/* =============== Random values ============= */

/*
 * Name:
 *     miscSetRandSeed
 * Description:
 *     Sets the seed for the random generator. If 'seed' is 0, then the
 *     current system time is used as seed.
 * Input:
 *     seed: Seed
 */
void miscSetRandSeed(int seed)
{
    if (! seed) {

        seed = time(0);

    }

    srand(seed);
}

/*
 * Name:
 *     miscRandRange
 * Description:
 *     Returns a random in the range ipair[0] .. ipair[1]
 * Input:
 *     ipair[] : Minimum and maximum value
 * Output:
 *     Generated random number (min .. max)
 */
int miscRandRange(int ipair[2])
{
    if (ipair[1] < 2 || ipair[0] > ipair[1])
    {
        if (ipair[1] < 0)
        {
            ipair[1] = 0;
        }

        return ipair[1];
    }

    return (int)((((long)rand()*(ipair[1]))/(RAND_MAX+1)) + ipair[0]);
}

/*
 * Name:
 *     miscRandVal
 * Description:
 *     Returns a random number between 1 and 'max'.
 * Input:
 *     max: Maximal vale
 * Output:
 *     Generated random number (1 .. RAND_MAX)
 */
int miscRandVal(int max)
{
    return (int)((((long)rand()*(max))/(RAND_MAX+1)));
}


