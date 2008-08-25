#pragma once

#include "egoboo_types.h"
#include "enet/enet.h"

#include <stdio.h>

///
/// @file
/// @brief Egoboo Stream Handling
/// @details Definitions for generic stream handling

//------------------------------------------------------------------
//------------------------------------------------------------------
struct s_stream
{
  FILE  * pfile;
  Uint8 * data;
  size_t data_size, readLocation;
};
typedef struct s_stream STREAM;
