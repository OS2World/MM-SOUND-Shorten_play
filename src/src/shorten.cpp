#include "shorten.hpp"

ShnDecoder::ShnDecoder():
	seektable(), buffer(), offset() {
	ReadingFunctionCode=FALSE;
	memset(&seek,0,sizeof(seek));

	filenamei = NULL;
    filei = NULL;

	writebuf=NULL;
#ifdef WORDS_BIGENDIAN
    writefub=NULL;
#endif
    nwritebuf=0;

  ftype = TYPE_S16LH;
  magic = MAGIC;

//  maxresnstr = DEFAULT_MAXRESNSTR;
  blocksize = DEFAULT_BLOCK_SIZE;
  nchan = DEFAULT_NCHAN;

  nskip = DEFAULT_NSKIP;
  ndiscard = DEFAULT_NDISCARD;

  maxnlpc = DEFAULT_MAXNLPC;
  nmean = UNDEFINED_UINT;

  quanterror = DEFAULT_QUANTERROR;
  minsnr = DEFAULT_MINSNR;

	reblocksize=0;
	}

ShnDecoder::~ShnDecoder() {
	close();
	}

int ShnDecoder::close() {
	if (filenamei) {
		delete[] filenamei;
		filenamei=NULL;
		}
    if (filei) {
        fclose(filei);
        filei=NULL;
    	}
    if (writebuf) {
    	delete[] writebuf;
    	writebuf=NULL;
		}
#ifdef WORDS_BIGENDIAN
    if (writefub) {
        delete[] writefub;
        writefub=NULL;
        }
#endif
    nwritebuf=0;

    if (qlpc) {
        delete[] qlpc;
        qlpc = NULL;
        }

	seektable.clear();
    reblocksize=0;
    return 0;
	}

int ShnDecoder::init(const char* filename, int loadseektable) {

  ftype = TYPE_S16LH;
  magic = MAGIC;

//  maxresnstr = DEFAULT_MAXRESNSTR;
  blocksize = DEFAULT_BLOCK_SIZE;
  nchan = DEFAULT_NCHAN;

  nskip = DEFAULT_NSKIP;
  ndiscard = DEFAULT_NDISCARD;

  maxnlpc = DEFAULT_MAXNLPC;
  nmean = UNDEFINED_UINT;

  quanterror = DEFAULT_QUANTERROR;
  minsnr = DEFAULT_MINSNR;

  reblocksize=0;
  lpcqoffset = 0;
  version = MAX_SUPPORTED_VERSION;
  lastbitshift = 0;
  WriteCount=0;

  memset(&seek,0,sizeof(seek));
  seektable.clear();
  seektable[0]=seek;

  if (filenamei) delete[] filenamei;
  filenamei = strnewdup(filename);
//  filenamei_orig = NULL;
//  dupfilenamei = NULL;
//  dupfilenameo = NULL;

  qlpc = NULL;

  ulawZeroMerge = 0;
  datalen = -1;
  wavhdr = NULL;

  filei = fopen(filenamei,"rb");
  if (filei==NULL) {
  	ddebug("fail to open: %s\n",filename);
  	return -1;
  	}

    // load seektable (if any)
  if (loadseektable&1) {
    load_seektable();
    if (fseek(filei,0,SEEK_SET)) {
        ddebug_ret("input '%s' seek (after load seektable) failed: %s\n",filename,strerror(errno));
    	}
	}
  if (loadseektable&2)
    load_external_seektable();

  /* initialize the seek table header and trailer */

/*  SeekTableHeader.data[0]='S';
  SeekTableHeader.data[1]='E';
  SeekTableHeader.data[2]='E';
  SeekTableHeader.data[3]='K';
  ulong_to_uchar_le(SeekTableHeader.data+4,SEEK_TABLE_REVISION);
  ulong_to_uchar_le(SeekTableHeader.data+8,0);

  SeekTableTrailer.data[4]='S';
  SeekTableTrailer.data[5]='H';
  SeekTableTrailer.data[6]='N';
  SeekTableTrailer.data[7]='A';
  SeekTableTrailer.data[8]='M';
  SeekTableTrailer.data[9]='P';
  SeekTableTrailer.data[10]='S';
  SeekTableTrailer.data[11]='K';
*/

  if (version > FORMAT_VERSION)
      version = FORMAT_VERSION;

  if(nmean == UNDEFINED_UINT)
    nmean = (version < 2) ? DEFAULT_V0NMEAN : DEFAULT_V2NMEAN;

  /* discard header on input file - can't rely on fseek() here */
  // ntim override: rely on fseek (good for a files only)

  if(ndiscard != 0) {
    if(fseek(filei, ndiscard, SEEK_SET)) {
        ddebug("input '%s' seek failed: %s\n",filename,strerror(errno));
        return -2;
    	}
    bytes_read=ndiscard;
    ddebug("input '%s': %d byte(s) discarded...\n",filename,ndiscard);
    }

  nwritebuf=nchan * blocksize * TYPE_S16LH_SIZE;
  writebuf=new char[nwritebuf];
#ifdef WORDS_BIGENDIAN
  writefub=new char[nwritebuf];
#endif



    /* Firstly skip the number of bytes requested in the command line */
    for(int i = 0; i < nskip; i++)
    {
      int byte = getc(filei);
      bytes_read++;
      if(byte == EOF)
        ddebug_ret("File too short for requested alignment\n");
      //putc_exit(byte, fileo);
    }
    if (nskip) ddebug("input '%s': %d byte(s) discarded (from cmd-line)...\n",filename,nskip);

    /* read magic number */
#ifdef STRICT_FORMAT_COMPATABILITY
    if(FORMAT_VERSION < 2)
    {
      for(int i = 0; i < strlen(magic); i++) {
        if(getc_exit(filei) != magic[i])
          ddebug_ret("Bad magic number\n");
        bytes_read++;
      }

      /* get version number */
      version = getc_exit(filei);
      bytes_read++;
    }
    else
#endif /* STRICT_FORMAT_COMPATABILITY */
    {
      int nscan = 0;

      version = MAX_VERSION + 1;
      while(version > MAX_VERSION)
      {
        int byte = getc(filei);
        bytes_read++;
        if(byte == EOF) {
            ddebug_ret("No magic number\n");
        }
        if(magic[nscan] != '\0' && byte == magic[nscan])
          nscan++;
        else if(magic[nscan] == '\0' && byte <= MAX_VERSION)
          version = byte;
        else
        {
          if(byte == magic[0])
            nscan = 1;
          else
          {
            nscan = 0;
          }
          version = MAX_VERSION + 1;
        }
      }
    }

    /* check version number */
    if(version > MAX_SUPPORTED_VERSION)
      ddebug_ret("can't decode version %d\n", version);

    /* set up the default nmean, ignoring the command line state */
    nmean = (version < 2) ? DEFAULT_V0NMEAN : DEFAULT_V2NMEAN;

    /* initialise the variable length file read for the compressed stream */
  mkmasktab();
  getbufp  = getbuf;
  nbyteget = 0;
  gbuffer  = 0;
  nbitget  = 0;

    /* initialise the fixed length file write for the uncompressed stream */
//    fwrite_type_init();

    /* get the internal file type */
    internal_ftype = UINT_GET(TYPESIZE);
    ddebug("shorten::init : internal_ftype = %d...\n",internal_ftype);

    /* has the user requested a change in file type? */
    if(internal_ftype != ftype) {
      if(ftype == TYPE_EOF)
        ftype = internal_ftype;    /*  no problems here */
      else             /* check that the requested conversion is valid */
        if(internal_ftype == TYPE_AU1 || internal_ftype == TYPE_AU2 ||
           internal_ftype == TYPE_AU3 || ftype == TYPE_AU1 ||ftype == TYPE_AU2 || ftype == TYPE_AU3)
          ddebug_ret("Not able to perform requested output format conversion\n");
    }

    nchan = UINT_GET(CHANSIZE);
    ddebug("shorten::init : channels = %d...\n",nchan);

    /* get blocksize if version > 0 */
    if(version > 0)
    {
      blocksize = UINT_GET((int) (log((double) DEFAULT_BLOCK_SIZE) / M_LN2));
      maxnlpc = UINT_GET(LPCQSIZE);
      nmean = UINT_GET(0);
      nskip = UINT_GET(NSKIPSIZE);
      for(int i = 0; i < nskip; i++)
      {
        //int byte =
            uvar_get(XBYTESIZE);
        //putc_exit(byte, fileo);
      }
    }
    else
      blocksize = DEFAULT_BLOCK_SIZE;

    recheckbuf();

    nwrap = MAX(NWRAP, maxnlpc);

    /* grab some space for the input buffer */
    buffer.init((ulong) nchan, (ulong) (blocksize + nwrap));
    offset.init((ulong) nchan, (ulong) MAX(1, nmean));
	reblocksize=blocksize;

    for(int lchan = 0; lchan < nchan; lchan++)
    {
      for(int i = 0; i < nwrap; i++)
       buffer.toc[lchan][i] = 0;
      buffer.toc[lchan] += nwrap;
    }

    if (qlpc) {
        delete[] qlpc;
        qlpc = NULL;
        }
    if(maxnlpc > 0)
      qlpc = new int[maxnlpc];

    if(version > 1)
      lpcqoffset = V2LPCQOFFSET;

    init_offset(nchan, MAX(1, nmean), internal_ftype);

    seek.bitshift = 0;
    bytes_read = 0;

    /* get commands from file and execute them */
    chan = 0;
    eof = 0;

    ddebug("ShnDecoder::init final - seek tables (loadseektable=%d) contain %d elements...\n",loadseektable,seektable.size());

    return 0; //success
  }

int ShnDecoder::load_seektable() {

    char SeekTrailerSignature[9]="SHNAMPSK";
    char SeekHeaderSignature[5]="SEEK";

    TSeekTableTrailer SeekTrailer;
    TSeekTableHeader SeekHeader;

    if (fseek(filei,-SEEK_TRAILER_SIZE,SEEK_END))
    	ddebug_ret("ShnDecoder::load_seektable - fseek failed: %s\n",strerror(errno));

    if (fread(SeekTrailer.data,1,SEEK_TRAILER_SIZE,filei)!=SEEK_TRAILER_SIZE)
        ddebug_ret("ShnDecoder::load_seektable - fread(%d bytes) failed: %s\n",SEEK_TRAILER_SIZE,strerror(errno));

    if(memcmp(SeekTrailerSignature,SeekTrailer.data+4,8))
        ddebug_ret("ShnDecoder::load_seektable - check TrailerSignature failed!\n");

    if (fseek(filei,-uchar_to_ulong_le(SeekTrailer.data),SEEK_END))
    	ddebug_ret("ShnDecoder::load_seektable - fseek#2(-%u) failed: %s\n",uchar_to_ulong_le(SeekTrailer.data),strerror(errno));
    if (fread(SeekHeader.data,1,SEEK_HEADER_SIZE,filei)!=SEEK_HEADER_SIZE)
    	ddebug_ret("ShnDecoder::load_seektable - fread#2(%d bytes) failed: %s\n",SEEK_HEADER_SIZE,strerror(errno));
    if (memcmp(SeekHeaderSignature,SeekHeader.data,4))
        ddebug_ret("ShnDecoder::load_seektable - check HeaderSignature failed!\n");

    if ( uchar_to_ulong_le(SeekHeader.data+4) != SEEK_TABLE_REVISION)
        ddebug_ret("ShnDecoder::load_seektable - unknown SEEK_TABLE_REVISION (%u)!\n",uchar_to_ulong_le(SeekHeader.data+4));

	int amount=(uchar_to_ulong_le(SeekTrailer.data)-SEEK_TRAILER_SIZE)/sizeof(TSeekEntryVerbose);

    TSeekEntryVerbose seek;
	for(int i=0; i<amount; i++) {
		if (fread(&seek,1,sizeof(seek),filei)!=sizeof(seek))
			ddebug_ret("ShnDecoder::load_seektable - fread entry#%d (%d bytes) failed: %s\n",i,sizeof(seek),strerror(errno));
		seektable[seek.SampleNumber]=seek;
		}
    ddebug("ShnDecoder::load_seektable - %u entries loaded!\n",amount);


    return 0;
    }

int ShnDecoder::load_external_seektable() {
      char SeekHeaderSignature[5]="SEEK";
      TSeekTableHeader SeekHeader;

      int r=strlen(filenamei);
      char* fname=strcpy(new char[r+4],filenamei);
      for (r--;r>0;r--) {
      	if (fname[r]=='.') {
      	    strcpy(fname+r,SHNSEEKTABLEEXT);
      	    break;
      		}
        if (fname[r]=='\\'||fname[r]=='/') {
        	r=0;
            }
        }
      if (r<0)
         strcat(fname,SHNSEEKTABLEEXT);

      FILE* fp=fopen(fname,"rb");
      class Remover {
        		FILE* fp;
        		char* fname;
        	public:
        		Remover(FILE* _fp,char* _fname):
        			fp(_fp),
        			fname(_fname)
        			{}
        		~Remover() {
        			if (fp)
        				fclose(fp);
        			if (fname)
        				delete[] fname;
        			}
        } remover(fp,fname);

      if (fp==NULL)
         ddebug_ret("ShnDecoder::load_external_seektable - can't open '%s': %s\n",fname,strerror(errno));
      if (fread(SeekHeader.data,1,SEEK_HEADER_SIZE,fp)!=SEEK_HEADER_SIZE)
         ddebug_ret("ShnDecoder::load_external_seektable - %s - fread(%d) failed: %s\n",fname,SEEK_HEADER_SIZE,strerror(errno));
      if (memcmp(SeekHeaderSignature,SeekHeader.data,4))
         ddebug_ret("ShnDecoder::load_external_seektable - %s - check HeaderSignature failed!\n",fname);
      if ( uchar_to_ulong_le(SeekHeader.data+4) != SEEK_TABLE_REVISION)
         ddebug_ret("ShnDecoder::load_external_seektable - %s - unknown SEEK_TABLE_REVISION (%u)!\n",fname,uchar_to_ulong_le(SeekHeader.data+4));

      TSeekEntryVerbose seek;
      unsigned int res=0,count=0;
      while ((res=fread(&seek,1,sizeof(seek),fp))==sizeof(seek)) {
        	seektable[seek.SampleNumber]=seek;
        	count++;
        	}
	  if (res)
	     ddebug("ShnDecoder::load_external_seektable - %s - fread#%u (%u) return only %u bytes:%s\n",fname,count,r,sizeof(seek),strerror(errno));

	  ddebug("ShnDecoder::load_external_seektable - %u entries loaded!\n",count);
	  return 0;
    }

ulong ShnDecoder::seekAtSample(ulong sample) {

  // find the closest entry in seektable
  // reprogram engine
  // report resulting position (sample)

  // in the worst cae we find zero position...

  eof=0;

  if (seek.SampleNumber<=sample &&
      seektable.lower_bound(sample)->second.SampleNumber<=seek.SampleNumber)
	  return seek.SampleNumber;

  seek.SampleNumber=sample+1;
  do {
    seek= seektable.lower_bound(seek.SampleNumber-1)->second;

    buffer.toc[0][-1]=(slong)seek.CBuf_0_Minus1;
    buffer.toc[0][-2]=(slong)seek.CBuf_0_Minus2;
    buffer.toc[0][-3]=(slong)seek.CBuf_0_Minus3;
    offset.toc[0][0]=(slong)seek.Offset_0_0;
    offset.toc[0][1]=(slong)seek.Offset_0_1;
    offset.toc[0][2]=(slong)seek.Offset_0_2;
    offset.toc[0][3]=(slong)seek.Offset_0_3;

    if (nchan > 1)
    {
      buffer.toc[1][-1]=(slong)seek.CBuf_1_Minus1;
      buffer.toc[1][-2]=(slong)seek.CBuf_1_Minus2;
      buffer.toc[1][-3]=(slong)seek.CBuf_1_Minus3;
      offset.toc[1][0]=(slong)seek.Offset_1_0;
      offset.toc[1][1]=(slong)seek.Offset_1_1;
      offset.toc[1][2]=(slong)seek.Offset_1_2;
      offset.toc[1][3]=(slong)seek.Offset_1_3;
    }

    LastBufferReadPosition=seek.SHNLastBufferReadPosition;
    if (seek.SHNFilePosition<BUFSIZ) {
       ddebug("seekAt(%u->%u): wrong file offset %u ( -%u)\n",sample,seek.SampleNumber,seek.SHNFilePosition,BUFSIZ);
       continue;
       }
    if (fseek(filei,seek.SHNFilePosition-BUFSIZ,SEEK_SET)) {
       ddebug("seekAt(%u->%u): seek at %u failed: %s\n",sample,seek.SampleNumber,seek.SHNFilePosition,strerror(errno));
       continue;
       }
	if (fread((char*) getbuf, 1, BUFSIZ, filei)!=BUFSIZ) {
       ddebug("seekAt(%u->%u): fread at %u failed: %s\n",sample,seek.SampleNumber,seek.SHNFilePosition-BUFSIZ,strerror(errno));
       continue;
       }
    if (seek.SHNBufferOffset>BUFSIZ) {
       ddebug("seekAt(%u->%u): wrong buffer offset %u of %u\n",sample,seek.SampleNumber,seek.SHNBufferOffset,BUFSIZ);
       continue;
       }
    getbufp=getbuf+	   seek.SHNBufferOffset;
    nbitget = (ushort) seek.SHNBitPosition;
    gbuffer = (ulong)  seek.SHNGBuffer    ;
    nbyteget= (ushort) seek.SHNByteGet    ;
    chan=0;
    WriteCount=0;
    blocksize=reblocksize;
    // checks passed!
    return seek.SampleNumber;

  } while (seek.SampleNumber>0);

  // all seek elements wrong!
  // perform full reset
  ddebug("seekAt(%u->%u): all dead!\n",sample,seek.SampleNumber);
  char* fn=filenamei;
  filenamei=NULL;
  close();
  init(fn);
  delete[] fn;

  return seek.SampleNumber;
}

int ShnDecoder::proceed()
{
  try
    {
    /***********************/
    /* EXTRACT starts here */
    /***********************/

    int i, cmd;

//    while(1)
    do {
      ddebug("shorten:proceed - circle enter!\n");

      ReadingFunctionCode=TRUE;
      cmd = uvar_get(FNSIZE);
      ddebug("shorten:proceed - new cmd: %u\n",cmd);

      seek.CBuf_0_Minus1=(slong)buffer.toc[0][-1];
      seek.CBuf_0_Minus2=(slong)buffer.toc[0][-2];
      seek.CBuf_0_Minus3=(slong)buffer.toc[0][-3];
      seek.Offset_0_0=(slong)offset.toc[0][0];
      seek.Offset_0_1=(slong)offset.toc[0][1];
      seek.Offset_0_2=(slong)offset.toc[0][2];
      seek.Offset_0_3=(slong)offset.toc[0][3];

      if (nchan > 1)
      {
        seek.CBuf_1_Minus1=(slong)buffer.toc[1][-1];
        seek.CBuf_1_Minus2=(slong)buffer.toc[1][-2];
        seek.CBuf_1_Minus3=(slong)buffer.toc[1][-3];
        seek.Offset_1_0=(slong)offset.toc[1][0];
        seek.Offset_1_1=(slong)offset.toc[1][1];
        seek.Offset_1_2=(slong)offset.toc[1][2];
        seek.Offset_1_3=(slong)offset.toc[1][3];
      }
      else
      {
        seek.CBuf_1_Minus1=0;
        seek.CBuf_1_Minus2=0;
        seek.CBuf_1_Minus3=0;
        seek.Offset_1_0=0;
        seek.Offset_1_1=0;
        seek.Offset_1_2=0;
        seek.Offset_1_3=0;
      }

      ReadingFunctionCode=FALSE;

      if(FN_QUIT==cmd) {
        eof=1;
        break;
    	}
      else
      {
        switch(cmd)
        {
          case FN_ZERO:
          case FN_DIFF0:
          case FN_DIFF1:
          case FN_DIFF2:
          case FN_DIFF3:
          case FN_QLPC:
          {
            slong coffset, *cbuffer = buffer.toc[chan];
            int resn = 0, nlpc, j;

            if(cmd != FN_ZERO)
            {
              resn = uvar_get(ENERGYSIZE);
              /* this is a hack as version 0 differed in definition of var_get */
              if(version == 0)
                resn--;
            }

            /* find mean offset : N.B. this code duplicated */
            if(nmean == 0)
              coffset = offset.toc[chan][0];
            else
            {
              slong sum = (version < 2) ? 0 : nmean / 2;
              for(i = 0; i < nmean; i++)
                sum += offset.toc[chan][i];
              if(version < 2)
                coffset = sum / nmean;
              else
                coffset = ROUNDEDSHIFTDOWN(sum / nmean, seek.bitshift);
            }

            ddebug("ShnDecoder::proceed cmd:%u nmean:%u coffset:%u chan:%u blocksize:%u resn:%u WriteCount:%u, nwrap:%u at pos %lu\n", cmd,nmean,coffset,chan,blocksize,resn,WriteCount,nwrap,ftell(filei));
            switch(cmd)
            {
              case FN_ZERO:
                for(i = 0; i < blocksize; i++)
                  cbuffer[i] = 0;
                break;
              case FN_DIFF0:
                for(i = 0; i < blocksize; i++)
                  cbuffer[i] = var_get(resn) + coffset;
                break;
              case FN_DIFF1:
                for(i = 0; i < blocksize; i++)
                  cbuffer[i] = var_get(resn) + cbuffer[i - 1];
                break;
              case FN_DIFF2:
                for(i = 0; i < blocksize; i++)
                  cbuffer[i] = var_get(resn) + (2 * cbuffer[i - 1] - cbuffer[i - 2]);
                break;
              case FN_DIFF3:
                for(i = 0; i < blocksize; i++)
                  cbuffer[i] = var_get(resn) + 3 * (cbuffer[i - 1] -  cbuffer[i - 2]) + cbuffer[i - 3];
                break;
              case FN_QLPC:
                //ddebug("ShnDecoder::proceed - start QLPC ...\n");
                nlpc = uvar_get(LPCQSIZE);

                //ddebug("ShnDecoder::proceed - LPCQSIZE: %u (maxnlpc=%u, qlpc=%p)\n",nlpc,maxnlpc,qlpc);

                if (nlpc>maxnlpc) {
                	if (nlpc>blocksize) {
                		ddebug_ret("ShnDecoder::proceed - LPCQSIZE too big: %u (blocksize=%u) at position %lu\n",nlpc,blocksize,ftell(filei));
                		}
                	if (qlpc) delete[] qlpc;
                    qlpc = new int[maxnlpc=nlpc];
                    ddebug("ShnDecoder::proceed - qlpc (re)created!\n");
                	}

                for(i = 0; i < nlpc; i++)
                  qlpc[i] = var_get(LPCQUANT);
                for(i = 0; i < nlpc; i++)
                  cbuffer[i - nlpc] -= coffset;
                for(i = 0; i < blocksize; i++)
                {
                  slong sum = lpcqoffset;

                  for(j = 0; j < nlpc; j++)
                    sum += qlpc[j] * cbuffer[i - j - 1];
                  cbuffer[i] = var_get(resn) + (sum >> LPCQUANT);
                }
                if(coffset != 0)
                  for(i = 0; i < blocksize; i++)
                    cbuffer[i] += coffset;
                //ddebug("ShnDecoder::proceed - fin QLPC ...\n");
                break;
            }

            /* store mean value if appropriate : N.B. Duplicated code */
            if(nmean > 0)
            {
              slong sum = (version < 2) ? 0 : blocksize / 2;

              for(i = 0; i < blocksize; i++)
                sum += cbuffer[i];

              for(i = 1; i < nmean; i++)
                offset.toc[chan][i - 1] = offset.toc[chan][i];
              if(version < 2)
                offset.toc[chan][nmean - 1] = sum / blocksize;
              else
                offset.toc[chan][nmean - 1] = (sum / blocksize) << seek.bitshift;
            }

            if(chan==0)
            {
            	if (WriteCount%100==0) {
            		ddebug("ShnDecoder::proceed - adding seek entry at %u ...",seek.SampleNumber);
            	    seektable[seek.SampleNumber]=seek;
                    ddebug(" ok\n");
            		}
              WriteCount++;
            }

            /* do the wrap */
            for(i = -nwrap; i < 0; i++)
              cbuffer[i] = cbuffer[i + blocksize];

            shn_fix_bitshift(cbuffer, seek.bitshift);

            if(chan == nchan - 1)
            {
              seek.SampleNumber+=blocksize;
			  if (buffer_output())
                 ddebug_ret("output failed!");

            }
            chan = (chan + 1) % nchan;
            break;
          }

          case FN_BLOCKSIZE:
            {
            ulong saved=blocksize;
            blocksize = UINT_GET((int) (log((double) blocksize) / M_LN2));
            if (blocksize>reblocksize)
				ddebug_ret("shorten::proceed - requested blocksize to big: %u of %u at position %lu\n",blocksize,reblocksize,ftell(filei));
            recheckbuf();
            ddebug("shorten::proceed - blocksize changed %u -> %u\n",saved,blocksize);
        	}
            break;
          case FN_BITSHIFT:
            seek.bitshift = uvar_get(BITSHIFTSIZE);
            break;
          case FN_VERBATIM:
          {
            int cklen = uvar_get(VERBATIM_CKSIZE_SIZE);
            while (cklen--)
            {
              int ByteToWrite = uvar_get(VERBATIM_BYTE_SIZE);
              if (byte_output(ByteToWrite))
              	 ddebug_ret("output failed!");
              ddebug("shorten::proceed - verbatim byte output: %#02x\n",ByteToWrite);
            }
            break;
          }

          default:
            ddebug_ret("sanity check fails trying to decode function: %d\n",cmd);
        }
      }
    } while(chan);

    /* wind up */
//    fwrite_type_quit();

//    free((void *) offset);
  }
  catch (const char* p) {
    ddebug("shorten:proceed - exception catched: '%s', exiting...\n",p);
    return -1;
	}

  ddebug("shorten:proceed - (normal) leave!\n");
  return 0;
}

#define CAPMAXSHORT(x)  ((x > 32767) ? 32767 : x)

/* convert from signed ints to a given type and write */
int ShnDecoder::shnwrite(int nitem) {
  int nwrite;
  int towrite = TYPE_S16LH_SIZE * nchan * nitem;
  slong *data0 = buffer.toc[0];

  {
    sshort *writebufp = (sshort*) writebuf;
    if(nchan == 1)
      for(int i = 0; i < nitem; i++)
        *writebufp++ = CAPMAXSHORT(data0[i]);
    else
      for(int i = 0; i < nitem; i++)
        for(int chan = 0; chan < nchan; chan++)
          *writebufp++ = CAPMAXSHORT(buffer.toc[chan][i]);
	}

#ifdef WORDS_BIGENDIAN
      swab(writebuf, writefub, (size_t)(towrite));
      nwrite = waveoutput(writefub, towrite);
#else
      nwrite = waveoutput(writebuf, towrite);
#endif

  if(nwrite) {
    ddebug("failed to write decompressed stream\n");
    throw("failed to write decompressed stream");
	}

  return 0;
}


void ShnDecoder::shn_fix_bitshift(slong *buffer, int bitshift) {
  if(internal_ftype == TYPE_AU1)
    for(int i = 0; i < blocksize; i++)
      buffer[i] = ulaw_outward[bitshift][buffer[i] + 128];
  else if(internal_ftype == TYPE_AU2)
    for(int i = 0; i < blocksize; i++) {
      if(buffer[i] >= 0)
        buffer[i] = ulaw_outward[bitshift][buffer[i] + 128];
      else if(buffer[i] == -1)
        buffer[i] =  NEGATIVE_ULAW_ZERO;
      else
        buffer[i] = ulaw_outward[bitshift][buffer[i] + 129];
    }
  else
    if(bitshift != 0)
      for(int i = 0; i < blocksize; i++)
        buffer[i] <<= bitshift;
}

