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
#include <memory.h>
#include <time.h>

#include "editfile.h"       /* Get filepath name by type                    */ 
#include "sdlglcfg.h"       /* Read egoboo text files eg. passage, spawn    */

#include "misc.h"           /* Own header   */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define MAX_TABLES	  32	
#define MAX_TREASURE 500
#define MAX_STRLEN    31

/*******************************************************************************
* TYPEDEFS 								                                       *
*******************************************************************************/

typedef struct {

    char line_name[2];              /* Dummy for empty name                           */
    char obj_name[MAX_STRLEN + 1];  /* Name of object to load, "-": Deleted in editor */

} TABLE_LINE_T;

typedef struct {
    char *table_name;       /* Pointer on table-name                        */
    int  first_obj;         /* Line number of first object in list          */
    int  num_obj;           /* Number of objects loaded into this table     */
} TABLE_T;

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

static TABLE_T TreasureTableList[MAX_TABLES + 1];
static TABLE_LINE_T TreasureTable[MAX_TREASURE + 2];

static SDLGLCFG_VALUE TreasureVal[] = {
	{ SDLGLCFG_VAL_STRING, TreasureTable[0].line_name, 1 },
	{ SDLGLCFG_VAL_STRING, TreasureTable[0].obj_name, MAX_STRLEN},
	{ 0 }
};

static SDLGLCFG_LINEINFO TreasureRec = {
    &TreasureTable[0],
	&TreasureTable[1],
	MAX_TREASURE,
	sizeof(TABLE_LINE_T),
	&TreasureVal[0]
};

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

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
 *     miscRand
 * Description:
 *     Returns a random number between min and max
 * Input:
 *     min: Lower bound
 *     max: Upper bound
 * Output:
 *     Generated random number (min .. max)
 */
int miscRand(int min, int max)
{

    if (max < 2 || min > max) {

        if (max < 0) {

            max = 0;

        }

        return max;

    }

    return (int)((((long)rand()*(max))/(RAND_MAX+1)) + min);
}

/*
 * Name:
 *     miscRandom
 * Description:
 *     Returns a random number between 1 and RAND_MAX.
 * Input:
 *     None
 * Output:
 *     Generated random number (1 .. RAND_MAX)
 */
int miscRandom(void)
{

    return rand();

}


/*
 * Name:
 *     treasureTableLoad
 * Description:
 *     Loads the table for the random treasures
 * Input:
 *     Nome
 */
void miscLoadRandTreasure(void)
{
    char *fname;
    int num_table, i, num_el;

    
    fname = editfileMakeFileName(EDITFILE_BASICDATDIR, "randomtreasure.txt");
    
    // Read the table
    sdlglcfgEgobooRecord(fname, &TreasureRec, 0);
    // Record 0 is always empty
    TreasureTable[0].obj_name[0] = 0;
    
    // Now create the table for random treasure
  	//Load each treasure table
	num_table = 0;
    i = 1;
    
    while(TreasureTable[i].obj_name[0] != 0)
    {
        if(num_table >= MAX_TABLES)
        {
            // Too much treasure tables
            break;
        }
        
        // Starts with table-name
        TreasureTableList[num_table].table_name = TreasureTable[i].obj_name;
        // Point on first object in list
        i++;
        TreasureTableList[i].first_obj = i;     // Number of first object in list
        
        num_el = 0;
        while(TreasureTable[i].obj_name[0] != 0 && strcmp(TreasureTable[i].obj_name, "END" ) != 0)
        {
            // It's an object name
            num_el++;
            i++; 
        }
        
        TreasureTableList[i].num_obj = num_el;
        // skip
    }
    
}

/*
 * Name:
 *     treasureGetRandom
 * Description:
 *     Gets the name for a random treasure object from the table 
 * Input:
 *     pname *: Pointer on name of random treasure (including %-sign for random names)
 * Output:
 *     Pointer of the object name for the random treasure 
 */
char *treasureGetRandom(char *pname)
{
    int i;
    int treasure_index;
   
    
    //Trap invalid strings
	if(NULL == pname)
    {
        // Return an empty object name
        return TreasureTable[0].obj_name;
    }
    
    // Skip the '%'-sign
    pname++;
    
    //Iterate through every treasure table until we find the one we want
	for(i = 0; i < MAX_TABLES; i++ )
	{
        if(TreasureTableList[i].table_name[0] != 0)
        {
            if(strcmp(TreasureTableList[i].table_name, &pname[1]) == 0)
            {
                // We found a name
        		//Pick a random number between 0 and the length of the table to get a random element out of the array
                treasure_index = miscRand(0, TreasureTableList[i].num_obj - 1);

                pname = TreasureTable[TreasureTableList[i].first_obj + treasure_index].obj_name;
        		
        		//See if it is an actual random object or a reference to a different random table
        		if('%' == pname[0])
                {
                    // Is a random treasure
                    treasureGetRandom(pname);
                }
                // All objects have lower-case names
                return strlwr(pname);
            }
		}
        else
        {
            break;
        }
	}
    
    // If not found, than it's an empty string */
    return TreasureTable[0].obj_name;
}