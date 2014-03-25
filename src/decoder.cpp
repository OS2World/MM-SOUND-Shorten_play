/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Shorten player plug-in for PM123 */

#define INCL_DOS
#define INCL_PM

#include <os2.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <glob.h>
#include <sys/time.h>

#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <locale.h>

#include "decoder.h"

void decoder_thread(void *arg) {
   ((Decoder *) arg)->decoder_thread();
}

int _System decoder_init(void **W) {
   debug("init called!\n");
   Decoder *w=new Decoder;
   w->decodertid = _beginthread(decoder_thread,0,64*1024,(void *) w);

   if(w->decodertid < 1) {
      delete w;
      return -1;
   	  }

   *W=w;
   return w->decodertid;
}

BOOL _System decoder_uninit(void *W) {
   debug("uninit called!\n");
   Decoder *w = (Decoder *) W;
   int decodertid = w->decodertid;
   BOOL rc=!DosKillThread(decodertid);
   delete w;
   return rc;
}

ULONG _System decoder_command(void *W, ULONG msg, DECODER_PARAMS *params) {
   debug("command %lu called!\n",msg);
   return ((Decoder *) W)->decoder_command(msg,params);
   }

ULONG _System decoder_length(void *W)
{
   ulong r=((Decoder *)W)->decoder_length();
   debug("decoder_length called = %u!\n",r);
   return r;
}

ULONG _System decoder_status(void *W)
{
//   debug("decoder_status called (status=%d)!\n",((Decoder *)W)->decoder_status());
   return ((Decoder *)W)->decoder_status();
}

ULONG _System decoder_trackinfo(char *drive, int track, DECODER_INFO *info)
{
   return 200;
}

ULONG _System decoder_cdinfo(char *drive, DECODER_CDINFO *info)
{
   return 100;
}


ULONG _System decoder_support(char *ext[], int *size)
{
   debug("decoder_support %d called!\n",*size);
   if(size)
   {
      if(ext != NULL)
      {
         if (*size >= 1) strcpy(ext[0],"*.shn");
      }
      *size = 1;
   }
   return DECODER_FILENAME;
}

void _System plugin_query(PLUGIN_QUERYPARAM *param)
{
   debug("plugin_query called!\n");
   param->type = PLUGIN_DECODER;
   param->author = "ntim";
   param->desc = PRODNAME " " PRODVERSION "-" PRODBUILD;
   param->configurable = FALSE;
}


ULONG _System decoder_fileinfo(char *filename, DECODER_INFO *info)
{
   debug("decoder_fileinfo %s called!\n",filename);

   int ssize=info->size;
   memset(info,0,ssize);
   info->size = ssize;

   int vers_1_32= info->size >= INFO_SIZE_2; // detect pm123 version

   if (vers_1_32)
      info->haveinfo = 0;
   debug("pm123 version %s 1.32 detected!\n",vers_1_32?">=":"<");

//   memset(info,0,sizeof(*info));
//   info->size = sizeof(*info);
//   info->mpeg = 0;

   //info->numchannels = 0;

    Decoder w;
    char sbuf[32];

    int r=w.decoder_open(filename);
    r=r?r:w.decoder_init();

    if (r==0) {
    	{
      	struct info_item {
        	char* name;
        	char* field;
        	int flag;
        	int size;
        //char* name;
         } cp_infos[]={
            {info->title    ,w.title    ,DECODER_HAVE_TITLE     ,128},
            {info->artist   ,w.artist   ,DECODER_HAVE_ARTIST    ,128},
            {info->album    ,w.album    ,DECODER_HAVE_ALBUM     ,128},
            {info->year     ,w.year     ,DECODER_HAVE_YEAR      ,128},
            {info->comment  ,w.comment  ,DECODER_HAVE_COMMENT   ,128},
            {info->genre    ,w.genre    ,DECODER_HAVE_GENRE     ,128},
            {info->track    ,w.track    ,DECODER_HAVE_TRACK     ,128},
            {info->copyright,w.copyright,DECODER_HAVE_COPYRIGHT ,128}
            };

        unsigned int infofields=vers_1_32?sizeof(cp_infos)/sizeof(cp_infos[0]):6;

    	for (unsigned int i=0; i<infofields; i++) {
    		if (cp_infos[i].field) {
    			strncpy(cp_infos[i].name,cp_infos[i].field,cp_infos[i].size);
    			cp_infos[i].name[cp_infos[i].size-1]='\0';
                if (vers_1_32)
                	info->haveinfo |= cp_infos[i].flag;
            	}
    		}
    	}

        if (vers_1_32) {
           struct info_float_item {
               float* name;
               char* field;
               int flag;
            	} ft_infos[]={
               {&info->track_gain,  w.REPLAYGAIN_TRACK_GAIN, DECODER_HAVE_TRACK_GAIN},
               {&info->track_peak,  w.REPLAYGAIN_TRACK_PEAK, DECODER_HAVE_TRACK_PEAK},
               {&info->album_gain,  w.REPLAYGAIN_ALBUM_GAIN, DECODER_HAVE_ALBUM_GAIN},
               {&info->album_peak,  w.REPLAYGAIN_ALBUM_PEAK, DECODER_HAVE_ALBUM_PEAK}
               };
            char* save_locale=strnewdup(setlocale(LC_NUMERIC,NULL));
            if (save_locale)
            	setlocale(LC_NUMERIC,"C");
            else
            	debug("save locale failed!\n");
        	for (unsigned int i=0; i<sizeof(ft_infos)/sizeof(ft_infos[0]); i++) {
            	if (ft_infos[i].field) {
                	*ft_infos[i].name=atof(ft_infos[i].field);
                	debug("replay/gain specified: %g (%s)\n",*ft_infos[i].name,ft_infos[i].field);
                    info->haveinfo |= ft_infos[i].flag;
                	}
            	}
    		if (save_locale) {
                setlocale(LC_NUMERIC,save_locale);
                delete save_locale;
            	}
            }


        info->songlength=w.decoder_filelength();
	    debug("reported filelength = %u! (%lu) \n",info->songlength,w.decoder_filelength());
        info->format.size=sizeof(FORMAT_INFO);
        info->format.bits=w.format.bits;
        info->format.channels=w.format.channels;
        info->format.samplerate=w.format.samplerate;
        info->format.format=w.format.format;

        snprintf( info->tech_info, sizeof(info->tech_info), "Shorten %d bits, %.1f kHz, %s",
            info->format.bits,(float) info->format.samplerate / 1000.0f,
            info->format.channels == 1 ? "Mono" : info->format.channels == 2 ? "Stereo" : _ltoa(info->format.channels,sbuf,10) );


        {

        FILE* fp=fopen(filename,"rb");
        if (fp!=NULL) {

   			TagInfo_t taginfo;


            struct info_item {
                 char* s;
                 int size;
                 int flag;
                 char* from; } song_infos[]={
                     { info->title,    128, DECODER_HAVE_TITLE     ,/* "title", */ taginfo.Title},
                     { info->artist,   128, DECODER_HAVE_ARTIST    ,/* "artist", */ taginfo.Artist},
                     { info->album,    128, DECODER_HAVE_ALBUM     ,/* "album", */ taginfo.Album},
                     { info->year,     128, DECODER_HAVE_YEAR      ,/* "year", */ taginfo.Year},
                     { info->comment,  128, DECODER_HAVE_COMMENT   ,/* "comment", */ taginfo.Comment},
             		 { info->genre,    128, DECODER_HAVE_GENRE     ,/* "genre", */ taginfo.Genre},
            		 { info->track,    128, DECODER_HAVE_TRACK     ,/* "track", */ taginfo.Track}
            		 //,{ info->copyright 128, DECODER_HAVE_COPYRIGHT ,taginfo.Copyright}
					 //{ info->tech_info, 128, "CUESHEET", taginfo.}
             		  };
            unsigned limit=vers_1_32?sizeof(song_infos)/sizeof(info_item):6;


        	if (Read_APE_Tags ( fp, &taginfo ))
            	for (unsigned int i=0; i<limit; i++)
                	if ( song_infos[i].s[0] == '\0') {
                		strncpy(song_infos[i].s,song_infos[i].from,song_infos[i].size-1);
                		song_infos[i].s[song_infos[i].size-1]='\0';
                		}

            if (Read_ID3V1_Tags ( fp, &taginfo ))
                for (unsigned int i=0; i<limit; i++)
                    if ( song_infos[i].s[0] == '\0') {
                        strncpy(song_infos[i].s,song_infos[i].from,song_infos[i].size-1);
                        song_infos[i].s[song_infos[i].size-1]='\0';
                        }

            if (vers_1_32)
            	info->filesize |= filelength(fileno(fp));

            fclose(fp);
            }
        }

		/*
        if (info->title && info->title[0])
            strcpy(info->comment,info->title);
        else
            {
            char* s=strchr(filename,'/');
            strncpy(info->comment,s&&*s?s+1:filename,128);
            info->comment[127]='\0';
            }
        */
    }
    else {
    	debug("open %s failed: %d\n",filename,r);
        }

    return r;
}


