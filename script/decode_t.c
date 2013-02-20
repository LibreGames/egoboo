/*******************************************************************************
*  DECODE_T.C                                                                  *
*	   - Decodes an uncompressed EOB1-Info-File                                *
*                                                                              *
*   JUST ANOTHER DUNGEON                 								       *
*       Copyright (C) 2010  Paul Mueller <pmtech@swissonline.ch>               *
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
*   This program is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*   GNU Library General Public License for more details.                       *
*                                                                              *
*   You should have received a copy of the GNU General Public License          *
*   along with this program; if not, write to the Free Software                *
*   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
*******************************************************************************/

/*******************************************************************************
* INCLUDES                                                                     *
*******************************************************************************/

#include <stdio.h>
#include <string.h>


#include "script.h"
#include "evcoddef.h" 


#include "decode_t.h"       /* Own header */

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef char *( *DECODE_PROC)(char *code, char *textbuf);

typedef struct {

    short int pos;
    char      triggerbits;
    short int codestart;

} EVENT_DESC;

/*******************************************************************************
* DATA                                                                         *
*******************************************************************************/
/* ---------- Main Functions ---------- */
/*
static SC_PROC ScriptProc[] = {

    {  9, "Encounter", 0xE6, 0, 1, { SC_CHAR } },
    {  4, "Wait", 0xE5, 0, 1, { SC_SMALLINT } },
    { 12, "UpdateScreen", 0xE4 },
    {  8, "TextMenu", 0xE3, 0 }, // FIXME: Add actual code definition 
    { 21, "SpecialWindowPictures", 0xE2, 0, 1, { SC_CHAR } },
    { 0 }
};
*/

static SC_PROC CompareFunc[] = 
{
    { 11, "EventChoice", EVENT_CHOICE },
    { 17, "EventTriggerFlags", EVENT_TRIGGERFLAGS },
    { 14, "PartyDirection", EVENT_PARTYDIRECTION },
    { 14, "PartyNumMember", EVENT_WHOLEPARTY },
    { 11, "MazeGetWall", EVENT_WHOLEWALL, 0, 1, { SC_SMALLINT } },
    { 15, "ItemNoCountMaze", EVENT_ITEMNO, 0, 2, { SC_SMALLINT, SC_SMALLINT } },
    { 16, "MonsterCountMaze", EVENT_MONSTER, 0, 2, { SC_CHAR, SC_SMALLINT } },
        /* TODO: Count all monster types, as long as first argument >= 0, then second argument = SC_CHAR */
    { 18, "ItemExactCountMaze", EVENT_THISITEMNO, 0, 2, { SC_SMALLINT, SC_SMALLINT } }, /* itemno, pos */
    { 15, "PartyCountItems", EVENT_PARTY, EVENT_ITEMNO, 2, { SC_SMALLINT, SC_SMALLINT } },
    { 12, "PartyIsAtPos", EVENT_PARTY, EVENT_PLAYERPOS, 1, { SC_SMALLINT } },
    { 13, "FlagGetGlobal", EVENT_GLOBALFLAG, 0, 1, { SC_CHAR } },
    { 12, "FlagGetLevel", EVENT_LEVELFLAG, 0, 1, { SC_CHAR } },
    { 17, "PartyUsedItemType", EVENT_USEDITEM, EVENT_ITEMTYPE },
    { 18, "PartyUsedItemValue", EVENT_USEDITEM, EVENT_ITEMVALUE },  /* Is 'sub_type' */
    { 15, "PartyUsedItemNo", EVENT_USEDITEM, EVENT_ITEMNO },
    { 17, "PartyUsedItemUnID", EVENT_USEDITEM, EVENT_UNIDSTRING, { SC_STRING } },
    { 15, "PartyUsedItemID", EVENT_USEDITEM, EVENT_ITEMIDSTRING, { SC_STRING } },  /* TODO: Use String-Number */
    { 15, "MazeGetWallSide", EVENT_WALLPART, 0, 2, { SC_CHAR, SC_SMALLINT } },    /* Type-No, Position       */
    { 17, "PartyGetMemberPic", EVENT_MEMBERPICNO, 0, 1, { SC_CHAR } },  
    { 12, "PartyGetRace", EVENT_MEMBERRACE, 0, 1, { SC_CHAR } },  
    { 13, "PartyGetClass", EVENT_MEMBERCLASS, 0, 1, { SC_CHAR } }, 
    {  6, "Random", EVENT_RANDOMVALUE, 0, 3, { SC_CHAR, SC_CHAR, SC_CHAR } },  /* Base, dice, numthrow   */
    { 12, "TriggerItemNo", EVENT_TRIGGERITEM, EVENT_ITEMNO },  
    { 14, "TriggerItemType", EVENT_TRIGGERITEM, EVENT_ITEMTYPE },
    { 15, "TriggerItemClass", EVENT_TRIGGERITEM, EVENT_ITEMCLASS },
    { 15, "TriggerItemValue", EVENT_TRIGGERITEM, EVENT_ITEMVALUE },
    { 11, "SpellGetAct", EVENT_SPELL },
    { 17, "PartyGetVisible", EVENT_PARTYVISIBLE },
    {  9, "CompValue", EVENT_COMPAREVALUE, 0, 1, { SC_SMALLINT } },  
    /* For display of "comparision-strings" */
    { 0 }
};

static SC_PROC CompareOperation[] =
{
    {  1, "=", SC_COMPAREEQUAL },
    {  2, "<>", SC_COMPARENOTEQUAL },
    {  1, "<", SC_COMPARELESS },
    {  2, "<=", SC_COMPARELESSEQUAL },
    {  1, ">", SC_COMPAREGREATER },
    {  2, ">=", SC_COMPAREGREATEREQUAL },
    {  3, "AND", SC_COMPAREAND },
    {  2, "OR", SC_COMPAREOR },
    { 0 }
};

static char InfBuffer[12000];
static int  InfSize;             /* Size of data in 'InfBuffer'     */
static char PrintBuf[4096];    /* Buffer for line to write to file */
static FILE *FOut;
static EVENT_DESC *pEventDesc;  /* Pointer on event-descriptors     */
static char EndProc;
static int   EventCallStackIdx;
/* static char *EventCallStack[20]; */

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

/*
 * Name:
 *     decodeArguments
 * Description:
 *     Decodes the arguments 
 * Input:
 *     code *: Pointer on code to decode to arguments
 *     pd *:   Pointer on descriptor with arguments 
 *     dest *: Where to print the decoded arguments 
 * Output:
 *      Number of bytes used for arguments     
 */
static int decodeArguments(char *code, SC_PROC *pd, char *dest)
{

    char num_arg;
    int  num_byte;
    char valstr[100];

    
    num_byte = 0;       /* Counter for bytes used for arguments */
    strcat(dest, "(");  /* Open bracket for argument            */
    
    for (num_arg = 0; num_arg <  pd -> argcount; num_arg++)
    {
        switch(pd -> args[num_arg])
        {
            case SC_CHAR:
                sprintf(valstr, "%d", (int)*code);
                code++;
                num_byte++; 
                strcat(dest, valstr);                
                break;
            case SC_SMALLINT:
                sprintf(valstr, "%hi", *(short int *)code);
                code += 2;
                num_byte += 2;
                strcat(dest, valstr);                  
                break;
            case SC_STRING:
                num_byte = strlen(code) + 1;
                strcat(dest, "\"");  
                strcat(dest, code);
                strcat(dest, "\"");                  
                code += num_byte;                
                break;

        }

        if (num_arg < (pd -> argcount - 1)) {
            strcat(dest, ", ");
        }

    }

    strcat(dest, ")");       /* Closing bracket for arguments */

    return num_byte;
}

/*
 * Name:
 *     decodeFunc
 * Description:
 *     Decodes all procedures to string
 * Input:
 *     main_code: Decode this function
 *     sub_code:  Decode this subcode   (may be 0, by caller)
 *     code *:    Pointer on arguments
 *     textbuf *: Where to print the strings
 * Output:
 *     Number of bytes used for function
 */
static int decodeFunc(char main_code, char sub_code, char *code, char *textbuf)
{
    SC_PROC  *cp;


    cp = CompareFunc;

    while(cp -> len > 0)
    {
        if (cp -> code == main_code && cp -> sub_code == sub_code)
        {
            strcpy(textbuf, cp -> name);
            return decodeArguments(code, cp, textbuf); /* May be no arguments at all */
        }

        cp++;
    }

    EndProc = 2;    /* Stop decoding with error */

    return 0;
}

/*
 * Name:
 *     decodeComparision
 * Description:
 *     Prints a procedure and its arguments to 'textbuf'
 * Input:
 *     comp_code:    Code for comparision
 *     first_arg *:  String with first argument for comparision
 *     second_arg *: String with first argument for comparision
 *     textbuf *:    Where to print
 * Output:
 *     Number of bytes used for condition statement
 */
static void decodeComparision(char comp_code, char *first_arg, char *second_arg, char *textbuf)
{
    SC_PROC  *cp;


    cp = CompareOperation;
    while(cp -> len > 0)
    {
        if (cp -> code == comp_code)
        {
            sprintf(textbuf, "%s %s %s ", first_arg, cp -> name, second_arg);
            return;

        }

        cp++;
    }

    EndProc = 2;    /* Stop decoding with error */
}

/*
 * Name:
 *     eventDecodePrintProc
 * Description:
 *     Prints a procedure and its arguments to 'textbuf'
 * Input:
 *     code *:    Pointer on arguments
 *     sub_code:  Number of subcode, if any
 *     sp *:      Pointer on possible procedures
 *     textbuf *: Where to print
 * Output:
 *     Number of bytes used for condition statement
 */
static int eventDecodePrintProc(char *code, char sub_code, SC_PROC *sp, char *textbuf)
{
    while(sp -> len > 0)
    {
        if (sp -> sub_code == sub_code)
        {
            strcat(textbuf, sp -> name);        /* Main name or additional name */
            return decodeArguments(code, sp, textbuf);
        }

        sp++;
    }

    EndProc = 2;    /* Stop decoding with error */

    return 0;
}

/*
 * Name:
 *     decodeSetWall
 * Description:
 *     Decodes the procedures "SetWall"
 * Input:
 *     code *:    Code to translate
 *     textbuf *: Where to print
 * Output:
 *     Number of bytes used for condition statement  
 */
static char *decodeSetWall(char *code, char *textbuf)
{

    static SC_PROC setwall[] =
    {
        { 5, "Whole", 0xFF, EVENT_WHOLEWALL, 2, { SC_SMALLINT, SC_CHAR } },
        { 4, "Part", 0xFF, EVENT_WALLPART, 3, { SC_SMALLINT, SC_CHAR, SC_CHAR } },
        { 8, "PartyDir", 0xFF, EVENT_PARTYDIRECTION, 1, { SC_CHAR } },
        { 0 }
    };

    char sub_code;


    sub_code = *code;
    code++;

    strcat(textbuf, "SetWall");

    code += eventDecodePrintProc(code, sub_code, setwall, textbuf);
    strcat(textbuf, "\n");

    return code;

}

/*
 * Name:
 *     decodeSwitchWall
 * Description:
 *     Decodes the 'SwitchWall' 
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeSwitchWall(char *code, char *textbuf)
{

    static SC_PROC switchwall[] = {
        { 5, "Whole", 0xFE, EVENT_WHOLEWALL, 3, { SC_SMALLINT, SC_CHAR, SC_CHAR } }, 
        { 4, "Part", 0xFE, EVENT_WALLPART, 4, { SC_SMALLINT, SC_CHAR, SC_CHAR, SC_CHAR } }, 
        { 4, "Door", 0xFE, EVENT_DOOR, 1, { SC_SMALLINT } },
        { 0 }
    };
    
    char sub_code;
    
    
    sub_code = *code;      
    code++;
    
    strcat(textbuf, "SwitchWall");
    /* TODO: Print identation */
    code += eventDecodePrintProc(code, sub_code, switchwall, textbuf);
    strcat(textbuf, "\n");
    
    return code;

}

/*
 * Name:
 *     decodeOpenDoor
 * Description:
 *     Decodes the 'OpenDoor' 
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeOpenDoor(char *code, char *textbuf)
{

    static SC_PROC opendoor = { 8, "OpenDoor", 0xFD, 0, 1, { SC_SMALLINT } };

    strcat(textbuf, opendoor.name);
    
    code += decodeArguments(code, &opendoor, textbuf);
    strcat(textbuf, "\n");
    
    return code;

}

/*
 * Name:
 *     decodeCloseDoor
 * Description:
 *     Decodes the 'CloseDoor' 
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeCloseDoor(char *code, char *textbuf)
{

    static SC_PROC closedoor = { 9, "CloseDoor", 0xFD, 0, 1, { SC_SMALLINT } };

    strcat(textbuf, closedoor.name);
    
    code += decodeArguments(code, &closedoor, textbuf);
    strcat(textbuf, "\n");
    
    return code;
    
}


/*
 * Name:
 *     decodeCreateMonster
 * Description:
 *     Decodes the 'CreateMonster' 
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeCreateMonster(char *code, char *textbuf)
{

    static SC_PROC createmonster = 
        { 13, "CreateMonster", 0xFB, 0, 10, 
            { SC_CHAR, SC_SMALLINT, SC_CHAR, SC_CHAR, SC_CHAR, SC_CHAR, 
                SC_CHAR, SC_CHAR, SC_SMALLINT, SC_SMALLINT } }; 

    strcat(textbuf, createmonster.name);
    
    code += decodeArguments(code, &createmonster, textbuf);
    strcat(textbuf, "\n");
    
    return code;
    
}


/*
 * Name:
 *     decodeTeleport
 * Description:
 *     Decodes the 'Teleport' 
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeTeleport(char *code, char *textbuf)
{
    static SC_PROC teleport[] = {
        { 5, "Party", 0xFA, EVENT_WHOLEPARTY, 2, { SC_SMALLINT, SC_SMALLINT } }, /* src, dest */     
        { 7, "Monster", 0xFA, EVENT_MONSTER, 2, { SC_SMALLINT, SC_SMALLINT } }, /* src, dest */   
        { 8, "ItemType", 0xFA, EVENT_ITEMTYPE, 5, { SC_SMALLINT, SC_CHAR, SC_SMALLINT, SC_CHAR, SC_SMALLINT } },
        /* itemtype, srclevel, srcpos, destlevel, destpos src/destlevel may be a CONSTANT 'EVENT_ACTLEVEL' */
        { 6, "ItemNo", 0xFA, EVENT_ITEMNO, 5, { SC_SMALLINT, SC_CHAR, SC_SMALLINT, SC_CHAR, SC_SMALLINT } },
        { 0 }
    };

    char sub_code;
    
    
    sub_code = *code;      
    code++;
    
    strcat(textbuf, "Teleport");           
    /* TODO: Print identation */
    code += eventDecodePrintProc(code, sub_code, teleport, textbuf);        
    strcat(textbuf, "\n");
    
    return code;

}

/*
 * Name:
 *     decodeStealSmallItem
 * Description:
 *
 * Input:
 *      code *: Pointer on code to decode
 *     textbuf *: Where to print  
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeStealSmallItem(char *code, char *textbuf)
{
        
    static SC_PROC stealitem = 
        { 14, "StealSmallItem", 0xF9, 0, 3, { SC_CHAR, SC_SMALLINT, SC_CHAR } };

    strcat(textbuf, stealitem.name);
    
    code += decodeArguments(code, &stealitem, textbuf);
    strcat(textbuf, "\n");
    
    return code;
    
}

/*
 * Name:
 *     decodeMessage
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeMessage(char *code, char *textbuf)
{

    static SC_PROC message = 
        {  7, "Message", 0xF8, 0, 2, { SC_STRING, SC_SMALLINT } }; 
        /* stringno, color EOB1: String as arg */
        
    strcat(textbuf, message.name);
    
    code += decodeArguments(code, &message, textbuf);
    strcat(textbuf, "\n");
    
    return code;

}

/*
 * Name:
 *     decodeSetFlag
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeSetFlag(char *code, char *textbuf)
{

    static SC_PROC setflag[] = {
        { 5, "Level", 0xF7, EVENT_LEVELFLAG, 1, { SC_CHAR } },
        { 6, "Global", 0xF7, EVENT_GLOBALFLAG, 1, { SC_CHAR } },
        { 7, "Monster", 0xF7, EVENT_MONSTER, 1, { SC_CHAR } },  
        { 6, "Choice", 0xF7, EVENT_CHOICE },
        { 6, "Danger", 0xF7, EVENT_DANGER },
        { 0 }
    };

    char sub_code;
    
        
    sub_code = *code;      
    code++;
    
    strcat(textbuf, "SetFlag");
    
    code += eventDecodePrintProc(code, sub_code, setflag, textbuf); 
    strcat(textbuf, "\n");    
    
    return code;
    
}


/*
 * Name:
 *     eventDecodePlaySound
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodePlaySound(char *code, char *textbuf)
{

    static SC_PROC sound = 
        {  9, "PlaySound", 0xF6, 0, 2, { SC_CHAR, SC_SMALLINT } };    /* Soundno, Distpos */
        /* stringno, color EOB1: String as arg */
        
    strcat(textbuf, sound.name);
    
    code += decodeArguments(code, &sound, textbuf);
    strcat(textbuf, "\n");
    
    return code;

}

/*
 * Name:
 *     decodeClearFlag
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeClearFlag(char *code, char *textbuf)
{  
    
    static SC_PROC clearflag[] = {
        { 5, "Level", 0xF5, EVENT_LEVELFLAG, 1, { SC_CHAR } },
        { 6, "Global", 0xF5, EVENT_GLOBALFLAG, 1, { SC_CHAR } },
        { 6, "Choice", 0xF5, EVENT_CHOICE },
        { 6, "Danger", 0xF5, EVENT_DANGER },
        { 0 }
    };

    char sub_code;


    sub_code = *code;      
    code++;
    
    strcat(textbuf, "ClearFlag");
    
    code += eventDecodePrintProc(code, sub_code, clearflag, textbuf);        
    strcat(textbuf, "\n");
    
    return code;

}

/*
 * Name:
 *     decodeHealParty
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeHealParty(char *code, char *textbuf)
{

    static SC_PROC heal = 
        {  9, "HealParty", 0xF4, 0, 2, { SC_CHAR, SC_CHAR } };  /* who (-1=all, healpoints) */
        
    strcat(textbuf, heal.name);
    
    code += decodeArguments(code, &heal, textbuf);
    strcat(textbuf, "\n");
    
    return code;

}

/*
 * Name:
 *     decodeDamageParty
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeDamageParty(char *code, char *textbuf)
{

    static SC_PROC damage = 
        { 17, "DamageParty", 0xF3, 0, 7, { SC_CHAR, SC_CHAR, SC_CHAR, SC_CHAR, SC_CHAR, SC_CHAR, SC_CHAR } };
        
    strcat(textbuf, damage.name);
    
    code += decodeArguments(code, &damage, textbuf);
    strcat(textbuf, "\n");
    
    return code;


}

/*
 * Name:
 *     decodeKeyWordELSE
 * Description:
 *
 * Input:
 *      code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeKeyWordELSE(char *code, char *textbuf)
{

    char valbuf[50];
    int jumpadress;


    jumpadress = *(short int *)code;
    
    sprintf(valbuf, "ELSE (at adr: %04X)\n", (int)jumpadress);
    strcat(textbuf, valbuf);
    
    return (code + 2);

}
       

/*
 * Name:
 *     decodeEND_PROC
 * Description:
 *     Decodes the byte for "END PROC"
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *     Number of bytes used for condition statement  
 */
static char *decodeEND_PROC(char *code, char *textbuf)
{

    strcat(textbuf, "END PROC\n\n");
    
    EndProc = 1;
    EventCallStackIdx = 0;
    
    return code;
    
}

/*
 * Name:
 *     decodeReturnSub
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeReturnSub(char *code, char *textbuf)
{

    strcat(textbuf, "END SUB\n\n");
    
    EndProc = 1;
    
    return code;

}

/*
 * Name:
 *     decodeCallSub
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeCallSub(char *code, char *textbuf)
{

    char expbuf[100];
    short int calladress;
    
    
    calladress = *(short int *)code;
    code += 2;

    sprintf(expbuf, "CALL SUB_FUNC%d()\n\n", (int)calladress);
    strcat(textbuf, expbuf);
    
    return code;

}

/*
 * Name:
 *     decodeIF_THEN
 * Description:
 *     Decodes a complete conditional code
 * Input:
 *     code *: Pointer on first by code of procedure 
 *     textbuf *: Where to print 
 * Output:
 *     Pointer on next code-bytes  to decode
 */ 
static char *decodeIF_THEN(char *code, char *textbuf) 
{

    char expression[256];
    char CompStack[10][100];
    int  CompStackIdx;
    char main_code, sub_code;
    short int else_adr;
        
    
    CompStackIdx = 0;
    
    strcat(textbuf, "IF ");

    main_code = *code;
    code++;

    while (main_code != EVENT_ENDCONDITION) {

        if (main_code >= 0) {
            /* It's a comparision value */
            sprintf(CompStack[CompStackIdx], "%d", (int)main_code);
         }
        else {
            /* It's a comparision code or a comparision function */
            switch(main_code) {

                case EVENT_CHOICE:			    /* 0xE4:    */
                case EVENT_TRIGGERFLAGS:		/* 0xE0:    */
                case EVENT_PARTYDIRECTION:		/* 0xED:    */
                case EVENT_WHOLEPARTY:       	/*  0xE8:   */
                case EVENT_WHOLEWALL:	        /* 0xF7:    */
                case EVENT_ITEMNO:		        /* 0xF5:    */
                case EVENT_THISITEMNO:		    /* 0xF2:    */
                case EVENT_PARTY:	            /* 0xF1:    */
                case EVENT_GLOBALFLAG: 	        /* 0xF0:    */
                case EVENT_LEVELFLAG:           /* 0xEF:    */
                case EVENT_WALLPART:		    /* 0xE9     */
                case EVENT_MEMBERPICNO:		    /* 0xCE     */
                case EVENT_MEMBERRACE:	        /* 0xDD:    */
                case EVENT_MEMBERCLASS:		    /*  0xDC:	*/
                case EVENT_RANDOMVALUE:		    /* 0xDB:    */
                case EVENT_SPELL:
                case EVENT_PARTYVISIBLE:	    /* 0xDA:    */
                    code += decodeFunc(main_code, 0, code, CompStack[CompStackIdx]);
                	break;
                case EVENT_MONSTER:		        /* 0xF3:    */
                    /* FIXME: Add correct code */
                    code++;
                    break;
                case EVENT_USEDITEM:            /* 0xE7:    */
                case EVENT_TRIGGERITEM:		    /* 0xD7:    */
                    sub_code = *code;
                    code++;
                    code += decodeFunc(main_code, sub_code, code, CompStack[CompStackIdx]);
                    break;
                case EVENT_COMPAREVALUE:	    /* 0xD2:    */
                    sprintf(CompStack[CompStackIdx], "%d", (int)*code);

                    break;
                /* Comparision functions */
                case SC_COMPAREEQUAL:	        /* 0xFF:    */
                case SC_COMPARENOTEQUAL:	    /* 0xFE:    */
                case SC_COMPARELESS:		    /* 0xFD:    */
                case SC_COMPARELESSEQUAL:	    /* 0xFC:    */
                case SC_COMPAREGREATER:	        /* 0xFB:    */
                case SC_COMPAREGREATEREQUAL:	/* 0xFA:    */
                    if (CompStackIdx > 1) {
                        decodeComparision(main_code, CompStack[CompStackIdx-2], CompStack[CompStackIdx-1], expression);
                        CompStackIdx -= 2;
                        strcpy(CompStack[CompStackIdx], expression);
                    }
                    else {
                        EndProc = 2;            /* Error    */
                    }
                    break;
                case SC_COMPAREAND:		        /* 0xF9:    */
                case SC_COMPAREOR:		        /* 0xF8:    */
                    if (CompStackIdx > 1) {
                        decodeComparision(main_code, CompStack[CompStackIdx-2], CompStack[CompStackIdx-1], expression);
                        CompStackIdx -= 2;
                        strcpy(CompStack[CompStackIdx], expression);
                    }
                    else {
                        EndProc = 2;            /* Error    */
                    }
                    break;

            }

        }

        CompStackIdx++;

        main_code = *code;
        code++;
        if (EndProc > 0) {
            return code;        /* Error */
        }

    }

    CompStackIdx--;
    strcat(textbuf, CompStack[CompStackIdx]);

    else_adr = *(short int *)code;
    code += 2;
    sprintf(expression, " THEN (else/end-if-adr: %04X)\n", (int)else_adr);
    /* TODO: Set and print identation */
    strcat(textbuf, expression);

    return code;

}

/*
 * Name:
 *     decodeItemConsume
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeItemConsume(char *code, char *textbuf) 
{
    static SC_PROC item[] = {
        { 4, "Hand", 0xED, EVENT_ITEMHAND },
        { 6, "Bottom", 0xED, EVENT_ITEMBOTTOM, 2, { SC_CHAR, SC_CHAR } }, 
        { 4, "Type", 0xED, EVENT_ITEMTYPE, 2, { SC_CHAR, SC_CHAR } },
        { 0 }
    };  
     
    char sub_code;
    
    
    sub_code = *code;      
    code++;
    
    strcat(textbuf, "ItemConsume");
    /* TODO: Print identation */
    code += eventDecodePrintProc(code, sub_code, item, textbuf);
    strcat(textbuf, "\n");
    
    return code;

}


/*
 * Name:
 *     decodeChangeLevel
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeChangeLevel(char *code, char *textbuf) 
{

    static SC_PROC level[] = {
        { 5, "Party", 0xEC, EVENT_PLAYER, 3, { SC_CHAR, SC_SMALLINT, SC_CHAR } },
                                             /* levelno, position, direction */
        { 7, "Monster", 0xEC, EVENT_MONSTER }, /* Only EOB 2 */
        { 0 }
    };  
     
    char sub_code;
    
    
    sub_code = *code;      
    code++;
    
    strcat(textbuf, "ChangeLevel");
    /* TODO: Print identation */
    code += eventDecodePrintProc(code, sub_code, level, textbuf);        
    strcat(textbuf, "\n");
    
    return code;

}

/*
 * Name:
 *     decodeGiveExperience
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *decodeGiveExperience(char *code, char *textbuf) 
{
    
    static SC_PROC experience = 
        { 14, "GiveExperience", 0xEB, EVENT_VALUE, 2, { SC_CHAR, SC_SMALLINT } }; 

    strcat(textbuf, experience.name);
    
    code += decodeArguments(code, &experience, textbuf);
    strcat(textbuf, "\n");
    
    return code;
    
}

/*
 * Name:
 *     eventDecodeCreateItem
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodeCreateItem(char *code, char *textbuf) 
{
                 
    static SC_PROC createitem = 
        { 10, "CreateItem", 0xEA, 0, 4, { SC_SMALLINT, SC_SMALLINT, SC_CHAR, SC_CHAR, SC_VARARG } };
        /* Check 'Create item': Multiple functions, depending on flags... */ 
    strcat(textbuf, createitem.name);
    
    code += decodeArguments(code, &createitem, textbuf);
    strcat(textbuf, "\n");
    
    return code;

}
    	
/*
 * Name:
 *     eventDecodeThrow
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodeThrow(char *code, char *textbuf) 
{

    static SC_PROC thrown[] = {
        { 4, "Item", 0xE9, EVENT_ITEMNO, 4, { SC_SMALLINT, SC_SMALLINT, SC_CHAR, SC_CHAR } }, 
        { 5, "Spell", 0xE9, EVENT_SPELL, 4, { SC_SMALLINT, SC_SMALLINT, SC_CHAR, SC_CHAR } },
        { 0 }
    };  
     
    char sub_code;

    
    sub_code = *code;      
    code++;
    
    strcat(textbuf, "Throw");
    /* TODO: Print identation */
    code += eventDecodePrintProc(code, sub_code, thrown, textbuf);
    strcat(textbuf, "\n");
    
    return code;
    
}

/*
 * Name:
 *     eventDecodeTurn
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodeTurn(char *code, char *textbuf) 
{

    static SC_PROC turn[] = {
        {  9, "TurnParty", 0xE8, EVENT_PARTY, 1, { SC_CHAR } },
        {  8, "TurnItem", 0xE8, EVENT_ITEMNO, 1, { SC_CHAR } },
        { 0 }
    };  
     
    char sub_code;

    
    sub_code = *code;      
    code++;
    
    strcat(textbuf, "TurnDir");
    /* TODO: Print identation */
    code += eventDecodePrintProc(code, sub_code, turn, textbuf);
    strcat(textbuf, "\n");
    
    return code;
    
}

/*
 * Name:
 *     eventDecodeIdentifyItems
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodeIdentifyItems(char *code, char *textbuf) 
{

    static SC_PROC identitem = 
        { 13, "IdentifyItems", 0xE7, 0, 1, { SC_SMALLINT } };
        
    strcat(textbuf, identitem.name);
    code += decodeArguments(code, &identitem, textbuf);
    strcat(textbuf, "\n");
    
    return code;
    
}

/*
 * Name:
 *     eventDecodeMenu
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodeMenu(char *code, char *textbuf) 
{

    
    return code;
    
}

/*
 * Name:
 *     eventDecodeWait
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodeWait(char *code, char *textbuf) 
{


    return code;
    
}
    
/*
 * Name:
 *     eventDecodeWait
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodeUpdateScreen(char *code, char *textbuf) 
{


    return code;
    
}

/*
 * Name:
 *     eventDecodeWait
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodeEncounter(char *code, char *textbuf) 
{


    return code;
    
}
    	
/*
 * Name:
 *     eventDecodeWait
 * Description:
 *
 * Input:
 *     code *: Pointer on code to decode
 *     textbuf *: Where to print 
 * Output:
 *      Pointer on next bytes to decode
 */
static char *eventDecodeSpecialWinPics(char *code, char *textbuf) 
{


    return code;
    
}

/*
 * Name:
 *     decodeScriptFunc
 * Description:
 *     Decodes a complete script function 
 * Input:
 *     code *:    Pointer on first by code of procedure 
 *     textbuf *: Wher to put the decoded strings
 */ 
static void decodeScriptFunc(char *code, char *textbuf) 
{

    static DECODE_PROC DecodeFuncs[] = {

        decodeSetWall,
    	decodeSwitchWall,
    	decodeOpenDoor,
    	decodeCloseDoor,
    	decodeCreateMonster,
    	decodeTeleport,
    	decodeStealSmallItem,
    	decodeMessage,
    	decodeSetFlag,
    	eventDecodePlaySound,
    	decodeClearFlag,
    	decodeHealParty,
    	decodeDamageParty,
    	decodeKeyWordELSE,
    	decodeEND_PROC,
    	decodeReturnSub,
    	decodeCallSub,
    	decodeIF_THEN,
    	decodeItemConsume,
    	decodeChangeLevel,
    	decodeGiveExperience,
    	eventDecodeCreateItem,
    	eventDecodeThrow,
    	eventDecodeTurn,
    	eventDecodeIdentifyItems,
    	eventDecodeMenu,    
    	eventDecodeWait,
    	eventDecodeUpdateScreen,
    	eventDecodeEncounter,
    	eventDecodeSpecialWinPics

    };

    char line_nr[100];
    char maincode;
    int i;


    EndProc = 0;

    while(! EndProc) {

        sprintf(line_nr, "%04X: ", code - InfBuffer);
        strcat(textbuf, line_nr);
        maincode = *code;
        code++;

        if ((maincode < 0) && (maincode > -31)) {

                maincode++;
                maincode = -maincode;

                code = DecodeFuncs[maincode](code, textbuf);

        }
        
    }
    
    if (EndProc > 1) {
        /* An error happened, print the line */
        fprintf(FOut, "%02X", maincode);
        for (i = 0; i < 10; i++) {
            fprintf(FOut, "%02X", *code);
            code++;
        }
        fprintf(FOut, "\n\n");
    }

}

/*
 * Name:
 *     decodeInfFile
 * Description:
 *     Decodes  the complete Inf-File 
 * Input:
 *     infbuf *: Pointer on buffer holding the whoel event data
 */ 
static void decodeInfFile(char *infbuf) 
{

    int  i, num_event;
    char *pevent;
    
    
    pevent = infbuf + *(short int *)infbuf;     /* Points on numbers of events  */

    num_event = *(short int *)pevent;           /* Number of events             */

    pEventDesc = (EVENT_DESC *)&pevent[2];      /* Pointer on event descriptors */

    PrintBuf[0] = 0;

    /* -------- Decode all scripts --- */
    for (i = 0; i < num_event; i++) {

        fprintf(FOut, "PROC EventAt%04X()\n",  pEventDesc[i].pos);
        PrintBuf[0] = 0;
        decodeScriptFunc(infbuf + pEventDesc[i].codestart, PrintBuf);
        fprintf(FOut, PrintBuf);

    }

    fprintf(FOut, "\n\n");
    for (i = 0; i < num_event; i++) {

        fprintf(FOut, "EventAt%04X, %02X, %04X (Pos: x:%d y:%d)\n",
                      (int)pEventDesc[i].pos,
                      pEventDesc[i].triggerbits,
                      (int)pEventDesc[i].codestart,
                      pEventDesc[i].pos % 32,
                      pEventDesc[i].pos / 32 );

    }

}

/* ========================================================================== */
/* ============================= THE PUBLIC ROUTINES ======================== */
/* ========================================================================== */

/*
 * Name:
 *     decodeMain
 * Description:
 *     Decodes an uncompressed EOB1-Inf-File
 * Input:
 *     level_no:  Number of level to read in the 'inf'-file
 */
int decodeMain(int level_no)
{
    
    FILE *fin;
    char filename[128];
    
    
    sprintf(filename, "level%d.unc", level_no);

    fin = fopen(filename, "rt");
    if (fin) {        

        InfSize = fread(InfBuffer,1, 9500, fin);
        fclose(fin);
        
        sprintf(filename, "level%d.txt", level_no);
        FOut = fopen(filename, "wt");
        if (! FOut) {
            return 0;
        }
        
        /* ------ Decode the whole file ---- */
        decodeInfFile(InfBuffer);
        /* --------------------------------- */
        fclose(FOut);
        
    }
    
    return 0;

}

/*
 * Name:
 *     main
 * Description:
 *     Decodes an uncompressed EOB1-Inf-File
 * Input:
 *     level_no:  Number of level to read in the 'inf'-file
 */
void main(void)
{

    decodeMain(1);

}