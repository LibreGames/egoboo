/*******************************************************************************
*  SCRIPT.C                                                                    *
*    - EGOBOO-Game                                                             *
*                                                                              *
*    - All functions to run a script                                           *
*      (c) 2013 The Egoboo Team                                                *
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
* INCLUDES                                                                     *
*******************************************************************************/

#include "msg.h"        // Type of messages

/*******************************************************************************
* DEFINES                                                                      *
*******************************************************************************/

#define ARG_CHAR ((char)1)
#define ARG_SINT ((char)2)
#define ARG_INT  ((char)3)
#define ARG_STR  ((char)4)      // Argument is a string

#define SCMP_EQUAL        0xFF
#define SCMP_NOTEQUAL     0xFE
#define SCMP_LESS         0xFD
#define SCMP_LESSEQUAL    0xFC
#define SCMP_GREATER      0xFB
#define SCMP_GREATEREQUAL 0xFA
#define SCMP_AND	      0xF9	// SC_CODEAND
#define SCMP_OR		      0xF8    // SC_CODEOR

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

// codes less then zero are functions, codes >= 0 are argument values
typedef struct
{
    char num_code;          // Number of codes to set
    char codes[4];
    char *name;             // Name of function
    char args[6+1];         // Arguments, if any    

} SCRIPT_FUNC_T;

// Constants
typedef struct
{
    // Always a prefix 'which' and a value
    char code;
    short int subcode;
    char *name;             // Name of constant
    
} SCRIPT_CONST_T;

// Arguments for functions
typedef struct
{
    // Always a prefix 'which' and a value
    char code[2];    
    char *name;             // Name of constant
    
} SCRIPT_ARG_T;

// Operators
typedef struct
{
    char code;
    char name[2];
   
} SCRIPT_OP_T;

// Conditions for encoding
typedef struct
{
    char which_val;     // Which value to check
    char which_arg;     // Which argument its in
    char arg_type;      // Type of argument
    char *name;         // Name of condition
    
} SCRIPT_COND_T; 

/*******************************************************************************
* DATA                                                                         *
*******************************************************************************/

// All conditions "FUNC_IF", Comparisions returning true/false
static SCRIPT_FUNC_T ScrCond[] = 
{
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifammoout" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifarmoris" },
    { SVAR_ALERT, MSG_ATLASTWAYPOINT, 0, "ifatlastwaypoint" },
    { SVAR_ALERT, MSG_ATTACKED, 0, "ifattacked" },
    { SVAR_ALERT, MSG_ATWAYPOINT, 0, "ifatwaypoint" },
    { SVAR_CHAR,  FUNC_GETFLAG, 0, "ifbackstabbed" },
    { SVAR_ALERT, MSG_BLOCKED, 0, "ifblocked" },
    { SVAR_ALERT, MSG_BORED, 0, "ifbored" },
    { SVAR_ALERT, MSG_BUMPED, 0, "ifbumped" },
    { SVAR_ALERT, MSG_CALLEDFORHELP, 0, "ifcalledforhelp" },
    { SVAR_ALERT, MSG_CHANGED, 0, "ifchanged" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifcharacterwasabook" },
    { SVAR_ALERT, MSG_CLEANEDUP, 0, "ifcleanedup" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifcontentis" },
    { SVAR_ALERT, MSG_CRUSHED, 0, "ifcrushed" },
    { SVAR_CHAR, FUNC_GETFLAG, 0, "ifdazed" },
    { SVAR_ALERT, MSG_DISAFFIRMED, 0, "ifdisaffirmed" },
    { SVAR_CHAR, FUNC_GETFLAG, 0, "ifdismounted" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifdistanceismorethanturn" },
    { SVAR_ALERT, MSG_DROPPED, 0, "ifdropped" },
    { SVAR_CHAR, FUNC_GETFLAG, 0, "ifequipped" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "iffacingtarget" },
    { SVAR_ALERT, MSG_GRABBED, 0, "ifgrabbed" },
    { SVAR_CHAR, FUNC_GETFLAG, 0, "ifgrogged" },
    { SVAR_ALERT, MSG_HEALED, 0, "ifhealed" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifheld" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifheldinlefthand" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifhitfrombehind" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifhitfromfront" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifhitfromleft" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifhitfromright" },
    { SVAR_ALERT, MSG_HITGROUND, 0, "ifhitground" },
    { SVAR_ALERT, MSG_HITVULNERABLE, 0, "ifhitvulnerable" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifholderblocked" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifholdingitemid" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifholdingmeleeweapon" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifholdingrangedweapon" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifholdingshield" },
    { SVAR_CHAR, FUNC_GETFLAG, 0, "ifinvisible" },
    { SVAR_ALERT, MSG_INWATER, 0, "ifinwater" },
    { SVAR_ALERT, MSG_KILLED, 0, "ifkilled" },
    { SVAR_CHAR, FUNC_GETFLAG, 0, "ifkursed" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifleaderisalive" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifleaderkilled" },
    { SVAR_ALERT, MSG_LEVELUP }, "iflevelup" },
    { SVAR_MODULE , FUNC_GETVALUE, 0, "ifmodulehasidsz" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifmounted" },
    { SVAR_CHAR, FUNC_GETFLAG, 0, "ifnameisknown" },
    { SVAR_ALERT, MSG_NOTDROPPED, 0, "ifnotdropped" },
    { SVAR_ALERT, MSG_NOTPUTAWAY, 0, "ifnotputaway" },
    { SVAR_ALERT, MSG_NOTTAKENOUT, 0, "ifnottakenout" },
    { SVAR_GAME, FUNC_GETFLAG, 0, "ifoperatorislinux" },
    { SVAR_GAME, FUNC_GETFLAG, 0, "ifoperatorismacintosh" },
    { SVAR_ALERT, MSG_ORDERED, 0, "ifordered" },
    { SVAR_CHAR, FUNC_GETFLAG, 0, "ifoverwater" },
    { SVAR_PASSAGE, FUNC_GETFLAG, 0, "ifpassageopen" },
    { SVAR_ALERT, MSG_PUTAWAY, 0, "ifputaway" },
    { SVAR_ALERT, MSG_REAFFIRMED, 0, "ifreaffirmed" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifscoredahit" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifsitting" },
    { SVAR_GAME, FUNC_GETVALUE, 0, "ifsomeoneisstealing" },
    { SVAR_ALERT, MSG_SPAWNED, 0, "ifspawned" },
    { SVAR_STATE, SVAR_TMP, ARG_CHAR, "ifstateis" },    // Variable argument
    { SVAR_STATE, SVAR_FIX,  0,"ifstateis0" },  // Fix encoded value to compare
    { SVAR_STATE, SVAR_FIX,  1,  "ifstateis1" },
    { SVAR_STATE, SVAR_FIX, 10, "ifstateis10" },
    { SVAR_STATE, SVAR_FIX, 11, "ifstateis11" },
    { SVAR_STATE, SVAR_FIX, 12, "ifstateis12" },
    { SVAR_STATE, SVAR_FIX, 13, "ifstateis13" },
    { SVAR_STATE, SVAR_FIX, 14, "ifstateis14" },
    { SVAR_STATE, SVAR_FIX, 15, "ifstateis15" },
    { SVAR_STATE, SVAR_FIX,  2, "ifstateis2" },
    { SVAR_STATE, SVAR_FIX,  3, "ifstateis3" },
    { SVAR_STATE, SVAR_FIX,  4, "ifstateis4" },
    { SVAR_STATE, SVAR_FIX,  5, "ifstateis5" },
    { SVAR_STATE, SVAR_FIX,  6, "ifstateis6" },
    { SVAR_STATE, SVAR_FIX,  7, "ifstateis7" },
    { SVAR_STATE, SVAR_FIX,  8, "ifstateis8" },
    { SVAR_STATE, SVAR_FIX,  9, "ifstateis9" },
    { SVAR_STATE, SVAR_FIX,  6, "ifstateischarge" },
    { SVAR_STATE, SVAR_FIX,  7, "ifstateiscombat" },
    { SVAR_STATE, SVAR_FIX,  3, "ifstateisfollow" },
    { SVAR_STATE, SVAR_FIX,  2, "ifstateisguard" },
    // @todo: Add argument for proper function
    { SVAR_STATE, FUNC_STATE, 0, "ifstateisnot" },
    { SVAR_STATE, FUNC_STATE, 0, "ifstateisodd" },
    { SVAR_STATE, SVAR_FIX, 0, "ifstateisparry" },
    { SVAR_STATE, SVAR_FIX, 5, "ifstateisretreat" },
    { SVAR_STATE, SVAR_FIX, 4, "ifstateissurround" },
    { SVAR_STATE, SVAR_FIX, 1, "ifstateiswander" },
    { SVAR_ALERT, MSG_TAKENOUT, 0, "iftakenout" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetcanopenstuff" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetcanseeinvisible" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetcanseekurses" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargethasanyid" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargethasid" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargethasitemid" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargethasitemidequipped" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargethasnotfullmana" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargethasquest" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargethasskillid" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargethasspecialid" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargethasvulnerabilityid" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetholdingitemid" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisalive" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetisamount" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetisaplatform" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetisaplayer" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetisaspell" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisattacking" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetisaweapon" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisdefending" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisdressedup" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisfacingself" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisfemale" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetisflying" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetishurt" },
    { SVAR_TARGET, FUNC_GETFLAG, 0, "iftargetiskursed" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetismale" },
    { SVAR_TARGET, FUNC_GETVALUE0, 0, "iftargetismounted" },
    { SVAR_TARGET, FUNC_GETVALUE0, 0, "iftargetisoldtarget" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisonhatedteam" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisonotherteam" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisonsameteam" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisowner" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetisself" },
    { SVAR_TARGET, FUNC_GETVALUE, 0, "iftargetissneaking" },
    { SVAR_ALERT, MSG_TARGETKILLED, 0, "iftargetkilled" },
    { SVAR_ALERT, MSG_THROWN, 0, "ifthrown" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "iftimeout" },
    { SVAR_ALERT, MSG_TOOMUCHBAGGAGE, 0, "iftoomuchbaggage" },
    { SVAR_CHAR, FUNC_GETVALUE, 0, "ifunarmed" },
    { SVAR_CHAR, FUNC_GETFLAG, 0, "ifusageisknown" },
    { SVAR_ALERT, MSG_USED, 0, "ifused" },
    // Always compare tmpx, tmpy
    { SVAR_COMPXY, SCMP_EQUAL, 0, "ifxisequaltoy" }, 
    { SVAR_COMPXY, SCMP_LESS, 0, "ifxislessthany" },
    { SVAR_COMPXY, SCMP_GREATER, 0, "ifxismorethany" },
    { SVAR_COMPXY, SCMP_EQUAL, 0, "ifyisequaltox" },
    { SVAR_COMPXY, SCMP_GREATER, 0, "ifyislessthanx" },
    { SVAR_COMPXY, SCMP_LESS, 0, "ifyismorethanx" },
    { 0 }
};

// Do more work while encoding to save function calls at decoding time
static SCRIPT_FUNC_T ScrFunc[] = 
{
    { 0, { FUNC_ACCEL }, "acceleratetarget", { 0 } },
    { 0, { FUNC_ACCEL }, "acceleratetargetup", { 0 } },
    { 0, { FUNC_ACCEL }, "accelerateup", { 0 } },
    { 0, { FUNC_ADD }, "addblipallenemies", { 0 } },
    { 0, { FUNC_ADD, SVAR_MSG }, "addendmessage", { 0 } },
    { 0, { FUNC_ADD }, "addidsz", { 0 } },
    { 0, { FUNC_ADD }, "addquestallplayers", { 0 } },
    { 0, { FUNC_ADD, SVAR_PASSAGE }, "addshoppassage", { 0 } },
    { 0, { FUNC_ADD }, "addstat", { 0 } },
    { 0, { FUNC_ADD }, "addtargetquest", { 0 } },
    { 0, { FUNC_ADD }, "addwaypoint", { 0 } },
    { 0, { FUNC_ADD }, "addxy", { 0 } },
    { 0, { FUNC_MODULE }, "beatmodule", { 0 } },
    { 0, { FUNC_MODULE }, "beatquestallplayers", { 0 } },
    { 0, { FUNC_TEAM }, "becomeleader", { 0 } },
    { 0, { FUNC_CHAR }, "becomespell", { 0 } },
    { 0, { FUNC_CHAR }, "becomespellbook", { 0 } },
    { 0, { FUNC_CHARTGT }, "blacktarget", { 0 } },
    { 0, { FUNC_PASSAGE, 3 }, "breakpassage", { 0 } },
    { 0, { FUNC_TEAM }, "callforhelp", { 0 } },
    { 0, { FUNC_CHAR }, "changearmor", { 0 } },
    { 0, { FUNC_CHAR }, "changetargetarmor", { 0 } },
    { 0, { FUNC_CHAR }, "changetargetclass", { 0 } },
    { 0, { FUNC_CHAR }, "changetile", { 0 } },
    { 0, { FUNC_CHAR }, "childdoactionoverride", { 0 } },
    { 0, { FUNC_CHAR }, "cleanup", { 0 } },
    { 0, { FUNC_MSG }, "clearendmessage", { 0 } },
    { 0, { FUNC_PASSAGE, 4 }, "clearmusicpassage", { 0 } },
    { 0, { FUNC_CHAR }, "clearwaypoints", { 0 } },
    { 0, { FUNC_PASSAGE, 1 }, "closepassage", { 0 } },
    { 0, { FUNC_CHAR }, "compass", { 0 } },
    { 0, { FUNC_CHAR }, "correctactionforhand", { 0 } },
    { 0, { FUNC_CHAR }, "costammo", { 0 } },
    { 0, { FUNC_CHARTGT }, "costtargetitemid", { 0 } },
    { 0, { FUNC_CHARTGT }, "costtargetmana", { 0 } },
    { 0, { FUNC_ORDER }, "createorder", { 0 } },
    { 0, { FUNC_CHARTGT }, "damagetarget", { 0 } },
    { 0, { FUNC_CHARTGT }, "dazetarget", { 0 } },
    { 0, { FUNC_MSG }, "debugmessage", { 0 } },
    { 0, { FUNC_CHAR }, "detachfromholder", { 0 } },
    { 0, { FUNC_GAME }, "disableexport", { 0 } },
    { 0, { FUNC_CHAR }, "disableinvictus", { 0 } },
    { 0, { FUNC_GAME }, "disablerespawn", { 0 } },
    { 0, { FUNC_CHAR }, "disaffirmcharacter", { 0 } },
    { 0, { FUNC_SPELL }, "disenchantall", { 0 } },
    { 0, { FUNC_SPELL }, "disenchanttarget", { 0 } },
    { 0, { FUNC_SPELL }, "dispeltargetenchantid", { 0 } },
    { 0, { FUNC_CHAR }, "doaction", { 0 } },
    { 0, { FUNC_CHAR }, "doactionoverride", { 0 } },
    { 0, { FUNC_END }, "donothing", { 0 } },
    { 0, { FUNC_GAME }, "drawbillboard", { 0 } },
    { 0, { FUNC_DROP }, "dropitems", { 0 } },
    { 0, { FUNC_DROP }, "dropkeys", { 0 } },
    { 0, { FUNC_DROP, SVAR_MONEY }, "dropmoney", { 0 } },
    { 0, { FUNC_DROP, SVAR_TARGET }, "droptargetkeys", { 0 } },
    { 0, { FUNC_DROP, SVAR_TARGET, SVAR_MONEY }, "droptargetmoney", { 0 } },
    { 0, { FUNC_DROP }, "dropweapons", { 0 } },
    { 0, { FUNC_ELSE }, "else", { 0 } },
    { 0, { FUNC_ENABLE }, "enableexport", { 0 } },
    { 0, { FUNC_ENABLE }, "enableinvictus", { 0 } },
    { 0, { FUNC_ENABLE }, "enablelistenskill", { 0 } },
    { 0, { FUNC_ENABLE }, "enablerespawn", { 0 } },
    { 0, { FUNC_SPELL }, "enchantchild", { 0 } },
    { 0, { FUNC_SPELL }, "enchanttarget", { 0 } },
    { 0, { FUNC_END }, "end", { 0 } },
    { 0, { FUNC_MODULE }, "endmodule", { 0 } },
    { 0, { FUNC_MIXED }, "equip", { 0 } },
    { 0, { FUNC_MIXED }, "findpath", { 0 } },
    { 0, { FUNC_PASSAGE, 6 }, "findtileinpassage", { 0 } },
    { 0, { FUNC_PASSAGE, 7 }, "flashpassage", { 0 } },
    { 0, { FUNC_CHARTGT }, "flashtarget", { 0 } },
    { 0, { FUNC_MIXED }, "flashvariable", { 0 } },
    { 0, { FUNC_MIXED }, "flashvariableheight", { 0 } },
    { 0, { FUNC_MIXED }, "followlink", { 0 } },
    { 0, { FUNC_GET }, "getattackturn", { 0 } },
    { 0, { FUNC_GET }, "getbumpheight", { 0 } },
    { 0, { FUNC_GET }, "getcontent", { 0 } },
    { 0, { FUNC_GET }, "getdamagetype", { 0 } },
    { 0, { FUNC_GET, SVAR_DISPLAY }, "getfogbottomlevel", { 0 } },
    { 0, { FUNC_GET, SVAR_DISPLAY }, "getfoglevel", { 0 } },
    { 0, { FUNC_GET, SVAR_STATE }, "getstate", { 0 } },
    { 0, { FUNC_GET, SVAR_TARGET }, "gettargetarmorprice", { 0 } },
    { 0, { FUNC_GET, SVAR_TARGET }, "gettargetcontent", { 0 } },
    { 0, { FUNC_GET, SVAR_TARGET }, "gettargetdamagetype", { 0 } },
    { 0, { FUNC_GET, SVAR_TARGET }, "gettargetdazetime", { 0 } },
    { 0, { FUNC_GET, SVAR_TARGET }, "gettargetgrogtime", { 0 } },
    { 0, { FUNC_GET, SVAR_TARGET }, "gettargetshieldprofiency", { 0 } },
    { 0, { FUNC_GET, SVAR_TARGET }, "gettargetstate", { 0 } },
    { 0, { FUNC_MAPGET }, "gettilexy", { 0 } },
    { 0, { FUNC_MAPGET }, "getwaterlevel", { 0 } },
    { 0, { FUNC_MAPGET }, "getxy", { 0 } },
    { 0, { FUNC_CHARTGT }, "givedexteritytotarget", { VAR_CHAR } },
    { 0, { FUNC_TEAM }, "giveexperiencetogoodteam", { VAR_SINT } },
    { 0, { FUNC_CHARTGT }, "giveexperiencetotarget", { VAR_CHAR } },
    { 0, { FUNC_CHARTGT }, "giveexperiencetotargetteam", { VAR_SINT } },
    { 0, { FUNC_CHARTGT }, "giveintelligencetotarget", { VAR_CHAR } },
    { 0, { FUNC_CHARTGT }, "givelifetotarget", { VAR_SINT } },
    { 0, { FUNC_CHARTGT }, "givemanaflowtotarget", { VAR_CHAR } },
    { 0, { FUNC_CHARTGT }, "givemanareturntotarget", { VAR_CHAR } },
    { 0, { FUNC_CHARTGT }, "givemanatotarget", { VAR_CHAR } },
    { 0, { FUNC_CHARTGT }, "givemoneytotarget", { VAR_SINT } },
    { 0, { FUNC_CHARTGT }, "giveskilltotarget", { VAR_SINT } },
    { 0, { FUNC_CHARTGT }, "givestrengthtotarget", { VAR_CHAR } },
    { 0, { FUNC_CHARTGT }, "givewisdomtotarget", { VAR_CHAR } },
    { 0, { FUNC_CHAR }, "gopoof", { 0 } },
    { 0, { FUNC_CHARTGT }, "grogtarget", { 0 } },
    { 0, { FUNC_SELF }, "healself", { 0 } },
    { 0, { FUNC_CHARTGT }, "healtarget", { 0 } },
    { 0, { FUNC_CHARTGT }, "identifytarget", { 0 } },
    
    { 0, { FUNC_CHAR }, "increaseammo", { 0 } },
    { 0, { FUNC_ORDER }, "issueorder", { 0 } },
    { 0, { FUNC_TEAM }, "joinevilteam", { 0 } },
    { 0, { FUNC_TEAM }, "joingoodteam", { 0 } },
    { 0, { FUNC_TEAM }, "joinnullteam", { 0 } },
    { 0, { FUNC_TEAM }, "jointargetteam", { 0 } },
    { 0, { FUNC_TEAM }, "jointeam", { 0 } },
    { 0, { FUNC_ORDER }, "keepaction", { 0 } },
    { 0, { FUNC_CHARTGT }, "killtarget", { 0 } },
    { 0, { FUNC_CHARTGT }, "kursetarget", { 0 } },
    { 0, { FUNC_MAKE }, "makeammoknown", { 0 } },
    { 0, { FUNC_MAKE }, "makecrushinvalid", { 0 } },
    { 0, { FUNC_MAKE }, "makecrushvalid", { 0 } },
    { 0, { FUNC_MAKE }, "makenameknown", { 0 } },
    { 0, { FUNC_MAKE }, "makenameunknown", { 0 } },
    { 0, { FUNC_MAKE }, "makesimilarnamesknown", { 0 } },
    { 0, { FUNC_MAKE }, "makeusageknown", { 0 } },
    { 0, { FUNC_CHARTGT }, "morphtotarget", { 0 } },
    { 0, { FUNC_CHAR }, "notanitem", { 0 } },
    { 0, { FUNC_PASSAGE, 0 }, "openpassage", { 0 } },
    { 0, { FUNC_ORDER }, "orderspecialid", { 0 } },
    { 0, { FUNC_ORDER }, "ordertarget", { 0 } },
    { 0, { FUNC_MAP }, "pitsfall", { 0 } },
    { 0, { FUNC_MAP }, "pitskill", { 0 } },
    { 0, { FUNC_SOUND }, "playfullsound", { 0 } },
    { 0, { FUNC_SOUND }, "playmusic", { 0 } },
    { 0, { FUNC_SOUND }, "playsound", { 0 } },
    { 0, { FUNC_SOUND }, "playsoundlooped", { 0 } },
    { 0, { FUNC_SOUND }, "playsoundvolume", { 0 } },
    { 0, { FUNC_CHARTGT }, "pooftarget", { 0 } },
    { 0, { FUNC_CHAR }, "presslatchbutton", { 0 } },
    { 0, { FUNC_CHARTGT }, "presstargetlatchbutton", { 0 } },
    { 0, { FUNC_CHARTGT }, "pumptarget", { 0 } },
    { 0, { FUNC_CHAR }, "reaffirmcharacter", { 0 } },
    { 0, { FUNC_CHAR }, "respawncharacter", { 0 } },
    { 0, { FUNC_CHARTGT }, "respawntarget", { 0 } },
    { 0, { FUNC_CHARTGT }, "restocktargetammoidall", { 0 } },
    { 0, { FUNC_CHARTGT }, "restocktargetammoidfirst", { 0 } },
    { 0, { FUNC_CHAR }, "run", { 0 } },
    { 0, { FUNC_MSG, 1 }, "sendmessage", { 0 } },
    { 0, { FUNC_MSG, 2 }, "sendmessagenear", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setalpha", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setblueshift", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setbumpheight", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setbumpsize", { 0 } },
    { 0, { FUNC_SET }, "setchildammo", { 0 } },
    { 0, { FUNC_SET }, "setchildcontent", { 0 } },
    { 0, { FUNC_SET }, "setchildstate", { 0 } },
    { 0, { FUNC_SET }, "setcontent", { 0 } },
    { 0, { FUNC_SET }, "setdamagethreshold", { 0 } },
    { 0, { FUNC_SET }, "setdamagetime", { 0 } },
    { 0, { FUNC_SET }, "setdamagetype", { 0 } },
    { 0, { FUNC_SET }, "setenchantboostvalues", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setflyheight", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setfogbottomlevel", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setfoglevel", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setfogtad", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setframe", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setgreenshift", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setlight", { 0 } },
    { 0, { FUNC_SET, SVAR_MONEY }, "setmoney", { 0 } },
    { 0, { FUNC_SET, SVAR_SOUND, SVAR_PASSAGE }, "setmusicpassage", { 0 } },
    { 0, { FUNC_SET }, "setoldtarget", { 0 } },
    { 0, { FUNC_SET }, "setownertotarget", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setredshift", { 0 } },
    { 0, { FUNC_SET }, "setreloadtime", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setshadowsize", { 0 } },
    { 0, { FUNC_SET }, "setspeech", { 0 } },
    { 0, { FUNC_SET, SVAR_DISPLAY }, "setspeedpercent", { 0 } },
    { 0, { FUNC_SET, SVAR_STATE }, "setstate", { VAR_CHAR } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargetammo", { VAR_CHAR } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargetquestlevel", { VAR_CHAR } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargetreloadtime", { VAR_CHAR } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargetsize", { VAR_CHAR } },
    { 0, { FUNC_SET, SVAR_TARGET, FUNC_PASSAGE }, "settargettoblahinpassage", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettochild", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettodistantenemy", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettolastitemused", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettoleader", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettolowesttarget", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettonearbyenemy", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettonearbymeleeweapon", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettonearestblahid", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettonearestenemy", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettonearestfriend", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettonearestlifeform", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettooldtarget", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettoowner", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET, FUNC_PASSAGE }, "settargettopassageid", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettorider", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettoself", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettotargetlefthand", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettotargetofleader", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettotargetrighthand", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettowhoeverattacked", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettowhoeverbumped", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettowhoevercalledforhelp", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettowhoeverhealed", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettowhoeverisholding", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET, FUNC_PASSAGE }, "settargettowhoeverisinpassage", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettowhoeverwashit", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettowideblahid", { 0 } },
    { 0, { FUNC_SET, SVAR_TARGET }, "settargettowideenemy", { 0 } },
    { 0, { FUNC_SET, SVAR_MAP }, "settilexy", { 0 } },
    { 0, { FUNC_SET }, "settime", { 0 } },
    { 0, { FUNC_SET, SVAR_TURNMODE }, "setturnmodetospin", { 0 } },
    { 0, { FUNC_SET, SVAR_TURNMODE }, "setturnmodetovelocity", { 0 } },
    { 0, { FUNC_SET, SVAR_TURNMODE }, "setturnmodetowatch", { 0 } },
    { 0, { FUNC_SET, SVAR_TURNMODE }, "setturnmodetowatchtarget", { 0 } },
    { 0, { FUNC_SET }, "setvolumenearestteammate", { 0 } },
    { 0, { FUNC_SET, SVAR_MAP }, "setwaterlevel", { 0 } },
    { 0, { FUNC_SET, SVAR_MAP }, "setweathertime", { 0 } },
    { 0, { FUNC_SET, SVAR_MAP }, "setxy", { 0 } },
    { 0, { FUNC_SHOW }, "showblipxy", { 0 } },
    { 0, { FUNC_SHOW }, "showmap", { 0 } },
    { 0, { FUNC_SHOW }, "showtimer", { 0 } },
    { 0, { FUNC_SHOW }, "showyouarehere", { 0 } },
    { 0, { FUNC_CHAR }, "sneak", { 0 } },
    { 0, { FUNC_CHAR }, "sparkleicon", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnattachedcharacter", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnattachedfacedparticle", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnattachedholderparticle", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnattachedparticle", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnattachedsizedparticle", { 0 } },
    { 0, { FUNC_SPAWN }, "spawncharacter", { 0 } },
    { 0, { FUNC_SPAWN }, "spawncharacterxyz", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnexactcharacterxyz", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnexactchaseparticle", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnexactparticle", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnexactparticleendspawn", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnparticle", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnpoof", { 0 } },
    { 0, { FUNC_SPAWN }, "spawnpoofspeedspacingdamage", { 0 } },
    { 0, { FUNC_CHAR }, "stop", { 0 } },
    { 0, { FUNC_SOUND }, "stopmusic", { 0 } },
    { 0, { FUNC_SOUND }, "stopsound", { 0 } },
    { 0, { FUNC_CHARTGT }, "stoptargetmovement", { 0 } },
    { 0, { FUNC_MIXED }, "takepicture", { 0 } },
    { 0, { FUNC_CHARTGT }, "targetdamageself", { 0 } },
    { 0, { FUNC_CHARTGT }, "targetdoaction", { 0 } },
    { 0, { FUNC_CHARTGT }, "targetdoactionsetframe", { 0 } },
    { 0, { FUNC_TEAM }, "targetjointeam", { 0 } },
    { 0, { FUNC_CHARTGT }, "targetpayforarmor", { 0 } },
    { 0, { FUNC_TELEPORT }, "teleport", { 0 } },
    { 0, { FUNC_TELEPORT }, "teleporttarget", { 0 } },
    { 0, { FUNC_ORDER }, "translateorder", { 0 } },
    { 0, { FUNC_SPELL }, "undoenchant", { 0 } },
    { 0, { FUNC_CHAR }, "unkeepaction", { 0 } },
    { 0, { FUNC_CHARTGT }, "unkursetarget", { 0 } },
    { 0, { FUNC_CHARTGT }, "unkursetargetinventory", { 0 } },
    { 0, { FUNC_CHAR }, "unsparkleicon", { 0 } },
    { 0, { FUNC_CHAR }, "walk", { 0 } },
    { 0, { 0 }, "", { 0 } },
};

static SCRIPT_CONST_T ScrConst[] = 
{
    { SVAR_BLAH,   1, "blahdead" },
    { SVAR_BLAH,   2, "blahenemies" },
    { SVAR_BLAH,   4, "blahfriends" },
    { SVAR_BLAH,   8, "blahitems" },
    { SVAR_BLAH,  16, "blahinvertid" },
    { SVAR_BLAH,  32, "blahplayers" },"
    { SVAR_BLAH,  64, "blahskill" },
    { SVAR_BLAH, 128, "blahquest" },
    { SVAR_STATE,   0, "stateparry" },
    { SVAR_STATE,   1, "statewander" },
    { SVAR_STATE,   2, "stateguard" },
    { SVAR_STATE,   3, "statefollow" },
    { SVAR_STATE,   4, "statesurround" },
    { SVAR_STATE,   5, "stateretreat" },
    { SVAR_STATE,   6, "statecharge" },
    { SVAR_STATE,   7, "statecombat" },
    { SVAR_GRIP,    4, "griponly" },
    { SVAR_GRIP,    4, "gripleft" },
    { SVAR_GRIP,    8, "gripright" },
    { SVAR_SPAWN,   0, "spawnorigin" },
    { SVAR_SPAWN,   1, "spawnlast" },
    { SVAR_LATCH,   1, "latchleft" },
    { SVAR_LATCH,   2, "latchright" },
    { SVAR_LATCH,   4, "latchjump" },
    { SVAR_LATCH,   8, "latchaltleft" },
    { SVAR_LATCH,  16, "latchaltright" },
    { SVAR_LATCH,  32, "latchpackleft" },
    { SVAR_LATCH,  64, "latchpackright" },
    { SVAR_DAMAGE,  0, "damageslash" },
    { SVAR_DAMAGE,  1, "damagecrush" },
    { SVAR_DAMAGE,  2, "damagepoke" },
    { SVAR_DAMAGE,  3, "damageholy" },
    { SVAR_DAMAGE,  4, "damageevil" },
    { SVAR_DAMAGE,  5, "damagefire" },
    { SVAR_DAMAGE,  6, "damageice" },
    { SVAR_DAMAGE,  7, "damagezap" },
    { SVAR_ACTION, 'DA', "actionda" },
    { SVAR_ACTION, 'DB', "actiondb" },
    { SVAR_ACTION, 'DC', "actiondc" },
    { SVAR_ACTION, 'DD', "actiondd" },
    { SVAR_ACTION, 'UA', "actionua" },
    { SVAR_ACTION, 'UB', "actionub" },
    { SVAR_ACTION, 'UC', "actionuc" },
    { SVAR_ACTION, 'UD', "actionud" },
    { SVAR_ACTION, 'TA', "actionta" },
    { SVAR_ACTION, 'TB', "actiontb" },
    { SVAR_ACTION, 'TC', "actiontc" },
    { SVAR_ACTION, 'TD', "actiontd" },
    { SVAR_ACTION, 'CA', "actionca" },
    { SVAR_ACTION, 'CB', "actioncb" },
    { SVAR_ACTION, 'CC', "actioncc" },
    { SVAR_ACTION, 'CD', "actioncd" },
    { SVAR_ACTION, 'SA', "actionsa" },
    { SVAR_ACTION, 'SB', "actionsb" },
    { SVAR_ACTION, 'SC', "actionsc" },
    { SVAR_ACTION, 'SD', "actionsd" },
    { SVAR_ACTION, 'BA', "actionba" },
    { SVAR_ACTION, 'BB', "actionbb" },
    { SVAR_ACTION, 'BC', "actionbc" },
    { SVAR_ACTION, 'BD', "actionbd" },
    { SVAR_ACTION, 'LA', "actionla" },
    { SVAR_ACTION, 'LB', "actionlb" },
    { SVAR_ACTION, 'LC', "actionlc" },
    { SVAR_ACTION, 'LD', "actionld" },
    { SVAR_ACTION, 'XA', "actionxa" },
    { SVAR_ACTION, 'XB', "actionxb" },
    { SVAR_ACTION, 'XC', "actionxc" },
    { SVAR_ACTION, 'XD', "actionxd" },
    { SVAR_ACTION, 'FA', "actionfa" },
    { SVAR_ACTION, 'FB', "actionfb" },
    { SVAR_ACTION, 'FC', "actionfc" },
    { SVAR_ACTION, 'FD', "actionfd" },
    { SVAR_ACTION, 'PA', "actionpa" },
    { SVAR_ACTION, 'PB', "actionpb" },
    { SVAR_ACTION, 'PC', "actionpc" },
    { SVAR_ACTION, 'PD', "actionpd" },
    { SVAR_ACTION, 'EA', "actionea" },
    { SVAR_ACTION, 'EB', "actioneb" },
    { SVAR_ACTION, 'RA', "actionra" },
    { SVAR_ACTION, 'ZA', "actionza" },
    { SVAR_ACTION, 'ZB', "actionzb" },
    { SVAR_ACTION, 'ZC', "actionzc" },
    { SVAR_ACTION, 'ZD', "actionzd" },
    { SVAR_ACTION, 'WA', "actionwa" },
    { SVAR_ACTION, 'WB', "actionwb" },
    { SVAR_ACTION, 'WC', "actionwc" },
    { SVAR_ACTION, 'WD', "actionwd" },
    { SVAR_ACTION, 'JA', "actionja" },
    { SVAR_ACTION, 'JB', "actionjb" },
    { SVAR_ACTION, 'JC', "actionjc" },
    { SVAR_ACTION, 'HA', "actionha" },
    { SVAR_ACTION, 'HB', "actionhb" },
    { SVAR_ACTION, 'HC', "actionhc" },
    { SVAR_ACTION, 'HD', "actionhd" },
    { SVAR_ACTION, 'KA', "actionka" },
    { SVAR_ACTION, 'KB', "actionkb" },
    { SVAR_ACTION, 'KC', "actionkc" },
    { SVAR_ACTION, 'KD', "actionkd" },
    { SVAR_ACTION, 'MA', "actionma" },
    { SVAR_ACTION, 'MB', "actionmb" },
    { SVAR_ACTION, 'MC', "actionmc" },
    { SVAR_ACTION, 'MD', "actionmd" },
    { SVAR_ACTION, 'ME', "actionme" },
    { SVAR_ACTION, 'MF', "actionmf" },
    { SVAR_ACTION, 'MG', "actionmg" },
    { SVAR_ACTION, 'MH', "actionmh" },
    { SVAR_ACTION, 'MI', "actionmi" },
    { SVAR_ACTION, 'MJ', "actionmj" },
    { SVAR_ACTION, 'MK', "actionmk" },
    { SVAR_ACTION, 'ML', "actionml" },
    { SVAR_ACTION, 'MM', "actionmm" },
    { SVAR_ACTION, 'MN', "actionmn" },
    { SVAR_EXP, 0, "expsecret" },
    { SVAR_EXP, 1, "expquest" },
    { SVAR_EXP, 2, "expdare" },
    { SVAR_EXP, 3, "expkill" },
    { SVAR_EXP, 4, "expmurder" },
    { SVAR_EXP, 5, "exprevenge" },
    { SVAR_EXP, 6, "expteamwork" },
    { SVAR_EXP, 7, "exproleplay" },
    { SVAR_MSG, 0, "messagedeath" },
    { SVAR_MSG, 1, "messagehate" },
    { SVAR_MSG, 2, "messageouch" },
    { SVAR_MSG, 3, "messagefrag" },
    { SVAR_MSG, 4, "messageaccident" },
    { SVAR_MSG, 5, "messagecostume" },
    { SVAR_ORDER, 0, "ordermove" },
    { SVAR_ORDER, 1, "orderattack" },
    { SVAR_ORDER, 2, "orderassist" },
    { SVAR_ORDER, 3, "orderstand" },
    { SVAR_ORDER, 4, "orderterrain" },
    { SVAR_COLXP, 0, "white" },
    { SVAR_COLXP, 1, "red" },
    { SVAR_COLXP, 2, "yellow" },
    { SVAR_COLXP, 3, "green" },
    { SVAR_COLXP, 4, "blue" },
    { SVAR_COLXP, 0, "purple" },
    { SVAR_FX,   1, "fxnoreflect" },
    { SVAR_FX,   2, "fxdrawreflect" },
    { SVAR_FX,   4, "fxanim" },
    { SVAR_FX,   8, "fxwater" },
    { SVAR_FX,  16, "fxbarrier" },
    { SVAR_FX,  32, "fximpass" },
    { SVAR_FX,  64, "fxdamage" },
    { SVAR_FX, 128, "fxslippy" },
    { SVAR_TEAM,  0, "teama" },
    { SVAR_TEAM,  1, "teamb" },
    { SVAR_TEAM,  2, "teamc" },
    { SVAR_TEAM,  3, "teamd" },
    { SVAR_TEAM,  4, "teame" },
    { SVAR_TEAM,  5, "teamf" },
    { SVAR_TEAM,  6, "teamg" },
    { SVAR_TEAM,  7, "teamh" },
    { SVAR_TEAM,  8, "teami" },
    { SVAR_TEAM,  9, "teamj" },
    { SVAR_TEAM, 10, "teamk" },
    { SVAR_TEAM, 11, "teaml" },
    { SVAR_TEAM, 12, "teamm" },
    { SVAR_TEAM, 13, "teamn" },
    { SVAR_TEAM, 14, "teamo" },
    { SVAR_TEAM, 15, "teamp" },
    { SVAR_TEAM, 16, "teamq" },
    { SVAR_TEAM, 17, "teamr" },
    { SVAR_TEAM, 18, "teams" },
    { SVAR_TEAM, 19, "teamt" },
    { SVAR_TEAM, 20, "teamu" },
    { SVAR_TEAM, 21, "teamv" },
    { SVAR_TEAM, 22, "teamw" },
    { SVAR_TEAM, 23, "teamx" },
    { SVAR_TEAM, 24, "teamy" },
    { SVAR_TEAM, 25, "teamz" },
    { SVAR_INVENT, 1, "inventory" },
    { SVAR_INVENT, 2, "left" },
    { SVAR_INVENT, 3, "right" },
    { SVAR_GAMEL, 0, "easy" },
    { SVAR_GAMEL, 1, "normal" },
    { SVAR_GAMEL, 2, "hard" },
    { 0, 0, "" }   
};

static SCRIPT_ARG_T ScrArgVal[] = 
{
    { { FUNC_DATE, 1 } , "dateday" }, 
    { { FUNC_DATE, 2 } , "datemonth" }, 
    { { FUNC_GAMEL, 4 } , "difficulty" },
    { { FUNC_GOTO, 1 } , "gotodistance" },
    { { FUNC_GOTO, 3 } , "gotox" },
    { { FUNC_GOTO, 4 } , "gotoy" },
    { { FUNC_LEADER, 1 } , "leaderdistance" },
    { { FUNC_LEADER, 2 } , "leaderturn" },
    { { FUNC_LEADER, 3 } , "leaderx" },
    { { FUNC_LEADER, 4 } , "leadery" },
    { { FUNC_OWNER, 1 } , "ownerdistance" },
    { { FUNC_OWNER, 2 } , "ownerturn" },
    { { FUNC_OWNER, 3 } , "ownerx" },
    { { FUNC_OWNER, 4 } , "ownery" },
    { { FUNC_OWNER, 5 } , "ownerturnto" },
    // @todo: Check if this functions need an argument
    { { FUNC_MIXED, 1 } , "passage" },
    { { FUNC_MIXED, 2 } , "rand" },
    // Self
    { { FUNC_SELF,  1 } , "selfaccel" },
    { { FUNC_SELF,  2 } , "selfaltitude" },
    { { FUNC_SELF,  3 } , "selfammo" },
    { { FUNC_SELF,  4 } , "selfattached" },
    { { FUNC_SELF,  5 } , "selfcontent" },
    { { FUNC_SELF,  6 } , "selfcounter" },
    { { FUNC_SELF,  7 } , "selfdex" },
    { { FUNC_SELF,  8 } , "selfhateid" },
    { { FUNC_SELF,  9 } , "selfid" },
    { { FUNC_SELF, 10 } , "selfindex" },
    { { FUNC_SELF, 11 } , "selfint" },
    { { FUNC_SELF, 12 } , "selflevel" },
    { { FUNC_SELF, 13 } , "selflife" },
    { { FUNC_SELF, 14 } , "selfmana" },
    { { FUNC_SELF, 15 } , "selfmanaflow" },
    { { FUNC_SELF, SVAR_MONEY } , "selfmoney" },
    { { FUNC_SELF, 17 } , "selfmorale" },
    { { FUNC_SELF, 18 } , "selforder" },
    { { FUNC_SELF, 19 } , "selfspawnx" },
    { { FUNC_SELF, 20 } , "selfspawny" },
    { { FUNC_SELF, 21 } , "selfstate" },
    { { FUNC_SELF, 22 } , "selfstr" },
    { { FUNC_SELF, 23 } , "selfturn" },
    { { FUNC_SELF, 24 } , "selfwis" },
    { { FUNC_SELF, 25 } , "selfx" },
    { { FUNC_SELF, 26 } , "selfy" },
    { { FUNC_SELF, 27 } , "selfz" },
    // More mixed functions
    { { FUNC_MIXED, 3 } , "spawndistance" },
    { { FUNC_MIXED, 4 } , "swingturn" },
    // Target values 
    { {FUNC_CHARTGT,  1 } , "targetaltitude" },
    { {FUNC_CHARTGT,  2 } , "targetammo" },
    { {FUNC_CHARTGT,  3 } , "targetarmor" },
    { {FUNC_CHARTGT,  4 } , "targetdex" },
    { {FUNC_CHARTGT,  5 } , "targetdistance" },
    { {FUNC_CHARTGT,  6 } , "targetexp" },
    { {FUNC_CHARTGT,  7 } , "targetint" },
    { {FUNC_CHARTGT,  8 } , "targetlevel" },
    { {FUNC_CHARTGT,  9 } , "targetlife" },
    { {FUNC_CHARTGT, 10 } , "targetmana" },
    { {FUNC_CHARTGT, 11 } , "targetmanaflow" },
    { {FUNC_CHARTGT, 12 } , "targetmaxlife" },
    { {FUNC_CHARTGT, SVAR_MONEY } , "targetmoney" },
    { {FUNC_CHARTGT, 14 } , "targetreloadtime" },
    { {FUNC_CHARTGT, 15 } , "targetspeedx" },
    { {FUNC_CHARTGT, 16 } , "targetspeedy" },
    { {FUNC_CHARTGT, 17 } , "targetspeedz" },
    { {FUNC_CHARTGT, 18 } , "targetstr" },
    { {FUNC_CHARTGT, 19 } , "targetteam" },
    { {FUNC_CHARTGT, 20 } , "targetturn" },
    { {FUNC_CHARTGT, 21 } , "targetturnfrom" },
    { {FUNC_CHARTGT, 22 } , "targetturnto" },
    { {FUNC_CHARTGT, 23 } , "targetwis" },
    { {FUNC_CHARTGT, 24 } , "targetx" },
    { {FUNC_CHARTGT, 25 } , "targety" },
    { {FUNC_CHARTGT, 26 } , "targetz" },
    // time
    { { FUNC_TIME, 1 } , "timehours" },
    { { FUNC_TIME, 2 } , "timeminutes" },
    { { FUNC_TIME, 3 } , "timeseconds" },
    // more mixed functions
    { { FUNC_MIXED, 4 } , "weight" },
    { { FUNC_MIXED, 5 } , "xydistance" },
    { { FUNC_MIXED, 6 } , "xyturnto" },
    { { 0, 0 } , "" }
};

// Arguments for functions
static SCRIPT_ARG_T ScrArgs[] = 
{
    { { FUNC_TMP, 0 } , "tmpx" },
    { { FUNC_TMP, 1 } , "tmpy" },
    { { FUNC_TMP, 2 } , "tmpdist" },
    { { FUNC_TMP, 2 } , "tmpdistance" },
    { { FUNC_TMP, 3 } , "tmpturn" },
    { { FUNC_TMP, 4 } , "tmpargument" },
    { { 0, 0 } , "" }
};

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     scriptLoad
 * Function:
 *     Loads a script from     { 0, { 0 }, "given file, returns the number of the script to be
 *         { 0, { 0 }, "given as argument for the function 'scriptRun()' 
 * Input:
 *     char_no:   Run script for this character
 *     script_no: Run script with this number
 *     why:       Why is the script     { 0, { 0 }, "called ? (Replaces 'alerts')  
 */
void scriptLoad(int char_no, int script_no, int why)
{
}

/*
 * Name:
 *     scriptRun
 * Function:
 *     Runs the script for     { 0, { 0 }, "given character
 * Input:
 *     char_no:   Run script for this character
 *     script_no: Run script with this number
 *     why:       Why is the script     { 0, { 0 }, "called ? (Replaces 'alerts')  
 */
void scriptRun(int char_no, int script_no, int why)
{
}