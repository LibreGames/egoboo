//********************************************************************************************
//* Egoboo - script.c
//*
//* Implements the game's scripting language.
//*
//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include <assert.h>

#include "script.h"

#include "Log.h"
#include "Mad.h"
#include "passage.h"
#include "camera.h"
#include "enchant.h"
#include "egoboo.h"
#include "passage.h"
#include "Client.h"
#include "AStar.h"
#include "sound.h"

#include "egoboo_utility.h"

#include "particle.inl"
#include "object.inl"
#include "input.inl"
#include "char.inl"
#include "mesh.inl"
#include "egoboo_math.inl"

#define FUNCTION_FLAG_BIT 0x80000000
#define CONSTANT_FLAG_BIT 0x80000000
#define CONSTANT_BITS     0x07FFFFFF

#define INDENT_BITS  0x78000000
#define INDENT_SHIFT 27

#define OP_BITS  0x78000000
#define OP_SHIFT 27

#define VAR_BITS     0x07FFFFFF
#define OPCODE_BITS  0x07FFFFFF

#define HIGH_BITS  0xF8000000
#define HIGH_SHIFT 27

#define END_FUNCTION  (FUNCTION_FLAG_BIT | F_End)
#define IS_FUNCTION(XX)  HAS_SOME_BITS( XX, FUNCTION_FLAG_BIT )
#define NOT_FUNCTION(XX) HAS_NO_BITS( XX, FUNCTION_FLAG_BIT )

#define IS_CONSTANT(XX) HAS_SOME_BITS( XX, CONSTANT_FLAG_BIT )
#define GET_CONSTANT(XX) ( (XX) & CONSTANT_BITS )
#define PUT_CONSTANT(XX) ( ((XX) & CONSTANT_BITS) | CONSTANT_FLAG_BIT )

#define GET_FUNCTION_BITS(XX) ( (XX) & (FUNCTION_FLAG_BIT | 0x07FFFFFF) )

#define GET_FUNCTION_BIT(XX) ( ((XX) & FUNCTION_FLAG_BIT) >> 31 )
#define PUT_FUNCTION_BIT(XX) ( ((XX) << 31) & FUNCTION_FLAG_BIT )

#define GET_CONSTANT_BIT(XX) ( ((XX) & CONSTANT_FLAG_BIT) >> 31 )
#define PUT_CONSTANT_BIT(XX) ( ((XX) << 31) & CONSTANT_FLAG_BIT )

#define GET_OP_BITS(XX) ( ((XX) & OP_BITS) >> OP_SHIFT )
#define PUT_OP_BITS(XX) ( ((XX) << OP_SHIFT) & OP_BITS  )

#define GET_VAR_BITS(XX) ( (XX) & VAR_BITS )
#define PUT_VAR_BITS(XX) ( (XX) & VAR_BITS )

#define GET_OPCODE_BITS(XX) ( (XX) & VAR_BITS )
#define PUT_OPCODE_BITS(XX) ( (XX) & VAR_BITS )

#define GET_INDENT(XX) ( ((XX) & INDENT_BITS) >> INDENT_SHIFT )
#define PUT_INDENT(XX) ( ((XX) << INDENT_SHIFT) & INDENT_BITS )

#define IS_END(XX) ( GET_FUNCTION_BITS( XX ) == END_FUNCTION )


struct s_opcode_element
{
  Uint8           type;
  Uint32          value;
  char            name[MAXCODENAMESIZE];
};
typedef struct s_opcode_element OPCODE_ELEMENT;

struct s_opcode_list
{
  int            count;
  OPCODE_ELEMENT opcode[MAXCODE];
};
typedef struct s_opcode_list OPCODE_LIST;

static OPCODE_LIST opcode_lst = {0};

struct sCompilerState
{
  size_t          file_size;
  char            file_buffer[BUFFER_SIZE];         // Where to put an MD2

  size_t          line_size;
  char            line_buffer[MAXLINESIZE];

  int             line_num;
  int             index;
  int             temp;

};

typedef struct sCompilerState CompilerState_t;

static CompilerState_t _cstate;


static bool_t run_function( Game_t * gs, Uint32 value, CHR_REF character );

static bool_t scr_break_passage( Game_t * gs, AI_STATE * pstate );
static bool_t scr_search_tile_in_passage( Game_t * gs, AI_STATE * pstate );

static Uint32 get_high_bits();
static size_t tell_code( size_t read );

static retval_t run_operand( Game_t * gs, Uint32 value, CHR_REF character );
static retval_t set_operand( AI_STATE * pstate, ScriptInfo_t * slist, Uint8 variable );


static void add_code( ScriptInfo_t * slist, Uint32 highbits );
static void parse_line_by_line( ScriptInfo_t * slist );
static Uint32 jump_goto( ScriptInfo_t * slist, int index );
static void parse_jumps( ScriptInfo_t * slist, int index_stt, int index_end );
static void log_code( ScriptInfo_t * slist, int ainumber, char* savename );


//------------------------------------------------------------------------------
//AI Script Routines------------------------------------------------------------
//------------------------------------------------------------------------------
void insert_space( size_t position )
{
  // ZZ> This function adds a space into the load line if there isn't one
  //     there already

  Uint8 cTmp, cSwap;

  if ( _cstate.line_buffer[position] != ' ' )
  {
    cTmp = _cstate.line_buffer[position];
    _cstate.line_buffer[position] = ' ';
    position++;
    _cstate.line_size++;
    while ( position < _cstate.line_size )
    {
      cSwap = _cstate.line_buffer[position];
      _cstate.line_buffer[position] = cTmp;
      cTmp = cSwap;
      position++;
    }
    _cstate.line_buffer[position] = 0;
  }
}

//------------------------------------------------------------------------------
void copy_one_line( size_t write )
{
  // ZZ> This function copies the line back to the load buffer

  size_t read;
  Uint8 cTmp;

  read = 0;
  cTmp = _cstate.line_buffer[read];
  while ( cTmp != 0 )
  {
    cTmp = _cstate.line_buffer[read];  read++;
    _cstate.file_buffer[write] = cTmp;  write++;
  }



  _cstate.line_num++;
}

//------------------------------------------------------------------------------
size_t load_one_line( size_t read )
{
  // ZZ> This function loads a line into the line buffer

  bool_t stillgoing, foundtext;
  Uint8 cTmp;

  // Parse to start to maintain indentation
  _cstate.line_size = 0;
  stillgoing = btrue;
  while ( stillgoing && read < _cstate.file_size )
  {
    cTmp = _cstate.file_buffer[read];
    stillgoing = bfalse;
    if ( cTmp == ' ' )
    {
      read++;
      _cstate.line_buffer[_cstate.line_size] = cTmp; _cstate.line_size++;
      stillgoing = btrue;
    }
    // _cstate.line_buffer[_cstate.line_size] = 0; // or cTmp as cTmp == 0
  }


  // Parse to comment or end of line
  foundtext = bfalse;
  stillgoing = btrue;
  while ( stillgoing && read < _cstate.file_size )
  {
    cTmp = _cstate.file_buffer[read];  read++;
    if ( cTmp == '\t' )
      cTmp = ' ';
    if ( cTmp != ' ' && cTmp != 0x0d && cTmp != 0x0a &&
         ( cTmp != '/' || _cstate.file_buffer[read] != '/' ) )
      foundtext = btrue;
    _cstate.line_buffer[_cstate.line_size] = cTmp;
    if ( cTmp != ' ' || ( _cstate.file_buffer[read] != ' ' && _cstate.file_buffer[read] != '\t' ) )
      _cstate.line_size++;
    if ( cTmp == 0x0d || cTmp == 0x0a )
      stillgoing = bfalse;
    if ( cTmp == '/' && _cstate.file_buffer[read] == '/' )
      stillgoing = bfalse;
  }
  if ( !stillgoing ) _cstate.line_size--;

  _cstate.line_buffer[_cstate.line_size] = 0;
  if ( _cstate.line_size >= 1 )
  {
    if ( _cstate.line_buffer[_cstate.line_size-1] == ' ' )
    {
      _cstate.line_size--;  _cstate.line_buffer[_cstate.line_size] = 0;
    }
  }
  _cstate.line_size++;


  // Parse to end of line
  stillgoing = btrue;
  read--;
  while ( stillgoing )
  {
    cTmp = _cstate.file_buffer[read];  read++;
    if ( cTmp == 0x0a || cTmp == 0x0d )
      stillgoing = bfalse;
  }

  if ( !foundtext )
  {
    _cstate.line_size = 0;
  }


  return read;
}

//------------------------------------------------------------------------------
size_t load_parsed_line( size_t read )
{
  // ZZ> This function loads a line into the line buffer

  Uint8 cTmp;

  // Parse to start to maintain indentation
  _cstate.line_size = 0;
  cTmp = _cstate.file_buffer[read];
  while ( cTmp != 0 )
  {
    _cstate.line_buffer[_cstate.line_size] = cTmp;  _cstate.line_size++;
    read++;  cTmp = _cstate.file_buffer[read];
  }
  _cstate.line_buffer[_cstate.line_size] = 0;
  read++; // skip terminating zero for next call of load_parsed_line()
  return read;
}

//------------------------------------------------------------------------------
void surround_space( size_t position )
{
  insert_space( position + 1 );
  if ( position > 0 )
  {
    if ( _cstate.line_buffer[position-1] != ' ' )
    {
      insert_space( position );
    }
  }
}

//------------------------------------------------------------------------------
void parse_null_terminate_comments()
{
  // ZZ> This function removes comments and endline codes, replacing
  //     them with a 0

  size_t read, write;

  read = 0;
  write = 0;
  while ( read < _cstate.file_size )
  {
    read = load_one_line( read );
    if ( _cstate.line_size > 2 )
    {
      copy_one_line( write );
      write += _cstate.line_size;
    }
  }
}

//------------------------------------------------------------------------------
int get_indentation()
{
  // ZZ> This function returns the number of starting spaces in a line

  int cnt;
  Uint8 cTmp;

  cnt = 0;
  cTmp = _cstate.line_buffer[cnt];
  while ( cTmp == ' ' )
  {
    cnt++;
    cTmp = _cstate.line_buffer[cnt];
  }

  if( iscntrl(cTmp) )
  {
    log_message( "SCRIPT ERROR FOUND: %s (%d) - %d illegal control code\n", globalparsename, _cstate.line_num, cnt );
    parseerror = btrue;
  }

  if( HAS_SOME_BITS(cnt, 0x01) )
  {
    log_message( "SCRIPT ERROR FOUND: %s (%d) - %d odd number of spaces\n", globalparsename, _cstate.line_num, cnt );
    parseerror = btrue;
  };

  cnt >>= 1;

  if ( cnt > 15 )
  {
    log_warning( "SCRIPT ERROR FOUND: %s (%d) - %d levels of indentation\n", globalparsename, _cstate.line_num, cnt );
    parseerror = btrue;
    cnt = 15;
  };

  return cnt;
}

//------------------------------------------------------------------------------
void fix_operators()
{
  // ZZ> This function puts spaces around operators to seperate words
  //     better

  Uint32 cnt;
  Uint8 cTmp;

  cnt = 0;
  while ( cnt < _cstate.line_size )
  {
    cTmp = _cstate.line_buffer[cnt];
    if ( cTmp == '+' || cTmp == '-' || cTmp == '/' || cTmp == '*' ||
         cTmp == '%' || cTmp == '>' || cTmp == '<' || cTmp == '&' ||
         cTmp == '=' )
    {
      surround_space( cnt );
      cnt++;
    }
    cnt++;
  }
}

//------------------------------------------------------------------------------
int starts_with_capital_letter()
{
  // ZZ> This function returns btrue if the line starts with a capital

  int cnt;
  Uint8 cTmp;

  cnt = 0;
  cTmp = _cstate.line_buffer[cnt];
  while ( cTmp == ' ' )
  {
    cnt++;
    cTmp = _cstate.line_buffer[cnt];
  }
  if ( cTmp >= 'A' && cTmp <= 'Z' )
    return btrue;
  return bfalse;
}

//------------------------------------------------------------------------------
Uint32 get_high_bits()
{
  // ZZ> This function looks at the first word and generates the high
  //     bit codes for it

  Uint32 highbits, indent;

  indent = get_indentation();
  highbits = PUT_INDENT(indent);
  if ( starts_with_capital_letter() )
  {
    highbits |= FUNCTION_FLAG_BIT;
  }

  return highbits;
}

//------------------------------------------------------------------------------
size_t tell_code( size_t read )
{
  // ZZ> This function tells what code is being indexed by read, it
  //     will return the next spot to read from and stick the code number
  //     in _cstate.index

  int cnt, wordsize;
  bool_t codecorrect;
  Uint8 cTmp;
  int idsz;
  char cWordBuffer[MAXCODENAMESIZE];


  // Check bounds
  _cstate.index = MAXCODE;
  if ( read >= _cstate.line_size )  return read;


  // Skip spaces
  cTmp = _cstate.line_buffer[read];
  while ( cTmp == ' ' )
  {
    read++;
    cTmp = _cstate.line_buffer[read];
  }
  if ( read >= _cstate.line_size )  return read;


  // Load the word into the other buffer
  wordsize = 0;
  while ( !isspace( cTmp ) && EOS != cTmp )
  {
    cWordBuffer[wordsize] = cTmp;  wordsize++;
    read++;
    cTmp = _cstate.line_buffer[read];
  }
  cWordBuffer[wordsize] = 0;


  // Check for numeric constant
  if ( isdigit( cWordBuffer[0] ) )
  {
    sscanf( cWordBuffer + 0, "%d", &_cstate.temp );
    _cstate.index = -1;
    return read;
  }


  // Check for IDSZ constant
  idsz = IDSZ_NONE;
  if ( cWordBuffer[0] == '[' )
  {
    idsz = MAKE_IDSZ( cWordBuffer + 1 );
    _cstate.temp = idsz;
    _cstate.index = -1;
    return read;
  }



  // Compare the word to all the codes
  codecorrect = bfalse;
  _cstate.index = 0;
  while ( _cstate.index < opcode_lst.count && !codecorrect )
  {
    codecorrect = bfalse;
    if ( cWordBuffer[0] == opcode_lst.opcode[_cstate.index].name[0] && !codecorrect )
    {
      codecorrect = btrue;
      cnt = 1;
      while ( cnt < wordsize )
      {
        if ( cWordBuffer[cnt] != opcode_lst.opcode[_cstate.index].name[cnt] )
        {
          codecorrect = bfalse;
          cnt = wordsize;
        }
        cnt++;
      }
      if ( cnt < MAXCODENAMESIZE )
      {
        if ( opcode_lst.opcode[_cstate.index].name[cnt] != 0 )  codecorrect = bfalse;
      }
    }
    _cstate.index++;
  }

  if ( codecorrect )
  {
    _cstate.index--;
    _cstate.temp = opcode_lst.opcode[_cstate.index].value;
    if ( opcode_lst.opcode[_cstate.index].type == 'C' )
    {
      // Check for constants
      _cstate.index = -1;
    }
  }
  else
  {
    // Throw out an error code if we're loggin' 'em
    if ( '=' != cWordBuffer[0] || EOS != cWordBuffer[1])
    {
      log_message( "SCRIPT ERROR FOUND: %s (%d) - %s undefined\n", globalparsename, _cstate.line_num, cWordBuffer );
      parseerror = btrue;
    }
  }

  return read;
}

//------------------------------------------------------------------------------
void add_code( ScriptInfo_t * slist, Uint32 highbits )
{
  Uint32 value;

  if ( _cstate.index == -1 )  highbits |= FUNCTION_FLAG_BIT;
  if ( _cstate.index != MAXCODE )
  {
    value = highbits | _cstate.temp;

    slist->buffer[slist->buffer_index] = value;
    slist->buffer_index++;
  }
}

//------------------------------------------------------------------------------
void parse_line_by_line(ScriptInfo_t * slist)
{
  // ZZ> This function removes comments and endline codes, replacing
  //     them with a 0

  size_t read, line;
  Uint32 highbits;
  size_t parseposition;
  Uint32 operands;


  line = 0;
  read = 0;
  while ( line < _cstate.line_num )
  {
    read = load_parsed_line( read );
    fix_operators();
    highbits = get_high_bits();

    parseposition = tell_code( 0 );   // VALUE
    add_code( slist, highbits );
    _cstate.temp = 0;  // SKIP FOR CONTROL CODES
    add_code( slist, 0 );
    if ( NOT_FUNCTION( highbits ) )
    {
      parseposition = tell_code( parseposition );   // EQUALS
      parseposition = tell_code( parseposition );   // VALUE
      add_code( slist, 0 );
      operands = 1;
      while ( parseposition < _cstate.line_size )
      {
        parseposition = tell_code( parseposition );   // OPERATOR
        if ( _cstate.index == -1 ) _cstate.index = 1;
        else _cstate.index = 0;
        highbits = PUT_OP_BITS(_cstate.temp) | PUT_CONSTANT_BIT( _cstate.index );
        parseposition = tell_code( parseposition );   // VALUE
        add_code( slist, highbits );
        if ( _cstate.index != MAXCODE )
          operands++;
      }

      slist->buffer[slist->buffer_index - (operands+1)] = operands;  // Number of operands
    }
    line++;
  }

  // make absolutely sure there is an END function
  slist->buffer[slist->buffer_index] = END_FUNCTION;
  slist->buffer_index++;
}

//------------------------------------------------------------------------------
Uint32 jump_goto( ScriptInfo_t * slist, int index )
{
  // ZZ> This function figures out where to jump to on a fail based on the
  //     starting location and the following code.  The starting location
  //     should always be a function code with indentation

  Uint32 value;
  int targetindent, indent;


  value = slist->buffer[index];  index += 2;
  targetindent = GET_INDENT( value );
  indent = 100;
  while ( indent > targetindent )
  {
    value = slist->buffer[index];
    indent = GET_INDENT( value );
    if ( indent > targetindent )
    {
      // Was it a function
      if ( IS_FUNCTION( value ) )
      {
        // Each function needs a jump
        index++;
        index++;
      }
      else
      {
        // Operations cover each operand
        index++;
        value = slist->buffer[index];
        index++;
        index += ( value & 255 );
      }
    }
  }
  return index;
}

//------------------------------------------------------------------------------
void parse_jumps( ScriptInfo_t * slist, int index_stt, int index_end )
{
  // ZZ> This function sets up the fail jumps for the down-and-dirty code

  int index;
  Uint32 value, iTmp;

  index = index_stt;
  while ( index < index_end ) // End Function
  {
    value = slist->buffer[index];
    if( value == END_FUNCTION ) break; // tests for END function with no indent

    // Was it a function
    if ( IS_FUNCTION( value ) )
    {
      // Each function needs a jump
      iTmp = jump_goto( slist, index );
      assert(iTmp < index_end);
      index++;
      slist->buffer[index] = iTmp;
      index++;
    }
    else
    {
      // Operations cover each operand
      index++;
      iTmp = slist->buffer[index];
      index++;
      index += ( iTmp & 255 );
    }
  }
}

//------------------------------------------------------------------------------
void log_code( ScriptInfo_t * slist, int ainumber, char* savename )
{
  // ZZ> This function shows the actual code, saving it in a file

  int index;
  Uint32 value;
  FILE* filewrite;

  filewrite = fs_fileOpen( PRI_NONE, NULL, savename, "w" );
  if ( filewrite )
  {
    index = slist->offset_stt[ainumber];
    value = slist->buffer[index];
    while ( !IS_END( value ) ) // End Function
    {
      value = slist->buffer[index];
      fprintf( filewrite, "0x%08x--0x%08x\n", index, value );
      index++;
    }
    fs_fileClose( filewrite );
  }
  SDL_Quit();
}

//------------------------------------------------------------------------------
int ai_goto_colon( int read )
{
  // ZZ> This function goes to spot after the next colon

  Uint8 cTmp;

  cTmp = _cstate.file_buffer[read];
  while ( cTmp != ':' && read < _cstate.file_size )
  {
    read++;  cTmp = _cstate.file_buffer[read];
  }
  if ( read < _cstate.file_size )  read++;
  return read;
}

//------------------------------------------------------------------------------
void fget_code( FILE * pfile )
{
  // ZZ> This function gets code names and other goodies

  char cTmp;
  int iTmp;

  fscanf( pfile, "%c%d%s", &cTmp, &iTmp, opcode_lst.opcode[opcode_lst.count].name );
  opcode_lst.opcode[opcode_lst.count].type = cTmp;
  opcode_lst.opcode[opcode_lst.count].value = iTmp;
  opcode_lst.count++;
}



#define REGISTER_OPCODE(LIST,TYPE,CODE,NAME)  { strncpy(LIST.opcode[LIST.count].name, NAME, MAXCODENAMESIZE); LIST.opcode[LIST.count].type = (Uint8)TYPE; LIST.opcode[LIST.count].value = (Uint16)CODE; LIST.count++; }
#define REGISTER_FUNCTION(LIST,NAME)          { strncpy(LIST.opcode[LIST.count].name, #NAME, MAXCODENAMESIZE); LIST.opcode[LIST.count].type = (Uint8)'F'; LIST.opcode[LIST.count].value = (Uint16)F_##NAME; LIST.count++; }
#define REGISTER_FUNCTION_ALIAS(LIST,NAME,ALIAS)          { strncpy(LIST.opcode[LIST.count].name, ALIAS, MAXCODENAMESIZE); LIST.opcode[LIST.count].type = (Uint8)'F'; LIST.opcode[LIST.count].value = (Uint16)F_##NAME; LIST.count++; }

//------------------------------------------------------------------------------
void load_ai_codes( char* loadname )
{
  // ZZ> This function loads all of the function and variable names

  FILE* fileread;

  log_info( "load_ai_codes() - \n\tregistering internal script constants\n" );

  opcode_lst.count = 0;

  // register all the operator opcodes
  REGISTER_OPCODE( opcode_lst, 'O', OP_ADD, "+" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_SUB, "-" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_AND, "&" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_SHR, ">" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_SHL, "<" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_MUL, "*" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_DIV, "/" );
  REGISTER_OPCODE( opcode_lst, 'O', OP_MOD, "%" );

  // register all of the internal variables

  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_X,           "tmpx" );   // 000
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_Y,           "tmpy" );   // 001
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_DISTANCE,    "tmpdist" );   // 002
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_DISTANCE,    "tmpdistance" );   // 002
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_TURN,        "tmpturn" );   // 003
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TMP_ARGUMENT,    "tmpargument" );   // 004
  REGISTER_OPCODE( opcode_lst, 'V', VAR_RAND,            "rand" );   // 005
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_X,          "selfx" );   // 006
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_Y,          "selfy" );   // 007
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_TURN,       "selfturn" );   // 008
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_COUNTER,    "selfcounter" );   // 009
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ORDER,      "selforder" );   // 010
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_MORALE,     "selfmorale" );   // 011
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_LIFE,       "selflife" );   // 012
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_X,        "targetx" );   // 013
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_Y,        "targety" );   // 014
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_DISTANCE, "targetdistance" );   // 015
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_TURN,     "targetturn" );   // 016
  REGISTER_OPCODE( opcode_lst, 'V', VAR_LEADER_X,        "leaderx" );   // 017
  REGISTER_OPCODE( opcode_lst, 'V', VAR_LEADER_Y,        "leadery" );   // 018
  REGISTER_OPCODE( opcode_lst, 'V', VAR_LEADER_DISTANCE, "leaderdistance" );   // 019
  REGISTER_OPCODE( opcode_lst, 'V', VAR_LEADER_TURN,     "leaderturn" );   // 020
  REGISTER_OPCODE( opcode_lst, 'V', VAR_GOTO_X,          "gotox" );   // 021
  REGISTER_OPCODE( opcode_lst, 'V', VAR_GOTO_Y,          "gotoy" );   // 022
  REGISTER_OPCODE( opcode_lst, 'V', VAR_GOTO_DISTANCE,   "gotodistance" );   // 023
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_TURNTO,   "targetturnto" );   // 024
  REGISTER_OPCODE( opcode_lst, 'V', VAR_PASSAGE,         "passage" );   // 025
  REGISTER_OPCODE( opcode_lst, 'V', VAR_WEIGHT,          "weight" );   // 026
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ALTITUDE,   "selfaltitude" );   // 027
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ID,         "selfid" );   // 028
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_HATEID,     "selfhateid" );   // 029
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_MANA,       "selfmana" );   // 030
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_STR,      "targetstr" );   // 031
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_WIS,      "targetwis" );   // 032
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_INT,      "targetint" );   // 033
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_DEX,      "targetdex" );   // 034
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_LIFE,     "targetlife" );   // 035
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_MANA,     "targetmana" );   // 036
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_LEVEL,    "targetlevel" );   // 037
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_SPEEDX,   "targetspeedx" );   // 038
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_SPEEDY,   "targetspeedy" );   // 039
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_SPEEDZ,   "targetspeedz" );   // 040
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_SPAWNX,     "selfspawnx" );   // 041
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_SPAWNY,     "selfspawny" );   // 042
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_STATE,      "selfstate" );   // 043
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_STR,        "selfstr" );   // 044
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_WIS,        "selfwis" );   // 045
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_INT,        "selfint" );   // 046
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_DEX,        "selfdex" );   // 047
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_MANAFLOW,   "selfmanaflow" );   // 048
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_MANAFLOW, "targetmanaflow" );   // 049
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ATTACHED,   "selfattached" );   // 050
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SWINGTURN,       "swingturn" );   // 051
  REGISTER_OPCODE( opcode_lst, 'V', VAR_XYDISTANCE,      "xydistance" );   // 052
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_Z,          "selfz" );   // 053
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_ALTITUDE, "targetaltitude" );   // 054
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_Z,        "targetz" );   // 055
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_INDEX,      "selfindex" );   // 056
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_X,         "ownerx" );   // 057
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_Y,         "ownery" );   // 058
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_TURN,      "ownerturn" );   // 059
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_DISTANCE,  "ownerdistance" );   // 060
  REGISTER_OPCODE( opcode_lst, 'V', VAR_OWNER_TURNTO,    "ownerturnto" );   // 061
  REGISTER_OPCODE( opcode_lst, 'V', VAR_XYTURNTO,        "xyturnto" );   // 062
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_MONEY,      "selfmoney" );   // 063
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_ACCEL,      "selfaccel" );   // 064
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_EXP,      "targetexp" );   // 065
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_AMMO,       "selfammo" );   // 066
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_AMMO,     "targetammo" );   // 067
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_MONEY,    "targetmoney" );   // 068
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_TURNAWAY, "targetturnfrom" );   // 069
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_LEVEL,      "selflevel" );   // 070
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SPAWN_DISTANCE,  "selfspawndistance" );   // 070
  REGISTER_OPCODE( opcode_lst, 'V', VAR_TARGET_MAX_LIFE, "targetmaxlife" );   // 070
  REGISTER_OPCODE( opcode_lst, 'V', VAR_SELF_CONTENT,    "selfcontent" );   // 070

  // register internal constants
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_LEFT, "LATCHLEFT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_RIGHT, "LATCHRIGHT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_JUMP, "LATCHJUMP" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_ALTLEFT, "LATCHALTLEFT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_ALTRIGHT, "LATCHALTRIGHT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_PACKLEFT, "LATCHPACKLEFT" );
  REGISTER_OPCODE( opcode_lst, 'C', LATCHBUTTON_PACKRIGHT, "LATCHPACKRIGHT" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_SLASH, "DAMAGESLASH" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_CRUSH, "DAMAGECRUSH" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_POKE, "DAMAGEPOKE" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_HOLY, "DAMAGEHOLY" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_EVIL, "DAMAGEEVIL" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_FIRE, "DAMAGEFIRE" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_ICE, "DAMAGEICE" );
  REGISTER_OPCODE( opcode_lst, 'C', DAMAGE_ZAP, "DAMAGEZAP" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_DA, "ACTIONDA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_DB, "ACTIONDB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_DC, "ACTIONDC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_DD, "ACTIONDD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_UA, "ACTIONUA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_UB, "ACTIONUB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_UC, "ACTIONUC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_UD, "ACTIONUD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_TA, "ACTIONTA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_TB, "ACTIONTB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_TC, "ACTIONTC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_TD, "ACTIONTD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_CA, "ACTIONCA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_CB, "ACTIONCB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_CC, "ACTIONCC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_CD, "ACTIONCD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_SA, "ACTIONSA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_SB, "ACTIONSB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_SC, "ACTIONSC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_SD, "ACTIONSD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_BA, "ACTIONBA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_BB, "ACTIONBB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_BC, "ACTIONBC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_BD, "ACTIONBD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_LA, "ACTIONLA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_LB, "ACTIONLB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_LC, "ACTIONLC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_LD, "ACTIONLD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_XA, "ACTIONXA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_XB, "ACTIONXB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_XC, "ACTIONXC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_XD, "ACTIONXD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_FA, "ACTIONFA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_FB, "ACTIONFB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_FC, "ACTIONFC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_FD, "ACTIONFD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_PA, "ACTIONPA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_PB, "ACTIONPB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_PC, "ACTIONPC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_PD, "ACTIONPD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_EA, "ACTIONEA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_EB, "ACTIONEB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_RA, "ACTIONRA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ZA, "ACTIONZA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ZB, "ACTIONZB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ZC, "ACTIONZC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ZD, "ACTIONZD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_WA, "ACTIONWA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_WB, "ACTIONWB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_WC, "ACTIONWC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_WD, "ACTIONWD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_JA, "ACTIONJA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_JB, "ACTIONJB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_JC, "ACTIONJC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_HA, "ACTIONHA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_HB, "ACTIONHB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_HC, "ACTIONHC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_HD, "ACTIONHD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_KA, "ACTIONKA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_KB, "ACTIONKB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_KC, "ACTIONKC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_KD, "ACTIONKD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MA, "ACTIONMA" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MB, "ACTIONMB" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MC, "ACTIONMC" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MD, "ACTIONMD" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ME, "ACTIONME" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MF, "ACTIONMF" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MG, "ACTIONMG" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MH, "ACTIONMH" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MI, "ACTIONMI" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MJ, "ACTIONMJ" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MK, "ACTIONMK" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_ML, "ACTIONML" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MM, "ACTIONMM" );
  REGISTER_OPCODE( opcode_lst, 'C', ACTION_MN, "ACTIONMN" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_FINDSECRET, "EXPSECRET" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_WINQUEST, "EXPQUEST" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_USEDUNKOWN, "EXPDARE" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_KILLENEMY, "EXPKILL" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_KILLSLEEPY, "EXPMURDER" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_KILLHATED, "EXPREVENGE" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_TEAMKILL, "EXPTEAMWORK" );
  REGISTER_OPCODE( opcode_lst, 'C', XP_TALKGOOD, "EXPROLEPLAY" );
  REGISTER_OPCODE( opcode_lst, 'C', MPDFX_NOREFLECT, "FXNOREFLECT" );
  REGISTER_OPCODE( opcode_lst, 'C', MPDFX_SHINY,     "FXDRAWREFLECT" );
  REGISTER_OPCODE( opcode_lst, 'C', MPDFX_ANIM,      "FXANIM" );
  REGISTER_OPCODE( opcode_lst, 'C', MPDFX_WATER,     "FXWATER" );
  REGISTER_OPCODE( opcode_lst, 'C', MPDFX_WALL,      "FXBARRIER" );
  REGISTER_OPCODE( opcode_lst, 'C', MPDFX_IMPASS,    "FXIMPASS" );
  REGISTER_OPCODE( opcode_lst, 'C', MPDFX_DAMAGE,    "FXDAMAGE" );
  REGISTER_OPCODE( opcode_lst, 'C', MPDFX_SLIPPY,    "FXSLIPPY" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_SADDLE, "GRIPONLY" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_LEFT,   "GRIPLEFT" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_RIGHT,  "GRIPRIGHT" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_ORIGIN, "SPAWNORIGIN" );
  REGISTER_OPCODE( opcode_lst, 'C', GRIP_LAST,   "SPAWNLAST" );

  REGISTER_OPCODE( opcode_lst, 'C', COLR_WHITE,  "WHITE" );
  REGISTER_OPCODE( opcode_lst, 'C', COLR_RED,    "RED" );
  REGISTER_OPCODE( opcode_lst, 'C', COLR_YELLOW, "YELLOW" );
  REGISTER_OPCODE( opcode_lst, 'C', COLR_GREEN,  "GREEN" );
  REGISTER_OPCODE( opcode_lst, 'C', COLR_BLUE,   "BLUE" );
  REGISTER_OPCODE( opcode_lst, 'C', COLR_PURPLE, "PURPLE" );

  REGISTER_OPCODE( opcode_lst, 'C', MOVE_MELEE,    "MOVEMELEE" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_RANGED,   "MOVERANGED" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_DISTANCE, "MOVEDISTANCE" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_RETREAT,  "MOVERETREAT" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_CHARGE,   "MOVECHARGE" );
  REGISTER_OPCODE( opcode_lst, 'C', MOVE_FOLLOW,   "MOVEFOLLOW" );

  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_DEAD,    "BLAHDEAD" );
  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_ENEMIES, "BLAHENEMIES" );
  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_FRIENDS, "BLAHFRIENDS" );
  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_ITEMS,   "BLAHITEMS" );
  REGISTER_OPCODE( opcode_lst, 'C', SEARCH_INVERT,  "BLAHINVERTID" );

  // register all the function opcodes
  REGISTER_FUNCTION( opcode_lst, IfSpawned);
  REGISTER_FUNCTION( opcode_lst, IfTimeOut);
  REGISTER_FUNCTION( opcode_lst, IfAtWaypoint);
  REGISTER_FUNCTION( opcode_lst, IfAtLastWaypoint);
  REGISTER_FUNCTION( opcode_lst, IfAttacked);
  REGISTER_FUNCTION( opcode_lst, IfBumped);
  REGISTER_FUNCTION( opcode_lst, IfSignaled);
  REGISTER_FUNCTION( opcode_lst, IfCalledForHelp);
  REGISTER_FUNCTION( opcode_lst, SetContent);
  REGISTER_FUNCTION( opcode_lst, IfKilled);
  REGISTER_FUNCTION( opcode_lst, IfTargetKilled);
  REGISTER_FUNCTION( opcode_lst, ClearWaypoints);
  REGISTER_FUNCTION( opcode_lst, AddWaypoint);
  REGISTER_FUNCTION( opcode_lst, FindPath);
  REGISTER_FUNCTION( opcode_lst, Compass);
  REGISTER_FUNCTION( opcode_lst, GetTargetArmorPrice);
  REGISTER_FUNCTION( opcode_lst, SetTime);
  REGISTER_FUNCTION( opcode_lst, GetContent);
  REGISTER_FUNCTION( opcode_lst, JoinTargetTeam);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearbyEnemy);
  REGISTER_FUNCTION( opcode_lst, SetTargetToTargetLeftHand);
  REGISTER_FUNCTION( opcode_lst, SetTargetToTargetRightHand);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverAttacked);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverAttacked);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverBumped);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverCalledForHelp);
  REGISTER_FUNCTION( opcode_lst, SetTargetToOldTarget);
  REGISTER_FUNCTION( opcode_lst, SetTurnModeToVelocity);
  REGISTER_FUNCTION( opcode_lst, SetTurnModeToWatch);
  REGISTER_FUNCTION( opcode_lst, SetTurnModeToSpin);
  REGISTER_FUNCTION( opcode_lst, SetBumpHeight);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasID);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasItemID);
  REGISTER_FUNCTION( opcode_lst, IfTargetHoldingItemID);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasSkillID);
  REGISTER_FUNCTION( opcode_lst, Else);
  REGISTER_FUNCTION( opcode_lst, Run);
  REGISTER_FUNCTION( opcode_lst, Walk);
  REGISTER_FUNCTION( opcode_lst, Sneak);
  REGISTER_FUNCTION( opcode_lst, DoAction);
  REGISTER_FUNCTION( opcode_lst, KeepAction);
  REGISTER_FUNCTION( opcode_lst, SignalTeam);
  REGISTER_FUNCTION( opcode_lst, DropWeapons);
  REGISTER_FUNCTION( opcode_lst, TargetDoAction);
  REGISTER_FUNCTION( opcode_lst, OpenPassage);
  REGISTER_FUNCTION( opcode_lst, ClosePassage);
  REGISTER_FUNCTION( opcode_lst, IfPassageOpen);
  REGISTER_FUNCTION( opcode_lst, GoPoof);
  REGISTER_FUNCTION( opcode_lst, CostTargetItemID);
  REGISTER_FUNCTION( opcode_lst, DoActionOverride);
  REGISTER_FUNCTION( opcode_lst, IfHealed);
  REGISTER_FUNCTION( opcode_lst, DisplayMessage);
  REGISTER_FUNCTION( opcode_lst, CallForHelp);
  REGISTER_FUNCTION( opcode_lst, AddIDSZ);
  REGISTER_FUNCTION( opcode_lst, End);
  REGISTER_FUNCTION( opcode_lst, SetState);
  REGISTER_FUNCTION( opcode_lst, GetState);
  REGISTER_FUNCTION( opcode_lst, IfStateIs);
  REGISTER_FUNCTION( opcode_lst, IfTargetCanOpenStuff);
  REGISTER_FUNCTION( opcode_lst, IfGrabbed);
  REGISTER_FUNCTION( opcode_lst, IfDropped);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverIsHolding);
  REGISTER_FUNCTION( opcode_lst, DamageTarget);
  REGISTER_FUNCTION( opcode_lst, IfXIsLessThanY);
  REGISTER_FUNCTION( opcode_lst, SetWeatherTime);
  REGISTER_FUNCTION( opcode_lst, GetBumpHeight);
  REGISTER_FUNCTION( opcode_lst, IfReaffirmed);
  REGISTER_FUNCTION( opcode_lst, UnkeepAction);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOnOtherTeam);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOnHatedTeam);
  REGISTER_FUNCTION( opcode_lst, PressLatchButton);
  REGISTER_FUNCTION( opcode_lst, SetTargetToTargetOfLeader);
  REGISTER_FUNCTION( opcode_lst, IfLeaderKilled);
  REGISTER_FUNCTION( opcode_lst, BecomeLeader);
  REGISTER_FUNCTION( opcode_lst, ChangeTargetArmor);
  REGISTER_FUNCTION( opcode_lst, GiveMoneyToTarget);
  REGISTER_FUNCTION( opcode_lst, DropKeys);
  REGISTER_FUNCTION( opcode_lst, IfLeaderIsAlive);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOldTarget);
  REGISTER_FUNCTION( opcode_lst, SetTargetToLeader);
  REGISTER_FUNCTION( opcode_lst, SpawnCharacter);
  REGISTER_FUNCTION( opcode_lst, RespawnCharacter);
  REGISTER_FUNCTION( opcode_lst, ChangeTile);
  REGISTER_FUNCTION( opcode_lst, IfUsed);
  REGISTER_FUNCTION( opcode_lst, DropMoney);
  REGISTER_FUNCTION( opcode_lst, SetOldTarget);
  REGISTER_FUNCTION( opcode_lst, DetachFromHolder);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasVulnerabilityID);
  REGISTER_FUNCTION( opcode_lst, CleanUp);
  REGISTER_FUNCTION( opcode_lst, IfCleanedUp);
  REGISTER_FUNCTION( opcode_lst, IfSitting);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsHurt);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAPlayer);
  REGISTER_FUNCTION( opcode_lst, PlaySound);
  REGISTER_FUNCTION( opcode_lst, SpawnParticle);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAlive);
  REGISTER_FUNCTION( opcode_lst, Stop);
  REGISTER_FUNCTION( opcode_lst, DisaffirmCharacter);
  REGISTER_FUNCTION( opcode_lst, ReaffirmCharacter);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsSelf);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsMale);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsFemale);
  REGISTER_FUNCTION( opcode_lst, SetTargetToSelf);
  REGISTER_FUNCTION( opcode_lst, SetTargetToRider);
  REGISTER_FUNCTION( opcode_lst, GetAttackTurn);
  REGISTER_FUNCTION( opcode_lst, GetDamageType);
  REGISTER_FUNCTION( opcode_lst, BecomeSpell);
  REGISTER_FUNCTION( opcode_lst, BecomeSpellbook);
  REGISTER_FUNCTION( opcode_lst, IfScoredAHit);
  REGISTER_FUNCTION( opcode_lst, IfDisaffirmed);
  REGISTER_FUNCTION( opcode_lst, DecodeOrder);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverWasHit);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWideEnemy);
  REGISTER_FUNCTION( opcode_lst, IfChanged);
  REGISTER_FUNCTION( opcode_lst, IfInWater);
  REGISTER_FUNCTION( opcode_lst, IfBored);
  REGISTER_FUNCTION( opcode_lst, IfTooMuchBaggage);
  REGISTER_FUNCTION( opcode_lst, IfGrogged);
  REGISTER_FUNCTION( opcode_lst, IfDazed);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasSpecialID);
  REGISTER_FUNCTION( opcode_lst, PressTargetLatchButton);
  REGISTER_FUNCTION( opcode_lst, IfInvisible);
  REGISTER_FUNCTION( opcode_lst, IfArmorIs);
  REGISTER_FUNCTION( opcode_lst, GetTargetGrogTime);
  REGISTER_FUNCTION( opcode_lst, GetTargetDazeTime);
  REGISTER_FUNCTION( opcode_lst, SetDamageType);
  REGISTER_FUNCTION( opcode_lst, SetWaterLevel);
  REGISTER_FUNCTION( opcode_lst, EnchantTarget);
  REGISTER_FUNCTION( opcode_lst, EnchantChild);
  REGISTER_FUNCTION( opcode_lst, TeleportTarget);
  REGISTER_FUNCTION( opcode_lst, GiveExperienceToTarget);
  REGISTER_FUNCTION( opcode_lst, IncreaseAmmo);
  REGISTER_FUNCTION( opcode_lst, UnkurseTarget);
  REGISTER_FUNCTION( opcode_lst, GiveExperienceToTargetTeam);
  REGISTER_FUNCTION( opcode_lst, IfUnarmed);
  REGISTER_FUNCTION( opcode_lst, RestockTargetAmmoIDAll);
  REGISTER_FUNCTION( opcode_lst, RestockTargetAmmoIDFirst);
  REGISTER_FUNCTION( opcode_lst, FlashTarget);
  REGISTER_FUNCTION( opcode_lst, SetRedShift);
  REGISTER_FUNCTION( opcode_lst, SetGreenShift);
  REGISTER_FUNCTION( opcode_lst, SetBlueShift);
  REGISTER_FUNCTION( opcode_lst, SetLight);
  REGISTER_FUNCTION( opcode_lst, SetAlpha);
  REGISTER_FUNCTION( opcode_lst, IfHitFromBehind);
  REGISTER_FUNCTION( opcode_lst, IfHitFromFront);
  REGISTER_FUNCTION( opcode_lst, IfHitFromLeft);
  REGISTER_FUNCTION( opcode_lst, IfHitFromRight);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOnSameTeam);
  REGISTER_FUNCTION( opcode_lst, KillTarget);
  REGISTER_FUNCTION( opcode_lst, UndoEnchant);
  REGISTER_FUNCTION( opcode_lst, GetWaterLevel);
  REGISTER_FUNCTION( opcode_lst, CostTargetMana);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasAnyID);
  REGISTER_FUNCTION( opcode_lst, SetBumpSize);
  REGISTER_FUNCTION( opcode_lst, IfNotDropped);
  REGISTER_FUNCTION( opcode_lst, IfYIsLessThanX);
  REGISTER_FUNCTION( opcode_lst, IfYIsLessThanX);
  REGISTER_FUNCTION( opcode_lst, SetFlyHeight);
  REGISTER_FUNCTION( opcode_lst, IfBlocked);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsDefending);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAttacking);
  REGISTER_FUNCTION( opcode_lst, IfStateIs0);
  REGISTER_FUNCTION( opcode_lst, IfStateIs0);
  REGISTER_FUNCTION( opcode_lst, IfStateIs1);
  REGISTER_FUNCTION( opcode_lst, IfStateIs1);
  REGISTER_FUNCTION( opcode_lst, IfStateIs2);
  REGISTER_FUNCTION( opcode_lst, IfStateIs2);
  REGISTER_FUNCTION( opcode_lst, IfStateIs3);
  REGISTER_FUNCTION( opcode_lst, IfStateIs3);
  REGISTER_FUNCTION( opcode_lst, IfStateIs4);
  REGISTER_FUNCTION( opcode_lst, IfStateIs4);
  REGISTER_FUNCTION( opcode_lst, IfStateIs5);
  REGISTER_FUNCTION( opcode_lst, IfStateIs5);
  REGISTER_FUNCTION( opcode_lst, IfStateIs6);
  REGISTER_FUNCTION( opcode_lst, IfStateIs6);
  REGISTER_FUNCTION( opcode_lst, IfStateIs7);
  REGISTER_FUNCTION( opcode_lst, IfStateIs7);
  REGISTER_FUNCTION( opcode_lst, IfContentIs);
  REGISTER_FUNCTION( opcode_lst, SetTurnModeToWatchTarget);
  REGISTER_FUNCTION( opcode_lst, IfStateIsNot);
  REGISTER_FUNCTION( opcode_lst, IfXIsEqualToY);
  REGISTER_FUNCTION( opcode_lst, DisplayDebugMessage);
  REGISTER_FUNCTION( opcode_lst, BlackTarget);
  REGISTER_FUNCTION( opcode_lst, DisplayMessageNear);
  REGISTER_FUNCTION( opcode_lst, IfHitGround);
  REGISTER_FUNCTION( opcode_lst, IfNameIsKnown);
  REGISTER_FUNCTION( opcode_lst, IfUsageIsKnown);
  REGISTER_FUNCTION( opcode_lst, IfHoldingItemID);
  REGISTER_FUNCTION( opcode_lst, IfHoldingRangedWeapon);
  REGISTER_FUNCTION( opcode_lst, IfHoldingMeleeWeapon);
  REGISTER_FUNCTION( opcode_lst, IfHoldingShield);
  REGISTER_FUNCTION( opcode_lst, IfKursed);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsKursed);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsDressedUp);
  REGISTER_FUNCTION( opcode_lst, IfOverWater);
  REGISTER_FUNCTION( opcode_lst, IfThrown);
  REGISTER_FUNCTION( opcode_lst, MakeNameKnown);
  REGISTER_FUNCTION( opcode_lst, MakeUsageKnown);
  REGISTER_FUNCTION( opcode_lst, StopTargetMovement);
  REGISTER_FUNCTION( opcode_lst, SetXY);
  REGISTER_FUNCTION( opcode_lst, GetXY);
  REGISTER_FUNCTION( opcode_lst, AddXY);
  REGISTER_FUNCTION( opcode_lst, MakeAmmoKnown);
  REGISTER_FUNCTION( opcode_lst, SpawnAttachedParticle);
  REGISTER_FUNCTION( opcode_lst, SpawnExactParticle);
  REGISTER_FUNCTION( opcode_lst, AccelerateTarget);
  REGISTER_FUNCTION( opcode_lst, IfDistanceIsMoreThanTurn);
  REGISTER_FUNCTION( opcode_lst, IfCrushed);
  REGISTER_FUNCTION( opcode_lst, MakeCrushValid);
  REGISTER_FUNCTION( opcode_lst, SetTargetToLowestTarget);
  REGISTER_FUNCTION( opcode_lst, IfNotPutAway);
  REGISTER_FUNCTION( opcode_lst, IfNotPutAway);
  REGISTER_FUNCTION( opcode_lst, IfTakenOut);
  REGISTER_FUNCTION( opcode_lst, IfAmmoOut);
  REGISTER_FUNCTION( opcode_lst, PlaySoundLooped);
  REGISTER_FUNCTION( opcode_lst, StopSoundLoop);
  REGISTER_FUNCTION( opcode_lst, HealSelf);
  REGISTER_FUNCTION( opcode_lst, Equip);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasItemIDEquipped);
  REGISTER_FUNCTION( opcode_lst, SetOwnerToTarget);
  REGISTER_FUNCTION( opcode_lst, SetTargetToOwner);
  REGISTER_FUNCTION( opcode_lst, SetFrame);
  REGISTER_FUNCTION( opcode_lst, BreakPassage);
  REGISTER_FUNCTION( opcode_lst, SetReloadTime);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWideBlahID);
  REGISTER_FUNCTION( opcode_lst, PoofTarget);
  REGISTER_FUNCTION( opcode_lst, ChildDoActionOverride);
  REGISTER_FUNCTION( opcode_lst, SpawnPoof);
  REGISTER_FUNCTION( opcode_lst, SetSpeedPercent);
  REGISTER_FUNCTION( opcode_lst, SetChildState);
  REGISTER_FUNCTION( opcode_lst, SpawnAttachedSizedParticle);
  REGISTER_FUNCTION( opcode_lst, ChangeArmor);
  REGISTER_FUNCTION( opcode_lst, ShowTimer);
  REGISTER_FUNCTION( opcode_lst, IfFacingTarget);
  REGISTER_FUNCTION( opcode_lst, PlaySoundVolume);
  REGISTER_FUNCTION( opcode_lst, SpawnAttachedFacedParticle);
  REGISTER_FUNCTION( opcode_lst, IfStateIsOdd);
  REGISTER_FUNCTION( opcode_lst, SetTargetToDistantEnemy);
  REGISTER_FUNCTION( opcode_lst, Teleport);
  REGISTER_FUNCTION( opcode_lst, GiveStrengthToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveWisdomToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveIntelligenceToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveDexterityToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveLifeToTarget);
  REGISTER_FUNCTION( opcode_lst, GiveManaToTarget);
  REGISTER_FUNCTION( opcode_lst, ShowMap);
  REGISTER_FUNCTION( opcode_lst, ShowYouAreHere);
  REGISTER_FUNCTION( opcode_lst, ShowBlipXY);
  REGISTER_FUNCTION( opcode_lst, HealTarget);
  REGISTER_FUNCTION( opcode_lst, PumpTarget);
  REGISTER_FUNCTION( opcode_lst, CostAmmo);
  REGISTER_FUNCTION( opcode_lst, MakeSimilarNamesKnown);
  REGISTER_FUNCTION( opcode_lst, SpawnAttachedHolderParticle);
  REGISTER_FUNCTION( opcode_lst, SetTargetReloadTime);
  REGISTER_FUNCTION( opcode_lst, SetFogLevel);
  REGISTER_FUNCTION( opcode_lst, GetFogLevel);
  REGISTER_FUNCTION( opcode_lst, SetFogTAD);
  REGISTER_FUNCTION( opcode_lst, SetFogBottomLevel);
  REGISTER_FUNCTION( opcode_lst, GetFogBottomLevel);
  REGISTER_FUNCTION( opcode_lst, CorrectActionForHand);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsMounted);
  REGISTER_FUNCTION( opcode_lst, SparkleIcon);
  REGISTER_FUNCTION( opcode_lst, UnsparkleIcon);
  REGISTER_FUNCTION( opcode_lst, GetTileXY);
  REGISTER_FUNCTION( opcode_lst, SetTileXY);
  REGISTER_FUNCTION( opcode_lst, SetShadowSize);
  REGISTER_FUNCTION( opcode_lst, SignalTarget);
  REGISTER_FUNCTION( opcode_lst, SetTargetToWhoeverIsInPassage);
  REGISTER_FUNCTION( opcode_lst, IfCharacterWasABook);
  REGISTER_FUNCTION( opcode_lst, SetEnchantBoostValues);
  REGISTER_FUNCTION( opcode_lst, SpawnCharacterXYZ);
  REGISTER_FUNCTION( opcode_lst, SpawnExactCharacterXYZ);
  REGISTER_FUNCTION( opcode_lst, ChangeTargetClass);
  REGISTER_FUNCTION( opcode_lst, PlayFullSound);
  REGISTER_FUNCTION( opcode_lst, SpawnExactChaseParticle);
  REGISTER_FUNCTION( opcode_lst, EncodeOrder);
  REGISTER_FUNCTION( opcode_lst, SignalSpecialID);
  REGISTER_FUNCTION( opcode_lst, UnkurseTargetInventory);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsSneaking);
  REGISTER_FUNCTION( opcode_lst, DropItems);
  REGISTER_FUNCTION( opcode_lst, RespawnTarget);
  REGISTER_FUNCTION( opcode_lst, TargetDoActionSetFrame);
  REGISTER_FUNCTION( opcode_lst, IfTargetCanSeeInvisible);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearestBlahID);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearestEnemy);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearestFriend);
  REGISTER_FUNCTION( opcode_lst, SetTargetToNearestLifeform);
  REGISTER_FUNCTION( opcode_lst, FlashPassage);
  REGISTER_FUNCTION( opcode_lst, FindTileInPassage);
  REGISTER_FUNCTION( opcode_lst, IfHeldInLeftSaddle);
  REGISTER_FUNCTION( opcode_lst, NotAnItem);
  REGISTER_FUNCTION( opcode_lst, SetChildAmmo);
  REGISTER_FUNCTION( opcode_lst, IfHitVulnerable);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsFlying);
  REGISTER_FUNCTION( opcode_lst, IdentifyTarget);
  REGISTER_FUNCTION( opcode_lst, BeatModule);
  REGISTER_FUNCTION( opcode_lst, EndModule);
  REGISTER_FUNCTION( opcode_lst, DisableExport);
  REGISTER_FUNCTION( opcode_lst, EnableExport);
  REGISTER_FUNCTION( opcode_lst, GetTargetState);
  REGISTER_FUNCTION( opcode_lst, SetSpeech);
  REGISTER_FUNCTION( opcode_lst, SetMoveSpeech);
  REGISTER_FUNCTION( opcode_lst, SetSecondMoveSpeech);
  REGISTER_FUNCTION( opcode_lst, SetAttackSpeech);
  REGISTER_FUNCTION( opcode_lst, SetAssistSpeech);
  REGISTER_FUNCTION( opcode_lst, SetTerrainSpeech);
  REGISTER_FUNCTION( opcode_lst, SetSelectSpeech);
  REGISTER_FUNCTION( opcode_lst, ClearEndText);
  REGISTER_FUNCTION( opcode_lst, AddEndText);
  REGISTER_FUNCTION( opcode_lst, PlayMusic);
  REGISTER_FUNCTION( opcode_lst, SetMusicPassage);
  REGISTER_FUNCTION( opcode_lst, MakeCrushInvalid);
  REGISTER_FUNCTION( opcode_lst, StopMusic);
  REGISTER_FUNCTION( opcode_lst, FlashVariable);
  REGISTER_FUNCTION( opcode_lst, AccelerateUp);
  REGISTER_FUNCTION( opcode_lst, FlashVariableHeight);
  REGISTER_FUNCTION( opcode_lst, SetDamageTime);
  REGISTER_FUNCTION( opcode_lst, IfStateIs8);
  REGISTER_FUNCTION( opcode_lst, IfStateIs9);
  REGISTER_FUNCTION( opcode_lst, IfStateIs10);
  REGISTER_FUNCTION( opcode_lst, IfStateIs11);
  REGISTER_FUNCTION( opcode_lst, IfStateIs12);
  REGISTER_FUNCTION( opcode_lst, IfStateIs13);
  REGISTER_FUNCTION( opcode_lst, IfStateIs14);
  REGISTER_FUNCTION( opcode_lst, IfStateIs15);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAMount);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsAPlatform);
  REGISTER_FUNCTION( opcode_lst, AddStat);
  REGISTER_FUNCTION( opcode_lst, DisenchantTarget);
  REGISTER_FUNCTION( opcode_lst, DisenchantAll);
  REGISTER_FUNCTION( opcode_lst, SetVolumeNearestTeammate);
  REGISTER_FUNCTION( opcode_lst, AddShopPassage);
  REGISTER_FUNCTION( opcode_lst, TargetPayForArmor);
  REGISTER_FUNCTION( opcode_lst, JoinEvilTeam);
  REGISTER_FUNCTION( opcode_lst, JoinNullTeam);
  REGISTER_FUNCTION( opcode_lst, JoinGoodTeam);
  REGISTER_FUNCTION( opcode_lst, PitsKill);
  REGISTER_FUNCTION( opcode_lst, SetTargetToPassageID);
  REGISTER_FUNCTION( opcode_lst, MakeNameUnknown);
  REGISTER_FUNCTION( opcode_lst, SpawnExactParticleEndSpawn);
  REGISTER_FUNCTION( opcode_lst, SpawnPoofSpeedSpacingDamage);
  REGISTER_FUNCTION( opcode_lst, GiveExperienceToGoodTeam);
  REGISTER_FUNCTION( opcode_lst, DoNothing);
  REGISTER_FUNCTION( opcode_lst, DazeTarget);
  REGISTER_FUNCTION( opcode_lst, GrogTarget);
  REGISTER_FUNCTION( opcode_lst, AddQuest);
  REGISTER_FUNCTION( opcode_lst, BeatQuest);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasQuest);
  REGISTER_FUNCTION( opcode_lst, SetQuestLevel);
  REGISTER_FUNCTION( opcode_lst, IfTargetHasNotFullMana);
  REGISTER_FUNCTION( opcode_lst, IfDoingAction);
  REGISTER_FUNCTION( opcode_lst, DropTargetKeys);
  REGISTER_FUNCTION( opcode_lst, TargetJoinTeam);
  REGISTER_FUNCTION( opcode_lst, GetTargetContent);
  REGISTER_FUNCTION( opcode_lst, JoinTeam);
  REGISTER_FUNCTION( opcode_lst, TargetJoinTeam);
  REGISTER_FUNCTION( opcode_lst, ClearMusicPassage);
  REGISTER_FUNCTION( opcode_lst, IfOperatorIsLinux);
  REGISTER_FUNCTION( opcode_lst, IfTargetIsOwner);
  REGISTER_FUNCTION( opcode_lst, SetCameraSwing);
  REGISTER_FUNCTION( opcode_lst, EnableRespawn);
  REGISTER_FUNCTION( opcode_lst, DisableRespawn);
  REGISTER_FUNCTION( opcode_lst, IfButtonPressed);
  REGISTER_FUNCTION( opcode_lst, IfHolderScoredAHit);


  // register all the function !!!ALIASES!!!
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfAtLastWaypoint, "IfPutAway" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfSignaled, "IfOrdered" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, SetTargetToWhoeverAttacked, "SetTargetToWhoeverHealed" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, SignalTeam, "IssueOrder" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, DisplayMessage, "SendMessage" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfGrabbed, "IfMounted" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfDropped, "IfDismounted" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfXIsLessThanY, "IfYIsMoreThanX" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfSitting, "IfHeld" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, DecodeOrder, "TranslateOrder" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfYIsLessThanX, "IfXIsMoreThanY" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs0, "IfStateIsParry" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs1, "IfStateIsWander" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs2, "IfStateIsGuard" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs3, "IfStateIsFollow" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs4, "IfStateIsSurround" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs5, "IfStateIsRetreat" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs6, "IfStateIsCharge" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfStateIs7, "IfStateIsCombat" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfXIsEqualToY, "IfYIsEqualToX" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, DisplayDebugMessage, "DebugMessage" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, DisplayMessageNear, "SendMessageNear" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfNotPutAway, "IfNotTakenOut" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, SignalTarget, "OrderTarget" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, EncodeOrder, "CreateOrder" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, SignalSpecialID, "OrderSpecialID" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, IfHeldInLeftSaddle, "IfHeldInLeftHand" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, ClearEndText, "ClearEndMessage" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, AddEndText, "AddEndMessage" );
  REGISTER_FUNCTION_ALIAS( opcode_lst, StopSoundLoop, "StopSound" );

  log_info( "load_ai_codes() - \n\tloading external script constants\n" );

  // read all of the remaining opcodes from the file
  fileread = fs_fileOpen( PRI_NONE, NULL, loadname, "rb" );
  if ( NULL == fileread )
  {
    log_warning( "Could not load script AI functions or variables (%s)\n", loadname );
    return;
  }

  while ( fgoto_colon_yesno( fileread ) && !feof( fileread ) )
  {
    fget_code( fileread );
  }

  fs_fileClose( fileread );
}


//------------------------------------------------------------------------------
Uint32 load_ai_script( ScriptInfo_t * slist, const char * szObjectpath, const char * szObjectname )
{
  // ZZ> This function loads a script to memory and
  //     returns bfalse if it fails to do so

  FILE * fileread;
  char * loc_fname;
  Uint32 script_idx = AILST_COUNT;

  if(NULL == slist || slist->offset_count >= AILST_COUNT) return AILST_COUNT;

  loc_fname = inherit_fname(szObjectpath, szObjectname, CData.script_file);
  globalparsename = loc_fname;
  fileread = fs_fileOpen(PRI_NONE, "load_ai_script()", loc_fname, "r");
  if ( NULL == fileread ) return script_idx;

  // Load into md2 load buffer
  _cstate.line_num  = 0;
  _cstate.file_size = ( int ) fread( _cstate.file_buffer, 1, MD2MAXLOADSIZE, fileread );
  fs_fileClose( fileread );

  // prepare the file for compiling
  parse_null_terminate_comments();

  // grab a free script index
  script_idx = slist->offset_count;
  slist->offset_count++;

  // store some info
  slist->offset_stt[script_idx] = slist->buffer_index;

  // stage 1 of compilation
  parse_line_by_line( slist );

  // stage 2 of compilation
  parse_jumps( slist, slist->offset_stt[script_idx], slist->buffer_index );

  // store some debug info
  strcpy(slist->fname[script_idx], loc_fname);
  slist->offset_end[script_idx] = slist->buffer_index;


  return script_idx;
}

//------------------------------------------------------------------------------

bool_t ScriptInfo_reset( ScriptInfo_t * si )
{
  if(NULL == si) return bfalse;

  si->buffer_index = 0;
  si->offset_count = 0;

  return btrue;
};

//------------------------------------------------------------------------------
void reset_ai_script(Game_t * gs)
{
  // ZZ> This function starts ai loading in the right spot

  OBJ_REF obj_cnt;

  ScriptInfo_reset( Game_getScriptInfo(gs) );

  for ( obj_cnt = 0; obj_cnt < MADLST_COUNT; obj_cnt++ )
  {
    gs->ObjList[obj_cnt].ai = 0;
  }

}

//--------------------------------------------------------------------------------------------
bool_t run_function( Game_t * gs, Uint32 value, CHR_REF ichr )
{
  // ZZ> This function runs a script function for the AI.
  //     It returns bfalse if the script should jump over the
  //     indented code that follows

  // Mask out the indentation
  OPCODE opcode = (OPCODE)GET_OPCODE_BITS(value);

  // Assume that the function will pass, as most do
  bool_t returncode = btrue;

  Uint16 sTmp;
  float fTmp;
  int iTmp, tTmp;
  int volume;
  Uint32 test;
  STRING cTmp;
  SearchInfo_t loc_search;
  Uint32 loc_rand;
  ENC_SPAWN_INFO enc_si;

  CHR_REF tmpchr, tmpchr1, tmpchr2, tmpchr3;
  PRT_REF tmpprt;

  ScriptInfo_t * slist = Game_getScriptInfo(gs);

  Chr_t     * pchr = gs->ChrList + ichr;
  AI_STATE * pstate = &(pchr->aistate);

  Obj_t     * pobj = gs->ObjList + pchr->model;

  CHR_REF loc_aichild = chr_get_aichild( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
  Chr_t   * pchild     = gs->ChrList + loc_aichild;

  CHR_REF loc_target = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
  Chr_t   * ptarget   = gs->ChrList + loc_target;

  CHR_REF loc_leader  = team_get_leader( gs, pchr->team );

  Mad_t  * pmad = ChrList_getPMad(gs, ichr);
  Cap_t  * pcap = ChrList_getPCap(gs, ichr);

  Mesh_t * pmesh = Game_getMesh(gs);

  loc_rand = gs->randie_index;

  // Figure out which function to run
  switch ( opcode )
  {
    case F_IfSpawned:
      // Proceed only if it's a new character
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_SPAWNED );
      break;

    case F_IfTimeOut:
      // Proceed only if time alert is set
      returncode = ( pstate->time <= 0.0f );
      break;

    case F_IfAtWaypoint:
      // Proceed only if the character reached a waypoint
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_ATWAYPOINT );
      break;

    case F_IfAtLastWaypoint:
      // Proceed only if the character reached its last waypoint
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_ATLASTWAYPOINT );

      // TODO: should this be
      //returncode = wp_list_empty( &(pstate->wp) );
      // TODO: then we could separate PUTAWAY from ATLASTWAYPOINT
      break;

    case F_IfAttacked:
      // Proceed only if the character was damaged
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_ATTACKED );
      break;

    case F_IfBumped:
      // Proceed only if the character was bumped
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_BUMPED );
      break;

    case F_IfSignaled:
      // Proceed only if the character was GOrder.ed
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_SIGNALED );
      break;

    case F_IfCalledForHelp:
      // Proceed only if the character was called for help
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_CALLEDFORHELP );
      break;

    case F_SetContent:
      // Set the content
      pstate->content = pstate->tmpargument;
      break;

    case F_IfKilled:
      // Proceed only if the character's been killed
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_KILLED );
      break;

    case F_IfTargetKilled:
      // Proceed only if the character's target has just died
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_TARGETKILLED );
      break;

    case F_ClearWaypoints:
      // Clear out all waypoints
      {
        bool_t btemp = !wp_list_empty(&(pstate->wp));
        wp_list_clear( &(pstate->wp) );
        wp_list_add( &(pstate->wp), pchr->ori.pos.x, pchr->ori.pos.y );
        if(btemp)
        {
          pstate->alert |= ALERT_ATLASTWAYPOINT;
        }
      }
      break;

    case F_AddWaypoint:
      // Add a waypoint to the waypoint list
      returncode = wp_list_add( &(pstate->wp), pstate->tmpx, pstate->tmpy);
      if(returncode) pstate->alert &= ~ALERT_ATLASTWAYPOINT;
      break;

    case F_FindPath:
      {
        // This function adds enough waypoints to get from one point to another
        // And only proceeds if the target is not the character himself
        // !!!BAD!!! Todo: Only adds one straight waypoint...

        int src_ix, src_iy;
        int dst_ix, dst_iy;

        src_ix = MESH_FLOAT_TO_FAN(pchr->ori.pos.x);
        src_iy = MESH_FLOAT_TO_FAN(pchr->ori.pos.y);

        dst_ix = MESH_FLOAT_TO_FAN(ptarget->ori.pos.x);
        dst_iy = MESH_FLOAT_TO_FAN(ptarget->ori.pos.y);

        if(src_ix != dst_ix && src_iy != dst_iy)
        {
          bool_t try_astar;

          try_astar = (pstate->tmpdistance == MOVE_MELEE) ||
                      (pstate->tmpdistance == MOVE_CHARGE) ||
                      (pstate->tmpdistance == MOVE_FOLLOW);


          if( try_astar && AStar_prepare_path(gs, pchr->stoppedby, src_ix, src_iy, dst_ix, dst_iy) )
          {
            AStar_Node_t nbuffer[4*MAXWAY + 1];
            int        nbuffer_size = 4*MAXWAY + 1;

            nbuffer_size = AStar_get_path(src_ix, src_iy, nbuffer, nbuffer_size);
            if(nbuffer_size > 0)
            {
              nbuffer_size = AStar_Node_list_prune(nbuffer, nbuffer_size);

              if(nbuffer_size<MAXWAY)
              {
                int i;

                wp_list_clear( &(pstate->wp) );

                // copy the buffer into the waypoints
                for(i=nbuffer_size-1; i>=0; i--)
                {
                  float fx = MESH_FAN_TO_FLOAT(nbuffer[i].ix + 0.5f);
                  float fy = MESH_FAN_TO_FLOAT(nbuffer[i].iy + 0.5f);

                  wp_list_add( &(pstate->wp), fx, fy );
                }

                //// optimize the waypoints
                //wp_list_prune(  &(pstate->wp) );
              }
            }
          }
          else
          {
            // just use a straight line path

            //First setup the variables needed for the target waypoint
            if(pstate->target != ichr)
            {
              if(pstate->tmpdistance != MOVE_FOLLOW)
              {
                pstate->tmpx = ptarget->ori.pos.x;
                pstate->tmpy = ptarget->ori.pos.y;
              }
              else
              {
                pstate->tmpx = FRAND(&loc_rand) * 512 + ptarget->ori.pos.x;
                pstate->tmpy = FRAND(&loc_rand) * 512 + ptarget->ori.pos.y;
              }

              if(pstate->tmpdistance == MOVE_RETREAT)
              {
                pstate->tmpturn = (IRAND(&loc_rand, 15) + 16384) + pstate->tmpturn;
              }
              else
              {
                pstate->tmpturn = vec_to_turn( pstate->tmpx - pchr->ori.pos.x, pstate->tmpy - pchr->ori.pos.y );
              }

              //Secondly we run the Compass function (If we are not in follow mode)
              if(pstate->tmpdistance != MOVE_FOLLOW)
              {
                pstate->tmpx -= turntocos[( pstate->tmpturn>>2 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
                pstate->tmpy -= turntosin[( pstate->tmpturn>>2 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
              }

              //Then we add the waypoint(s), without clearing existing ones...
              returncode = wp_list_add( &(pstate->wp), pstate->tmpx, pstate->tmpy);
              if(returncode) pstate->alert &= ~ALERT_ATLASTWAYPOINT;
            }
          }

          if((pstate->tmpdistance == MOVE_CHARGE) || (pstate->tmpdistance == MOVE_RETREAT))
          {
            chr_reset_accel( gs, ichr ); //Force 100% speed
          }
        }
      }
      break;

    case F_Compass:
      // This function changes tmpx and tmpy in a circlular manner according
      // to tmpturn and tmpdistance
      pstate->tmpx -= turntocos[( pstate->tmpturn>>2 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
      pstate->tmpy -= turntosin[( pstate->tmpturn>>2 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
      break;

    case F_GetTargetArmorPrice:
      // This function gets the armor cost for the given skin
      sTmp = pstate->tmpargument  % MAXSKIN;
      pstate->tmpx = ChrList_getPCap(gs, loc_target)->skin[sTmp].cost;
      break;

    case F_SetTime:
      // This function resets the time
      pstate->time = pstate->tmpargument;
      break;

    case F_GetContent:
      // Get the content
      pstate->tmpargument = pstate->content;
      break;

    case F_JoinTargetTeam:
      // This function allows the character to leave its own team and join another
      returncode = bfalse;
      if ( EKEY_PVALID(ptarget) )
      {
        switch_team( gs, ichr, ptarget->team );
        returncode = btrue;
      }
      break;

    case F_SetTargetToNearbyEnemy:
      // This function finds a nearby enemy, and proceeds only if there is one

      returncode = chr_search_nearby( gs, SearchInfo_new(&loc_search), ichr, bfalse, bfalse, btrue, bfalse, IDSZ_NONE );

      if ( returncode )
      {
        pstate->target = loc_search.besttarget;
      }

      break;

    case F_SetTargetToTargetLeftHand:
      {
      // This function sets the target to the target's left item
        tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, SLOT_LEFT );

        returncode = bfalse;
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          pstate->target = tmpchr;
          returncode = btrue;
        }
      }
      break;

    case F_SetTargetToTargetRightHand:
      {
        // This function sets the target to the target's right item
        tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, SLOT_RIGHT );
        returncode = bfalse;
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          pstate->target = tmpchr;
          returncode = btrue;
        }
      }
      break;

    case F_SetTargetToWhoeverAttacked:
      {
        // This function sets the target to whoever attacked the character last,
        // failing for damage tiles
        CHR_REF attacklast = chr_get_aiattacklast( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
        if ( ACTIVE_CHR( gs->ChrList, attacklast ) )
        {
          pstate->target = attacklast;
        }
        else
        {
          returncode = bfalse;
        }
      }
      break;

    case F_SetTargetToWhoeverBumped:
      // This function sets the target to whoever bumped into the
      // character last.  It never fails
      pstate->target = chr_get_aibumplast( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
      break;

    case F_SetTargetToWhoeverCalledForHelp:
      // This function sets the target to whoever needs help
      pstate->target = team_get_sissy( gs, pchr->team );
      break;

    case F_SetTargetToOldTarget:
      // This function reverts to the target with whom the script started
      pstate->target = pstate->oldtarget;
      break;

    case F_SetTurnModeToVelocity:
      // This function sets the turn mode
      pstate->turnmode = TURNMODE_VELOCITY;
      break;

    case F_SetTurnModeToWatch:
      // This function sets the turn mode
      pstate->turnmode = TURNMODE_WATCH;
      break;

    case F_SetTurnModeToSpin:
      // This function sets the turn mode
      pstate->turnmode = TURNMODE_SPIN;
      break;

    case F_SetBumpHeight:
      // This function changes a character's bump height
      //pchr->bmpdata.height = pstate->tmpargument * pchr->fat;
      //pchr->bmpdata_save.height = pstate->tmpargument;
      break;

    case F_IfTargetHasID:
      // This function proceeds if ID matches tmpargument
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, pstate->target ) )
      {
        returncode = CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, loc_target), pstate->tmpargument );
      };
      break;

    case F_IfTargetHasItemID:
      // This function proceeds if the target has a matching item in his/her pack
      returncode = bfalse;
      {
        // Check the pack
        tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, pstate->target );
        while ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          if ( CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
          {
            returncode = btrue;
            break;
          }
          tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr );
        }

        for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
        {
          tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
          if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
          {
            if ( CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
              returncode = btrue;
          }
        }
      }
      break;

    case F_IfTargetHoldingItemID:
      // This function proceeds if ID matches tmpargument and returns the latch for the
      // hand in tmpargument
      returncode = bfalse;

      if ( ACTIVE_CHR( gs->ChrList, pstate->target ) )
      {
        for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
        {
          tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
          if ( ACTIVE_CHR( gs->ChrList, tmpchr ) && CAP_INHERIT_IDSZ( gs, ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
          {
            pstate->tmpargument = slot_to_latch( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
            returncode = btrue;
          }
        }
      }
      break;

    case F_IfTargetHasSkillID:
      // This function proceeds if ID matches tmpargument
      returncode = check_skills( gs, pstate->target, pstate->tmpargument );
      break;

    case F_Else:
      // This function fails if the last one was more indented
      returncode = (pstate->lastindent <= GET_INDENT( value ));
      break;

    case F_Run:
      chr_reset_accel( gs, ichr );
      break;

    case F_Walk:
      chr_reset_accel( gs, ichr );
      pchr->skin.maxaccel *= .66;
      break;

    case F_Sneak:
      chr_reset_accel( gs, ichr );
      pchr->skin.maxaccel *= .33;
      break;

    case F_DoAction:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid or if the character is doing
      // something else already
      returncode = bfalse;
      if ( pstate->tmpargument < MAXACTION && pchr->action.ready )
      {
        if ( pmad->actionvalid[pstate->tmpargument] )
        {
          pchr->action.now = (ACTION)pstate->tmpargument;
          pchr->action.ready = bfalse;

          pchr->anim.ilip = 0;
          pchr->anim.flip = 0.0f;
          pchr->anim.last = pchr->anim.next;
          pchr->anim.next = pmad->actionstart[pstate->tmpargument];

          returncode = btrue;
        }
      }
      break;

    case F_KeepAction:
      // This function makes the current animation halt on the last frame
      pchr->action.keep = btrue;
      break;

    case F_SignalTeam:
      // This function issues an order to all teammates
      signal_team( gs, ichr, pstate->tmpargument );
      break;

    case F_DropWeapons:
      // This funtion drops the character's in hand items/riders

      for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, ichr, _slot );
        if ( detach_character_from_mount( gs, tmpchr, btrue, btrue ) )
        {
          if ( _slot == SLOT_SADDLE )
          {
            gs->ChrList[tmpchr].ori.vel.z  = DISMOUNTZVEL;
            gs->ChrList[tmpchr].ori.pos.z += DISMOUNTZVEL;
            gs->ChrList[tmpchr].jumptime   = DELAY_JUMP;
          }
        }
      };
      break;

    case F_TargetDoAction:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid or if the target is doing
      // something else already
      returncode = bfalse;
      if ( ptarget->alive )
      {
        if ( pstate->tmpargument < MAXACTION && ptarget->action.ready )
        {
          if ( ChrList_getPMad(gs, loc_target)->actionvalid[pstate->tmpargument] )
          {
            ptarget->action.now = (ACTION)pstate->tmpargument;
            ptarget->action.ready = bfalse;

            ptarget->anim.ilip = 0;
            ptarget->anim.flip    = 0.0f;
            ptarget->anim.last    = ptarget->anim.next;
            ptarget->anim.next    = ChrList_getPMad(gs, loc_target)->actionstart[pstate->tmpargument];

            returncode = btrue;
          }
        }
      }
      break;

    case F_OpenPassage:
      // This function opens the passage specified by tmpargument, failing if the
      // passage was already open
      returncode = open_passage( gs, pstate->tmpargument );
      break;

    case F_ClosePassage:
      // This function closes the passage specified by tmpargument, and proceeds
      // only if the passage is clear of obstructions
      returncode = close_passage( gs, pstate->tmpargument );
      break;

    case F_IfPassageOpen:
      // This function proceeds only if the passage specified by tmpargument
      // is both valid and open
      returncode = bfalse;
      if ( pstate->tmpargument < gs->PassList_count && pstate->tmpargument >= 0 )
      {
        returncode = gs->PassList[pstate->tmpargument].open;
      }
      break;

    case F_GoPoof:
      // This function flags the character to be removed from the game
      returncode = bfalse;
      if ( !chr_is_player(gs, ichr) )
      {
        returncode = btrue;
        pchr->gopoof = btrue;
      }
      break;

    case F_CostTargetItemID:
      // This function checks if the target has a matching item, and poofs it
      returncode = bfalse;
      {
        // Check the pack
        tmpchr1 = INVALID_CHR;
        tmpchr2 = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
        tmpchr3 = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr2 );
        while ( ACTIVE_CHR( gs->ChrList, tmpchr3 ) )
        {
          if ( CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr3), pstate->tmpargument ) )
          {
            returncode = btrue;
            tmpchr1 = tmpchr3;
            break;
          }
          else
          {
            tmpchr2 = tmpchr3;
            tmpchr3  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr3 );
          }
        }

        for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
        {
          tmpchr3 = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
          if ( ACTIVE_CHR( gs->ChrList, tmpchr3 ) )
          {
            if ( CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr3), pstate->tmpargument ) )
            {
              returncode = btrue;
              tmpchr2 = INVALID_CHR;
              tmpchr1 = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
              break;
            }
          }
        };


        if ( returncode )
        {
          if ( gs->ChrList[tmpchr1].ammo <= 1 )
          {
            // Poof the item
            if ( chr_in_pack(gs->ChrList, CHRLST_COUNT,  tmpchr1 ) )
            {
              // Remove from the pack
              gs->ChrList[tmpchr2].nextinpack  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr1 );
              ptarget->numinpack--;
              gs->ChrList[tmpchr1].freeme = btrue;
            }
            else
            {
              // Drop from hand
              detach_character_from_mount( gs, tmpchr1, btrue, bfalse );
              gs->ChrList[tmpchr1].freeme = btrue;
            }
          }
          else
          {
            // Cost one ammo
            gs->ChrList[tmpchr1].ammo--;
          }
        }
      }
      break;

    case F_DoActionOverride:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid
      returncode = bfalse;
      if ( pstate->tmpargument < MAXACTION )
      {
        if ( pmad->actionvalid[pstate->tmpargument] )
        {
          pchr->action.now   = (ACTION)pstate->tmpargument;
          pchr->action.ready = bfalse;

          pchr->anim.ilip = 0;
          pchr->anim.flip    = 0.0f;
          pchr->anim.last    = pchr->anim.next;
          pchr->anim.next    = pmad->actionstart[pstate->tmpargument];

          returncode = btrue;
        }
      }
      break;

    case F_IfHealed:
      // Proceed only if the character was healed
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_HEALED );
      break;

    case F_DisplayMessage:
      // This function sends a message to the players
      display_message( gs,  pobj->msg_start + pstate->tmpargument, ichr );
      break;

    case F_CallForHelp:
      // This function issues a call for help
      call_for_help( gs,  ichr );
      break;

    case F_AddIDSZ:
      // This function adds an idsz to the module's menu.txt file
      module_add_idsz( gs->mod.loadname, pstate->tmpargument );
      break;

    case F_SetState:
      // This function sets the character's state variable
      pstate->state = pstate->tmpargument;
      break;

    case F_GetState:
      // This function reads the character's state variable
      pstate->tmpargument = pstate->state;
      break;

    case F_IfStateIs:
      // This function fails if the character's state is inequal to tmpargument
      returncode = ( pstate->tmpargument == pstate->state );
      break;

    case F_IfTargetCanOpenStuff:
      // This function fails if the target can't open stuff
      returncode = ptarget->prop.canopenstuff;
      break;

    case F_IfGrabbed:
      // Proceed only if the character was picked up
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_GRABBED );
      break;

    case F_IfDropped:
      // Proceed only if the character was dropped
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_DROPPED );
      break;

    case F_SetTargetToWhoeverIsHolding:
      // This function sets the target to the character's mount or holder,
      // failing if the character has no mount or holder
      returncode = bfalse;
      if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        pstate->target = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );
        returncode = btrue;
      }
      break;

    case F_DamageTarget:
      {
        // This function applies little bit of love to the character's target.
        // The amount is set in tmpargument
        PAIR ptemp = {pstate->tmpargument, 1};
        damage_character( gs, pstate->target, 0, &ptemp, pchr->damagetargettype, pchr->team, ichr, DAMFX_BLOC );
      }
      break;

    case F_IfXIsLessThanY:
      // Proceed only if tmpx is less than tmpy
      returncode = ( pstate->tmpx < pstate->tmpy );
      break;

    case F_SetWeatherTime:
      // Set the weather timer
      GWeather.timereset = pstate->tmpargument;
      GWeather.time = pstate->tmpargument;
      break;

    case F_GetBumpHeight:
      // Get the characters bump height
      pstate->tmpargument = pchr->bmpdata.calc_height;
      break;

    case F_IfReaffirmed:
      // Proceed only if the character was reaffirmed
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_REAFFIRMED );
      break;

    case F_UnkeepAction:
      // This function makes the current animation start again
      pchr->action.keep = bfalse;
      break;

    case F_IfTargetIsOnOtherTeam:
      // This function proceeds only if the target is on another team
      returncode = ( ptarget->alive && ptarget->team != pchr->team );
      break;

    case F_IfTargetIsOnHatedTeam:
      // This function proceeds only if the target is on an enemy team
      returncode = ( ptarget->alive && gs->TeamList[pchr->team].hatesteam[ptarget->REF_TO_INT(team)] && !ptarget->prop.invictus );
      break;

    case F_PressLatchButton:
      // This function sets the latch buttons
      pstate->latch.b |= pstate->tmpargument;
      break;

    case F_SetTargetToTargetOfLeader:
      // This function sets the character's target to the target of its leader,
      // or it fails with no change if the leader is dead
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, loc_leader ) )
      {
        pstate->target = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + loc_leader );
        returncode = btrue;
      }
      break;

    case F_IfLeaderKilled:
      // This function proceeds only if the character's leader has just died
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_LEADERKILLED );
      break;

    case F_BecomeLeader:
      // This function makes the character the team leader
      gs->TeamList[pchr->team].leader = ichr;
      break;

    case F_ChangeTargetArmor:
      // This function sets the target's armor type and returns the old type
      // as tmpargument and the new type as tmpx
      iTmp = ptarget->skin_ref % MAXSKIN;
      pstate->tmpx = change_armor( gs, pstate->target, pstate->tmpargument );
      pstate->tmpargument = iTmp;  // The character's old armor
      break;

    case F_GiveMoneyToTarget:
      // This function transfers money from the character to the target, and sets
      // tmpargument to the amount transferred
      iTmp = pchr->money;
      tTmp = ptarget->money;
      iTmp -= pstate->tmpargument;
      tTmp += pstate->tmpargument;
      if ( iTmp < 0 ) { tTmp += iTmp;  pstate->tmpargument += iTmp;  iTmp = 0; }
      if ( tTmp < 0 ) { iTmp += tTmp;  pstate->tmpargument += tTmp;  tTmp = 0; }
      if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }
      if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }
      pchr->money = iTmp;
      ptarget->money = tTmp;
      break;

    case F_DropKeys:
      drop_keys( gs, ichr );
      break;

    case F_IfLeaderIsAlive:
      // This function fails if there is no team leader
      returncode = ACTIVE_CHR( gs->ChrList, loc_leader );
      break;

    case F_IfTargetIsOldTarget:
      // This function returns bfalse if the target has changed
      returncode = ( pstate->target == pstate->oldtarget );
      break;

    case F_SetTargetToLeader:
      // This function fails if there is no team leader
      if ( !ACTIVE_CHR( gs->ChrList, loc_leader ) )
      {
        returncode = bfalse;
      }
      else
      {
        pstate->target = loc_leader;
      }
      break;

    case F_SpawnCharacter:
      {
        vect3 chr_pos = {pstate->tmpx, pstate->tmpy, 0};
        vect3 chr_vel = {0,0,0};

        tTmp = pchr->ori.turn_lr >> 2;
        chr_vel.x = turntocos[( tTmp+8192 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
        chr_vel.y = turntosin[( tTmp+8192 ) & TRIGTABLE_MASK] * pstate->tmpdistance;

        // This function spawns a ichr, failing if x,y is invalid
        returncode = bfalse;
        tmpchr = chr_spawn( gs, chr_pos, chr_vel, pchr->model, pchr->team, 0, pstate->tmpturn, NULL, INVALID_CHR );
        if ( VALID_CHR( gs->ChrList, tmpchr ) )
        {
          if ( 0 != chr_hitawall( gs, gs->ChrList + tmpchr, NULL ) )
          {
            gs->ChrList[tmpchr].freeme = btrue;
          }
          else
          {
            gs->ChrList[tmpchr].passage       = pchr->passage;
            gs->ChrList[tmpchr].prop.iskursed = bfalse;

            pstate->child                     = tmpchr;
            gs->ChrList[tmpchr].aistate.owner = pstate->owner;

            returncode = btrue;
          }
        }
      }
      break;

    case F_RespawnCharacter:
      // This function respawns the character at its starting location
      respawn_character( gs, ichr );
      break;

    case F_ChangeTile:
      // This function changes the floor image under the character
      mesh_set_tile( Game_getMesh(gs), MESH_FLOAT_TO_FAN( pchr->ori.pos.x ), MESH_FLOAT_TO_FAN( pchr->ori.pos.y ), pstate->tmpargument & 0xFF );
      break;

    case F_IfUsed:
      // This function proceeds only if the character has been used
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_USED );
      break;

    case F_DropMoney:
      // This function drops some of a character's money
      drop_money( gs, ichr, pstate->tmpargument );
      break;

    case F_SetOldTarget:
      // This function sets the old target to the current target
      pstate->oldtarget = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
      break;

    case F_DetachFromHolder:
      // This function drops the ichr, failing only if it was not held
      returncode = detach_character_from_mount( gs, ichr, btrue, btrue );
      break;

    case F_IfTargetHasVulnerabilityID:
      // This function proceeds if ID matches tmpargument
      returncode = ( ChrList_getPCap(gs, loc_target)->idsz[IDSZ_VULNERABILITY] == ( IDSZ ) pstate->tmpargument );
      break;

    case F_CleanUp:
      // This function issues the clean up order to all teammates
      issue_clean( gs, ichr );
      break;

    case F_IfCleanedUp:
      // This function proceeds only if the character was told to clean up
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_CLEANEDUP );
      break;

    case F_IfSitting:
      // This function proceeds if the character is riding another
      returncode = bfalse;
      {
        tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );
        if( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          returncode = (ichr == chr_get_holdingwhich(gs->ChrList, CHRLST_COUNT, tmpchr, SLOT_SADDLE));
        };
      }
      break;

    case F_IfTargetIsHurt:
      // This function passes only if the target is hurt and alive
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, pstate->target ) )
      {
        returncode = ptarget->alive &&  ptarget->stats.lifemax_fp8 > 0 && ptarget->stats.life_fp8 < ptarget->stats.lifemax_fp8 - DAMAGE_HURT;
      }
      break;

    case F_IfTargetIsAPlayer:
      // This function proceeds only if the target is a player ( may not be local )
      returncode = chr_is_player(gs, pstate->target);
      break;

    case F_PlaySound:
      // This function plays a sound
      returncode = bfalse;
      if ( INVALID_SOUND != pstate->tmpargument && pchr->ori_old.pos.z > PITNOSOUND )
      {
        returncode = ( INVALID_SOUND != snd_play_sound( gs, 1.0f, pchr->ori_old.pos, pobj->wavelist[pstate->tmpargument], 0, pchr->model, pstate->tmpargument) );
      }
      break;

    case F_SpawnParticle:
      // This function spawns a particle
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, ichr ) && !chr_in_pack( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        tmpchr = ichr;
        if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )  tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );

        tmpprt = prt_spawn( gs, 1.0f, pchr->ori.pos, pchr->ori.vel, pchr->ori.turn_lr, pchr->model, PIP_REF(pstate->tmpargument), ichr, (GRIP)pstate->tmpdistance, pchr->team, tmpchr, 0, INVALID_CHR );
        if ( RESERVED_PRT( gs->PrtList, tmpprt ) )
        {
          // Detach the particle
          attach_particle_to_character( gs, tmpprt, ichr, pstate->tmpdistance );
          gs->PrtList[tmpprt].attachedtochr = INVALID_CHR;

          // Correct X, Y, Z spacing
          gs->PrtList[tmpprt].ori.pos.x += pstate->tmpx;
          gs->PrtList[tmpprt].ori.pos.y += pstate->tmpy;
          gs->PrtList[tmpprt].ori.pos.z += gs->PipList[gs->PrtList[tmpprt].pip].zspacing.ibase;

          // Don't spawn in walls
          if ( 0 != prt_hitawall( gs, tmpprt, NULL ) )
          {
            gs->PrtList[tmpprt].ori.pos.x = pchr->ori.pos.x;
            if ( 0 != prt_hitawall( gs, tmpprt, NULL ) )
            {
              gs->PrtList[tmpprt].ori.pos.y = pchr->ori.pos.y;
            }
          }
          returncode = btrue;
        }
      }
      break;

    case F_IfTargetIsAlive:
      // This function proceeds only if the target is alive
      returncode = ptarget->alive;
      break;

    case F_Stop:
      pchr->skin.maxaccel = 0;
      break;

    case F_DisaffirmCharacter:
      disaffirm_attached_particles( gs, ichr );
      break;

    case F_ReaffirmCharacter:
      reaffirm_attached_particles( gs, ichr );
      break;

    case F_IfTargetIsSelf:
      // This function proceeds only if the target is the character too
      returncode = ( pstate->target == ichr );
      break;

    case F_IfTargetIsMale:
      // This function proceeds only if the target is male
      returncode = ( pchr->gender == GEN_MALE );
      break;

    case F_IfTargetIsFemale:
      // This function proceeds only if the target is female
      returncode = ( pchr->gender == GEN_FEMALE );
      break;

    case F_SetTargetToSelf:
      // This function sets the target to the character
      pstate->target = ichr;
      break;

    case F_SetTargetToRider:
      // This function sets the target to the character's left/only grip weapon,
      // failing if there is none
      returncode = bfalse;
      if ( chr_using_slot( gs->ChrList, CHRLST_COUNT, ichr, SLOT_SADDLE ) )
      {
        pstate->target = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, ichr, SLOT_SADDLE );
        returncode = btrue;
      }
      break;

    case F_GetAttackTurn:
      // This function sets tmpturn to the direction of the last attack
      pstate->tmpturn = pstate->directionlast;
      break;

    case F_GetDamageType:
      // This function gets the last type of damage
      pstate->tmpargument = pstate->damagetypelast;
      break;

    case F_BecomeSpell:
      // This function turns the spellbook character into a spell based on its
      // content
      pchr->money = pchr->skin_ref % MAXSKIN;
      change_character( gs, ichr, OBJ_REF(pstate->content), 0, LEAVE_NONE );
      pstate->content = 0;  // Reset so it doesn't mess up
      pstate->state   = 0;  // Reset so it doesn't mess up
      pstate->morphed = btrue;
      break;

    case F_BecomeSpellbook:
      // This function turns the spell into a spellbook, and sets the content
      // accordingly
      pstate->content = REF_TO_INT(pchr->model);
      change_character( gs, ichr, OBJ_REF(SPELLBOOK), pchr->money % MAXSKIN, LEAVE_NONE );
      pstate->state = 0;  // Reset so it doesn't burn up
      pstate->morphed = btrue;
      break;

    case F_IfScoredAHit:
      // Proceed only if the character scored a hit
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_SCOREDAHIT );
      break;

    case F_IfDisaffirmed:
      // Proceed only if the character was disaffirmed
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_DISAFFIRMED );
      break;

    case F_DecodeOrder:
      // This function gets the order and sets tmpx, tmpy, tmpargument and the
      // target ( if valid )

      returncode = bfalse;
      {
        tmpchr = CHR_REF(pchr->message.data >> 20);
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          pstate->target      = tmpchr;
          pstate->tmpx        = (( pchr->message.data >> 12 ) & 0x00FF ) << 8;
          pstate->tmpy        = (( pchr->message.data >>  4 ) & 0x00FF ) << 8;
          pstate->tmpargument = pchr->message.data & 0x000F;
          returncode = btrue;
        }
      }
      break;

    case F_SetTargetToWhoeverWasHit:
      // This function sets the target to whoever the character hit last,
      pstate->target = chr_get_aihitlast( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
      break;

    case F_SetTargetToWideEnemy:
      // This function finds an enemy, and proceeds only if there is one

      returncode = chr_search_wide( gs, SearchInfo_new(&loc_search), ichr, bfalse, bfalse, btrue, bfalse, IDSZ_NONE, bfalse );

      if ( returncode )
      {
        pstate->target = loc_search.besttarget;
      }
      break;

    case F_IfChanged:
      // Proceed only if the character was polymorphed
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_CHANGED );
      break;

    case F_IfInWater:
      // Proceed only if the character got wet
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_INWATER );
      break;

    case F_IfBored:
      // Proceed only if the character is bored
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_BORED );
      break;

    case F_IfTooMuchBaggage:
      // Proceed only if the character tried to grab too much
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_TOOMUCHBAGGAGE );
      break;

    case F_IfGrogged:
      // Proceed only if the character was grogged
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_GROGGED );
      break;

    case F_IfDazed:
      // Proceed only if the character was dazed
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_DAZED );
      break;

    case F_IfTargetHasSpecialID:
      // This function proceeds if ID matches tmpargument
      returncode = ( ChrList_getPCap(gs, loc_target)->idsz[IDSZ_SPECIAL] == ( IDSZ ) pstate->tmpargument );
      break;

    case F_PressTargetLatchButton:
      // This function sets the target's latch buttons
      ptarget->aistate.latch.b |= pstate->tmpargument;
      break;

    case F_IfInvisible:
      // This function passes if the character is invisible
      returncode = chr_is_invisible( gs->ChrList, CHRLST_COUNT, ichr );
      break;

    case F_IfArmorIs:
      // This function passes if the character's skin is tmpargument
      tTmp = pchr->skin_ref % MAXSKIN;
      returncode = ( tTmp == pstate->tmpargument );
      break;

    case F_GetTargetGrogTime:
      // This function returns tmpargument as the grog time, and passes if it is not 0
      pstate->tmpargument = pchr->grogtime;
      returncode = ( pstate->tmpargument != 0 );
      break;

    case F_GetTargetDazeTime:
      // This function returns tmpargument as the daze time, and passes if it is not 0
      pstate->tmpargument = pchr->dazetime;
      returncode = ( pstate->tmpargument != 0 );
      break;

    case F_SetDamageType:
      // This function sets the bump damage type
      pchr->damagetargettype = (DAMAGE)(pstate->tmpargument % MAXDAMAGETYPE);
      break;

    case F_SetWaterLevel:
      // This function raises and lowers the module's water
      fTmp = ( pstate->tmpargument / 10.0 ) - gs->water.douselevel;
      gs->water.surfacelevel += fTmp;
      gs->water.douselevel += fTmp;
      for ( iTmp = 0; iTmp < MAXWATERLAYER; iTmp++ )
        gs->water.layer[iTmp].z += fTmp;
      break;

    case F_EnchantTarget:
      // This function enchants the target
      {
        ENC_REF tmpenc;
        enc_spawn_info_init( &enc_si, gs, pstate->owner, pstate->target, ichr, INVALID_ENC, INVALID_OBJ );
        tmpenc = req_spawn_one_enchant( enc_si );
        returncode = ( INVALID_ENC != tmpenc );
      }
      break;

    case F_EnchantChild:
      // This function can be used with SpawnCharacter to enchant the
      // newly spawned character
      {
        ENC_REF tmpenc;
        enc_spawn_info_init( &enc_si, gs, pstate->owner, pstate->child, ichr, INVALID_ENC, INVALID_OBJ );
        tmpenc = req_spawn_one_enchant( enc_si );
        returncode = ( INVALID_ENC != tmpenc );
      }
      break;

    case F_TeleportTarget:
      // This function teleports the target to the X, Y location, failing if the
      // location is off the map or blocked. Z position is defined in tmpdistance
      returncode = bfalse;
      if ( mesh_check( &(pmesh->Info), pstate->tmpx, pstate->tmpy ) )
      {
        // Yeah!  It worked!
        tmpchr = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
        detach_character_from_mount( gs, tmpchr, btrue, bfalse );
        gs->ChrList[tmpchr].ori_old.pos = gs->ChrList[tmpchr].ori.pos;

        gs->ChrList[tmpchr].ori.pos.x = pstate->tmpx;
        gs->ChrList[tmpchr].ori.pos.y = pstate->tmpy;
        gs->ChrList[tmpchr].ori.pos.z = pstate->tmpdistance;
		gs->ChrList[tmpchr].ori.turn_lr = pstate->tmpturn;
        if ( 0 != chr_hitawall( gs, gs->ChrList + tmpchr, NULL ) )
        {
          // No it didn't...
          gs->ChrList[tmpchr].ori.pos = gs->ChrList[tmpchr].ori_old.pos;
          returncode = bfalse;
        }
        else
        {
          gs->ChrList[tmpchr].ori_old.pos = gs->ChrList[tmpchr].ori.pos;
          returncode = btrue;
        }
      }
      break;

    case F_GiveExperienceToTarget:
      // This function gives the target some experience, xptype from distance,
      // amount from argument...
      give_experience( gs, pstate->target, pstate->tmpargument, (EXPERIENCE)pstate->tmpdistance );
      break;

    case F_IncreaseAmmo:
      // This function increases the ammo by one
      if ( pchr->ammo < pchr->ammomax )
      {
        pchr->ammo++;
      }
      break;

    case F_UnkurseTarget:
      // This function unkurses the target
      ptarget->prop.iskursed = bfalse;
      break;

    case F_GiveExperienceToTargetTeam:
      // This function gives experience to everyone on the target's team
      give_team_experience( gs, ptarget->team, pstate->tmpargument, (EXPERIENCE)pstate->tmpdistance );
      break;

    case F_IfUnarmed:
      // This function proceeds if the character has no item in hand
      returncode = !chr_using_slot( gs->ChrList, CHRLST_COUNT, ichr, SLOT_LEFT ) && !chr_using_slot( gs->ChrList, CHRLST_COUNT, ichr, SLOT_RIGHT );
      break;

    case F_RestockTargetAmmoIDAll:
      // This function restocks the ammo of every item the character is holding,
      // if the item matches the ID given ( parent or child type )

      iTmp = 0;  // Amount of ammo given
      {
        tmpchr;

        for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
        {
          tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
          iTmp += restock_ammo( gs, tmpchr, pstate->tmpargument );
        }

        tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, pstate->target );
        while ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          iTmp += restock_ammo( gs, tmpchr, pstate->tmpargument );
          tmpchr = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr );
        }

        pstate->tmpargument = iTmp;
      }
      returncode = ( iTmp != 0 );
      break;

    case F_RestockTargetAmmoIDFirst:
      // This function restocks the ammo of the first item the character is holding,
      // if the item matches the ID given ( parent or child type )
      iTmp = 0;  // Amount of ammo given

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
        iTmp += restock_ammo( gs, tmpchr, pstate->tmpargument );
        if ( iTmp != 0 ) break;
      }

      if ( iTmp == 0 )
      {
        tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, pstate->target );
        while ( ACTIVE_CHR( gs->ChrList, tmpchr ) && iTmp == 0 )
        {
          iTmp += restock_ammo( gs, tmpchr, pstate->tmpargument );
          tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr );
        }
      }

      pstate->tmpargument = iTmp;
      returncode = ( iTmp != 0 );
      break;

    case F_FlashTarget:
      // This function flashes the character
      flash_character( gs, pstate->target, 255 );
      break;

    case F_SetRedShift:
      // This function alters a character's coloration
      pchr->redshift = pstate->tmpargument;
      break;

    case F_SetGreenShift:
      // This function alters a character's coloration
      pchr->grnshift = pstate->tmpargument;
      break;

    case F_SetBlueShift:
      // This function alters a character's coloration
      pchr->blushift = pstate->tmpargument;
      break;

    case F_SetLight:
      // This function alters a character's transparency
      pchr->light_fp8 = pstate->tmpargument;
      break;

    case F_SetAlpha:
      // This function alters a character's transparency
      pchr->alpha_fp8    = pstate->tmpargument;
      pchr->bumpstrength = pcap->bumpstrength * FP8_TO_FLOAT( pstate->tmpargument );
      break;

    case F_IfHitFromBehind:
      // This function proceeds if the character was attacked from behind
      returncode = bfalse;
      if ( pstate->directionlast >= BEHIND - 8192 && pstate->directionlast < BEHIND + 8192 )
        returncode = btrue;
      break;

    case F_IfHitFromFront:
      // This function proceeds if the character was attacked from the front
      returncode = bfalse;
      if ( pstate->directionlast >= 49152 + 8192 || pstate->directionlast < FRONT + 8192 )
        returncode = btrue;
      break;

    case F_IfHitFromLeft:
      // This function proceeds if the character was attacked from the left
      returncode = bfalse;
      if ( pstate->directionlast >= LEFT - 8192 && pstate->directionlast < LEFT + 8192 )
        returncode = btrue;
      break;

    case F_IfHitFromRight:
      // This function proceeds if the character was attacked from the right
      returncode = bfalse;
      if ( pstate->directionlast >= RIGHT - 8192 && pstate->directionlast < RIGHT + 8192 )
        returncode = btrue;
      break;

    case F_IfTargetIsOnSameTeam:
      // This function proceeds only if the target is on another team
      returncode = bfalse;
      if ( ptarget->team == pchr->team )
        returncode = btrue;
      break;

    case F_KillTarget:
      // This function kills the target
      kill_character( gs, pstate->target, ichr );
      break;

    case F_UndoEnchant:
      // This function undoes the last enchant
      returncode = ( INVALID_ENC != pchr->undoenchant );
      remove_enchant( gs, pchr->undoenchant );
      break;

    case F_GetWaterLevel:
      // This function gets the douse level for the water, returning it in tmpargument
      pstate->tmpargument = gs->water.douselevel * 10;
      break;

    case F_CostTargetMana:
      // This function costs the target some mana
      returncode = cost_mana( gs, pstate->target, pstate->tmpargument, ichr );
      break;

    case F_IfTargetHasAnyID:
      // This function proceeds only if one of the target's IDSZ's matches tmpargument
      returncode = bfalse;
      for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
      {
        Cap_t * trg_cap = ChrList_getPCap(gs, loc_target);
        if ( NULL != trg_cap && trg_cap->idsz[tTmp] == ( IDSZ ) pstate->tmpargument )
        {
          returncode = btrue;
          break;
        }
      }
      break;

    case F_SetBumpSize:
      // This function sets the character's bump size
      //fTmp = pchr->bmpdata.calc_size_big;
      //fTmp /= pchr->bmpdata.calc_size;  // 1.5 or 2.0
      //pchr->bmpdata.size = pstate->tmpargument * pchr->fat;
      //pchr->bmpdata.sizebig = fTmp * pchr->bmpdata.calc_size;
      //pchr->bmpdata_save.size = pstate->tmpargument;
      //pchr->bmpdata_save.sizebig = fTmp * pchr->bmpdata_save.size;
      break;

    case F_IfNotDropped:
      // This function passes if a kursed item could not be dropped
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_NOTDROPPED );
      break;

    case F_IfYIsLessThanX:
      // This function passes only if tmpy is less than tmpx
      returncode = ( pstate->tmpy < pstate->tmpx );
      break;

    case F_SetFlyHeight:
      // This function sets a character's fly height
      pchr->flyheight = pstate->tmpargument;
      break;

    case F_IfBlocked:
      // This function passes if the character blocked an attack
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_BLOCKED );
      break;

    case F_IfTargetIsDefending:
      returncode = ( ptarget->action.now >= ACTION_PA && ptarget->action.now <= ACTION_PD );
      break;

    case F_IfTargetIsAttacking:
      returncode = ( ptarget->action.now >= ACTION_UA && ptarget->action.now <= ACTION_FD );
      break;

    case F_IfStateIs0:
      returncode = ( 0 == pstate->state );
      break;

    case F_IfStateIs1:
      returncode = ( 1 == pstate->state );
      break;

    case F_IfStateIs2:
      returncode = ( 2 == pstate->state );
      break;

    case F_IfStateIs3:
      returncode = ( 3 == pstate->state );
      break;

    case F_IfStateIs4:
      returncode = ( 4 == pstate->state );
      break;

    case F_IfStateIs5:
      returncode = ( 5 == pstate->state );
      break;

    case F_IfStateIs6:
      returncode = ( 6 == pstate->state );
      break;

    case F_IfStateIs7:
      returncode = ( 7 == pstate->state );
      break;

    case F_IfContentIs:
      returncode = ( pstate->tmpargument == pstate->content );
      break;

    case F_SetTurnModeToWatchTarget:
      // This function sets the turn mode
      pstate->turnmode = TURNMODE_WATCHTARGET;
      break;

    case F_IfStateIsNot:
      returncode = ( pstate->tmpargument != pstate->state );
      break;

    case F_IfXIsEqualToY:
      returncode = ( pstate->tmpx == pstate->tmpy );
      break;

    case F_DisplayDebugMessage:
      // This function spits out a debug message
      if( CData.DevMode )
      {
        debug_message( 1, "aistate %d, aicontent %d, target %d", pstate->state, pstate->content, pstate->target );
        debug_message( 1, "tmpx %d, tmpy %d", pstate->tmpx, pstate->tmpy );
        debug_message( 1, "tmpdistance %d, tmpturn %d", pstate->tmpdistance, pstate->tmpturn );
        debug_message( 1, "tmpargument %d, selfturn %d", pstate->tmpargument, pchr->ori.turn_lr );
      }
      break;

    case F_BlackTarget:
      // This function makes the target flash black
      flash_character( gs, pstate->target, 0 );
      break;

    case F_DisplayMessageNear:
      // This function sends a message if the camera is in the nearby area.
      iTmp = ABS( pchr->ori_old.pos.x - GCamera.trackpos.x ) + ABS( pchr->ori_old.pos.y - GCamera.trackpos.y );
      if ( iTmp < MSGDISTANCE )
      {
        display_message( gs, pobj->msg_start + pstate->tmpargument, ichr );
      }
      break;

    case F_IfHitGround:
      // This function passes if the character just hit the ground
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_HITGROUND );
      break;

    case F_IfNameIsKnown:
      // This function passes if the character's name is known
      returncode = pchr->prop.nameknown;
      break;

    case F_IfUsageIsKnown:
      // This function passes if the character's usage is known
      returncode = pcap->prop.usageknown;
      break;

    case F_IfHoldingItemID:
      // This function passes if the character is holding an item with the IDSZ given
      // in tmpargument, returning the latch to press to use it
      returncode = bfalse;

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, ichr, _slot );
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          if ( CAP_INHERIT_IDSZ( gs, ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
          {
            pstate->tmpargument = slot_to_latch( gs->ChrList, CHRLST_COUNT, ichr, _slot );
            returncode = btrue;
          }
        }
      }
      break;

    case F_IfHoldingRangedWeapon:
      // This function passes if the character is holding a ranged weapon, returning
      // the latch to press to use it.  This also checks ammo/ammoknown.
      returncode = bfalse;
      pstate->tmpargument = 0;

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, ichr, _slot );
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          if ( gs->ChrList[tmpchr].prop.isranged && ( gs->ChrList[tmpchr].ammomax == 0 || ( gs->ChrList[tmpchr].ammo > 0 && gs->ChrList[tmpchr].ammoknown ) ) )
          {
            pstate->tmpargument = slot_to_latch( gs->ChrList, CHRLST_COUNT, ichr, _slot );
            returncode = btrue;
          }
        }
      };
      break;

    case F_IfHoldingMeleeWeapon:
      // This function passes if the character is holding a melee weapon, returning
      // the latch to press to use it
      returncode = bfalse;
      pstate->tmpargument = 0;

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, ichr, _slot );
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          if ( !gs->ChrList[tmpchr].prop.isranged && ChrList_getPCap(gs, tmpchr)->weaponaction != ACTION_PA )
          {
            pstate->tmpargument = slot_to_latch( gs->ChrList, CHRLST_COUNT, ichr, _slot );
            returncode = btrue;
          }
        }
      };
      break;

    case F_IfHoldingShield:
      // This function passes if the character is holding a shield, returning the
      // latch to press to use it
      returncode = bfalse;
      pstate->tmpargument = 0;

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, ichr, _slot );
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) && ChrList_getPCap(gs, tmpchr)->weaponaction == ACTION_PA )
        {
          pstate->tmpargument = slot_to_latch( gs->ChrList, CHRLST_COUNT, ichr, _slot );
          returncode = btrue;
        }
      };
      break;

    case F_IfKursed:
      // This function passes if the character is kursed
      returncode = pchr->prop.iskursed;
      break;

    case F_IfTargetIsKursed:
      // This function passes if the target is kursed
      returncode = ptarget->prop.iskursed;
      break;

    case F_IfTargetIsDressedUp:
      // This function passes if the character's skin is dressy
      iTmp = pchr->skin_ref % MAXSKIN;
      iTmp = 1 << iTmp;
      returncode = (( pcap->skindressy & iTmp ) != 0 );
      break;

    case F_IfOverWater:
      // This function passes if the character is on a water tile
      returncode = mesh_has_some_bits( pmesh->Mem.tilelst, pchr->onwhichfan, MPDFX_WATER ) && gs->water.iswater;
      break;

    case F_IfThrown:
      // This function passes if the character was thrown
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_THROWN );
      break;

    case F_MakeNameKnown:
      // This function makes the name of an item/character known.
      pchr->prop.nameknown = btrue;
      pchr->prop.icon = btrue;
      break;

    case F_MakeUsageKnown:
      // This function makes the usage of an item known...  For XP gains from
      // using an unknown potion or such
      pcap->prop.usageknown = btrue;
      break;

    case F_StopTargetMovement:
      // This function makes the target stop moving temporarily
      ptarget->ori.vel.x = 0;
      ptarget->ori.vel.y = 0;
      if ( ptarget->ori.vel.z > 0 ) ptarget->ori.vel.z = gs->phys.gravity;
      break;

    case F_SetXY:
      // This function stores tmpx and tmpy in the storage array
      pstate->x[pstate->tmpargument&STOR_AND] = pstate->tmpx;
      pstate->y[pstate->tmpargument&STOR_AND] = pstate->tmpy;
      break;

    case F_GetXY:
      // This function gets previously stored data, setting tmpx and tmpy
      pstate->tmpx = pstate->x[pstate->tmpargument&STOR_AND];
      pstate->tmpy = pstate->y[pstate->tmpargument&STOR_AND];
      break;

    case F_AddXY:
      // This function adds tmpx and tmpy to the storage array
      pstate->x[pstate->tmpargument&STOR_AND] += pstate->tmpx;
      pstate->y[pstate->tmpargument&STOR_AND] += pstate->tmpy;
      break;

    case F_MakeAmmoKnown:
      // This function makes the ammo of an item/character known.
      pchr->ammoknown = btrue;
      break;

    case F_SpawnAttachedParticle:
      // This function spawns an attached particle
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, ichr ) && !chr_in_pack( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        tmpchr = ichr;
        if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )  tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );
        tmpprt = prt_spawn( gs, 1.0f, pchr->ori.pos, pchr->ori.vel, pchr->ori.turn_lr, pchr->model, PIP_REF(pstate->tmpargument), ichr, (GRIP)pstate->tmpdistance, pchr->team, tmpchr, 0, INVALID_CHR );
        returncode = RESERVED_PRT( gs->PrtList, tmpprt );
      }
      break;

    case F_SpawnExactParticle:
      // This function spawns an exactly placed particle
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, ichr ) && !chr_in_pack( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        vect3 prt_pos = {pstate->tmpx, pstate->tmpy, pstate->tmpdistance};
        vect3 prt_vel = {0,0,0};

        tmpchr = ichr;
        if ( chr_attached( gs->ChrList, CHRLST_COUNT, tmpchr ) )  tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, tmpchr );

        tmpprt = prt_spawn( gs, 1.0f, prt_pos, prt_vel, pchr->ori.turn_lr, pchr->model, PIP_REF(pstate->tmpargument), INVALID_CHR, (GRIP)0, pchr->team, tmpchr, 0, INVALID_CHR );
        returncode = RESERVED_PRT( gs->PrtList, tmpprt );
      };
      break;

    case F_AccelerateTarget:
      // This function changes the target's speeds
      ptarget->ori.vel.x += pstate->tmpx;
      ptarget->ori.vel.y += pstate->tmpy;
      break;

    case F_IfDistanceIsMoreThanTurn:
      // This function proceeds tmpdistance is greater than tmpturn
      returncode = ( pstate->tmpdistance > ( int ) pstate->tmpturn );
      break;

    case F_IfCrushed:
      // This function proceeds only if the character was crushed
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_CRUSHED );
      break;

    case F_MakeCrushValid:
      // This function makes doors able to close on this object
      pchr->prop.canbecrushed = btrue;
      break;

    case F_SetTargetToLowestTarget:
      // This sets the target to whatever the target is being held by,
      // The lowest in the set.  This function never fails
      if ( ACTIVE_CHR( gs->ChrList, pstate->target ) )
      {
        CHR_REF holder   = chr_get_attachedto(gs->ChrList, CHRLST_COUNT, pstate->target);
        CHR_REF attached = chr_get_attachedto(gs->ChrList, CHRLST_COUNT, holder);
        while ( ACTIVE_CHR(gs->ChrList, attached) )
        {
          holder = attached;
          attached = chr_get_attachedto(gs->ChrList, CHRLST_COUNT, holder);
        }
        pstate->target = holder;
      };
      break;

    case F_IfNotPutAway:
      // This function proceeds only if the character couln't be put in the pack
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_NOTPUTAWAY );
      break;

    case F_IfTakenOut:
      // This function proceeds only if the character was taken out of the pack
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_TAKENOUT );
      break;

    case F_IfAmmoOut:
      // This function proceeds only if the character has no ammo
      returncode = ( pchr->ammo == 0 );
      break;

    case F_PlaySoundLooped:
      // This function plays a looped sound
      returncode = bfalse;
      if ( !gs->modstate.Paused && (0 <= pstate->tmpargument) && (pchr->ori_old.pos.z > PITNOSOUND) && (INVALID_SOUND == pchr->loopingchannel))
      {
        //You could use this, but right now there's no way to stop the sound later, so it's better not to start it
        pchr->loopingchannel = snd_play_sound( gs, 1.0f, pchr->ori.pos, pobj->wavelist[pstate->tmpargument], 0, pchr->model, pstate->tmpargument);
        pchr->loopingvolume = 1.0f;
      }
      break;

    case F_StopSoundLoop:
      // This function stops playing a sound
      if( INVALID_CHANNEL != pchr->loopingchannel )
      {
        snd_stop_sound( pchr->loopingchannel );
        pchr->loopingchannel = INVALID_CHANNEL;
      };
      break;

    case F_HealSelf:
      // This function heals the ichr, without setting the alert or modifying
      // the amount
      if ( pchr->alive )
      {
        iTmp = pchr->stats.life_fp8 + pstate->tmpargument;
        if ( iTmp > pchr->stats.lifemax_fp8 ) iTmp = pchr->stats.lifemax_fp8;
        if ( iTmp < 1 ) iTmp = 1;
        pchr->stats.life_fp8 = iTmp;
      }
      break;

    case F_Equip:
      // This function flags the character as being equipped
      pchr->isequipped = btrue;
      break;

    case F_IfTargetHasItemIDEquipped:
      // This function proceeds if the target has a matching item equipped
      returncode = bfalse;
      {
        tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, pstate->target );
        while ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          if ( tmpchr != ichr && gs->ChrList[tmpchr].isequipped && CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
          {
            returncode = btrue;
            break;
          }
          else
          {
            tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr );
          }
        }
      }
      break;

    case F_SetOwnerToTarget:
      // This function sets the owner
      pstate->owner = pstate->target;
      break;

    case F_SetTargetToOwner:
      // This function sets the target to the owner
      pstate->target = pstate->owner;
      break;

    case F_SetFrame:
      // This function sets the character's current frame
      sTmp = pstate->tmpargument & 3;
      iTmp = pstate->tmpargument >> 2;
      set_frame( gs, ichr, iTmp, sTmp );
      break;

    case F_BreakPassage:
      // This function makes the tiles fall away ( turns into damage terrain )
      returncode = scr_break_passage( gs, pstate );
      break;

    case F_SetReloadTime:
      // This function makes weapons fire slower
      pchr->reloadtime = pstate->tmpargument;
      break;

    case F_SetTargetToWideBlahID:
      // This function sets the target based on the settings of tmpargument and tmpdistance

      returncode = chr_search_wide( gs, SearchInfo_new(&loc_search), ichr,
                                    HAS_SOME_BITS( pstate->tmpdistance, SEARCH_ITEMS   ),
                                    HAS_SOME_BITS( pstate->tmpdistance, SEARCH_FRIENDS ),
                                    HAS_SOME_BITS( pstate->tmpdistance, SEARCH_ENEMIES ),
                                    HAS_SOME_BITS( pstate->tmpdistance, SEARCH_DEAD    ),
                                    pstate->tmpargument,
                                    HAS_SOME_BITS( pstate->tmpdistance, SEARCH_INVERT  ) );

      if ( returncode )
      {
        pstate->target = loc_search.besttarget;
      }
      break;

    case F_PoofTarget:
      // This function makes the target go away
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, pstate->target ) && !chr_is_player(gs, pstate->target) )
      {
        // tell target to go away
        ptarget->gopoof = btrue;
        pstate->target  = ichr;
        returncode      = btrue;
      }
      break;

    case F_ChildDoActionOverride:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid
      returncode = bfalse;
      if ( pstate->tmpargument < MAXACTION )
      {
        if ( ChrList_getPMad(gs, loc_aichild)->actionvalid[pstate->tmpargument] )
        {
          pchild->action.now = (ACTION)pstate->tmpargument;
          pchild->action.ready = bfalse;

          pchild->anim.ilip = 0;
          pchild->anim.flip    = 0.0f;
          pchild->anim.next    = ChrList_getPMad(gs, loc_aichild)->actionstart[pstate->tmpargument];
          pchild->anim.last    = pchild->anim.next;
          returncode = btrue;
        }
      }
      break;

    case F_SpawnPoof:
      // This function makes a lovely little poof at the character's location
      spawn_poof( gs, ichr, pchr->model );
      break;

    case F_SetSpeedPercent:
      chr_reset_accel( gs, ichr );
      pchr->skin.maxaccel *= pstate->tmpargument / 100.0;
      break;

    case F_SetChildState:
      // This function sets the child's state
      pchild->aistate.state = pstate->tmpargument;
      break;

    case F_SpawnAttachedSizedParticle:
      // This function spawns an attached particle, then sets its size
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, ichr ) && !chr_in_pack( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        tmpchr = ichr;
        if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )  tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );

        tmpprt = prt_spawn( gs, 1.0f, pchr->ori.pos, pchr->ori.vel, pchr->ori.turn_lr, pchr->model, PIP_REF(pstate->tmpargument), ichr, (GRIP)pstate->tmpdistance, pchr->team, tmpchr, 0, INVALID_CHR );
        if ( RESERVED_PRT( gs->PrtList, tmpprt ) )
        {
          gs->PrtList[tmpprt].size_fp8 = pstate->tmpturn;
          returncode = btrue;
        }
      }
      break;

    case F_ChangeArmor:
      // This function sets the character's armor type and returns the old type
      // as tmpargument and the new type as tmpx
      pstate->tmpx = pstate->tmpargument;
      iTmp = pchr->skin_ref % MAXSKIN;
      pstate->tmpx = change_armor( gs, ichr, pstate->tmpargument );
      pstate->tmpargument = iTmp;  // The character's old armor
      break;

    case F_ShowTimer:
      // This function turns the timer on, using the value for tmpargument
      timeron = btrue;
      timervalue = pstate->tmpargument;
      break;

    case F_IfFacingTarget:
      {
        // This function proceeds only if the character is facing the target
        // try to avoid using inverse trig functions
        float dx, dy, d2, ftmpx, ftmpy, ftmp;

        returncode = bfalse;
        dx = ptarget->ori.pos.x - pchr->ori.pos.x;
        dy = ptarget->ori.pos.y - pchr->ori.pos.y;
        d2 = dx * dx + dy * dy;

        if ( d2 > 0.0f )
        {
          // use non-inverse function to get direction vec from gs->ChrList[].turn
          turn_to_vec( pchr->ori.turn_lr, &ftmpx, &ftmpy );

          // calc the dotproduct
          ftmp = ( dx * ftmpx + dy * ftmpy );

          // negative dotprod means facing the wrong direction
          if ( ftmp > 0.0f )
          {
            // normalized dot product, squared
            ftmp *= ftmp / d2;

            // +/- 45 degrees means ftmp > 0.5f
            returncode = ftmp > 0.5f;
          }
        }

      }
      break;

    case F_PlaySoundVolume:
      // This function sets the volume of a sound and plays it
      returncode = bfalse;
      if ( INVALID_SOUND != pstate->tmpargument && pstate->tmpdistance >= 0 )
      {
        volume = pstate->tmpdistance;
        returncode = ( INVALID_SOUND != snd_play_sound( gs, MIN( 1.0f, volume / 255.0f ), pchr->ori_old.pos, pobj->wavelist[pstate->tmpargument], 0, pchr->model, pstate->tmpargument ) );
      }
      break;

    case F_SpawnAttachedFacedParticle:
      // This function spawns an attached particle with facing
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, ichr ) && !chr_in_pack( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        tmpchr = ichr;
        if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )  tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );

        tmpprt = prt_spawn( gs, 1.0f, pchr->ori.pos, pchr->ori.vel, pstate->tmpturn, pchr->model, PIP_REF(pstate->tmpargument), ichr, (GRIP)pstate->tmpdistance, pchr->team, tmpchr, 0, INVALID_CHR );
        returncode = RESERVED_PRT( gs->PrtList, tmpprt );
      }
      break;

    case F_IfStateIsOdd:
      returncode = HAS_SOME_BITS( pstate->state, 1 );
      break;

    case F_SetTargetToDistantEnemy:
      // This function finds an enemy, within a certain distance to the ichr, and
      // proceeds only if there is one

      returncode = chr_search_distant( gs, SearchInfo_new(&loc_search), ichr, pstate->tmpdistance, btrue, bfalse );

      if ( returncode )
      {
        pstate->target = loc_search.besttarget;
      }
      break;

    case F_Teleport:
      // This function teleports the character to the X, Y location, failing if the
      // location is off the map or blocked
      returncode = bfalse;
      if ( mesh_check( &(pmesh->Info), pstate->tmpx, pstate->tmpy ) )
      {
        // Yeah!  It worked!
        detach_character_from_mount( gs, ichr, btrue, bfalse );
        pchr->ori_old.pos = pchr->ori.pos;

        pchr->ori.pos.x = pstate->tmpx;
        pchr->ori.pos.y = pstate->tmpy;
        if ( 0 != chr_hitawall( gs, gs->ChrList + ichr, NULL ) )
        {
          // No it didn't...
          pchr->ori.pos = pchr->ori_old.pos;
          returncode = bfalse;
        }
        else
        {
          pchr->ori_old.pos = pchr->ori.pos;
          returncode = btrue;
        }
      }
      break;

    case F_GiveStrengthToTarget:
      // Permanently boost the target's strength
      if ( ptarget->alive )
      {
        iTmp = pstate->tmpargument;
        getadd( 0, ptarget->stats.strength_fp8, PERFECTSTAT, &iTmp );
        ptarget->stats.strength_fp8 += iTmp;
      }
      break;

    case F_GiveWisdomToTarget:
      // Permanently boost the target's wisdom
      if ( ptarget->alive )
      {
        iTmp = pstate->tmpargument;
        getadd( 0, ptarget->stats.wisdom_fp8, PERFECTSTAT, &iTmp );
        ptarget->stats.wisdom_fp8 += iTmp;
      }
      break;

    case F_GiveIntelligenceToTarget:
      // Permanently boost the target's intelligence
      if ( ptarget->alive )
      {
        iTmp = pstate->tmpargument;
        getadd( 0, ptarget->stats.intelligence_fp8, PERFECTSTAT, &iTmp );
        ptarget->stats.intelligence_fp8 += iTmp;
      }
      break;

    case F_GiveDexterityToTarget:
      // Permanently boost the target's dexterity
      if ( ptarget->alive )
      {
        iTmp = pstate->tmpargument;
        getadd( 0, ptarget->stats.dexterity_fp8, PERFECTSTAT, &iTmp );
        ptarget->stats.dexterity_fp8 += iTmp;
      }
      break;

    case F_GiveLifeToTarget:
      // Permanently boost the target's life
      if ( ptarget->alive )
      {
        iTmp = pstate->tmpargument;
        getadd( LOWSTAT, ptarget->stats.lifemax_fp8, PERFECTBIG, &iTmp );
        ptarget->stats.lifemax_fp8 += iTmp;
        if ( iTmp < 0 )
        {
          getadd( 1, ptarget->stats.life_fp8, PERFECTBIG, &iTmp );
        }
        ptarget->stats.life_fp8 += iTmp;
      }
      break;

    case F_GiveManaToTarget:
      // Permanently boost the target's mana
      if ( ptarget->alive )
      {
        iTmp = pstate->tmpargument;
        getadd( 0, ptarget->stats.manamax_fp8, PERFECTBIG, &iTmp );
        ptarget->stats.manamax_fp8 += iTmp;
        if ( iTmp < 0 )
        {
          getadd( 0, ptarget->stats.mana_fp8, PERFECTBIG, &iTmp );
        }
        ptarget->stats.mana_fp8 += iTmp;
      }
      break;

    case F_ShowMap:
      // Show the map...  Fails if map already visible
      if ( mapon )  returncode = bfalse;
      mapon = btrue;
      break;

    case F_ShowYouAreHere:
      // Show the camera target location
      youarehereon = btrue;
      break;

    case F_ShowBlipXY:
      // Add a blip
      if ( numblip < MAXBLIP )
      {
        if ( mesh_check( &(pmesh->Info), pstate->tmpx, pstate->tmpy ) )
        {
          if ( pstate->tmpargument < NUMBAR && pstate->tmpargument >= 0 )
          {
            BlipList[numblip].x = mesh_fraction_x( &(pmesh->Info), pstate->tmpx ) * MAPSIZE * mapscale;
            BlipList[numblip].y = mesh_fraction_y( &(pmesh->Info), pstate->tmpy ) * MAPSIZE * mapscale ;
            BlipList[numblip].c = (COLR)pstate->tmpargument;
            numblip++;
          }
        }
      }
      break;

    case F_HealTarget:
      // Give some life to the target
      if ( ptarget->alive )
      {
        ENC_REF tmpenc;
        iTmp = pstate->tmpargument;
        getadd( 1, ptarget->stats.life_fp8, ptarget->stats.lifemax_fp8, &iTmp );
        ptarget->stats.life_fp8 += iTmp;

        // Check all enchants to see if they are removed
        tmpenc = ptarget->firstenchant;
        while ( INVALID_ENC != tmpenc )
        {
          ENC_REF nextenc = gs->EncList[tmpenc].nextenchant;
          if ( MAKE_IDSZ( "HEAL" ) == gs->EveList[gs->EncList[tmpenc].eve].removedbyidsz )
          {
            remove_enchant( gs, tmpenc );
          }
          tmpenc = nextenc;
        }
      }
      break;

    case F_PumpTarget:
      // Give some mana to the target
      if ( ptarget->alive )
      {
        iTmp = pstate->tmpargument;
        getadd( 0, ptarget->stats.mana_fp8, ptarget->stats.manamax_fp8, &iTmp );
        ptarget->stats.mana_fp8 += iTmp;
      }
      break;

    case F_CostAmmo:
      // Take away one ammo
      if ( pchr->ammo > 0 )
      {
        pchr->ammo--;
      }
      break;

    case F_MakeSimilarNamesKnown:
      // Make names of matching objects known
      {
        tmpchr;
        for ( tmpchr = 0; tmpchr < CHRLST_COUNT; tmpchr++ )
        {
          sTmp = btrue;
          for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
          {
            if ( pcap->idsz[tTmp] != ChrList_getPCap(gs, tmpchr)->idsz[tTmp] )
            {
              sTmp = bfalse;
              break;
            }
          }

          if ( sTmp )
          {
            gs->ChrList[tmpchr].prop.nameknown = btrue;
          }
        }
      }
      break;

    case F_SpawnAttachedHolderParticle:
      // This function spawns an attached particle, attached to the holder
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, ichr ) && !chr_in_pack( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        tmpchr = ichr;
        if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )  tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );

        tmpprt = prt_spawn( gs, 1.0f, pchr->ori.pos, pchr->ori.vel, pchr->ori.turn_lr, pchr->model, PIP_REF(pstate->tmpargument), tmpchr, (GRIP)pstate->tmpdistance, pchr->team, tmpchr, 0, INVALID_CHR );
        returncode = RESERVED_PRT( gs->PrtList, tmpprt );
      };
      break;

    case F_SetTargetReloadTime:
      // This function sets the target's reload time
      ptarget->reloadtime = pstate->tmpargument;
      break;

    case F_SetFogLevel:
      // This function raises and lowers the module's fog
      fTmp = ( pstate->tmpargument / 10.0 ) - GFog.top;
      GFog.top += fTmp;
      GFog.distance += fTmp;
      GFog.on = CData.fogallowed;
      if ( GFog.distance < 1.0 )  GFog.on = bfalse;
      break;

    case F_GetFogLevel:
      // This function gets the fog level
      pstate->tmpargument = GFog.top * 10;
      break;

    case F_SetFogTAD:
      // This function changes the fog color
      GFog.red = pstate->tmpturn;
      GFog.grn = pstate->tmpargument;
      GFog.blu = pstate->tmpdistance;
      break;

    case F_SetFogBottomLevel:
      // This function sets the module's bottom fog level...
      fTmp = ( pstate->tmpargument / 10.0 ) - GFog.bottom;
      GFog.bottom += fTmp;
      GFog.distance -= fTmp;
      GFog.on = CData.fogallowed;
      if ( GFog.distance < 1.0 )  GFog.on = bfalse;
      break;

    case F_GetFogBottomLevel:
      // This function gets the fog level
      pstate->tmpargument = GFog.bottom * 10;
      break;

    case F_CorrectActionForHand:
      // This function turns ZA into ZA, ZB, ZC, or ZD...
      // tmpargument must be set to one of the A actions beforehand...
      if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        if ( pchr->inwhichslot == SLOT_RIGHT )
        {
          // C or D
          pstate->tmpargument += 2;
        };
        pstate->tmpargument += IRAND(&loc_rand, 1);
      }
      break;

    case F_IfTargetIsMounted:
      // This function proceeds if the target is riding a mount
      returncode = bfalse;
      {
        tmpchr = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          tmpchr = chr_get_attachedto(gs->ChrList, CHRLST_COUNT, tmpchr);
          if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
          {
            returncode = gs->ChrList[tmpchr].bmpdata.calc_is_mount;
          }
        }
      }
      break;

    case F_SparkleIcon:
      // This function makes a blippie thing go around the icon
      if ( pstate->tmpargument < NUMBAR && pstate->tmpargument > -1 )
      {
        pchr->sparkle = pstate->tmpargument;
      }
      break;

    case F_UnsparkleIcon:
      // This function stops the blippie thing
      pchr->sparkle = NOSPARKLE;
      break;

    case F_GetTileXY:
      // This function gets the tile at x,y
      pstate->tmpargument = mesh_get_tile( pmesh, MESH_FLOAT_TO_FAN( pstate->tmpx ), MESH_FLOAT_TO_FAN( pstate->tmpy ) ) & 0xFF;
      break;

    case F_SetTileXY:
      // This function changes the tile at x,y
      mesh_set_tile( pmesh, MESH_FLOAT_TO_FAN( pstate->tmpx ), MESH_FLOAT_TO_FAN( pstate->tmpy ), pstate->tmpargument & 255 );
      break;

    case F_SetShadowSize:
      // This function changes a character's shadow size
      //pchr->bmpdata.shadow = pstate->tmpargument * pchr->fat;
      //pchr->bmpdata_save.shadow = pstate->tmpargument;
      break;

    case F_SignalTarget:
      // This function orders one specific character...  The target
      // Be careful in using this, always checking IDSZ first
      ptarget->message.type = pstate->tmpargument;
      ptarget->message.data = 0;
      ptarget->aistate.alert |= ALERT_SIGNALED;
      break;

    case F_SetTargetToWhoeverIsInPassage:
      // This function lets passage rectangles be used as event triggers
      {
        tmpchr = who_is_blocking_passage( gs, pstate->tmpargument );
        returncode = bfalse;
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          pstate->target = tmpchr;
          returncode = btrue;
        }
      }
      break;

    case F_IfCharacterWasABook:
      // This function proceeds if the base model is the same as the current
      // model or if the base model is SPELLBOOK
      returncode = ( pchr->model_base == SPELLBOOK ||
                     pchr->model_base == pchr->model );
      break;

    case F_SetEnchantBoostValues:
      // This function sets the boost values for the last enchantment
      {
        ENC_REF tmpenc = pchr->undoenchant;
        if ( INVALID_ENC != tmpenc )
        {
          gs->EncList[tmpenc].ownermana_fp8  = pstate->tmpargument;
          gs->EncList[tmpenc].ownerlife_fp8  = pstate->tmpdistance;
          gs->EncList[tmpenc].targetmana_fp8 = pstate->tmpx;
          gs->EncList[tmpenc].targetlife_fp8 = pstate->tmpy;
        }
      }
      break;

    case F_SpawnCharacterXYZ:
      {
        vect3 chr_pos = {pstate->tmpx, pstate->tmpy, pstate->tmpdistance};
        vect3 chr_vel = {0,0,0};

        // This function spawns a ichr, failing if x,y,z is invalid
        returncode = bfalse;
        tmpchr = chr_spawn( gs, chr_pos, chr_vel, pchr->model, pchr->team, 0, pstate->tmpturn, NULL, INVALID_CHR );
        if ( VALID_CHR( gs->ChrList, tmpchr ) )
        {
          if ( 0 != chr_hitawall( gs, gs->ChrList + tmpchr, NULL ) )
          {
            gs->ChrList[tmpchr].freeme = btrue;
          }
          else
          {
            gs->ChrList[tmpchr].prop.iskursed = bfalse;
            gs->ChrList[tmpchr].passage       = pchr->passage;

            pstate->child                     = tmpchr;
            gs->ChrList[tmpchr].aistate.owner = pstate->owner;

            returncode = btrue;
          }
        }
      }
      break;

    case F_SpawnExactCharacterXYZ:
      // This function spawns a character ( specific model slot ),
      // failing if x,y,z is invalid
      {
        vect3 chr_pos = {pstate->tmpx, pstate->tmpy, pstate->tmpdistance};
        vect3 chr_vel = {0,0,0};

        returncode = bfalse;
        tmpchr = chr_spawn( gs, chr_pos, chr_vel, OBJ_REF(pstate->tmpargument), pchr->team, 0, pstate->tmpturn, NULL, INVALID_CHR );
        if ( VALID_CHR( gs->ChrList, tmpchr ) )
        {
          if ( 0 != chr_hitawall( gs, gs->ChrList + tmpchr, NULL ) )
          {
            gs->ChrList[tmpchr].freeme = btrue;
          }
          else
          {
            gs->ChrList[tmpchr].prop.iskursed = bfalse;
            gs->ChrList[tmpchr].passage       = pchr->passage;

            pstate->child                     = tmpchr;
            gs->ChrList[tmpchr].aistate.owner = pstate->owner;

            returncode = btrue;
          }
        }
      }
      break;

    case F_ChangeTargetClass:
      // This function changes a character's model ( specific model slot )
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, pstate->target ) )
      {
        change_character( gs, pstate->target, OBJ_REF(pstate->tmpargument), 0, LEAVE_ALL );
        ptarget->aistate.morphed = btrue;
      };
      break;

    case F_PlayFullSound:
      // This function plays a sound loud for everyone...  Victory music
      returncode = bfalse;
      if (pstate->tmpargument>=0 && pstate->tmpargument!=MAXWAVE )
      {
        returncode = ( INVALID_SOUND != snd_play_sound( gs, 1.0f, GCamera.trackpos, pobj->wavelist[pstate->tmpargument], 0, pchr->model, pstate->tmpargument) );
      }
      break;

    case F_SpawnExactChaseParticle:
      // This function spawns an exactly placed particle that chases the target
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, ichr ) && !chr_in_pack( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        vect3 prt_pos = {pstate->tmpx, pstate->tmpy, pstate->tmpdistance};
        vect3 prt_vel = {0,0,0};

        tmpchr = ichr;
        if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )  tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );

        tmpprt = prt_spawn( gs, 1.0f, prt_pos, prt_vel, pchr->ori.turn_lr, pchr->model, PIP_REF(pstate->tmpargument), INVALID_CHR, (GRIP)0, pchr->team, tmpchr, 0, INVALID_CHR );
        if ( RESERVED_PRT( gs->PrtList, tmpprt ) )
        {
          gs->PrtList[tmpprt].target = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
          returncode = btrue;
        }
      }
      break;

    case F_EncodeOrder:
      // This function packs up an order, using tmpx, tmpy, tmpargument and the
      // target ( if valid ) to create a new tmpargument
      sTmp  = ( REF_TO_INT(pstate->target)   & 0x0FFF ) << 20;
      sTmp |= (( pstate->tmpx >> 8 ) & 0x00FF ) << 12;
      sTmp |= (( pstate->tmpy >> 8 ) & 0x00FF ) << 4;
      sTmp |=  ( pstate->tmpargument & 0x000F );
      pstate->tmpargument = sTmp;
      break;

    case F_SignalSpecialID:
      // This function issues an order to all with the given special IDSZ
      signal_idsz_index( gs, pchr->team_rank, pstate->tmpargument, pstate->tmpdistance, IDSZ_SPECIAL );
      break;

    case F_UnkurseTargetInventory:
      // This function unkurses every item a character is holding
      {
        tmpchr;

        for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
        {
          tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
          gs->ChrList[tmpchr].prop.iskursed = bfalse;
        };

        tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, pstate->target );
        while ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          gs->ChrList[tmpchr].prop.iskursed = bfalse;
          tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr );
        }
      }
      break;

    case F_IfTargetIsSneaking:
      // This function proceeds if the target is doing ACTION_DA or ACTION_WA
      returncode = ( ptarget->action.now == ACTION_DA || ptarget->action.now == ACTION_WA );
      break;

    case F_DropItems:
      // This function drops all of the character's items
      drop_all_items( gs, ichr );
      break;

    case F_RespawnTarget:
      // This function respawns the target at its current location
      {
        tmpchr = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
        gs->ChrList[tmpchr].ori_old.pos = gs->ChrList[tmpchr].ori.pos;
        respawn_character( gs, tmpchr );
        gs->ChrList[tmpchr].ori.pos = gs->ChrList[tmpchr].ori_old.pos;
      }
      break;

    case F_TargetDoActionSetFrame:
      // This function starts a new action, if it is valid for the model and
      // sets the starting frame.  It will fail if the action is invalid
      returncode = bfalse;
      if ( pstate->tmpargument < MAXACTION )
      {
        if ( ChrList_getPMad(gs, loc_target)->actionvalid[pstate->tmpargument] )
        {
          ptarget->action.now = (ACTION)pstate->tmpargument;
          ptarget->action.ready = bfalse;

          ptarget->anim.ilip = 0;
          ptarget->anim.flip    = 0.0f;
          ptarget->anim.next    = ChrList_getPMad(gs, loc_target)->actionstart[pstate->tmpargument];
          ptarget->anim.last    = ptarget->anim.next;

          returncode = btrue;
        }
      }
      break;

    case F_IfTargetCanSeeInvisible:
      // This function proceeds if the target can see invisible
      returncode = ptarget->prop.canseeinvisible;
      break;

    case F_SetTargetToNearestBlahID:
      // This function finds the nearest target that meets the
      // requirements

      returncode = chr_search_wide_nearest( gs, SearchInfo_new(&loc_search), ichr,
                                            HAS_SOME_BITS( pstate->tmpdistance, SEARCH_ITEMS   ),
                                            HAS_SOME_BITS( pstate->tmpdistance, SEARCH_FRIENDS ),
                                            HAS_SOME_BITS( pstate->tmpdistance, SEARCH_ENEMIES ),
                                            HAS_SOME_BITS( pstate->tmpdistance, SEARCH_DEAD    ),
                                            pstate->tmpargument );
      if ( returncode )
      {
        pstate->target = loc_search.nearest;
      }
      break;

    case F_SetTargetToNearestEnemy:
      // This function finds the nearest target that meets the
      // requirements

      returncode = chr_search_wide_nearest( gs, SearchInfo_new(&loc_search), ichr, bfalse, bfalse, btrue, bfalse, IDSZ_NONE );

      if ( returncode )
      {
        pstate->target = loc_search.nearest;
      }
      break;

    case F_SetTargetToNearestFriend:
      // This function finds the nearest target that meets the
      // requirements

      returncode = chr_search_wide_nearest( gs, SearchInfo_new(&loc_search), ichr, bfalse, btrue, bfalse, bfalse, IDSZ_NONE );

      if ( returncode )
      {
        pstate->target = loc_search.nearest;
      }
      break;

    case F_SetTargetToNearestLifeform:
      // This function finds the nearest target that meets the
      // requirements

      returncode = chr_search_wide_nearest( gs, SearchInfo_new(&loc_search), ichr, bfalse, btrue, btrue, bfalse, IDSZ_NONE );

      if ( returncode )
      {
        pstate->target = loc_search.nearest;
      }
      break;

    case F_FlashPassage:
      // This function makes the passage light or dark...  For debug...
      flash_passage( gs, pstate->tmpargument, pstate->tmpdistance );
      break;

    case F_FindTileInPassage:
      // This function finds the next tile in the passage, tmpx and tmpy are
      // required and set on return
      returncode = scr_search_tile_in_passage( gs, pstate );

      break;

    case F_IfHeldInLeftSaddle:
      // This function proceeds if the character is in the left hand of another
      // character
      returncode = bfalse;
      {
        tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          returncode = ( gs->ChrList[tmpchr].holdingwhich[SLOT_SADDLE] == ichr );
        }
      }
      break;

    case F_NotAnItem:
      // This function makes the character a non-item character
      pchr->prop.isitem = bfalse;
      break;

    case F_SetChildAmmo:
      // This function sets the child's ammo
      pchild->ammo = pstate->tmpargument;
      break;

    case F_IfHitVulnerable:
      // This function proceeds if the character was hit by a weapon with the
      // correct vulnerability IDSZ...  [SILV] for Werewolves...
      returncode = HAS_SOME_BITS( pstate->alert, ALERT_HITVULNERABLE );
      break;

    case F_IfTargetIsFlying:
      // This function proceeds if the character target is flying
      returncode = ( ptarget->flyheight > 0 );
      break;

    case F_IdentifyTarget:
      // This function reveals the target's name, ammo, and usage
      // Proceeds if the target was unknown
      returncode = bfalse;
      {
        tmpchr = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
        if ( gs->ChrList[tmpchr].ammomax != 0 )  gs->ChrList[tmpchr].ammoknown = btrue;
        if ( 0 == strncmp(gs->ChrList[tmpchr].name, "Blah", 4) )
        {
          returncode = !gs->ChrList[tmpchr].prop.nameknown;
          gs->ChrList[tmpchr].prop.nameknown = btrue;
        }
        gs->ChrList[tmpchr].prop.usageknown = btrue;
      }
      break;

    case F_BeatModule:
      // This function displays the Module Ended message
      gs->modstate.beat = btrue;
      break;

    case F_EndModule:
      // This function presses the Escape key
      keyb.state[SDLK_ESCAPE] = 1;
      break;

    case F_DisableExport:
      // This function turns export off
      gs->modstate.exportvalid = bfalse;
      break;

    case F_EnableExport:
      // This function turns export on
      gs->modstate.exportvalid = btrue;
      break;

    case F_GetTargetState:
      // This function sets tmpargument to the state of the target
      pstate->tmpargument = ptarget->aistate.state;
      break;

    case F_ClearEndText:
      // This function empties the end-module text buffer
      gs->endtext[0]   = EOS;
      break;

    case F_AddEndText:
      // This function appends a message to the end-module text buffer
      append_end_text( gs, pobj->msg_start + pstate->tmpargument, ichr );
      break;

    case F_PlayMusic:
      // This function begins playing a new track of music
      if ( CData.allow_music )
      {
        SoundState_t * snd = snd_getState(&CData);
        if( NULL != snd && snd->song_index != pstate->tmpargument )
        {
          snd_play_music( pstate->tmpargument, pstate->tmpdistance, -1 );
        }
      }
      break;

    case F_SetMusicPassage:
      // This function makes the given passage play music if a player enters it
      // tmpargument is the passage to set and tmpdistance is the music track to play...
      gs->PassList[pstate->tmpargument].music = pstate->tmpdistance;
      break;

    case F_MakeCrushInvalid:
      // This function makes doors unable to close on this object
      pchr->prop.canbecrushed = bfalse;
      break;

    case F_StopMusic:
      // This function stops the interactive music
      snd_stop_music(1000);
      break;

    case F_FlashVariable:
      // This function makes the character flash according to tmpargument
      flash_character( gs, ichr, pstate->tmpargument );
      break;

    case F_AccelerateUp:
      // This function changes the character's up down velocity
      pchr->ori.vel.z += pstate->tmpargument / 100.0;
      break;

    case F_FlashVariableHeight:
      // This function makes the character flash, feet one color, head another...
      flash_character_height( gs, ichr, pstate->tmpturn, pstate->tmpx,
                              pstate->tmpdistance, pstate->tmpy );
      break;

    case F_SetDamageTime:
      // This function makes the character invincible for a little while
      pchr->damagetime = pstate->tmpargument;
      break;

    case F_IfStateIs8:
      returncode = ( 8 == pstate->state );
      break;

    case F_IfStateIs9:
      returncode = ( 9 == pstate->state );
      break;

    case F_IfStateIs10:
      returncode = ( 10 == pstate->state );
      break;

    case F_IfStateIs11:
      returncode = ( 11 == pstate->state );
      break;

    case F_IfStateIs12:
      returncode = ( 12 == pstate->state );
      break;

    case F_IfStateIs13:
      returncode = ( 13 == pstate->state );
      break;

    case F_IfStateIs14:
      returncode = ( 14 == pstate->state );
      break;

    case F_IfStateIs15:
      returncode = ( 15 == pstate->state );
      break;

    case F_IfTargetIsAMount:
      returncode = ptarget->bmpdata.calc_is_mount;
      break;

    case F_IfTargetIsAPlatform:
      returncode = ptarget->bmpdata.calc_is_platform;
      break;

    case F_AddStat:
      if ( !pchr->staton )
      {
        add_status( gs, ichr );
      }
      break;

    case F_DisenchantTarget:
      returncode = ( INVALID_ENC != ptarget->firstenchant );
      disenchant_character( gs, pstate->target );
      break;

    case F_DisenchantAll:
      {
        ENC_REF tmpenc;
        for ( tmpenc=0; tmpenc < ENCLST_COUNT; tmpenc++ )
        {
          remove_enchant( gs, tmpenc );
        }
      }
      break;

    case F_SetVolumeNearestTeammate:
      //This sets the volume for the looping sounds of all the character's teammates
      if( !gs->modstate.Paused && pstate->tmpdistance >= 0)
      {
        SoundState_t * snd = snd_getState(&CData);

        if(snd->soundActive)
        {
          //Go through all teammates
          tmpchr;
          for(tmpchr = 0; tmpchr < CHRLST_COUNT; tmpchr++)
          {
            if(!ACTIVE_CHR(gs->ChrList, tmpchr) ) continue;

            if(gs->ChrList[tmpchr].alive && gs->ChrList[tmpchr].team == pchr->team)
            {
              //And set their volume to tmpdistance
              if( pstate->tmpdistance >= 0 && INVALID_SOUND != gs->ChrList[tmpchr].loopingchannel )
              {
                Mix_Volume(gs->ChrList[tmpchr].loopingchannel, pstate->tmpdistance);
              }
            }
          }
        }
      }
      break;

    case F_AddShopPassage:
      // This function defines a shop area
      ShopList_add( gs, ichr, pstate->tmpargument );
      break;

    case F_TargetPayForArmor:
      // This function costs the target some money, or fails if 'e doesn't have
      // enough...
      // tmpx is amount needed
      // tmpy is cost of new skin
      {
        CAP_REF tmp_icap;
        tmpchr = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );    // The target
        tmp_icap = ChrList_getRCap(gs, tmpchr);                                     // The target's model
        iTmp =  gs->CapList[tmp_icap].skin[pstate->tmpargument % MAXSKIN].cost;
        pstate->tmpy = iTmp;                // Cost of new skin
        iTmp -= gs->CapList[tmp_icap].skin[gs->ChrList[tmpchr].skin_ref % MAXSKIN].cost;   // Refund
        if ( iTmp > gs->ChrList[tmpchr].money )
        {
          // Not enough...
          pstate->tmpx = iTmp - gs->ChrList[tmpchr].money;  // Amount needed
          returncode = bfalse;
        }
        else
        {
          // Pay for it...  Cost may be negative after refund...
          gs->ChrList[tmpchr].money -= iTmp;
          if ( gs->ChrList[tmpchr].money > MAXMONEY )  gs->ChrList[tmpchr].money = MAXMONEY;
          pstate->tmpx = 0;
          returncode = btrue;
        }
      }
      break;

    case F_JoinEvilTeam:
      // This function adds the character to the evil team...
      switch_team( gs, ichr, TEAM_REF(TEAM_EVIL) );
      break;

    case F_JoinNullTeam:
      // This function adds the character to the null team...
      switch_team( gs,  ichr, TEAM_REF(TEAM_NULL) );
      break;

    case F_JoinGoodTeam:
      // This function adds the character to the good team...
      switch_team( gs,  ichr, TEAM_REF(TEAM_GOOD) );
      break;

    case F_PitsKill:
      // This function activates pit deaths...
      gs->pitskill = btrue;
      break;

    case F_SetTargetToPassageID:
      // This function finds a character who is both in the passage and who has
      // an item with the given IDSZ
      {
        tmpchr = who_is_blocking_passage_ID( gs, pstate->tmpargument, pstate->tmpdistance );
        returncode = bfalse;
        if ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
        {
          pstate->target = tmpchr;
          returncode = btrue;
        }
      }
      break;

    case F_MakeNameUnknown:
      // This function makes the name of an item/character unknown.
      pchr->prop.nameknown = bfalse;
      break;

    case F_SpawnExactParticleEndSpawn:
      {
        vect3 prt_pos = {pstate->tmpx, pstate->tmpy, pstate->tmpdistance};
        vect3 prt_vel = {0,0,0};

        // This function spawns a particle that spawns a character...
        returncode = bfalse;
        tmpchr = ichr;
        if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )  tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, ichr );

        tmpprt = prt_spawn( gs, 1.0f, prt_pos, prt_vel, pchr->ori.turn_lr, pchr->model, PIP_REF(pstate->tmpargument), INVALID_CHR, (GRIP)0, pchr->team, tmpchr, 0, INVALID_CHR );
        if ( RESERVED_PRT( gs->PrtList, tmpprt ) )
        {
          gs->PrtList[tmpprt].spawncharacterstate = pstate->tmpturn;
          returncode = btrue;
        }
      }
      break;

    case F_SpawnPoofSpeedSpacingDamage:
      // This function makes a lovely little poof at the character's location,
      // adjusting the xy speed and spacing and the base damage first
      // Temporarily adjust the values for the particle type
      {
        PIP_REF tmp_ipip;
        CAP_REF tmp_icap = ChrList_getRCap(gs, ichr);
        if ( LOADED_CAP(gs->CapList, tmp_icap) )
        {
          Cap_t * tmp_pcap = ChrList_getPCap(gs, ichr);
          if ( tmp_pcap->gopoofprttype <= PRTPIP_PEROBJECT_COUNT )
          {
            tmp_ipip = ChrList_getRPip(gs, ichr, REF_TO_INT(tmp_pcap->gopoofprttype) );
          }
          else
          {
            tmp_ipip = tmp_pcap->gopoofprttype;
          }

          if ( INVALID_PIP != tmp_ipip )
          {
            // store the base values
            iTmp = gs->PipList[tmp_ipip].xyvel.ibase;
            tTmp = gs->PipList[tmp_ipip].xyspacing.ibase;
            test = gs->PipList[tmp_ipip].damage_fp8.ibase;

            // set some temporary values
            gs->PipList[tmp_ipip].xyvel.ibase = pstate->tmpx;
            gs->PipList[tmp_ipip].xyspacing.ibase = pstate->tmpy;
            gs->PipList[tmp_ipip].damage_fp8.ibase = pstate->tmpargument;

            // do the poof
            spawn_poof( gs, ichr, pchr->model );

            // Restore the saved values
            gs->PipList[tmp_ipip].xyvel.ibase = iTmp;
            gs->PipList[tmp_ipip].xyspacing.ibase = tTmp;
            gs->PipList[tmp_ipip].damage_fp8.ibase = test;
          };
        }
      }
      break;

    case F_GiveExperienceToGoodTeam:
      // This function gives experience to everyone on the G Team
      give_team_experience( gs, TEAM_REF(TEAM_GOOD), pstate->tmpargument, (EXPERIENCE)pstate->tmpdistance );
      break;

    case F_DoNothing:
      //This function does nothing (For use with combination with Else function or debugging)
      break;

    case F_DazeTarget:
      //This function dazes the target for a duration equal to tmpargument
      ptarget->dazetime += pstate->tmpargument;
      break;

    case F_GrogTarget:
      //This function grogs the target for a duration equal to tmpargument
      ptarget->grogtime += pstate->tmpargument;
      break;

    case F_IfEquipped:
      //This proceeds if the character is equipped
      returncode = pchr->isequipped;
      break;

    case F_DropTargetMoney:
      // This function drops some of the target's money
      drop_money( gs, pstate->target, pstate->tmpargument );
      break;

    case F_GetTargetContent:
      //This sets tmpargument to the current target's content value
      pstate->tmpargument = ptarget->aistate.content;
      break;

    case F_DropTargetKeys:
      //This function makes the target drops keys in inventory (Not inhand)
      drop_keys( gs, pstate->target );
      break;

    case F_JoinTeam:
      //This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)
      switch_team( gs,  ichr, (TEAM_REF)pstate->tmpargument );
      break;

    case F_TargetJoinTeam:
      //This makes the target join a team specified in tmpargument (A = 0, 23 = Z, etc.)
      switch_team( gs,  pstate->target, (TEAM_REF)pstate->tmpargument );
      break;

    case F_ClearMusicPassage:
      //This clears the music for an specified passage
      gs->PassList[pstate->tmpargument].music = INVALID_SOUND;
      break;


    case F_AddQuest:
      //This function adds a quest idsz set in tmpargument into the targets quest.txt
      if ( chr_is_player(gs, pstate->target) )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", ptarget->name );
        returncode = add_quest_idsz( cTmp, pstate->tmpargument );
      }
      break;

    case F_BeatQuest:
      //This function marks a IDSZ in the targets quest.txt as beaten
      returncode = bfalse;
      if ( chr_is_player(gs, pstate->target) )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", ptarget->name );
        if(modify_quest_idsz( cTmp, pstate->tmpargument, 0 ) == -2) returncode = btrue;
      }
      break;

    case F_IfTargetHasQuest:
      //This function proceeds if the target has the unfinished quest specified in tmpargument
      //and sets tmpx to the Quest Level of the specified quest.
      if ( chr_is_player(gs, pstate->target) )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", ptarget->name );
        iTmp = check_player_quest( cTmp, pstate->tmpargument );
        if ( iTmp > QUESTBEATEN )
        {
          returncode = btrue;
          pstate->tmpx = iTmp;
        }
        else returncode = bfalse;
      }
      break;

    case F_SetQuestLevel:
      //This function modifies the quest level for a specific quest IDSZ
      //tmpargument specifies quest idsz and tmpdistance the adjustment (which may be negative)	
      returncode = bfalse;
      if ( chr_is_player(gs, pstate->target) && pstate->tmpdistance != 0 )
      {
        snprintf( cTmp, sizeof( cTmp ), "%s.obj", ptarget->name );
        if(modify_quest_idsz( cTmp, pstate->tmpargument, pstate->tmpdistance ) != -1) returncode = btrue;
      }
      break;

    case F_IfTargetHasNotFullMana:
      //This function proceeds if the target has more than one point of mana and is alive
      returncode = bfalse;
      if ( ACTIVE_CHR( gs->ChrList, pstate->target ) )
      {
        returncode = ptarget->alive && ptarget->stats.manamax_fp8 > 0 && ptarget->stats.mana_fp8 < ptarget->stats.manamax_fp8 - DAMAGE_MIN;
      };
      break;

    case F_IfDoingAction:
      //This function proceeds if the character is preforming the animation specified in tmpargument
      returncode = pchr->action.now >= pstate->tmpargument && pchr->action.now <= pstate->tmpargument;
      break;

    case F_IfOperatorIsLinux:
      //This function proceeds if the computer is running a UNIX OS

      #ifdef __unix__
        returncode = btrue;    //Player_t running Linux
      #else
        returncode = bfalse;    //Player_t running something else.
      #endif

      break;

    case F_IfTargetIsOwner:
    //This function proceeds if the target is the characters owner
      returncode = (pstate->target == pstate->owner);
      break;

    case F_SetCameraSwing:
      //This function sets the camera swing rate
      GCamera.swing = 0;
      GCamera.swingrate = pstate->tmpargument;
      GCamera.swingamp = pstate->tmpdistance;
      break;

    case F_EnableRespawn:
      // This function turns respawn with JUMP button on
      gs->modstate.respawnvalid = btrue;
      break;

    case F_DisableRespawn:
      // This function turns respawning with JUMP button off
      gs->modstate.respawnvalid = bfalse;
      break;

    case F_IfButtonPressed:
      // This proceeds if the character is a player and pressing the latch specified in tmpargument
      returncode = HAS_SOME_BITS( pstate->latch.b, pstate->tmpargument ) && chr_is_player(gs, ichr);
      break;

    case F_IfHolderScoredAHit:
      // Proceed only if the character holder scored a hit
      returncode = bfalse;

      if ( chr_attached( gs->ChrList, CHRLST_COUNT, ichr ) )
      {
        pstate->tmpargument = HAS_SOME_BITS( gs->ChrList[pstate->target].aistate.alert, ALERT_SCOREDAHIT );
        returncode = btrue;
      }
      break;


    case F_End:
      break;

    default:
      assert( bfalse );

  }

  return returncode;
}

//--------------------------------------------------------------------------------------------
retval_t set_operand( AI_STATE * pstate, ScriptInfo_t * slist, Uint8 variable )
{
  // ZZ> This function sets one of the tmp* values for scripted AI

  retval_t retval = rv_succeed;
  char * scriptname = slist->fname[pstate->type];

  switch ( variable )
  {
    case VAR_TMP_X:
      pstate->tmpx = pstate->operationsum;
      break;

    case VAR_TMP_Y:
      pstate->tmpy = pstate->operationsum;
      break;

    case VAR_TMP_DISTANCE:
      pstate->tmpdistance = pstate->operationsum;
      break;

    case VAR_TMP_TURN:
      pstate->tmpturn = pstate->operationsum;
      break;

    case VAR_TMP_ARGUMENT:
      pstate->tmpargument = pstate->operationsum;
      break;

    default:
      log_warning("set_operand() - \n\t\"%s\"(0x%x) unknown variable", scriptname, pstate->offset);
      retval = rv_error;
      break;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
retval_t run_operand( Game_t * gs, Uint32 value, CHR_REF ichr )
{
  // ZZ> This function does the scripted arithmetic in operator,operand pairs

  retval_t retval = rv_succeed;
  int iTmp;

  ScriptInfo_t * slist = Game_getScriptInfo(gs);

  Chr_t * pchr = ChrList_getPChr(gs, ichr);
  Cap_t * pcap = ChrList_getPCap(gs, ichr);

  AI_STATE * pstate = &(pchr->aistate);

  CHR_REF   loc_aitarget;
  Chr_t    * ptarget;
  Cap_t    * ptarget_cap;

  CHR_REF   loc_aiowner;
  Chr_t    * powner;
  Cap_t    * powner_cap;

  CHR_REF   loc_leader;
  Chr_t    * pleader;
  Cap_t    * pleader_cap;

  // for debugging
  char    * scriptname = slist->fname[pchr->aistate.type];
  Uint32    offset     = pstate->offset;


  loc_aitarget = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
  if(INVALID_CHR == loc_aitarget)
    loc_aitarget = ichr;
  ptarget     = ChrList_getPChr(gs, loc_aitarget);
  ptarget_cap = ChrList_getPCap(gs, loc_aitarget);

  loc_aiowner  = chr_get_aiowner( gs->ChrList, CHRLST_COUNT, gs->ChrList + ichr );
  if(INVALID_CHR == loc_aiowner)
    loc_aiowner = ichr;
  powner       = ChrList_getPChr(gs, loc_aiowner);
  powner_cap   = ChrList_getPCap(gs, loc_aiowner);

  loc_leader   = team_get_leader( gs, pchr->team );
  if(INVALID_CHR == loc_leader)
    loc_leader = ichr;
  pleader      = ChrList_getPChr(gs, loc_leader);
  pleader_cap  = ChrList_getPCap(gs, loc_leader);

  // Get the operation code
  if ( IS_CONSTANT(value) )
  {
    // Get the working value from a constant, constants are all but high 5 bits
    iTmp = GET_CONSTANT(value);
  }
  else
  {
    // Get the working value from a register
    iTmp = 1;
    switch ( GET_VAR_BITS(value) )
    {
      case VAR_TMP_X:
        iTmp = pstate->tmpx;
        break;

      case VAR_TMP_Y:
        iTmp = pstate->tmpy;
        break;

      case VAR_TMP_DISTANCE:
        iTmp = pstate->tmpdistance;
        break;

      case VAR_TMP_TURN:
        iTmp = pstate->tmpturn;
        break;

      case VAR_TMP_ARGUMENT:
        iTmp = pstate->tmpargument;
        break;

      case VAR_RAND:
        iTmp = RANDIE(gs->randie_index);
        break;

      case VAR_SELF_X:
        iTmp = pchr->ori.pos.x;
        break;

      case VAR_SELF_Y:
        iTmp = pchr->ori.pos.y;
        break;

      case VAR_SELF_TURN:
        iTmp = pchr->ori.turn_lr;
        break;

      case VAR_SELF_COUNTER:
        iTmp = pchr->team_rank;
        break;

      case VAR_SELF_ORDER:
        iTmp = pchr->message.data;
        break;

      case VAR_SELF_MORALE:
        iTmp = gs->TeamList[pchr->team_base].morale;
        break;

      case VAR_SELF_LIFE:
        iTmp = pchr->stats.life_fp8;
        break;

      case VAR_TARGET_X:
        iTmp = ptarget->ori.pos.x;
        break;

      case VAR_TARGET_Y:
        iTmp = ptarget->ori.pos.y;
        break;

      case VAR_TARGET_DISTANCE:
        iTmp = 0;
        if(NULL != ptarget)
        {
          iTmp = sqrt(( ptarget->ori.pos.x - pchr->ori.pos.x ) * ( ptarget->ori.pos.x - pchr->ori.pos.x ) +
                      ( ptarget->ori.pos.y - pchr->ori.pos.y ) * ( ptarget->ori.pos.y - pchr->ori.pos.y ) +
                      ( ptarget->ori.pos.z - pchr->ori.pos.z ) * ( ptarget->ori.pos.z - pchr->ori.pos.z ) );
        }
        break;

      case VAR_TARGET_TURN:
        iTmp = ptarget->ori.turn_lr;
        break;

      case VAR_LEADER_X:
        iTmp = pchr->ori.pos.x;
        if ( ACTIVE_CHR( gs->ChrList, loc_leader ) )
          iTmp = pleader->ori.pos.x;
        break;

      case VAR_LEADER_Y:
        iTmp = pchr->ori.pos.y;
        if ( ACTIVE_CHR( gs->ChrList, loc_leader ) )
          iTmp = pleader->ori.pos.y;
        break;

      case VAR_LEADER_DISTANCE:
        iTmp = 10000;
        if ( ACTIVE_CHR( gs->ChrList, loc_leader ) )
          iTmp = ABS(( int )( pleader->ori.pos.x - pchr->ori.pos.x ) ) +
                 ABS(( int )( pleader->ori.pos.y - pchr->ori.pos.y ) );
        break;

      case VAR_LEADER_TURN:
        iTmp = pchr->ori.turn_lr;
        if ( ACTIVE_CHR( gs->ChrList, loc_leader ) )
          iTmp = pleader->ori.turn_lr;
        break;

      case VAR_GOTO_X:
        if( wp_list_empty( &(pstate->wp) ) )
        {
          iTmp = pchr->ori.pos.x;
        }
        else
        {
          iTmp = wp_list_x( &(pstate->wp) );
        }
        break;

      case VAR_GOTO_Y:
        if( wp_list_empty( &(pstate->wp) ) )
        {
          iTmp = pchr->ori.pos.y;
        }
        else
        {
          iTmp = wp_list_y( &(pstate->wp) );
        }
        break;

      case VAR_GOTO_DISTANCE:
        if( wp_list_empty( &(pstate->wp) ) )
        {
          iTmp = 0;
        }
        else
        {
          iTmp = ABS(( int )( wp_list_x( &(pstate->wp) ) - pchr->ori.pos.x ) ) +
                ABS(( int )( wp_list_y( &(pstate->wp) ) - pchr->ori.pos.y ) );
        }
        break;

      case VAR_TARGET_TURNTO:
        iTmp = vec_to_turn( ptarget->ori.pos.x - pchr->ori.pos.x, ptarget->ori.pos.y - pchr->ori.pos.y );
        break;

      case VAR_PASSAGE:
        iTmp = pchr->passage;
        break;

      case VAR_WEIGHT:
        iTmp = pchr->holdingweight;
        break;

      case VAR_SELF_ALTITUDE:
        iTmp = pchr->ori.pos.z - pchr->level;
        break;

      case VAR_SELF_ID:
        iTmp = pcap->idsz[IDSZ_TYPE];
        break;

      case VAR_SELF_HATEID:
        iTmp = pcap->idsz[IDSZ_HATE];
        break;

      case VAR_SELF_MANA:
        iTmp = pchr->stats.mana_fp8;
        if ( pchr->prop.canchannel )  iTmp += pchr->stats.life_fp8;
        break;

      case VAR_TARGET_STR:
        iTmp = ptarget->stats.strength_fp8;
        break;

      case VAR_TARGET_WIS:
        iTmp = ptarget->stats.wisdom_fp8;
        break;

      case VAR_TARGET_INT:
        iTmp = ptarget->stats.intelligence_fp8;
        break;

      case VAR_TARGET_DEX:
        iTmp = ptarget->stats.dexterity_fp8;
        break;

      case VAR_TARGET_LIFE:
        iTmp = ptarget->stats.life_fp8;
        break;

      case VAR_TARGET_MANA:
        iTmp = ptarget->stats.mana_fp8;
        if ( ptarget->prop.canchannel )  iTmp += ptarget->stats.life_fp8;
        break;

      case VAR_TARGET_LEVEL:
        iTmp = calc_chr_level( gs, loc_aitarget );
        break;

      case VAR_TARGET_SPEEDX:
        iTmp = ptarget->ori.vel.x;
        break;

      case VAR_TARGET_SPEEDY:
        iTmp = ptarget->ori.vel.y;
        break;

      case VAR_TARGET_SPEEDZ:
        iTmp = ptarget->ori.vel.z;
        break;

      case VAR_SELF_SPAWNX:
        iTmp = pchr->spinfo.pos.x;
        break;

      case VAR_SELF_SPAWNY:
        iTmp = pchr->spinfo.pos.y;
        break;

      case VAR_SELF_STATE:
        iTmp = pstate->state;
        break;

      case VAR_SELF_STR:
        iTmp = pchr->stats.strength_fp8;
        break;

      case VAR_SELF_WIS:
        iTmp = pchr->stats.wisdom_fp8;
        break;

      case VAR_SELF_INT:
        iTmp = pchr->stats.intelligence_fp8;
        break;

      case VAR_SELF_DEX:
        iTmp = pchr->stats.dexterity_fp8;
        break;

      case VAR_SELF_MANAFLOW:
        iTmp = pchr->stats.manaflow_fp8;
        break;

      case VAR_TARGET_MANAFLOW:
        iTmp = ptarget->stats.manaflow_fp8;
        break;

      case VAR_SELF_ATTACHED:
        iTmp = number_of_attached_particles( gs, ichr );
        break;

      case VAR_SWINGTURN:
        iTmp = GCamera.swing << 2;
        break;

      case VAR_XYDISTANCE:
        iTmp = sqrt( pstate->tmpx * pstate->tmpx + pstate->tmpy * pstate->tmpy );
        break;

      case VAR_SELF_Z:
        iTmp = pchr->ori.pos.z;
        break;

      case VAR_TARGET_ALTITUDE:
        iTmp = ptarget->ori.pos.z - ptarget->level;
        break;

      case VAR_TARGET_Z:
        iTmp = ptarget->ori.pos.z;
        break;

      case VAR_SELF_INDEX:
        iTmp = REF_TO_INT(ichr);
        break;

      case VAR_OWNER_X:
        iTmp = powner->ori.pos.x;
        break;

      case VAR_OWNER_Y:
        iTmp = powner->ori.pos.y;
        break;

      case VAR_OWNER_TURN:
        iTmp = powner->ori.turn_lr;
        break;

      case VAR_OWNER_DISTANCE:
        iTmp = sqrt(( powner->ori.pos.x - pchr->ori.pos.x ) * ( powner->ori.pos.x - pchr->ori.pos.x ) +
                    ( powner->ori.pos.y - pchr->ori.pos.y ) * ( powner->ori.pos.y - pchr->ori.pos.y ) +
                    ( powner->ori.pos.z - pchr->ori.pos.z ) * ( powner->ori.pos.z - pchr->ori.pos.z ) );
        break;

      case VAR_OWNER_TURNTO:
        iTmp = vec_to_turn( powner->ori.pos.x - pchr->ori.pos.x, powner->ori.pos.y - pchr->ori.pos.y );
        break;

      case VAR_XYTURNTO:
        iTmp = vec_to_turn( pstate->tmpx - pchr->ori.pos.x, pstate->tmpy - pchr->ori.pos.y );
        break;

      case VAR_SELF_MONEY:
        iTmp = pchr->money;
        break;

      case VAR_SELF_ACCEL:
        iTmp = ( pchr->skin.maxaccel * 100.0 );
        break;

      case VAR_TARGET_EXP:
        iTmp = ptarget->experience;
        break;

      case VAR_SELF_AMMO:
        iTmp = pchr->ammo;
        break;

      case VAR_TARGET_AMMO:
        iTmp = ptarget->ammo;
        break;

      case VAR_TARGET_MONEY:
        iTmp = ptarget->money;
        break;

      case VAR_TARGET_TURNAWAY:
        iTmp = vec_to_turn( pchr->ori.pos.x - ptarget->ori.pos.x, pchr->ori.pos.y - ptarget->ori.pos.y );
        break;

      case VAR_SELF_LEVEL:
        iTmp = calc_chr_level( gs, ichr );
        break;

      case VAR_SPAWN_DISTANCE:
        iTmp = sqrt(( pchr->spinfo.pos.x - pchr->ori.pos.x ) * ( pchr->spinfo.pos.x - pchr->ori.pos.x ) +
                    ( pchr->spinfo.pos.y - pchr->ori.pos.y ) * ( pchr->spinfo.pos.y - pchr->ori.pos.y ) +
                    ( pchr->spinfo.pos.z - pchr->ori.pos.z ) * ( pchr->spinfo.pos.z - pchr->ori.pos.z ) );
        break;

	  case VAR_TARGET_MAX_LIFE:
        iTmp = ptarget->stats.lifemax_fp8;
        break;

	  case VAR_SELF_CONTENT:
        iTmp = pstate->content;
        break;

      default:
        log_warning("run_operand() - \n\t\"%s\"(0x%x) unknown variable\n", scriptname, pstate->offset);
        return rv_error;
        break;

    }
  }


  // Now do the math
  switch ( GET_OP_BITS(value) )
  {
    case OP_ADD:
      pstate->operationsum += iTmp;
      break;

    case OP_SUB:
      pstate->operationsum -= iTmp;
      break;

    case OP_AND:
      pstate->operationsum &= iTmp;
      break;

    case OP_SHR:
      pstate->operationsum >>= iTmp;
      break;

    case OP_SHL:
      pstate->operationsum <<= iTmp;
      break;

    case OP_MUL:
      pstate->operationsum *= iTmp;
      break;

    case OP_DIV:
      if ( iTmp == 0 )
      {
        log_warning("run_operand() - \n\t\"%s\"(0x%x) divide by zero", scriptname, pstate->offset);
      }
      else
      {
        pstate->operationsum /= iTmp;
      }
      break;

    case OP_MOD:
      if ( iTmp == 0 )
      {
        log_warning("run_operand() - \n\t\"%s\"(0x%x) modulus using zero", scriptname, pstate->offset);
      }
      else
      {
        pstate->operationsum %= iTmp;
      }
      break;

    default:
      log_warning("run_operand() - \n\t\"%s\"(0x%x) unknown operation", scriptname, pstate->offset);
      return rv_error;
      break;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
retval_t run_script( Game_t * gs, CHR_REF ichr, float dUpdate )
{
  // ZZ> This function lets one ichr do AI stuff

  Uint16   type_stt;
  Uint32   offset_end, offset_last;
  Uint32   opcode;
  Uint32   iTmp;
  bool_t   functionreturn;
  int      operands;
  retval_t retval = rv_succeed;

  ScriptInfo_t  * slist;
  Chr_t        * pchr;
  AI_STATE    * pstate;

  if( !EKEY_PVALID(gs) ) return rv_error;
  slist  = Game_getScriptInfo(gs);

  if( !ACTIVE_CHR(gs->ChrList, ichr) ) return rv_error;
  pchr   = gs->ChrList + ichr;
  pstate = &(pchr->aistate);

  // Make life easier
  pstate->oldtarget = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, pchr );
  type_stt = pstate->type;

  // down the ai timer
  pstate->time -= dUpdate;
  if ( pstate->time < 0 ) pstate->time = 0.0f;

  // Figure out alerts that weren't already set
  set_alerts( gs, ichr, dUpdate );

  // Clear the button latches

  if ( !chr_is_player(gs, ichr) )
  {
    pstate->latch.b = 0;
  }

  // make sure that all of the ai "pointers" are valid
  // if a "pointer" is not valid, make it point to the character
  if( !ACTIVE_CHR(gs->ChrList, pstate->target)     ) pstate->target     = ichr;
  if( !ACTIVE_CHR(gs->ChrList, pstate->oldtarget)  ) pstate->oldtarget  = ichr;
  if( !ACTIVE_CHR(gs->ChrList, pstate->owner)      ) pstate->owner      = ichr;
  if( !ACTIVE_CHR(gs->ChrList, pstate->child)      ) pstate->child      = ichr;
  if( !ACTIVE_CHR(gs->ChrList, pstate->bumplast)   ) pstate->bumplast   = ichr;
  if( !ACTIVE_CHR(gs->ChrList, pstate->attacklast) ) pstate->attacklast = ichr;
  if( !ACTIVE_CHR(gs->ChrList, pstate->hitlast)    ) pstate->hitlast    = ichr;

  // Reset the target if it can't be seen
  if ( !pchr->prop.canseeinvisible && pchr->alive && ichr != pstate->oldtarget )
  {
    if ( chr_is_invisible( gs->ChrList, CHRLST_COUNT, pstate->target ) )
    {
      pstate->target = ichr;
    }
  }

  // Run the AI Script
  pstate->indent       = 0;
  pstate->lastindent   = 0;
  pstate->operationsum = 0;
  pstate->offset       = slist->offset_stt[type_stt];
  offset_last          = pstate->offset;
  offset_end           = slist->offset_end[type_stt];
  while ( pstate->offset < offset_end )  // End Function
  {
    offset_last = pstate->offset;
    opcode = slist->buffer[pstate->offset];
    if( IS_END( opcode ) ) break;

    // set the indents
    pstate->lastindent = pstate->indent;
    pstate->indent     = GET_INDENT(opcode);

    // Was it a function
    if ( IS_FUNCTION( opcode ) )
    {
      // Run the function
      functionreturn = run_function( gs, opcode, ichr );

      // Get the jump code
      pstate->offset++;
      iTmp = slist->buffer[pstate->offset];
      if ( functionreturn )
      {
        // Proceed to the next function
        pstate->offset++;
      }
      else
      {
        // Jump to where the jump code says to go
        pstate->offset = iTmp;
      }
    }
    else
    {
      // Get the number of operands
      pstate->offset++;
      operands = slist->buffer[pstate->offset];

      // Now run the operation
      pstate->operationsum = 0;
      pstate->offset++;
      while ( operands > 0 )
      {
        iTmp = slist->buffer[pstate->offset];
        if(rv_error == run_operand( gs, iTmp, ichr )) // This sets pstate->operationsum
        {
          return rv_error;
        };
        operands--;
        pstate->offset++;
      }

      // Save the results in the register that called the arithmetic
      if(rv_error == set_operand( pstate, slist, opcode ))
      {
        return rv_error;
      };
    }

    // This is used by the Else function
    pstate->lastindent = GET_INDENT(opcode);
  }

  assert(pstate->offset <= offset_end);

  // Set latches
  if ( !chr_is_player(gs, ichr) && 0 != type_stt )
  {
    CHR_REF rider = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, ichr, SLOT_SADDLE );

    if ( pchr->prop.ismount && ACTIVE_CHR( gs->ChrList, rider ) && !gs->ChrList[rider].prop.isitem )
    {
      // Mount
      pstate->latch.x = gs->ChrList[rider].aistate.latch.x;
      pstate->latch.y = gs->ChrList[rider].aistate.latch.y;
    }
    else
    {


      // Normal AI
      if( wp_list_empty(&(pstate->wp)) )
      {
        pstate->latch.x = pstate->latch.y = 0;
      }
      else
      {
        float fnum, fden;

        pstate->latch.x = wp_list_x( &(pstate->wp) ) - pchr->ori.pos.x;
        pstate->latch.y = wp_list_y( &(pstate->wp) ) - pchr->ori.pos.y;

        fnum = pstate->latch.x * pstate->latch.x + pstate->latch.y * pstate->latch.y;
        fden = fnum + 25 * pchr->bmpdata.calc_size * pchr->bmpdata.calc_size;
        if ( fnum > 0.0f )
        {
          float ftmp = 1.0f / sqrt( fnum ) * fnum / fden;
          pstate->latch.x *= ftmp;
          pstate->latch.y *= ftmp;
        }
      };

    }
  }


  // Clear alerts for next time around
  pstate->alert = 0;
  if ( pstate->morphed || pstate->type != type_stt )
  {
    pstate->alert |= ALERT_CHANGED;
    pstate->morphed = bfalse;
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
void run_all_scripts( Game_t * gs, float dUpdate )
{
  // ZZ> This function lets every computer controlled character do AI stuff

  CHR_REF character;
  bool_t allow_thinking;
  PChr_t chrlst = gs->ChrList;

  numblip = 0;
  for ( character = 0; character < CHRLST_COUNT; character++ )
  {
    if ( !ACTIVE_CHR( chrlst, character ) ) continue;

    allow_thinking = bfalse;

    // Cleaned up characters shouldn't be alert to anything else
    if ( HAS_SOME_BITS( chrlst[character].aistate.alert, ALERT_CRUSHED ) )
    {
      chrlst[character].aistate.alert = ALERT_CRUSHED;
      allow_thinking = btrue;
    }

    if ( HAS_SOME_BITS( chrlst[character].aistate.alert, ALERT_CLEANEDUP ) )
    {
      chrlst[character].aistate.alert = ALERT_CLEANEDUP;
      allow_thinking = btrue;
    };

    // Do not exclude items in packs. In NetHack, eggs can hatch while in your pack...
    // this may need to be handled differently. Currently dead things are thinking too much...
    if ( !chr_in_pack( chrlst, CHRLST_COUNT, character ) || chrlst[character].alive )
    {
      allow_thinking = btrue;
    }

    if ( allow_thinking )
    {
      run_script( gs, character, dUpdate );
    }
  }


}



//------------------------------------------------------------------------------
// Don't use the aicodes file, just register them in the program.
// There will never be a reason to change them, and it will prevent transcription mistakes between the
// source code and the aicodes.txt file

//void load_ai_codes(char* loadname)
//{
//  // ZZ> This function loads all of the function and variable names
//
//  FILE* fileread;
//  int read;
//
//  opcode_lst.count = 0;
//  fileread = fs_fileOpen(PRI_NONE, NULL, loadname, "rb");
//  if (NULL != fileread)
//  {
//    _cstate.file_size = (int)fread(_cstate.file_buffer, 1, MD2MAXLOADSIZE, fileread);
//    read = 0;
//    read = ai_goto_colon(read);
//    while (read != _cstate.file_size)
//    {
//      fget_code(read);
//      read = ai_goto_colon(read);
//    }
//    fs_fileClose(fileread);
//  }
//  else log_warning("Could not load script AI functions or variables (%s)\n", loadname);
//}



bool_t scr_break_passage( Game_t * gs, AI_STATE * pstate )
{
  return break_passage(gs, pstate->tmpargument, pstate->tmpturn, pstate->tmpdistance, pstate->tmpx, pstate->tmpy, &(pstate->tmpx), &(pstate->tmpy) );
}

bool_t scr_search_tile_in_passage( Game_t * gs, AI_STATE * pstate )
{
  return search_tile_in_passage( gs, pstate->tmpargument, pstate->tmpdistance, pstate->tmpx, pstate->tmpy, &(pstate->tmpx), &(pstate->tmpy) );
}



