#pragma once

#include "egoboo_types.h"
#include "egoboo_config.h"

#include <stdio.h>

enum blud_level_e
{
  BLUD_NONE = 0,
  BLUD_NORMAL,
  BLUD_ULTRA                         // This makes any damage draw blud
};
typedef enum blud_level_e BLUD_LEVEL;

enum slot_e;
enum grip_e;
enum Action_e;
enum lip_transition_e;
enum damage_e;
enum Experience_e;
enum Team_e;
enum gender_e;
enum dyna_mode_e;
enum particle_type;
enum respawn_mode_e;
enum idsz_index_e;
enum color_e;

struct MessageData_t;

bool_t undo_pair_fp8( PAIR * ppair, RANGE * prange );
bool_t fget_pair_fp8( FILE* fileread, PAIR * ppair );
bool_t fget_next_pair_fp8( FILE* fileread, PAIR * ppair );

bool_t undo_pair( PAIR * ppair, RANGE * prange );
bool_t fget_pair( FILE* fileread, PAIR * ppair );
bool_t fget_next_pair( FILE* fileread, PAIR * ppair );

char * undo_idsz( IDSZ idsz );
IDSZ fget_idsz( FILE* fileread );
IDSZ fget_next_idsz( FILE* fileread );

int   fget_int( FILE* fileread );
int   fget_next_int( FILE* fileread );

float fget_float( FILE* fileread );
float fget_next_float( FILE* fileread );

Uint16 fget_fixed( FILE* fileread );
Uint16 fget_next_fixed( FILE* fileread );

bool_t fget_bool( FILE* fileread );
bool_t fget_next_bool( FILE* fileread );

enum gender_e fget_gender( FILE* fileread );
enum gender_e fget_next_gender( FILE* fileread );

enum damage_e fget_damage( FILE *fileread );
enum damage_e fget_next_damage( FILE *fileread );

enum blud_level_e fget_blud( FILE *fileread );
enum blud_level_e fget_next_blud( FILE *fileread );

RESPAWN_MODE fget_respawn( FILE *fileread );
RESPAWN_MODE fget_next_respawn( FILE *fileread );

enum dyna_mode_e fget_dynamode( FILE *fileread );
enum dyna_mode_e fget_next_dynamode( FILE *fileread );

bool_t fget_name( FILE* fileread, char *szName, size_t lnName );
bool_t fget_next_name( FILE* fileread, char *szName, size_t lnName );

bool_t fget_string( FILE* fileread, char *szLine, size_t lnLine );
bool_t fget_next_string( FILE* fileread, char *szLine, size_t lnLine );

enum particle_type fget_prttype( FILE * fileread );
enum particle_type fget_next_prttype( FILE * fileread );

enum Action_e fget_action( FILE* fileread );
enum Action_e fget_next_action( FILE* fileread );

void ftruthf( FILE* filewrite, char* text, bool_t truth );
void fdamagf( FILE* filewrite, char* text, enum damage_e damagetype );
void factiof( FILE* filewrite, char* text, enum Action_e action );
void fgendef( FILE* filewrite, char* text, enum gender_e gender );
void fpairof( FILE* filewrite, char* text, PAIR * ppair );
void funderf( FILE* filewrite, char* text, char* usename );

bool_t fget_message( FILE* fileread, struct MessageData_t * msglst  );
bool_t fget_next_message( FILE* fileread, struct MessageData_t * msglst );

void   fgoto_colon( FILE* fileread );
bool_t fgoto_colon_yesno( FILE* fileread );
char   fget_first_letter( FILE* fileread );


//FILE * inherit_fopen(char * szObjPath, char * szObject, char *szFname, char * mode);
const char * inherit_fname(const char * szObjPath, const char * szObject, const char *szFname );

retval_t util_calculateCRC(char * filename, Uint32 seed, Uint32 * pCRC);

Uint32 generate_unsigned( Uint32 * pseed, PAIR * ppair );
Sint32 generate_signed( Uint32 * pseed, PAIR * ppair );
Sint32 generate_dither( Uint32 * pseed, PAIR * ppair, Uint16 strength_fp8 );

void make_randie();

