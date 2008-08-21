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

struct sMessageData;

// My lil' random number table
#define RANDIE_BITS  12
#define RANDIE_COUNT (1 << RANDIE_BITS)
#define RANDIE_MASK  (RANDIE_COUNT - 1)
#define RANDIE(IND) randie[IND]; IND++; IND &= RANDIE_MASK;
extern  Uint16 randie[RANDIE_COUNT];


#define FRAND(PSEED) ( 2.0f*( float ) ego_rand_32(PSEED) / ( float ) (1 << 16) / ( float ) (1 << 16) - 1.0f )
#define RAND(PSEED, MINVAL, MAXVAL) ((((ego_rand_32(PSEED) >> 16) * (MAXVAL-MINVAL)) >> 16)  + MINVAL)
#define IRAND(PSEED, BITS) ( ego_rand_32(PSEED) & ((1<<BITS)-1) )

extern EGO_CONST char *globalname;   // For debuggin' fgoto_colon

bool_t undo_pair_fp8( PAIR * ppair, RANGE * prange );
bool_t fget_pair_fp8( FILE* fileread, PAIR * ppair );
bool_t fget_next_pair_fp8( FILE* fileread, PAIR * ppair );

bool_t undo_pair( PAIR * ppair, RANGE * prange );
bool_t fget_pair( FILE* fileread, PAIR * ppair );
bool_t fget_next_pair( FILE* fileread, PAIR * ppair );

bool_t fput_next( FILE* filewrite, EGO_CONST char * comment );

char * undo_idsz( IDSZ idsz );
bool_t ftest_idsz( FILE* fileread );

IDSZ   fget_idsz( FILE* fileread );
IDSZ   fget_next_idsz( FILE* fileread );
bool_t fput_idsz( FILE* filewrite, IDSZ data );
bool_t fput_next_idsz( FILE* filewrite, EGO_CONST char * comment, IDSZ data );

int   fget_int( FILE* fileread );
int   fget_next_int( FILE* fileread );
bool_t fput_int( FILE* filewrite, int data );
bool_t fput_next_int( FILE* filewrite, EGO_CONST char * comment, int data );

float fget_float( FILE* fileread );
float fget_next_float( FILE* fileread );
bool_t fput_float( FILE* filewrite, float data );
bool_t fput_next_float( FILE* filewrite, EGO_CONST char * comment, float data );

Uint16 fget_fixed( FILE* fileread );
Uint16 fget_next_fixed( FILE* fileread );
bool_t fput_fixed( FILE* filewrite, Uint16 data );
bool_t fput_next_fixed( FILE* filewrite, EGO_CONST char * comment, Uint16 data );

bool_t fget_bool( FILE* fileread );
bool_t fget_next_bool( FILE* fileread );
bool_t fput_bool( FILE* filewrite, bool_t data );
bool_t fput_next_bool( FILE* filewrite, EGO_CONST char * comment, bool_t data );

enum e_gender fget_gender( FILE* fileread );
enum e_gender fget_next_gender( FILE* fileread );
bool_t fput_gender( FILE* filewrite, enum e_gender data );
bool_t fput_next_gender( FILE* filewrite, EGO_CONST char * comment, enum e_gender data );

enum e_damage fget_damage( FILE *fileread );
enum e_damage fget_next_damage( FILE *fileread );
bool_t fput_damage( FILE* filewrite, enum e_damage data );
bool_t fput_next_damage( FILE* filewrite, EGO_CONST char * comment, enum e_damage data );

enum e_blud_level fget_blud( FILE *fileread );
enum e_blud_level fget_next_blud( FILE *fileread );
bool_t fput_blud( FILE* filewrite, enum e_blud_level data );
bool_t fput_next_blud( FILE* filewrite, EGO_CONST char * comment, enum e_blud_level data );

RESPAWN_MODE fget_respawn( FILE *fileread );
RESPAWN_MODE fget_next_respawn( FILE *fileread );

enum e_dyna_mode fget_dynamode( FILE *fileread );
enum e_dyna_mode fget_next_dynamode( FILE *fileread );
bool_t fput_dynamode( FILE* filewrite, enum e_dyna_mode data );
bool_t fput_next_dynamode( FILE* filewrite, EGO_CONST char * comment, enum e_dyna_mode data );

bool_t fget_name( FILE* fileread, char *szName, size_t lnName );
bool_t fget_next_name( FILE* fileread, char *szName, size_t lnName );
bool_t fput_name( FILE* filewrite, char * data );
bool_t fput_next_name( FILE* filewrite, EGO_CONST char * comment, char * data );

bool_t fget_string( FILE* fileread, char *szLine, size_t lnLine );
bool_t fget_next_string( FILE* fileread, char *szLine, size_t lnLine );
bool_t fput_string( FILE* filewrite, char * data );
bool_t fput_next_string( FILE* filewrite, EGO_CONST char * comment, char * data );

enum e_particle_alpha_type fget_prttype( FILE * fileread );
enum e_particle_alpha_type fget_next_prttype( FILE * fileread );
bool_t fput_alpha_type( FILE* filewrite, enum e_particle_alpha_type data );
bool_t fput_next_alpha_type( FILE* filewrite, EGO_CONST char * comment, enum e_particle_alpha_type data );

enum e_Action fget_action( FILE* fileread );
enum e_Action fget_next_action( FILE* fileread );
bool_t fput_action( FILE* filewrite, enum e_Action data );
bool_t fput_next_action( FILE* filewrite, EGO_CONST char * comment, enum e_Action data );

void ftruthf( FILE* filewrite, char* text, bool_t truth );
void fdamagf( FILE* filewrite, char* text, enum e_damage damagetype );
void factiof( FILE* filewrite, char* text, enum e_Action action );
void fgendef( FILE* filewrite, char* text, enum e_gender gender );
void fpairof( FILE* filewrite, char* text, PAIR * ppair );
void funderf( FILE* filewrite, char* text, char* usename );

bool_t fget_message( FILE* fileread, struct sMessageData * msglst  );
bool_t fget_next_message( FILE* fileread, struct sMessageData * msglst );
bool_t fput_message( FILE* filewrite, struct sMessageData * data, int num );
bool_t fput_next_message( FILE* filewrite, EGO_CONST char * comment, struct sMessageData * data, int num  );

bool_t fput_expansion( FILE* filewrite, char * idsz_string, int value );
bool_t fput_next_expansion( FILE* filewrite, EGO_CONST char * comment, char * idsz_string, int value );

bool_t fput_pair( FILE* filewrite, PAIR * data );
bool_t fput_next_pair( FILE* filewrite, EGO_CONST char * comment, PAIR * data );

Uint32 fget_damage_modifier( FILE * fileread );
bool_t fput_damage_modifier_char( FILE * filewrite, Uint32 mod );
bool_t fput_damage_modifier_shift( FILE * filewrite, Uint32 mod );
bool_t fput_damage_modifier( FILE * filewrite, Uint32 mod );

void   fgoto_colon( FILE* fileread );
bool_t fgoto_colon_yesno( FILE* fileread );
char   fget_first_letter( FILE* fileread );
bool_t fgoto_colon_yesno_buffer( FILE* fileread, char * buffer, size_t bufflen  );
void   fgoto_colon_buffer( FILE* fileread, char * buffer, size_t bufflen );


//FILE * inherit_fopen(char * szObjPath, char * szObject, char *szFname, char * mode);
EGO_CONST char * inherit_fname(EGO_CONST char * szObjPath, EGO_CONST char * szObject, EGO_CONST char *szFname );

retval_t util_calculateCRC(char * filename, Uint32 seed, Uint32 * pCRC);

Uint32 generate_unsigned( Uint32 * pseed, PAIR * ppair );
Sint32 generate_signed( Uint32 * pseed, PAIR * ppair );
Sint32 generate_dither( Uint32 * pseed, PAIR * ppair, Uint16 strength_fp8 );

void make_randie( void );
