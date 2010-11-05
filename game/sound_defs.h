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

#include "egoboo_typedef.h"

#include <SDL_mixer.h>

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_config_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAX_WAVE              30           ///< Up to 30 wave/ogg per model
#define MAXPLAYLISTLENGTH     35           ///< Max number of different tracks loaded into memory
#define INVALID_SOUND         -1           ///< The index of this sound is not valid
#define INVALID_SOUND_CHANNEL -1           ///< SDL_mixer sound channel is invalid
#define MENU_SONG              0           ///< default music theme played when in the menu

#define MIX_HIGH_QUALITY   44100        ///< frequency 44100 for 44.1KHz, which is CD audio rate.
    /// \details Most games use 22050, because 44100 requires too much
    /// CPU power on older computers.

#define VALID_SND( ISND )       ( ISND >= 0 && ISND < MAX_WAVE )

/// Pre defined global particle sounds
    typedef enum e_global_sounds
    {
        GSND_GETCOIN = 0,
        GSND_DEFEND,
        GSND_WEATHER1,
        GSND_WEATHER2,
        GSND_COINFALL,
        GSND_LEVELUP,
        GSND_PITFALL,
        GSND_SHIELDBLOCK,
        GSND_COUNT
    }
    GSND_GLOBAL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The global variables for the sound module
    struct s_snd_config_data
    {
        bool_t       soundvalid;           ///< Allow playing of sound?
        Uint8        soundvolume;          ///< Volume of sounds played

        bool_t       musicvalid;           ///< Allow music and loops?
        Uint8        musicvolume;          ///< The sound volume of music

        int          maxsoundchannel;      ///< Max number of sounds playing at the same time
        int          buffersize;           ///< Buffer size set in setup.txt
        bool_t       highquality;          ///< Allow CD quality frequency sounds?
    };

    typedef struct s_snd_config_data snd_config_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    snd_config_data_t * snd_get_config();

    bool_t snd_config_synch( snd_config_data_t * psnd, struct s_config_data * pcfg );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

#define sound_defs_h
