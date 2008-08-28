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

///
/// @file
/// @brief 
/// @details functions that will be declared inside the base class

#include "Network.h"

#include "egoboo_stream.inl"
#include "egoboo_types.inl"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t net_startNewSysPacket(SYS_PACKET * egop);
INLINE void   net_packet_startReading(NET_PACKET * p, ENetPacket * enpkt);
INLINE void   net_packet_doneReading(NET_PACKET * p);
INLINE size_t net_packet_remainingSize(NET_PACKET * p);

INLINE bool_t packet_readString(NET_PACKET * p, char *buffer, int maxLen);
INLINE Uint8  net_packet_readUint8(NET_PACKET * p);
INLINE Sint8  net_packet_readSint8(NET_PACKET * p);
INLINE Uint16 net_packet_readUint16(NET_PACKET * p);
INLINE Uint16 net_packet_peekUint16(NET_PACKET * p);
INLINE Sint16 net_packet_readSint16(NET_PACKET * p);
INLINE Uint32 net_packet_readUint32(NET_PACKET * p);
INLINE Sint32 net_packet_readSint32(NET_PACKET * p);

INLINE void sys_packet_addUint8(SYS_PACKET * egop, Uint8 uc);
INLINE void sys_packet_addSint8(SYS_PACKET * egop, Sint8 sc);
INLINE void sys_packet_addUint16(SYS_PACKET * egop, Uint16 us);
INLINE void sys_packet_addSint16(SYS_PACKET * egop, Sint16 ss);
INLINE void sys_packet_addUint32(SYS_PACKET * egop, Uint32 ui);
INLINE void sys_packet_addSint32(SYS_PACKET * egop, Sint32 si);
INLINE void sys_packet_addFString(SYS_PACKET * egop, const char *format, ...);
INLINE void sys_packet_addString(SYS_PACKET * egop, const char *string);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE void net_packet_startReading(NET_PACKET * p, ENetPacket * enpkt)
{
  p->pkt = enpkt;
  stream_startENet(&p->wrapper, enpkt);
}

//--------------------------------------------------------------------------------------------
INLINE void net_packet_doneReading(NET_PACKET * p)
{
  p->pkt = NULL;
  stream_done(&p->wrapper);
}
//--------------------------------------------------------------------------------------------
INLINE bool_t packet_readString(NET_PACKET * p, char *buffer, int maxLen)
{
  /// @details ZZ@> This function reads a null terminated string from the packet

  if(NULL ==p) return bfalse;

  return stream_readString(&(p->wrapper), buffer, maxLen);
}

//--------------------------------------------------------------------------------------------
INLINE Uint8 net_packet_readUint8(NET_PACKET * p)
{
  /// @details ZZ@> This function reads an Uint8 from the packet
  if(NULL ==p) return 0;

  return stream_readUint8(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Sint8 net_packet_readSint8(NET_PACKET * p)
{
  /// @details ZZ@> This function reads a Sint8 from the packet
  if(NULL ==p) return 0;

  return stream_readSint8(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Uint16 net_packet_readUint16(NET_PACKET * p)
{
  /// @details ZZ@> This function reads an Uint16 from the packet
  if(NULL ==p) return 0;

  return stream_readUint16(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Uint16 net_packet_peekUint16(NET_PACKET * p)
{
  /// @details ZZ@> This function reads an Uint16 from the packet
  if(NULL ==p) return 0;

  return stream_peekUint16(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Sint16 net_packet_readSint16(NET_PACKET * p)
{
  /// @details ZZ@> This function reads a Sint16 from the packet
  if(NULL ==p) return 0;

  return stream_readSint16(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Uint32 net_packet_readUint32(NET_PACKET * p)
{
  /// @details ZZ@> This function reads an Uint32 from the packet
  if(NULL ==p) return 0;

  return stream_readUint32(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Sint32 net_packet_readSint32(NET_PACKET * p)
{
  /// @details ZZ@> This function reads a Sint32 from the packet
  if(NULL ==p) return 0;

  return stream_readSint32(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE size_t net_packet_remainingSize(NET_PACKET * p)
{
  /// @details ZZ@> This function tells if there's still data left in the packet
  if(NULL ==p) return 0;

  return stream_remainingSize(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addUint8(SYS_PACKET * egop, Uint8 uc)
{
  /// @details ZZ@> This function appends an Uint8 to the packet
  Uint8* ucp;
  ucp = (Uint8*)(&egop->buffer[egop->head]);
  *ucp = uc;
  egop->head += 1;
  egop->size += 1;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addSint8(SYS_PACKET * egop, Sint8 sc)
{
  /// @details ZZ@> This function appends a Sint8 to the packet
  Sint8* scp;
  scp = (Sint8*)(&egop->buffer[egop->head]);
  *scp = sc;
  egop->head += 1;
  egop->size += 1;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addUint16(SYS_PACKET * egop, Uint16 us)
{
  /// @details ZZ@> This function appends an Uint16 to the packet
  Uint16* usp;
  usp = (Uint16*)(&egop->buffer[egop->head]);

  *usp = ENET_HOST_TO_NET_16(us);
  egop->head += 2;
  egop->size += 2;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addSint16(SYS_PACKET * egop, Sint16 ss)
{
  /// @details ZZ@> This function appends a Sint16 to the packet
  Sint16* ssp;
  ssp = (Sint16*)(&egop->buffer[egop->head]);

  *ssp = ENET_HOST_TO_NET_16(ss);

  egop->head += 2;
  egop->size += 2;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addUint32(SYS_PACKET * egop, Uint32 ui)
{
  /// @details ZZ@> This function appends an Uint32 to the packet
  Uint32* uip;
  uip = (Uint32*)(&egop->buffer[egop->head]);

  *uip = ENET_HOST_TO_NET_32(ui);

  egop->head += 4;
  egop->size += 4;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addSint32(SYS_PACKET * egop, Sint32 si)
{
  /// @details ZZ@> This function appends a Sint32 to the packet
  Sint32* sip;
  sip = (Sint32*)(&egop->buffer[egop->head]);

  *sip = ENET_HOST_TO_NET_32(si);

  egop->head += 4;
  egop->size += 4;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addFString(SYS_PACKET * egop, const char *format, ...)
{
  char stringbuffer[COPYSIZE];
  va_list args;

  va_start(args, format);
  vsnprintf(stringbuffer, sizeof(stringbuffer)-1, format, args);
  va_end(args);

  sys_packet_addString(egop, stringbuffer);
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addString(SYS_PACKET * egop, const char *string)
{
  /// @details ZZ@> This function appends a null terminated string to the packet
  char* cp;
  char cTmp;
  int cnt;

  cnt = 0;
  cTmp = 1;
  cp = (char*)(&egop->buffer[egop->head]);
  while (cTmp != 0)
  {
    cTmp = string[cnt];
    *cp = cTmp;
    cp += 1;
    egop->head += 1;
    egop->size += 1;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
INLINE bool_t net_startNewSysPacket(SYS_PACKET * egop)
{
  /// @details ZZ@> This function starts building a network packet

  if(NULL == egop)
  {
    net_logf("NET ERROR: net_startNewSysPacket() - Called with invalid parameters.\n");
    return bfalse;
  }

  egop->head = 0;
  egop->size = 0;

  return btrue;
}
