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

#include "egoboo_types.h"
#include "egoboo_math.h"

#include <SDL_mixer.h> // for Mix_* stuff.
#include <SDL_opengl.h>

#include <stdio.h>

enum e_slot;
enum e_grip;
enum e_Action;
enum e_lip_transition;
enum e_damage;
enum e_Experience;
enum e_Team;
enum e_gender;
enum e_particle_alpha_type;
enum e_blud_level;
enum e_respawn_mode;
enum e_idsz_index;
enum e_color;

struct sChr;
struct sStatus;
struct sPlayer;
struct s_mod_info;
struct sGame;
struct sKeyboardBuffer;
struct sConfigData;

void insert_space( size_t position );
void copy_one_line( size_t write );
size_t load_one_line( size_t read );
size_t load_parsed_line( size_t read );
void surround_space( size_t position );
void parse_null_terminate_comments();
int get_indentation();
void fix_operators();
int starts_with_capital_letter();

size_t ai_goto_colon( size_t read );
void fget_code( FILE * pfile );

enum e_Action what_action( char cTmp );
void release_all_textures(struct sGame * gs);
Uint32 load_one_icon( struct sGame * gs, char * szPathname, const char * szObjectname, char * szFilename );
void release_all_icons(struct sGame * gs);



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


void   keep_weapons_with_holders( struct sGame * gs );
void   make_prtlist( struct sGame * gs );
bool_t debug_message( int time, const char *format, ... );
void   reset_end_text( struct sGame * gs );

Uint16 terp_dir( Uint16 majordir, float dx, float dy, float dUpdate );
Uint16 terp_dir_fast( Uint16 majordir, float dx, float dy, float dUpdate );


void make_textureoffset( void );



bool_t PlaList_set_latch( struct sGame * gs, struct sPlayer * player );

void check_add( Uint8 key, char bigletter, char littleletter );
void camera_calc_turn_lr();
void make_camera_matrix();
void figure_out_what_to_draw();
void set_local_latches( struct sGame * gs );
void adjust_camera_angle( int height );
void move_camera( float dUpdate );
void make_onwhichfan( struct sGame * gs );
void do_bumping( struct sGame * gs, float dUpdate );

void do_weather_spawn( struct sGame * gs, float dUpdate );
void stat_return( struct sGame * gs, float dUpdate );
void pit_kill( struct sGame * gs, float dUpdate );
void reset_players( struct sGame * gs );

void resize_characters( struct sGame * gs, float dUpdate );



void export_one_character_name( struct sGame * gs, char *szSaveName, CHR_REF character );
void CapList_save_one( struct sGame * gs, char *szSaveName, CHR_REF character );
void export_one_character_skin( struct sGame * gs, char *szSaveName, CHR_REF character );


bool_t load_bars( char* szBitmap );
void load_map( struct sGame * gs, char* szModule );

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
void do_chr_dynalight(struct sGame * gs);
void do_prt_dynalight(struct sGame * gs);
void set_fan_dyna_light( int fanx, int fany, PRT_REF particle );
void render_water();
void draw_scene_zreflection();
void draw_blip( enum e_color color, float x, float y );
void draw_one_icon( int icontype, int x, int y, Uint8 sparkle );

void   draw_map( float x, float y );
int    draw_one_bar( int bartype, int x, int y, int ticks, int maxticks );
bool_t draw_scene( struct sGame * gs );
void   draw_main( float );


void load_blip_bitmap( char * modname );


bool_t check_skills( struct sGame * gs, CHR_REF who, Uint32 whichskill );
void check_player_import(struct sGame * gs);
void reset_camera();

void gltitle();





void render_mad_lit( CHR_REF character );
void render_particle_reflections();
void render_water_fan_lit( Uint32 fan, Uint8 layer, Uint8 mode );

void make_speklut();

bool_t handle_opengl_error();

char * str_encode( char *strout, size_t insize, char * strin );
char * str_decode( char *strout, size_t insize, char * strin );


// MD2 Stuff
struct s_ego_md2_model;

int mad_vertexconnected( struct s_ego_md2_model * m, int vertex );
int mad_calc_transvertices( struct s_ego_md2_model * m );



#endif //#ifndef _PROTO_H_
