/******************************************************************************
*                                                                             *
*       Copyright (C) 1992-1995 Tony Robinson                                 *
*                                                                             *
*       See the file LICENSE for conditions on distribution and usage         *
*                                                                             *
******************************************************************************/

/*
 * $Id: vario.cpp,v 1.5 2008/01/18 20:59:58 tim Exp $
 */

#include "shorten.hpp"

void ShnDecoder::mkmasktab() {
  int i;
  ulong val = 0;

  masktab[0] = val;
  for(i = 1; i < MASKTABSIZE; i++) {
    val <<= 1;
    val |= 1;
    masktab[i] = val;
  }
}

ulong ShnDecoder::word_get()
{
  ulong buffer;

  if(nbyteget < 4)
  {
    int bytes;

/*    LastBufferReadPosition = (ulong)ftell(filei); */
//#ifdef DEBUG
//	int realpos=(ulong)ftell(filei);
//#endif

    LastBufferReadPosition = bytes_read;
    bytes = fread((char*) getbuf, 1, BUFSIZ, filei);
    nbyteget += bytes;
    if(nbyteget < 4) {
      ddebug("premature EOF (get only %d/%d bytes) on compressed stream at %d...\n",bytes,BUFSIZ,bytes_read);
      throw("premature EOF on compressed stream");
	  }
    bytes_read += bytes;
    getbufp = getbuf;
  }

  buffer = (((slong) getbufp[0]) << 24) | (((slong) getbufp[1]) << 16) |
    (((slong) getbufp[2]) <<  8) | ((slong) getbufp[3]);

  getbufp += 4;
  nbyteget -= 4;

  return(buffer);
}

slong ShnDecoder::uvar_get(int nbin)
{
  slong result;

  if(ReadingFunctionCode == TRUE)
  {
    seek.SHNLastBufferReadPosition = (ulong)  LastBufferReadPosition;
    seek.SHNFilePosition           = (ulong)  ftell(filei);
    seek.SHNBitPosition            = (ushort) nbitget;
    seek.SHNGBuffer                = (ulong)  gbuffer;
    seek.SHNByteGet                = (ushort) nbyteget;
    seek.SHNBufferOffset           = (ushort) (getbufp-getbuf);
  }

  if(nbitget == 0)
  {
    gbuffer = word_get();
    nbitget = 32;
  }

  for(result = 0; !(gbuffer & (1L << --nbitget)); result++)
  {
    if(nbitget == 0)
    {
      gbuffer = word_get();
      nbitget = 32;
    }
  }

  while(nbin != 0)
  {
    if(nbitget >= nbin)
    {
      result = (result << nbin) | ((gbuffer >> (nbitget-nbin)) &masktab[nbin]);
      nbitget -= nbin;
      nbin = 0;
    }
    else
    {
      result = (result << nbitget) | (gbuffer & masktab[nbitget]);
      gbuffer = word_get();
      nbin -= nbitget;
      nbitget = 32;
    }
  }

#if (DEBUG>2)
  if(ReadingFunctionCode == TRUE)
    ddebug("CMD:%d\n",result);
  else
    ddebug("=%d\n",result);
#endif
  return(result);
}


