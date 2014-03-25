#ifndef __Decoder_h_
#define __Decoder_h_

#include "pm123/format.h"
#include "pm123/decoder_plug.h"
#include "pm123/plugin.h"
#include "src/shorten.hpp"
#include <io.h>

//#define NO_WAV_FILE 200

/* definitions for id3tag.cpp */
#ifndef FILE_T
#define FILE_T FILE*
#endif
#ifndef OFF_T
#define OFF_T signed long
#endif
typedef signed int Int;


typedef struct {
    OFF_T         FileSize;
    Int           GenreNo;
    Int           TrackNo;
    char          Genre   [128];
    char          Year    [ 20];
    char          Track   [  8];
    char          Title   [256];
    char          Artist  [256];
    char          Album   [256];
    char          Comment [512];
    } TagInfo_t;


#pragma pack(1)
typedef struct _RIFF_HEADER
{
  char           id_riff[4]; /* RIFF */
  unsigned long  len;
  char           id_wave[4]; /* WAVE */

} RIFF_HEADER;

typedef struct _CHNK_HEADER
{
  char           id[4];
  unsigned long  len;

} CHNK_HEADER;

typedef struct _FORMAT
{
  unsigned short FormatTag;
  unsigned short Channels;
  unsigned long  SamplesPerSec;
  unsigned long  AvgBytesPerSec;
  unsigned short BlockAlign;
  unsigned short BitsPerSample;

} FORMAT;

typedef struct WAVE_HEADER
{
  RIFF_HEADER  riff;
  CHNK_HEADER  format_header;
  FORMAT       format;
  CHNK_HEADER  data_header;
} WAVE_HEADER;

typedef union {
	WAVE_HEADER header;
	char        data[sizeof(WAVE_HEADER)];
} HEADER_UNION;
#pragma pack()
/* */

#ifndef STRNEWDUP_DEFINED
inline char* strnewdup(const char* str) {
    return str?strcpy(new char[strlen(str)+1],str):NULL;
    }
#define STRNEWDUP_DEFINED
#endif

void decoder_thread(void *arg);


#ifndef DEBUG_DEFINED
#define DEBUG_DEFINED
#if defined(DEBUG)
int debug(const char* str, ...) __attribute__ ((format (printf, 1, 2)));
#if DEBUG>1
#define ddebug debug
#else
inline int ddebug(const char* str, ...) { return -1; }
#endif
#else
inline int ddebug(const char* str, ...) { return -1; }
inline int  debug(const char* str, ...) { return -1; }
#endif
#endif


Int Read_APE_Tags ( FILE_T fp, TagInfo_t* tip );
Int Read_ID3V1_Tags ( FILE_T fp, TagInfo_t* tip );

class Decoder: public ShnDecoder, public DECODER_PARAMS {
protected:
	virtual int waveoutput(const char* buf,unsigned int size);
	virtual int byte_output(int byte);

	FORMAT_INFO format;
    HEV play,ok;
    int decodertid;
    int status;
    int stop;
	unsigned long filepos; // in samples!
	char* buffer;
	int bufused;
    const char* file2open;

	ulong total_samples;
	unsigned sample_rate;
	unsigned channels;
	unsigned bps;
	long last_length;
	int blockalign;

						// parameters for forward/rewind:
    unsigned long samples2play;	// amount to play (better is factor of audio_buffersize)
	unsigned long samples2skip;	// amount to skip

	unsigned long skipsamples;  // samples to skip - after inaccurate jump
   	unsigned long lastplayed;   // samples played (ajter last jump/skip)

	unsigned long saved_vbr; 	// saved vbr level
								// for vbr calculation
	unsigned long saved_filepos;
    unsigned long saved_real_filepos;

//(debug)	FILE* fpraw;

   /* general technical information string */
   char  *tech_info;
   /* meta information */
   char  *title  ;
   char  *artist ;
   char  *album  ;
   char  *year   ;
   char  *comment;
   char  *genre  ;

   /* added since PM123 1.32 */
   char  *track    ;
   char  *copyright;

   char  *REPLAYGAIN_REFERENCE_LOUDNESS;
   char  *REPLAYGAIN_TRACK_GAIN;
   char  *REPLAYGAIN_TRACK_PEAK;
   char  *REPLAYGAIN_ALBUM_GAIN;
   char  *REPLAYGAIN_ALBUM_PEAK;

#ifdef DEBUG
   FILE* fpraw;
#endif

   HEADER_UNION riff;
   unsigned int riff_set;

   int needReport;
   unsigned long filesize;
public:
    Decoder();
    virtual ~Decoder();

	void decoder_thread();
	ULONG decoder_length();
	ULONG decoder_command(ULONG msg, DECODER_PARAMS *params);
    int decoder_status() { return status; }

protected:
      int decoder_open(const char *filename,int loadseektable=0) {
        return init(filename,loadseektable);
        }

      int decoder_init();

	  ULONG decoder_filepos() {
	    	return ULONG(double(filepos)*1000/sample_rate);
	    }
      int decoder_close() {
      	if (buffer) {
      		delete buffer;
      		buffer = NULL;
      		}
      	#ifdef DEBUG
      	if (fpraw) {
      	    fclose(fpraw);
      	    fpraw=NULL;
      		}
        #endif
      	return close();
      	}

	  int flush() {
	    filepos+=bufused/blockalign;
	    return ((bufused=0));
		}
      int reportSeek() {
        WinPostMsg(hwnd,WM_SEEKSTOP,0,0);
        return 0;
        }

      int decoder_jumpto_sample(unsigned long sample, int report=0) {
            filepos=seekAtSample(sample);
            debug("decoder_jumpto: need %ld, seekAtSample report %ld\n",sample,filepos);

            skipsamples=sample-filepos;
            lastplayed=0;
            if (skipsamples<=0 ) {
               if (report) reportSeek();
               skipsamples=0;
    		   saved_filepos=0;
    		   saved_real_filepos=0;
               needReport=0;
        	   }
            else {
            	needReport=report;
            	flush();
            	}
        	return 0;
        	}
      int decoder_jumpto(long offset, int report=0) {
            debug("decoder_jumpto: %ld\n",offset);
            offset=offset<0?0:offset;
            unsigned long sample=(unsigned long)(double(offset)*sample_rate/1000);
            return decoder_jumpto_sample(
            	sample>total_samples?total_samples:sample,report);
            }

      //int flac_skip(long ms);
      ULONG decoder_filelength() {
      		ULONG r=(total_samples&&sample_rate)?ULONG(double(total_samples)*1000/sample_rate):((ULONG)-1);
      		//debug("decoder_filelength: %lu = %lu * %d (= %lu) / %d \n",r,ULONG(total_samples),1000,ULONG(total_samples)*1000,sample_rate);
      		return r;
      		}

      unsigned long decoder_vbr() {
        return (total_samples&&sample_rate&&filesize)?ULONG(double(filesize)/total_samples*sample_rate/1024*8):0;
        }
      void update_vbr(unsigned long vbr) {
         if (vbr!=saved_vbr) {
            WinPostMsg(hwnd,WM_CHANGEBR,MPFROMLONG(saved_vbr=vbr),0);
            }
    	 }

friend ULONG _System decoder_fileinfo(char *filename, DECODER_INFO *info);
friend int _System decoder_init(void **W);
friend BOOL _System decoder_uninit(void *W);

};


#endif // defined __Decoder_h_


