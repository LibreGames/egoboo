/* *****************************************************************************
*  SDLGLCFG.H                                                                  *
*      - Read procedures for the configuration of SDLGL                        *
*                                                                              *
*  SDLGL_Lybrary                                                               *
*      (c)2005-2010 Paul Mueller <pmtech@swissonline.ch>                       *
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

#ifndef _SDLGL_CONFIG_H_
#define _SDLGL_CONFIG_H_

/* *****************************************************************************
* DEFINES                               				                       *
*******************************************************************************/

/* ------- The different values, that can be read, decomma limited ---------- */

#define SDLGLCFG_VAL_NONE    ((char)0)
#define SDLGLCFG_VAL_END     ((char)0)
#define SDLGLCFG_VAL_CHAR    ((char)1)
#define SDLGLCFG_VAL_UCHAR   ((char)2)
#define SDLGLCFG_VAL_SHORT   ((char)3)
#define SDLGLCFG_VAL_USHORT  ((char)4)
#define SDLGLCFG_VAL_INT     ((char)5)
#define SDLGLCFG_VAL_UINT    ((char)6)
#define SDLGLCFG_VAL_FLOAT   ((char)7)
#define SDLGLCFG_VAL_STRING  ((char)8)
#define SDLGLCFG_VAL_ONECHAR ((char)9)      /* Read a single char              */
#define SDLGLCFG_VAL_LABEL   ((char)10)     /* Write a single string to file   */
#define SDLGLCFG_VAL_BOOLEAN ((char)11) 
#define SDLGLCFG_VAL_IPAIR   ((char)12)     /* Pair of int:   i1 [- i2] */
#define SDLGLCFG_VAL_FPAIR   ((char)13)     /* Pair of float: f1 [- f2] */
#define SDLGLCFG_VAL_EGOSTR  ((char)13)     /* Underlines have to be removed at loading time */

/* *****************************************************************************
* TYPEDEFS                                 				                       *
*******************************************************************************/

typedef struct
{
    char type;      /* Type of value to read                	*/
    void *data;     /* Where to put the data                	*/
    char len;       /* Len of data (strings and arrays)     	*/
    char pos;	    /* Of value in 'data-line' ignore delimiter	*/

} SDLGLCFG_VALUE;

typedef struct
{
    char  type;     /* Type of value to read            */
    void *data;     /* Where to put the value           */     
    char  len;      /* Len of data (strings and arrays) */
    char *name;     /* name of value (case insensitive) */
    char info;      /* If commands are for multiple players on same PC */

} SDLGLCFG_NAMEDVALUE;

typedef struct
{
    void *readbuf;      /* Read into this buffer, holding          */
                        /* holding data from SDLGLCFG_VALUE        */
    void *recbuf;       /* Where to put the data read into readbuf */
                        /* Points on record[1],                    */                         
    int  maxrec;        /* Maximum of records                      */
    int  recsize;       /* Size of record in buffer 'recdata'      */
    SDLGLCFG_VALUE *rcf;/* Descriptor for values on single line    */

} SDLGLCFG_LINEINFO;     /* Info for reading configuration lines */

typedef struct
{  
    char *filename;
    char *buffer;        /* Pointer on buffer holding all file data as block  */
    int  size;           /* Size of data in 'buffer'                          */ 

} SDLGLCFG_FILE;

/* *****************************************************************************
* CODE                                 				                           *
*******************************************************************************/

int  sdlglcfgOpenFile(char *filename, char blocksigns[4]);
int  sdlglcfgSkipBlock(void);
int  sdlglcfgIsActualBlockName(char *name);
int  sdlglcfgReadNamedValues(SDLGLCFG_NAMEDVALUE *vallist);
int  sdlglcfgReadValues(SDLGLCFG_VALUE *vallist);
int  sdlglcfgReadRecordLines(SDLGLCFG_LINEINFO *lineinfo, int fixedpos);
int  sdlglcfgReadStrings(char *targetbuf, int bufsize);
void sdlglcfgCloseFile(void);
void sdlglcfgReadSimple(char *filename, SDLGLCFG_NAMEDVALUE *vallist);
void sdlglcfgLoadFile(char *dir_name, SDLGLCFG_FILE *fdesc);
void sdlglcfgFreeFile(SDLGLCFG_FILE *fdesc);

void sdlglcfgEgobooRecord(char *fname, SDLGLCFG_LINEINFO *lineinfo, int write);
char sdlglcfgEgobooValues(char *fname, SDLGLCFG_NAMEDVALUE *vallist, int write); 
char sdlglcfgRawLines(char *filename, char *destbuffer, int dest_size, int line_len, char write);

#endif
