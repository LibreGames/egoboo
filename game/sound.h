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

/// \file sound.h
/// \details Sound handling using SDL_mixer

#include "egoboo_typedef_cpp.h"

#include "sound_defs.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// what type of music data is used by snd_mix_ptr
enum e_mix_type { MIX_UNKNOWN = 0, MIX_MUS, MIX_SND };
typedef enum e_mix_type snd_mix_type;

/// an anonymized "pointer" type in case we want to store data that is either a chunk or a music
struct snd_mix_ptr
{
    snd_mix_type type;

    union
    {
        void      * unk;
        Mix_Music * mus;
        Mix_Chunk * snd;
    } ptr;
};

//--------------------------------------------------------------------------------------------
struct ego_snd_config : public s_snd_config_data {};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern ego_snd_config snd;

extern Mix_Chunk * g_wavelist[GSND_COUNT];      ///< All sounds loaded into memory

extern bool_t      musicinmemory;                          ///< Is the music loaded in memory?
extern Sint8       songplaying;                            ///< Current song that is playing
extern Mix_Music * musictracksloaded[MAXPLAYLISTLENGTH];   ///< This is a specific music file loaded into memory

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// The global functions for the sound module

bool_t sound_initialize();
void   sound_restart();

Mix_Chunk * sound_load_chunk_vfs( const char * szFileName );
Mix_Music * sound_load_music( const char * szFileName );
bool_t      sound_load( snd_mix_ptr * pptr, const char * szFileName, snd_mix_type type );

int     sound_play_mix( fvec3_t pos, snd_mix_ptr * pptr );
int     sound_play_chunk_looped( fvec3_t pos, Mix_Chunk * pchunk, int loops, const CHR_REF & object );
#define sound_play_chunk( pos, pchunk ) sound_play_chunk_looped( pos, pchunk, 0, CHR_REF(MAX_CHR) )
void    sound_play_song( int songnumber, Uint16 fadetime, int loops );
void    sound_finish_song( Uint16 fadetime );
int     sound_play_chunk_full( Mix_Chunk * pchunk );

void    sound_fade_all();
void    fade_in_music( Mix_Music * music );

void    sound_stop_channel( int whichchannel );
void    sound_stop_song();

void    load_global_waves( void );
void    load_all_music_sounds_vfs();

bool_t looped_stop_object_sounds( const CHR_REF &  ichr );
void   looped_update_all_sound();

void   sound_finish_sound();
void   sound_free_chunk( Mix_Chunk * pchunk );

int get_current_song_playing();
bool_t LoopedList_remove( int channel );
