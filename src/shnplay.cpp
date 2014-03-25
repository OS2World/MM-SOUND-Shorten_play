#include <os2.h>

#include "decoder.h"

int Decoder::byte_output(int byte) {
	if (riff_set<sizeof(riff.data))
		riff.data[riff_set++]=(char)byte;
#ifdef DEBUG
    if (fpraw) fwrite(buffer,bufused,1,fpraw);
#endif
	return 0;
}

int Decoder::waveoutput(const char* buf, unsigned int size) {
    ddebug("waveoutput callback: buf=%p, size=%d...\n",buf,size);

    if (skipsamples>0) {
        ddebug("waveoutput callback: skipsamples=%d...\n",skipsamples);
    	ulong add=ulong(skipsamples)<(size/blockalign)?ulong(skipsamples):(size/blockalign);
    	filepos+=add;
    	skipsamples-=add;
    	buf+=add*blockalign;
        size-=add*blockalign;
        if (skipsamples==0 && needReport) {
           reportSeek();
    	   }
    	}

    while (size>0) {
        unsigned char* start=(unsigned char* )buffer+bufused;
        unsigned limit = size<ulong(audio_buffersize-bufused)?size:(audio_buffersize-bufused);
        memcpy(start,buf,limit);
        buf+=limit;
        size-=limit;
        if ((bufused+=limit)==audio_buffersize) {
		   filepos += bufused/blockalign;
           int written = output_play_samples(a, &format, buffer, bufused, decoder_filepos());
           if ( ((long)filepos-(long)saved_filepos) > (long)sample_rate
             || ((long)filepos-(long)saved_filepos) < -(long)sample_rate )
			{
           		unsigned long real_filepos=shorten_filepos();
           		update_vbr((unsigned long)((double(real_filepos)-saved_real_filepos)*sample_rate/((double)filepos-saved_filepos)/1024*8));
           		saved_filepos=filepos;
           		saved_real_filepos=real_filepos;
           		}
           lastplayed += written;
#ifdef DEBUG
           if (fpraw) fwrite(buffer,bufused,1,fpraw);
#endif
           ddebug("write_callback play: %d bytes (written %d)\n",bufused,written);
                    if (written < bufused) {
                        WinPostMsg(hwnd,WM_PLAYERROR,0,0);
                        ddebug("ERROR: write error\n");
                        return -1;
                        }
            bufused=0;
            }
        }
    ddebug("waveoutput callback: leave (bufused=%d)...\n",bufused);
    return 0;
}

