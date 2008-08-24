#pragma once

///
/// @file
/// @brief A* pathfinding.
/// @details A very ugly implementation of the AStar pathfinding agorithm.
///   There is lots of room for improvement.

#include "egoboo_types.h"

struct sGame;

//------------------------------------------------------------------------------
// A* pathfinding --------------------------------------------------------------
//------------------------------------------------------------------------------

struct sAStar_Node
{
  float  weight;
  bool_t closeme;
  int    ix, iy;
  int    parent_ix, parent_iy;
};

typedef struct sAStar_Node AStar_Node_t;

bool_t AStar_open(int buffer_size);
bool_t AStar_close( void );
void   AStar_reinit( void );

bool_t AStar_prepare_path(struct sGame * gs, Uint32 stoppedby, int src_ix, int src_iy, int dst_ix, int dst_iy);

/// @details Fill buffer with the AStar_Node_t's that connect the source with the destination.
///      The best destination tile will be the first in the AStar_open_list.
///      The node list that is placed in buffer[] is actually reversed.
///      Returns the number of nodes in buffer[], -1 if the buffer is too small
/// @author BB
int AStar_get_path(int src_ix, int src_iy, AStar_Node_t buffer[], int buffer_size);


int AStar_Node_list_prune(AStar_Node_t buffer[], int buffer_size);
