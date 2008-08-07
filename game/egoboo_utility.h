#pragma once

#include "egoboo_types.h"
#include "egoboo_config.h"

#include <stdio.h>

enum e_blud_level
{
  BLUD_NONE = 0,
  BLUD_NORMAL,
  BLUD_ULTRA                         // This makes any damage draw blud
};
typedef enum e_blud_level BLUD_LEVEL;

enum e_slot;
enum e_grip;
enum e_Action;
enum e_lip_transition;
enum e_damage;
enum e_Experience;
enum e_Team;
enum e_gender;
enum e_dyna_mode;
enum e_particle_alpha_type;
enum e_respawn_mode;
enum e_idsz_index;
enum e_color;

struct sMessageData;

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

enum e_gender fget_gender( FILE* fileread );
enum e_gender fget_next_gender( FILE* fileread );

enum e_damage fget_damage( FILE *fileread );
enum e_damage fget_next_damage( FILE *fileread );

enum e_blud_level fget_blud( FILE *fileread );
enum e_blud_level fget_next_blud( FILE *fileread );

RESPAWN_MODE fget_respawn( FILE *fileread );
RESPAWN_MODE fget_next_respawn( FILE *fileread );

enum e_dyna_mode fget_dynamode( FILE *fileread );
enum e_dyna_mode fget_next_dynamode( FILE *fileread );

bool_t fget_name( FILE* fileread, char *szName, size_t lnName );
bool_t fget_next_name( FILE* fileread, char *szName, size_t lnName );

bool_t fget_string( FILE* fileread, char *szLine, size_t lnLine );
bool_t fget_next_string( FILE* fileread, char *szLine, size_t lnLine );

enum e_particle_alpha_type fget_prttype( FILE * fileread );
enum e_particle_alpha_type fget_next_prttype( FILE * fileread );

enum e_Action fget_action( FILE* fileread );
enum e_Action fget_next_action( FILE* fileread );

void ftruthf( FILE* filewrite, char* text, bool_t truth );
void fdamagf( FILE* filewrite, char* text, enum e_damage damagetype );
void factiof( FILE* filewrite, char* text, enum e_Action action );
void fgendef( FILE* filewrite, char* text, enum e_gender gender );
void fpairof( FILE* filewrite, char* text, PAIR * ppair );
void funderf( FILE* filewrite, char* text, char* usename );

bool_t fget_message( FILE* fileread, struct sMessageData * msglst  );
bool_t fget_next_message( FILE* fileread, struct sMessageData * msglst );

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

