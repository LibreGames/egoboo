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

#include <stdio.h>


#include "msg.h"        // Type of messages

// Header with script codes
#include "script.h"

/*******************************************************************************
* DEFINES                                                                      *
*******************************************************************************/

#define ARG_CHAR ((char)1)
#define ARG_SINT ((char)2)
#define ARG_INT  ((char)3)
#define ARG_STR  ((char)4)      // Argument is a string

#define SCRIPT_MAX        200   // Maximum number of scripts in List
#define SCRIPT_CODE_MAX 20000   // Maximum number of code-bytes for all scripts 

/*******************************************************************************
* DATA                                                                         *
*******************************************************************************/

static int  Script_No = 1;                  // Where to put the pointer on script
static char ScriptCode[SCRIPT_CODE_MAX];    // Buffer for scripted code

static char *ScriptList[SCRIPT_MAX + 2] = 
{
    ScriptCode      // Keep the compiler quiet
};

// Additional info used for decoding as values for script
// E.g. 'Script_Target' is the sender of the message
static Script_Target = 0;

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

/*
 * Name:
 *     scriptFuncIF
 * Function:
 *     Evaluate the condition and set the code pointer (jump od no jump) to
 *     next code to execute  
 * Input:
 *     code *:    Pointer on next command to execute
 *     pchar *:   Work with his one (is owner of the script)  
 *     sender_no: Is the 'Target' in code 
 */
static char *scriptFuncIF(char *code, CHAR_T *pchar, int sender_no)
{
    char func_no;
    int  cmp_stack[50]; // Stack for comparision values
    int  s_idx;
    
    func_no = *char;            // Which function/value ?
    char++;                     // Point on argument
    s_idx = 0;              // First comparision value
    
    if(func_no >= 0)
    {
        // Its a char value to compare
        cmp_stack[s_idx] = func_no;
        s_idx++;        
        code++;
    }
    else
    {
        switch(func_no)
        {
            case SCMP_SINT: // It follows a short int as comparision value
                cmp_stack[s_idx] = *(short int *)code;
                s_idx++;        
                code += 2;
                break;
            case SCMP_INT:  // it follows an integer comparision value
                cmp_stack[s_idx] = *(int *)code;
                s_idx++;        
                code += 4;
                break;
            // Jump on comparision
            case SCMP_THEN:
                if(s_idx > 0)   // For safety reasons
                {
                    s_idx--;
                    if(cmp_stack[s_idx])
                    {
                        // True
                        code += 2;  // Jump-Number is a short int, point on code to execute
                    }
                    else
                    {
                        // False
                        code += *(short int *)code;     // Jump over this code
                    }
                }
                break;
        }
    }
    // Pointer on next code to scan
    return code;    
}	

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     scriptInit
 * Function:
 *     Reset the Script buffer an the pointer for the scripts 
 * Input:
 *     None 
 */
void scriptInit(void)
{
    Script_No = 1;
    ScriptList[0] = ScriptCode;
    ScriptList[1] = NULL;
    ScriptList[2] = NULL;
}

/*
 * Name:
 *     scriptLoad
 * Function:
 *     Loads a script from given directory    
 * Input:
 *     fdir*:     Directory where to load  
 */
void scriptLoad(char *fdir)
{
    char fname[512];
    
    
    sprintf(fname, "%sscript.txt");
    // @todo: Load it and encode it
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
 *     sender_no: Sent by this character / Normally target e.g. 'HURT' by attack 
 */
void scriptRun(int char_no, int script_no, int why, int sender_no)
{
	CHAR_T *pchar;
	char *code;

    
    if(!script_no)  return;         // Invalid script number
	code = ScriptList[script_no];
	if(NULL == code) return;		// No script available
	pchar = charGet(char_no);

    // Save the sender for checking target...
    Script_Target = sender_no;
    // Now decode
	while(*code != FUNC_END && *code < 0 && *code >= FUNC_MAX)
	{
		func_no = *code;
		code++;
        
		switch(func_no)
		{
			case FUNC_IF;
				code = scriptFuncIF(code, pchar, sender_no);		
				break;
			default:
				// @todo:
				return;
		}
	}
}