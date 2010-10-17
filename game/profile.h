#pragma once

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

#include "bsp.h"

#include "egoboo_setup.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_chr;
struct ego_prt_data;

struct ego_cap;
struct ego_mad;
struct s_eve_data;
struct ego_pip;
struct Mix_Chunk;

struct mpd_BSP  ;

struct ego_prt_bundle;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Placeholders used while importing profiles
struct pro_import_t
{
    int   slot;
    int   player;
    int   slot_lst[MAX_PROFILE];
    int   max_slot;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// This is for random naming

#define CHOPPERMODEL                    32
#define MAXCHOP                         (MAX_PROFILE*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)
#define MAXSECTION                      4              ///< T-wi-n-k...  Most of 4 sections

/// The buffer for the random naming data
struct chop_data_t
{
    size_t  chop_count;             ///< The global number of name parts

    Uint32  carat;                  ///< The data pointer
    char    buffer[CHOPDATACHUNK];  ///< The name parts
    int     start[MAXCHOP];         ///< The first character of each part
};

chop_data_t * chop_data_init( chop_data_t * pdata );

bool_t        chop_export_vfs( const char *szSaveName, const char * szChop );

//--------------------------------------------------------------------------------------------

/// Definition of a single chop section
struct chop_section_t
{
    int size;     ///< Number of choices, 0
    int start;    ///< A reference to a specific offset in the chop_data_t buffer
};

#define CHOP_SECTION_INIT { /* size */ -1, /* start */ -1 }

//--------------------------------------------------------------------------------------------

/// Definition of the chop info needed to create a name
struct chop_definition_t
{
    chop_section_t  section[MAXSECTION];
};

chop_definition_t * chop_definition_init( chop_definition_t * pdefinition );

#define CHOP_DEFINITION_INIT { {CHOP_SECTION_INIT} }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a wrapper for all the datafiles in the *.obj dir
struct ego_pro
{
    EGO_PROFILE_STUFF;

    // the sub-profiles
    REF_T   iai;                              ///< the AI  for this profile
    CAP_REF icap;                             ///< the cap for this profile
    MAD_REF imad;                             ///< the mad for this profile
    EVE_REF ieve;                             ///< the eve for this profile

    PIP_REF prtpip[MAX_PIP_PER_PROFILE];      ///< Local particles

    // the profile skins
    size_t  skins;                            ///< Number of skins
    TX_REF  tex_ref[MAX_SKIN];                ///< references to the icon textures
    TX_REF  ico_ref[MAX_SKIN];                ///< references to the skin textures

    // the profile message info
    int     message_start;                    ///< The first message

    /// the random naming info
    chop_definition_t chop;

    // sounds
    struct Mix_Chunk *  wavelist[MAX_WAVE];             ///< sounds in a object
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the profile list

DECLARE_LIST_EXTERN( ego_pro, ProList, MAX_PROFILE );

int          pro_get_slot_vfs( const char * tmploadname, int slot_override );
const char * pro_create_chop( const PRO_REF by_reference profile_ref );
bool_t       pro_load_chop_vfs( const PRO_REF by_reference profile_ref, const char *szLoadname );

void    ProList_init();
//void    ProList_free_all();
size_t  ProList_get_free( const PRO_REF by_reference override_ref );
bool_t  ProList_free_one( const PRO_REF by_reference object_ref );

#define VALID_PRO_RANGE( IOBJ ) ( ((IOBJ) >= 0) && ((IOBJ) < MAX_PROFILE) )
#define LOADED_PRO( IOBJ )       ( VALID_PRO_RANGE( IOBJ ) && ProList.lst[IOBJ].loaded )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the BSP structure housing the object
struct ego_obj_BSP
{
    // the BSP of characters for character-character and character-particle interactions
    ego_BSP_tree     tree;

    static ego_obj_BSP root;
    static int         chr_count;      ///< the number of characters in the ego_obj_BSP::root structure
    static int         prt_count;      ///< the number of particles  in the ego_obj_BSP::root structure

    static bool_t ctor( ego_obj_BSP * pbsp, struct mpd_BSP   * pmesh_bsp );
    static bool_t dtor( ego_obj_BSP * pbsp );

    static bool_t alloc( ego_obj_BSP * pbsp, int depth );
    static bool_t dealloc( ego_obj_BSP * pbsp );

    static bool_t fill( ego_obj_BSP * pbsp );
    static bool_t empty( ego_obj_BSP * pbsp );

    //bool_t insert_leaf( ego_obj_BSP * pbsp, ego_BSP_leaf   * pnode, int depth, int address_x[], int address_y[], int address_z[] );
    static bool_t insert_chr( ego_obj_BSP * pbsp, struct ego_chr * pchr );
    static bool_t insert_prt( ego_obj_BSP * pbsp, struct ego_prt_bundle * pbdl_prt );

    static int    collide( ego_obj_BSP * pbsp, ego_BSP_aabb   * paabb, ego_BSP_leaf_pary_t * colst );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern size_t  bookicon_count;
extern TX_REF  bookicon_ref[MAX_SKIN];                      ///< The first book icon

extern pro_import_t import_data;
extern chop_data_t chop_mem;

DECLARE_STATIC_ARY_TYPE( MessageOffsetAry, int, MAXTOTALMESSAGE );
DECLARE_EXTERN_STATIC_ARY( MessageOffsetAry, MessageOffset );

extern Uint32          message_buffer_carat;                                  ///< Where to put letter
extern char            message_buffer[MESSAGEBUFFERSIZE];                     ///< The text buffer

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void profile_system_begin();
void profile_system_end();

void   init_all_profiles();
int    load_profile_skins_vfs( const char * tmploadname, const PRO_REF by_reference object_ref );
void   load_all_messages_vfs( const char *loadname, const PRO_REF by_reference object_ref );
void   release_all_pro_data();
void   release_all_profiles();
void   release_all_pro();
void   release_all_local_pips();
bool_t release_one_pro( const PRO_REF by_reference object_ref );
bool_t release_one_local_pips( const PRO_REF by_reference object_ref );

int load_one_profile_vfs( const char* tmploadname, int slot_override );

void reset_messages();

const char *  chop_create( chop_data_t * pdata, chop_definition_t * pdef );
bool_t        chop_load_vfs( chop_data_t * pchop_data, const char *szLoadname, chop_definition_t * pchop_definition );
