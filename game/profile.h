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
struct ego_pro_import
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
struct ego_chop_data
{
    size_t  chop_count;             ///< The global number of name parts

    Uint32  carat;                  ///< The data pointer
    char    buffer[CHOPDATACHUNK];  ///< The name parts
    int     start[MAXCHOP];         ///< The first character of each part
};

ego_chop_data * chop_data_init( ego_chop_data * pdata );

bool_t        chop_export_vfs( const char *szSaveName, const char * szChop );

//--------------------------------------------------------------------------------------------

/// Definition of a single chop section
struct ego_chop_section
{
    int size;     ///< Number of choices, 0
    int start;    ///< A reference to a specific offset in the ego_chop_data buffer

    ego_chop_section()
    {
        size = start = -1;
    }
};

//--------------------------------------------------------------------------------------------

/// Definition of the chop info needed to create a name
struct ego_chop_definition
{
    ego_chop_section  section[MAXSECTION];
};

ego_chop_definition * chop_definition_init( ego_chop_definition * pdefinition );

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
    ego_chop_definition chop;

    // sounds
    struct Mix_Chunk *  wavelist[MAX_WAVE];             ///< sounds in a object
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the profile list

extern t_cpp_list< ego_pro, MAX_PROFILE  > ProList;

int          pro_get_slot_vfs( const char * tmploadname, int slot_override );
const char * pro_create_chop( const PRO_REF & profile_ref );
bool_t       pro_load_chop_vfs( const PRO_REF & profile_ref, const char *szLoadname );

void    ProList_init();
//void    ProList_free_all();
size_t  ProList_get_free( const PRO_REF & override_ref );
bool_t  ProList_free_one( const PRO_REF & object_ref );

#define VALID_PRO_RANGE( IOBJ ) ( ((IOBJ) >= 0) && ((IOBJ) < MAX_PROFILE) )
#define LOADED_PRO( IOBJ )       ( VALID_PRO_RANGE( IOBJ ) && ProList.lst[IOBJ].loaded )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern size_t  bookicon_count;
extern TX_REF  bookicon_ref[MAX_SKIN];                      ///< The first book icon

extern ego_pro_import import_data;
extern ego_chop_data chop_mem;

DECLARE_STATIC_ARY_TYPE( MessageOffsetAry, int, MAXTOTALMESSAGE );
DECLARE_EXTERN_STATIC_ARY( MessageOffsetAry, MessageOffset )

extern Uint32          message_buffer_carat;                                  ///< Where to put letter
extern char            message_buffer[MESSAGEBUFFERSIZE];                     ///< The text buffer

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void profile_system_begin();
void profile_system_end();

void   init_all_profiles();
int    load_profile_skins_vfs( const char * tmploadname, const PRO_REF & object_ref );
void   load_all_messages_vfs( const char *loadname, const PRO_REF & object_ref );
void   release_all_pro_data();
void   release_all_profiles();
void   release_all_pro();
void   release_all_local_pips();
bool_t release_one_pro( const PRO_REF & object_ref );
bool_t release_one_local_pips( const PRO_REF & object_ref );

int load_one_profile_vfs( const char* tmploadname, int slot_override );

void reset_messages();

const char *  chop_create( ego_chop_data * pdata, ego_chop_definition * pdef );
bool_t        chop_load_vfs( ego_chop_data * pchop_data, const char *szLoadname, ego_chop_definition * pchop_definition );
