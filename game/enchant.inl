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

/// @file enchant.inl

#include "EncList.h"
#include "ChrList.h"

#include "char.inl"
#include "profile.inl"

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
CHR_REF ego_enc::get_iowner( const ENC_REF & ienc )
{
    ego_enc * penc;

    if ( !DEFINED_ENC( ienc ) ) return CHR_REF( MAX_CHR );
    penc = EncObjList.get_pdata( ienc );

    if ( !INGAME_CHR( penc->owner_ref ) ) return CHR_REF( MAX_CHR );

    return penc->owner_ref;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_enc::get_powner( const ENC_REF & ienc )
{
    ego_enc * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EncObjList.get_pdata( ienc );

    if ( !INGAME_CHR( penc->owner_ref ) ) return NULL;

    return ChrObjList.get_pdata( penc->owner_ref );
}

//--------------------------------------------------------------------------------------------
EVE_REF ego_enc::get_ieve( const ENC_REF & ienc )
{
    ego_enc * penc;

    if ( !DEFINED_ENC( ienc ) ) return EVE_REF( MAX_EVE );
    penc = EncObjList.get_pdata( ienc );

    if ( !LOADED_EVE( penc->eve_ref ) ) return EVE_REF( MAX_EVE );

    return penc->eve_ref;
}

//--------------------------------------------------------------------------------------------
ego_eve * ego_enc::get_peve( const ENC_REF & ienc )
{
    ego_enc * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EncObjList.get_pdata( ienc );

    if ( !LOADED_EVE( penc->eve_ref ) ) return NULL;

    return EveStack.lst + penc->eve_ref;
}

//--------------------------------------------------------------------------------------------
PRO_REF  ego_enc::get_ipro( const ENC_REF & ienc )
{
    ego_enc * penc;

    if ( !DEFINED_ENC( ienc ) ) return PRO_REF( MAX_PROFILE );
    penc = EncObjList.get_pdata( ienc );

    if ( !LOADED_PRO( penc->profile_ref ) ) return PRO_REF( MAX_PROFILE );

    return penc->profile_ref;
}

//--------------------------------------------------------------------------------------------
ego_pro * ego_enc::get_ppro( const ENC_REF & ienc )
{
    ego_enc * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EncObjList.get_pdata( ienc );

    if ( !LOADED_PRO( penc->profile_ref ) ) return NULL;

    return ProList.lst + penc->profile_ref;
}

//--------------------------------------------------------------------------------------------
IDSZ ego_enc::get_idszremove( const ENC_REF & ienc )
{
    ego_eve * peve = ego_enc::get_peve( ienc );
    if ( NULL == peve ) return IDSZ_NONE;

    return peve->removedbyidsz;
}

//--------------------------------------------------------------------------------------------
bool_t ego_enc::is_removed( const ENC_REF & ienc, const PRO_REF & test_profile )
{
    IDSZ idsz_remove;

    if ( !INGAME_ENC( ienc ) ) return bfalse;
    idsz_remove = ego_enc::get_idszremove( ienc );

    // if nothing can remove it, just go on with your business
    if ( IDSZ_NONE == idsz_remove ) return bfalse;

    // check vs. every IDSZ that could have something to do with cancelling the enchant
    if ( idsz_remove == pro_get_idsz( test_profile, IDSZ_TYPE ) ) return btrue;
    if ( idsz_remove == pro_get_idsz( test_profile, IDSZ_PARENT ) ) return btrue;

    return bfalse;
}

