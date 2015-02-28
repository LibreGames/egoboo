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

/// @file game/graphic_texture.h

#pragma once

#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Special Textures
enum e_global_tx_type
{
	/* "mp_data/particle_trans", TRANSCOLOR */
    TX_PARTICLE_TRANS = 0,
	/* "mp_data/particle_light", INVALID_KEY */
    TX_PARTICLE_LIGHT,
	/* "mp_data/tile0",TRANSCOLOR */
    TX_TILE_0,
	/* "mp_data/tile1",TRANSCOLOR */
    TX_TILE_1,
	/* "mp_data/tile2",TRANSCOLOR */
    TX_TILE_2,
	/* "mp_data/tile3",TRANSCOLOR */
    TX_TILE_3,
	/* "mp_data/watertop", TRANSCOLOR */
    TX_WATER_TOP,
	/* "mp_data/waterlow", TRANSCOLOR */
    TX_WATER_LOW,
	/* "mp_data/phong", TRANSCOLOR */
    TX_PHONG,
	/* "mp_data/bars", INVALID_KEY vs. TRANSCOLOR */
    TX_BARS,
	/* "mp_data/blip", INVALID_KEY */
    TX_BLIP,
	/* "mp_data/plan", INVALID_KEY */
    TX_MAP,
	/* "mp_data/xpbar", TRANSCOLOR*/
    TX_XP_BAR,
	/* "mp_data/nullicon", INVALID_KEY */
    TX_ICON_NULL,           //Empty icon
    TX_FONT_BMP,            //Font bitmap
    TX_ICON_KEYB,           //Keyboard
    TX_ICON_MOUS,           //Mouse
    TX_ICON_JOYA,           //White joystick
    TX_ICON_JOYB,           //Black joystick
    TX_CURSOR,              //Mouse cursor
    TX_SKULL,               //Difficulity skull
    TX_SPECIAL_LAST
};



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

inline bool VALID_TX_RANGE(const TX_REF ref)
{
    return ref >= 0 && ref < TX_COUNT;
}

struct TextureManager
{
protected:
    /**
     * @brief
     *  The texture manager singleton.
     */
    static TextureManager *_singleton;

    /**
     * @brief
     *  The list of texture objects.
     */
    oglx_texture_t *_lst[TX_COUNT];

    /**
     * @brief
     *  The set of free texture references.
     */
    std::unordered_set<TX_REF> _free;

    /**
     * @brief
     *  Construct this texture manager.
     * @remark
     *  Intentionally protected.
     */
    TextureManager();

    /**
     * @brief
     *  Destruct this texture manager.
     * @remark
     *  Intentionally protected.
     */
    virtual ~TextureManager();

    /**
     * @brief
     *  Mark all textures as free.
     */
    void freeAll();

public:

    //Disable copying class
    TextureManager(const TextureManager& copy) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

	/**
	 * @brief
	 *	Get the texture manager.
	 * @return
	 *	the texture manager
	 * @pre
	 *	The texture manager must be initialized.
	 * @warning
	 *	Uninitializing the texture manager will invalidate any pointers returned by calls to this method prior to uninitialization.
	 */
	static TextureManager *getSingleton();
	
	/**
	 * @brief
	 *	Start-up the texture manager.
	 * @remark
	 *	If the texture manager is initialized, a call to this method is a no-op.
	 */
	static void initialize();

	/**
	 * @brief
	 *	Uninitialize the texture manager.
	 * @remark
	 *	If the texture manager is not initialized, a call to this method is a no-op.
	 */
	static void uninitialize();

	/**
	 * @brief
	 *	Acquire a texture reference.
	 * @param ref
     *  if not equal to #INVALID_TX_REF, this texture reference is acquired
	 * @return
	 *	the texture reference on success, #INVALID_TX_REF on failure
	 */
	TX_REF acquire(const TX_REF ref);

	/**
	 * @brief
	 *	Relinquish texture reference.
	 * @param ref
	 *	the texture reference
	 */
	bool relinquish(const TX_REF ref);

	/**
	 * @brief
	 *	Reload all textures from their surfaces.
	 */
	void reload_all();
	/**
	 * @brief
	 *	Release all textures.
	 */
	void release_all();

	TX_REF load(const char *filename, const TX_REF ref, Uint32 key);
	oglx_texture_t *get_valid_ptr(const TX_REF ref);

};
