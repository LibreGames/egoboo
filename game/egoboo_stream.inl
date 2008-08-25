#pragma once

#include "egoboo_stream.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t stream_startFile(STREAM * pwrapper, FILE * pfile);
INLINE bool_t stream_startRaw(STREAM * pwrapper, Uint8 * buffer, size_t buffer_size);
INLINE bool_t stream_startLocal(STREAM * pwrapper, struct s_local_packet * pegopkt);
INLINE bool_t stream_startENet(STREAM * pwrapper, ENetPacket * packet);
INLINE bool_t stream_startRemote(STREAM * pwrapper, NET_PACKET * pnetpkt);
INLINE bool_t stream_done(STREAM * pwrapper);

INLINE bool_t stream_readString(STREAM * p, char *buffer, size_t maxLen);
INLINE Uint8  stream_readUint8(STREAM * p);
INLINE Sint8  stream_readSint8(STREAM * p);
INLINE Uint16 stream_readUint16(STREAM * p);
INLINE Uint16 stream_peekUint16(STREAM * p);
INLINE Sint16 stream_readSint16(STREAM * p);
INLINE Uint32 stream_readUint32(STREAM * p);
INLINE Sint32 stream_readSint32(STREAM * p);
INLINE size_t stream_remainingSize(STREAM * p);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t stream_reset(STREAM * pwrapper)
{
  if(NULL ==pwrapper) return bfalse;

  memset(pwrapper, 0, sizeof(STREAM));
  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t stream_startFile(STREAM * pwrapper, FILE * pfile)
{
  long pos;

  if(NULL ==pwrapper || NULL ==pfile || feof(pfile)) return bfalse;

  stream_reset(pwrapper);

  pwrapper->pfile        = pfile;

  // set the read position in the file
  pos = ftell(pfile);
  pwrapper->readLocation = pos;

  // measure the file length
  fseek(pfile, 0, SEEK_END);
  pwrapper->data_size    = ftell(pfile);

  // reset the file stream
  fseek(pfile, pos, SEEK_SET);

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t stream_startRaw(STREAM * pwrapper, Uint8 * buffer, size_t buffer_size)
{
  if(NULL ==pwrapper || NULL ==buffer || 0==buffer_size) return bfalse;

  stream_reset(pwrapper);
  pwrapper->data         = buffer;
  pwrapper->data_size    = buffer_size;
  pwrapper->readLocation = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t stream_startLocal(STREAM * pwrapper, SYS_PACKET * pegopkt)
{
  if(NULL ==pwrapper || NULL ==pegopkt) return bfalse;

  stream_reset(pwrapper);
  pwrapper->data         = pegopkt->buffer;
  pwrapper->data_size    = pegopkt->size;
  pwrapper->readLocation = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t stream_startENet(STREAM * pwrapper, ENetPacket * packet)
{
  if(NULL ==pwrapper || NULL ==packet) return bfalse;

  stream_reset(pwrapper);
  pwrapper->data         = packet->data;
  pwrapper->data_size    = packet->dataLength;
  pwrapper->readLocation = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t stream_startRemote(STREAM * pwrapper, NET_PACKET * pnetpkt)
{
  if(NULL ==pwrapper || NULL ==pnetpkt) return bfalse;

  stream_reset(pwrapper);
  pwrapper->data         = pnetpkt->pkt->data;
  pwrapper->data_size    = pnetpkt->pkt->dataLength;
  pwrapper->readLocation = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t stream_done(STREAM * pwrapper)
{
  return stream_reset(pwrapper);
}

//--------------------------------------------------------------------------------------------
INLINE bool_t stream_readString(STREAM * p, char *buffer, size_t maxLen)
{
  /// @details ZZ@> This function reads a NULL terminated string from the packet
  size_t copy_length;

  if(NULL ==p) return bfalse;
  if(p->data_size==0 && maxLen==0) return btrue;
  if(p->data_size==0 && maxLen>0)
  {
    buffer[0] = EOS;
    return btrue;
  };

  copy_length = MIN(maxLen, p->data_size);
  strncpy(buffer, (char *)(p->data + p->readLocation), copy_length);
  copy_length = MIN(copy_length, strlen(buffer) + 1);
  p->readLocation += copy_length;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE Uint8 stream_readUint8(STREAM * p)
{
  /// @details ZZ@> This function reads an Uint8 from the packet
  Uint8 uc;
  uc = (Uint8)(p->data[p->readLocation]);
  p->readLocation += sizeof(Uint8);
  return uc;
}

//--------------------------------------------------------------------------------------------
INLINE Sint8 stream_readSint8(STREAM * p)
{
  /// @details ZZ@> This function reads a Sint8 from the packet
  Sint8 sc;
  sc = (Sint8)(p->data[p->readLocation]);
  p->readLocation += sizeof(Sint8);
  return sc;
}

//--------------------------------------------------------------------------------------------
INLINE Uint16 stream_readUint16(STREAM * p)
{
  /// @details ZZ@> This function reads an Uint16 from the packet
  Uint16 us;
  Uint16* usp;
  usp = (Uint16*)(&p->data[p->readLocation]);

  us = ENET_NET_TO_HOST_16(*usp);

  p->readLocation += sizeof(Uint16);
  return us;
}

//--------------------------------------------------------------------------------------------
INLINE Uint16 stream_peekUint16(STREAM * p)
{
  /// @details ZZ@> This function reads an Uint16 from the packet
  Uint16 us;
  Uint16* usp;
  usp = (Uint16*)(&p->data[p->readLocation]);

  us = ENET_NET_TO_HOST_16(*usp);

  return us;
}

//--------------------------------------------------------------------------------------------
INLINE Sint16 stream_readSint16(STREAM * p)
{
  /// @details ZZ@> This function reads a Sint16 from the packet
  Sint16 ss;
  Sint16* ssp;
  ssp = (Sint16*)(&p->data[p->readLocation]);

  ss = ENET_NET_TO_HOST_16(*ssp);

  p->readLocation += sizeof(Sint16);
  return ss;
}

//--------------------------------------------------------------------------------------------
INLINE Uint32 stream_readUint32(STREAM * p)
{
  /// @details ZZ@> This function reads an Uint32 from the packet
  Uint32 ui;
  Uint32* uip;
  uip = (Uint32*)(&p->data[p->readLocation]);

  ui = ENET_NET_TO_HOST_32(*uip);

  p->readLocation += sizeof(Uint32);
  return ui;
}

//--------------------------------------------------------------------------------------------
INLINE Sint32 stream_readSint32(STREAM * p)
{
  /// @details ZZ@> This function reads a Sint32 from the packet
  Sint32 si;
  Sint32* sip;
  sip = (Sint32*)(&p->data[p->readLocation]);

  si = ENET_NET_TO_HOST_32(*sip);

  p->readLocation += sizeof(Sint32);
  return si;
}

//--------------------------------------------------------------------------------------------
INLINE size_t stream_remainingSize(STREAM * p)
{
  /// @details ZZ@> This function tells if there's still data left in the packet
  return p->data_size - p->readLocation;
}
