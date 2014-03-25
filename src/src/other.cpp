#include "shorten.hpp"
#include "bitshift.h"

void ShnDecoder::init_offset(int nchan,int nblock,int ftype)
{
  slong mean = 0;
  int  chan, i;

  /* initialise offset */
  switch(ftype)
  {
    case TYPE_AU1:
    case TYPE_S8:
    case TYPE_S16HL:
    case TYPE_S16LH:
    case TYPE_ULAW:
    case TYPE_AU2:
    case TYPE_AU3:
    case TYPE_ALAW:
      mean = 0;
      break;
    case TYPE_U8:
      mean = 0x80;
      break;
    case TYPE_U16HL:
    case TYPE_U16LH:
      mean = 0x8000;
      break;
    default:
      ddebug("unknown file type: %d\n", ftype);
  }

  for(chan = 0; chan < nchan; chan++)
    for(i = 0; i < nblock; i++)
      offset.toc[chan][i] = mean;
}

/*
slong **long2d(ulong n0, ulong n1) {
  slong **array0;

  if((array0 = (slong**) pmalloc((ulong) (n0 * sizeof(slong*) +
      n0 * n1 * sizeof(slong)))) != NULL ) {
    slong *array1 = (slong*) (array0 + n0);
    int i;

    for(i = 0; i < n0; i++)
      array0[i] = array1 + i * n1;
  }
  return(array0);
}
*/

