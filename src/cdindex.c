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

Based upon: 
NIST Secure Hash Algorithm 
heavily modified by Uwe Hollerbach <uh@alumni.caltech.edu>
from Peter C. Gutmann's implementation as found in
Applied Cryptography by Bruce Schneier
*/

/* XXX This file is NOT strtok() clean */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifndef WIN32
#include <sys/socket.h>
#include <unistd.h>
#endif

#define __LIBCDAUDIO_INTERNAL

#include "cdlyte.h"
#include "data.h"
#include "config.h"

extern char cddb_message[256];

/* cdindex-tweaked base64 encoder */
int cdindex_encode64(unsigned char *outbuffer,unsigned char *inbuffer,int inlen,int outlen);

#define SHA_BLOCKSIZE 64
#define SHA_VERSION 1

#define f1(x,y,z)       ((x&y)|(~x&z))
#define f2(x,y,z)       (x^y^z)
#define f3(x,y,z)       ((x&y)|(x&z)|(y&z))
#define f4(x,y,z)       (x^y^z)

#define CONST1          0x5a827999L
#define CONST2          0x6ed9eba1L
#define CONST3          0x8f1bbcdcL
#define CONST4          0xca62c1d6L

#define T32(x)  ((x)&0xffffffffL)
#define R32(x,n)        T32(((x<<n)|(x>>(32-n))))

#define FA(n)   \
    	T = T32(R32(A,5) + f##n(B,C,D) + E + *WP++ + CONST##n); B = R32(B,30)

#define FB(n)   \
    	E = T32(R32(T,5) + f##n(A,B,C) + D + *WP++ + CONST##n); A = R32(A,30)

#define FC(n)   \
	D = T32(R32(E,5) + f##n(T,A,B) + C + *WP++ + CONST##n); T = R32(T,30)

#define FD(n)   \
	C = T32(R32(D,5) + f##n(E,T,A) + B + *WP++ + CONST##n); E = R32(E,30)

#define FE(n)   \
	B = T32(R32(C,5) + f##n(D,E,T) + A + *WP++ + CONST##n); D = R32(D,30)

#define FT(n)   \
	A = T32(R32(B,5) + f##n(C,D,E) + T + *WP++ + CONST##n); C = R32(C,30)
                    
struct sha_data {
  unsigned long digest[5];
  unsigned long count_lo,count_hi;
  unsigned char data[SHA_BLOCKSIZE];
  int local;
};

static void sha_init(struct sha_data *sha)
{
  sha->digest[0]=0x67452301L;
  sha->digest[1]=0xefcdab89L;
  sha->digest[2]=0x98badcfeL;
  sha->digest[3]=0x10325476L;
  sha->digest[4]=0xc3d2e1f0L;
  sha->count_lo=0L;
  sha->count_hi=0L;
  sha->local=0;
}

static void sha_transform(struct sha_data *sha)
{
  int i;
  unsigned char *dp;
  unsigned long T,A,B,C,D,E,W[80],*WP;

  dp=sha->data;

#if SIZEOF_LONG == 4
#ifndef WORDS_BIGENDIAN
#define SWAP_DONE
  for(i=0;i<16;++i)
  {
    T=*((unsigned long *)dp);
    dp+=4;
    W[i]=((T<<24)&0xff000000)|((T<<8)&0x00ff0000)|
         ((T>>8)&0x0000ff00)|((T>>24)&0x000000ff);
  }
#endif

#ifdef WORDS_BIGENDIAN
#define SWAP_DONE
  for(i=0;i<16;++i)
  {
    T=*((unsigned long *)dp);
    dp+=4;
    W[i]=T32(T);
  }
#endif
#endif

#if SIZEOF_LONG == 8
#ifndef WORDS_BIGENDIAN
#define SWAP_DONE 
  for(i=0;i<16;i+=2)
  {
    T=*((unsigned long *)dp);  
    dp+=8;
    W[i]=((T<<24)&0xff000000)|((T<<8)&0x00ff0000)|
         ((T>>8)&0x0000ff00)|((T>>24)&0x000000ff);
    T>>=32;
    W[i+1]=((T<<24)&0xff000000)|((T<<8)&0x00ff0000)|
           ((T>>8)&0x0000ff00)|((T>>24)&0x000000ff);
  }
#endif

#ifdef WORDS_BIGENDIAN
#define SWAP_DONE
  for(i=0;i<16;i+=2)
  {
    T=*((unsigned long *)dp);  
    dp+=8;
    W[i]=T32(T>>32);
    W[i+1]=T32(T);
  }
#endif
#endif

  for(i=16;i<80;++i)
  {
    W[i]=W[i-3]^W[i-8]^W[i-14]^W[i-16];
#if (SHA_VERSION == 1)
    W[i]=R32(W[i],1);
#endif
  }
  A=sha->digest[0];
  B=sha->digest[1];
  C=sha->digest[2];
  D=sha->digest[3];
  E=sha->digest[4];
  WP=W;

  FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1); FC(1); FD(1);
  FE(1); FT(1); FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1);
  FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2); FE(2); FT(2);
  FA(2); FB(2); FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2);
  FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3); FA(3); FB(3);
  FC(3); FD(3); FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3);
  FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4); FC(4); FD(4);
  FE(4); FT(4); FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4);
  sha->digest[0]=T32(sha->digest[0]+E);
  sha->digest[1]=T32(sha->digest[1]+T);
  sha->digest[2]=T32(sha->digest[2]+A);
  sha->digest[3]=T32(sha->digest[3]+B);
  sha->digest[4]=T32(sha->digest[4]+C);
}

static void sha_update(struct sha_data *sha, unsigned char *buffer, int count)
{
  int i;   
  unsigned long clo;
   
  clo=T32(sha->count_lo+((unsigned long)count<<3));
  if(clo<sha->count_lo)
    ++sha->count_hi;
  sha->count_lo=clo;
  sha->count_hi+=(unsigned long)count>>29;
  if(sha->local)
  {
    i=SHA_BLOCKSIZE-sha->local;
    if(i>count)
      i=count;
    memcpy(((unsigned char *)sha->data)+sha->local,buffer,i);
    count-=i;
    buffer+=i;
    sha->local+=i;
    if(sha->local==SHA_BLOCKSIZE)
      sha_transform(sha);
    else return;
  }

  while(count>=SHA_BLOCKSIZE)
  {
    memcpy(sha->data,buffer,SHA_BLOCKSIZE);
    buffer+=SHA_BLOCKSIZE;
    count-=SHA_BLOCKSIZE;
    sha_transform(sha);
  }

  memcpy(sha->data,buffer,count);
  sha->local=count;
}

static void sha_final(unsigned char digest[20],struct sha_data *sha)
{ 
  int count;
  unsigned long lo_bit_count,hi_bit_count;

  lo_bit_count=sha->count_lo;
  hi_bit_count=sha->count_hi; 
  count=(int)((lo_bit_count>>3)&0x3f);
  ((unsigned char *)sha->data)[count++]=0x80;
  if(count>SHA_BLOCKSIZE-8)
  {
    memset(((unsigned char *)sha->data)+count,0,SHA_BLOCKSIZE-count);
    sha_transform(sha);
    memset((unsigned char *)sha->data,0,SHA_BLOCKSIZE-8);
  }
  else
    memset(((unsigned char *)sha->data)+count,0,SHA_BLOCKSIZE-8-count);

  sha->data[56]=(hi_bit_count>>24)&0xff;
  sha->data[57]=(hi_bit_count>>16)&0xff;
  sha->data[58]=(hi_bit_count>>8)&0xff;
  sha->data[59]=(hi_bit_count>>0)&0xff;
  sha->data[60]=(lo_bit_count>>24)&0xff;
  sha->data[61]=(lo_bit_count>>16)&0xff;
  sha->data[62]=(lo_bit_count>>8)&0xff;
  sha->data[63]=(lo_bit_count>>0)&0xff;
  sha_transform(sha);
  digest[0]=(unsigned char)((sha->digest[0]>>24)&0xff);
  digest[1]=(unsigned char)((sha->digest[0]>>16)&0xff);
  digest[2]=(unsigned char)((sha->digest[0]>>8)&0xff);
  digest[3]=(unsigned char)((sha->digest[0])&0xff);
  digest[4]=(unsigned char)((sha->digest[1]>>24)&0xff);
  digest[5]=(unsigned char)((sha->digest[1]>>16)&0xff);
  digest[6]=(unsigned char)((sha->digest[1]>>8)&0xff);
  digest[7]=(unsigned char)((sha->digest[1])&0xff);
  digest[8]=(unsigned char)((sha->digest[2]>>24)&0xff);
  digest[9]=(unsigned char)((sha->digest[2]>>16)&0xff);
  digest[10]=(unsigned char)((sha->digest[2]>>8)&0xff);
  digest[11]=(unsigned char)((sha->digest[2])&0xff);
  digest[12]=(unsigned char)((sha->digest[3]>>24)&0xff);
  digest[13]=(unsigned char)((sha->digest[3]>>16)&0xff);
  digest[14]=(unsigned char)((sha->digest[3]>>8)&0xff);
  digest[15]=(unsigned char)((sha->digest[3])&0xff);
  digest[16]=(unsigned char)((sha->digest[4]>>24)&0xff);
  digest[17]=(unsigned char)((sha->digest[4]>>16)&0xff);
  digest[18]=(unsigned char)((sha->digest[4]>>8)&0xff);
  digest[19]=(unsigned char)((sha->digest[4])&0xff);
}

int cdindex_direct_discid(const struct disc_info *disc,char *discid,int len)
{
  int index;
  struct sha_data sha;
  unsigned char digest[20],temp[9];

  memset(sha.data,'\0',64);
  sha_init(&sha);

  snprintf(temp,9,"%02X",disc->disc_first_track);
  sha_update(&sha,temp,strlen(temp));
  snprintf(temp,9,"%02X",disc->disc_total_tracks);
  sha_update(&sha,temp,strlen(temp));
  snprintf(temp,9,"%08X",disc->disc_track[disc->disc_total_tracks].track_lba+150);
  sha_update(&sha,temp,strlen(temp));
  for(index=0;index<99;index++)
  {
    if(index<disc->disc_total_tracks)
      snprintf(temp,9,"%08X",disc->disc_track[index].track_lba+150);
    else
      snprintf(temp,9,"%08X",0);
    sha_update(&sha,temp,strlen(temp));
  }
  sha_final(digest,&sha);
  return cdindex_encode64(discid,digest,20,len);
}

int __internal_cdindex_discid(const struct disc_info *disc,char *discid,int len)
{
  return cdindex_direct_discid(disc,discid,len);
}

int cdindex_discid(int cd_desc,char *discid,int len)
{
  struct disc_info disc;

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if(!disc.disc_present)
    return -1;

  if(__internal_cdindex_discid(&disc,discid,len)<0)
    return -1;

  return 0;
}

int cdindex_read_line(int sock,char *inbuffer,int len)
{
  int index;
  char inchar;

  for(index=0;index<len;index++)
  {
    if(read(sock,&inchar,1)<=0)
      return -1;
    if(inchar=='\n')
    {
      inbuffer[index]='\0';
      return 0;
    }
    inbuffer[index]=inchar;
  }

  return index;
}

int cdindex_connect_server(const struct cddb_host *host,struct cddb_server *proxy,char *http_string,int len)
{
  int sock;

  if(proxy==NULL)
  {
    if((sock=cddb_connect(&host->host_server))<0)
      return -1;
  }
  else
  {
    if((sock=cddb_connect(proxy))<0)
      return -1;
  }

  snprintf(http_string,len,"GET http://%s:%d/%s",host->host_server.server_name,host->host_server.server_port,host->host_addressing);

  return sock;
}

void strip_whitespace(char *outbuffer,char *inbuffer,int len)
{
  int index,outdex=0,readwhite=1;

  for(index=0;index<len;index++)
  {
    switch(inbuffer[index])
    {
      case '\n':
	outbuffer[outdex]='\0';
	return;
      case '\0':
	outbuffer[outdex]='\0';
	return;
      case ' ':
	if(readwhite==0)
        {
	  outbuffer[outdex++]=' ';
	  readwhite=1;
	}
	break;
      case '\t':
	if(readwhite==0)
        {
	  outbuffer[outdex++]=' ';
	  readwhite=1;
	}
	break;
      default:
	outbuffer[outdex++]=inbuffer[index];
	readwhite=0;
    }
  }
}
   
static void cdindex_process_line(char *line,struct disc_data *data)
{
  char *var,*value;

  if(strchr(line,':')==NULL)
    return;

  var=strtok(line,":");
  if(var==NULL)
    return;

  value=strtok(NULL,":");

  if(value==NULL)
    value="";
  else
    value+=1;

  if(strcmp(var,"Artist")==0)
    strncpy(data->data_artist,value,256);
  else if(strcmp(var,"Album")==0)
    strncpy(data->data_title, value, 256);
  else if(strcmp(var,"Tracks")==0);
  else if(strncmp(var,"Track",5)==0)
    strncpy(data->data_track[strtol((char *)var+5,NULL,10)-1].track_name,value,256);
  else if(strncmp(var,"Artist",6)==0)
    strncpy(data->data_track[strtol((char *)var+6,NULL,10)-1].track_artist,value,256);
}

int cdindex_read(int cd_desc,int sock,struct disc_data *data,char *http_string)
{
  struct cddb_host host;
  struct disc_info disc;
  char outbuffer[512];
  char inbuffer[256];
  char string[512];

  memset(data,'\0',sizeof(struct disc_data));
  if(cd_stat(cd_desc,&disc)<0)
   return -1;

  if((data->data_id=__internal_cddb_discid(&disc)<0))
    return -1;

  if(cdindex_discid(cd_desc,data->data_cdindex_id,CDINDEX_ID_SIZE)<0)
    return -1;

  data->data_revision=0;
  data->data_genre=CDDB_MISC;

  snprintf(outbuffer,512,"%s?id=%s HTTP/1.0\n\n",http_string,data->data_cdindex_id);

  write(sock,outbuffer,strlen(outbuffer));

  cdindex_read_line(sock,inbuffer,256);
  if(strncmp(inbuffer,"HTTP/1.1 200",12)==0)
  {
    cddb_skip_http_header(sock);
    cdindex_read_line(sock,inbuffer,256);
    if(strncmp("NumMatches: 0",inbuffer,13)==0)
      return -1;
    while(cdindex_read_line(sock,inbuffer,256)>=0)
      cdindex_process_line(inbuffer,data);
  }
  else if(strncmp(inbuffer,"HTTP/1.1 302",12)==0)
  {
    while(cdindex_read_line(sock,inbuffer,256)>=0)
    {
      if(strncmp(inbuffer,"Location:",9)==0)
      {
	strtok(inbuffer," ");
	cddb_process_url(&host,strtok(NULL," "));
	close(sock);
	if((sock=cdindex_connect_server(&host,NULL,string,512)) < 0)
	  return -1;
	return cdindex_read(cd_desc,sock,data,string);
      }
    }
    return -1;
  }
  else
    return -1;

  return 0;
}

int cdindex_generate_new_entry(int cd_desc,struct disc_data *data)
{  
  if(cddb_read_data(cd_desc,data)<0)
    cddb_generate_unknown_entry(cd_desc,data);

  return 0;
}

int cdindex_read_disc_data(int cd_desc,struct disc_data *data)
{
  FILE *cdindex_xml;
  int track;
  char root_dir[PATH_MAX],file[PATH_MAX],inbuffer[512],procbuffer[512];
  struct disc_info disc;
  struct stat st;
   
  if(getenv("HOME")==NULL)
  {
    strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  snprintf(root_dir,PATH_MAX,"%s/.cdindex",getenv("HOME"));

  if(stat(root_dir,&st)<0)
  {
    if(errno!=ENOENT)
      return -1;
    else
    {
      cdindex_generate_new_entry(cd_desc,data);
      return 0;
    }
  }
  else
  {
    if(!S_ISDIR(st.st_mode))
    {
      errno = ENOTDIR;
      return -1;
    }
  }

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if((data->data_id=__internal_cddb_discid(&disc))<0)
    return -1;

  if(__internal_cdindex_discid(&disc,data->data_cdindex_id,CDINDEX_ID_SIZE)<0)
    return -1;

  data->data_revision=0;
  data->data_genre=CDDB_UNKNOWN;

  snprintf(file,PATH_MAX,"%s/%s",root_dir,data->data_cdindex_id);
  if(stat(file,&st)<0)
  {
    if(errno!=ENOENT)
      return -1;
    else
    {
      cdindex_generate_new_entry(cd_desc, data);
      return 0;
    }
  }

  cdindex_xml=fopen(file,"r");
  while(!feof(cdindex_xml))
  {
    fgets(inbuffer,512,cdindex_xml);
    strip_whitespace(procbuffer,inbuffer,512);
    if(procbuffer[0]!='<')
      continue;

    if(strncmp(procbuffer,"<Title>",7)==0)
    {
      strtok(procbuffer,">");
      strncpy(inbuffer,strtok(NULL,">"),512);
      strncpy(data->data_title,strtok(inbuffer,"<"),256);
    }
    else if(strncmp(procbuffer,"<SingleArtistCD>",16)==0)
      data->data_artist_type=CDINDEX_SINGLE_ARTIST;
    else if(strncmp(procbuffer,"<MultipleArtistCD>",18)==0)
    {
      data->data_artist_type=CDINDEX_MULTIPLE_ARTIST;
      strncpy(data->data_artist,"(various)",256);
    }
    else if(data->data_artist_type==CDINDEX_SINGLE_ARTIST&&strncmp(procbuffer,"<Artist>",8)==0)
    {
      strtok(procbuffer,">");
      strncpy(inbuffer,strtok(NULL,">"),512);
      strncpy(data->data_artist,strtok(inbuffer,"<"),256);
    }
    else if(strncmp(procbuffer,"<Track",6)==0)
    {
      strtok(procbuffer,"\"");
      track=strtol(strtok(NULL,"\""),NULL,10);
      if(track>0)
	track--;
      fgets(inbuffer,512,cdindex_xml);
      strip_whitespace(procbuffer,inbuffer,512);
      if(data->data_artist_type==CDINDEX_MULTIPLE_ARTIST)
      {
	strtok(procbuffer,">");
	strncpy(inbuffer,strtok(NULL,">"),512);
	strncpy(data->data_track[track].track_artist,strtok(inbuffer,"<"),256);
	fgets(inbuffer,512,cdindex_xml);
	strip_whitespace(procbuffer,inbuffer,512);
      }
      strtok(procbuffer,">");
      strncpy(inbuffer,strtok(NULL,">"),512);
      strncpy(data->data_track[track].track_name,strtok(inbuffer,"<"),256);
    }
  }

  fclose(cdindex_xml);

  return 0;
}

int cdindex_write_data(int cd_desc,struct disc_data *data)
{
  FILE *cdindex_xml;
  int tracks;
  char root_dir[PATH_MAX],file[PATH_MAX];
  struct stat st;
  struct disc_info disc;

  if(getenv("HOME")==NULL)
  {
    strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if((data->data_id=__internal_cddb_discid(&disc))<0)
    return -1;

  if(__internal_cdindex_discid(&disc,data->data_cdindex_id,CDINDEX_ID_SIZE)<0)
    return -1;

  snprintf(root_dir,PATH_MAX,"%s/.cdindex",getenv("HOME"));
  snprintf(file,PATH_MAX,"%s/%s",root_dir,data->data_cdindex_id);

  if(stat(root_dir,&st)<0)
  {
    if(errno!=ENOENT)
      return -1;
    else
      mkdir(root_dir,0755);
  }
  else
  {
    if(!S_ISDIR(st.st_mode))
    {
      errno = ENOTDIR;
      return -1;
    }
  }

  if((cdindex_xml=fopen(file,"w"))==NULL)
    return -1;

  fputs("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n",cdindex_xml);
  fputs("<!DOCTYPE CDInfo SYSTEM \"http://www.freeamp.org/dtd/CDInfo.dtd\">\n\n",cdindex_xml);
  fputs("<CDInfo>\n\n",cdindex_xml);
  fprintf(cdindex_xml,"   <Title>%s</Title>\n",data->data_title);
  fprintf(cdindex_xml,"   <NumTracks>%d</NumTracks>\n\n",disc.disc_total_tracks);
  fputs("   <IdInfo>\n",cdindex_xml);
  fputs("      <DiskId>\n",cdindex_xml);
  fprintf(cdindex_xml,"         <Id>%s</Id>\n",data->data_cdindex_id);
  fprintf(cdindex_xml,"         <TOC First=\"%d\" Last=\"%d\">\n",disc.disc_first_track, disc.disc_total_tracks);
  fprintf(cdindex_xml,"            <Offset Num=\"0\">%d</Offset>\n",disc.disc_track[disc.disc_total_tracks].track_lba);
  for(tracks=0;tracks<disc.disc_total_tracks;tracks++)
    fprintf(cdindex_xml,"            <Offset Num=\"%d\">%d</Offset>\n",tracks+1,disc.disc_track[tracks].track_lba);
  fputs("         </TOC>\n",cdindex_xml);
  fputs("      </DiskId>\n",cdindex_xml);
  fputs("   </IdInfo>\n\n",cdindex_xml);
  if(strcmp(data->data_artist,"(various)")!=0)
  {
    fputs("   <SingleArtistCD>\n",cdindex_xml);
    fprintf(cdindex_xml,"      <Artist>%s</Artist>\n",data->data_artist);
    for(tracks=0;tracks<disc.disc_total_tracks;tracks++)
    {
      fprintf(cdindex_xml,"      <Track Num=\"%d\">\n",tracks+1);
      fprintf(cdindex_xml,"         <Name>%s</Name>\n",data->data_track[tracks].track_name);
      fputs("      </Track>\n",cdindex_xml);
    }
    fputs("   </SingleArtistCD>\n\n",cdindex_xml);
  }
  else
  {
    fputs("   <MultipleArtistCD>\n",cdindex_xml);
    for(tracks=0;tracks<disc.disc_total_tracks;tracks++)
    {
      fprintf(cdindex_xml,"      <Track Num=\"%d\">\n",tracks+1);
      fprintf(cdindex_xml,"         <Artist>%s</Artist>\n",data->data_track[tracks].track_artist);
      fprintf(cdindex_xml,"         <Name>%s</Name>\n",data->data_track[tracks].track_name);
      fputs("      </Track>\n",cdindex_xml);
    }
    fputs("   </MultipleArtistCD>\n\n",cdindex_xml);
  }
  fputs("</CDInfo>\n\n\n\n\n",cdindex_xml);
  fclose(cdindex_xml);

  return 0;
}

int cdindex_mc_read(int cd_desc,int sock,struct disc_mc_data *data,char *http_string)
{
  struct disc_data indata;

  if(cdindex_read(cd_desc,sock,&indata,http_string)<0)
    return -1;

  if(cddb_mc_alloc(cd_desc,data)<0)
    return -1;

  if(cddb_mc_copy_from_data(data,&indata)<0)
  {
    cddb_mc_free(data);
    return -1;
  }

  return 0;
}

int cdindex_mc_read_disc_data(int cd_desc,struct disc_mc_data *data)
{
  struct disc_data indata;

  if(cdindex_read_disc_data(cd_desc,&indata)<0)
    return -1;

  if(cddb_mc_alloc(cd_desc,data)<0)
    return -1;

  if(cddb_mc_copy_from_data(data,&indata)<0)
  {
    cddb_mc_free(data);
    return -1;
  }

  return 0;
}

int cdindex_mc_write_disc_data(int cd_desc,struct disc_mc_data *data)
{
  struct disc_data outdata;

  if(cddb_data_copy_from_mc(&outdata,data)<0)
    return -1;

  if(cdindex_write_data(cd_desc,&outdata)<0)
    return -1;

  return 0;
}

int cdindex_mc_generate_new_entry(int cd_desc,struct disc_mc_data *data)
{
  return cddb_mc_generate_new_entry(cd_desc,data);
}

int cdindex_stat_disc_data(int cd_desc,struct cddb_entry *entry)
{
  struct disc_info disc;
  struct stat st;
  char root_dir[PATH_MAX],file[PATH_MAX];

  if(getenv("HOME")==NULL)
  {
    strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if((entry->entry_id=__internal_cddb_discid(&disc))<0)
    return -1;

  if(__internal_cdindex_discid(&disc,entry->entry_cdindex_id,CDINDEX_ID_SIZE)<0)
    return -1;

  snprintf(root_dir,PATH_MAX,"%s/.cdindex",getenv("HOME"));

  if(stat(root_dir,&st)<0)
  {
    if(errno!=ENOENT)
      return -1;
    else
    {
      entry->entry_present=0;
        return 0;
    }
  }
  else
  {
    if(!S_ISDIR(st.st_mode))
    {
      errno=ENOTDIR;
      return -1;
    }
  }

  snprintf(file,PATH_MAX,"%s/%s",root_dir,entry->entry_cdindex_id);
  if(stat(file,&st)==0)
  {
    entry->entry_timestamp=st.st_mtime;
    entry->entry_present=1;
    entry->entry_genre=CDDB_MISC;
    return 0;
  }

  entry->entry_present=0;

  return 0;
}

int cdindex_http_submit(int cd_desc,const struct cddb_host *host,struct cddb_server *proxy)
{
  FILE *cdindex_entry;
  int sock,token[3];
  char outbuffer[512],cdindex_file[PATH_MAX];
  struct stat st;
  struct cddb_entry entry;
  struct disc_status status;

  if(getenv("HOME")==NULL)
  {
    strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  if(cd_poll(cd_desc,&status)<0)
    return -1;

  if(!status.status_present)
    return -1;

  cdindex_stat_disc_data(cd_desc,&entry);

  if(!entry.entry_present)
  {
    strncpy(cddb_message,"No CD Index entry present in cache",256);
    return -1;
  }

  if(proxy!=NULL)
  {
    if((sock=cddb_connect(proxy))<0)
    {
      strncpy(cddb_message,strerror(errno),256);
      return -1;
    }
    snprintf(outbuffer,512,"POST http://%s:%d/%s HTTP/1.0\n",host->host_server.server_name,host->host_server.server_port,CDINDEX_SUBMIT_CGI);
  }
  else
  {
    if((sock=cddb_connect(&host->host_server))<0)
    {
      strncpy(cddb_message,strerror(errno),256);
      return -1;
    }
    snprintf(outbuffer,512,"POST /%s HTTP/1.0\n",CDINDEX_SUBMIT_CGI);
  }
  write(sock,outbuffer,strlen(outbuffer));

  strncpy(outbuffer,"Content-Type: text/plain\n",512);
  write(sock,outbuffer,strlen(outbuffer));

  snprintf(cdindex_file,PATH_MAX,"%s/.cdindex/%s",getenv("HOME"),entry.entry_cdindex_id);
  stat(cdindex_file,&st);

  snprintf(outbuffer,512,"Content-Length: %d\n\r\n",(int)st.st_size+1);
  write(sock,outbuffer,strlen(outbuffer));

  cdindex_entry=fopen(cdindex_file,"r");
  while(!feof(cdindex_entry))
  {
    fgets(outbuffer,512,cdindex_entry);
    write(sock,outbuffer,strlen(outbuffer));
  }

  cddb_skip_http_header(sock);

  if(cddb_read_token(sock,token)<0)
    return -1;

  if(token[0]!=1)
    return -1;

  close(sock);

  return 0;
}
