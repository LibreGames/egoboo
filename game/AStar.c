#include "AStar.h"

#include "game.h"
#include "mesh.inl"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define MAX_ASTAR_NODES 512

//------------------------------------------------------------------------------
static bool_t       AStar_initialized = bfalse;

static int          AStar_allocated = 0;
static int          AStar_open_count = 0;
static AStar_Node * AStar_open_list = NULL;
static int          AStar_closed_count = 0;
static AStar_Node * AStar_closed_list = NULL;

static int AStar_cmp_nodes( const void * pleft, const void * pright );

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool_t AStar_deallocate()
{
  if(AStar_allocated == 0) return btrue;

  FREE(AStar_open_list);
  FREE(AStar_closed_list);

  AStar_allocated = 0;

  return btrue;
}

//------------------------------------------------------------------------------
bool_t AStar_allocate(int requested_size)
{
  int size;

  size = requested_size;
  if(size < 0) size = MAX_ASTAR_NODES;

  if(AStar_allocated)
  {
    AStar_deallocate();
  }

  AStar_open_list   = calloc(size, sizeof(AStar_Node));
  if(NULL == AStar_open_list) return bfalse;

  AStar_closed_list = calloc(size, sizeof(AStar_Node));
  if(NULL == AStar_closed_list)
  {
    free(AStar_open_list);
    return bfalse;
  }

  AStar_allocated = size;

  return btrue;
}

//------------------------------------------------------------------------------
bool_t AStar_open(int requested_size)
{
  if( !AStar_allocate(requested_size) ) return bfalse;

  AStar_reinit();

  AStar_initialized = btrue;

  return AStar_initialized;
}

//------------------------------------------------------------------------------
bool_t AStar_close()
{
  if(!AStar_initialized) return btrue;

  if(!AStar_deallocate()) return bfalse;

  AStar_initialized = bfalse;

  return !AStar_initialized;
}

//------------------------------------------------------------------------------
void AStar_reinit()
{
  AStar_open_count   = 0;
  AStar_closed_count = 0;
}

//------------------------------------------------------------------------------
int AStar_closed_find(AStar_Node * pn)
{
  int i;

  for(i=0; i<AStar_closed_count; i++)
  {
    if( (pn->ix == AStar_closed_list[i].ix) && (pn->iy == AStar_closed_list[i].iy) )
    {
      return i;
    }
  }

  return -1;
}

//------------------------------------------------------------------------------
int AStar_open_find(AStar_Node * pn)
{
  int i;

  for(i=0; i<AStar_open_count; i++)
  {
    if( (pn->ix == AStar_open_list[i].ix) && (pn->iy == AStar_open_list[i].iy) )
    {
      return i;
    }
  }

  return -1;
}

//------------------------------------------------------------------------------
bool_t AStar_open_add(AStar_Node * pn)
{
  bool_t retval = bfalse;

  if(NULL == pn || pn->closeme) return bfalse;

  if(AStar_open_count >= AStar_allocated-1) return bfalse;

  memcpy(AStar_open_list + AStar_open_count, pn, sizeof(AStar_Node));
  AStar_open_count++;

  return btrue;
}

//------------------------------------------------------------------------------
bool_t AStar_closed_add(AStar_Node * pn)
{
  bool_t retval = bfalse;
  int i = AStar_closed_find(pn);

  if(NULL == pn) return bfalse;

  if(i>0 && AStar_closed_list[i].weight < pn->weight)
  {
    memcpy(AStar_closed_list + i, pn, sizeof(AStar_Node));
    retval = btrue;
  }
  else if(AStar_closed_count < AStar_allocated-1)
  {
    memcpy(AStar_closed_list + AStar_closed_count, pn, sizeof(AStar_Node));
    AStar_closed_count++;
    retval = btrue;
  }

  return retval;
}

//------------------------------------------------------------------------------
bool_t AStar_prepare_path(CGame * gs, Uint32 stoppedby, int src_ix, int src_iy, int dst_ix, int dst_iy)
{
  int i,j,k;
  bool_t dst_off_mesh;
  AStar_Node tmp_node;
  Uint32     tmp_fan;
  MESH_FAN  * mf_list = gs->Mesh_Mem.fanlst;
  MESH_INFO * mi = &(gs->mesh);
  bool_t done, retval = bfalse;
  int deadend_count, open_fraction;
  AStar_Node * popen;

  // do not start if the initial point is off the mesh
  if(!mesh_check_fan( mi, src_ix, src_iy )) return bfalse;

  dst_off_mesh = !mesh_check_fan( mi, dst_ix, dst_iy );

  // restart the algorithm
  if(!AStar_initialized) AStar_open(-1); else AStar_reinit();
  done = bfalse;

  // initialize the list
  memset(&tmp_node, 0, sizeof(AStar_Node));
  tmp_node.ix     = src_ix;
  tmp_node.iy     = src_iy;
  tmp_node.weight = sqrt((src_ix-dst_ix)*(src_ix-dst_ix) + (src_iy-dst_iy)*(src_iy-dst_iy));
  AStar_open_add(&tmp_node);

  // do the algorithm
  while( !done && AStar_open_count>0 )
  {

    if(AStar_open_count == AStar_allocated || AStar_closed_count == AStar_allocated)
    {
      // one of the two lists is completely full... we failed
      return bfalse;
    }

    open_fraction = MIN( 3, AStar_open_count);
    for(i=0; i<open_fraction; i++)
    {
      popen = AStar_open_list + i;

      // if this node has already been looked at, go to the nest one
      if(popen->closeme) continue;

      // store the parent node
      tmp_node.parent_ix = popen->ix;
      tmp_node.parent_iy = popen->iy;

      // find some child nodes
      deadend_count = 0;
      for(j=-1; j<=1; j++)
      {
        tmp_node.ix = popen->ix + j;

        for(k=-1; k<=1; k++)
        {
          // do not recompute the open node!
          if(j==0 && k==0) continue;

          tmp_node.iy = popen->iy + k;

          // check for the simplest case
          if(tmp_node.ix == dst_ix && tmp_node.iy == dst_iy)
          {
            tmp_node.weight = (tmp_node.ix-popen->ix)*(tmp_node.ix-popen->ix) + (tmp_node.iy-popen->iy)*(tmp_node.iy-popen->iy);
            tmp_node.weight = sqrt(tmp_node.weight);
            AStar_open_add(&tmp_node);
            done = btrue;
            continue;
          }

          tmp_fan = mesh_convert_fan( mi, tmp_node.ix, tmp_node.iy );

          // is the test node on the mesh?
          if( INVALID_FAN == tmp_fan )
          {
            deadend_count++;
            continue;
          }

          // is this a valid tile?
          if( mesh_has_some_bits( mf_list, tmp_fan, stoppedby ) )
          {
            // add the invalid tile to the closed list
            AStar_closed_add(&tmp_node);
            deadend_count++;
            continue;
          }

          // TODO: I need to check for collisions with static objects, like trees


          // is this already in the closed list?
          if( AStar_closed_find( &tmp_node ) > 0 )
          {
            deadend_count++;
            continue;
          }

          // is this already in the open list?
          if( AStar_open_find( &tmp_node ) > 0 )
          {
            deadend_count++;
            continue;
          }

          // OK. determine the weight
          tmp_node.weight  = (tmp_node.ix-popen->ix)*(tmp_node.ix-popen->ix) + (tmp_node.iy-popen->iy)*(tmp_node.iy-popen->iy);
          tmp_node.weight += (tmp_node.ix-dst_ix)*(tmp_node.ix-dst_ix) + (tmp_node.iy-dst_iy)*(tmp_node.iy-dst_iy);
          tmp_node.weight  = sqrt(tmp_node.weight);

          AStar_open_add(&tmp_node);
        }
      }


      if(deadend_count == 8)
      {
        // this node is no longer active.
        // move it to the closed list so that we do not get any loops
        popen->closeme = btrue;
      }
    }

    qsort(AStar_open_list, AStar_open_count, sizeof(AStar_Node), AStar_cmp_nodes);

    // remove the "closeme" nodes from the open list
    open_fraction = AStar_open_count;
    for(i=0; i<open_fraction; i++)
    {
      if(AStar_open_list[i].closeme) break;
    }
    AStar_open_count = i-1;

    // add the "closeme" nodes to the closed list
    for(/*nothing*/; i<open_fraction; i++)
    {
      popen = AStar_open_list + i;
      AStar_closed_add(popen);
    }
  }

  if(done)
  {
    // the last node in the open list contains the
    retval = btrue;
  }

  return retval;
}

//------------------------------------------------------------------------------
int AStar_get_path(int src_ix, int src_iy, AStar_Node buffer[], int buffer_size)
{
  // BB > Fill buffer with the AStar_Node's that connect the source with the destination.
  //      The best destination tile will be the first in the AStar_open_list.
  //      The node list that is placed in buffer[] is actually reversed.
  //      Returns the number of nodes in buffer[], -1 if the buffer is too small

  int i, buffer_count;
  AStar_Node tmp_node;
  bool_t found, found_parent;

  if(NULL == buffer || buffer_size < 0) return 0;

  buffer_count = 0;
  tmp_node = AStar_open_list[0];
  tmp_node.closeme = bfalse;
  found = bfalse;
  while( buffer_count < buffer_size-1 )
  {
    // add the node to the end of the buffer
    buffer[buffer_count++] = tmp_node;

    if(tmp_node.ix == src_ix && tmp_node.iy == src_iy)
    {
      found = btrue;
      break;
    }

    found_parent = bfalse;

    //try to find the parent in the open list
    if(!found_parent)
    {
      for(i=0; i<AStar_open_count; i++)
      {
        if(tmp_node.parent_ix == AStar_open_list[i].ix && tmp_node.parent_iy == AStar_open_list[i].iy)
        {
          tmp_node = AStar_open_list[i];
          tmp_node.closeme = bfalse;
          found_parent = btrue;
          break;
        }
      }
    }


    if(!found_parent)
    {
      for(i=0; i<AStar_closed_count; i++)
      {
        if(tmp_node.parent_ix == AStar_closed_list[i].ix && tmp_node.parent_iy == AStar_closed_list[i].iy)
        {
          tmp_node = AStar_closed_list[i];
          tmp_node.closeme = bfalse;
          found_parent = btrue;
          break;
        }
      }
    }
  }

  return found ? buffer_count : -1;
}
//------------------------------------------------------------------------------
int AStar_cmp_nodes( const void * pleft, const void * pright )
{
  AStar_Node * as_lft = (AStar_Node *)pleft;
  AStar_Node * as_rgt = (AStar_Node *)pright;

  // move all "closeme" nodes to the end of the list
  if(as_lft->closeme && !as_rgt->closeme) return 1;
  if(as_lft->closeme && as_rgt->closeme ) return 0;

  if(as_lft->weight > as_rgt->weight) return  1;
  if(as_lft->weight < as_rgt->weight) return -1;

  return 0;
}





//------------------------------------------------------------------------------
int AStar_Node_list_prune(AStar_Node buffer[], int buffer_size)
{
  int i, i1, i2;
  bool_t * eliminate_lst;
  bool_t done;

  // require at least 2 points
  if(NULL == buffer || buffer_size < 2) return buffer_size;

  // allocate a list to track the elimination of nodes
  eliminate_lst = calloc(buffer_size, sizeof(bool_t));
  if(NULL == eliminate_lst) return buffer_size;

  done = bfalse;
  while(!done && buffer_size > 2)
  {
    // determine which waypoints may be eliminated
    for(i=0; i<buffer_size; i++)
    {
      eliminate_lst[i] = bfalse;
    }


    for(i = i1 = i2 = 0; i<buffer_size; i++)
    {
      if(i != i1 && i1 != i2)
      {
        vect2 midpoint;
        float diff;
        midpoint.x = 0.5f * ((buffer[i].ix + 0.5f) + (buffer[i2].ix + 0.5f));
        midpoint.y = 0.5f * ((buffer[i].iy + 0.5f) + (buffer[i2].iy + 0.5f));

        // use metropolis metric
        diff = ABS( midpoint.x - (buffer[i1].ix + 0.5f) ) + ABS( midpoint.y - (buffer[i1].iy + 0.5f) );

        eliminate_lst[i1] = (diff < 1.5f);
      }

      i2 = i1;
      i1 = i;
    };

    // make sure long runs of waypoints aren't eliminated willy-nilly
    for(i = i1 = 0; i < buffer_size; i++)
    {

      if(i != i1 && eliminate_lst[i] && eliminate_lst[i1])
      {
        eliminate_lst[i1] = bfalse;
      }

      i1 = i;
    };

    // keep going if one waypoint is marked for elimination
    done = btrue;
    for(i=0; i<buffer_size; i++)
    {
      if(eliminate_lst[i]) { done = bfalse; break; }
    }

    if(done) continue;

    // do the actual trimming of the waypoints
    for(i = 0; i < buffer_size; i++)
    {
      if(eliminate_lst[i] && i<buffer_size-1)
      {
        for(i1=i+1; i1<buffer_size; i1++)
        {
          memcpy( buffer + i1 - 1, buffer + i1, sizeof(AStar_Node) );
        }
        buffer_size--;
      }
    };
  }

  FREE(eliminate_lst);
  return buffer_size;
}
