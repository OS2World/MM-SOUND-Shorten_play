#ifndef __SHORTEN_HPP
#define __SHORTEN_HPP

/******************************************************************************
*                                                                             *
*       Copyright (C) 1992-1997 Tony Robinson and SoftSound Limited           *
*                                                                             *
*       See the file LICENSE for conditions on distribution and usage         *
*                                                                             *
******************************************************************************/

/*
 * $Id: shorten.hpp,v 1.8 2008/01/16 18:45:23 tim Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __OS2__
#include <os2.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <map>
#include <functional>

//#include <setjmp.h>
extern "C" {
#include "shorten.h"
};

#define V2LPCQOFFSET (1 << LPCQUANT);

#define TYPE_S16LH_SIZE ((signed)sizeof(sshort))

#define MASKTABSIZE 33

typedef struct tag_TSeekTableHeader
{
  uchar data[SEEK_HEADER_SIZE];
}TSeekTableHeader;

typedef struct tag_TSeekTableTrailer
{
  uchar data[SEEK_TRAILER_SIZE];
}TSeekTableTrailer;

typedef struct tag_TSeekEntry
{
  uchar data[SEEK_ENTRY_SIZE];
}TSeekEntry;

#pragma pack(2)
struct TSeekEntryVerbose
{
ulong	SampleNumber,
		SHNFilePosition,
		SHNLastBufferReadPosition;

ushort  SHNByteGet,
		SHNBufferOffset,
		SHNBitPosition;
ulong	SHNGBuffer;
ushort  bitshift;

slong   CBuf_0_Minus1,
        CBuf_0_Minus2,
        CBuf_0_Minus3,

        CBuf_1_Minus1,
        CBuf_1_Minus2,
        CBuf_1_Minus3,

        Offset_0_0,
        Offset_0_1,
        Offset_0_2,
        Offset_0_3,

        Offset_1_0,
        Offset_1_1,
        Offset_1_2,
        Offset_1_3;
	};

typedef std::map<ulong,TSeekEntryVerbose,std::greater<ulong> > TSeekTable;

#pragma pack()

#ifndef STRNEWDUP_DEFINED
inline char* strnewdup(const char* str) {
    return str?strcpy(new char[strlen(str)+1],str):NULL;
    }
#define STRNEWDUP_DEFINED
#endif

#ifndef SHNSEEKTABLEEXT
#define SHNSEEKTABLEEXT ".skt"
#endif

class TocData{
	public:
	slong** toc;
	slong*  data;

	TocData() {
		toc=NULL;
		data=NULL;
		}
	void init(ulong n0, ulong n1) {
		toc  = new (slong*)[n0];
		data = new slong[n0*n1];

		for (ulong i=0; i<n0; i++) {
			toc[i]=data+i*n1;
			}
		}
	~TocData() {
		if (toc) delete[] toc;
		if (data) delete[] data;
		}

	};

inline int sizeof_uvar(ulong val, int nbin) {
        return((val >> nbin) + nbin);
        }

inline int sizeof_var(slong val, int nbin) {
        return((labs(val) >> nbin) + nbin + 1);
        }


class ShnDecoder {
protected:
        int  Pass;

        ulong bytes_read;

        //FILE *fileo;

        /* globals for seek table */


        ulong LastBufferReadPosition;

        int ReadingFunctionCode;

		TSeekEntryVerbose seek;
	    TSeekTable seektable;
		int load_seektable();
		int load_external_seektable();
		ulong seekAtSample(ulong sample);
          // find the closest entry in seektable
          // reprogram engine
          // report resulting position (sample)

/*
	***not used in decoder***
        TSeekTableHeader SeekTableHeader;
        TSeekTableTrailer SeekTableTrailer;
*/

          slong  lpcqoffset;
          int   version, lastbitshift, want_seeking;
          int   ftype;
          char  *magic, *filenamei;
        //, *filenamei_orig, *dupfilenamei, *dupfilenameo;
        //  char  *tmpfilename;
//          char  *maxresnstr;
          FILE  *filei;
          int   blocksize, reblocksize, nchan, chan;
          int   nskip, ndiscard;
          int   *qlpc, maxnlpc, nmean;
          int   quanterror, minsnr, nfilename;
          int   ulawZeroMerge;
          slong  datalen;
          Iff_Header *wavhdr;
          ulong WriteCount;
          int internal_ftype;
          int nwrap;

          char *writebuf;
        #ifdef WORDS_BIGENDIAN
          char *writefub;
        #endif
          int  nwritebuf;

          TocData buffer, offset;

        // from vario.c
        	uchar getbuf[BUFSIZ];
        	uchar *getbufp;
        	int    nbyteget;
			ulong  gbuffer;
			int    nbitget;
		    ulong masktab[MASKTABSIZE];
		    int eof;
    int isEof() {
    	return eof;
    	}
	ulong ulong_get() {
  		return(uvar_get(uvar_get(ULONGSIZE)));
		}

	const char* opened_file() {
		return filenamei;
		}

    slong uvar_get(int nbin);
    void  mkmasktab();
	ulong word_get();
	unsigned long shorten_filepos() {
		return seek.SHNFilePosition +getbufp-getbuf;
		}
    unsigned long shorten_samples() {
        return seek.SampleNumber;
        }
	slong var_get(int nbin) {
  		ulong uvar = (ulong) uvar_get(nbin + 1);
        if (uvar & 1)
        	return((slong) ~(uvar >> 1));
	    else
	    	return((slong) (uvar >> 1));
		}

  virtual int waveoutput(const char* buf,unsigned int size)=0;
  int shnwrite(int nitem);
  void shn_fix_bitshift(slong *buffer, int bitshift);
  void init_offset(int nchan,int nblock,int ftype);

  int buffer_output() {
        return shnwrite(blocksize);
    	}
  virtual int byte_output(int byte)=0;
/*  int byte_output(int ByteToWrite) {
  		return waveoutput((char*)&ByteToWrite,1);
		}
*/

  ulong UINT_GET(int nbit) {
    	return (version == 0)
    		? uvar_get(nbit)
    		: ulong_get();
    	}

public:
		ShnDecoder();
		virtual ~ShnDecoder();
		int init(const char* filename, int loadseektable=0);

		void recheckbuf() {
	    	if (nwritebuf < nchan * blocksize * TYPE_S16LH_SIZE) {
	    		nwritebuf = nchan * blocksize * TYPE_S16LH_SIZE;
		    	if (writebuf) {
		        	delete[] writebuf;
		        	writebuf=new char[nwritebuf];
		        	}
#ifdef WORDS_BIGENDIAN
			    if (writefub) {
			        delete[] writefub;
			        writefub=new char[nwritebuf];
		    	    }
#endif
				}
			}
		int close();
		int proceed();

};


inline ulong uchar_to_ulong_le(uchar * buf)
/* converts 4 bytes stored in little-endian format to a ulong */
{
  return (ulong)((buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0]);
}

inline void ulong_to_uchar_le(uchar buf[],ulong num)
/* converts a ulong to 4 bytes stored in little-endian format */
{
  buf[0] = (uchar)(num);
  buf[1] = (uchar)(num >> 8);
  buf[2] = (uchar)(num >> 16);
  buf[3] = (uchar)(num >> 24);
}

inline void long_to_uchar_le(uchar buf[],slong num)
{
  ulong_to_uchar_le(buf,(ulong)num);
}

inline void ushort_to_uchar_le(uchar buf[],ushort num)
/* converts a ushort to 2 bytes stored in little-endian format */
{
  buf[0] = (uchar)(num);
  buf[1] = (uchar)(num >> 8);
}

#ifndef DEBUG_DEFINED
#define DEBUG_DEFINED
#ifdef DEBUG
int debug(const char* str, ...) __attribute__ ((format (printf, 1, 2)));
#if (DEBUG > 1)
#define ddebug debug
#else
inline int ddebug(const char* str, ...) { return -1; }
#endif
#else
inline int ddebug(const char* str, ...) { return -1; }
inline int  debug(const char* str, ...) { return -1; }
#endif
#endif

#define ddebug_ret return ddebug
#define debug_ret  return  debug

extern char ulaw_maxshift[256];
extern schar ulaw_inward[13][256];
extern uchar ulaw_outward[13][256];

#endif  /*__SHORTEN_HPP*/

