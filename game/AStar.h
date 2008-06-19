#pragma once


// A very ugly implementation of the AStar pathfinding agorithm
// Lots of room for improvement

#include "egoboo_types.inl"

struct CGame_t;

//------------------------------------------------------------------------------
// A* pathfinding --------------------------------------------------------------
//------------------------------------------------------------------------------

typedef struct AStar_Node_t
{
  float  weight;
  bool_t closeme;
  int    ix, iy;
  int    parent_ix, parent_iy;
} AStar_Node;

bool_t AStar_open(int buffer_size);
bool_t AStar_close();
void   AStar_reinit();

bool_t AStar_prepare_path(struct CGame_t * gs, Uint32 stoppedby, int src_ix, int src_iy, int dst_ix, int dst_iy);
int AStar_get_path(int src_ix, int src_iy, AStar_Node buffer[], int buffer_size);
int AStar_Node_list_prune(AStar_Node buffer[], int buffer_size);