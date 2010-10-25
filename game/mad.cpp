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

/// @file mad.c
/// @brief The files for handling Egoboo's internal model definitions
/// @details

#include "mad.h"

#include "md2.inl"

#include "file_formats/cap_file.h"
#include "particle.inl"

#include "log.h"
#include "script_compile.h"
#include "graphic.h"
#include "texture.h"
#include "sound.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

t_cpp_stack< ego_mad, MAX_MAD  > MadStack;

static STRING  szModelName     = EMPTY_CSTR;      ///< MD2 Model name
static char    cActionName[ACTION_COUNT][2];      ///< Two letter name code
static STRING  cActionComent[ACTION_COUNT];       ///< Strings explaining the action codes

static int    action_number( const char * cFrameName );
static void   action_check_copy_vfs( const char* loadname, const MAD_REF & imad );
static void   action_copy_correct( const MAD_REF & imad, int actiona, int actionb );

static void   mad_get_framefx( const char * cFrameName, const MAD_REF & model, int frame );
static void   mad_get_walk_frame( const MAD_REF & imad, int lip, int action );
static void   mad_make_framelip( const MAD_REF & imad, int action );
static void   mad_rip_actions( const MAD_REF & imad );

static void mad_finalize( const MAD_REF & imad );
static void mad_heal_actions( const MAD_REF & imad, const char * loadname );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//static void md2_fix_normals( const MAD_REF & imad );
//static void md2_get_transvertices( const MAD_REF & imad );
//static int  vertexconnected( md2_ogl_commandlist_t * pclist, int vertex );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void MadList_init()
{
    MAD_REF cnt;

    // initialize all ego_mad
    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        ego_mad * pmad = MadStack + cnt;

        // blank out all the data, including the obj_base data
        ego_mad::dtor_this( pmad );

        ego_mad::reconstruct( pmad );
    }
}

//--------------------------------------------------------------------------------------------
void MadList_dtor()
{
    MAD_REF cnt;

    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        ego_mad::dtor_this( MadStack + cnt );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int action_number( const char * cFrameName )
{
    /// @details ZZ@> This function returns the number of the action in cFrameName, or
    ///    it returns NOACTION if it could not find a match

    int cnt;
    char tmp_str[16];
    char first, second;

    sscanf( cFrameName, " %15s", tmp_str );

    first  = tmp_str[0];
    second = tmp_str[1];

    for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
    {
        if ( first == cActionName[cnt][0] && second == cActionName[cnt][1] )
        {
            return cnt;
        }
    }

    return NOACTION;
}

//--------------------------------------------------------------------------------------------
void action_copy_correct( const MAD_REF & imad, int actiona, int actionb )
{
    /// @details ZZ@> This function makes sure both actions are valid if either of them
    ///    are valid.  It will copy start and ends to mirror the valid action.

    ego_mad * pmad;

    if ( actiona < 0 || actiona >= ACTION_COUNT ) return;
    if ( actionb < 0 || actionb >= ACTION_COUNT ) return;

    if ( !LOADED_MAD( imad ) ) return;
    pmad = MadStack + imad;

    // With the new system using the action_map, this is all that is really necessary
    if ( ACTION_COUNT == pmad->action_map[actiona] )
    {
        if ( pmad->action_valid[actionb] )
        {
            pmad->action_map[actiona] = actionb;
        }
        else if ( ACTION_COUNT != pmad->action_map[actionb] )
        {
            pmad->action_map[actiona] = pmad->action_map[actionb];
        }
    }
    else if ( ACTION_COUNT == pmad->action_map[actionb] )
    {
        if ( pmad->action_valid[actiona] )
        {
            pmad->action_map[actionb] = actiona;
        }
        else if ( ACTION_COUNT != pmad->action_map[actiona] )
        {
            pmad->action_map[actionb] = pmad->action_map[actiona];
        }
    }

    //if ( pmad->action_valid[actiona] == pmad->action_valid[actionb] )
    //{
    //    // They are either both valid or both invalid, in either case we can't help
    //    return;
    //}
    //else
    //{
    //    // Fix the invalid one
    //    if ( !pmad->action_valid[actiona] )
    //    {
    //        // Fix actiona
    //        pmad->action_valid[actiona] = btrue;
    //        pmad->action_stt[actiona] = pmad->action_stt[actionb];
    //        pmad->action_end[actiona]   = pmad->action_end[actionb];
    //    }
    //    else
    //    {
    //        // Fix actionb
    //        pmad->action_valid[actionb] = btrue;
    //        pmad->action_stt[actionb] = pmad->action_stt[actiona];
    //        pmad->action_end[actionb]   = pmad->action_end[actiona];
    //    }
    //}
}

//--------------------------------------------------------------------------------------------
int mad_get_action( const MAD_REF & imad, int action )
{
    /// @detaills BB@> translate the action that was given into a valid action for the model
    ///
    /// returns ACTION_COUNT on a complete failure, or the default ACTION_DA if it exists

    int     retval;
    ego_mad * pmad;

    if ( !LOADED_MAD( imad ) ) return ACTION_COUNT;
    pmad = MadStack + imad;

    // you are pretty much guaranteed that ACTION_DA will be valid for a model,
    // I guess it could be invalid if the model had no frames or something
    retval = ACTION_DA;
    if ( !pmad->action_valid[ACTION_DA] )
    {
        retval = ACTION_COUNT;
    }

    // check for a valid action range
    if ( action < 0 || action > ACTION_COUNT ) return retval;

    // track down a valid value
    if ( pmad->action_valid[action] )
    {
        retval = action;
    }
    else if ( ACTION_COUNT != pmad->action_map[action] )
    {
        int cnt, tnc;

        // do a "recursive" search for a valid action
        // we should never really have to check more than once if the map is prepared
        // properly BUT you never can tell. Make sure we do not get a runaway loop by
        // you never go farther than ACTION_COUNT steps and that you never see the
        // original action again

        tnc = pmad->action_map[action];
        for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
        {
            if ( tnc >= ACTION_COUNT || tnc < 0 || tnc == action ) break;

            if ( pmad->action_valid[tnc] )
            {
                retval = tnc;
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD mad_get_actionfx( const MAD_REF & imad, int action )
{
    BIT_FIELD retval = EMPTY_BIT_FIELD;
    int cnt;
    ego_mad * pmad;
    ego_MD2_Model * md2;
    ego_MD2_Frame * frame_lst, * pframe;

    if ( !LOADED_MAD( imad ) ) return 0;
    pmad = MadStack + imad;

    md2 = MadStack[imad].md2_ptr;
    if ( NULL == md2 ) return 0;

    if ( action < 0 || action >= ACTION_COUNT ) return 0;

    if ( !pmad->action_valid[action] ) return 0;

    frame_lst = ( ego_MD2_Frame * )md2_get_Frames( md2 );
    for ( cnt = pmad->action_stt[action]; cnt <= pmad->action_end[action]; cnt++ )
    {
        pframe = frame_lst + cnt;

        ADD_BITS( retval, pframe->framefx );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void action_check_copy_vfs( const char* loadname, const MAD_REF & imad )
{
    /// @details ZZ@> This function copies a model's actions

    vfs_FILE *fileread;
    int actiona, actionb;
    char szOne[16] = EMPTY_CSTR;
    char szTwo[16] = EMPTY_CSTR;

    if ( !LOADED_MAD( imad ) ) return;

    fileread = vfs_openRead( loadname );
    if ( NULL == fileread ) return;

    while ( goto_colon( NULL, fileread, btrue ) )
    {
        fget_string( fileread, szOne, SDL_arraysize( szOne ) );
        actiona = action_which( szOne[0] );

        fget_string( fileread, szTwo, SDL_arraysize( szTwo ) );
        actionb = action_which( szTwo[0] );

        action_copy_correct( imad, actiona + 0, actionb + 0 );
        action_copy_correct( imad, actiona + 1, actionb + 1 );
        action_copy_correct( imad, actiona + 2, actionb + 2 );
        action_copy_correct( imad, actiona + 3, actionb + 3 );
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
int action_which( char cTmp )
{
    /// @details ZZ@> This function changes a letter into an action code
    int action;

    switch ( toupper( cTmp ) )
    {
            /// @note ZF@> Attack animation WALK is used for doing nothing (for example charging spells)
            ///            Make it default to ACTION_DA in this case.
        case 'W': action = ACTION_DA; break;
        case 'D': action = ACTION_DA; break;
        case 'U': action = ACTION_UA; break;
        case 'T': action = ACTION_TA; break;
        case 'C': action = ACTION_CA; break;
        case 'S': action = ACTION_SA; break;
        case 'B': action = ACTION_BA; break;
        case 'L': action = ACTION_LA; break;
        case 'X': action = ACTION_XA; break;
        case 'F': action = ACTION_FA; break;
        case 'P': action = ACTION_PA; break;
        case 'Z': action = ACTION_ZA; break;
        case 'H': action = ACTION_HA; break;
        case 'K': action = ACTION_KA; break;
        default:  action = ACTION_DA; break;
    }

    return action;
}

//--------------------------------------------------------------------------------------------
void mad_get_walk_frame( const MAD_REF & imad, int lip, int action )
{
    /// @details ZZ@> This helps make walking look right
    int frame = 0;
    int framesinaction, action_stt;
    ego_mad * pmad;

    if ( !LOADED_MAD( imad ) ) return;
    pmad = MadStack + imad;

    action = mad_get_action( imad, action );
    if ( ACTION_COUNT == action )
    {
        framesinaction = 1;
        action_stt     = pmad->action_stt[ACTION_DA];
    }
    else
    {
        framesinaction = pmad->action_end[action] - pmad->action_stt[action];
        action_stt     = pmad->action_stt[action];
    }

    for ( frame = 0; frame < 16; frame++ )
    {
        int framealong = 0;

        if ( framesinaction > 0 )
        {
            framealong = (( frame * framesinaction / 16 ) + 2 ) % framesinaction;
        }

        pmad->frameliptowalkframe[lip][frame] = action_stt + framealong;
    }
}

//--------------------------------------------------------------------------------------------
void mad_get_framefx( const char * cFrameName, const MAD_REF & imad, int frame )
{
    /// @details ZZ@> This function figures out the IFrame invulnerability, and Attack, Grab, and
    ///               Drop timings
    ///
    ///          BB@> made a bit more sturdy parser that is not going to confuse strings like "LCRA"
    ///               which would not crop up if the convention of L or R going first was applied universally.
    ///               However, there are existing (and common) models which use the opposite convention, leading
    ///               to the possibility that an fx string "LARC" could be interpreted as ACTLEFT, CHARRIGHT, *and*
    ///               ACTRIGHT.

    BIT_FIELD fx = 0;
    char name_action[16], name_fx[16];
    int name_count;
    int fields;
    int cnt;

    static int token_count = -1;
    static const char * tokens[] = { "I", "S", "F", "P", "A", "G", "D", "C",          /* the normal command tokens */
                                     "LA", "LG", "LD", "LC", "RA", "RG", "RD", "RC", NULL
                                   }; /* the "bad" token aliases */

    const char * ptmp, * ptmp_end;
    char *paction, *paction_end;

    ego_MD2_Model * md2;
    ego_MD2_Frame * pframe;

    if ( !MadStack.valid_ref( imad ) ) return;

    md2 = MadStack[imad].md2_ptr;
    if ( NULL == md2 ) return;

    // check for a valid frame number
    if ( frame >= md2_get_numFrames( md2 ) ) return;
    pframe = ( ego_MD2_Frame * )md2_get_Frames( md2 );
    pframe = pframe + frame;

    // this should only be initialized the first time through
    if ( token_count < 0 )
    {
        token_count = 0;
        for ( cnt = 0; NULL != tokens[token_count] && cnt < 256; cnt++ ) token_count++;
    }

    // set the default values
    fx = 0;
    pframe->framefx = fx;

    // check for a non-trivial frame name
    if ( !VALID_CSTR( cFrameName ) ) return;

    // skip over whitespace
    ptmp     = cFrameName;
    ptmp_end = cFrameName + 16;
    for ( /* nothing */; ptmp < ptmp_end && isspace( *ptmp ); ptmp++ ) {};

    // copy non-numerical text
    paction     = name_action;
    paction_end = name_action + 16;
    for ( /* nothing */; ptmp < ptmp_end && paction < paction_end && !isspace( *ptmp ); ptmp++, paction++ )
    {
        if ( isdigit( *ptmp ) ) break;
        *paction = *ptmp;
    }
    if ( paction < paction_end ) *paction = CSTR_END;

    name_fx[0] = CSTR_END;
    fields = sscanf( ptmp, "%d %15s", &name_count, name_fx );
    name_action[15] = CSTR_END;
    name_fx[15] = CSTR_END;

    // check for a non-trivial fx command
    if ( !VALID_CSTR( name_fx ) ) return;

    // scan the fx string for valid commands
    ptmp     = name_fx;
    ptmp_end = name_fx + 15;
    while ( CSTR_END != *ptmp && ptmp < ptmp_end )
    {
        size_t len;
        int token_index = -1;
        for ( cnt = 0; cnt < token_count; cnt++ )
        {
            len = strlen( tokens[cnt] );
            if ( 0 == strncmp( tokens[cnt], ptmp, len ) )
            {
                ptmp += len;
                token_index = cnt;
                break;
            }
        }

        if ( -1 == token_index )
        {
            // BB> disable this since the loader seems to be working perfectly fine as of 10-21-10
            // the problem is that models that are edited with certain editors pad the frame number with
            // zeros and that makes this interpreter think that there are excess unknown commands.

            //log_debug( "Model %s, frame %d, frame name \"%s\" has unknown frame effects command \"%s\"\n", szModelName, frame, cFrameName, ptmp );
            ptmp++;
        }
        else
        {
            bool_t bad_form = bfalse;
            switch ( token_index )
            {
                case  0: // "I" == invulnerable
                    ADD_BITS( fx, MADFX_INVICTUS );
                    break;

                case  1: // "S" == stop
                    ADD_BITS( fx, MADFX_STOP );
                    break;

                case  2: // "F" == footfall
                    ADD_BITS( fx, MADFX_FOOTFALL );
                    break;

                case  3: // "P" == poof
                    ADD_BITS( fx, MADFX_POOF );
                    break;

                case  4: // "A" == action

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        ADD_BITS( fx, ( 'L' == *ptmp ) ? MADFX_ACTLEFT : MADFX_ACTRIGHT );
                        ptmp++;
                    }
                    break;

                case  5: // "G" == grab

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        ADD_BITS( fx, ( 'L' == *ptmp ) ? MADFX_GRABLEFT : MADFX_GRABRIGHT );
                        ptmp++;
                    }
                    break;

                case  6: // "D" == drop

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        fx |= ( 'L' == *ptmp ) ? MADFX_DROPLEFT : MADFX_DROPRIGHT;
                        ptmp++;
                    }
                    break;

                case  7: // "C" == grab a character

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        ADD_BITS( fx, ( 'L' == *ptmp ) ? MADFX_CHARLEFT : MADFX_CHARRIGHT );
                        ptmp++;
                    }
                    break;

                case  8: // "LA"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_ACTLEFT );
                    break;

                case  9: // "LG"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_GRABLEFT );
                    break;

                case 10: // "LD"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_DROPLEFT );
                    break;

                case 11: // "LC"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_CHARLEFT );
                    break;

                case 12: // "RA"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_ACTRIGHT );
                    break;

                case 13: // "RG"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_GRABRIGHT );
                    break;

                case 14: // "RD"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_DROPRIGHT );
                    break;

                case 15: // "RC"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_CHARRIGHT );
                    break;
            }

            if ( bad_form && -1 != token_index )
            {
                log_warning( "Model %s, frame %d, frame name \"%s\" has a frame effects command in an improper configuration \"%s\"\n", szModelName, frame, cFrameName, tokens[token_index] );
            }
        }
    }

    pframe->framefx = fx;
}

//--------------------------------------------------------------------------------------------
void mad_make_framelip( const MAD_REF & imad, int action )
{
    /// @details ZZ@> This helps make walking look right

    int frame_count, frame, framesinaction;

    ego_mad       * pmad;
    ego_MD2_Model * md2;
    ego_MD2_Frame * frame_list, * pframe;

    if ( !LOADED_MAD( imad ) ) return;
    pmad = MadStack + imad;

    md2 = pmad->md2_ptr;
    if ( NULL == md2 ) return;

    action = mad_get_action( imad, action );
    if ( ACTION_COUNT == action || ACTION_DA == action ) return;

    if ( !pmad->action_valid[action] ) return;

    frame_count = md2_get_numFrames( md2 );
    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( md2 );

    framesinaction = pmad->action_end[action] - pmad->action_stt[action];

    for ( frame = pmad->action_stt[action]; frame < pmad->action_end[action]; frame++ )
    {
        if ( frame > frame_count ) continue;
        pframe = frame_list + frame;

        pframe->framelip = ( frame - pmad->action_stt[action] ) * 15 / framesinaction;
        pframe->framelip = ( pframe->framelip ) & 15;
    }
}

//--------------------------------------------------------------------------------------------
void mad_make_equally_lit( const MAD_REF & imad )
{
    /// @details ZZ@> This function makes ultra low poly models look better

    int cnt, vert;
    ego_MD2_Model * md2;
    int frame_count, vert_count;

    if ( !LOADED_MAD( imad ) ) return;

    md2         = MadStack[imad].md2_ptr;
    frame_count = md2_get_numFrames( md2 );
    vert_count  = md2_get_numVertices( md2 );

    for ( cnt = 0; cnt < frame_count; cnt++ )
    {
        ego_MD2_Frame * pframe = ( ego_MD2_Frame * )md2_get_Frames( md2 );
        for ( vert = 0; vert < vert_count; vert++ )
        {
            pframe->vertex_lst[vert].normal = EGO_AMBIENT_INDEX;
        }
    }
}

//--------------------------------------------------------------------------------------------
void load_action_names_vfs( const char* loadname )
{
    /// @details ZZ@> This function loads all of the 2 letter action names

    vfs_FILE* fileread;
    int cnt;

    char first = CSTR_END, second = CSTR_END;
    STRING comment;
    bool_t found;

    fileread = vfs_openRead( loadname );
    if ( !fileread ) return;

    for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
    {
        comment[0] = CSTR_END;

        found = bfalse;
        if ( goto_colon( NULL, fileread, bfalse ) )
        {
            if ( vfs_scanf( fileread, " %c%c %s", &first, &second, &comment ) >= 2 )
            {
                found = btrue;
            }
        }

        if ( found )
        {
            cActionName[cnt][0] = first;
            cActionName[cnt][1] = second;
            cActionComent[cnt][0] = CSTR_END;

            if ( VALID_CSTR( comment ) )
            {
                strncpy( cActionComent[cnt], comment, SDL_arraysize( cActionComent[cnt] ) );
                cActionComent[cnt][255] = CSTR_END;
            }
        }
        else
        {
            cActionName[cnt][0] = CSTR_END;
            cActionComent[cnt][0] = CSTR_END;
        }
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
MAD_REF load_one_model_profile_vfs( const char* tmploadname, const MAD_REF & imad )
{
    ego_mad * pmad;
    STRING  newloadname;

    if ( !MadStack.valid_ref( imad ) ) return MAD_REF( MAX_MAD );
    pmad = MadStack + imad;

    // clear out the mad
    ego_mad::reconstruct( pmad );

    // mark it as used
    pmad->loaded = btrue;

    // Make up a name for the model...  IMPORT\TEMP0000.OBJ
    strncpy( pmad->name, tmploadname, SDL_arraysize( pmad->name ) );
    pmad->name[ SDL_arraysize( pmad->name ) - 1 ] = CSTR_END;

    // Load the imad model
    make_newloadname( tmploadname, "/tris.md2", newloadname );

    // do this for now. maybe make it dynamic later...
    //pmad->md2_ref = imad;

    // load the model from the file
    pmad->md2_ptr = ego_MD2_Model::load( vfs_resolveReadFilename( newloadname ), NULL );

    // set the model's file name
    szModelName[0] = CSTR_END;
    if ( NULL != pmad->md2_ptr )
    {
        strncpy( szModelName, vfs_resolveReadFilename( newloadname ), SDL_arraysize( szModelName ) );

        /// @details BB@> Egoboo md2 models were designed with 1 tile = 32x32 units, but internally Egoboo uses
        ///      1 tile = 128x128 units. Previously, this was handled by sprinkling a bunch of
        ///      commands that multiplied various quantities by 4 or by 4.125 throughout the code.
        ///      It was very counter-intuitive, and caused me no end of headaches...  Of course the
        ///      solution is to scale the model!
        ego_MD2_Model::scale( pmad->md2_ptr, -3.5, 3.5, 3.5 );
    }

    // Create the actions table for this imad
    mad_rip_actions( imad );
    mad_heal_actions( imad, tmploadname );
    mad_finalize( imad );

    return imad;
}

//--------------------------------------------------------------------------------------------
void mad_heal_actions( const MAD_REF & imad, const char * tmploadname )
{
    STRING newloadname;

    if ( !LOADED_MAD( imad ) ) return;

    // Make sure actions are made valid if a similar one exists
    action_copy_correct( imad, ACTION_DA, ACTION_DB );  // All dances should be safe
    action_copy_correct( imad, ACTION_DB, ACTION_DC );
    action_copy_correct( imad, ACTION_DC, ACTION_DD );
    action_copy_correct( imad, ACTION_DB, ACTION_DC );
    action_copy_correct( imad, ACTION_DA, ACTION_DB );
    action_copy_correct( imad, ACTION_UA, ACTION_UB );
    action_copy_correct( imad, ACTION_UB, ACTION_UC );
    action_copy_correct( imad, ACTION_UC, ACTION_UD );
    action_copy_correct( imad, ACTION_TA, ACTION_TB );
    action_copy_correct( imad, ACTION_TC, ACTION_TD );
    action_copy_correct( imad, ACTION_CA, ACTION_CB );
    action_copy_correct( imad, ACTION_CC, ACTION_CD );
    action_copy_correct( imad, ACTION_SA, ACTION_SB );
    action_copy_correct( imad, ACTION_SC, ACTION_SD );
    action_copy_correct( imad, ACTION_BA, ACTION_BB );
    action_copy_correct( imad, ACTION_BC, ACTION_BD );
    action_copy_correct( imad, ACTION_LA, ACTION_LB );
    action_copy_correct( imad, ACTION_LC, ACTION_LD );
    action_copy_correct( imad, ACTION_XA, ACTION_XB );
    action_copy_correct( imad, ACTION_XC, ACTION_XD );
    action_copy_correct( imad, ACTION_FA, ACTION_FB );
    action_copy_correct( imad, ACTION_FC, ACTION_FD );
    action_copy_correct( imad, ACTION_PA, ACTION_PB );
    action_copy_correct( imad, ACTION_PC, ACTION_PD );
    action_copy_correct( imad, ACTION_ZA, ACTION_ZB );
    action_copy_correct( imad, ACTION_ZC, ACTION_ZD );
    action_copy_correct( imad, ACTION_WA, ACTION_WB );
    action_copy_correct( imad, ACTION_WB, ACTION_WC );
    action_copy_correct( imad, ACTION_WC, ACTION_WD );
    action_copy_correct( imad, ACTION_DA, ACTION_WD );  // All walks should be safe
    action_copy_correct( imad, ACTION_WC, ACTION_WD );
    action_copy_correct( imad, ACTION_WB, ACTION_WC );
    action_copy_correct( imad, ACTION_WA, ACTION_WB );
    action_copy_correct( imad, ACTION_JA, ACTION_JB );
    action_copy_correct( imad, ACTION_JB, ACTION_JC );
    action_copy_correct( imad, ACTION_DA, ACTION_JC );  // All jumps should be safe
    action_copy_correct( imad, ACTION_JB, ACTION_JC );
    action_copy_correct( imad, ACTION_JA, ACTION_JB );
    action_copy_correct( imad, ACTION_HA, ACTION_HB );
    action_copy_correct( imad, ACTION_HB, ACTION_HC );
    action_copy_correct( imad, ACTION_HC, ACTION_HD );
    action_copy_correct( imad, ACTION_HB, ACTION_HC );
    action_copy_correct( imad, ACTION_HA, ACTION_HB );
    action_copy_correct( imad, ACTION_KA, ACTION_KB );
    action_copy_correct( imad, ACTION_KB, ACTION_KC );
    action_copy_correct( imad, ACTION_KC, ACTION_KD );
    action_copy_correct( imad, ACTION_KB, ACTION_KC );
    action_copy_correct( imad, ACTION_KA, ACTION_KB );
    action_copy_correct( imad, ACTION_MH, ACTION_MI );
    action_copy_correct( imad, ACTION_DA, ACTION_MM );
    action_copy_correct( imad, ACTION_MM, ACTION_MN );

    // Copy entire actions to save frame space COPY.TXT
    make_newloadname( tmploadname, "/copy.txt", newloadname );
    action_check_copy_vfs( newloadname, imad );
}

//--------------------------------------------------------------------------------------------
void mad_finalize( const MAD_REF & imad )
{
    int frame, frame_count;

    ego_mad       * pmad;
    ego_MD2_Model * pmd2;
    ego_MD2_Frame * frame_list;

    if ( !LOADED_MAD( imad ) ) return;
    pmad = MadStack + imad;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return;

    frame_count = md2_get_numFrames( pmd2 );
    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( pmd2 );

    // Create table for doing transition from one type of walk to another...
    // Clear 'em all to start
    for ( frame = 0; frame < frame_count; frame++ )
    {
        frame_list[frame].framelip = 0;
    }

    // Need to figure out how far into action each frame is
    mad_make_framelip( imad, ACTION_WA );
    mad_make_framelip( imad, ACTION_WB );
    mad_make_framelip( imad, ACTION_WC );

    // Now do the same, in reverse, for walking animations
    mad_get_walk_frame( imad, LIPDA, ACTION_DA );
    mad_get_walk_frame( imad, LIPWA, ACTION_WA );
    mad_get_walk_frame( imad, LIPWB, ACTION_WB );
    mad_get_walk_frame( imad, LIPWC, ACTION_WC );
}

//--------------------------------------------------------------------------------------------
void mad_rip_actions( const MAD_REF & imad )
{
    /// @details ZZ@> This function creates the iframe lists for each action based on the
    ///    name of each md2 iframe in the model

    int frame_count, iframe, framesinaction;
    int action, lastaction;

    ego_mad          * pmad;
    ego_MD2_Model    * pmd2;
    ego_MD2_Frame    * frame_list;

    if ( !LOADED_MAD( imad ) ) return;
    pmad = MadStack + imad;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return;

    // Clear out all actions and reset to invalid
    for ( action = 0; action < ACTION_COUNT; action++ )
    {
        pmad->action_map[action]   = ACTION_COUNT;
        pmad->action_valid[action] = bfalse;
    }

    // Set the primary dance action to be the first iframe, just as a default
    pmad->action_map[ACTION_DA]   = ACTION_DA;
    pmad->action_valid[ACTION_DA] = btrue;
    pmad->action_stt[ACTION_DA]   = 0;
    pmad->action_end[ACTION_DA]   = 1;

    frame_count = md2_get_numFrames( pmd2 );
    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( pmd2 );

    // Now go huntin' to see what each iframe is, look for runs of same action
    lastaction     = action_number( frame_list[0].name );
    framesinaction = 0;
    for ( iframe = 0; iframe < frame_count; iframe++ )
    {
        action = action_number( frame_list[iframe].name );
        pmad->action_map[action] = action;

        if ( lastaction == action )
        {
            framesinaction++;
        }
        else
        {
            // Write the old action
            if ( lastaction < ACTION_COUNT )
            {
                pmad->action_valid[lastaction] = btrue;
                pmad->action_stt[lastaction]   = iframe - framesinaction;
                pmad->action_end[lastaction]   = iframe;
            }

            framesinaction = 1;
            lastaction = action;
        }

        mad_get_framefx( frame_list[iframe].name, imad, iframe );
    }

    // Write the old action
    if ( lastaction < ACTION_COUNT )
    {
        pmad->action_valid[lastaction] = btrue;
        pmad->action_stt[lastaction]   = iframe - framesinaction;
        pmad->action_end[lastaction]   = iframe;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mad * ego_mad::clear( ego_mad * pmad )
{
    if ( NULL == pmad ) return pmad;

    memset( pmad, 0, sizeof( *pmad ) );

    return pmad;
}

//--------------------------------------------------------------------------------------------
bool_t ego_mad::dealloc( ego_mad * pmad )
{
    /// Free all allocated memory

    if ( NULL == pmad || !pmad->loaded ) return bfalse;

    ego_MD2_Model::destroy( &( pmad->md2_ptr ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mad * ego_mad::reconstruct( ego_mad * pmad )
{
    /// @details BB@> initialize the character data to safe values
    ///     since we use memset(..., 0, ...), all = 0, = false, and = 0.0f
    ///     statements are redundant

    int action;

    if ( NULL == pmad ) return NULL;

    ego_mad::dealloc( pmad );

    pmad = ego_mad::clear( pmad );

    strncpy( pmad->name, "*NONE*", SDL_arraysize( pmad->name ) );

    // Clear out all actions and reset to invalid
    for ( action = 0; action < ACTION_COUNT; action++ )
    {
        pmad->action_map[action]   = ACTION_COUNT;
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * ego_mad::ctor_this( ego_mad * pmad )
{
    if ( NULL == ego_mad::reconstruct( pmad ) ) return NULL;

    // nothing to do yet

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * ego_mad::dtor_this( ego_mad * pmad )
{
    /// @details BB@> deinitialize the character data

    if ( NULL == pmad || !pmad->loaded ) return NULL;

    // deinitialize the object
    ego_mad::reconstruct( pmad );

    // "destruct" the base object
    pmad->loaded = bfalse;

    return pmad;
}

//--------------------------------------------------------------------------------------------
void init_all_mad()
{
    MAD_REF cnt;

    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        ego_mad::reconstruct( MadStack + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_mad()
{
    MAD_REF cnt;

    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        release_one_mad( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_mad( const MAD_REF & imad )
{
    ego_mad * pmad;

    if ( !MadStack.valid_ref( imad ) ) return bfalse;
    pmad = MadStack + imad;

    if ( !pmad->loaded ) return btrue;

    // free any md2 data
    ego_mad::reconstruct( pmad );

    return btrue;
}

//--------------------------------------------------------------------------------------------
int randomize_action( int action, int slot )
{
    /// @details BB@> this function actually determines whether the action fillows the
    ///               pattern of ACTION_?A, ACTION_?B, ACTION_?C, ACTION_?D, with
    ///               A and B being for the left hand, and C and D being for the right hand

    int diff = 0;

    // a valid slot?
    if ( slot < 0 || slot >= SLOT_COUNT ) return action;

    // a valid action?
    if ( action < 0 || action >= ACTION_COUNT ) return bfalse;

    diff = slot * 2;

    //---- non-randomizable actions
    if ( ACTION_MG == action ) return action;       // MG      = Open Chest
    else if ( ACTION_MH == action ) return action;       // MH      = Sit
    else if ( ACTION_MI == action ) return action;       // MI      = Ride
    else if ( ACTION_MJ == action ) return action;       // MJ      = Object Activated
    else if ( ACTION_MK == action ) return action;       // MK      = Snoozing
    else if ( ACTION_ML == action ) return action;       // ML      = Unlock
    else if ( ACTION_JA == action ) return action;       // JA      = Jump
    else if ( ACTION_RA == action ) return action;       // RA      = Roll
    else if ( ACTION_IS_TYPE( action, W ) ) return action;  // WA - WD = Walk

    //---- do a couple of special actions that have left/right
    else if ( ACTION_EA == action || ACTION_EB == action ) action = ACTION_JB + slot;   // EA/EB = Evade left/right
    else if ( ACTION_JB == action || ACTION_JC == action ) action = ACTION_JB + slot;   // JB/JC = Dropped item left/right
    else if ( ACTION_MA == action || ACTION_MB == action ) action = ACTION_MA + slot;   // MA/MB = Drop left/right item
    else if ( ACTION_MC == action || ACTION_MD == action ) action = ACTION_MC + slot;   // MC/MD = Slam left/right
    else if ( ACTION_ME == action || ACTION_MF == action ) action = ACTION_ME + slot;   // ME/MF = Grab item left/right
    else if ( ACTION_MM == action || ACTION_MN == action ) action = ACTION_MM + slot;   // MM/MN = Held left/right

    //---- actions that can be randomized, but are not left/right sensitive
    // D = dance
    else if ( ACTION_IS_TYPE( action, D ) ) action = ACTION_TYPE( D ) + generate_randmask( 0, 3 );

    //---- handle all the normal attack/defense animations
    // U = unarmed
    else if ( ACTION_IS_TYPE( action, U ) ) action = ACTION_TYPE( U ) + diff + generate_randmask( 0, 1 );
    // T = thrust
    else if ( ACTION_IS_TYPE( action, T ) ) action = ACTION_TYPE( T ) + diff + generate_randmask( 0, 1 );
    // C = chop
    else if ( ACTION_IS_TYPE( action, C ) ) action = ACTION_TYPE( C ) + diff + generate_randmask( 0, 1 );
    // S = slice
    else if ( ACTION_IS_TYPE( action, S ) ) action = ACTION_TYPE( S ) + diff + generate_randmask( 0, 1 );
    // B = bash
    else if ( ACTION_IS_TYPE( action, B ) ) action = ACTION_TYPE( B ) + diff + generate_randmask( 0, 1 );
    // L = longbow
    else if ( ACTION_IS_TYPE( action, L ) ) action = ACTION_TYPE( L ) + diff + generate_randmask( 0, 1 );
    // X = crossbow
    else if ( ACTION_IS_TYPE( action, X ) ) action = ACTION_TYPE( X ) + diff + generate_randmask( 0, 1 );
    // F = fling
    else if ( ACTION_IS_TYPE( action, F ) ) action = ACTION_TYPE( F ) + diff + generate_randmask( 0, 1 );
    // P = parry/block
    else if ( ACTION_IS_TYPE( action, P ) ) action = ACTION_TYPE( P ) + diff + generate_randmask( 0, 1 );
    // Z = zap
    else if ( ACTION_IS_TYPE( action, Z ) ) action = ACTION_TYPE( Z ) + diff + generate_randmask( 0, 1 );

    //---- these are passive actions
    // H = hurt
    else if ( ACTION_IS_TYPE( action, H ) ) action = ACTION_TYPE( H ) + generate_randmask( 0, 3 );
    // K = killed
    else if ( ACTION_IS_TYPE( action, K ) ) action = ACTION_TYPE( K ) + generate_randmask( 0, 3 );

    return action;
}

////--------------------------------------------------------------------------------------------
//Uint16 test_frame_name( char letter )
//{
//    /// @details ZZ@> This function returns btrue if the 4th, 5th, 6th, or 7th letters
//    ///    of the frame name matches the input argument
//
//    if ( cFrameName[4] == letter ) return btrue;
//    if ( cFrameName[4] == 0 ) return bfalse;
//    if ( cFrameName[5] == letter ) return btrue;
//    if ( cFrameName[5] == 0 ) return bfalse;
//    if ( cFrameName[6] == letter ) return btrue;
//    if ( cFrameName[6] == 0 ) return bfalse;
//    if ( cFrameName[7] == letter ) return btrue;
//
//    return bfalse;
//}

//--------------------------------------------------------------------------------------------
//void md2_fix_normals( const MAD_REF & imad )
//{
//    /// @details ZZ@> This function helps light not flicker so much
//    int cnt, tnc;
//    int indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
//    int indexofnextnextnextnext;
//    int frame;
//
//    frame = ego_md2_data[MadStack[imad].md2_ref].framestart;
//    cnt = 0;
//
//    while ( cnt < ego_md2_data[MadStack[imad].md2_ref].vertex_lst )
//    {
//        tnc = 0;
//
//        while ( tnc < ego_md2_data[MadStack[imad].md2_ref].frames )
//        {
//            indexofcurrent = pframe->vrta[cnt];
//            indexofnext = Md2FrameList[frame+1].vrta[cnt];
//            indexofnextnext = Md2FrameList[frame+2].vrta[cnt];
//            indexofnextnextnext = Md2FrameList[frame+3].vrta[cnt];
//            indexofnextnextnextnext = Md2FrameList[frame+4].vrta[cnt];
//            if ( indexofcurrent == indexofnextnext && indexofnext != indexofcurrent )
//            {
//                Md2FrameList[frame+1].vrta[cnt] = indexofcurrent;
//            }
//            if ( indexofcurrent == indexofnextnextnext )
//            {
//                if ( indexofnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+1].vrta[cnt] = indexofcurrent;
//                }
//                if ( indexofnextnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+2].vrta[cnt] = indexofcurrent;
//                }
//            }
//            if ( indexofcurrent == indexofnextnextnextnext )
//            {
//                if ( indexofnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+1].vrta[cnt] = indexofcurrent;
//                }
//                if ( indexofnextnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+2].vrta[cnt] = indexofcurrent;
//                }
//                if ( indexofnextnextnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+3].vrta[cnt] = indexofcurrent;
//                }
//            }
//
//            tnc++;
//        }
//
//        cnt++;
//    }
//}

//--------------------------------------------------------------------------------------------
//void md2_get_transvertices( const MAD_REF & imad )
//{
//    /// @details ZZ@> This function gets the number of vertices to transform for a model...
//    //    That means every one except the grip ( unconnected ) vertices
//
//    // if (imad == 0)
//    // {
//    //   for ( cnt = 0; cnt < MadStack[imad].vertex_lst; cnt++ )
//    //   {
//    //       printf("%d-%d\n", cnt, vertexconnected( imad, cnt ) );
//    //   }
//    // }
//
//    MadStack[imad].transvertices = ego_md2_data[MadStack[imad].md2_ref].vertex_lst;
//}

//--------------------------------------------------------------------------------------------
/*int vertexconnected( md2_ogl_commandlist_t * pclist, int vertex )
{
    /// @details ZZ@> This function returns 1 if the model vertex is connected, 0 otherwise
    int cnt, tnc, entry;

    entry = 0;

    for ( cnt = 0; cnt < pclist->count; cnt++ )
    {
        for ( tnc = 0; tnc < pclist->size[cnt]; tnc++ )
        {
            if ( pclist->vrt[entry] == vertex )
            {
                // The vertex is used
                return 1;
            }

            entry++;
        }
    }

    // The vertex is not used
    return 0;
}

*/

////--------------------------------------------------------------------------------------------
//int action_frame()
//{
//    /// @details ZZ@> This function returns the frame number in the third and fourth characters
//    ///    of cFrameName
//
//    int number;
//    char tmp_str[16];
//
//    sscanf( cFrameName, " %15s%d", tmp_str, &number );
//
//    return number;
//}

