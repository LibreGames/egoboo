#pragma once

#include "Network.h"

#include "egoboo_types.inl"

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
  // ZZ> This function reads a null terminated string from the packet

  if(NULL ==p) return bfalse;

  return stream_readString(&(p->wrapper), buffer, maxLen);
}

//--------------------------------------------------------------------------------------------
INLINE Uint8 net_packet_readUint8(NET_PACKET * p)
{
  // ZZ> This function reads an Uint8 from the packet
  if(NULL ==p) return 0;

  return stream_readUint8(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Sint8 net_packet_readSint8(NET_PACKET * p)
{
  // ZZ> This function reads a Sint8 from the packet
  if(NULL ==p) return 0;

  return stream_readSint8(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Uint16 net_packet_readUint16(NET_PACKET * p)
{
  // ZZ> This function reads an Uint16 from the packet
  if(NULL ==p) return 0;

  return stream_readUint16(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Uint16 net_packet_peekUint16(NET_PACKET * p)
{
  // ZZ> This function reads an Uint16 from the packet
  if(NULL ==p) return 0;

  return stream_peekUint16(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Sint16 net_packet_readSint16(NET_PACKET * p)
{
  // ZZ> This function reads a Sint16 from the packet
  if(NULL ==p) return 0;

  return stream_readSint16(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Uint32 net_packet_readUint32(NET_PACKET * p)
{
  // ZZ> This function reads an Uint32 from the packet
  if(NULL ==p) return 0;

  return stream_readUint32(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE Sint32 net_packet_readSint32(NET_PACKET * p)
{
  // ZZ> This function reads a Sint32 from the packet
  if(NULL ==p) return 0;

  return stream_readSint32(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE size_t net_packet_remainingSize(NET_PACKET * p)
{
  // ZZ> This function tells if there's still data left in the packet
  if(NULL ==p) return 0;

  return stream_remainingSize(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addUint8(SYS_PACKET * egop, Uint8 uc)
{
  // ZZ> This function appends an Uint8 to the packet
  Uint8* ucp;
  ucp = (Uint8*)(&egop->buffer[egop->head]);
  *ucp = uc;
  egop->head += 1;
  egop->size += 1;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addSint8(SYS_PACKET * egop, Sint8 sc)
{
  // ZZ> This function appends a Sint8 to the packet
  Sint8* scp;
  scp = (Sint8*)(&egop->buffer[egop->head]);
  *scp = sc;
  egop->head += 1;
  egop->size += 1;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addUint16(SYS_PACKET * egop, Uint16 us)
{
  // ZZ> This function appends an Uint16 to the packet
  Uint16* usp;
  usp = (Uint16*)(&egop->buffer[egop->head]);

  *usp = ENET_HOST_TO_NET_16(us);
  egop->head += 2;
  egop->size += 2;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addSint16(SYS_PACKET * egop, Sint16 ss)
{
  // ZZ> This function appends a Sint16 to the packet
  Sint16* ssp;
  ssp = (Sint16*)(&egop->buffer[egop->head]);

  *ssp = ENET_HOST_TO_NET_16(ss);

  egop->head += 2;
  egop->size += 2;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addUint32(SYS_PACKET * egop, Uint32 ui)
{
  // ZZ> This function appends an Uint32 to the packet
  Uint32* uip;
  uip = (Uint32*)(&egop->buffer[egop->head]);

  *uip = ENET_HOST_TO_NET_32(ui);

  egop->head += 4;
  egop->size += 4;
}

//--------------------------------------------------------------------------------------------
INLINE void sys_packet_addSint32(SYS_PACKET * egop, Sint32 si)
{
  // ZZ> This function appends a Sint32 to the packet
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
  // ZZ> This function appends a null terminated string to the packet
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
  // ZZ> This function reads a NULL terminated string from the packet
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
  // ZZ> This function reads an Uint8 from the packet
  Uint8 uc;
  uc = (Uint8)(p->data[p->readLocation]);
  p->readLocation += sizeof(Uint8);
  return uc;
}

//--------------------------------------------------------------------------------------------
INLINE Sint8 stream_readSint8(STREAM * p)
{
  // ZZ> This function reads a Sint8 from the packet
  Sint8 sc;
  sc = (Sint8)(p->data[p->readLocation]);
  p->readLocation += sizeof(Sint8);
  return sc;
}

//--------------------------------------------------------------------------------------------
INLINE Uint16 stream_readUint16(STREAM * p)
{
  // ZZ> This function reads an Uint16 from the packet
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
  // ZZ> This function reads an Uint16 from the packet
  Uint16 us;
  Uint16* usp;
  usp = (Uint16*)(&p->data[p->readLocation]);

  us = ENET_NET_TO_HOST_16(*usp);

  return us;
}

//--------------------------------------------------------------------------------------------
INLINE Sint16 stream_readSint16(STREAM * p)
{
  // ZZ> This function reads a Sint16 from the packet
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
  // ZZ> This function reads an Uint32 from the packet
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
  // ZZ> This function reads a Sint32 from the packet
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
  // ZZ> This function tells if there's still data left in the packet
  return p->data_size - p->readLocation;
}



//--------------------------------------------------------------------------------------------
INLINE bool_t net_startNewSysPacket(SYS_PACKET * egop)
{
  // ZZ> This function starts building a network packet

  if(NULL == egop)
  {
    net_logf("NET ERROR: net_startNewSysPacket() - Called with invalid parameters.\n");
    return bfalse;
  }

  egop->head = 0;
  egop->size = 0;

  return btrue;
}
