/*******************************************************************************
*  IDSZ.C                                                                      *
*    - EGOBOO-Editor                                                           *
*                                                                              *
*    - Handling of all IDSZ-Stuff                                              *
*      (c) 2013 Paul Mueller <muellerp61@bluewin.ch>                           *
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
* INCLUDES								                                   *
*******************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>


// own header
#include "idsz.h"

/*******************************************************************************
* DEFINES								                                   *
*******************************************************************************/

#define MAX_IDSZ_LIST  128

/*******************************************************************************
* TYPEDEFS								                                   *
*******************************************************************************/

typedef union
{
    unsigned int value;
    char val_str[6];

} IDSZ_CHAR_T;

/*******************************************************************************
* DATA									                                   *
*******************************************************************************/

// List of IDSZs used in game
static IDSZ_T IDSZList[MAX_IDSZ_LIST + 2] =
{
    // Fixed IDSZs for character profiles
    { 'DRES', 0 },  // ADD_BITS( pcap->skindressy, 1 << fget_int( fileread ) );
    { 'GOLD', 0 },  // pcap->money = fget_int( fileread );
    { 'STUK', 0 },  // pcap->resistbumpspawn = ( 0 != ( 1 - fget_int( fileread ) ) );
    { 'PACK', 0 },  // pcap->istoobig = !( 0 != fget_int( fileread ) );
    { 'VAMP', 0 },  // pcap->reflect = !( 0 != fget_int( fileread ) );
    { 'DRAW', 0 },  // pcap->alwaysdraw = ( 0 != fget_int( fileread ) );
    { 'RANG', 0 },  // pcap->isranged = ( 0 != fget_int( fileread ) );
    { 'HIDE', 0 },  // pcap->hidestate = fget_int( fileread );
    { 'EQUI', 0 },  // pcap->isequipment = ( 0 != fget_int( fileread ) );
    { 'SQUA', 0 },  // pcap->bump_sizebig = pcap->bump_size * 2;
    { 'ICON', 0 },  // pcap->draw_icon = ( 0 != fget_int( fileread ) );
    { 'SHAD', 0 },  // pcap->forceshadow = ( 0 != fget_int( fileread ) );
    { 'SKIN', 0 },  // pcap->skin_override = fget_int( fileread ) & 3;
    { 'CONT', 0 },  // pcap->content_override = fget_int( fileread );
    { 'STAT', 0 },  // pcap->state_override = fget_int( fileread );
    { 'LEVL', 0 },  // pcap->level_override = fget_int( fileread );
    { 'PLAT', 0 },  // pcap->canuseplatforms = ( 0 != fget_int( fileread ) );
    { 'RIPP', 0 },  // pcap->ripple = ( 0 != fget_int( fileread ) );
    { 'VALU', 0 },  // pcap->isvaluable = fget_int( fileread );
    { 'LIFE', 0 },  // pcap->life_spawn = 256.0f * fget_float( fileread );
    { 'MANA', 0 },  // pcap->mana_spawn = 256.0f * fget_float( fileread );
    { 'BOOK', 0 },  // pcap->spelleffect_type = fget_int( fileread ) % CHAR_MAX_SKIN;
    { 'FAST', 0 },  // pcap->attack_fast = ( 0 != fget_int( fileread ) );
        // Damage bonuses from stats
    { 'STRD', 0 },  // pcap->str_bonus = fget_float( fileread );
    { 'INTD', 0 },  // pcap->int_bonus = fget_float( fileread );
    { 'WISD', 0 },  // pcap->wis_bonus = fget_float( fileread );
    { 'DEXD', 0 },  // pcap->dex_bonus = fget_float( fileread );
    { 'MODL', 0 },  // fget_string( fileread) "SBH" Special case:
                                  // 'S' pcap->bump_override_size = btrue;
                                  // 'B' pcap->bump_override_sizebig = btrue;
                                  // 'H' pcap->bump_override_height = btrue;
    { 'DARK', 0 },  // pchr->darkvision_level = chr_get_skill( pchr, MAKE_IDSZ( 'DARK' ) );
                    // update_chr_darkvision  // Natural darkvision
        // 'enchant.c': target_ptr->see_kurse_level = chr_get_skill( target_ptr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
    { 'HEAL', 0 },  // Script: HealTarget( tmpargument = "amount" )
    { 'XWEP', 0 },  // pcap->isranged || chr_has_idsz( pself->target, MAKE_IDSZ( 'XWEP' ) );
    { 'STAB', 0 },  // Script: IfBackstabbed()
    // 'char.c': Do not allow poison or backstab skill if we are restricted by code of conduct
    { 'POIS', 0 },  // 'char.c': chr_get_skill
    { 'CODE', 0 },  // 'char.c': chr_get_skill
    { 'READ', 0 },  // 'char.c': chr_get_skill
    { 'CKUR', 0 },  // 'char.c': chr_get_skill
        // 'enchant.c': target_ptr->see_kurse_level = chr_get_skill( target_ptr, MAKE_IDSZ( 'C', 'K', 'U', 'R' ) );
    { 'BLOC', 0 },  // 'collision.c':
        // 'char.c': init_slot_idsz: [INVEN_PACK]  = IDSZ_NONE;
    { 'NECK', 0 },  // 'char.c': init_slot_idsz: [INVEN_NECK]  = MAKE_IDSZ( 'N', 'E', 'C', 'K' );
    { 'WRIS', 0 },  // 'char.c': init_slot_idsz: [INVEN_WRIS]  = MAKE_IDSZ( 'W', 'R', 'I', 'S' );
    { 'FOOT', 0 },  // 'char.c': [INVEN_FOOT]  = MAKE_IDSZ( 'F', 'O', 'O', 'T' );
    { ' ', 0 },  //
    { 0 }
};

// From 'char.c': 
// If it is none of the predefined IDSZ extensions then add it as a new skill
        // else idsz_map_add( pcap->skills, SDL_arraysize( pcap->skills ), idsz, fget_int( fileread ) );

/*******************************************************************************
* CODE									                                   *
*******************************************************************************/

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/* ============ Tools for loading and saving ========== */

/*
 * Name:
 *     idszIDSZtoString
 * Description:
 *     Converts a given IDSZ to a string
 * Input:
 *     idsz *: To convert to string
 *     idsz_str *: idsz as string [#idsz] #value
 */
void idszIDSZtoString(IDSZ_T *idsz, char *idsz_str)
{
    IDSZ_CHAR_T idsz_char;


    if(idsz -> idsz == 0)
    {
        idsz_char.value = 'NONE';
    }
    else
    {
        idsz_char.value = idsz -> idsz;
        idsz_char.val_str[4] = 0;
    }

    sprintf(idsz_str, "[%s] %d", idsz_char.val_str, idsz -> level);
}

/*
 * Name:
 *     idszStringtoIDSZ
 * Description:
 *     Converts a given IDSZ to a string
 * Input:
 *     idsz_str *: idsz as string [#idsz] #value
 *     idsz *:     To convert to string
 *
 */
void idszStringtoIDSZ(char *idsz_str, IDSZ_T *idsz)
{
    char sidsz[20];
    char value[20];


    sscanf(idsz_str, "%s%s", sidsz, value);


    if(strncmp(sidsz, "NONE", 4) == 0)
    {
        idsz -> idsz = 0;
    }
    else
    {
        // Now remove the leading bracket
        idsz -> idsz = *(unsigned int *)&sidsz[1];
    }

    if(isdigit(value[0]))
    {
        // @todo: Handle other special cases
        sscanf(value, "%d", &idsz -> level);
    }
}

/*
 * Name:
 *     idszMapAdd
 * Description:
 *     Adds given idsz to map, if not already in list.
 * Input:
 *     idsz_list *: To add IDSZ
 *     idsz:        Add this one to list IDSZ
 *     list_len:    Length of given list
 */
void idszMapAdd(IDSZ_T *idsz_list, unsigned int idsz, int list_len)
{
    int i;


    for(i = 0; i < list_len; i++)
    {
        if(idsz_list[i].idsz == 0)
        {
            idsz_list[i].idsz = idsz;
            return;
        }
    }
}

/*
 * Name:
 *     idszMapGet
 * Description:
 *     Returns 1 if given IDSZ is found in list
 * Input:
 *     idsz_list *: To get IDSZ from, if NULL, the internal list is used
 *     idsz:        Add this one to list IDSZ
 *     list_len:    Length of given list
 * Output:
 *     Pointer on idsz-description, if any
 */
IDSZ_T *idszMapGet(IDSZ_T *idsz_list, unsigned int idsz, int list_len)
{
    static IDSZ_T no_idsz;
    int i;


    if(NULL == idsz_list)
    {
        idsz_list = &IDSZList[0];
        list_len  = MAX_IDSZ_LIST;
    }

    for(i = 0; i < list_len; i++)
    {
        if(idsz_list[i].idsz == idsz)
        {
            return &idsz_list[i];
        }
    }

    // Not found
    return &no_idsz;
}