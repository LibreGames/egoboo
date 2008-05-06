/* Egoboo - proto.h
 * Function prototypes for a huge portion of the game code.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _PROTO_H_
#define _PROTO_H_

#include "egoboo_types.h"
#include "egoboo_math.h"

#include <SDL_mixer.h> // for Mix_* stuff.
#include <SDL_opengl.h>

typedef enum slot_e SLOT;
typedef enum grip_e GRIP;
typedef enum action_e ACTION;
typedef enum lip_transition_e LIPT;
typedef enum damage_e DAMAGE;
typedef enum experience_e EXPERIENCE;
typedef enum team_e TEAM;
typedef enum gender_e GENDER;
typedef enum particle_type PRTTYPE;
typedef enum blud_level_e BLUD_LEVEL;
typedef enum respawn_mode_e RESPAWN_MODE;
typedef enum idsz_index_e IDSZ_INDEX;
typedef enum color_e COLR;

void load_graphics();

void insert_space( size_t position );
void copy_one_line( size_t write );
size_t load_one_line( size_t read );
size_t load_parsed_line( size_t read );
void surround_space( size_t position );
void parse_null_terminate_comments();
int get_indentation();
void fix_operators();
int starts_with_capital_letter();
Uint32 get_high_bits();
size_t tell_code( size_t read );
void add_code( Uint32 highbits );
void parse_line_by_line();
Uint32 jump_goto( int index );
void parse_jumps( int ainumber );
void log_code( int ainumber, char* savename );
int ai_goto_colon( int read );
void fget_code( FILE * pfile );
void load_ai_codes( char* loadname );
Uint32 load_ai_script( char * szModpath, char * szObjectname );
void reset_ai_script();
ACTION what_action( char cTmp );
float mesh_get_level( Uint32 fan, float x, float y, bool_t waterwalk );
void release_all_textures();
void load_one_icon( char * szModname, char * szObjectname, char * szFilename );
void prime_titleimage();
void prime_icons();
void release_all_icons();
void release_all_titleimages();
void release_map();
void module_release( void );
void close_session();
void make_newloadname( char *modname, char *appendname, char *newloadname );
void export_all_local_players( void );
void quit_module( void );


//Enchants
void free_all_enchants();



int get_free_message( void );

void remove_enchant( Uint16 enchantindex );
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex );
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
void getadd( int MIN, int value, int MAX, int* valuetoadd );
void fgetadd( float MIN, float value, float MAX, float* valuetoadd );
void add_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
Uint16 spawn_enchant( Uint16 owner, Uint16 target,
                      Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional );
void load_action_names( char* loadname );
void read_setup( char* filename );
void log_madused( char *savename );
void make_lightdirectionlookup();
float spek_global_lighting( int rotation, int inormal, vect3 lite );
void make_spektable( vect3 lite );
void make_lighttospek( void );


//Sound and music stuff
bool_t load_all_music_sounds();
void stop_music(int fadetime);
void load_global_waves( char *modname );
bool_t sdlmixer_initialize();
void sound_apply_mods( int channel, float intensity, vect3 snd_pos, vect3 ear_pos, Uint16 ear_turn_lr  );
int play_sound( float intensity, vect3 pos, Mix_Chunk *loadedwave, int loops, int whichobject, int soundnumber);
void stop_sound( int whichchannel );
void play_music( int songnumber, int fadetime, int loops );

//Graphical Functions
void render_particles();
void render_particle_reflections();
void render_mad_lit( CHR_REF character );
void render_water_fan_lit( Uint32 fan, Uint8 layer, Uint8 mode );

// Passage control functions
bool_t open_passage( Uint32 passage );
void check_passage_music();
void flash_passage( Uint32 passage, Uint8 color );
Uint16 who_is_blocking_passage( Uint32 passage );
Uint16 who_is_blocking_passage_ID( Uint32 passage, IDSZ idsz );
bool_t close_passage( Uint32 passage );
void clear_passages();
Uint32 add_shop_passage( Uint16 owner, Uint32 passage );
Uint32 add_passage( int tlx, int tly, int brx, int bry, bool_t open, Uint32 mask );

void add_to_dolist( Uint16 cnt );
void order_dolist( void );
void make_dolist( void );


void keep_weapons_with_holders();
void make_enviro( void );
void make_prtlist( void );
void debug_message( int time, const char *format, ... );
void reset_end_text();

void do_enchant_spawn( float dUpdate );

void show_stat( Uint16 statindex );
void check_stats();
void check_screenshot();
bool_t dump_screenshot();
void add_stat( CHR_REF character );
void move_to_top( CHR_REF character );
void sort_stat();

Uint16 terp_dir( Uint16 majordir, float dx, float dy, float dUpdate );
Uint16 terp_dir_fast( Uint16 majordir, float dx, float dy, float dUpdate );

void move_water( float dUpdate );

Uint32 generate_unsigned( PAIR * ppair );
Sint32 generate_signed( PAIR * ppair );
Sint32 generate_dither( PAIR * ppair, Uint16 strength_fp8 );

CHR_REF search_best_leader( TEAM team, Uint16 exclude );

void give_team_experience( TEAM team, int amount, EXPERIENCE xptype );
void disenchant_character( CHR_REF character );

void naming_names( int profile );
void read_naming( char * szModpath, char * szObjectname, int profile );
void prime_names( void );


void signal_target( Uint16 target, Uint16 upper, Uint16 lower );
void signal_team( CHR_REF character, Uint32 order );
void signal_idsz_index( Uint32 order, IDSZ idsz, IDSZ_INDEX index );
void set_alerts( CHR_REF character, float dUpdate );
bool_t module_reference_matches( char *szLoadName, IDSZ idsz );
void add_module_idsz( char *szLoadName, IDSZ idsz );

bool_t add_quest_idsz( char *whichplayer, IDSZ idsz );
int    modify_quest_idsz( char *whichplayer, IDSZ idsz, int adjustment );
int    check_player_quest( char *whichplayer, IDSZ idsz );




void make_textureoffset( void );
int add_player( CHR_REF character, Uint16 player, Uint8 device );
void clear_messages();
void setup_passage( char *modname );
void setup_alliances( char *modname );


void check_add( Uint8 key, char bigletter, char littleletter );
void camera_calc_turn_lr();
//void project_view();
void make_renderlist();
void make_camera_matrix();
void figure_out_what_to_draw();
void set_one_player_latch( Uint16 player );
void set_local_latches( void );
void adjust_camera_angle( int height );
void move_camera( float dUpdate );
void make_onwhichfan( void );
void do_bumping( float dUpdate );
bool_t prt_is_over_water( int cnt );
void do_weather_spawn( float dUpdate );
void animate_tiles( float dUpdate );
void stat_return( float dUpdate );
void pit_kill( float dUpdate );
void reset_players();
int find_module( char *smallname );
void resize_characters( float dUpdate );
void update_game( float dUpdate );
void update_timers();
void load_basic_textures( char *modname );





void export_one_character_name( char *szSaveName, CHR_REF character );
void export_one_character_profile( char *szSaveName, CHR_REF character );
void export_one_character_skin( char *szSaveName, CHR_REF character );
void reset_particles( char* modname );


void load_all_messages( char *szModpath, char *szObjectname, Uint16 object );

bool_t load_bars( char* szBitmap );
void load_map( char* szModule );
bool_t load_font( char* szBitmap, char* szSpacing );
void make_water();
void read_wawalite( char *modname );
void reset_teams();
void reset_messages();
void make_randie();
void module_load( char *smallname );
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
void render_refmad( int tnc, Uint8 trans );
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
bool_t request_pageflip();
bool_t do_pageflip();
bool_t do_clear();
void draw_scene();
void draw_main( float );

bool_t load_one_title_image( int titleimage, char *szLoadName );
bool_t module_read_data( int modnumber, char *szLoadName );
bool_t module_read_summary( char *szLoadName );
void load_all_menu_images();
void load_blip_bitmap( char * modname );


bool_t check_skills( int who, Uint32 whichskill );
void check_player_import();
void reset_timers();
void reset_camera();
void sdlinit( int argc, char **argv );
int  glinit( int argc, char **argv );
void gltitle();

void memory_cleanUp(void);

//---------------------------------------------------------------------------------------------
// Filesystem functions

typedef enum priority_e
{
  PRI_NONE = 0,
  PRI_WARN,
  PRI_FAIL
} PRIORITY;

void   empty_import_directory();
int DirGetAttrib( char *fromdir );
void fs_init();
FILE * fs_fileOpen( PRIORITY pri, const char * src, const char * fname, const char * mode );
void fs_fileClose( FILE * pfile );
const char *fs_getTempDirectory();
const char *fs_getImportDirectory();
const char *fs_getGameDirectory();
const char *fs_getSaveDirectory();
int fs_fileIsDirectory( const char *filename );
int fs_createDirectory( const char *dirname );
int fs_removeDirectory( const char *dirname );
void fs_deleteFile( const char *filename );
void fs_copyFile( const char *source, const char *dest );
void fs_removeDirectoryAndContents( const char *dirname );
void fs_copyDirectory( const char *sourceDir, const char *destDir );

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

bool_t passage_check_any( CHR_REF ichr, Uint16 pass, Uint16 * powner );
bool_t passage_check_all( CHR_REF ichr, Uint16 pass, Uint16 * powner );
bool_t passage_check( CHR_REF ichr, Uint16 pass, Uint16 * powner );

// MD2 Stuff
typedef struct ego_md2_model_t MD2_Model;

int vertexconnected( MD2_Model * m, int vertex );
int mad_calc_transvertices( MD2_Model * m );

void release_all_models();
void init_all_models();

void md2_blend_vertices(CHR_REF ichr, Sint32 vrtmin, Sint32 vrtmax);
void md2_blend_lighting(CHR_REF ichr);


typedef struct collision_volume_t CVolume;




#endif //#ifndef _PROTO_H_
