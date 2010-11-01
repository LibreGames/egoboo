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
t_cpp_stack< ego_eve, MAX_EVE  > EveStack;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void enchant_system_begin()
{
    EncObjList.init();
    init_all_eve();
}

//--------------------------------------------------------------------------------------------
void enchant_system_end()
{
    release_all_eve();
    EncObjList.deinit();
}

//--------------------------------------------------------------------------------------------
// struct ego_enc_data -
//--------------------------------------------------------------------------------------------
ego_enc_data * ego_enc_data::ctor_this( ego_enc_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    // set the critical values to safe ones
    pdata = ego_enc_data::init( pdata );
    if ( NULL == pdata ) return NULL;

    // construct/allocate any dynamic data
    pdata = ego_enc_data::alloc( pdata );
    if ( NULL == pdata ) return NULL;

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_enc_data * ego_enc_data::dtor_this( ego_enc_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    // destruct/free any dynamic data
    pdata = ego_enc_data::dealloc( pdata );
    if ( NULL == pdata ) return NULL;

    // set the critical values to safe ones
    pdata = ego_enc_data::init( pdata );
    if ( NULL == pdata ) return NULL;

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_enc_data * ego_enc_data::init( ego_enc_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    pdata->profile_ref      = PRO_REF( MAX_PROFILE );
    pdata->eve_ref          = EVE_REF( MAX_EVE );

    pdata->target_ref       = CHR_REF( MAX_CHR );
    pdata->owner_ref        = CHR_REF( MAX_CHR );
    pdata->spawner_ref      = CHR_REF( MAX_CHR );
    pdata->spawnermodel_ref = PRO_REF( MAX_PROFILE );
    pdata->overlay_ref      = CHR_REF( MAX_CHR );

    pdata->nextenchant_ref  = ENC_REF( MAX_ENC );

    return pdata;
}

//--------------------------------------------------------------------------------------------
// struct ego_enc -
//--------------------------------------------------------------------------------------------
ego_enc *  ego_enc::alloc( ego_enc * penc )
{
    if ( !VALID_PENC( penc ) ) return bfalse;

    // nothing to do yet

    return penc;
}

//--------------------------------------------------------------------------------------------
ego_enc *  ego_enc::dealloc( ego_enc * penc )
{
    if ( !VALID_PENC( penc ) ) return bfalse;

    // nothing to do yet

    return penc;
}

//--------------------------------------------------------------------------------------------
ego_enc * ego_enc::do_alloc( ego_enc * penc )
{
    if ( NULL == penc ) return penc;

    ego_enc::alloc( penc );
    ego_enc_data::alloc( penc );

    return penc;
}

//--------------------------------------------------------------------------------------------
ego_enc * ego_enc::do_dealloc( ego_enc * penc )
{
    if ( NULL == penc ) return penc;

    ego_enc_data::dealloc( penc );

    ego_enc::dealloc( penc );

    return penc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_enc * ego_enc::ctor_this( ego_enc * penc )
{
    /// @details BB@> initialize the ego_enc

    if ( NULL == penc ) return penc;

    // construct/allocate any dynamic data
    penc = ego_enc::init( penc );
    if ( NULL == penc ) return penc;

    // construct/allocate any dynamic data
    penc = ego_enc::alloc( penc );
    if ( NULL == penc ) return penc;

    return penc;
}

//--------------------------------------------------------------------------------------------
ego_enc * ego_enc::dtor_this( ego_enc * penc )
{
    if ( NULL == penc ) return penc;

    // destruct/free any dynamic data
    ego_enc::dealloc( penc );

    // construct/allocate any dynamic data
    penc = ego_enc::init( penc );
    if ( NULL == penc ) return penc;

    return penc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t unlink_enchant( const ENC_REF & ienc, ENC_REF * ego_enc_parent )
{
    ego_enc * penc = EncObjList.get_allocated_data_ptr( ienc );
    if ( NULL == penc ) return bfalse;

    // Unlink it from the spawner (if possible)
    ego_chr * pspawner = ChrObjList.get_allocated_data_ptr( penc->spawner_ref );
    if ( NULL != pspawner )
    {
        if ( ienc == pspawner->undoenchant )
        {
            pspawner->undoenchant = ENC_REF( MAX_ENC );
        }
    }

    // find the parent reference for the enchant
    if ( NULL == ego_enc_parent )
    {
        ENC_REF ienc_last, ienc_now;

        ego_chr * ptarget =  ChrObjList.get_allocated_data_ptr( penc->target_ref );
        if ( NULL != ptarget )
        {
            if ( ptarget->firstenchant == ienc )
            {
                // It was the first in the list
                ego_enc_parent = &( ptarget->firstenchant );
            }
            else
            {
                // Search until we find it
                ienc_last = ienc_now = ptarget->firstenchant;
                while ( MAX_ENC != ienc_now && ienc_now != ienc )
                {
                    ienc_last    = ienc_now;
                    ienc_now = EncObjList.get_data_ref( ienc_now ).nextenchant_ref;
                }

                // Relink the last enchantment
                if ( ienc_now == ienc )
                {
                    ego_enc_parent = &( EncObjList.get_data_ref( ienc_last ).nextenchant_ref );
                }
            }
        }
    }

    // unlink the enchant from the parent reference
    if ( NULL != ego_enc_parent )
    {
        *ego_enc_parent = EncObjList.get_data_ref( ienc ).nextenchant_ref;
    }

    return NULL != ego_enc_parent;
}

//--------------------------------------------------------------------------------------------
bool_t remove_all_enchants_with_idsz( CHR_REF ichr, IDSZ remove_idsz )
{
    /// @details ZF@> This function removes all enchants with the character that has the specified
    ///               IDSZ. If idsz [NONE] is specified, all enchants will be removed. Return btrue
    ///               if at least one enchant was removed.

    ENC_REF ego_enc_now, ego_enc_next;
    ego_eve * peve;
    bool_t retval = bfalse;
    ego_chr *pchr;

    // Stop invalid pointers
    if ( !PROCESSING_CHR( ichr ) ) return bfalse;
    pchr = ChrObjList.get_data_ptr( ichr );

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pchr );

    // Check all enchants to see if they are removed
    ego_enc_now = pchr->firstenchant;
    while ( ego_enc_now != MAX_ENC )
    {
        ego_enc_next  = EncObjList.get_data_ref( ego_enc_now ).nextenchant_ref;

        peve = ego_enc::get_peve( ego_enc_now );
        if ( NULL != peve && ( IDSZ_NONE == remove_idsz || remove_idsz == peve->removedbyidsz ) )
        {
            remove_enchant( ego_enc_now, NULL );
            retval = btrue;
        }

        ego_enc_now = ego_enc_next;
    }
    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t remove_enchant( const ENC_REF & ienc, ENC_REF * ego_enc_parent )
{
    /// @details ZZ@> This function removes a specific enchantment and adds it to the unused list

    int     iwave;
    CHR_REF overlay_ref;
    int add_type, set_type;

    ego_enc * penc;
    ego_eve * peve;
    CHR_REF itarget, ispawner;

    penc = EncObjList.get_allocated_data_ptr( ienc );
    if ( NULL == penc ) return bfalse;

    itarget  = penc->target_ref;
    ispawner = penc->spawner_ref;
    peve     = ego_enc::get_peve( ienc );

    // Unsparkle the spellbook
    if ( INGAME_CHR( ispawner ) )
    {
        ego_chr * pspawner = ChrObjList.get_data_ptr( ispawner );

        pspawner->sparkle = NOSPARKLE;

        // Make the spawner unable to undo the enchantment
        if ( pspawner->undoenchant == ienc )
        {
            pspawner->undoenchant = ENC_REF( MAX_ENC );
        }
    }

    //---- Remove all the enchant stuff in exactly the opposite order to how it was applied

    // Remove all of the cumulative values first, since we did it
    for ( add_type = ENC_ADD_LAST; add_type >= ENC_ADD_FIRST; add_type-- )
    {
        ego_enc::remove_add( ienc, add_type );
    }

    // unset them in the reverse order of setting them, doing morph last
    for ( set_type = ENC_SET_LAST; set_type >= ENC_SET_FIRST; set_type-- )
    {
        ego_enc::remove_set( ienc, set_type );
    }

    // Now fix dem weapons
    if ( INGAME_CHR( itarget ) )
    {
        ego_chr * ptarget = ChrObjList.get_data_ptr( itarget );
        reset_character_alpha( ptarget->holdingwhich[SLOT_LEFT] );
        reset_character_alpha( ptarget->holdingwhich[SLOT_RIGHT] );
    }

    // unlink this enchant from its parent
    unlink_enchant( ienc, ego_enc_parent );

    // Kill overlay too...
    overlay_ref = penc->overlay_ref;
    if ( INGAME_CHR( overlay_ref ) )
    {
        ego_chr * povl = ChrObjList.get_data_ptr( overlay_ref );

        if ( IS_INVICTUS_PCHR_RAW( povl ) )
        {
            ego_chr::get_pteam_base( overlay_ref )->morale++;
        }

        kill_character( overlay_ref, CHR_REF( MAX_CHR ), btrue );
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
                    sound_play_chunk( ChrObjList.get_data_ref( itarget ).pos_old, pro_get_chunk( imodel, iwave ) );
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
            ego_chr * ptarget = ChrObjList.get_data_ptr( penc->target_ref );

            if ( peve->seekurse && !ego_chr_data::get_skill( ptarget, MAKE_IDSZ( 'C', 'K', 'U', 'R' ) ) )
            {
                ptarget->see_kurse_level = bfalse;
            }
        }
    }

    EncObjList.free_one( ienc );

    // save this until the enchant is completely dead, since kill character can generate a
    // recursive call to this function through cleanup_one_character()
    // @note all of the values in the penc are now invalid. we have to use previously evaluated
    // values of itarget and penc to kill the target (if necessary)
    if ( INGAME_CHR( itarget ) && NULL != peve && peve->killtargetonend )
    {
        ego_chr * ptarget = ChrObjList.get_data_ptr( itarget );

        if ( IS_INVICTUS_PCHR_RAW( ptarget ) )
        {
            ego_chr::get_pteam_base( itarget )->morale++;
        }

        kill_character( itarget, CHR_REF( MAX_CHR ), btrue );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ENC_REF ego_enc::value_filled( const ENC_REF &  ienc, int value_idx )
{
    /// @details ZZ@> This function returns MAX_ENC if the enchantment's target has no conflicting
    ///    set values in its other enchantments.  Otherwise it returns the ienc
    ///    of the conflicting enchantment

    CHR_REF character;
    ENC_REF currenchant;
    ego_chr * pchr;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return ENC_REF( MAX_ENC );

    if ( !INGAME_ENC( ienc ) ) return ENC_REF( MAX_ENC );

    character = EncObjList.get_data_ref( ienc ).target_ref;
    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return ENC_REF( MAX_ENC );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // scan the enchant list
    currenchant = pchr->firstenchant;
    while ( currenchant != MAX_ENC )
    {
        if ( INGAME_ENC( currenchant ) && EncObjList.get_data_ref( currenchant ).setyesno[value_idx] )
        {
            break;
        }

        currenchant = EncObjList.get_data_ref( currenchant ).nextenchant_ref;
    }

    return currenchant;
}

//--------------------------------------------------------------------------------------------
void ego_enc::apply_set( const ENC_REF &  ienc, int value_idx, const PRO_REF & profile )
{
    /// @details ZZ@> This function sets and saves one of the character's stats

    ENC_REF conflict;
    CHR_REF character;
    ego_enc * penc;
    ego_eve * peve;
    ego_chr * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return;

    if ( !DEFINED_ENC( ienc ) ) return;
    penc = EncObjList.get_allocated_data_ptr( ienc );

    peve = pro_get_peve( profile );
    if ( NULL == peve ) return;

    penc->setyesno[value_idx] = bfalse;
    if ( peve->setyesno[value_idx] )
    {
        conflict = ego_enc::value_filled( ienc, value_idx );
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
                    ego_enc::remove_set( conflict, value_idx );
                }
            }

            // Set the value, and save the character's real stat
            if ( DEFINED_CHR( penc->target_ref ) )
            {
                character = penc->target_ref;
                ptarget = ChrObjList.get_data_ptr( character );

                penc->setyesno[value_idx] = btrue;

                switch ( value_idx )
                {
                    case SETDAMAGETYPE:
                        penc->setsave[value_idx]  = ptarget->damagetargettype;
                        ptarget->damagetargettype = peve->setvalue[value_idx];
                        break;

                    case SETNUMBEROFJUMPS:
                        penc->setsave[value_idx] = ptarget->jump_number_reset;
                        ego_chr::set_jump_number_reset( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETLIFEBARCOLOR:
                        penc->setsave[value_idx] = ptarget->life_color;
                        ptarget->life_color       = peve->setvalue[value_idx];
                        break;

                    case SETMANABARCOLOR:
                        penc->setsave[value_idx] = ptarget->mana_color;
                        ptarget->mana_color       = peve->setvalue[value_idx];
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
                        penc->setsave[value_idx] = ptarget->gfx_inst.light;
                        ego_chr::set_light( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETALPHABLEND:
                        penc->setsave[value_idx] = ptarget->gfx_inst.alpha;
                        ego_chr::set_alpha( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETSHEEN:
                        penc->setsave[value_idx] = ptarget->gfx_inst.sheen;
                        ego_chr::set_sheen( ptarget, peve->setvalue[value_idx] );
                        break;

                    case SETFLYTOHEIGHT:
                        penc->setsave[value_idx] = ptarget->fly_height;
                        if ( ptarget->pos.z > -2.0f )
                        {
                            ego_chr_data::set_fly_height( ptarget, peve->setvalue[value_idx] );
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
                        ego_chr::change_profile( character, profile, 0, ENC_LEAVE_ALL ); // ENC_LEAVE_FIRST);
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
void ego_enc::apply_add( const ENC_REF & ienc, int value_idx, const EVE_REF & ieve )
{
    /// @details ZZ@> This function does cumulative modification to character stats

    int valuetoadd, newvalue;
    float fvaluetoadd, fnewvalue;
    CHR_REF character;
    ego_enc * penc;
    ego_eve * peve;
    ego_chr * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_ADD ) return;

    if ( !DEFINED_ENC( ienc ) ) return;
    penc = EncObjList.get_allocated_data_ptr( ienc );

    if ( ieve >= MAX_EVE || !EveStack[ieve].loaded ) return;
    peve = EveStack + ieve;

    if ( !peve->addyesno[value_idx] )
    {
        penc->addyesno[value_idx] = bfalse;
        penc->addsave[value_idx]  = 0.0f;
        return;
    }

    if ( !DEFINED_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = ChrObjList.get_data_ptr( character );

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
            ego_chr::set_maxaccel( ptarget, ptarget->maxaccel_reset + fvaluetoadd );
            break;

        case ADDRED:
            newvalue = ptarget->gfx_inst.redshift;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 6, &valuetoadd );
            ego_chr::set_redshift( ptarget, ptarget->gfx_inst.redshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDGRN:
            newvalue = ptarget->gfx_inst.grnshift;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 6, &valuetoadd );
            ego_chr::set_grnshift( ptarget, ptarget->gfx_inst.grnshift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDBLU:
            newvalue = ptarget->gfx_inst.blushift;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, 6, &valuetoadd );
            ego_chr::set_blushift( ptarget, ptarget->gfx_inst.blushift + valuetoadd );
            fvaluetoadd = valuetoadd;
            break;

        case ADDDEFENSE:
            /// @note ZF@> why limit min to 55?
            newvalue = ptarget->defense;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 55, newvalue, 255, &valuetoadd );  // Don't fix again!
            ptarget->defense += valuetoadd;
            fvaluetoadd = valuetoadd;
            break;

        case ADDMANA:
            /// @note ZF@> bit of a problem here, we don't want players to heal or lose life by requipping magic ornaments
            newvalue = ptarget->mana_max;
            valuetoadd = peve->addvalue[value_idx];
            getadd( 0, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->mana_max += valuetoadd;
            //ptarget->mana    += valuetoadd;
            ptarget->mana = CLIP( ptarget->mana, 0, ptarget->mana_max );
            fvaluetoadd = valuetoadd;
            break;

        case ADDLIFE:
            /// @note ZF@> bit of a problem here, we don't want players to heal or lose life by requipping magic ornaments
            newvalue = ptarget->life_max;
            valuetoadd = peve->addvalue[value_idx];
            getadd( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
            ptarget->life_max += valuetoadd;
            //ptarget->life += valuetoadd;
            ptarget->life = CLIP( ptarget->life, 1, ptarget->life_max );
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
ego_enc * ego_enc::do_constructing( ego_enc * penc )
{
    // this object has already been constructed as a part of the
    // ego_obj_enc, so its parent is properly defined
    ego_obj_enc * old_parent = ego_enc::get_obj_ptr( penc );

    // reconstruct the enchant data with the old parent
    penc = ego_enc::ctor_all( penc, old_parent );

    /* add something here */

    // call the parent's virtual function
    get_obj_ptr( penc )->ego_obj::do_constructing();

    return penc;
}

//--------------------------------------------------------------------------------------------
ego_enc * ego_enc::do_initializing( ego_enc * penc )
{
    ego_enc_spawn_data * pdata;
    ENC_REF ienc;
    CHR_REF overlay;

    ego_eve * peve;
    ego_chr * ptarget;

    int add_type, set_type;

    if ( NULL == penc ) return NULL;
    ienc  = GET_IDX_PENC( penc );

    // get the profile data
    pdata = &( penc->spawn_data );

    // store the profile
    penc->profile_ref  = pdata->profile_ref;

    // Convert from local pdata->eve_ref to global enchant profile
    if ( !LOADED_EVE( pdata->eve_ref ) )
    {
        log_debug( "spawn_one_enchant() - cannot spawn enchant with invalid enchant template (\"eve\") == %d\n", ( pdata->eve_ref ).get_value() );

        return NULL;
    }
    penc->eve_ref = pdata->eve_ref;
    peve = EveStack + pdata->eve_ref;

    // does the target exist?
    if ( !DEFINED_CHR( pdata->target_ref ) )
    {
        penc->target_ref   = CHR_REF( MAX_CHR );
        ptarget            = NULL;
    }
    else
    {
        penc->target_ref = pdata->target_ref;
        ptarget = ChrObjList.get_data_ptr( penc->target_ref );
    }
    penc->target_mana  = peve->target_mana;
    penc->target_life  = peve->target_life;

    // does the owner exist?
    if ( !DEFINED_CHR( pdata->owner_ref ) )
    {
        penc->owner_ref = CHR_REF( MAX_CHR );
    }
    else
    {
        penc->owner_ref  = pdata->owner_ref;
    }
    penc->owner_mana = peve->owner_mana;
    penc->owner_life = peve->owner_life;

    // does the spawner exist?
    if ( !DEFINED_CHR( pdata->spawner_ref ) )
    {
        penc->spawner_ref      = CHR_REF( MAX_CHR );
        penc->spawnermodel_ref = PRO_REF( MAX_PROFILE );
    }
    else
    {
        penc->spawner_ref = pdata->spawner_ref;
        penc->spawnermodel_ref = ego_chr::get_ipro( pdata->spawner_ref );

        ChrObjList.get_data_ref( penc->spawner_ref ).undoenchant = ienc;
    }

    // set some other spawning parameters
    penc->time         = peve->time;
    penc->spawntime    = 1;

    // Now set all of the specific values, morph first
    for ( set_type = ENC_SET_FIRST; set_type <= ENC_SET_LAST; set_type++ )
    {
        ego_enc::apply_set( ienc, set_type, pdata->profile_ref );
    }

    // Now do all of the stat adds
    for ( add_type = ENC_ADD_FIRST; add_type <= ENC_ADD_LAST; add_type++ )
    {
        ego_enc::apply_add( ienc, add_type, pdata->eve_ref );
    }

    // Add it as first in the list
    if ( NULL != ptarget )
    {
        penc->nextenchant_ref = ptarget->firstenchant;
        ptarget->firstenchant = ienc;
    }

    // Create an overlay character?
    if ( peve->spawn_overlay && NULL != ptarget )
    {
        overlay = spawn_one_character( ptarget->pos, pdata->profile_ref, ptarget->team, 0, ptarget->ori.facing_z, NULL, CHR_REF( MAX_CHR ) );
        if ( DEFINED_CHR( overlay ) )
        {
            ego_chr * povl;
            ego_mad * povl_mad;
            int action;

            povl     = ChrObjList.get_data_ptr( overlay );
            povl_mad = ego_chr::get_pmad( overlay );

            penc->overlay_ref = overlay;  // Kill this character on end...
            povl->ai.target   = pdata->target_ref;
            povl->ai.state    = peve->spawn_overlay;    // ??? WHY DO THIS ???
            povl->is_overlay  = btrue;

            // Start out with ActionMJ...  Object activated
            action = mad_get_action( ego_chr::get_imad( overlay ), ACTION_MJ );
            if ( !ACTION_IS_TYPE( action, D ) )
            {
                ego_chr::start_anim( povl, action, bfalse, btrue );
            }

            // Assume it's transparent...
            ego_chr::set_light( povl, 254 );
            ego_chr::set_alpha( povl,   0 );
        }
    }

    // Allow them to see kurses?
    if ( peve->seekurse && NULL != ptarget )
    {
        ptarget->see_kurse_level = btrue;
    }

    // call the parent's virtual function
    get_obj_ptr( penc )->ego_obj::do_initializing();

    return penc;
}

//--------------------------------------------------------------------------------------------
ego_enc * ego_enc::do_processing( ego_enc * penc )
{
    /// @details ZZ@> This function allows enchantments to update, spawn particles,
    //  do drains, stat boosts and despawn.

    ENC_REF  ienc;
    CHR_REF  owner, target;
    EVE_REF  eve;
    ego_eve * peve;
    ego_chr * ptarget;

    if ( NULL == penc ) return penc;
    ienc = GET_REF_PENC( penc );

    // the following functions should not be done the first time through the update loop
    if ( 0 == clock_wld ) return penc;

    peve = ego_enc::get_peve( ienc );
    if ( NULL == peve ) return penc;

    // check to see whether the enchant needs to spawn some particles
    if ( penc->spawntime > 0 ) penc->spawntime--;
    if ( penc->spawntime == 0 && peve->contspawn_amount <= 0 && INGAME_CHR( penc->target_ref ) )
    {
        int      tnc;
        FACING_T facing;
        penc->spawntime = peve->contspawn_delay;
        ptarget = ChrObjList.get_data_ptr( penc->target_ref );

        facing = ptarget->ori.facing_z;
        for ( tnc = 0; tnc < peve->contspawn_amount; tnc++ )
        {
            spawn_one_particle( ptarget->pos, facing, penc->profile_ref, peve->contspawn_pip,
                                CHR_REF( MAX_CHR ), GRIP_LAST, ego_chr::get_iteam( penc->owner_ref ), penc->owner_ref, PRT_REF( MAX_PRT ), tnc, CHR_REF( MAX_CHR ) );

            facing += peve->contspawn_facingadd;
        }
    }

    // Do enchant drains and regeneration
    if ( clock_enc_stat >= ONESECOND )
    {
        if ( 0 == penc->time )
        {
            ego_obj_enc::request_terminate( ienc );
        }
        else
        {
            // Do enchant timer
            if ( penc->time > 0 ) penc->time--;

            // To make life easier
            target = penc->target_ref;
            owner  = ego_enc::get_iowner( ienc );
            eve    = ego_enc::get_ieve( ienc );

            ego_obj_chr * powner  = ChrObjList.get_allocated_data_ptr( owner );
            ego_obj_chr * ptarget = ChrObjList.get_allocated_data_ptr( target );
            ego_eve *     peve    = !EveStack.in_range_ref( eve ) ? NULL : EveStack + eve;

            // Do drains
            if ( NULL != powner && powner->alive )
            {
                // Change life
                if ( penc->owner_life != 0 )
                {
                    powner->life += penc->owner_life;
                    if ( powner->life <= 0 )
                    {
                        kill_character( owner, target, bfalse );
                    }
                    if ( powner->life > powner->life_max )
                    {
                        powner->life = powner->life_max;
                    }
                }

                // Change mana
                if ( penc->owner_mana != 0 )
                {
                    bool_t mana_paid = cost_mana( owner, -penc->owner_mana, target );
                    if ( NULL != peve && peve->endifcantpay && !mana_paid )
                    {
                        ego_obj_enc::request_terminate( ienc );
                    }
                }

            }
            else if ( NULL == peve || !peve->stayifnoowner )
            {
                ego_obj_enc::request_terminate( ienc );
            }

            // the enchant could have been inactivated by the stuff above
            // check it again
            if ( INGAME_ENC( ienc ) && NULL != ptarget )
            {
                if ( ptarget->alive )
                {
                    // Change life
                    if ( penc->target_life != 0 )
                    {
                        ptarget->life += penc->target_life;
                        if ( ptarget->life <= 0 )
                        {
                            kill_character( target, owner, bfalse );
                        }
                        if ( ptarget->life > ptarget->life_max )
                        {
                            ptarget->life = ptarget->life_max;
                        }
                    }

                    // Change mana
                    if ( penc->target_mana != 0 )
                    {
                        bool_t mana_paid = cost_mana( target, -penc->target_mana, owner );
                        if ( NULL == peve || ( peve->endifcantpay && !mana_paid ) )
                        {
                            ego_obj_enc::request_terminate( ienc );
                        }
                    }
                }
                else if ( NULL == peve || !peve->stayiftargetdead )
                {
                    ego_obj_enc::request_terminate( ienc );
                }
            }
        }
    }

    return penc;
}

//--------------------------------------------------------------------------------------------
ego_enc * ego_enc::do_deinitializing( ego_enc * penc )
{
    if ( NULL == penc ) return penc;

    /* nothing to do yet */

    // call the parent's virtual function
    get_obj_ptr( penc )->ego_obj::do_deinitializing();

    return penc;
}

//--------------------------------------------------------------------------------------------
ego_enc * ego_enc::do_destructing( ego_enc * penc )
{
    penc = ego_enc::dtor_all( penc );

    /* add something here */

    // call the parent's virtual function
    get_obj_ptr( penc )->ego_obj::do_destructing();

    return penc;
}

//-------------------------------------------------------------------------------------------
// ego_enc - specialization (if any) of the i_ego_obj interface
//--------------------------------------------------------------------------------------------
bool_t ego_obj_enc::object_allocated( void )
{
    if ( NULL == this || NULL == _container_ptr ) return bfalse;

    return FLAG_ALLOCATED_PCONT( container_type,  _container_ptr );
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_enc::object_update_list_id( void )
{
    if ( NULL == this ) return bfalse;

    // get a non-const version of the container pointer
    container_type * pcont = get_container_ptr( this );
    if ( NULL == pcont ) return bfalse;

    if ( !FLAG_ALLOCATED_PCONT( container_type,  pcont ) ) return bfalse;

    // deal with the return state
    if ( !get_valid( this ) )
    {
        container_type::set_list_id( pcont, INVALID_UPDATE_GUID );
    }
    else if ( ego_obj_processing == get_action( this ) )
    {
        container_type::set_list_id( pcont, EncObjList.get_list_id() );
    }

    return INVALID_UPDATE_GUID != container_type::get_list_id( pcont );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ENC_REF spawn_one_enchant( const CHR_REF & owner, const CHR_REF & target, const CHR_REF & spawner, const ENC_REF & ego_enc_override, const PRO_REF & modeloptional )
{
    /// @details ZZ@> This function enchants a target, returning the enchantment index or MAX_ENC
    ///    if failed

    ENC_REF ego_enc_ref;
    EVE_REF eve_ref;

    ego_obj_enc * pobj;
    ego_enc     * penc;
    ego_eve     * peve;
    ego_chr     * ptarget;

    PRO_REF loc_profile;
    CHR_REF loc_target;

    // Target must both be alive and on and valid
    loc_target = target;
    if ( !INGAME_CHR( loc_target ) )
    {
        log_warning( "spawn_one_enchant() - failed because the target does not exist.\n" );
        return ENC_REF( MAX_ENC );
    }
    ptarget = ChrObjList.get_data_ptr( loc_target );

    // you should be able to enchant dead stuff to raise the dead...
    // if( !ptarget->alive ) return ENC_REF(MAX_ENC);

    if ( LOADED_PRO( modeloptional ) )
    {
        // The enchantment type is given explicitly
        loc_profile = modeloptional;
    }
    else
    {
        // The enchantment type is given by the spawner
        loc_profile = ego_chr::get_ipro( spawner );

        if ( !LOADED_PRO( loc_profile ) )
        {
            log_warning( "spawn_one_enchant() - no valid profile for the spawning character \"%s\"(%d).\n", ChrObjList.get_data_ref( spawner ).base_name, ( spawner ).get_value() );
            return ENC_REF( MAX_ENC );
        }
    }

    eve_ref = pro_get_ieve( loc_profile );
    if ( !LOADED_EVE( eve_ref ) )
    {
        log_warning( "spawn_one_enchant() - the object \"%s\"(%d) does not have an enchant profile.\n", ProList.lst[loc_profile].name, ( loc_profile ).get_value() );

        return ENC_REF( MAX_ENC );
    }
    peve = EveStack + eve_ref;

    // count all the requests for this enchantment type
    peve->ego_enc_request_count++;

    // Owner must both be alive and on and valid if it isn't a stayifnoowner enchant
    if ( !peve->stayifnoowner && ( !INGAME_CHR( owner ) || !ChrObjList.get_data_ref( owner ).alive ) )
    {
        log_warning( "spawn_one_enchant() - failed because the required enchant owner cannot be found.\n" );
        return ENC_REF( MAX_ENC );
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
            loc_target = CHR_REF( MAX_CHR );
        }
    }

    // make sure the loc_target is valid
    if ( !INGAME_CHR( loc_target ) || !ptarget->alive )
    {
        log_warning( "spawn_one_enchant() - failed because the target is not alive.\n" );
        return ENC_REF( MAX_ENC );
    }
    ptarget = ChrObjList.get_data_ptr( loc_target );

    // Check peve->dontdamagetype
    if ( peve->dontdamagetype != DAMAGE_NONE )
    {
        if ( GET_DAMAGE_RESIST( ptarget->damagemodifier[peve->dontdamagetype] ) >= 3 ||
             HAS_SOME_BITS( ptarget->damagemodifier[peve->dontdamagetype], DAMAGECHARGE ) )
        {
            log_warning( "spawn_one_enchant() - failed because the target is immune to the enchant.\n" );
            return ENC_REF( MAX_ENC );
        }
    }

    // Check peve->onlydamagetype
    if ( peve->onlydamagetype != DAMAGE_NONE )
    {
        if ( ptarget->damagetargettype != peve->onlydamagetype )
        {
            log_warning( "spawn_one_enchant() - failed because the target not have the right damagetargettype.\n" );
            return ENC_REF( MAX_ENC );
        }
    }

    // Find an enchant index to use
    ego_enc_ref = EncObjList.allocate( ego_enc_override );

    if ( !VALID_ENC( ego_enc_ref ) )
    {
        log_warning( "spawn_one_enchant() - could not allocate an enchant.\n" );
        return ENC_REF( MAX_ENC );
    }
    pobj = EncObjList.get_data_ptr( ego_enc_ref );
    penc = ego_obj_enc::get_data_ptr( pobj );

    penc->spawn_data.owner_ref   = owner;
    penc->spawn_data.target_ref  = loc_target;
    penc->spawn_data.spawner_ref = spawner;
    penc->spawn_data.profile_ref = loc_profile;
    penc->spawn_data.eve_ref     = eve_ref;

    // actually force the character to spawn
    if ( NULL != ego_object_engine::run_activate( pobj, 100 ) )
    {
        peve->ego_enc_create_count++;
    }

    return ego_enc_ref;
}

//--------------------------------------------------------------------------------------------
EVE_REF load_one_enchant_profile_vfs( const char* szLoadName, const EVE_REF & ieve )
{
    /// @details ZZ@> This function loads an enchantment profile into the EveStack

    EVE_REF retval = EVE_REF( MAX_EVE );

    if ( EveStack.in_range_ref( ieve ) )
    {
        ego_eve * peve = EveStack + ieve;

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
void ego_enc::remove_set( const ENC_REF & ienc, int value_idx )
{
    /// @details ZZ@> This function unsets a set value
    CHR_REF character;
    ego_enc * penc;
    ego_chr * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_SET ) return;

    penc = EncObjList.get_allocated_data_ptr( ienc );
    if ( NULL == penc ) return;

    if ( value_idx >= MAX_ENCHANT_SET || !penc->setyesno[value_idx] ) return;

    if ( !INGAME_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget   = ChrObjList.get_data_ptr( penc->target_ref );

    switch ( value_idx )
    {
        case SETDAMAGETYPE:
            ptarget->damagetargettype = penc->setsave[value_idx];
            break;

        case SETNUMBEROFJUMPS:
            ego_chr::set_jump_number_reset( ptarget, penc->setsave[value_idx] );
            break;

        case SETLIFEBARCOLOR:
            ptarget->life_color = penc->setsave[value_idx];
            break;

        case SETMANABARCOLOR:
            ptarget->mana_color = penc->setsave[value_idx];
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
            ego_chr::set_light( ptarget, penc->setsave[value_idx] );
            break;

        case SETALPHABLEND:
            ego_chr::set_alpha( ptarget, penc->setsave[value_idx] );
            break;

        case SETSHEEN:
            ego_chr::set_sheen( ptarget, penc->setsave[value_idx] );
            break;

        case SETFLYTOHEIGHT:
            ego_chr_data::set_fly_height( ptarget, penc->setsave[value_idx] );
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
            ego_chr::change_profile( character, ptarget->basemodel_ref, penc->setsave[value_idx], ENC_LEAVE_ALL );
            break;

        case SETCHANNEL:
            ptarget->canchannel = ( 0 != penc->setsave[value_idx] );
            break;
    }

    penc->setyesno[value_idx] = bfalse;
}

//--------------------------------------------------------------------------------------------
void ego_enc::remove_add( const ENC_REF & ienc, int value_idx )
{
    /// @details ZZ@> This function undoes cumulative modification to character stats

    float fvaluetoadd;
    int valuetoadd;
    CHR_REF character;
    ego_enc * penc;
    ego_chr * ptarget;

    if ( value_idx < 0 || value_idx >= MAX_ENCHANT_ADD ) return;

    penc = EncObjList.get_allocated_data_ptr( ienc );
    if ( NULL == penc ) return;

    if ( !INGAME_CHR( penc->target_ref ) ) return;
    character = penc->target_ref;
    ptarget = ChrObjList.get_data_ptr( penc->target_ref );

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
                ego_chr::set_maxaccel( ptarget, ptarget->maxaccel_reset - fvaluetoadd );
                break;

            case ADDRED:
                valuetoadd = penc->addsave[value_idx];
                ego_chr::set_redshift( ptarget, ptarget->gfx_inst.redshift - valuetoadd );
                break;

            case ADDGRN:
                valuetoadd = penc->addsave[value_idx];
                ego_chr::set_grnshift( ptarget, ptarget->gfx_inst.grnshift - valuetoadd );
                break;

            case ADDBLU:
                valuetoadd = penc->addsave[value_idx];
                ego_chr::set_blushift( ptarget, ptarget->gfx_inst.blushift - valuetoadd );
                break;

            case ADDDEFENSE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->defense -= valuetoadd;
                break;

            case ADDMANA:
                valuetoadd = penc->addsave[value_idx];
                ptarget->mana_max -= valuetoadd;
                ptarget->mana -= valuetoadd;
                if ( ptarget->mana < 0 ) ptarget->mana = 0;
                break;

            case ADDLIFE:
                valuetoadd = penc->addsave[value_idx];
                ptarget->life_max -= valuetoadd;
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
        ego_eve::init( EveStack + cnt );
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
bool_t release_one_eve( const EVE_REF & ieve )
{
    ego_eve * peve;

    if ( !EveStack.in_range_ref( ieve ) ) return bfalse;
    peve = EveStack + ieve;

    if ( !peve->loaded ) return btrue;

    ego_eve::init( peve );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void update_all_enchants()
{
    ENC_BEGIN_LOOP_DEFINED( ienc, penc )
    {
        ego_object_engine::run( ego_enc::get_obj_ptr( penc ) );
    }
    ENC_END_LOOP();

    // fix the stat timer
    if ( clock_enc_stat >= ONESECOND )
    {
        // Reset the clock
        clock_enc_stat -= ONESECOND;
    }
}

//--------------------------------------------------------------------------------------------
ENC_REF cleanup_enchant_list( const ENC_REF & ienc, ENC_REF * ego_enc_parent )
{
    /// @details BB@> remove all the dead enchants from the enchant list
    ///     and report back the first non-dead enchant in the list.

    bool_t ego_enc_used[MAX_ENC];

    int cnt;
    ENC_REF first_valid_enchant;
    ENC_REF ego_enc_now, ego_enc_next;

    if ( !EncObjList.in_range_ref( ienc ) ) return ENC_REF( MAX_ENC );

    // clear the list
    SDL_memset( ego_enc_used, 0, sizeof( ego_enc_used ) );

    // scan the list of enchants
    ego_enc_next            = ENC_REF( MAX_ENC );
    first_valid_enchant = ego_enc_now = ienc;
    for ( cnt = 0; ego_enc_now < MAX_ENC && ego_enc_next != ego_enc_now && cnt < MAX_ENC; cnt++ )
    {
        ego_enc_next = EncObjList.get_data_ref( ego_enc_now ).nextenchant_ref;

        // coerce the list of enchants to a valid value
        if ( !EncObjList.in_range_ref( ego_enc_next ) )
        {
            ego_enc_next = EncObjList.get_data_ref( ego_enc_now ).nextenchant_ref = MAX_ENC;
        }

        // fix any loops in the enchant list
        if ( ego_enc_used[( ego_enc_next ).get_value()] )
        {
            EncObjList.get_data_ref( ego_enc_now ).nextenchant_ref = MAX_ENC;
            break;
        }

        //( !INGAME_CHR( EncObjList.get_data_ref(ego_enc_now).target_ref ) && !EveStack[EncObjList.get_data_ref(ego_enc_now).eve_ref].stayiftargetdead )

        // remove any expired enchants
        if ( !INGAME_ENC( ego_enc_now ) )
        {
            remove_enchant( ego_enc_now, ego_enc_parent );
            ego_enc_used[( ego_enc_now ).get_value()] = btrue;
        }
        else
        {
            // store this enchant in the list of used enchants
            ego_enc_used[( ego_enc_now ).get_value()] = btrue;

            // keep track of the first valid enchant
            if ( MAX_ENC == first_valid_enchant )
            {
                first_valid_enchant = ego_enc_now;
            }
        }

        ego_enc_parent = &( EncObjList.get_data_ref( ego_enc_now ).nextenchant_ref );
        ego_enc_now    = ego_enc_next;
    }

    if ( ego_enc_now == ego_enc_next || cnt >= MAX_ENC )
    {
        // break the loop. this will only happen if the list is messed up.
        EncObjList.get_data_ref( ego_enc_now ).nextenchant_ref = ENC_REF( MAX_ENC );

        log_warning( "cleanup_enchant_list() - The enchantment list is corrupt!\n" );
    }

    return first_valid_enchant;
}

//--------------------------------------------------------------------------------------------
void cleanup_all_enchants()
{
    /// @details ZZ@> this function scans all the enchants and removes any dead ones.
    ///               this happens only once a loop

    ENC_BEGIN_LOOP_DEFINED( ienc, penc )
    {
        ENC_REF * ego_enc_lst;
        ego_eve   * peve;
        bool_t    do_remove;
        bool_t valid_owner, valid_target;

        ego_obj_enc * pobj = ego_enc::get_obj_ptr( penc );
        if ( NULL == pobj ) continue;

        // try to determine something about the parent
        ego_enc_lst = NULL;
        valid_target = bfalse;
        if ( INGAME_CHR( penc->target_ref ) )
        {
            valid_target = ChrObjList.get_data_ref( penc->target_ref ).alive;

            // this is linked to a known character
            ego_enc_lst = &( ChrObjList.get_data_ref( penc->target_ref ).firstenchant );
        }

        // try to determine if the owner exists and is alive
        valid_owner = bfalse;
        if ( INGAME_CHR( penc->owner_ref ) )
        {
            valid_owner = ChrObjList.get_data_ref( penc->owner_ref ).alive;
        }

        if ( !LOADED_EVE( penc->eve_ref ) )
        {
            // this should never happen
            EGOBOO_ASSERT( bfalse );
            continue;
        }
        peve = EveStack + penc->eve_ref;

        do_remove = bfalse;
        if ( WAITING_PBASE( pobj ) )
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
            if ( ChrObjList.get_data_ref( penc->owner_ref ).mana == 0 ) do_remove = btrue;
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
    ENC_BEGIN_LOOP_ACTIVE( cnt, penc )
    {
        ego_obj * pbase = ego_enc::get_obj_ptr( penc );
        if ( NULL == pbase ) continue;

        pbase->update_count++;
    }
    ENC_END_LOOP();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_obj_enc::request_terminate( const ENC_REF & ienc )
{
    /// @details BB@> Tell the game to get rid of this object and treat it
    ///               as if it was already dead
    ///
    /// @note ego_obj_enc::request_terminate() will force the game to
    ///       (eventually) call free_one_enchant_in_game() on this enchant

    return request_terminate( EncObjList.get_allocated_data_ptr( ienc ) );
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_enc::request_terminate( ego_obj_enc * pobj )
{
    /// @details BB@> Tell the game to get rid of this object and treat it
    ///               as if it was already dead
    ///
    /// @note ego_obj_enc::request_terminate() will force the game to
    ///       (eventually) call free_one_enchant_in_game() on this enchant

    if ( NULL == pobj || TERMINATED_PENC( pobj ) ) return bfalse;

    // wait for EncObjList.cleanup() to work its magic
    pobj->begin_waiting( );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//bool_t ego_obj_enc::request_terminate( ego_enc_bundle * pbdl_enc )
//{
//    bool_t retval;
//    if ( NULL == pbdl_enc ) return bfalse;
//
//    retval = ego_obj_enc::request_terminate( pbdl_enc->enc_ref );
//
//    if ( retval )
//    {
//        ego_enc_bundle::validate( pbdl_enc );
//    }
//
//    return retval;
//}
//
