/*
This is part of the audio CD player library
Copyright (C)1998-99 Tony Arcieri <bascule@inferno.tusculum.edu>
Copyright (C)2001 Dustin Graves <dgraves@computer.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

/* XXX This file is NOT strtok() clean */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cdaudio2.h>
#include <config.h>
#include <data.h>

static int data_process_control_codes(char *outbuffer,const char *inbuffer,int len);
static int data_process_block(char *outbuffer,int len,const char (*block)[80],int bounds);
static int data_format_line(char *outbuffer,const char *inbuffer,int len);
static int data_format_block(char (*block)[80],int arraylen,const char *inbuffer);

#define __LIBCDAUDIO_INTERNAL
int parse_track_artist=1;

/* Process formatting control codes - OK */
static int data_process_control_codes(char *outbuffer,const char *inbuffer,int len)
{
  int index;
  int outbufferindex=0;

  for(index=0;outbufferindex<len;index++)
  {
    if(inbuffer[index]=='\\')
    {
      switch(inbuffer[++index])
      {
        case 'n':
	  outbuffer[outbufferindex++]='\n';
          break;
        case 't':
          outbuffer[outbufferindex++]='\t';
          break;
        case '\\':
          outbuffer[outbufferindex++]='\\';
          break;
      }
    }
    else if(inbuffer[index]=='\0')
    {
      outbuffer[outbufferindex]='\0';
      return 0;
    }
    else
      outbuffer[outbufferindex++]=inbuffer[index];
  }

  return 0;
}

/* Format a block of CDDB data */
static int data_process_block(char *outbuffer,int len,const char (*block)[80],int bounds)
{
  int index, copyindex;
  int outbufferindex=0;
  char procbuffer[256];

  memset(outbuffer,'\0',len);

  for(index=0;index<bounds;index++)
  {
    data_process_control_codes(procbuffer,block[index],256);
    for(copyindex=0;copyindex<256;copyindex++)
    {
      if(procbuffer[copyindex]=='\0')
	break;
      else
      {
        outbuffer[outbufferindex++]=procbuffer[copyindex];
        if(outbufferindex>=len)
        {
          outbuffer[len-1]='\0';
          return 0;
	}
      }
    }

    outbuffer[outbufferindex]='\0';
  }

  return 0;
}

/* Format a line for CDDB cache output */
static int data_format_line(char *outbuffer,const char *inbuffer,int len)
{
  int index;
  int outbufferindex=0;

  for(index=0;(outbufferindex<len)&&(index<64);index++)
  {
    switch(inbuffer[index])
    {
      case '\t':
        outbuffer[outbufferindex++]='\\';
        outbuffer[outbufferindex++]='t';
        break;
      case '\n':
        outbuffer[outbufferindex++]='\\';
        outbuffer[outbufferindex++]='n';
        break;
      case '\0':
        outbuffer[outbufferindex]='\0';
        return 0;
      default:
        outbuffer[outbufferindex++]=inbuffer[index];
        break;
    }
  }

  outbuffer[outbufferindex]='\0';
  return 0;
}

static int data_format_block(char (*block)[80],int arraylen,const char *inbuffer)
{
  int index;
  char procbuffer[80];

  for(index=0;index<arraylen;index++)
  {
    strncpy(procbuffer,inbuffer,64);
    data_format_line(block[index],procbuffer,80);
    inbuffer+=64;
  }

  return 0;
}

int data_format_input(struct disc_data *outdata,const struct __unprocessed_disc_data *indata,int tracks)
{
  int index;
  char trackbuffer[256],procbuffer[EXTENDED_DATA_SIZE];

  outdata->data_id=indata->data_id;
  strncpy(outdata->data_cdindex_id,indata->data_cdindex_id,CDINDEX_ID_SIZE);
  outdata->data_revision=indata->data_revision;
  data_process_block(procbuffer,EXTENDED_DATA_SIZE,indata->data_title,indata->data_title_index);
  memset(outdata->data_artist,'\0',256);
  memset(outdata->data_title,'\0',256);
  if(strstr(procbuffer," / ")!=NULL)
  {
    index=0;
    while(strncmp(procbuffer+index," / ",3)!=0)
      index++;
    strncpy(outdata->data_artist,procbuffer,index);
    strncpy(outdata->data_title,(char *)procbuffer+index+3,256);
  }
  else
  {
    strncpy(outdata->data_artist,"",256);
    strncpy(outdata->data_title,procbuffer,256);
  }
  data_process_block(outdata->data_extended,EXTENDED_DATA_SIZE,indata->data_extended,indata->data_extended_index);
  outdata->data_genre=indata->data_genre;

  for(index=0;index<tracks;index++)
  {
    memset(trackbuffer,'\0',256);
    data_process_block(trackbuffer,256,indata->data_track[index].track_name,indata->data_track[index].track_name_index);
    if(strchr(trackbuffer,'/')!=NULL&&parse_track_artist)
    {
      strtok(trackbuffer,"/");
      strncpy(outdata->data_track[index].track_artist,trackbuffer,strlen(trackbuffer)-1);
      strncpy(outdata->data_track[index].track_name,(char *)strtok(NULL, "/")+1,256);
    }
    else
    {
      strncpy(outdata->data_track[index].track_artist,"",256);
      strncpy(outdata->data_track[index].track_name,trackbuffer,256);
    }

    data_process_block(outdata->data_track[index].track_extended,EXTENDED_DATA_SIZE,indata->data_track[index].track_extended,indata->data_track[index].track_extended_index);
  }

  return 0;
}

int data_format_output(struct __unprocessed_disc_data *outdata,const struct disc_data *indata,int tracks)
{
  int index;
  char trackbuffer[256],procbuffer[EXTENDED_DATA_SIZE];

  outdata->data_id=indata->data_id;
  strncpy(outdata->data_cdindex_id,indata->data_cdindex_id,CDINDEX_ID_SIZE);
  outdata->data_revision=indata->data_revision;
  outdata->data_genre=indata->data_genre;

  memset(procbuffer,'\0',EXTENDED_DATA_SIZE);
  if(strlen(indata->data_artist))
    snprintf(procbuffer,EXTENDED_DATA_SIZE,"%s / %s",indata->data_artist,indata->data_title);
  else
    strncpy(procbuffer,indata->data_title,EXTENDED_DATA_SIZE);
  data_format_block(outdata->data_title,MAX_EXTEMPORANEOUS_LINES,procbuffer);
  for(outdata->data_title_index=0;outdata->data_title_index<MAX_EXTEMPORANEOUS_LINES;outdata->data_title_index++)
  {
    if(strlen(outdata->data_title[outdata->data_title_index])<=0)
      break;
  }

  data_format_block(outdata->data_extended,MAX_EXTENDED_LINES,indata->data_extended);
  for(outdata->data_extended_index=0;outdata->data_extended_index<MAX_EXTENDED_LINES;outdata->data_extended_index++)
  {
    if(strlen(outdata->data_extended[outdata->data_extended_index])<=0)
      break;
  }

  for(index=0;index<tracks;index++)
  {
    memset(trackbuffer,'\0',256);
    if(strlen(indata->data_track[index].track_artist)>0)
      snprintf(trackbuffer,256,"%s / %s",indata->data_track[index].track_artist,indata->data_track[index].track_name);
    else
      strncpy(trackbuffer,indata->data_track[index].track_name,256);
    data_format_block(outdata->data_track[index].track_name,MAX_EXTEMPORANEOUS_LINES,trackbuffer);
    for(outdata->data_track[index].track_name_index=0;outdata->data_track[index].track_name_index<MAX_EXTEMPORANEOUS_LINES;outdata->data_track[index].track_name_index++)
    {
      if(strlen(outdata->data_track[index].track_name[outdata->data_track[index].track_name_index])<=0)
        break;
    }

    data_format_block(outdata->data_track[index].track_extended,MAX_EXTENDED_LINES,indata->data_track[index].track_extended);
    for(outdata->data_track[index].track_extended_index=0;outdata->data_track[index].track_extended_index<MAX_EXTENDED_LINES;outdata->data_track[index].track_extended_index++)
    {
      if(strlen(outdata->data_track[index].track_extended[outdata->data_track[index].track_extended_index])<=0)
	break;
    }
  }

  return 0;
}
