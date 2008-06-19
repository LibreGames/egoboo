/* Egoboo - proto.h
 * Function prototypes for a huge portion of the game code.
 */

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

#ifndef _PROTO_H_
#define _PROTO_H_

#include "egoboo_types.inl"
#include "egoboo_math.inl"

#include <SDL_mixer.h> // for Mix_* stuff.
#include <SDL_opengl.h>

typedef enum slot_e SLOT;
typedef enum grip_e GRIP;
typedef enum Action_e ACTION;
typedef enum lip_transition_e LIPT;
typedef enum damage_e DAMAGE;
typedef enum Experience_e EXPERIENCE;
typedef enum Team_e TEAM;
typedef enum gender_e GENDER;
typedef enum particle_type PRTTYPE;
typedef enum blud_level_e BLUD_LEVEL;
typedef enum respawn_mode_e RESPAWN_MODE;
typedef enum idsz_index_e IDSZ_INDEX;
typedef enum color_e COLR;

struct CChr_t;
struct Status_t;
struct CPlayer_t;
struct mod_data_t;
struct CGame_t;
struct KeyboardBuffer_t;

void insert_space( size_t position );
void copy_one_line( size_t write );
size_t load_one_line( size_t read );
size_t load_parsed_line( size_t read );
void surround_space( size_t position );
void parse_null_terminate_comments();
int get_indentation();
void fix_operators();
int starts_with_capital_letter();

int ai_goto_colon( int read );
void fget_code( FILE * pfile );

ACTION what_action( char cTmp );
void release_all_textures(struct CGame_t * gs);
Uint32 load_one_icon( char * szModname, const char * szObjectname, char * szFilename );
void release_all_icons(struct CGame_t * gs);
void release_map();


void close_session();


void read_setup( char* filename );
float spek_global_lighting( int rotation, int inormal, vect3 lite );
void make_spektable( vect3 lite );
void make_lighttospek( void );


//Graphical Functions
void render_particles();
void render_particle_reflections();
void render_mad_lit( CHR_REF character );
void render_water_fan_lit( Uint32 fan, Uint8 layer, Uint8 mode );


void dolist_add( Uint16 cnt );
void dolist_sort( void );
void dolist_make( void );


void   keep_weapons_with_holders( struct CGame_t * gs );
void   make_prtlist();
bool_t debug_message( int time, const char *format, ... );
void   reset_end_text( struct CGame_t * gs );

Uint16 terp_dir( Uint16 majordir, float dx, float dy, float dUpdate );
Uint16 terp_dir_fast( Uint16 majordir, float dx, float dy, float dUpdate );


void make_textureoffset( void );



bool_t PlaList_set_latch( struct CGame_t * gs, struct CPlayer_t * player );

void check_add( Uint8 key, char bigletter, char littleletter );
void camera_calc_turn_lr();
void make_camera_matrix();
void figure_out_what_to_draw();
void set_local_latches( struct CGame_t * gs );
void adjust_camera_angle( int height );
void move_camera( float dUpdate );
void make_onwhichfan( struct CGame_t * gs );
void do_bumping( struct CGame_t * gs, float dUpdate );

void do_weather_spawn( struct CGame_t * gs, float dUpdate );
void animate_tiles( float dUpdate );
void stat_return( struct CGame_t * gs, float dUpdate );
void pit_kill( struct CGame_t * gs, float dUpdate );
void reset_players( struct CGame_t * gs );

void resize_characters( struct CGame_t * gs, float dUpdate );
void load_basic_textures( struct CGame_t * gs, char *modname );





void export_one_character_name( struct CGame_t * gs, char *szSaveName, CHR_REF character );
void export_one_character_profile( struct CGame_t * gs, char *szSaveName, CHR_REF character );
void export_one_character_skin( struct CGame_t * gs, char *szSaveName, CHR_REF character );


bool_t load_bars( char* szBitmap );
void load_map( struct CGame_t * gs, char* szModule );
bool_t load_font( char* szBitmap, char* szSpacing );
void make_water( struct CGame_t * gs );
void read_wawalite( struct CGame_t * gs, char *modname );

void render_prt();
void render_shadow( CHR_REF character );
//void render_bad_shadow( CHR_REF character );
void render_refprt();
void render_fan( Uint32 fan, char tex_loaded );
void render_fan_ref( Uint32 fan, char tex_loaded, float level );
void render_water_fan( Uint32 fan, Uint8 layer, Uint8 mode );
void render_enviromad( CHR_REF character, Uint8 trans );
void render_texmad( CHR_REF character, Uint8 trans );
void render_mad( CHR_REF character, Uint8 trans );
void render_refmad( CHR_REF tnc, Uint16 trans );
void light_characters();
void light_particles();
void set_fan_light( int fanx, int fany, PRT_REF particle );
void do_dynalight();
void render_water();
void draw_scene_zreflection();
void draw_blip( COLR color, float x, float y );
void draw_one_icon( int icontype, int x, int y, Uint8 sparkle );
void draw_one_font( int fonttype, float x, float y );
void draw_map( float x, float y );
int draw_one_bar( int bartype, int x, int y, int ticks, int maxticks );
int length_of_word( char *szText );
void draw_scene();
void draw_main( float );


void load_blip_bitmap( char * modname );


bool_t check_skills( struct CGame_t * gs, CHR_REF who, Uint32 whichskill );
void check_player_import(struct CGame_t * gs);
void reset_camera();

void gltitle();

//---------------------------------------------------------------------------------------------
// Filesystem functions

typedef enum priority_e
{
  PRI_NONE = 0,
  PRI_WARN,
  PRI_FAIL
} PRIORITY;

void fs_init();
const char *fs_getTempDirectory();
const char *fs_getImportDirectory();
const char *fs_getGameDirectory();
const char *fs_getSaveDirectory();

FILE * fs_fileOpen( PRIORITY pri, const char * src, const char * fname, const char * mode );
void fs_fileClose( FILE * pfile );
int  fs_fileIsDirectory( const char *filename );
int  fs_createDirectory( const char *dirname );
int  fs_removeDirectory( const char *dirname );
void fs_deleteFile( const char *filename );
void fs_copyFile( const char *source, const char *dest );
void fs_removeDirectoryAndContents( const char *dirname );
void fs_copyDirectory( const char *sourceDir, const char *destDir );
void empty_import_directory();
int  DirGetAttrib( char *fromdir );

// Enumerate directory contents
typedef enum fs_type_e
{
  FS_UNKNOWN = -1,
  FS_WIN32 = 0,
  FS_LIN,
  FS_MAC
} FS_TYPE;

typedef struct fs_find_info_win32_t FS_FIND_INFO_WIN32;
typedef struct fs_find_info_lin_t   FS_FIND_INFO_LIN;
typedef struct fs_find_info_mac_t   FS_FIND_INFO_MAC;

typedef struct fs_find_info_t
{
  FS_TYPE type;

  union
  {
    FS_FIND_INFO_WIN32 * W;
    FS_FIND_INFO_LIN   * L;
    FS_FIND_INFO_MAC   * M;
  };

} FS_FIND_INFO;

FS_FIND_INFO * fs_find_info_new(FS_FIND_INFO * i);
bool_t         fs_find_info_delete(FS_FIND_INFO * i);

const char *fs_findFirstFile(FS_FIND_INFO * i, const char *searchPath, const char *searchBody, const char *searchExtension );
const char *fs_findNextFile(FS_FIND_INFO * i);
void fs_findClose(FS_FIND_INFO * i);



void render_mad_lit( CHR_REF character );
void render_particle_reflections();
void render_water_fan_lit( Uint32 fan, Uint8 layer, Uint8 mode );

void make_speklut();

bool_t handle_opengl_error();

char * str_convert_spaces( char *strout, size_t insize, char * strin );
char * str_convert_underscores( char *strout, size_t insize, char * strin );


// MD2 Stuff
typedef struct ego_md2_model_t MD2_Model;

int mad_vertexconnected( MD2_Model * m, int vertex );
int mad_calc_transvertices( MD2_Model * m );

typedef struct collision_volume_t CVolume;



#endif //#ifndef _PROTO_H_
