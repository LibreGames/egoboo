#include "Mad.h"

#include "script.h"
#include "Log.h"

#include "egoboo_utility.h"

#include <assert.h>

#include "Md2.inl"
#include "particle.inl"
#include "graphic.inl"
#include "egoboo_types.inl"

float  spek_global[MAXLIGHTROTATION][MD2LIGHTINDICES];
float  spek_local[MAXLIGHTROTATION][MD2LIGHTINDICES];
float  indextoenvirox[MD2LIGHTINDICES];                    // Environment map
float  lighttoenviroy[256];                                // Environment map
Uint32 lighttospek[MAXSPEKLEVEL][256];                     //



//--------------------------------------------------------------------------------------------

static bool_t test_frame_name( char * szName, char letter );

//--------------------------------------------------------------------------------------------
ACTION action_number(char * szName)
{
  // ZZ> This function returns the number of the action in cFrameName, or
  //     it returns ACTION_INVALID if it could not find a match

  ACTION cnt;

  if( !VALID_CSTR(szName)  || EOS == szName[1]) return ACTION_INVALID;

  for ( cnt = 0; cnt < MAXACTION; cnt++ )
  {
    if ( 0 == strncmp(szName, cActionName[cnt], 2) ) return cnt;
  }

  return ACTION_INVALID;
}

//--------------------------------------------------------------------------------------------
Uint16 action_frame()
{
  // ZZ> This function returns the frame number in the third and fourth characters
  //     of cFrameName

  int number;
  sscanf( cFrameName + 2, "%d", &number );
  return number;
}

//--------------------------------------------------------------------------------------------
bool_t test_frame_name( char * szName, char letter )
{
  // ZZ> This function returns btrue if the 4th, 5th, 6th, or 7th letters
  //     of the frame name matches the input argument

  int i;

  if(NULL == szName ) return bfalse;

  // max md2 frame name is 15 characters
  for(i=4; i<15; i++)
  {
    if ( EOS   == szName[i] ) break;
    if ( letter == szName[i] ) return btrue;
  }

  return bfalse;
}

//--------------------------------------------------------------------------------------------
void action_copy_correct( Game_t * gs, MAD_REF imad, ACTION actiona, ACTION actionb )
{
  // ZZ> This function makes sure both actions are valid if either of them
  //     are valid.  It will copy start and ends to mirror the valid action.

  Mad_t * pmad;

  if( !LOADED_MAD(gs->MadList, imad) ) return;
  pmad = gs->MadList + imad;

  if ( pmad->actionvalid[actiona] == pmad->actionvalid[actionb] )
  {
    // They are either both valid or both invalid, in either case we can't help
  }
  else
  {
    // Fix the invalid one
    if ( !pmad->actionvalid[actiona] )
    {
      // Fix actiona
      pmad->actionvalid[actiona] = btrue;
      pmad->actionstart[actiona] = pmad->actionstart[actionb];
      pmad->actionend[actiona] = pmad->actionend[actionb];
    }
    else
    {
      // Fix actionb
      pmad->actionvalid[actionb] = btrue;
      pmad->actionstart[actionb] = pmad->actionstart[actiona];
      pmad->actionend[actionb] = pmad->actionend[actiona];
    }
  }
}

//--------------------------------------------------------------------------------------------
void get_walk_frame( Game_t * gs, MAD_REF imad, LIPT lip_trans, ACTION action )
{
  // ZZ> This helps make walking look right

  Mad_t * pmad;
  int frame = 0;
  int framesinaction = 0;

  if( !LOADED_MAD(gs->MadList, imad) ) return;
  pmad = gs->MadList + imad;

  framesinaction = pmad->actionend[action] - pmad->actionstart[action];

  while ( frame < 16 )
  {
    int framealong = 0;
    if ( framesinaction > 0 )
    {
      framealong = (( float )( frame * framesinaction ) / ( float ) MAXFRAMESPERANIM ) + 2;
      framealong %= framesinaction;
    }
    pmad->frameliptowalkframe[lip_trans][frame] = pmad->actionstart[action] + framealong;
    frame++;
  }
}

//--------------------------------------------------------------------------------------------
Uint16 get_framefx( char * szName )
{
  // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
  //     Drop timings

  Uint16 fx = 0;

  if( !VALID_CSTR(szName)  || EOS == szName[1] ) return 0;

  if ( test_frame_name( szName, 'I' ) ) fx |= MADFX_INVICTUS;
  if ( test_frame_name( szName, 'S' ) ) fx |= MADFX_STOP;
  if ( test_frame_name( szName, 'F' ) ) fx |= MADFX_FOOTFALL;
  if ( test_frame_name( szName, 'P' ) ) fx |= MADFX_POOF;

  if ( test_frame_name( szName, 'L' ) )
  {
    if ( test_frame_name( szName, 'A' ) ) fx |= MADFX_ACTLEFT;
    if ( test_frame_name( szName, 'G' ) ) fx |= MADFX_GRABLEFT;
    if ( test_frame_name( szName, 'D' ) ) fx |= MADFX_DROPLEFT;
    if ( test_frame_name( szName, 'C' ) ) fx |= MADFX_CHARLEFT;
  }

  if ( test_frame_name( szName, 'R' ) )
  {
    if ( test_frame_name( szName, 'A' ) ) fx |= MADFX_ACTRIGHT;
    if ( test_frame_name( szName, 'G' ) ) fx |= MADFX_GRABRIGHT;
    if ( test_frame_name( szName, 'D' ) ) fx |= MADFX_DROPRIGHT;
    if ( test_frame_name( szName, 'C' ) ) fx |= MADFX_CHARRIGHT;
  }



  return fx;
}

//--------------------------------------------------------------------------------------------
void make_framelip( Game_t * gs, MAD_REF imad, ACTION action )
{
  // ZZ> This helps make walking look right

  Mad_t * pmad;
  int frame, framesinaction;

  if( !LOADED_MAD(gs->MadList, imad) ) return;
  pmad = gs->MadList + imad;

  if ( pmad->actionvalid[action] )
  {
    framesinaction = pmad->actionend[action] - pmad->actionstart[action];
    frame = pmad->actionstart[action];
    while ( frame < pmad->actionend[action] )
    {
      pmad->framelip[frame] = ( frame - pmad->actionstart[action] ) * 15 / framesinaction;
      pmad->framelip[frame] = ( pmad->framelip[frame] ) % 16;
      frame++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void get_actions( Game_t * gs, MAD_REF imad )
{
  // ZZ> This function creates the iframe lists for each action based on the
  //     name of each md2 iframe in the model

  ACTION      action, lastaction;
  MD2_Model_t * pmd2;
  Mad_t       * pmad;

  int    iframe, framesinaction;
  int    iFrameCount;
  char * szName;

  if( !VALID_MAD(gs->MadList, imad) ) return;
  pmad = gs->MadList + imad;

  pmd2 = pmad->md2_ptr;
  if(NULL == pmd2) return;

  // Clear out all actions and reset to invalid
  for (action = 0; action < MAXACTION; action++)
  {
    pmad->actionvalid[action] = bfalse;
  }

  iFrameCount = md2_get_numFrames(pmd2);
  if(0 == iFrameCount) return;

  // Set the primary dance action to be the first iframe, just as a default
  pmad->actionvalid[ACTION_DA] = btrue;
  pmad->actionstart[ACTION_DA] = 0;
  pmad->actionend[ACTION_DA]   = 1;


  // Now go huntin' to see what each iframe is, look for runs of same action
  szName = md2_get_Frame(pmd2, 0)->name;
  lastaction = action_number(szName);
  framesinaction = 0;
  for ( iframe = 0; iframe < iFrameCount; iframe++ )
  {
    const MD2_Frame_t * pFrame = md2_get_Frame(pmd2, iframe);
    szName = pFrame->name;
    action = action_number(szName);
    if ( lastaction == action )
    {
      framesinaction++;
    }
    else
    {
      // Write the old action
      if ( lastaction < MAXACTION )
      {
        pmad->actionvalid[lastaction] = btrue;
        pmad->actionstart[lastaction] = iframe - framesinaction;
        pmad->actionend[lastaction]   = iframe;
      }
      framesinaction = 1;
      lastaction = action;
    }
    pmad->framefx[iframe] = get_framefx( szName );
  }

  // Write the old action
  if ( lastaction < MAXACTION )
  {
    pmad->actionvalid[lastaction] = btrue;
    pmad->actionstart[lastaction] = iframe - framesinaction;
    pmad->actionend[lastaction]   = iframe;
  }

  // Make sure actions are made valid if a similar one exists
  action_copy_correct( gs, imad, ACTION_DA, ACTION_DB );   // All dances should be safe
  action_copy_correct( gs, imad, ACTION_DB, ACTION_DC );
  action_copy_correct( gs, imad, ACTION_DC, ACTION_DD );
  action_copy_correct( gs, imad, ACTION_DB, ACTION_DC );
  action_copy_correct( gs, imad, ACTION_DA, ACTION_DB );
  action_copy_correct( gs, imad, ACTION_UA, ACTION_UB );
  action_copy_correct( gs, imad, ACTION_UB, ACTION_UC );
  action_copy_correct( gs, imad, ACTION_UC, ACTION_UD );
  action_copy_correct( gs, imad, ACTION_TA, ACTION_TB );
  action_copy_correct( gs, imad, ACTION_TC, ACTION_TD );
  action_copy_correct( gs, imad, ACTION_CA, ACTION_CB );
  action_copy_correct( gs, imad, ACTION_CC, ACTION_CD );
  action_copy_correct( gs, imad, ACTION_SA, ACTION_SB );
  action_copy_correct( gs, imad, ACTION_SC, ACTION_SD );
  action_copy_correct( gs, imad, ACTION_BA, ACTION_BB );
  action_copy_correct( gs, imad, ACTION_BC, ACTION_BD );
  action_copy_correct( gs, imad, ACTION_LA, ACTION_LB );
  action_copy_correct( gs, imad, ACTION_LC, ACTION_LD );
  action_copy_correct( gs, imad, ACTION_XA, ACTION_XB );
  action_copy_correct( gs, imad, ACTION_XC, ACTION_XD );
  action_copy_correct( gs, imad, ACTION_FA, ACTION_FB );
  action_copy_correct( gs, imad, ACTION_FC, ACTION_FD );
  action_copy_correct( gs, imad, ACTION_PA, ACTION_PB );
  action_copy_correct( gs, imad, ACTION_PC, ACTION_PD );
  action_copy_correct( gs, imad, ACTION_ZA, ACTION_ZB );
  action_copy_correct( gs, imad, ACTION_ZC, ACTION_ZD );
  action_copy_correct( gs, imad, ACTION_WA, ACTION_WB );
  action_copy_correct( gs, imad, ACTION_WB, ACTION_WC );
  action_copy_correct( gs, imad, ACTION_WC, ACTION_WD );
  action_copy_correct( gs, imad, ACTION_DA, ACTION_WD );   // All walks should be safe
  action_copy_correct( gs, imad, ACTION_WC, ACTION_WD );
  action_copy_correct( gs, imad, ACTION_WB, ACTION_WC );
  action_copy_correct( gs, imad, ACTION_WA, ACTION_WB );
  action_copy_correct( gs, imad, ACTION_JA, ACTION_JB );
  action_copy_correct( gs, imad, ACTION_JB, ACTION_JC );
  action_copy_correct( gs, imad, ACTION_DA, ACTION_JC );  // All jumps should be safe
  action_copy_correct( gs, imad, ACTION_JB, ACTION_JC );
  action_copy_correct( gs, imad, ACTION_JA, ACTION_JB );
  action_copy_correct( gs, imad, ACTION_HA, ACTION_HB );
  action_copy_correct( gs, imad, ACTION_HB, ACTION_HC );
  action_copy_correct( gs, imad, ACTION_HC, ACTION_HD );
  action_copy_correct( gs, imad, ACTION_HB, ACTION_HC );
  action_copy_correct( gs, imad, ACTION_HA, ACTION_HB );
  action_copy_correct( gs, imad, ACTION_KA, ACTION_KB );
  action_copy_correct( gs, imad, ACTION_KB, ACTION_KC );
  action_copy_correct( gs, imad, ACTION_KC, ACTION_KD );
  action_copy_correct( gs, imad, ACTION_KB, ACTION_KC );
  action_copy_correct( gs, imad, ACTION_KA, ACTION_KB );
  action_copy_correct( gs, imad, ACTION_MH, ACTION_MI );
  action_copy_correct( gs, imad, ACTION_DA, ACTION_MM );
  action_copy_correct( gs, imad, ACTION_MM, ACTION_MN );


  // Create table for doing transition from one type of walk to another...
  // Clear 'em all to start
  for ( iframe = 0; iframe < iFrameCount; iframe++ )
  {
    pmad->framelip[iframe] = 0;
  }

  // Need to figure out how far into action each iframe is
  make_framelip( gs, imad, ACTION_WA );
  make_framelip( gs, imad, ACTION_WB );
  make_framelip( gs, imad, ACTION_WC );

  // Now do the same, in reverse, for walking animations
  get_walk_frame( gs, imad, LIPT_DA, ACTION_DA );
  get_walk_frame( gs, imad, LIPT_WA, ACTION_WA );
  get_walk_frame( gs, imad, LIPT_WB, ACTION_WB );
  get_walk_frame( gs, imad, LIPT_WC, ACTION_WC );
}

//--------------------------------------------------------------------------------------------
void make_mad_equally_lit( Game_t * gs, MAD_REF imad )
{
  // ZZ> This function makes ultra low poly models look better

  int frame, vert;
  int iFrames, iVerts;
  Mad_t * pmad;
  MD2_Model_t * m;

  if( !LOADED_MAD(gs->MadList, imad) ) return;
  pmad = gs->MadList + imad;

  if(NULL == pmad->md2_ptr) return;
  m = pmad->md2_ptr;

  iFrames = md2_get_numFrames(m);
  iVerts  = md2_get_numVertices(m);

  for ( frame = 0; frame < iFrames; frame++ )
  {
    const MD2_Frame_t * f = md2_get_Frame(m, frame);
    for ( vert = 0; vert < iVerts; vert++ )
    {
      f->vertices[vert].normal = EQUALLIGHTINDEX;
    }
  }

}

//--------------------------------------------------------------------------------------------
void load_copy_file( Game_t * gs, const char * szObjectpath, const char * szObjectname, MAD_REF object )
{
  // ZZ> This function copies a model's actions

  FILE *fileread;
  ACTION actiona, actionb;
  char szOne[16], szTwo[16];

  fileread = fs_fileOpen(PRI_NONE, "load_copy_file()", inherit_fname(szObjectpath, szObjectname, CData.copy_file), "r");
  if ( NULL != fileread )
  {
    while ( fget_next_string( fileread, szOne, sizeof( szOne ) ) )
    {
      actiona = what_action( szOne[0] );

      fget_string( fileread, szTwo, sizeof( szTwo ) );
      actionb = what_action( szTwo[0] );

      action_copy_correct( gs, object, (ACTION)(actiona + 0), (ACTION)(actionb + 0) );
      action_copy_correct( gs, object, (ACTION)(actiona + 1), (ACTION)(actionb + 1) );
      action_copy_correct( gs, object, (ACTION)(actiona + 2), (ACTION)(actionb + 2) );
      action_copy_correct( gs, object, (ACTION)(actiona + 3), (ACTION)(actionb + 3) );
    }
    fs_fileClose( fileread );
  }

}

//--------------------------------------------------------------------------------------------
bool_t bbox_list_contract(BBOX_LIST *pnew)
{
  // BB > make the list as small as possible by removing dead nodes

  int i, j;

  // check for invalid lists
  if(NULL == pnew) return bfalse;

  // if there are no nodes, there is nothing to do
  if(0 == pnew->count) return btrue;

  for(i=0, j=0; i<pnew->count; i++)
  {
    if(!pnew->list[i].used) continue;

    if(i!=j)
    {
      memcpy(pnew->list + j, pnew->list + i, sizeof(AA_BBOX));
    }

    j++;
  }

  if(j < pnew->count)
  {
    bbox_list_realloc(pnew, j);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
// the most complete way of doing this would be to find the nodes of an octree
// at the given level that contain triangles.
// HOWEVER, the math on that is kicking my @$$, so I will do something simpler
bool_t mad_generate_bbox_list(BBOX_LIST *plist, MD2_Model_t * pmd2, int frame, int bbox_divisions)
{
  int ix, iy, iz;
  int i, j;
  int num_per_axis, bbox_alloc_count, bbox_count;

  AA_BBOX tmp_bb;

  int               vrt_count;
  const MD2_Frame_t * pframe;

  int                  tri_count;
  const md2_triangle * ptri_lst;

  vect3 diff, origin;

  // erase any existing data in the BBOX_LIST
  if(NULL == bbox_list_renew(plist)) return bfalse;

  // make sure we have valid values
  if(NULL == pmd2) return bfalse;

  // check the triangle list
  if(0 == md2_get_numTriangles(pmd2) || NULL == md2_get_Triangles(pmd2)) return bfalse;
  tri_count = md2_get_numTriangles(pmd2);
  ptri_lst  = md2_get_Triangles(pmd2);

  // check for valid frame
  if(0 == md2_get_numFrames(pmd2) || frame > md2_get_numFrames(pmd2)) return bfalse;
  pframe    = md2_get_Frame(pmd2, frame);
  vrt_count = md2_get_numVertices(pmd2);

  // do some bbox pre-calculation
  origin.x = pframe->bbmin[0];
  origin.y = pframe->bbmin[1];
  origin.z = pframe->bbmin[2];

  diff.x = pframe->bbmax[0] - origin.x;
  diff.y = pframe->bbmax[1] - origin.y;
  diff.z = pframe->bbmax[2] - origin.z;

  // allocate a bbox list that has the required number of bboxes
  num_per_axis     = 1 << (  bbox_divisions);
  bbox_alloc_count = 1 << (3*bbox_divisions);
  bbox_list_alloc( plist, bbox_alloc_count );

  // initialize the data with "invalid" bboxes
  for(i=0; i<bbox_alloc_count; i++)
  {
    AA_BBOX * pbbox = plist->list + i;

    pbbox->used     = bfalse;
    pbbox->level    = bbox_divisions;
    pbbox->address  = i;
    pbbox->weight   = 1.0f;
    pbbox->sub_used = 8;
  }

  // for each triangle in the tri_list, calculate its bbox and,
  // generate a bbox in the plist for every "octree node" that it overlaps
  bbox_count = 0;
  for(i=0; i<tri_count; i++)
  {
    int ix_min,ix_max, iy_min,iy_max, iz_min,iz_max;

    // test the three vertices to make sure they are valid
    for(j=0;j<3;j++)
    {
      int ivrt = ptri_lst[i].vertexIndices[j];
      if(ivrt>vrt_count) break;

      if(0 == j)
      {
        tmp_bb.mins.x = tmp_bb.maxs.x = pframe->vertices[ivrt].x;
        tmp_bb.mins.y = tmp_bb.maxs.y = pframe->vertices[ivrt].y;
        tmp_bb.mins.z = tmp_bb.maxs.z = pframe->vertices[ivrt].z;
      }
      else
      {
        float ftmp;

        ftmp = pframe->vertices[ivrt].x;
        if(ftmp<tmp_bb.mins.x) tmp_bb.mins.x = ftmp;
        else if (ftmp>tmp_bb.maxs.x) tmp_bb.maxs.x = ftmp;

        ftmp = pframe->vertices[ivrt].y;
        if(ftmp<tmp_bb.mins.y) tmp_bb.mins.y = ftmp;
        else if (ftmp>tmp_bb.maxs.y) tmp_bb.maxs.y = ftmp;

        ftmp = pframe->vertices[ivrt].z;
        if(ftmp<tmp_bb.mins.z) tmp_bb.mins.z = ftmp;
        else if (ftmp>tmp_bb.maxs.z) tmp_bb.maxs.z = ftmp;
      }

    };

    // if a vertex is invalid, skip the triangle
    if(j<3) continue;

    // calculate the integer limits of the triangle's bbox
    ix_min = (tmp_bb.mins.x - origin.x) / diff.x * num_per_axis;
    ix_max = (tmp_bb.maxs.x - origin.x) / diff.x * num_per_axis;

    iy_min = (tmp_bb.mins.y - origin.y) / diff.y * num_per_axis;
    iy_max = (tmp_bb.maxs.y - origin.y) / diff.y * num_per_axis;

    iz_min = (tmp_bb.mins.z - origin.z) / diff.z * num_per_axis;
    iz_max = (tmp_bb.maxs.z - origin.z) / diff.z * num_per_axis;

    // insert valid nodes into the data
    for(ix=ix_min; ix<=ix_max; ix++)
    {
      for(iy=iy_min; iy<=iy_max; iy++)
      {
        for(iz=iz_min; iz<=iz_max; iz++)
        {
          AA_BBOX * pbbox;
          int idx;

          // calculate the index from the integer positions
          idx = ((ix & (num_per_axis-1)) << (0*bbox_divisions)) |
                ((iy & (num_per_axis-1)) << (1*bbox_divisions)) |
                ((iz & (num_per_axis-1)) << (2*bbox_divisions)) ;

          pbbox = plist->list + idx;

          if( !pbbox->used )
          {
            assert(pbbox->address == idx && pbbox->level == bbox_divisions);

            pbbox->used    = btrue;

            pbbox->mins.x =  (ix + 0) / (float)num_per_axis;
            pbbox->maxs.x =  (ix + 1) / (float)num_per_axis;

            pbbox->mins.y =  (iy + 0) / (float)num_per_axis;
            pbbox->maxs.y =  (iy + 1) / (float)num_per_axis;

            pbbox->mins.z =  (iz + 0) / (float)num_per_axis;
            pbbox->maxs.z =  (iz + 1) / (float)num_per_axis;

            bbox_count++;
          }
        }
      }
    }
  }

  //if(bbox_count == bbox_alloc_count)
  //{
  //  // this bbox is completely full. It doesn't help us at all
  //  bbox_list_delete(plist);
  //  return bfalse;
  //}
  //else if(bbox_count > bbox_alloc_count * 0.75f)
  //{
  //  // this bbox is not efficient enough. It doesn't help us at all
  //  bbox_list_delete(plist);
  //  return bfalse;
  //}
  //else
  {
    return bbox_list_contract(plist);
  };
};
//--------------------------------------------------------------------------------------------
bool_t mad_cull_bbox_list(int max_lod, BBOX_LIST *plst, BBOX_LIST *pcull)
{
  int ix, iy, iz;
  int i, j, k;
  int lst_divisions, lst_per_axis, cull_divisions, cull_per_axis;

  if(NULL == plst || NULL == pcull) return bfalse;

  if(0 == plst->count || 0 == pcull->count) return btrue;

  // collapse inefficient bounding boxes in pcull
  for(i=0; i<plst->count; i++)
  {
    AA_BBOX * pbb_lst;
    int       idx_lst;
    bool_t    found;
    int       lst_full_weight;

    pbb_lst = plst->list + i;

    lst_full_weight = 1 << (3*(max_lod - pbb_lst->level));

    // collapse inefficient bounding boxes
    assert( pbb_lst->sub_used <= 8 );

    if( pbb_lst->sub_used > 6 )
    {
      // all of the 8 bboxes under this bbox are used
      // they are "degenerate" with this bbox and should be culled

      idx_lst       = pbb_lst->address;
      lst_divisions = pbb_lst->level;
      lst_per_axis  = 1 << lst_divisions;

      // scan through pcull to find the bboxes under this one
      found = bfalse;
      for(j=0; j<pcull->count; j++)
      {
        AA_BBOX * pbb_cull = pcull->list + j;
        int       idx_cull;

        if(!pbb_cull->used) continue;

        idx_cull       = pbb_cull->address;
        cull_divisions = pbb_cull->level;
        cull_per_axis  = 1 << cull_divisions;

        if(cull_divisions > lst_divisions)
        {
          // find the integer position of the pbb_cull box

          ix = (idx_cull >> (0*cull_divisions)) & (cull_per_axis-1);
          iy = (idx_cull >> (1*cull_divisions)) & (cull_per_axis-1);
          iz = (idx_cull >> (2*cull_divisions)) & (cull_per_axis-1);

          // convert this to a position in pbb_lst
          ix >>= (cull_divisions - lst_divisions);
          iy >>= (cull_divisions - lst_divisions);
          iz >>= (cull_divisions - lst_divisions);

          // find the address in pbb_lst
          k = ((ix & (lst_per_axis-1)) << (0*lst_divisions)) |
              ((iy & (lst_per_axis-1)) << (1*lst_divisions)) |
              ((iz & (lst_per_axis-1)) << (2*lst_divisions));

          // compare this address to the address of pbb_lst
          if(k == idx_lst)
          {
            // found a match
            //if(!found)
            //{
            //  memcpy(pbb_cull, pbb_lst, sizeof(AA_BBOX));
            //  found = btrue;
            //}
            //else
            {
              pbb_cull->used = bfalse;
            }
          }
        }
      }
    }
  }

  bbox_list_contract(pcull);

  return btrue;
}





//--------------------------------------------------------------------------------------------
bool_t mad_optimize_bbox_tree(Mad_t * pmad)
{
  // BB > optimize the bbox tree by culling inefficient bboxes from the more detailed lists

  int i,j,k, iframes, max_lod;
  BBOX_ARY  * pary;
  BBOX_LIST * pcull, * plst;
  AA_BBOX   * pbbox;
  MD2_Model_t * pmd2;
  const MD2_Frame_t * pframe;

  if(NULL == pmad) return bfalse;

  pmd2 = pmad->md2_ptr;

  if(NULL == pmad->bbox_arrays || 0 == pmad->bbox_frames) return btrue;

  // go through every frame
  iframes = MIN(pmad->bbox_frames, md2_get_numFrames(pmd2));
  for(i=0; i<iframes; i++)
  {
    pary    = pmad->bbox_arrays + i;
    pframe  = md2_get_Frame(pmd2, i);
    max_lod = pary->count - 1;

    for(j=max_lod; j>=0; j--)
    {
      pcull = pary->list + j;

      for(k=j-1; k>=1; k--)
      {
        plst = pary->list + k;

        mad_cull_bbox_list(max_lod, plst, pcull);
      }
    }

  }

  // convert the bbox weight variable to an "efficiency" variable
  // that might be used when testing collisions or displaying bboxes, etc.
  for(i=0; i<iframes; i++)
  {
    float max_weight;

    pary    = pmad->bbox_arrays + i;
    pframe  = md2_get_Frame(pmd2, i);
    max_lod = pary->count - 1;

    if(NULL == pary) continue;

    for(j=0; j<pary->count; j++)
    {
      plst = pary->list + j;
      if(NULL == plst) continue;

      max_weight = 1 << (3*(max_lod-j));

      for(k=0; k<plst->count; k++)
      {
        pbbox = plst->list + k;

        if(NULL == pbbox) continue;

        pbbox->weight /= max_weight;
      }
    };
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
// need to find some way to collect the data into larger sized blocks
static bool_t mad_simplify_bbox_list(int max_divisions, int new_divisions, BBOX_LIST *pnew, BBOX_LIST *pold)
{
  int ix,ix_min,ix_max, iy,iy_min,iy_max, iz,iz_min,iz_max;
  int i;
  int bbox_count, full_weight, full_weight_last;
  int new_per_axis, new_alloc_count, old_divisions, old_per_axis;

  // erase any existing data in the BBOX_LIST
  if(NULL == bbox_list_renew(pnew)) return bfalse;

  // check for valid lists
  if(NULL == pold) return bfalse;

  full_weight = 1 << (3*(max_divisions-new_divisions));
  full_weight_last = full_weight >> 3;

  if(0 == new_divisions)
  {
    if(0 != pold->count)
    {
      // this one must be used
      bbox_list_new( pnew );
      bbox_list_alloc( pnew, 1 );

      pnew->list[0].address = 0;
      pnew->list[0].level   = 0;
      pnew->list[0].used    = btrue;
      pnew->list[0].weight  = full_weight;

      pnew->list[0].mins.x = 0;
      pnew->list[0].mins.y = 0;
      pnew->list[0].mins.z = 0;

      pnew->list[0].maxs.x = 1;
      pnew->list[0].maxs.y = 1;
      pnew->list[0].maxs.z = 1;
    }

    return btrue;
  }

  // coalece the old data
  old_divisions = new_divisions + 1;
  old_per_axis = 1 << old_divisions;

  // allocate a bbox list that has the required number of bboxes
  new_per_axis    = 1 << (    new_divisions);
  new_alloc_count = 1 << (3 * new_divisions);
  bbox_list_alloc( pnew, new_alloc_count );

  // initialize pnew with "invalid" bboxes
  for(i=0; i<new_alloc_count; i++)
  {
    AA_BBOX * pbbox = pnew->list + i;

    pbbox->used     = bfalse;
    pbbox->level    = new_divisions;
    pbbox->address  = i;
    pbbox->weight   = 0;
    pbbox->sub_used = 0;
  }

  // insert valid nodes into the data
  bbox_count = 0;
  for(i=0; i<pold->count; i++)
  {
    float fweight, fused;
    int   idx_new, ivol;

    AA_BBOX * pbb_old = pold->list + i;

    if (!pbb_old->used) continue;

    ix_min = pbb_old->mins.x * new_per_axis;
    ix_max = pbb_old->maxs.x * new_per_axis - 0.125f/new_per_axis;

    iy_min = pbb_old->mins.y * new_per_axis;
    iy_max = pbb_old->maxs.y * new_per_axis - 0.125f/new_per_axis;

    iz_min = pbb_old->mins.z * new_per_axis;
    iz_max = pbb_old->maxs.z * new_per_axis - 0.125f/new_per_axis;

    ivol    = (ix_max-ix_min + 1) * (iy_max-iy_min + 1) * (iz_max-iz_min + 1);
    fweight = pbb_old->weight   / (float)ivol;
    fused   = pbb_old->sub_used / (float)ivol;

    for(ix=ix_min; ix<=ix_max; ix++)
    {
      for(iy=iy_min; iy<=iy_max; iy++)
      {
        for(iz=iz_min; iz<=iz_max; iz++)
        {
          AA_BBOX * pbbox;

          idx_new = ((ix & (new_per_axis-1)) << (0*new_divisions)) |
                    ((iy & (new_per_axis-1)) << (1*new_divisions)) |
                    ((iz & (new_per_axis-1)) << (2*new_divisions));

          pbbox = pnew->list + idx_new;

          if(!pbbox->used)
          {
            assert(pbbox->level == new_divisions && pbbox->address == idx_new);

            pbbox->used    = btrue;

            pbbox->mins.x =  (ix + 0) / (float)new_per_axis;
            pbbox->maxs.x =  (ix + 1) / (float)new_per_axis;
            pbbox->mins.y =  (iy + 0) / (float)new_per_axis;
            pbbox->maxs.y =  (iy + 1) / (float)new_per_axis;
            pbbox->mins.z =  (iz + 0) / (float)new_per_axis;
            pbbox->maxs.z =  (iz + 1) / (float)new_per_axis;

            bbox_count++;
          }

          pbbox->weight   += fweight;
          pbbox->sub_used += (fused >= 6) ? 1 : 0;
        }
      }
    }
  };

  //if(bbox_count == new_alloc_count)
  //{
  //  // this bbox is completely full. It doesn't help us at all
  //  bbox_list_delete(pnew);
  //  return bfalse;
  //}
  //else if(bbox_count > new_alloc_count * 0.75f)
  //{
  //  // this bbox is not efficient enough. there's not enough reason to keep it around
  //  bbox_list_delete(pnew);
  //  return bfalse;
  //}
  //else
  {
    return bbox_list_contract(pnew);
  };
}


//--------------------------------------------------------------------------------------------
bool_t mad_delete_bbox_tree(Mad_t * pmad)
{
  int i;

  if(NULL == pmad) return bfalse;

  if(0 == pmad->bbox_frames || NULL == pmad->bbox_arrays) return btrue;

  for(i=0; i<pmad->bbox_frames; i++)
  {
    bbox_ary_delete(pmad->bbox_arrays + i);
  }

  EGOBOO_DELETE(pmad->bbox_arrays);
  pmad->bbox_frames = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
static bool_t mad_scale_bbox_tree(Mad_t * pmad)
{
  // BB > scale bounding boxes to the size of the object

  int i, j, k;
  int iframes;

  MD2_Model_t * pmd2;
  const MD2_Frame_t * pframe;

  BBOX_ARY  * pary;
  BBOX_LIST * plst;
  AA_BBOX   * pbbox;

  vect3 diff, origin;

  if(NULL ==pmad) return bfalse;
  pmd2 = pmad->md2_ptr;

  if(0 == pmad->bbox_frames || NULL == pmad->bbox_arrays) return bfalse;

  iframes = MIN(pmad->bbox_frames, md2_get_numFrames(pmd2));
  for(i=0; i<iframes; i++)
  {
    pary   = pmad->bbox_arrays + i;
    pframe = md2_get_Frame(pmd2, i);

    if(NULL == pary) continue;

    diff.x = (pframe->bbmax[0] - pframe->bbmin[0]);
    diff.y = (pframe->bbmax[1] - pframe->bbmin[1]);
    diff.z = (pframe->bbmax[2] - pframe->bbmin[2]);

    origin.x = pframe->bbmin[0];
    origin.y = pframe->bbmin[1];
    origin.z = pframe->bbmin[2];


    for(j=0; j<pary->count; j++)
    {
      plst = pary->list + j;

      if(NULL == plst) continue;

      for(k=0; k<plst->count; k++)
      {
        pbbox = plst->list + k;

        if(NULL == pbbox) continue;

        pbbox->mins.x = pbbox->mins.x * diff.x + origin.x;
        pbbox->mins.y = pbbox->mins.y * diff.y + origin.y;
        pbbox->mins.z = pbbox->mins.z * diff.z + origin.z;

        pbbox->maxs.x = pbbox->maxs.x * diff.x + origin.x;
        pbbox->maxs.y = pbbox->maxs.y * diff.y + origin.y;
        pbbox->maxs.z = pbbox->maxs.z * diff.z + origin.z;

        pbbox->mins.x *= -1;
        pbbox->maxs.x *= -1;
      }
    }

  };

  return btrue;

}

//--------------------------------------------------------------------------------------------
//bool_t mad_optimize_bbox_tree(int max_level, Mad_t * pmad)
//{
//  // BB > try to collapse some of the bounding boxes on the more detailed levels
//
//  int i, j, k;
//  int iframes;
//
//  MD2_Model_t * pmd2;
//  const MD2_Frame_t * pframe;
//
//  BBOX_ARY  * pary;
//  BBOX_LIST * plst_base, * plst_test;
//  AA_BBOX   * pbbox;
//
//  vect3 diff, origin;
//
//  if(NULL ==pmad) return bfalse;
//  pmd2 = pmad->md2_ptr;
//
//  if(0 == pmad->bbox_frames || NULL == pmad->bbox_arrays) return bfalse;
//
//  iframes = MIN(pmad->bbox_frames, md2_get_numFrames(pmd2));
//  for(i=0; i<pmad->bbox_frames; i++)
//  {
//    pary   = pmad->bbox_arrays + i;
//    pframe = md2_get_Frame(pmd2, i);
//
//    if(NULL == pary) continue;
//
//    for(base_level = 1; base_level < pary->count; level++)
//    {
//      plst_base = pary->list + base_level;
//
//      base_per_axis = 1 << (  base_level);
//      base_count    = 1 << (3*base_level);
//
//      for(test_level = 1; test_level < pary->count; level++)
//      {
//        plst_test = pary->list + test_level;
//
//        test_per_axis = 1 << (  test_level);
//        test_count    = 1 << (3*test_level);
//
//
//      }
//
//    }
//
//    bbox_count = 0;
//    scan_size = 1 << (level_diff);
//    for(idx=0; idx<1; idx++)
//    {
//      for(idy=0; idy<1; idy++)
//      {
//        for(idz=0; idz<1; idz++)
//        {
//          AA_BBOX * pbbox;
//
//          ix = ix_min + idx;
//          iy = iy_min + idy;
//          iz = iz_min + idz;
//
//          index = (ix & (per_level-1)) << (0*level) |
//                  (iy & (per_level-1)) << (1*level) |
//                  (iz & (per_level-1)) << (2*level);
//
//          pbbox = plst_test->list + index;
//
//          if(pbbox->used)
//          {
//            bbox_count++;
//          }
//        }
//      }
//    }
//    if(bbox_count < XXXX * YYYY)
//    {
//      // too inefficient replace it with
//    }
//
//}

//--------------------------------------------------------------------------------------------
bool_t mad_generate_bbox_tree(int max_level, Mad_t * pmad)
{
  // BB > Make a series of progressively refined BBox lists.
  //      The list will have null nodes for every level of detail where
  //      the bounding volumes are not significantly different from a single bounding box

  int i,j;

  if(NULL == pmad) return bfalse;

  if(NULL == pmad->md2_ptr) return bfalse;

  // initialize the bbox_tree
  mad_delete_bbox_tree(pmad);

  // if there are no triangles, we are done
  if(0 == pmad->md2_ptr->m_numTriangles) return btrue;

  // read the frame count and allocate one BBOX_ARY per frame
  pmad->bbox_frames = pmad->md2_ptr->m_numFrames;
  pmad->bbox_arrays = EGOBOO_NEW_ARY( BBOX_ARY, pmad->bbox_frames );

  // go through every frame
  for(i=0; i<pmad->bbox_frames; i++)
  {
    // start with a nice clean bbox_ary
    bbox_ary_new(pmad->bbox_arrays + i);

    // allocate the lists for each level of detail
    bbox_ary_alloc(pmad->bbox_arrays + i, max_level + 1);

    // generate the most detailed bbox collection for this frame
    if(!mad_generate_bbox_list(pmad->bbox_arrays[i].list + max_level, pmad->md2_ptr, i, max_level))
    {
      // there was an error or the bbox list is completely full

      // deallocate everything
      mad_delete_bbox_tree(pmad);

      return bfalse;
    }

    // generate all of the remaining bbox collections by simplifying
    for(j=max_level-1; j>=0; j--)
    {
      if( !mad_simplify_bbox_list(max_level, j, pmad->bbox_arrays[i].list + j, pmad->bbox_arrays[i].list + j + 1) )
      {
        // there was an error or the bbox list is completely full

        // remove the last bbox list
        bbox_list_delete(pmad->bbox_arrays[i].list + j);

        mad_optimize_bbox_tree(pmad);

        mad_scale_bbox_tree(pmad);

        return btrue;
      };
    };
  };

  mad_optimize_bbox_tree(pmad);

  mad_scale_bbox_tree(pmad);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Mad_t * Mad_new(Mad_t * pmad )
{
  //fprintf( stdout, "Mad_new()\n");

  if(NULL == pmad) return pmad;

  memset(pmad, 0, sizeof(Mad_t));

  EKEY_PNEW(pmad, Mad_t);

  return pmad;
}

//--------------------------------------------------------------------------------------------
bool_t Mad_delete(Mad_t * pmad)
{
  if(NULL == pmad) return bfalse;

  if( !EKEY_PVALID(pmad) ) return btrue;

  EKEY_PINVALIDATE(pmad);

  if(NULL != pmad->md2_ptr)
  {
    md2_delete(pmad->md2_ptr);
    pmad->md2_ptr = NULL;
  }

  EGOBOO_DELETE(pmad->framelip);
  EGOBOO_DELETE(pmad->framefx);

  mad_delete_bbox_tree(pmad);

  pmad->Loaded = bfalse;

  return btrue;
};

//--------------------------------------------------------------------------------------------
Mad_t *  Mad_renew(Mad_t * pmad)
{
  Mad_delete(pmad);
  return Mad_new(pmad);
};


//---------------------------------------------------------------------------------------------
MAD_REF MadList_load_one( Game_t * gs, const char * szObjectpath, const char * szObjectname, MAD_REF imad )
{
  // ZZ> This function loads an id md2 file, storing the converted data in the indexed model
  //    int iFileHandleRead;

  int iFrames;
  Mad_t * pmad;

  PMad_t madlst = gs->MadList;

  // make the notation "easier"
  if( !VALID_MAD_RANGE(imad) ) return INVALID_MAD;
  pmad = gs->MadList + imad;

  // Make sure we don't load over an existing model
  if( EKEY_PVALID(pmad) && pmad->Loaded )
  {
    log_error( "Model template (mad) %i is already used. (%s%s)\n", imad, szObjectpath, szObjectname );
  }

  // initialize the model template
  Mad_new(pmad);

  // Make up a name for the mad...  IMPORT\TEMP0000.OBJ
  strncpy( pmad->name, szObjectpath, sizeof( pmad->name ) );
  if(NULL != szObjectname)
  {
    strncat( pmad->name, szObjectname, sizeof( pmad->name ) );
  }


  // make sure we have a clean Mad_t. More complicated because of dynamic allocation...
  if(NULL == Mad_renew(pmad)) return INVALID_MAD;

  // load the actual md2 data
  pmad->md2_ptr = md2_load( inherit_fname(szObjectpath, szObjectname, "tris.md2"), NULL );
  if(NULL == pmad->md2_ptr) return INVALID_MAD;

  // BB > Egoboo md2 models were designed with 1 tile = 32x32 units, but internally Egoboo uses
  //      1 tile = 128x128 units. Previously, this was handled by sprinkling a bunch of
  //      commands that multiplied various quantities by 4 or by 4.125 throughout the code.
  //      It was very counterintuitive, and caused me no end of headaches...  Of course the
  //      solution is to scale the model!
  md2_scale_model(pmad->md2_ptr, 4);

  // generate a bbox structure for each frame of the animation
  // this is dynamically allocated, Mad_delete() or Mad_renew() must be called
  // to make all of that go away
  mad_generate_bbox_tree(2, pmad);

  // Figure out how many vertices to transform
  pmad->vertices      = md2_get_numVertices( pmad->md2_ptr );
  pmad->transvertices = mad_calc_transvertices( pmad->md2_ptr );

  // allocate the extra animation data
  iFrames = md2_get_numFrames(pmad->md2_ptr);
  pmad->framelip = EGOBOO_NEW_ARY( Uint8 , iFrames );
  pmad->framefx  = EGOBOO_NEW_ARY( Uint16, iFrames );

  // tell everyone that we loaded correctly
  pmad->Loaded = btrue;

  // Create the actions table for this object
  get_actions( gs, imad );

  // Copy entire actions to save frame space "COPY.TXT"
  load_copy_file( gs, szObjectpath, szObjectname, imad );

  return imad;
}

//---------------------------------------------------------------------------------------------
void MadList_free_one( Game_t * gs, MAD_REF imad )
{
  // ZZ> This function loads an id md2 file, storing the converted data in the indexed model
  //    int iFileHandleRead;

  if( !VALID_MAD(gs->MadList, imad) ) return;

  Mad_renew(gs->MadList + imad);
}


//---------------------------------------------------------------------------------------------
bool_t mad_display_bbox_ary(BBOX_ARY * pary, int level)
{
  BBOX_LIST * plst;
  AA_BBOX   * pbbox;
  int i, j;

  if(NULL == pary) return bfalse;

  for(i=0; i<=level; i++)
  {
    if(i >= pary->count) continue;

    plst = pary->list + i;

    if(NULL == plst)
      continue;

    for(j=0; j<plst->count; j++)
    {
      pbbox = plst->list + j;

      if(NULL == pbbox) continue;

      bbox_gl_draw(pbbox);
    }
  };

  return btrue;
};

bool_t mad_display_bbox_tree(int level, matrix_4x4 matrix, Mad_t * pmad, int frame1, int frame2 )
{
  MD2_Model_t * pmd2;
  BBOX_ARY  * pary;

  if(NULL == pmad) return bfalse;

  pmd2 = pmad->md2_ptr;
  if(NULL == pmd2) return bfalse;

  ATTRIB_PUSH( "mad_display_bbox_tree", GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT | GL_TRANSFORM_BIT );
  {
    // don't write into the depth buffer
    glDepthMask( GL_FALSE );
    glEnable(GL_DEPTH_TEST);

    // fix the poorly chosen normals...
    glDisable( GL_CULL_FACE );

    // make them transparent
    glEnable(GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // choose a "white" texture
    GLtexture_Bind(NULL, &gfxState);

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    matrix.CNV( 3, 2 ) += RAISE;
    glMultMatrixf( matrix.v );
    matrix.CNV( 3, 2 ) -= RAISE;


      if(frame1 >=0 && frame1 < md2_get_numFrames(pmd2) && frame1 < pmad->bbox_frames)
      {
        pary = pmad->bbox_arrays + frame1;

        if(NULL != pary)
        {
          mad_display_bbox_ary(pary, level);
        };
      }

      if(frame2 >=0 && frame2 < md2_get_numFrames(pmd2) && frame2 < pmad->bbox_frames)
      {
        pary = pmad->bbox_arrays + frame2;

        if(NULL != pary)
        {
          mad_display_bbox_ary(pary, level);
        };
      }


    glPopMatrix();

  }
  ATTRIB_POP( "mad_display_bbox");

  return btrue;

}

//---------------------------------------------------------------------------------------------
int mad_vertexconnected( MD2_Model_t * m, int vertex )
{
  // ZZ> This function returns 1 if the model vertex is connected, 0 otherwise

  const MD2_GLCommand_t * g;
  int commands, entry;

  if(NULL == m) return 0;

  g = md2_get_Commands(m);
  if(NULL == g) return 0;

  for( /*nothing*/; NULL != g; g = g->next)
  {
    commands = g->command_count;
    for(entry = 0; entry<commands; entry++)
    {
      if(g->data[entry].index == vertex)
      {
        return 1;
      }
    }
  }

  // The vertex is not used
  return 0;
}

//---------------------------------------------------------------------------------------------
int mad_calc_transvertices( MD2_Model_t * m )
{
  // ZZ> This function gets the number of vertices to transform for a model...
  //     That means every one except the grip ( unconnected ) vertices

  int cnt, vrtcount, trans = 0;

  if(NULL == m) return 0;

  vrtcount = md2_get_numVertices(m);

  for ( cnt = 0; cnt < vrtcount; cnt++ )
    trans += mad_vertexconnected( m, cnt );

  return trans;
}



////--------------------------------------------------------------------------------------------
//bool_t mad_generate_bbox_level(int bbox_divisions, int frame, Mad_t * pmad)
//{
//  int            i, bbox_count, vrt_count;
//  int            num_per_axis;
//  BBOX_LIST      bblist;
//  AA_BBOX        tmp_bb;
//
//  MD2_Model_t    * pmd2;
//
//  int            vrt_count;
//  const MD2_Frame_t    * pframe;
//
//  int            tri_count;
//  md2_triangle * ptri_lst;
//
//  // check for valid Mad_t
//  if(NULL == pmad) return bfalse;
//
//  // check for valid MD2
//  if(NULL == pmad->md2_ptr) return bfalse;
//  pmd2 = pmad->md2_ptr->m_numFrames;
//
//  // check the triangle list
//  if(0 == md2_get_numTriangles(pmd2) || NULL == md2_get_Triangles(pmd2)) return bfalse;
//  tri_count = md2_get_numTriangles(pmd2);
//  ptri_lst  = md2_get_Triangles(pmd2);
//
//  // check for valid frame
//  if(0 == md2_get_numFrames(pmd2) || NULL == md2_get_Frames(pmd2) || frame > md2_get_numFrames(pmd2)) return bfalse;
//  pframe    = md2_get_Frame(pmd2, frame);
//  vrt_count = pframe->vertices;
//
//  // allocate a bbox list that has the required number of bboxes
//  num_per_axis = 1 << (  bbox_divisions)
//  bbox_count   = 1 << (3*bbox_divisions);
//  bbox_list_alloc( &bblist, bbox_count );
//
//  // handle the special case
//  if(num_per_axis <= 1)
//  {
//    // just copy the pre-calculated values
//    memcpy(bblist.list[0].maxs.v, pframe->bbmax, 3*sizeof(float));
//    memcpy(bblist.list[0].mins.v, pframe->bbin, 3*sizeof(float));
//
//    return btrue;
//  }
//
//  for(i=0; i<bbox_count; i++)
//  {
//    int ix, iy, iz;
//
//    ix = (i >> 0*bbox_divisions) & (num_per_axis-1);
//    iy = (i >> 1*bbox_divisions) & (num_per_axis-1);
//    iz = (i >> 2*bbox_divisions) & (num_per_axis-1);
//
//    bblist.list[i].mins.x =  (ix + 0) / (float)num_per_axis;
//    bblist.list[i].maxs.x =  (ix + 1) / (float)num_per_axis;
//    bblist.list[i].mins.y =  (iy + 0) / (float)num_per_axis;
//    bblist.list[i].maxs.y =  (iy + 1) / (float)num_per_axis;
//    bblist.list[i].mins.z =  (iz + 0) / (float)num_per_axis;
//    bblist.list[i].maxs.z =  (iz + 1) / (float)num_per_axis;
//  }
//
//  // !!!! GAR !!! - I have to roll my own triangle-bbox-array collision code
//  //                it will probably be the most inefficient algorithm possible!
//
//  // now scan the triangle list
//  for(i=0;i<tri_count;i++)
//  {
//    int ix_min,ix_max, iy_min,iy_max, iz_min,iz_max;
//    float dx,dy,dz;
//    float max_comp;
//    vect3 uvec, vvec, ovec, nrm, ovvec;
//
//    // test the vertices to make sure they are valid
//    for(j=0;j<3;j++)
//    {
//      ivrt = ptri_lst[i].vertexIndices[v];
//      if(ivrt>vrt_count) break;
//      if(0 == j)
//      {
//        tmp_bb.mins.x = tmp_bb.maxs.x = pframe->vertices[ptri_lst[i].vertexIndices[j]].x;
//        tmp_bb.mins.y = tmp_bb.maxs.y = pframe->vertices[ptri_lst[i].vertexIndices[j]].y;
//        tmp_bb.mins.z = tmp_bb.maxs.z = pframe->vertices[ptri_lst[i].vertexIndices[j]].z;
//      }
//      else
//      {
//        float ftmp;
//
//        ftmp = pframe->vertices[ptri_lst[i].vertexIndices[j]].x;
//        if(ftmp<tmp_bb.mins.x) tmp_bb.mins.x = ftmp;
//        else if (ftmp>tmp_bb.maxs.x) tmp_bb.maxs.x = ftmp;
//
//        ftmp = pframe->vertices[ptri_lst[i].vertexIndices[j]].y;
//        if(ftmp<tmp_bb.mins.y) tmp_bb.mins.y = ftmp;
//        else if (ftmp>tmp_bb.maxs.y) tmp_bb.maxs.y = ftmp;
//
//        ftmp = pframe->vertices[ptri_lst[i].vertexIndices[j]].z;
//        if(ftmp<tmp_bb.mins.z) tmp_bb.mins.x = ftmp;
//        else if (ftmp>tmp_bb.maxs.z) tmp_bb.maxs.z = ftmp;
//      }
//
//    };
//
//    // if a vertex is invalid, skip the triangle
//    if(j!=2) continue;
//
//    dx = tmp_bb.maxs.x - tmp_bb.mins.x;
//    dy = tmp_bb.maxs.y - tmp_bb.mins.y;
//    dz = tmp_bb.maxs.z - tmp_bb.mins.z;
//
//    ovec.x = pframe->vertices[ptri_lst[i].vertexIndices[0]].x;
//    ovec.y = pframe->vertices[ptri_lst[i].vertexIndices[0]].y;
//    ovec.z = pframe->vertices[ptri_lst[i].vertexIndices[0]].z;
//
//    uvec.x = pframe->vertices[ptri_lst[i].vertexIndices[1]].x - ovec.x;
//    uvec.y = pframe->vertices[ptri_lst[i].vertexIndices[1]].y - ovec.y;
//    uvec.z = pframe->vertices[ptri_lst[i].vertexIndices[1]].z - ovec.z;
//
//    vvec.x = pframe->vertices[ptri_lst[i].vertexIndices[2]].x - ovec.x;
//    vvec.y = pframe->vertices[ptri_lst[i].vertexIndices[2]].y - ovec.y;
//    vvec.z = pframe->vertices[ptri_lst[i].vertexIndices[2]].z - ovec.z;
//
//    nrm   = CrossProduct(uvec, vvec);
//    ovvec = CrossProduct(ovec, vvec);
//
//    min_extent = MIN(MIN(dx,dy),dz);
//
//    // no point in searching outside the volume of the triangle
//    ix_min =     (tmp_bb.mins.x - pframe->bbmin[0]) / (pframe->bbmax[0]-pframe->bbmin[0]) * num_per_axis;
//    ix_max = 1 + (tmp_bb.maxs.x - pframe->bbmin[0]) / (pframe->bbmax[0]-pframe->bbmin[0]) * num_per_axis;
//
//    iy_min =     (tmp_bb.mins.y - pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;
//    iy_max = 1 + (tmp_bb.maxs.y - pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;
//
//    iz_min =     (tmp_bb.mins.z - pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;
//    iz_max = 1 + (tmp_bb.maxs.z - pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;
//
//    // The equation of a plane that intersects the origin is
//    //
//    //     pnorm . <x,y,z> = <0,0,0>.
//    //
//    // The parametric equation of a triangle is given by
//    //
//    //     <x,y,z> = u * uvec + v * vvec + ovec,
//    //
//    // where u and v stay within the range [0,1], and uvec = p1 - p0, vvec = p2-p1, and ovec = p0, with
//    // p0, p1, and p2, being the corner points of the triangle
//    //
//    // Combining these two equations, we have
//    //
//    //     pnorm.x * (u * uvec.x + v * vvec.x + ovec.x) = 0
//    //     pnorm.y * (u * uvec.y + v * vvec.y + ovec.y) = 0
//    //     pnorm.z * (u * uvec.z + v * vvec.z + ovec.z) = 0
//
//    // divide the triangle along the axis with the largest component of the normal
//    if(dx == min_extent)
//    {
//      float minval = pframe->bbmin[0];
//      float maxval = pframe->bbmax[0];
//      float height;
//
//      for(i=ix_min; i<=ix_max; i++)
//      {
//        float u_min, u_max, u0,u1;
//        float y0,z0, y1,z1;
//
//        // the vector equation for the intersection of the triangle is
//        // <height,y,z> = u*uvec + v*vvec + ovec;
//
//        // height-ovec.x = u*uvec.x + v*vvec.x
//        // y - ovec.y = u*uvec.y + v*vvec.y
//        // z - ovec.z = u*uvec.z + v*vvec.z
//
//        // u = (height-ovec.x)/uvec.x - v*vvec.x/uvec.x
//        // v = (height-ovec.x)/vvec.x - u*uvec.x/vvec.x
//        // both u and v are valid within the range [0,1], so
//        // min_u = max(0, (height-ovec.x)/uvec.x, (height-ovec.x)/uvec.x - vvec.x/uvec.x)
//        // max_u = min(1, (height-ovec.x)/uvec.x, (height-ovec.x)/uvec.x - vvec.x/uvec.x)
//        // if min_u >= max_u, then there is no intersection
//
//        height = (i + 0.5f)/(float)num_per_axis * (maxval-minval) + minval;
//
//        u0 = (height-ovec.x)/uvec.x;
//        u1 = u0 - vvec.x/uvec.x;
//
//        u_min = MAX(0, u0, u1);
//        u_max = MIN(1, u0, u1);
//
//        if(min_u >= max_u) continue;
//
//        // Now calculate the endpoints of the line
//
//        // y = ovec.y + u*uvec.y + v*vvec.y = -(ovec x vvec).z/vvec.x - u*(uvec x vvec).z/vvec.x + height*vvec.y/vvec.x
//        // z = ovec.z + u*uvec.z + v*vvec.z =  (ovec x vvec).y/vvec.x + u*(uvec x vvec).y/vvec.x + height*vvec.z/vvec.x
//
//        y0 = (-ovvec.x - u_min*nrm.z + height*vvec.y) / vvec.x;
//        z0 = ( ovvec.y + u_min*nrm.y + height*vvec.z) / vvec.x;
//
//        y1 = (-ovvec.x - u_max*nrm.z + height*vvec.y) / vvec.x;
//        z1 = ( ovvec.y + u_max*nrm.y + height*vvec.z) / vvec.x;
//
//
//        // now calculate which of the bboxes intersects this line
//        tmp_bb.mins.x = height;
//        tmp_bb.maxs.x = height;
//
//        iy_min = (MIN(y0,y1) + pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;
//        iy_max = (MAX(y0,y1) + pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;
//
//        iz_min = (MIN(z0,z1) + pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;
//        iz_max = (MAX(z0,z1) + pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;
//
//
//        for(j=iy_min, j<iy_max; j++)
//        {
//          for(k=iz_min, k<iz_max; k++)
//          {
//          }
//        }
//
//        height = i /(float)num_per_axis * (maxval-minval) + minval;
//
//        tmp_bb.mins.y = MIN(y0,y1);
//        tmp_bb.maxs.y = MAX(y0,y1);
//
//        tmp_bb.mins.z = MIN(z0,z1);
//        tmp_bb.maxs.z = MAX(z0,z1);
//
//
//
//
//
//
//
//      }
//    }
//    else if(ABS(nrm.y) == max_comp)
//    {
//      // triangle is facing in the y direction,
//    }
//    else
//    {
//      // triangle is facing in the z direction
//    }
//
//  }
//
//
//  // initialize the sises of the bboxes
//  pframe->vertices
//
//
//
//
//  // deallocate the list
//  bbox_list_delete( &bblist );
//}

