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

/// @file char.inl
/// @note You will routinely include "char.inl" in all *.inl files or *.c/*.cpp files, instead of "char.h"

#include "char.h"

#include "enchant.inl"
#include "particle.inl"

/// @note include "profile.inl" here.
///  Do not include "char.inl" in "profile.inl", otherwise there is a bootstrapping problem.
#include "profile.inl"

#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARARIONS
//--------------------------------------------------------------------------------------------
// ego_cap accessor functions
INLINE bool_t cap_is_type_idsz( const CAP_REF by_reference icap, IDSZ test_idsz );
INLINE bool_t cap_has_idsz( const CAP_REF by_reference icap, IDSZ idsz );

//--------------------------------------------------------------------------------------------
// ego_team accessor functions
INLINE CHR_REF  team_get_ileader( const TEAM_REF by_reference iteam );
INLINE ego_chr  * team_get_pleader( const TEAM_REF by_reference iteam );

INLINE bool_t team_hates_team( const TEAM_REF by_reference ipredator_team, const TEAM_REF by_reference iprey_team );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
INLINE bool_t cap_is_type_idsz( const CAP_REF by_reference icap, IDSZ test_idsz )
{
    /// @details BB@> check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not
    ///     picky (i.e. IDSZ_NONE == test_idsz), then it matches any valid item.

    ego_cap * pcap;

    if ( !LOADED_CAP( icap ) ) return bfalse;
    pcap = CapStack.lst + icap;

    if ( IDSZ_NONE == test_idsz ) return btrue;
    if ( test_idsz == pcap->idsz[IDSZ_TYPE  ] ) return btrue;
    if ( test_idsz == pcap->idsz[IDSZ_PARENT] ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t cap_has_idsz( const CAP_REF by_reference icap, IDSZ idsz )
{
    /// @details BB@> does idsz match any of the stored values in pcap->idsz[]?
    ///               Matches anything if not picky (idsz == IDSZ_NONE)

    int     cnt;
    ego_cap * pcap;
    bool_t  retval;

    if ( !LOADED_CAP( icap ) ) return bfalse;
    pcap = CapStack.lst + icap;

    if ( IDSZ_NONE == idsz ) return btrue;

    retval = bfalse;
    for ( cnt = 0; cnt < IDSZ_COUNT; cnt++ )
    {
        if ( pcap->idsz[cnt] == idsz )
        {
            retval = btrue;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE CHR_REF team_get_ileader( const TEAM_REF by_reference iteam )
{
    CHR_REF ichr;

    if ( iteam >= TEAM_MAX ) return CHR_REF(MAX_CHR);

    ichr = TeamStack.lst[iteam].leader;
    if ( !DEFINED_CHR( ichr ) ) return CHR_REF(MAX_CHR);

    return ichr;
}

//--------------------------------------------------------------------------------------------
INLINE ego_chr  * team_get_pleader( const TEAM_REF by_reference iteam )
{
    CHR_REF ichr;

    if ( iteam >= TEAM_MAX ) return NULL;

    ichr = TeamStack.lst[iteam].leader;
    if ( !DEFINED_CHR( ichr ) ) return NULL;

    return ChrList.get_valid_ptr(ichr);
}

//--------------------------------------------------------------------------------------------
INLINE bool_t team_hates_team( const TEAM_REF by_reference ipredator_team, const TEAM_REF by_reference iprey_team )
{
    /// @details BB@> a wrapper function for access to the hatesteam data

    if ( ipredator_team >= TEAM_MAX || iprey_team >= TEAM_MAX ) return bfalse;

    return TeamStack.lst[ipredator_team].hatesteam[ REF_TO_INT( iprey_team )];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE PRO_REF ego_chr::get_ipro( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return ( PRO_REF )MAX_PROFILE;
    pchr = ChrList.get_valid_ptr(ichr);

    if ( !LOADED_PRO( pchr->profile_ref ) ) return ( PRO_REF )MAX_PROFILE;

    return pchr->profile_ref;
}

//--------------------------------------------------------------------------------------------
INLINE CAP_REF ego_chr::get_icap( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return ( CAP_REF )MAX_CAP;
    pchr = ChrList.get_valid_ptr(ichr);

    return pro_get_icap( pchr->profile_ref );
}

//--------------------------------------------------------------------------------------------
INLINE EVE_REF ego_chr::get_ieve( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return ( EVE_REF )MAX_EVE;
    pchr = ChrList.get_valid_ptr(ichr);

    return pro_get_ieve( pchr->profile_ref );
}

//--------------------------------------------------------------------------------------------
INLINE PIP_REF ego_chr::get_ipip( const CHR_REF by_reference ichr, int ipip )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return ( PIP_REF )MAX_PIP;
    pchr = ChrList.get_valid_ptr(ichr);

    return pro_get_ipip( pchr->profile_ref, ipip );
}

//--------------------------------------------------------------------------------------------
INLINE TEAM_REF ego_chr::get_iteam( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;
    int iteam;

    if ( !DEFINED_CHR( ichr ) ) return ( TEAM_REF )TEAM_DAMAGE;
    pchr = ChrList.get_valid_ptr(ichr);

    iteam = REF_TO_INT( pchr->team );
    iteam = CLIP( iteam, 0, TEAM_MAX );

    return ( TEAM_REF )iteam;
}

//--------------------------------------------------------------------------------------------
INLINE TEAM_REF ego_chr::get_iteam_base( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;
    int iteam;

    if ( !DEFINED_CHR( ichr ) ) return ( TEAM_REF )TEAM_MAX;
    pchr = ChrList.get_valid_ptr(ichr);

    iteam = REF_TO_INT( pchr->baseteam );
    iteam = CLIP( iteam, 0, TEAM_MAX );

    return ( TEAM_REF )iteam;
}

//--------------------------------------------------------------------------------------------
INLINE ego_pro * ego_chr::get_ppro( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.get_valid_ptr(ichr);

    if ( !LOADED_PRO( pchr->profile_ref ) ) return NULL;

    return ProList.lst + pchr->profile_ref;
}

//--------------------------------------------------------------------------------------------
INLINE ego_cap * ego_chr::get_pcap( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.get_valid_ptr(ichr);

    return pro_get_pcap( pchr->profile_ref );
}

//--------------------------------------------------------------------------------------------
INLINE ego_eve * ego_chr::get_peve( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.get_valid_ptr(ichr);

    return pro_get_peve( pchr->profile_ref );
}

//--------------------------------------------------------------------------------------------
INLINE ego_pip * ego_chr::get_ppip( const CHR_REF by_reference ichr, int ipip )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.get_valid_ptr(ichr);

    return pro_get_ppip( pchr->profile_ref, ipip );
}

//--------------------------------------------------------------------------------------------
INLINE Mix_Chunk * ego_chr::get_chunk( const CHR_REF by_reference ichr, int index )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.get_valid_ptr(ichr);

    return pro_get_chunk( pchr->profile_ref, index );
}

//--------------------------------------------------------------------------------------------
INLINE Mix_Chunk * ego_chr::get_chunk_ptr( ego_chr * pchr, int index )
{
    if ( !DEFINED_PCHR( pchr ) ) return NULL;

    return pro_get_chunk( pchr->profile_ref, index );
}

//--------------------------------------------------------------------------------------------
INLINE ego_team * ego_chr::get_pteam( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.get_valid_ptr(ichr);

    if ( pchr->team < 0 && pchr->team >= TEAM_MAX ) return NULL;

    return TeamStack.lst + pchr->team;
}

//--------------------------------------------------------------------------------------------
INLINE ego_team * ego_chr::get_pteam_base( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.get_valid_ptr(ichr);

    if ( pchr->baseteam < 0 || pchr->baseteam >= TEAM_MAX ) return NULL;

    return TeamStack.lst + pchr->baseteam;
}

//--------------------------------------------------------------------------------------------
INLINE ego_ai_state * ego_chr::get_pai( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.get_valid_ptr(ichr);

    return &( pchr->ai );
}

//--------------------------------------------------------------------------------------------
INLINE ego_chr_instance * ego_chr::get_pinstance( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.get_valid_ptr(ichr);

    return &( pchr->inst );
}

//--------------------------------------------------------------------------------------------
INLINE IDSZ ego_chr::get_idsz( const CHR_REF by_reference ichr, int type )
{
    ego_cap * pcap;

    if ( type >= IDSZ_COUNT ) return IDSZ_NONE;

    pcap = ego_chr::get_pcap( ichr );
    if ( NULL == pcap ) return IDSZ_NONE;

    return pcap->idsz[type];
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_chr::has_idsz( const CHR_REF by_reference ichr, IDSZ idsz )
{
    /// @details BB@> a wrapper for cap_has_idsz

    CAP_REF icap = ego_chr::get_icap( ichr );

    return cap_has_idsz( icap, idsz );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_chr::is_type_idsz( const CHR_REF by_reference item, IDSZ test_idsz )
{
    /// @details BB@> check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not
    ///     picky (i.e. IDSZ_NONE == test_idsz), then it matches any valid item.

    CAP_REF icap;

    icap = ego_chr::get_icap( item );

    return cap_is_type_idsz( icap, test_idsz );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_chr::has_vulnie( const CHR_REF by_reference item, const PRO_REF by_reference test_profile )
{
    /// @details BB@> is item vulnerable to the type in profile test_profile?

    IDSZ vulnie;

    if ( !INGAME_CHR( item ) ) return bfalse;
    vulnie = ego_chr::get_idsz( item, IDSZ_VULNERABILITY );

    // not vulnerable if there is no specific weakness
    if ( IDSZ_NONE == vulnie ) return bfalse;

    // check vs. every IDSZ that could have something to do with attacking
    if ( vulnie == pro_get_idsz( test_profile, IDSZ_TYPE ) ) return btrue;
    if ( vulnie == pro_get_idsz( test_profile, IDSZ_PARENT ) ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_chr::get_MatUp( ego_chr *pchr, fvec3_t   *pvec )
{
    /// @details BB@> MAKE SURE the value it calculated relative to a valid matrix

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    if ( NULL == pvec ) return bfalse;

    if ( !ego_chr::matrix_valid( pchr ) )
    {
        ego_chr::update_matrix( pchr, btrue );
    }

    if ( ego_chr::matrix_valid( pchr ) )
    {
        ( *pvec ) = mat_getChrUp( pchr->inst.matrix );
    }
    else
    {
        ( *pvec ).x = ( *pvec ).y = 0.0f;
        ( *pvec ).z = 1.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_chr::get_MatRight( ego_chr *pchr, fvec3_t   *pvec )
{
    /// @details BB@> MAKE SURE the value it calculated relative to a valid matrix

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    if ( NULL == pvec ) return bfalse;

    if ( !ego_chr::matrix_valid( pchr ) )
    {
        ego_chr::update_matrix( pchr, btrue );
    }

    if ( ego_chr::matrix_valid( pchr ) )
    {
        ( *pvec ) = mat_getChrForward( pchr->inst.matrix );
    }
    else
    {
        // assume default Right is +y
        ( *pvec ).y = 1.0f;
        ( *pvec ).x = ( *pvec ).z = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_chr::get_MatForward( ego_chr *pchr, fvec3_t   *pvec )
{
    /// @details BB@> MAKE SURE the value it calculated relative to a valid matrix

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    if ( NULL == pvec ) return bfalse;

    if ( !ego_chr::matrix_valid( pchr ) )
    {
        ego_chr::update_matrix( pchr, btrue );
    }

    if ( ego_chr::matrix_valid( pchr ) )
    {
        ( *pvec ) = mat_getChrRight( pchr->inst.matrix );
    }
    else
    {
        // assume default Forward is +x
        ( *pvec ).x = 1.0f;
        ( *pvec ).y = ( *pvec ).z = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_chr::get_MatTranslate( ego_chr *pchr, fvec3_t   *pvec )
{
    /// @details BB@> MAKE SURE the value it calculated relative to a valid matrix

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    if ( NULL == pvec ) return bfalse;

    if ( !ego_chr::matrix_valid( pchr ) )
    {
        ego_chr::update_matrix( pchr, btrue );
    }

    if ( ego_chr::matrix_valid( pchr ) )
    {
        ( *pvec ) = mat_getTranslate( pchr->inst.matrix );
    }
    else
    {
        ( *pvec ) = ego_chr::get_pos( pchr );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE void ego_chr::update_size( ego_chr * pchr )
{
    /// @details BB@> Convert the base size values to the size values that are used in the game

    if ( !VALID_PCHR( pchr ) ) return;

    pchr->shadow_size   = pchr->shadow_size_save   * pchr->fat;
    pchr->bump.size     = pchr->bump_save.size     * pchr->fat;
    pchr->bump.size_big = pchr->bump_save.size_big * pchr->fat;
    pchr->bump.height   = pchr->bump_save.height   * pchr->fat;

    ego_chr::update_collision_size( pchr, btrue );
}

//--------------------------------------------------------------------------------------------
INLINE void ego_chr::init_size( ego_chr * pchr, ego_cap * pcap )
{
    /// @details BB@> initialize the character size info

    if ( !VALID_PCHR( pchr ) ) return;
    if ( NULL == pcap || !pcap->loaded ) return;

    pchr->fat_stt           = pcap->size;
    pchr->shadow_size_stt   = pcap->shadow_size;
    pchr->bump_stt.size     = ( pcap->bump_override_size    ? 1.0f : -1.0f ) * pcap->bump_size;
    pchr->bump_stt.size_big = ( pcap->bump_override_sizebig ? 1.0f : -1.0f ) * pcap->bump_sizebig;
    pchr->bump_stt.height   = ( pcap->bump_override_height  ? 1.0f : -1.0f ) * pcap->bump_height;

    pchr->fat                = pchr->fat_stt;
    pchr->shadow_size_save   = pchr->shadow_size_stt;
    pchr->bump_save.size     = pchr->bump_stt.size;
    pchr->bump_save.size_big = pchr->bump_stt.size_big;
    pchr->bump_save.height   = pchr->bump_stt.height;

    ego_chr::update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE void ego_chr::set_size( ego_chr * pchr, float size )
{
    /// @details BB@> scale the entire character so that the size matches the given value

    float ratio;

    if ( !DEFINED_PCHR( pchr ) ) return;

    ratio = size / pchr->bump.size;

    pchr->shadow_size_save   *= ratio;
    pchr->bump_save.size     *= ratio;
    pchr->bump_save.size_big *= ratio;
    pchr->bump_save.height   *= ratio;

    ego_chr::update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE void ego_chr::set_width( ego_chr * pchr, float width )
{
    /// @details BB@> update the base character "width". This also modifies the shadow size

    float ratio;

    if ( !DEFINED_PCHR( pchr ) ) return;

    ratio = width / pchr->bump.size;

    pchr->shadow_size_stt    *= ratio;
    pchr->bump_stt.size    *= ratio;
    pchr->bump_stt.size_big *= ratio;

    ego_chr::update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE void ego_chr::set_shadow( ego_chr * pchr, float width )
{
    /// @details BB@> update the base shadow size

    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->shadow_size_stt = width;

    ego_chr::update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE void ego_chr::set_fat( ego_chr * pchr, float fat )
{
    /// @details BB@> update all the character size info by specifying the fat value

    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->fat = fat;

    ego_chr::update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE void ego_chr::set_height( ego_chr * pchr, float height )
{
    /// @details BB@> update the base character height

    if ( !DEFINED_PCHR( pchr ) ) return;

    if ( height < 0 ) height = 0;

    pchr->bump_save.height = height;

    ego_chr::update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE latch_2d_t ego_chr::convert_latch_2d( const ego_chr * pchr, const latch_2d_t by_reference src )
{
    latch_2d_t dst = src;

    if ( !DEFINED_PCHR( pchr ) ) return dst;

    // Reverse movements for daze
    if ( pchr->dazetime > 0 )
    {
        dst.dir[kX] = -dst.dir[kX];
        dst.dir[kY] = -dst.dir[kY];
    }

    // Switch x and y for grog
    if ( pchr->grogtime > 0 )
    {
        SWAP( float, dst.dir[kX], dst.dir[kY] );
    }

    return dst;
}
