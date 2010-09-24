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

/// @file enchant.c
/// @brief handles enchantments attached to objects
/// @details

#include "enchant.inl"

#include "mad.h"

#include "sound.h"
#include "camera.h"
#include "game.h"
#include "script_functions.h"
#include "log.h"
#include "network.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

#include "char.inl"
#include "particle.inl"
#include "profile.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INSTANTIATE_STACK( ACCESS_TYPE_NONE, eve_t, EveStack, MAX_EVE );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t  enc_free( enc_t * penc );

static enc_t * enc_config_ctor( enc_t * penc );
static enc_t * enc_config_init( enc_t * penc );
static enc_t * enc_config_deinit( enc_t * penc );
static enc_t * enc_config_active( enc_t * penc );
static enc_t * enc_config_dtor( enc_t * penc );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void enchant_system_begin()
{
    EncList_init();
    init_all_eve();
}

//--------------------------------------------------------------------------------------------
void enchant_system_end()
{
    release_all_eve();
    EncList_dtor();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t  enc_free( enc_t * penc )
{
    if ( !ALLOCATED_PENC( penc ) ) return bfalse;

    // nothing to do yet

    return btrue;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_ctor( enc_t * penc )
{
    ego_object_base_t save_base;
    ego_object_base_t * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase ) return NULL;

    memcpy( &save_base, pbase, sizeof( ego_object_base_t ) );

    memset( penc, 0, sizeof( *penc ) );

    // restore the base object data
    memcpy( pbase, &save_base, sizeof( ego_object_base_t ) );

    penc->profile_ref      = ( PRO_REF )MAX_PROFILE;
    penc->eve_ref          = ( EVE_REF )MAX_EVE;

    penc->target_ref       = ( CHR_REF )MAX_CHR;
    penc->owner_ref        = ( CHR_REF )MAX_CHR;
    penc->spawner_ref      = ( CHR_REF )MAX_CHR;
    penc->spawnermodel_ref = ( PRO_REF )MAX_PROFILE;
    penc->overlay_ref      = ( CHR_REF )MAX_CHR;

    penc->nextenchant_ref  = ( ENC_REF )MAX_ENC;

    // we are done constructing. move on to initializing.
    pbase->state = ego_object_initializing;

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_dtor( enc_t * penc )
{
    if( NULL == penc ) return penc;

    // destroy the object
    enc_free( penc );

    // Destroy the base object.
    // Sets the state to ego_object_terminated automatically.
    POBJ_TERMINATE( penc );

    return penc;
}

//--------------------------------------------------------------------------------------------
#if defined(__cplusplus)
s_enc::s_enc() { enc_ctor(this) };
s_enc::~s_enc() { enc_dtor( this ); };
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t unlink_enchant( const ENC_REF by_reference ienc, ENC_REF * enc_parent )
{
    enc_t * penc;

    if ( !ALLOCATED_ENC( ienc ) ) return bfalse;
    penc = EncList.lst + ienc;

    // Unlink it from the spawner (if possible)
    if ( ALLOCATED_CHR( penc->spawner_ref ) )
    {
        chr_t * pspawner = ChrList.lst + penc->spawner_ref;

        if ( ienc == pspawner->undoenchant )
        {
            pspawner->undoenchant = (ENC_REF) MAX_ENC;
        }
    }

    // find the parent reference for the enchant
    if( NULL == enc_parent && ALLOCATED_CHR( penc->target_ref ) )
    {
        ENC_REF ienc_last, ienc_now;
        chr_t * ptarget;

        ptarget =  ChrList.lst + penc->target_ref;

        if ( ptarget->firstenchant == ienc )
        {
            // It was the first in the list
            enc_parent = &(ptarget->firstenchant);
        }
        else
        {
            // Search until we find it
            ienc_last = ienc_now = ptarget->firstenchant;
            while ( MAX_ENC != ienc_now && ienc_now != ienc )
            {
                ienc_last    = ienc_now;
                ienc_now = EncList.lst[ienc_now].nextenchant_ref;
            }

            // Relink the last enchantment
            if ( ienc_now == ienc )
            {
                enc_parent = &(EncList.lst[ienc_last].nextenchant_ref);
            }
        }
    }

    // unlink the enchant from the parent reference
    if( NULL != enc_parent )
    {
        *enc_parent = EncList.lst[ienc].nextenchant_ref;
    }

    return NULL != enc_parent;
}

//--------------------------------------------------------------------------------------------
bool_t remove_all_enchants_with_idsz( CHR_REF ichr, IDSZ remove_idsz )
{
    /// @details ZF@> This function removes all enchants with the character that has the specified
    ///               IDSZ. If idsz [NONE] is specified, all enchants will be removed. Return btrue
    ///               if at least one enchant was removed.

    ENC_REF enc_now, enc_next;
    eve_t * peve;
    bool_t retval = bfalse;
    chr_t *pchr;

    // Stop invalid pointers
    if( !ACTIVE_CHR(ichr) ) return bfalse;
    pchr = ChrList.lst + ichr;

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pchr );

    // Check all enchants to see if they are removed
    enc_now = pchr->firstenchant;
    while ( enc_now != MAX_ENC )
    {
        enc_next  = EncList.lst[enc_now].nextenchant_ref;

        peve = enc_get_peve( enc_now );
        if ( NULL != peve && ( IDSZ_NONE == remove_idsz || remove_idsz == peve->removedbyidsz ) )
        {
            remove_enchant( enc_now, NULL );
            retval = btrue;
        }

        enc_now = enc_next;
    }
    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t remove_enchant( const ENC_REF by_reference ienc, ENC_REF * enc_parent )
{
    /// @details ZZ@> This function removes a specific enchantment and adds it to the unused list

    int     iwave;
    CHR_REF overlay_ref;
    int add_type, set_type;

    enc_t * penc;
    eve_t * peve;
    CHR_REF itarget, ispawner;

    if ( !ALLOCATED_ENC( ienc ) ) return bfalse;
    penc = EncList.lst + ienc;

    itarget  = penc->target_ref;
    ispawner = penc->spawner_ref;
    peve     = enc_get_peve( ienc );

    // Unsparkle the spellbook
    if ( INGAME_CHR( ispawner ) )
    {
        chr_t * pspawner = ChrList.lst + ispawner;

        pspawner->sparkle = NOSPARKLE;

        // Make the spawner unable to undo the enchantment
        if ( pspawner->undoenchant == ienc )
        {
            pspawner->undoenchant = (ENC_REF) MAX_ENC;
        }
    }

    //---- Remove all the enchant stuff in exactly the opposite order to how it was applied

    // Remove all of the cumulative values first, since we did it
    for ( add_type = ENC_ADD_LAST; add_type >= ENC_ADD_FIRST; add_type-- )
    {
        enc_remove_add( ienc, add_type );
    }

    // unset them in the reverse order of setting them, doing morph last
    for ( set_type = ENC_SET_LAST; set_type >= ENC_SET_FIRST; set_type-- )
    {
        enc_remove_set( ienc, set_type );
    }

    // Now fix dem weapons
    if ( INGAME_CHR( itarget ) )
    {
        chr_t * ptarget = ChrList.lst + itarget;
        reset_character_alpha( ptarget->holdingwhich[SLOT_LEFT] );
        reset_character_alpha( ptarget->holdingwhich[SLOT_RIGHT] );
    }

    // unlink this enchant from its parent
    unlink_enchant( ienc, enc_parent );

    // Kill overlay too...
    overlay_ref = penc->overlay_ref;
    if ( INGAME_CHR( overlay_ref ) )
    {
        chr_t * povl = ChrList.lst + overlay_ref;

        if ( IS_INVICTUS_PCHR_RAW( povl ) )
        {
            chr_get_pteam_base( overlay_ref )->morale++;
        }

        kill_character( overlay_ref, ( CHR_REF )MAX_CHR, btrue );
    }

    // nothing above this depends on having a valid enchant profile
    if ( NULL != peve )
    {
        // Play the end sound
        iwave = peve->endsound_index;
        if ( VALID_SND( iwave ) )
        {
            PRO_REF imodel = penc->spawnermodel_ref;
            if ( LOADED_PRO( imodel ) )
            {
                if ( INGAME_CHR( itarget ) )
                {
                    sound_play_chunk( ChrList.lst[itarget].pos_old, pro_get_chunk( imodel, iwave ) );
                }
                else
                {
                    sound_play_chunk_full( pro_get_chunk( imodel, iwave ) );
                }
            }
        }

        // See if we spit out an end message
        if ( peve->endmessage >= 0 )
        {
            _display_message( penc->target_ref, penc->profile_ref, peve->endmessage, NULL );
        }

        // Check to see if we spawn a poof
        if ( peve->poofonend )
        {
            spawn_poof( penc->target_ref, penc->profile_ref );
        }

        // Remove see kurse enchant
        if ( INGAME_CHR( itarget ) )
        {
            chr_t * ptarget = ChrList.lst + penc->target_ref;

            if ( peve->seekurse && !chr_get_skill( ptarget, MAKE_IDSZ( 'C', 'K', 'U', 'R' ) ) )
            {
                ptarget->see_kurse_level = bfalse;
            }
        }
    }

    EncList_free_one( ienc );

    // save this until the enchant is completely dead, since kill character can generate a
    // recursive call to this function through cleanup_one_character()
    // @note all of the values in the penc are now invalid. we have to use previously evaluated
    // values of itarget and penc to kill the target (if necessary)
    if ( INGAME_CHR( itarget ) && NULL != peve && peve->killtargetonend )
    {
        chr_t * ptarget = ChrList.lst + itarget;

        if ( IS_INVICTUS_PCHR_RAW( ptarget ) )
        {
            chr_get_pteam_base( itarget )->morale++;
        }

        kill_character( itarget, ( CHR_REF )MAX_CHR, btrue );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ENC_REF enc_value_filled( const ENC_REF by_reference  ienc, int value_idx )
{
    /// @details ZZ@> This function returns MAX_ENC if the enchantment's target has no conflicting
    ///    set values in its other enchantments.  Otherwise it returns the ienc
    ///    of the conflicting enchantment

    CHR_REF character;
    ENC_REF currenchant;
    chr_t * pchr;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return ( ENC_REF )MAX_ENC;

    if ( !INGAME_ENC( ienc ) ) return ( ENC_REF )MAX_ENC;

    character = EncList.lst[ienc].target_ref;
    if( !INGAME_CHR(character) ) return ( ENC_REF )MAX_ENC;
    pchr = ChrList.lst + character;

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // scan the enchant list
    currenchant = pchr->firstenchant;
    while ( currenchant != MAX_ENC )
    {
        if ( INGAME_ENC( currenchant ) && EncList.lst[currenchant].setyesno[value_idx] )
        {
            break;
        }

        currenchant = EncList.lst[currenchant].nextenchant_ref;
    }

    return currenchant;
}

//--------------------------------------------------------------------------------------------
void enc_apply_set( const ENC_REF by_reference  ienc, int value_idx, const PRO_REF by_reference profile )
{
    /// @details ZZ@> This function sets and saves one of the character's stats

    ENC_REF conflict;
    CHR_REF character;
    enc_t * penc;
    eve_t * peve;
    chr_t * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return;

    if ( !DEFINED_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    peve = pro_get_peve( profile );
    if ( NULL == peve ) return;

    penc->setyesno[value_idx] = bfalse;
    if ( peve->setyesno[value_idx] )
    {
        conflict = enc_value_filled( ienc, value_idx );
        if ( peve->override || MAX_ENC == conflict )
        {
            // Check for multiple enchantments
            if ( DEFINED_ENC( conflict ) )
            {
                // Multiple enchantments aren't allowed for sets
                if ( peve->removeoverridden )
                {
                    // Kill the old enchantment
                    remove_enchant( conflict, NULL );
                }
                else
                {
                    // Just unset the old enchantment's value
                    enc_remove_set( conflict, value_idx );
                }
            }

            // Set the value, and save the character's real stat
            if ( DEFINED_CHR( penc->target_ref ) )
            {
                character = penc->target_ref;
                ptarget = ChrList.lst + character;

                penc->setyesno[value_idx] = btrue;

                switch ( value_idx )
                {
                    case SETDAMAGETYPE:
                        penc->setsave[value_idx]  = ptarget->damagetargettype;
                        ptarget->damagetargettype = peve->setvalue[value_idx];
                        break;

                    case SETNUMBEROFJUMPS:
                        penc->setsave[value_idx] = ptarget->jump_number_reset;
                        chr_set_jump_number_reset( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETLIFEBARCOLOR:
                        penc->setsave[value_idx] = ptarget->lifecolor;
                        ptarget->lifecolor       = peve->setvalue[value_idx];
                        break;

                    case SETMANABARCOLOR:
                        penc->setsave[value_idx] = ptarget->manacolor;
                        ptarget->manacolor       = peve->setvalue[value_idx];
                        break;

                    case SETSLASHMODIFIER:
                        penc->setsave[value_idx]              = ptarget->damagemodifier[DAMAGE_SLASH];
                        ptarget->damagemodifier[DAMAGE_SLASH] = peve->setvalue[value_idx];
                        break;

                    case SETCRUSHMODIFIER:
                        penc->setsave[value_idx]              = ptarget->damagemodifier[DAMAGE_CRUSH];
                        ptarget->damagemodifier[DAMAGE_CRUSH] = peve->setvalue[value_idx];
                        break;

                    case SETPOKEMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damagemodifier[DAMAGE_POKE];
                        ptarget->damagemodifier[DAMAGE_POKE] = peve->setvalue[value_idx];
                        break;

                    case SETHOLYMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damagemodifier[DAMAGE_HOLY];
                        ptarget->damagemodifier[DAMAGE_HOLY] = peve->setvalue[value_idx];
                        break;

                    case SETEVILMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damagemodifier[DAMAGE_EVIL];
                        ptarget->damagemodifier[DAMAGE_EVIL] = peve->setvalue[value_idx];
                        break;

                    case SETFIREMODIFIER:
                        penc->setsave[value_idx]             = ptarget->damagemodifier[DAMAGE_FIRE];
                        ptarget->damagemodifier[DAMAGE_FIRE] = peve->setvalue[value_idx];
                        break;

                    case SETICEMODIFIER:
                        penc->setsave[value_idx]            = ptarget->damagemodifier[DAMAGE_ICE];
                        ptarget->damagemodifier[DAMAGE_ICE] = peve->setvalue[value_idx];
                        break;

                    case SETZAPMODIFIER:
                        penc->setsave[value_idx]            = ptarget->damagemodifier[DAMAGE_ZAP];
                        ptarget->damagemodifier[DAMAGE_ZAP] = peve->setvalue[value_idx];
                        break;

                    case SETFLASHINGAND:
                        penc->setsave[value_idx] = ptarget->flashand;
                        ptarget->flashand        = peve->setvalue[value_idx];
                        break;

                    case SETLIGHTBLEND:
                        penc->setsave[value_idx] = ptarget->inst.light;
                        chr_set_light( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETALPHABLEND:
                        penc->setsave[value_idx] = ptarget->inst.alpha;
                        chr_set_alpha( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETSHEEN:
                        penc->setsave[value_idx] = ptarget->inst.sheen;
                        chr_set_sheen( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETFLYTOHEIGHT:
                        penc->setsave[value_idx] = ptarget->fly_height;
                        if ( ptarget->pos.z > -2.0f )
                        {
                            chr_set_fly_height( ptarget, peve->setvalue[value_idx] );
                        }
                        break;

                    case SETWALKONWATER:
                        penc->setsave[value_idx] = ptarget->waterwalk;
                        if ( !ptarget->waterwalk )
                        {
                            ptarget->waterwalk = ( 0 != peve->setvalue[value_idx] );
                        }
                        break;

                    case SETCANSEEINVISIBLE:
                        penc->setsave[value_idx]     = ptarget->see_invisible_level > 0;
                        ptarget->see_invisible_level = peve->setvalue[value_idx];
                        break;

                    case SETMISSILETREATMENT:
                        penc->setsave[value_idx]  = ptarget->missiletreatment;
                        ptarget->missiletreatment = peve->setvalue[value_idx];
                        break;

                    case SETCOSTFOREACHMISSILE:
                        penc->setsave[value_idx] = ptarget->missilecost;
                        ptarget->missilecost     = peve->setvalue[value_idx] * 16.0f;    // adjustment to the value stored in the file
                        ptarget->missilehandler  = penc->owner_ref;
                        break;

                    case SETMORPH:
                        // Special handler for morph
                        penc->setsave[value_idx] = ptarget->skin;
                        change_character( character, profile, 0, ENC_LEAVE_ALL ); // ENC_LEAVE_FIRST);
                        break;

                    case SETCHANNEL:
                        penc->setsave[value_idx] = ptarget->canchannel;
                        ptarget->canchannel      = ( 0 != peve->setvalue[value_idx] );
                        break;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void enc_apply_add( const ENC_REF by_reference ienc, int value_idx, const EVE_REF by_reference ieve )
{
    /// @details ZZ@> This function does cumulative modification to character stats

    int valuetoadd, newvalue;
    float fvaluetoadd, fnewvalue;
    CHR_REF character;
    enc_t * penc;
    eve_t * peve;
    chr_t * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_ADD ) return;

    if ( !DEFINED_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if ( ieve >= MAX_EVE || !EveStack.lst[ieve].loaded ) return;
    peve = EveStack.lst + ieve;

    if ( !peve->addyesno[value_idx] )
    {
        penc->addyesno[value_idx] = bfalse;
        penc->addsave[value_idx]  = 0.0f;
        return;
    }

    if ( !DEFINED_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = ChrList.lst + character;

    valuetoadd  = 0;
    fvaluetoadd = 0.0f;
    switch ( value_idx )
    {
        case ADDJUMPPOWER:
            fnewvalue = ptarget->jump_power;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.0f, fnewvalue, 30.0f, &fvaluetoadd );
            ptarget->jump_power += fvaluetoadd;
            break;

        case ADDBUMPDAMPEN:
            fnewvalue = ptarget->phys.bumpdampen;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.0f, fnewvalue, 1.0f, &fvaluetoadd );
            ptarget->phys.bumpdampen += fvaluetoadd;
            break;

        case ADDBOUNCINESS:
            fnewvalue = ptarget->phys.dampen;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.0f, fnewvalue, 0.95f, &fvaluetoadd );
            ptarget->phys.dampen += fvaluetoadd;
            break;

        case ADDDAMAGE:
            newvalue = ptarget->damageboost;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 4096, &valuetoadd );
            ptarget->damageboost += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDSIZE:
            fnewvalue = ptarget->fat_goto;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
            ptarget->fat_goto += fvaluetoadd;
            ptarget->fat_goto_time = SIZETIME;
            break;

        case ADDACCEL:
            fnewvalue = ptarget->maxaccel_reset;
            fvaluetoadd = peve->addvalue[value_idx];
            fgetadd( 0.0f, fnewvalue, 1.50f, &fvaluetoadd );
            chr_set_maxaccel( ptarget, ptarget->maxaccel_reset + fvaluetoadd );
            break;

        case ADDRED:
            newvalue = ptarget->inst.redshift;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr_set_redshift( ptarget, ptarget->inst.redshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDGRN:
            newvalue = ptarget->inst.grnshift;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr_set_grnshift( ptarget, ptarget->inst.grnshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDBLU:
            newvalue = ptarget->inst.blushift;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 6, &valuetoadd );
            chr_set_blushift( ptarget, ptarget->inst.blushift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDDEFENSE:
            newvalue = ptarget->defense;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 55, newvalue, 255, &valuetoadd );  // Don't fix again!  //ZF> why limit min to 55?
            ptarget->defense += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDMANA:
            newvalue = ptarget->manamax;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->manamax += valuetoadd;
            //ptarget->mana    += valuetoadd;                        //ZF> bit of a problem here, we don't want players to heal or lose life by requipping magic ornaments
            ptarget->mana = CLIP(ptarget->mana, 0, ptarget->manamax);
            fvaluetoadd = valuetoadd;
            break;

        case ADDLIFE:
            newvalue = ptarget->lifemax;
            valuetoadd = peve->addvalue[value_idx];
            getadd( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->lifemax += valuetoadd;
            //ptarget->life += valuetoadd;                        //ZF> bit of a problem here, we don't want players to heal or lose life by requipping magic ornaments
            ptarget->life = CLIP(ptarget->life, 1, ptarget->lifemax);
            fvaluetoadd = valuetoadd;
            break;

        case ADDSTRENGTH:
            newvalue = ptarget->strength;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->strength += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDWISDOM:
            newvalue = ptarget->wisdom;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->wisdom += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDINTELLIGENCE:
            newvalue = ptarget->intelligence;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->intelligence += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDDEXTERITY:
            newvalue = ptarget->dexterity;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
            ptarget->dexterity += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;
    }

    // save whether there was any change in the value
    penc->addyesno[value_idx] = ( 0.0f != fvaluetoadd );

    // Save the value for undo
    penc->addsave[value_idx]  = fvaluetoadd;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enc_t * enc_config_do_init( enc_t * penc )
{
    enc_spawn_data_t * pdata;
    ENC_REF ienc;
    CHR_REF overlay;

    eve_t * peve;
    chr_t * ptarget;

    int add_type, set_type;

    if ( NULL == penc ) return NULL;
    ienc  = GET_INDEX_PENC( penc );

    // get the profile data
    pdata = &( penc->spawn_data );

    // store the profile
    penc->profile_ref  = pdata->profile_ref;

    // Convert from local pdata->eve_ref to global enchant profile
    if ( !LOADED_EVE( pdata->eve_ref ) )
    {
        log_debug( "spawn_one_enchant() - cannot spawn enchant with invalid enchant template (\"eve\") == %d\n", REF_TO_INT( pdata->eve_ref ) );

        return NULL;
    }
    penc->eve_ref = pdata->eve_ref;
    peve = EveStack.lst + pdata->eve_ref;

    // turn the enchant on here. you can't fail to spawn after this point.
    POBJ_ACTIVATE( penc, peve->name );

    // does the target exist?
    if( !DEFINED_CHR( pdata->target_ref ) )
    {
        penc->target_ref   = ( CHR_REF )MAX_CHR;
        ptarget            = NULL;
    }
    else
    {
        penc->target_ref = pdata->target_ref;
        ptarget = ChrList.lst + penc->target_ref;
    }
    penc->target_mana  = peve->target_mana;
    penc->target_life  = peve->target_life;

    // does the owner exist?
    if( !DEFINED_CHR( pdata->owner_ref ) )
    {
        penc->owner_ref = ( CHR_REF )MAX_CHR;
    }
    else
    {
        penc->owner_ref  = pdata->owner_ref;
    }
    penc->owner_mana = peve->owner_mana;
    penc->owner_life = peve->owner_life;

    // does the spawner exist?
    if( !DEFINED_CHR( pdata->spawner_ref ) )
    {
        penc->spawner_ref      = ( CHR_REF )MAX_CHR;
        penc->spawnermodel_ref = ( PRO_REF )MAX_PROFILE;
    }
    else
    {
        penc->spawner_ref = pdata->spawner_ref;
        penc->spawnermodel_ref = chr_get_ipro( pdata->spawner_ref );

        ChrList.lst[penc->spawner_ref].undoenchant = ienc;
    }

    // set some other spawning parameters
    penc->time         = peve->time;
    penc->spawntime    = 1;

    // Now set all of the specific values, morph first
    for ( set_type = ENC_SET_FIRST; set_type <= ENC_SET_LAST; set_type++ )
    {
        enc_apply_set( ienc, set_type, pdata->profile_ref );
    }

    // Now do all of the stat adds
    for ( add_type = ENC_ADD_FIRST; add_type <= ENC_ADD_LAST; add_type++ )
    {
        enc_apply_add( ienc, add_type, pdata->eve_ref );
    }

    // Add it as first in the list
    if( NULL != ptarget )
    {
        penc->nextenchant_ref = ptarget->firstenchant;
        ptarget->firstenchant = ienc;
    }

    // Create an overlay character?
    if ( peve->spawn_overlay && NULL != ptarget )
    {
        overlay = spawn_one_character( ptarget->pos, pdata->profile_ref, ptarget->team, 0, ptarget->ori.facing_z, NULL, ( CHR_REF )MAX_CHR );
        if ( DEFINED_CHR( overlay ) )
        {
            chr_t * povl;
            mad_t * povl_mad;
            int action;

            povl     = ChrList.lst + overlay;
            povl_mad = chr_get_pmad( overlay );

            penc->overlay_ref = overlay;  // Kill this character on end...
            povl->ai.target   = pdata->target_ref;
            povl->ai.state    = peve->spawn_overlay;    // ??? WHY DO THIS ???
            povl->is_overlay  = btrue;

            // Start out with ActionMJ...  Object activated
            action = mad_get_action( chr_get_imad( overlay ), ACTION_MJ );
            if ( !ACTION_IS_TYPE( action, D ) )
            {
                chr_start_anim( povl, action, bfalse, btrue );
            }

            // Assume it's transparent...
            chr_set_light( povl, 254 );
            chr_set_alpha( povl,   0 );
        }
    }

    // Allow them to see kurses?
    if ( peve->seekurse && NULL != ptarget )
    {
        ptarget->see_kurse_level = btrue;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_do_active( enc_t * penc )
{
    /// @details ZZ@> This function allows enchantments to update, spawn particles,
    //  do drains, stat boosts and despawn.

    ENC_REF  ienc;
    CHR_REF  owner, target;
    EVE_REF  eve;
    eve_t * peve;
    chr_t * ptarget;

    if( NULL == penc ) return penc;
    ienc = GET_REF_PENC( penc );

    // the following functions should not be done the first time through the update loop
    if ( 0 == clock_wld ) return penc;

    peve = enc_get_peve( ienc );
    if ( NULL == peve ) return penc;

    // check to see whether the enchant needs to spawn some particles
    if ( penc->spawntime > 0 ) penc->spawntime--;
    if ( penc->spawntime == 0 && peve->contspawn_amount <= 0 && INGAME_CHR( penc->target_ref ) )
    {
        int      tnc;
        FACING_T facing;
        penc->spawntime = peve->contspawn_delay;
        ptarget = ChrList.lst + penc->target_ref;

        facing = ptarget->ori.facing_z;
        for ( tnc = 0; tnc < peve->contspawn_amount; tnc++ )
        {
            spawn_one_particle( ptarget->pos, facing, penc->profile_ref, peve->contspawn_pip,
                ( CHR_REF )MAX_CHR, GRIP_LAST, chr_get_iteam( penc->owner_ref ), penc->owner_ref, ( PRT_REF )TOTAL_MAX_PRT, tnc, ( CHR_REF )MAX_CHR );

            facing += peve->contspawn_facingadd;
        }
    }

    // Do enchant drains and regeneration
    if ( clock_enc_stat >= ONESECOND )
    {
        if ( 0 == penc->time )
        {
            enc_request_terminate( ienc );
        }
        else
        {
            // Do enchant timer
            if ( penc->time > 0 ) penc->time--;

            // To make life easier
            owner  = enc_get_iowner( ienc );
            target = penc->target_ref;
            eve    = enc_get_ieve( ienc );

            // Do drains
            if ( ChrList.lst[owner].alive )
            {

                // Change life
                if( penc->owner_life != 0 )
                {
                    ChrList.lst[owner].life += penc->owner_life;
                    if ( ChrList.lst[owner].life <= 0 )
                    {
                        kill_character( owner, target, bfalse );
                    }
                    if ( ChrList.lst[owner].life > ChrList.lst[owner].lifemax )
                    {
                        ChrList.lst[owner].life = ChrList.lst[owner].lifemax;
                    }
                }

                // Change mana
                if( penc->owner_mana != 0 )
                {
                    bool_t mana_paid = cost_mana( owner, -penc->owner_mana, target );
                    if ( EveStack.lst[eve].endifcantpay && !mana_paid )
                    {
                        enc_request_terminate( ienc );
                    }
                }

            }
            else if ( !EveStack.lst[eve].stayifnoowner )
            {
                enc_request_terminate( ienc );
            }

            // the enchant could have been inactivated by the stuff above
            // check it again
            if ( INGAME_ENC( ienc ) )
            {
                if ( ChrList.lst[target].alive )
                {

                    // Change life
                    if( penc->target_life != 0 )
                    {
                        ChrList.lst[target].life += penc->target_life;
                        if ( ChrList.lst[target].life <= 0 )
                        {
                            kill_character( target, owner, bfalse );
                        }
                        if ( ChrList.lst[target].life > ChrList.lst[target].lifemax )
                        {
                            ChrList.lst[target].life = ChrList.lst[target].lifemax;
                        }
                    }

                    // Change mana
                    if( penc->target_mana != 0 )
                    {
                        bool_t mana_paid = cost_mana( target, -penc->target_mana, owner );
                        if ( EveStack.lst[eve].endifcantpay && !mana_paid )
                        {
                            enc_request_terminate( ienc );
                        }
                    }

                }
                else if ( !EveStack.lst[eve].stayiftargetdead )
                {
                    enc_request_terminate( ienc );
                }
            }
        }
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enc_t * enc_config_construct( enc_t * pprt, int max_iterations )
{
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( ego_object_constructing + 1 ) )
    {
        enc_t * tmp_enc = enc_config_deconstruct( pprt, max_iterations );
        if ( tmp_enc == pprt ) return NULL;
    }

    iterations = 0;
    while ( NULL != pprt && pbase->state <= ego_object_constructing && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( pprt );
        if ( ptmp != pprt ) return NULL;
        iterations++;
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_initialize( enc_t * pprt, int max_iterations )
{
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( ego_object_initializing + 1 ) )
    {
        enc_t * tmp_enc = enc_config_deconstruct( pprt, max_iterations );
        if ( tmp_enc == pprt ) return NULL;
    }

    iterations = 0;
    while ( NULL != pprt && pbase->state <= ego_object_initializing && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( pprt );
        if ( ptmp != pprt ) return NULL;
        iterations++;
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_activate( enc_t * penc, int max_iterations )
{
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( ego_object_active + 1 ) )
    {
        enc_t * tmp_enc = enc_config_deconstruct( penc, max_iterations );
        if ( tmp_enc == penc ) return NULL;
    }

    iterations = 0;
    while ( NULL != penc && pbase->state < ego_object_active && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( penc );
        if ( ptmp != penc ) return NULL;
        iterations++;
    }

    EGOBOO_ASSERT( pbase->state == ego_object_active );
    if( pbase->state == ego_object_active )
    {
        EncList_add_used( GET_INDEX_PENC( penc ) );
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_deinitialize( enc_t * penc, int max_iterations )
{
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deinitialize it
    if ( pbase->state > ( int )( ego_object_deinitializing + 1 ) )
    {
        return penc;
    }
    else if ( pbase->state < ego_object_deinitializing )
    {
        pbase->state = ego_object_deinitializing;
    }

    iterations = 0;
    while ( NULL != penc && pbase->state <= ego_object_deinitializing && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( penc );
        if ( ptmp != penc ) return NULL;
        iterations++;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_deconstruct( enc_t * penc, int max_iterations )
{
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it
    if ( pbase->state > ( int )( ego_object_destructing + 1 ) )
    {
        return penc;
    }
    else if ( pbase->state < ego_object_deinitializing )
    {
        // make sure that you deinitialize before destructing
        pbase->state = ego_object_deinitializing;
    }

    iterations = 0;
    while ( NULL != penc && pbase->state <= ego_object_destructing && iterations < max_iterations )
    {
        enc_t * ptmp = enc_run_config( penc );
        if ( ptmp != penc ) return NULL;
        iterations++;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enc_t * enc_run_config( enc_t * penc )
{
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // set the object to deinitialize if it is not "dangerous" and if was requested
    if ( pbase->kill_me )
    {
        if ( pbase->state > ego_object_constructing && pbase->state < ego_object_deinitializing )
        {
            pbase->state = ego_object_deinitializing;
        }

        pbase->kill_me = bfalse;
    }

    switch ( pbase->state )
    {
        default:
        case ego_object_invalid:
            penc = NULL;
            break;

        case ego_object_constructing:
            penc = enc_config_ctor( penc );
            break;

        case ego_object_initializing:
            penc = enc_config_init( penc );
            break;

        case ego_object_active:
            penc = enc_config_active( penc );
            break;

        case ego_object_deinitializing:
            penc = enc_config_deinit( penc );
            break;

        case ego_object_destructing:
            penc = enc_config_dtor( penc );
            break;

        case ego_object_waiting:
        case ego_object_terminated:
            /* do nothing */
            break;
    }

    if( NULL == penc )
    {
        pbase->update_guid = INVALID_UPDATE_GUID;
    }
    else if( ego_object_active == pbase->state )
    {
        pbase->update_guid = EncList.update_guid;
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_ctor( enc_t * penc )
{
    ego_object_base_t * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_CONSTRUCTING_PBASE( pbase ) ) return penc;

    return enc_ctor( penc );
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_init( enc_t * penc )
{
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase ) return NULL;

    if ( !STATE_INITIALIZING_PBASE( pbase ) ) return penc;

    POBJ_BEGIN_SPAWN(penc);

    penc = enc_config_do_init( penc );
    if ( NULL == penc ) return NULL;

    if ( 0 == enc_loop_depth )
    {
        penc->obj_base.on = btrue;
    }
    else
    {
        EncList_add_activation( GET_INDEX_PPRT( penc ) );
    }

    pbase->state = ego_object_active;

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_active( enc_t * penc )
{
    // there's nothing to configure if the object is active...

    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    if ( !STATE_ACTIVE_PBASE( pbase ) ) return penc;

    POBJ_END_SPAWN( penc );

    penc = enc_config_do_active( penc );

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_deinit( enc_t * penc )
{
    /// @details BB@> deinitialize the character data

    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase ) return NULL;

    if ( !STATE_DEINITIALIZING_PBASE( pbase ) ) return penc;

    POBJ_END_SPAWN( penc );

    pbase->state = ego_object_destructing;
    pbase->on    = bfalse;

    return penc;
}

//--------------------------------------------------------------------------------------------
enc_t * enc_config_dtor( enc_t * penc )
{
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( penc );
    if ( NULL == pbase ) return NULL;

    if ( !STATE_DESTRUCTING_PBASE( pbase ) ) return penc;

    POBJ_END_SPAWN( penc );

    return enc_dtor( penc );
}

//--------------------------------------------------------------------------------------------
ENC_REF spawn_one_enchant( const CHR_REF by_reference owner, const CHR_REF by_reference target, const CHR_REF by_reference spawner, const ENC_REF by_reference enc_override, const PRO_REF by_reference modeloptional )
{
    /// @details ZZ@> This function enchants a target, returning the enchantment index or MAX_ENC
    ///    if failed

    ENC_REF enc_ref;
    EVE_REF eve_ref;

    eve_t * peve;
    enc_t * penc;
    chr_t * ptarget;

    PRO_REF loc_profile;
    CHR_REF loc_target;

    // Target must both be alive and on and valid
    loc_target = target;
    if ( !INGAME_CHR( loc_target ) )
    {
        log_warning( "spawn_one_enchant() - failed because the target does not exist.\n" );
        return ( ENC_REF )MAX_ENC;
    }
    ptarget = ChrList.lst + loc_target;

    // you should be able to enchant dead stuff to raise the dead...
    // if( !ptarget->alive ) return (ENC_REF)MAX_ENC;

    if ( LOADED_PRO( modeloptional ) )
    {
        // The enchantment type is given explicitly
        loc_profile = modeloptional;
    }
    else
    {
        // The enchantment type is given by the spawner
        loc_profile = chr_get_ipro( spawner );

        if ( !LOADED_PRO( loc_profile ) )
        {
            log_warning( "spawn_one_enchant() - no valid profile for the spawning character \"%s\"(%d).\n", ChrList.lst[spawner].obj_base._name, REF_TO_INT( spawner ) );
            return ( ENC_REF )MAX_ENC;
        }
    }

    eve_ref = pro_get_ieve( loc_profile );
    if ( !LOADED_EVE( eve_ref ) )
    {
        log_warning( "spawn_one_enchant() - the object \"%s\"(%d) does not have an enchant profile.\n", ProList.lst[loc_profile].name, REF_TO_INT( loc_profile ) );

        return ( ENC_REF )MAX_ENC;
    }
    peve = EveStack.lst + eve_ref;

    // count all the requests for this enchantment type
    peve->enc_request_count++;

    // Owner must both be alive and on and valid if it isn't a stayifnoowner enchant
    if ( !peve->stayifnoowner && ( !INGAME_CHR( owner ) || !ChrList.lst[owner].alive ) )
    {
        log_warning( "spawn_one_enchant() - failed because the required enchant owner cannot be found.\n" );
        return ( ENC_REF )MAX_ENC;
    }

    // do retargeting, if necessary
    // Should it choose an inhand item?
    if ( peve->retarget )
    {
        // Left, right, or both are valid
        if ( INGAME_CHR( ptarget->holdingwhich[SLOT_LEFT] ) )
        {
            // Only right hand is valid
            loc_target = ptarget->holdingwhich[SLOT_RIGHT];
        }
        else if ( INGAME_CHR( ptarget->holdingwhich[SLOT_LEFT] ) )
        {
            // Pick left hand
            loc_target = ptarget->holdingwhich[SLOT_LEFT];
        }
        else
        {
            // No weapons to pick, should it pick itself???
            loc_target = ( CHR_REF )MAX_CHR;
        }
    }

    // make sure the loc_target is valid
    if ( !INGAME_CHR( loc_target ) || !ptarget->alive )
    {
        log_warning( "spawn_one_enchant() - failed because the target is not alive.\n" );
        return ( ENC_REF )MAX_ENC;
    }
    ptarget = ChrList.lst + loc_target;

    // Check peve->dontdamagetype
    if ( peve->dontdamagetype != DAMAGE_NONE )
    {
        if ( GET_DAMAGE_RESIST(ptarget->damagemodifier[peve->dontdamagetype]) >= 3 ||
              HAS_SOME_BITS(ptarget->damagemodifier[peve->dontdamagetype],DAMAGECHARGE) )
        {
            log_warning( "spawn_one_enchant() - failed because the target is immune to the enchant.\n" );
            return ( ENC_REF )MAX_ENC;
        }
    }

    // Check peve->onlydamagetype
    if ( peve->onlydamagetype != DAMAGE_NONE )
    {
        if ( ptarget->damagetargettype != peve->onlydamagetype )
        {
            log_warning( "spawn_one_enchant() - failed because the target not have the right damagetargettype.\n" );
            return ( ENC_REF )MAX_ENC;
        }
    }

    // Find an enchant index to use
    enc_ref = EncList_allocate( enc_override );

    if ( !ALLOCATED_ENC( enc_ref ) )
    {
        log_warning( "spawn_one_enchant() - could not allocate an enchant.\n" );
        return ( ENC_REF )MAX_ENC;
    }
    penc = EncList.lst + enc_ref;

    penc->spawn_data.owner_ref   = owner;
    penc->spawn_data.target_ref  = loc_target;
    penc->spawn_data.spawner_ref = spawner;
    penc->spawn_data.profile_ref = loc_profile;
    penc->spawn_data.eve_ref     = eve_ref;

    // actually force the character to spawn
    penc = enc_config_activate( penc, 100 );

    // count out all the requests for this particle type
    if ( NULL != penc )
    {
        peve->enc_create_count++;
    }

    return enc_ref;
}

//--------------------------------------------------------------------------------------------
EVE_REF load_one_enchant_profile_vfs( const char* szLoadName, const EVE_REF by_reference ieve )
{
    /// @details ZZ@> This function loads an enchantment profile into the EveStack

    EVE_REF retval = ( EVE_REF )MAX_EVE;

    if ( VALID_EVE_RANGE( ieve ) )
    {
        eve_t * peve = EveStack.lst + ieve;

        if ( NULL != load_one_enchant_file_vfs( szLoadName, peve ) )
        {
            retval = ieve;

            // limit the endsound_index
            peve->endsound_index = CLIP( peve->endsound_index, INVALID_SOUND, MAX_WAVE );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void enc_remove_set( const ENC_REF by_reference ienc, int value_idx )
{
    /// @details ZZ@> This function unsets a set value
    CHR_REF character;
    enc_t * penc;
    chr_t * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if ( value_idx >= MAX_ENCHANT_SET || !penc->setyesno[value_idx] ) return;

    if ( !INGAME_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget   = ChrList.lst + penc->target_ref;

    switch ( value_idx )
    {
        case SETDAMAGETYPE:
            ptarget->damagetargettype = penc->setsave[value_idx];
            break;

        case SETNUMBEROFJUMPS:
            chr_set_jump_number_reset( ptarget, penc->setsave[value_idx] );
            break;

        case SETLIFEBARCOLOR:
            ptarget->lifecolor = penc->setsave[value_idx];
            break;

        case SETMANABARCOLOR:
            ptarget->manacolor = penc->setsave[value_idx];
            break;

        case SETSLASHMODIFIER:
            ptarget->damagemodifier[DAMAGE_SLASH] = penc->setsave[value_idx];
            break;

        case SETCRUSHMODIFIER:
            ptarget->damagemodifier[DAMAGE_CRUSH] = penc->setsave[value_idx];
            break;

        case SETPOKEMODIFIER:
            ptarget->damagemodifier[DAMAGE_POKE] = penc->setsave[value_idx];
            break;

        case SETHOLYMODIFIER:
            ptarget->damagemodifier[DAMAGE_HOLY] = penc->setsave[value_idx];
            break;

        case SETEVILMODIFIER:
            ptarget->damagemodifier[DAMAGE_EVIL] = penc->setsave[value_idx];
            break;

        case SETFIREMODIFIER:
            ptarget->damagemodifier[DAMAGE_FIRE] = penc->setsave[value_idx];
            break;

        case SETICEMODIFIER:
            ptarget->damagemodifier[DAMAGE_ICE] = penc->setsave[value_idx];
            break;

        case SETZAPMODIFIER:
            ptarget->damagemodifier[DAMAGE_ZAP] = penc->setsave[value_idx];
            break;

        case SETFLASHINGAND:
            ptarget->flashand = penc->setsave[value_idx];
            break;

        case SETLIGHTBLEND:
            chr_set_light( ptarget, penc->setsave[value_idx] );
            break;

        case SETALPHABLEND:
            chr_set_alpha( ptarget, penc->setsave[value_idx] );
            break;

        case SETSHEEN:
            chr_set_sheen( ptarget, penc->setsave[value_idx] );
            break;

        case SETFLYTOHEIGHT:
            chr_set_fly_height( ptarget, penc->setsave[value_idx] );
            break;

        case SETWALKONWATER:
            ptarget->waterwalk = ( 0 != penc->setsave[value_idx] );
            break;

        case SETCANSEEINVISIBLE:
            ptarget->see_invisible_level = penc->setsave[value_idx];
            break;

        case SETMISSILETREATMENT:
            ptarget->missiletreatment = penc->setsave[value_idx];
            break;

        case SETCOSTFOREACHMISSILE:
            ptarget->missilecost = penc->setsave[value_idx];
            ptarget->missilehandler = character;
            break;

        case SETMORPH:
            // Need special handler for when this is removed
            change_character( character, ptarget->basemodel_ref, penc->setsave[value_idx], ENC_LEAVE_ALL );
            break;

        case SETCHANNEL:
            ptarget->canchannel = ( 0 != penc->setsave[value_idx] );
            break;
    }

    penc->setyesno[value_idx] = bfalse;
}

//--------------------------------------------------------------------------------------------
void enc_remove_add( const ENC_REF by_reference ienc, int value_idx )
{
    /// @details ZZ@> This function undoes cumulative modification to character stats

    float fvaluetoadd;
    int valuetoadd;
    CHR_REF character;
    enc_t * penc;
    chr_t * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_ADD ) return;

    if ( !ALLOCATED_ENC( ienc ) ) return;
    penc = EncList.lst + ienc;

    if ( !INGAME_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = ChrList.lst + penc->target_ref;

    if ( penc->addyesno[value_idx] )
    {
        switch ( value_idx )
        {
            case ADDJUMPPOWER:
                fvaluetoadd = penc->addsave[value_idx];
                ptarget->jump_power -= fvaluetoadd;
                break;

            case ADDBUMPDAMPEN:
                fvaluetoadd = penc->addsave[value_idx];
                ptarget->phys.bumpdampen -= fvaluetoadd;
                break;

            case ADDBOUNCINESS:
                fvaluetoadd = penc->addsave[value_idx];
                ptarget->phys.dampen -= fvaluetoadd;
                break;

            case ADDDAMAGE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->damageboost -= valuetoadd;
                break;

            case ADDSIZE:
                fvaluetoadd = penc->addsave[value_idx];
                ptarget->fat_goto -= fvaluetoadd;
                ptarget->fat_goto_time = SIZETIME;
                break;

            case ADDACCEL:
                fvaluetoadd = penc->addsave[value_idx];
                chr_set_maxaccel( ptarget, ptarget->maxaccel_reset - fvaluetoadd );
                break;

            case ADDRED:
                valuetoadd = penc->addsave[value_idx];
                chr_set_redshift( ptarget, ptarget->inst.redshift - valuetoadd );
                break;

            case ADDGRN:
                valuetoadd = penc->addsave[value_idx];
                chr_set_grnshift( ptarget, ptarget->inst.grnshift - valuetoadd );
                break;

            case ADDBLU:
                valuetoadd = penc->addsave[value_idx];
                chr_set_blushift( ptarget, ptarget->inst.blushift - valuetoadd );
                break;

            case ADDDEFENSE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->defense -= valuetoadd;
                break;

            case ADDMANA:
                valuetoadd = penc->addsave[value_idx];
                ptarget->manamax -= valuetoadd;
                ptarget->mana -= valuetoadd;
                if ( ptarget->mana < 0 ) ptarget->mana = 0;
                break;

            case ADDLIFE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->lifemax -= valuetoadd;
                ptarget->life -= valuetoadd;
                if ( ptarget->life < 1 ) ptarget->life = 1;
                break;

            case ADDSTRENGTH:
                valuetoadd = penc->addsave[value_idx];
                ptarget->strength -= valuetoadd;
                break;

            case ADDWISDOM:
                valuetoadd = penc->addsave[value_idx];
                ptarget->wisdom -= valuetoadd;
                break;

            case ADDINTELLIGENCE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->intelligence -= valuetoadd;
                break;

            case ADDDEXTERITY:
                valuetoadd = penc->addsave[value_idx];
                ptarget->dexterity -= valuetoadd;
                break;
        }

        penc->addyesno[value_idx] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_eve()
{
    EVE_REF cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        eve_init( EveStack.lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_eve()
{
    EVE_REF cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        release_one_eve( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_eve( const EVE_REF by_reference ieve )
{
    eve_t * peve;

    if ( !VALID_EVE_RANGE( ieve ) ) return bfalse;
    peve = EveStack.lst + ieve;

    if ( !peve->loaded ) return btrue;

    eve_init( peve );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void update_all_enchants()
{
    ENC_REF ienc;

    // update all enchants
    for( ienc = 0; ienc < MAX_ENC; ienc++ )
    {
        enc_run_config( EncList.lst + ienc );
    }

    // fix the stat timer
    if ( clock_enc_stat >= ONESECOND )
    {
        // Reset the clock
        clock_enc_stat -= ONESECOND;
    }
}

//--------------------------------------------------------------------------------------------
ENC_REF cleanup_enchant_list( const ENC_REF by_reference ienc, ENC_REF * enc_parent )
{
    /// @details BB@> remove all the dead enchants from the enchant list
    ///     and report back the first non-dead enchant in the list.

    bool_t enc_used[MAX_ENC];

    int cnt;
    ENC_REF first_valid_enchant;
    ENC_REF enc_now, enc_next;

    if( !VALID_ENC_RANGE(ienc) ) return MAX_ENC;

    // clear the list
    memset( enc_used, 0, sizeof(enc_used) );

    // scan the list of enchants
    enc_next            = (ENC_REF) MAX_ENC;
    first_valid_enchant = enc_now = ienc;
    for( cnt = 0; enc_now < MAX_ENC && enc_next != enc_now && cnt < MAX_ENC; cnt++ )
    {
        enc_next = EncList.lst[enc_now].nextenchant_ref;

        // coerce the list of enchants to a valid value
        if( !VALID_ENC_RANGE(enc_next) )
        {
            enc_next = EncList.lst[enc_now].nextenchant_ref = MAX_ENC;
        }

        // fix any loops in the enchant list
        if( enc_used[enc_next] )
        {
            EncList.lst[enc_now].nextenchant_ref = MAX_ENC;
            break;
        }

        //( !INGAME_CHR( EncList.lst[enc_now].target_ref ) && !EveStack.lst[EncList.lst[enc_now].eve_ref].stayiftargetdead )

        // remove any expired enchants
        if ( !INGAME_ENC( enc_now ) )
        {
            remove_enchant( enc_now, enc_parent );
            enc_used[enc_now] = btrue;
        }
        else
        {
            // store this enchant in the list of used enchants
            enc_used[enc_now] = btrue;

            // keep track of the first valid enchant
            if ( MAX_ENC == first_valid_enchant )
            {
                first_valid_enchant = enc_now;
            }
        }

        enc_parent = &(EncList.lst[enc_now].nextenchant_ref);
        enc_now    = enc_next;
    }

    if( enc_now == enc_next || cnt >= MAX_ENC )
    {
        // break the loop. this will only happen if the list is messed up.
        EncList.lst[enc_now].nextenchant_ref = (ENC_REF) MAX_ENC;

        log_warning("cleanup_enchant_list() - The enchantment list is corrupt!\n");
        //EGOBOO_ASSERT( bfalse );
    }

    return first_valid_enchant;
}

//--------------------------------------------------------------------------------------------
void cleanup_all_enchants()
{
    /// @details ZZ@> this function scans all the enchants and removes any dead ones.
    ///               this happens only once a loop

    ENC_BEGIN_LOOP_ACTIVE( ienc, penc )
    {
        ENC_REF * enc_lst;
        eve_t   * peve;
        bool_t    do_remove;
        bool_t valid_owner, valid_target;

        // try to determine something about the parent
        enc_lst = NULL;
        valid_target = bfalse;
        if ( INGAME_CHR( penc->target_ref ) )
        {
            valid_target = ChrList.lst[penc->target_ref].alive;

            // this is linked to a known character
            enc_lst = &( ChrList.lst[penc->target_ref].firstenchant );
        }

        //try to determine if the owner exists and is alive
        valid_owner = bfalse;
        if ( INGAME_CHR( penc->owner_ref ) )
        {
            valid_owner = ChrList.lst[penc->owner_ref].alive;
        }

        if ( !LOADED_EVE( penc->eve_ref ) )
        {
            // this should never happen
            EGOBOO_ASSERT( bfalse );
            continue;
        }
        peve = EveStack.lst + penc->eve_ref;

        do_remove = bfalse;
        if ( WAITING_PBASE( POBJ_GET_PBASE( penc ) ) )
        {
            // the enchant has been marked for removal
            do_remove = btrue;
        }
        else if ( !valid_owner && !peve->stayifnoowner )
        {
            // the enchant's owner has died
            do_remove = btrue;
        }
        else if ( !valid_target && !peve->stayiftargetdead )
        {
            // the enchant's target has died
            do_remove = btrue;
        }
        else if ( valid_owner && peve->endifcantpay )
        {
            // Undo enchants that cannot be sustained anymore
            if ( ChrList.lst[penc->owner_ref].mana == 0 ) do_remove = btrue;
        }
        else
        {
            // the enchant has timed out
            do_remove = ( 0 == penc->time );
        }

        if ( do_remove )
        {
            remove_enchant( ienc, NULL );
        }
    }
    ENC_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void increment_all_enchant_update_counters()
{
    ENC_REF cnt;

    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        ego_object_base_t * pbase;

        pbase = POBJ_GET_PBASE( EncList.lst + cnt );
        if ( !ACTIVE_PBASE( pbase ) ) continue;

        pbase->update_count++;
    }
}

//--------------------------------------------------------------------------------------------
bool_t enc_request_terminate( const ENC_REF by_reference ienc )
{
    if ( !ALLOCATED_ENC( ienc ) || TERMINATED_ENC( ienc ) ) return bfalse;

    POBJ_REQUEST_TERMINATE( EncList.lst + ienc );

    return btrue;
}

