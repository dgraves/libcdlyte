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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pwd.h>
#else
#include <winsock2.h>
#include <windows.h>

#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

#endif

#define __LIBCDAUDIO_INTERNAL

#ifndef INADDR_NONE
#define INADDR_NONE 0xFFFFFFFF
#endif

#include "cdlyte.h"
#include "data.h"

/* Static function prototypes */
static int cddb_sum(long val);
static int cddb_serverlist_process_line(char *line,struct cddb_conf *conf,struct cddb_serverlist *list,struct cddb_server *proxy);
static int cddb_process_line(char *line,struct __unprocessed_disc_data *data);
static int cddb_sites_process_line(char *line,struct cddb_host *host);
int __internal_cdindex_discid(const struct disc_info *disc,char *discid,int len);

/* Global definitions */
char cddb_message[256];
int use_cddb_message=1;
int parse_disc_artist=1;
int cddb_submit_method=CDDB_SUBMIT_EMAIL;
char *cddb_submit_email_address=CDDB_EMAIL_SUBMIT_ADDRESS;
char *proxy_auth_username=NULL;
char *proxy_auth_password=NULL;

/* CDDB sum function */
static int cddb_sum(long val)
{
  char *bufptr,buf[16];
  int ret=0;

#ifndef WIN32
  snprintf(buf,16,"%lu",val);
#else
  _snprintf(buf,16,"%lu",val);
#endif
  for(bufptr=buf;*bufptr!='\0';bufptr++)
    ret+=(*bufptr-'0');

  return ret;
}

/* Produce CDDB ID for the CD in the device referred to by cd_desc */
unsigned long cddb_direct_discid(const struct disc_info *disc)
{
  int index,tracksum=0,discid;

  for(index=0;index<disc->disc_total_tracks;index++)
    tracksum+=cddb_sum(disc->disc_track[index].track_pos.minutes*60+disc->disc_track[index].track_pos.seconds);

  discid=(disc->disc_length.minutes*60+disc->disc_length.seconds)-(disc->disc_track[0].track_pos.minutes*60+disc->disc_track[0].track_pos.seconds);

  return ((tracksum%0xFF)<<24|discid<<8|disc->disc_total_tracks)&0xFFFFFFFF;
}

unsigned long __internal_cddb_discid(struct disc_info *disc)
{
  return cddb_direct_discid(disc);
}

long cddb_discid(int cd_desc)
{
  struct disc_info disc;

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if(!disc.disc_present)
    return -1;

  return __internal_cddb_discid(&disc);
}

/* Allocate structure space for CDDB information */
int cddb_direct_mc_alloc(struct disc_mc_data *data, int tracks)
{
  int index,deindex;

  data->data_total_tracks=tracks;
  data->data_title_len=-1;
  data->data_title=NULL;
  data->data_artist_len=-1;
  data->data_artist=NULL;
  data->data_extended_len=-1;
  data->data_extended=NULL;

  if((data->data_track=calloc(tracks+1,sizeof(struct track_mc_data)))==NULL)
    return -1;

  for(index=0;index<tracks;index++)
  {
    if((data->data_track[index]=malloc(sizeof(struct track_mc_data)))==NULL)
    {
      for(deindex=0;deindex<index;deindex++)
	free(data->data_track[deindex]);
      free(data->data_track);
      return -1;
    }
    data->data_track[index]->track_name_len=-1;
    data->data_track[index]->track_name=NULL;
    data->data_track[index]->track_artist_len=-1;
    data->data_track[index]->track_artist=NULL;
    data->data_track[index]->track_extended_len=-1;
    data->data_track[index]->track_extended=NULL;
  }

  data->data_track[index+1]=NULL;

  return 0;
}

int cddb_mc_alloc(int cd_desc,struct disc_mc_data *data)
{
  struct disc_info disc;

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  return cddb_direct_mc_alloc(data,disc.disc_total_tracks);
}

void cddb_mc_free(struct disc_mc_data *data)
{
  int index=0;

  if(data->data_title!=NULL)
    free(data->data_title);

  if(data->data_artist!=NULL)
    free(data->data_artist);

  if(data->data_extended!=NULL)
    free(data->data_extended);

  while(data->data_track[index]!=NULL&&index<100)
  {
    if(data->data_track[index]->track_name!=NULL)
      free(data->data_track[index]->track_name);
    if(data->data_track[index]->track_artist!=NULL)
      free(data->data_track[index]->track_artist);
    if(data->data_track[index]->track_extended!=NULL)
      free(data->data_track[index]->track_extended);

    free(data->data_track[index++]);
  }

  free(data->data_track);
}

int cddb_mc_copy_from_data(struct disc_mc_data *outdata,const struct disc_data *indata)
{
  int track;

  outdata->data_id=indata->data_id;
  strncpy(outdata->data_cdindex_id,indata->data_cdindex_id,CDINDEX_ID_SIZE);

  outdata->data_revision=indata->data_revision;
  outdata->data_genre=indata->data_genre;
  outdata->data_artist_type=indata->data_artist_type;

  outdata->data_title_len=strlen(indata->data_title)+1;
  if((outdata->data_title = malloc(outdata->data_title_len))==NULL)
    return -1;
  strncpy(outdata->data_title,indata->data_title,outdata->data_title_len);
   
  outdata->data_artist_len=strlen(indata->data_artist)+1;
  if((outdata->data_artist=malloc(outdata->data_artist_len))==NULL)
    return -1;
  strncpy(outdata->data_artist,indata->data_artist,outdata->data_artist_len);

  outdata->data_extended_len=strlen(indata->data_extended)+1;
  if((outdata->data_extended=malloc(outdata->data_extended_len))==NULL)
    return -1;
  strncpy(outdata->data_extended,indata->data_extended,outdata->data_extended_len);

  for(track=0;track<outdata->data_total_tracks;track++)
  {
    outdata->data_track[track]->track_name_len=strlen(indata->data_track[track].track_name)+1;
    if((outdata->data_track[track]->track_name=malloc(outdata->data_track[track]->track_name_len))==NULL)
      return -1;
    strncpy(outdata->data_track[track]->track_name,indata->data_track[track].track_name,outdata->data_track[track]->track_name_len);

    outdata->data_track[track]->track_artist_len=strlen(indata->data_track[track].track_artist)+1;
    if((outdata->data_track[track]->track_artist=malloc(outdata->data_track[track]->track_artist_len))==NULL)
      return -1;
    strncpy(outdata->data_track[track]->track_artist, indata->data_track[track].track_artist, outdata->data_track[track]->track_artist_len);

    outdata->data_track[track]->track_extended_len=strlen(indata->data_track[track].track_extended)+1;
    if((outdata->data_track[track]->track_extended=malloc(outdata->data_track[track]->track_extended_len))==NULL)
      return -1;
    strncpy(outdata->data_track[track]->track_extended,indata->data_track[track].track_extended,outdata->data_track[track]->track_extended_len);
  }

  return 0;
}

int cddb_data_copy_from_mc(struct disc_data *outdata,const struct disc_mc_data *indata)
{
  int track;

  outdata->data_id=indata->data_id;
  strncpy(outdata->data_cdindex_id,indata->data_cdindex_id,CDINDEX_ID_SIZE);

  outdata->data_revision=indata->data_revision;
  outdata->data_genre=indata->data_genre;
  outdata->data_artist_type=indata->data_artist_type;

  strncpy(outdata->data_title,indata->data_title,256);
  strncpy(outdata->data_artist,indata->data_artist,256);
  strncpy(outdata->data_extended,indata->data_extended,EXTENDED_DATA_SIZE);

  for(track = 0; track < indata->data_total_tracks; track++)
  {
    strncpy(outdata->data_track[track].track_name,indata->data_track[track]->track_name,256);
    strncpy(outdata->data_track[track].track_artist,indata->data_track[track]->track_artist,256);
    strncpy(outdata->data_track[track].track_extended,indata->data_track[track]->track_extended,EXTENDED_DATA_SIZE);
  }

   return 0;
}

/* Transform a URL in the CDDB configuration file into a valid hostname */
int cddb_process_url(struct cddb_host *host,const char *url)
{
  int index=0;
  char *procbuffer;

  host->host_addressing[0]='\0';

  if(strchr(url,':')==NULL)
    return -1;

  while(url[index++]!=':'&&index<527)
  {
    if(index>5)
      return -1;
  }

  if(strncmp(url,"http",index-1)==0)
  {
    host->host_protocol=CDDB_MODE_HTTP;
    host->host_server.server_port=HTTP_DEFAULT_PORT;
  }
  else if(strncmp(url,"cddbp",index-1)==0)
  {
    host->host_protocol=CDDB_MODE_CDDBP;
    host->host_server.server_port=CDDBP_DEFAULT_PORT;
  }
  else
    return -1;

  url+=(index-1);

  if(strncmp(url,"://",3)!=0)
    return -1;

  url+=3;

  index=0;
  while(url[index]!=':'&&url[index]!='\0'&&url[index]!='/'&&index<527)
  {
    index++;
    if(index>256)
      return -1;
  }

  memset(host->host_server.server_name,'\0',256);
  strncpy(host->host_server.server_name,url,(index<256)?index:256);

  if(url[index]==':')
  {
    url+=(index+1);
    index=0;
    while(url[index]!='\0'&&url[index]!='/'&&index<527)
    {
      index++;
      if(index>5)
        return -1;
    }

    if((procbuffer=malloc(index+1))==NULL)
      return -1;

    memset(procbuffer,'\0',index+1);
    strncpy(procbuffer,url,index);
    host->host_server.server_port=strtol(procbuffer,NULL,10);
    free(procbuffer);
  }

  if(url[index]=='/')
  {
    url+=(index+1);
    if(url[0]=='\0')
      return 0;
    index=0;
    while(url[index++]!='\0')
    {
      if(index>256)
        return -1;
    }

    strncpy(host->host_addressing, url, index);

    return 0;
  }

  return 0;
}

/* Process a line in a CDDB configuration file */
static int cddb_serverlist_process_line(char *line,struct cddb_conf *conf,struct cddb_serverlist *list,struct cddb_server *proxy)
{
  int index = 0;
  struct cddb_host proxy_host;
  char *var, *value, *procval;

  if(strchr(line,'=')==NULL)
    return 0;

  line[strlen(line)-1]='\0';

  while(line[index]!='='&&line[index]!='\0')
    index++;

  line[index]='\0';
  var=line;
  value=line+index+1;

  if(strcasecmp(var,"ACCESS")==0)
  {
    if(strncasecmp(value,"LOCAL",2)==0)
      conf->conf_access=CDDB_ACCESS_LOCAL;
    else
      conf->conf_access=CDDB_ACCESS_REMOTE;
  }
  else if(strcasecmp(var,"PROXY")==0)
  {
    if(cddb_process_url(&proxy_host,value)<0)
      return -1;
    conf->conf_proxy=CDDB_PROXY_ENABLED;
    memcpy(proxy,&proxy_host.host_server,sizeof(struct cddb_server));
  }
  else if(strcasecmp(var,"SERVER")==0)
  {
    if(list->list_len>=CDDB_MAX_SERVERS)
      return 0;
    if(strchr(value,' ')!=NULL)
    {
      index=0;
      while(value[index]!=' '&&value[index]!='\0')
	index++;

      value[index]='\0';
      procval = value + index + 1;
    }
    else
      procval=NULL;
    if(cddb_process_url(&list->list_host[list->list_len],value)!=-1)
    {
      if(procval!=NULL&&strcmp(procval,"CDI")==0)
        list->list_host[list->list_len].host_protocol=CDINDEX_MODE_HTTP;
      else if(procval!=NULL&&strcmp(procval,"COVR")==0)
        list->list_host[list->list_len].host_protocol=COVERART_MODE_HTTP;
      list->list_len++;
    }
  }

  return 0;
}

/* Read ~/.cddbrc */
int cddb_read_serverlist(struct cddb_conf *conf,struct cddb_serverlist *list,struct cddb_server *proxy)
{
  FILE *cddbconf;
  int index;
  char inbuffer[PATH_MAX];
  struct stat st;

  if(getenv("HOME")==NULL)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  list->list_len=0;
  conf->conf_access=CDDB_ACCESS_REMOTE;
  conf->conf_proxy=CDDB_PROXY_DISABLED;

  snprintf(inbuffer,PATH_MAX,"%s/.cdserverrc",getenv("HOME"));
  if(stat(inbuffer,&st)<0)
    return 0;

  if((cddbconf=fopen(inbuffer,"r"))==NULL)
    return -1;

  while(!feof(cddbconf))
  {
    fgets(inbuffer,PATH_MAX,cddbconf);

    for(index=0;index<strlen(inbuffer);index++)
    {
      if(inbuffer[index]=='#')
      {
        inbuffer[index]='\0';
        break;
      }
    }

    if(cddb_serverlist_process_line(inbuffer,conf,list,proxy)<0)
      return -1;
  }

  fclose(cddbconf);

  return 0;
}

/* Write ~/.cddbrc */
int cddb_write_serverlist(const struct cddb_conf *conf,const  struct cddb_serverlist *list,const struct cddb_server *proxy)
{
  FILE *cddbconf;
  time_t timeval;
  int index;
  char localconfpath[PATH_MAX];

  if(getenv("HOME")==NULL)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  snprintf(localconfpath,PATH_MAX,"%s/.cdserverrc",getenv("HOME"));
  if((cddbconf=fopen(localconfpath,"w"))==NULL)
    return -1;

  timeval=time(NULL);
  fprintf(cddbconf,"# CD Server configuration file generated by %s %s.\n",PACKAGE,VERSION);
  fprintf(cddbconf,"# Created %s\n",ctime(&timeval));
  if(conf->conf_access==CDDB_ACCESS_REMOTE)
    fputs("ACCESS=REMOTE\n",cddbconf);
  else
    fputs("ACCESS=LOCAL\n",cddbconf);

  if(conf->conf_proxy==CDDB_PROXY_ENABLED)
    fprintf(cddbconf,"PROXY=http://%s:%d/\n",proxy->server_name,proxy->server_port);
  for(index=0;index<list->list_len;index++)
  {
    switch(list->list_host[index].host_protocol)
    {
      case CDDB_MODE_HTTP:
        fprintf(cddbconf,"SERVER=http://%s:%d/%s CDDB\n",list->list_host[index].host_server.server_name,list->list_host[index].host_server.server_port,list->list_host[index].host_addressing);
	break;
      case CDDB_MODE_CDDBP:
	fprintf(cddbconf,"SERVER=cddbp://%s:%d/ CDDB\n",list->list_host[index].host_server.server_name,list->list_host[index].host_server.server_port);
	break;
      case CDINDEX_MODE_HTTP:
	fprintf(cddbconf,"SERVER=http://%s:%d/%s CDI\n",list->list_host[index].host_server.server_name,list->list_host[index].host_server.server_port,list->list_host[index].host_addressing);
	break;
      case COVERART_MODE_HTTP:
	fprintf(cddbconf,"SERVER=http://%s:%d/%s COVR\n",list->list_host[index].host_server.server_name,list->list_host[index].host_server.server_port,list->list_host[index].host_addressing);
    }
  }

  fclose(cddbconf);

  return 0;
}

/* Convert numerical genre to text */
char *cddb_genre(int genre)
{
  switch(genre)
  {
    case CDDB_BLUES:
      return "blues";
    case CDDB_CLASSICAL:
      return "classical";
    case CDDB_COUNTRY:
      return "country";
    case CDDB_DATA:
      return "data";
    case CDDB_FOLK:
      return "folk";
    case CDDB_JAZZ:
      return "jazz";
    case CDDB_MISC:
      return "misc";
    case CDDB_NEWAGE:
      return "newage";
    case CDDB_REGGAE:
      return "reggae";
    case CDDB_ROCK:
      return "rock";
    case CDDB_SOUNDTRACK:
      return "soundtrack";
  }

  return "unknown";
}

/* Convert genre from text form into an integer value */
int cddb_genre_value(char *genre)
{
  if(strcmp(genre,"blues")==0)
    return CDDB_BLUES;
  else if(strcmp(genre,"classical")==0)
    return CDDB_CLASSICAL;
  else if(strcmp(genre,"country")==0)
    return CDDB_COUNTRY;
  else if(strcmp(genre,"data")==0)
    return CDDB_DATA;
  else if(strcmp(genre,"folk")==0)
    return CDDB_FOLK;
  else if(strcmp(genre,"jazz")==0)
    return CDDB_JAZZ;
  else if(strcmp(genre,"misc")==0)
    return CDDB_MISC;
  else if(strcmp(genre,"newage")==0)
    return CDDB_NEWAGE;
  else if(strcmp(genre,"reggae")==0)
    return CDDB_REGGAE;
  else if(strcmp(genre,"rock")==0)
    return CDDB_ROCK;
  else if(strcmp(genre,"soundtrack")==0)
    return CDDB_SOUNDTRACK;
  else
    return CDDB_UNKNOWN;
}

int cddb_connect(const struct cddb_server *server)
{
  int sock;
  struct sockaddr_in sin;
  struct hostent *host;

  sin.sin_family=AF_INET;
  sin.sin_port=htons(server->server_port);
     
  if((sin.sin_addr.s_addr=inet_addr(server->server_name))==INADDR_NONE)
  {
    if((host=gethostbyname(server->server_name))==NULL)
    {
      if(use_cddb_message)
        strncpy(cddb_message,strerror(errno),256);
      return -1;
    }
      
    memcpy(&sin.sin_addr,host->h_addr,host->h_length);
  }

  if((sock=socket(AF_INET,SOCK_STREAM,0))<0)
  {
    if(use_cddb_message)
      strncpy(cddb_message,strerror(errno),256);
    return -1;
  }

  if(connect(sock,(struct sockaddr *)&sin,sizeof(sin))<0)
  {
    if(use_cddb_message)
      strncpy(cddb_message,strerror(errno),256);
    return -1;
  }

  return sock;
}
		  
/* Connect to a CDDB server and say hello */
int cddb_connect_server(const struct cddb_host *host,const struct cddb_server *proxy,const  struct cddb_hello *hello,...)
{
  int sock,token[3],http_string_len;
  char outbuffer[256],*http_string;
  va_list arglist;

  va_start(arglist,hello);
   
  if(proxy!=NULL)
  {
    if((sock=cddb_connect(proxy))<0)
      return -1;
  }
  else
  {
    if((sock=cddb_connect(&host->host_server))<0)
      return -1;
  }

  if(host->host_protocol==CDDB_MODE_HTTP)
  {
    http_string=va_arg(arglist,char *);
    http_string_len=va_arg(arglist,int);
    if(proxy!=NULL)
      snprintf(http_string,http_string_len,"GET http://%s:%d/%s?hello=anonymous+anonymous+%s+%s&proto=%d HTTP/1.0\n\n",host->host_server.server_name,host->host_server.server_port,host->host_addressing,hello->hello_program,hello->hello_version,CDDB_PROTOCOL_LEVEL);
    else
      snprintf(http_string,http_string_len,"GET /%s?hello=anonymous+anonymous+%s+%s&proto=%d HTTP/1.0\n\n",host->host_addressing,hello->hello_program,hello->hello_version,CDDB_PROTOCOL_LEVEL);
  }
  else
  {
    if(cddb_read_token(sock,token)<0)
    {
      va_end(arglist);
      return -1;
    }

    if(token[0]!=2)
    {
      va_end(arglist);  
      return -1;
    }

    snprintf(outbuffer,256,"cddb hello anonymous anonymous %s %s\n",hello->hello_program,hello->hello_version);
    if(write(sock,outbuffer,strlen(outbuffer))<0)
    {
      va_end(arglist);
      return -1;
    }

    if(cddb_read_token(sock,token)<0)
    {
      va_end(arglist);
      return -1;
    }

    if(token[0]!=2)
    {
      va_end(arglist);
      return -1;
    }

    snprintf(outbuffer,256,"proto %d\n",CDDB_PROTOCOL_LEVEL);
    if(write(sock,outbuffer,strlen(outbuffer))<0)
    {
      va_end(arglist);
      return -1;
    }

    if(cddb_read_token(sock,token)<0)
    {
      va_end(arglist);
      return -1;
    }
  }

  va_end(arglist);
  return sock;
}

/* Generate the CDDB request string */
static int cddb_generate_http_request(char *outbuffer,const char *cmd,char *http_string,int outbuffer_len)
{
  int index=0;
  char *reqstring;

  if(strchr(http_string,'?')==NULL)
    return -1;

  while(http_string[index]!='?'&&http_string[index]!='\0')
    index++;

  http_string[index]='\0';
  reqstring=http_string+index+1;

  snprintf(outbuffer,outbuffer_len,"%s?cmd=%s&%s\n",http_string,cmd,reqstring);
  http_string[index]='\?';

  return 0;
}

/* Skip HTTP header */
int cddb_skip_http_header(int sock)
{
  char inchar;
  int len;

  do
  {
    len=0;
    do
    {
      if(read(sock,&inchar,1)<1)
      {
	if(use_cddb_message)
	  strncpy(cddb_message,"Unexpected socket closure",256);
	return -1;
      }
      len++;
    } while(inchar!='\n');
  } while(len>2);

  return 0;
}

/* Read a single line */
static int cddb_read_line(int sock,char *inbuffer,int len)
{
  int index;
  char inchar;

  for(index=0;index<len;index++)
  {
    read(sock,&inchar,1);
    if(inchar=='\n')
    {
      inbuffer[index]='\0';
      if(inbuffer[0]=='.')
        return 1;
      return 0;
    }
    inbuffer[index] = inchar;
  }

  return index;
}

/* Query the CDDB for the CD currently in the CD-ROM, and find the ID of the
   CD (which may or may not be the one generated) and what section it is
   under (genre) */
int cddb_query(int cd_desc,int sock,int mode,struct cddb_query *query,...)
{
  unsigned long discid;
  int index,slashed=0,token[3];
  struct disc_info disc;
  char outbuffer[1024],outtemp[1024],inbuffer[256],*inbuffer_ptr,*http_string;
  va_list arglist;

  va_start(arglist,query);
  query->query_matches=0;
  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if((discid=__internal_cddb_discid(&disc))<0)
    return -1;

  if(mode==CDDB_MODE_HTTP)
  {
    http_string=va_arg(arglist,char *);
    snprintf(outbuffer,1024,"%d",disc.disc_total_tracks);
    for(index=0;index<disc.disc_total_tracks;index++)
    {
      snprintf(outtemp,1024,"%s+%d",outbuffer,(disc.disc_track[index].track_pos.minutes*60+disc.disc_track[index].track_pos.seconds)*75+disc.disc_track[index].track_pos.frames);
      strncpy(outbuffer,outtemp,1024);
    }
    snprintf(outtemp,1024,"cddb+query+%08lx+%s+%d",discid,outbuffer,disc.disc_length.minutes*60+disc.disc_length.seconds);
    cddb_generate_http_request(outbuffer,outtemp,http_string,1024);
  }
  else
  {
    snprintf(outbuffer,1024,"%d",disc.disc_total_tracks);
    for(index=0;index<disc.disc_total_tracks;index++)
    {
      snprintf(outtemp,1024,"%s %d",outbuffer,(disc.disc_track[index].track_pos.minutes*60+disc.disc_track[index].track_pos.seconds)*75+disc.disc_track[index].track_pos.frames);
      strncpy(outbuffer,outtemp,1024);
    }
    strncpy(outtemp,outbuffer,1024);
    snprintf(outbuffer,1024,"cddb query %08lx %s %d\n",discid,outtemp,disc.disc_length.minutes*60+disc.disc_length.seconds);
  }

  va_end(arglist);

  if(write(sock,outbuffer,strlen(outbuffer))<0)
    return -1;

  if(mode==CDDB_MODE_HTTP)
    cddb_skip_http_header(sock);

  inbuffer_ptr=inbuffer;

  if(cddb_read_line(sock,inbuffer_ptr,256)<0)
    return -1;
   
  if(strncmp(inbuffer_ptr,"<!DOC",5)==0)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"404 CDDB CGI not found",256);
    return -1;
  }

  token[0]=inbuffer_ptr[0]-48;
  token[1]=inbuffer_ptr[1]-48;
  token[2]=inbuffer_ptr[2]-48;

  if(use_cddb_message)
    strncpy(cddb_message,inbuffer_ptr+4,256);

  if(token[0]!=2)
    return -1;
   
  if(token[1]==0)
  {
    if(token[2]!=0)
    {
      query->query_match=QUERY_NOMATCH;
      return 0;
    }

    query->query_match=QUERY_EXACT;
    query->query_matches=1;
    slashed = 0;
    if(strchr(inbuffer_ptr,'/')!=NULL&&parse_disc_artist)
    {
      index=0;
      while(inbuffer_ptr[index]!='/'&&inbuffer_ptr[index]!='\0')
        index++;
      inbuffer_ptr[index-1]='\0';
      strncpy(query->query_list[0].list_title,inbuffer_ptr+index+2,64);
      slashed=1;
    }

    index=0;
    while(inbuffer_ptr[index]!=' '&&inbuffer_ptr[index]!='\0')
      index++;
    if(inbuffer_ptr[index]=='\0')
      return -1;
    inbuffer_ptr+=index+1;
    index=0;
    while(inbuffer_ptr[index]!=' '&&inbuffer_ptr[index]!='\0')
      index++;
    if(inbuffer_ptr[index]=='\0')
      return -1;
    inbuffer_ptr[index]='\0';
    query->query_list[0].list_genre=cddb_genre_value(inbuffer_ptr);
    inbuffer_ptr+=index+1;
    index = 0;
    while(inbuffer_ptr[index]!=' '&&inbuffer_ptr[index]!='\0')
      index++;
    if(inbuffer_ptr[index]=='\0')
      return -1;
    inbuffer_ptr[index]='\0';
    query->query_list[0].list_id=strtoul(inbuffer_ptr,NULL,16); 
    inbuffer_ptr+=index+1;
    if(slashed)
      strncpy(query->query_list[0].list_artist,inbuffer_ptr,64);
    else
    {
      strncpy(query->query_list[0].list_title,inbuffer_ptr,64);
      strncpy(query->query_list[0].list_artist,"",64);
    }
    inbuffer_ptr=inbuffer;
  }
  else if(token[1]==1)
  {
    if(token[2]==0)
      query->query_match=QUERY_EXACT;
    else if(token[2]==1)
      query->query_match=QUERY_INEXACT;
    else
    {
      query->query_match = QUERY_NOMATCH;
      return 0;
    }

    query->query_matches=0;
    while(!cddb_read_line(sock,inbuffer_ptr,256))
    {
      slashed=0;
      if(strchr(inbuffer_ptr, '/')!=NULL&&parse_disc_artist)
      {
	index=0;
	while(inbuffer_ptr[index]!='/'&&inbuffer_ptr[index]!='\0')
	  index++;
	inbuffer_ptr[index-1]='\0';
	strncpy(query->query_list[query->query_matches].list_title,inbuffer_ptr+index+2,64);
	slashed=1;
      }

      index=0;
      while(inbuffer_ptr[index]!=' '&&inbuffer_ptr[index]!='\0')
	index++;
      if(inbuffer_ptr[index]=='\0')
	return -1;
      inbuffer_ptr[index]='\0';
      query->query_list[query->query_matches].list_genre=cddb_genre_value(inbuffer_ptr);
      inbuffer_ptr+=index+1;
      index=0;
      while(inbuffer_ptr[index]!=' '&&inbuffer_ptr[index]!='\0')
        index++;
      if(inbuffer_ptr[index]=='\0')
	return -1;
      inbuffer_ptr[index]='\0'; 
      query->query_list[query->query_matches].list_id=strtoul(inbuffer_ptr,NULL,16);
      inbuffer_ptr+=index+1;
      if(slashed)
        strncpy(query->query_list[query->query_matches++].list_artist,inbuffer_ptr,64);
      else
      {
	strncpy(query->query_list[query->query_matches].list_title,inbuffer_ptr,64);
	strncpy(query->query_list[query->query_matches++].list_artist,"",64);
      }
      inbuffer_ptr=inbuffer;
    }
  }
  else
  {
    query->query_match=QUERY_NOMATCH;
    return 0;
  }
   
  return 0;
}

static int cddb_process_line(char *line,struct __unprocessed_disc_data *data)
{
  int index=0;
  char *var,*value;

  line[strlen(line)-1]='\0';
  if(strstr(line,"Revision")!=NULL)
  {
    while(line[index]!=':'&&line[index]!='\0')
      index++;
    data->data_revision = strtol(line + index + 2, NULL, 10);
    return 0;
  }	  

  if(strchr(line,'=')==NULL)
    return 0;

  while(line[index]!='='&&line[index]!='\0')
    index++;
  line[index]='\0';
  var=line;
  value=line+index+1;

  if(value==NULL)
    value="";
   
  if(strcmp(var,"DTITLE")==0)
  {
    if(data->data_title_index>=MAX_EXTEMPORANEOUS_LINES)
      return 0;
    strncpy(data->data_title[data->data_title_index++], value, 80);
  }
  else if(strncmp(var,"TTITLE",6)==0)
  {
    if(data->data_track[strtol((char *)var+6,NULL,10)].track_name_index>=MAX_EXTEMPORANEOUS_LINES)
      return 0;
    strncpy(data->data_track[strtol((char *)var+6,NULL,10)].track_name[data->data_track[strtol((char *)var+6,NULL,10)].track_name_index++],value,80);
  }
  else if(strcmp(var,"EXTD")==0)
  {
    if(data->data_extended_index>=MAX_EXTENDED_LINES)
      return 0;
    strncpy(data->data_extended[data->data_extended_index++],value,80);
  }
  else if(strncmp(var,"EXTT",4)==0)
  {
    if(data->data_track[strtol((char *)var+4,NULL,10)].track_extended_index>=MAX_EXTENDED_LINES)
      return 0;
    strncpy(data->data_track[strtol((char *)var+4,NULL,10)].track_extended[data->data_track[strtol((char *)var+4,NULL,10)].track_extended_index++],value,80);
  }

  return 0;
}

/* Read the actual CDDB entry */
int cddb_vread(int cd_desc,int sock,int mode,const struct cddb_entry *entry,struct disc_data *data,va_list arglist)
{
  int index,token[3];
  char outbuffer[512],proc[512],*http_string;
  struct disc_info disc;
  struct __unprocessed_disc_data indata;

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if((indata.data_id=__internal_cddb_discid(&disc))<0)
    return -1;

  indata.data_genre=entry->entry_genre;
  indata.data_title_index=0;
  indata.data_extended_index=0;
  for(index=0;index<disc.disc_total_tracks;index++)
  {
    indata.data_track[index].track_name_index=0;
    indata.data_track[index].track_extended_index=0;
  }

  if(mode==CDDB_MODE_HTTP)
  {
    http_string=va_arg(arglist,char *);
    snprintf(proc,512,"cddb+read+%s+%08lx",cddb_genre(entry->entry_genre),entry->entry_id);
    cddb_generate_http_request(outbuffer,proc,http_string,512);
  }
  else
    snprintf(outbuffer,512,"cddb read %s %08lx\n",cddb_genre(entry->entry_genre),entry->entry_id);

  write(sock,outbuffer,strlen(outbuffer));

  if(mode==CDDB_MODE_HTTP)
    cddb_skip_http_header(sock);

  if(cddb_read_token(sock,token)<0)
    return -1;

  if(token[0]!=2&&token[1]!=1)
    return -1;

  while(!cddb_read_line(sock,proc,512))
    cddb_process_line(proc,&indata);

  data_format_input(data,&indata,disc.disc_total_tracks);
  data->data_revision++;

  return 0;
}

int cddb_read(int cd_desc,int sock,int mode,const struct cddb_entry *entry,struct disc_data *data,...)
{
  int ret;
  va_list arglist;

  va_start(arglist,data);
  ret=cddb_vread(cd_desc,sock,mode,entry,data,arglist);
  va_end(arglist);
   
  return ret;
}

/* Process a single line in the sites list */
static int cddb_sites_process_line(char *line,struct cddb_host *host)
{
  int index=0;

  if(strchr(line,' ')==NULL)
    return -1;

  while(line[index++]!=' ');
  index--;
  line[index]='\0';
  strncpy(host->host_server.server_name,line,256);

  line+=index+1;
  if(strncasecmp(line,"cddbp",5)==0)
  {
    host->host_protocol=CDDB_MODE_CDDBP;
    line+=6;
  }
  else if(strncasecmp(line,"http",4)==0)
  {
    host->host_protocol=CDDB_MODE_HTTP;
    line+=5;
  }
  else return -1;

  if(strchr(line,' ')==NULL)
    return -1;

  index=0;
  while(line[index++]!=' ');
  index--;
  line[index]='\0';
  host->host_server.server_port=strtol(line,NULL,10);

  line+=index+1;
  if(strcmp(line,"-")!=0)
    strncpy(host->host_addressing,line+1,256);
  else
    strncpy(host->host_addressing,"",256);

  return 0;
}

/* Read the CDDB sites list */
int cddb_sites(int sock,int mode,struct cddb_serverlist *list,...)
{
  int token[3],http_string_len;
  char buffer[512],*http_string;
  va_list arglist;

  va_start(arglist,list);
  if(mode==CDDB_MODE_HTTP)
  {
    http_string=va_arg(arglist,char *);
    http_string_len=va_arg(arglist,int);
    cddb_generate_http_request(buffer,"sites",http_string,512);
  }
  else
    strcpy(buffer,"sites\n");

  va_end(arglist);

  write(sock,buffer,strlen(buffer));

  if(mode==CDDB_MODE_HTTP)
    cddb_skip_http_header(sock);

  if(cddb_read_token(sock,token)<0)
    return -1;

  if(token[0]!=2)
    return -1;

  list->list_len=0;

  while(!cddb_read_line(sock,buffer,512))
  {
    if(cddb_sites_process_line(buffer,&list->list_host[list->list_len])!=-1)
      list->list_len++;
  }

  return 0;
}

/* Terminate the connection */
int cddb_quit(int sock)
{
  char outbuffer[8];

  strcpy(outbuffer, "quit\n");
  write(sock,outbuffer,strlen(outbuffer));
  close(sock);

  return 0;
}

/* Return the numerical value of a reply, allowing us to quickly check if
   anything went wrong */
int cddb_read_token(int sock,int token[3])
{
  char inbuffer[512];

  if(cddb_read_line(sock,inbuffer,512)<0)
    return -1;

  if(strncmp(inbuffer,"<!DOC",5)==0)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"404 CDDB CGI not found",256);
    return -1;
  }

  token[0]=inbuffer[0]-48;
  token[1]=inbuffer[1]-48;
  token[2]=inbuffer[2]-48;

  if(use_cddb_message)
    strncpy(cddb_message,(char *)inbuffer+4,256);

  return 0;
}

/* This is the function for completely automated CDDB operation */
int cddb_read_data(int cd_desc,struct disc_data *data)
{
  int sock=-1,index;
  char http_string[512];
  struct disc_info disc;
  struct cddb_entry entry;
  struct cddb_hello hello;
  struct cddb_query query;
  struct cddb_conf conf;
  struct cddb_server *proxy_ptr,proxy;
  struct cddb_serverlist list;
   
  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if(!disc.disc_present)
    return -1;

  cddb_read_serverlist(&conf,&list,&proxy);
  if(!conf.conf_access)
    return -1;

  if(!conf.conf_proxy)
    proxy_ptr = NULL;
  else
    proxy_ptr=&proxy;
   
  if(list.list_len<1)
    return -1;

  strncpy(hello.hello_program,PACKAGE,256);
  strncpy(hello.hello_version,VERSION,256);

  index=0;

  /* Connect to a server */
  do
  {
    switch(list.list_host[index].host_protocol)
    {
      case CDDB_MODE_CDDBP: 
	sock=cddb_connect_server(&list.list_host[index++],proxy_ptr,&hello);
	break;
      case CDDB_MODE_HTTP:
	sock=cddb_connect_server(&list.list_host[index++],proxy_ptr,&hello,http_string,512);
	break;
      case CDINDEX_MODE_HTTP:
	sock=cdindex_connect_server(&list.list_host[index++],proxy_ptr,http_string,512);
    }
  } while(index<list.list_len&&sock==-1);

  if(sock<0)
    return -1;

  index--;

  /* CDDB Query, not nessecary for CD Index operations */
  switch(list.list_host[index].host_protocol)
  {
    case CDDB_MODE_CDDBP:
      if(cddb_query(cd_desc,sock,CDDB_MODE_CDDBP,&query)<0)
        return -1;
      break;
    case CDDB_MODE_HTTP:
      if(cddb_query(cd_desc,sock,CDDB_MODE_HTTP,&query,http_string)<0)
        return -1;
      close(sock);

      /* We must now reconnect to execute the next command */
      if((sock=cddb_connect_server(&list.list_host[index],proxy_ptr,&hello,http_string,512))<0)
        return -1;
      break;
  }

  /* Since this is an automated operation, we'll assume inexact matches are
     correct. */

  entry.entry_id=query.query_list[0].list_id;
  entry.entry_genre=query.query_list[0].list_genre;

  /* Read operation */
  switch(list.list_host[index].host_protocol)
  {
    case CDDB_MODE_CDDBP:
      if(cddb_read(cd_desc,sock,CDDB_MODE_CDDBP,&entry,data)<0)
        return -1;
      cddb_quit(sock);
      break;
    case CDDB_MODE_HTTP:
      if(cddb_read(cd_desc,sock,CDDB_MODE_HTTP,&entry,data,http_string)<0)
        return -1;
      close(sock);
      break;
    case CDINDEX_MODE_HTTP:
      if(cdindex_read(cd_desc,sock,data,http_string)<0)
        return -1;
      close(sock);
      break;
   }
   
   return 0;
}

/* Generate an entry for when CDDB is disabled/not working */
int cddb_generate_unknown_entry(int cd_desc,struct disc_data *data)
{
  int index;
  struct disc_info disc;

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if((data->data_id=__internal_cddb_discid(&disc))<0)
    return -1;

  if(__internal_cdindex_discid(&disc,data->data_cdindex_id,CDINDEX_ID_SIZE)<0)
    return -1;

  strcpy(data->data_title,"");
  strcpy(data->data_artist,"");
  data->data_genre=CDDB_UNKNOWN;
  for(index=0;index<disc.disc_total_tracks;index++)
    strcpy(data->data_track[index].track_name,"");

  return 0;
}

/* Generate an entry using CDDB if it's available, or Unknowns if it's not */
int cddb_generate_new_entry(int cd_desc,struct disc_data *data)
{
  if(cddb_read_data(cd_desc,data)<0)
    cddb_generate_unknown_entry(cd_desc,data);

  return 0;
}

/* Read from the local database, using CDDB if there isn't an entry cached */
int cddb_read_disc_data(int cd_desc,struct disc_data *outdata)
{
  FILE *cddb_data;
  int index;
  char root_dir[PATH_MAX],file[PATH_MAX],inbuffer[256];
  struct disc_info disc;
  struct stat st;
  struct __unprocessed_disc_data data;

  if(getenv("HOME")==NULL)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  snprintf(root_dir,PATH_MAX,"%s/.cddb",getenv("HOME"));

  if(stat(root_dir,&st)<0)
  {
    if(errno!=ENOENT)
      return -1;
    else
    {
      cddb_generate_new_entry(cd_desc,outdata);
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

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if((data.data_id=__internal_cddb_discid(&disc))<0)
    return -1;

  if(cdindex_discid(cd_desc,data.data_cdindex_id,CDINDEX_ID_SIZE)<0)
    return -1;

  data.data_title_index=0;
  data.data_extended_index=0;
  for(index=0;index<disc.disc_total_tracks;index++)
  {
    data.data_track[index].track_name_index=0;
    data.data_track[index].track_extended_index=0;
  }

  for(index=0;index<12;index++)
  {
    snprintf(file,PATH_MAX,"%s/%s/%08lx",root_dir,cddb_genre(index),data.data_id);
    if(stat(file,&st)==0)
    {
      cddb_data=fopen(file,"r");
      while(!feof(cddb_data))
      {
	fgets(inbuffer,512,cddb_data);			   
	cddb_process_line(inbuffer,&data);
      }

      data.data_genre=index;
      fclose(cddb_data);

      data_format_input(outdata,&data,disc.disc_total_tracks);

      return 0;
    }
  }

  if(cddb_read_data(cd_desc,outdata)<0)
    cddb_generate_new_entry(cd_desc,outdata);

  return 0;
}
   
/* Write to the local cache */
int cddb_write_data(int cd_desc,const struct disc_data *indata)
{
  FILE *cddb_data;
  int index,tracks;
  char root_dir[PATH_MAX],genre_dir[PATH_MAX],file[PATH_MAX];
  struct stat st;
  struct disc_info disc;
  struct __unprocessed_disc_data data;

  if(getenv("HOME")==NULL)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  data_format_output(&data,indata,disc.disc_total_tracks);

  snprintf(root_dir,PATH_MAX,"%s/.cddb",getenv("HOME"));
  snprintf(genre_dir,PATH_MAX,"%s/%s",root_dir,cddb_genre(data.data_genre));
  snprintf(file,PATH_MAX,"%s/%08lx",genre_dir,data.data_id);

  if(stat(root_dir,&st)<0)
  {
    if(errno!=ENOENT)
      return -1;
    else if(mkdir(root_dir,0755)<0)
      return -1;
  }
  else
  {
    if(!S_ISDIR(st.st_mode))
    {
      errno=ENOTDIR;
      return -1;
    }   
  }

  if(stat(genre_dir,&st)<0)
  {
    if(errno!=ENOENT)
      return -1;
    else if(mkdir(genre_dir,0755)<0)
      return -1;
  }
  else
  {
    if(!S_ISDIR(st.st_mode))
    {
      errno=ENOTDIR;
      return -1;
    }
  }

  if((cddb_data=fopen(file,"w"))==NULL)
    return -1;

  fprintf(cddb_data,"# xmcd CD database file generated by %s %s\n",PACKAGE,VERSION);
  fputs("# \n",cddb_data);
  fputs("# Track frame offsets:\n",cddb_data);
  for(index=0;index<disc.disc_total_tracks;index++)
    fprintf(cddb_data,"#       %d\n",(disc.disc_track[index].track_pos.minutes*60+disc.disc_track[index].track_pos.seconds)*75+disc.disc_track[index].track_pos.frames);
  fputs("# \n",cddb_data);
  fprintf(cddb_data,"# Disc length: %d seconds\n",disc.disc_length.minutes*60+disc.disc_length.seconds);
  fputs("# \n",cddb_data);
  fprintf(cddb_data,"# Revision: %d\n",data.data_revision);
  fprintf(cddb_data,"# Submitted via: %s %s\n",PACKAGE,VERSION);
  fputs("# \n",cddb_data);
  fprintf(cddb_data,"DISCID=%08lx\n",data.data_id);
  for(index=0;index<data.data_title_index;index++)
    fprintf(cddb_data,"DTITLE=%s\n",data.data_title[index]);
  for(tracks=0;tracks<disc.disc_total_tracks;tracks++)
  {
    for(index=0;index<data.data_track[tracks].track_name_index;index++)
      fprintf(cddb_data,"TTITLE%d=%s\n",tracks,data.data_track[tracks].track_name[index]);
  }
  if(data.data_extended_index==0)
    fputs("EXTD=\n",cddb_data);
  else
  {
    for(index=0;index<data.data_extended_index;index++)
      fprintf(cddb_data,"EXTD=%s\n",data.data_extended[index]);
  }

  for(tracks=0;tracks<disc.disc_total_tracks;tracks++)
  {
    if(data.data_track[tracks].track_extended_index==0)
      fprintf(cddb_data,"EXTT%d=\n",tracks);
    else
    {
      for(index=0;index<data.data_track[tracks].track_extended_index;index++)
	fprintf(cddb_data,"EXTT%d=%s\n",tracks,data.data_track[tracks].track_extended[index]);
    }
  }

  fputs("PLAYORDER=",cddb_data);

  fclose(cddb_data);

  return 0;
}

int cddb_mc_read(int cd_desc,int sock,int mode,struct cddb_entry *entry,struct disc_mc_data *data,...)
{
  int ret;
  va_list arglist;
  struct disc_data indata;

  va_start(arglist, data);
  ret=cddb_vread(cd_desc,sock,mode,entry,&indata,arglist);
  va_end(arglist);

  if(ret<0)
    return ret;

  if(cddb_mc_alloc(cd_desc,data)<0)
    return -1;

  if(cddb_mc_copy_from_data(data,&indata)<0)
  {
    cddb_mc_free(data);
    return -1;
  }

  return ret;
}

int cddb_mc_read_disc_data(int cd_desc,struct disc_mc_data *data)
{
  struct disc_data indata;

  if(cddb_mc_alloc(cd_desc,data)<0)
    return -1;

  if(cddb_read_disc_data(cd_desc,&indata)<0)
  {
    cddb_mc_free(data);
    return -1;
  }

  if(cddb_mc_copy_from_data(data,&indata)<0)
  {
    cddb_mc_free(data);
    return -1;
  }

  return 0;
}

int cddb_mc_write_disc_data(int cd_desc,const struct disc_mc_data *data)
{
  struct disc_data outdata;

  if(cddb_data_copy_from_mc(&outdata,data)<0)
     return -1;

  if(cddb_write_data(cd_desc,&outdata)<0)
    return -1;

  return 0;
}

int cddb_mc_generate_new_entry(int cd_desc,struct disc_mc_data *data)
{
  struct disc_data indata;

  if(cddb_generate_new_entry(cd_desc,&indata)<0)
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

/* Delete an entry from the local cache based upon a data structure */
int cddb_direct_erase_data(int genre,unsigned long id)
{
  char root_dir[PATH_MAX],genre_dir[PATH_MAX],file[PATH_MAX];
  struct stat st;

  if(getenv("HOME")==NULL)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  snprintf(root_dir,PATH_MAX,"%s/.cddb",getenv("HOME"));
  snprintf(genre_dir,PATH_MAX,"%s/%s",root_dir,cddb_genre(genre));
  snprintf(file,PATH_MAX,"%s/%08lx",genre_dir,id);

  if(stat(root_dir,&st)<0)
  {
    if(errno!=ENOENT)
      return -1;
    else
      return 0;
  }
  else
  {
    if(!S_ISDIR(st.st_mode))
      return 0;  
  }

  if(stat(genre_dir,&st)<0)
  {
    if(errno!=ENOENT)
      return -1;
    else
      return 0;
  }
  else
  {
    if(!S_ISDIR(st.st_mode))
      return 0;
  }

  if(unlink(file)<0)
  {
    if(errno!=ENOENT)
      return -1;
  }

  return 0;
}

int cddb_erase_data(const struct disc_data *data)
{
  return cddb_direct_erase_data(data->data_genre,data->data_id);
}

/* Return the status of a CDDB entry */
int cddb_stat_disc_data(int cd_desc,struct cddb_entry *entry)
{
  int index;
  struct disc_info disc;
  struct stat st;
  char root_dir[PATH_MAX],file[PATH_MAX];

  if(getenv("HOME")==NULL)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if((entry->entry_id=__internal_cddb_discid(&disc))<0)
    return -1;

  if(cdindex_discid(cd_desc,entry->entry_cdindex_id,CDINDEX_ID_SIZE)<0)
    return -1;

  snprintf(root_dir,PATH_MAX,"%s/.cddb",getenv("HOME"));

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
      errno = ENOTDIR;
      return -1;
    }
  }

  for(index=0;index<12;index++)
  {
    snprintf(file,PATH_MAX,"%s/%s/%08lx",root_dir,cddb_genre(index),entry->entry_id);
    if(stat(file,&st)==0)
    {
      entry->entry_timestamp=st.st_mtime;
      entry->entry_present=1;
      entry->entry_genre=index;

      return 0;
    }
  }

  entry->entry_present=0;

  return 0;
}

/* Wrapper for HTTP CDDB query */
int cddb_http_query(int cd_desc,const struct cddb_host *host,const struct cddb_hello *hello,struct cddb_query *query)
{
  int sock;
  char http_string[512];

  if((sock=cddb_connect_server(host,NULL,hello,http_string,512))<0)
    return -1;

  if(cddb_query(cd_desc,sock,CDDB_MODE_HTTP,query,http_string)<0)
    return -1;

  close(sock);

  return 0;
}

/* Wrapper for HTTP CDDB query using a proxy */
int cddb_http_proxy_query(int cd_desc,const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello,struct cddb_query *query)
{
  int sock;
  char http_string[512];

  if((sock=cddb_connect_server(host,proxy,hello,http_string,512))<0)
    return -1;

  if(cddb_query(cd_desc,sock,CDDB_MODE_HTTP,query,http_string)<0)
    return -1;

  close(sock);

  return 0;
}

/* Wrapper for HTTP CDDB read */
int cddb_http_read(int cd_desc,const struct cddb_host *host,const struct cddb_hello *hello,const struct cddb_entry *entry,struct disc_data *data)
{
  int sock;
  char http_string[512];

  if((sock=cddb_connect_server(host,NULL,hello,http_string,512))<0)
    return -1;

  if(cddb_read(cd_desc,sock,CDDB_MODE_HTTP,entry,data,http_string)<0)
    return -1;

  close(sock);

  return 0;
}

/* Wrapper for HTTP CDDB read using a proxy */
int cddb_http_proxy_read(int cd_desc,const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello,const struct cddb_entry *entry,struct disc_data *data)
{
  int sock;
  char http_string[512];

  if((sock=cddb_connect_server(host,proxy,hello,http_string,512))<0)
    return -1;

  if(cddb_read(cd_desc,sock,CDDB_MODE_HTTP,entry,data,http_string)<0)
    return -1;

  close(sock);

  return 0;
}

int cddb_http_sites(int cd_desc,const struct cddb_host *host,const struct cddb_hello *hello,struct cddb_serverlist *list)
{
  int sock;
  char http_string[512];

  if((sock=cddb_connect_server(host,NULL,hello,http_string,512))<0)
    return -1;

  if(cddb_sites(cd_desc,CDDB_MODE_HTTP,list,http_string)<0)
    return -1;

  close(sock);

  return 0;
}

int cddb_http_proxy_sites(int cd_desc,const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello,struct cddb_serverlist *list)
{
  int sock;
  char http_string[512];

  if((sock=cddb_connect_server(host,proxy,hello,http_string,512))<0)
    return -1;

  if(cddb_sites(cd_desc,CDDB_MODE_HTTP,list,http_string)<0)
    return -1;

  close(sock);

  return 0;
}

int cddb_http_submit(int cd_desc,const struct cddb_host *host,struct cddb_server *proxy,char *email_address)
{
  FILE *cddb_entry;
  int sock,index,changed_artist=0,changed_track[MAX_TRACKS],token[3],error=0;
  char inbuffer[512],outbuffer[512],cddb_file[PATH_MAX],*home;
  struct stat st;
  struct cddb_entry entry;
  struct disc_info disc;
  struct disc_data data;
   
  if((home=getenv("HOME"))==NULL)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"$HOME is not set!",256);
    return -1;
  }

  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if(!disc.disc_present)
    return -1;

  if(cddb_stat_disc_data(cd_desc,&entry)<0)
    return -1;

  if(entry.entry_present)
  {
    if(cddb_read_disc_data(cd_desc,&data)<0)
      return -1;
  }
  else
  {
    if(use_cddb_message)
      strncpy(cddb_message,"No CDDB entry present in cache",256);
    return -1;
  }

  if(proxy!=NULL)
  {
    if((sock=cddb_connect(proxy))<0)
    {
      if(use_cddb_message)
	strncpy(cddb_message,strerror(errno),256);
      return -1;
    }
  }
  else
  {
    if((sock=cddb_connect(&host->host_server))<0)
    {
      if(use_cddb_message)
	strncpy(cddb_message,strerror(errno),256);
      return -1;
    }
  }

  if(strlen(data.data_title)<1||strcmp(data.data_title,"Unknown")==0)
  {
    if(use_cddb_message)
      strncpy(cddb_message,"Edit the disc title before submission.",256);
    return -1;
  }

  if(strcmp(data.data_artist,"Unknown")==0)
  {
    strncpy(data.data_artist,"",256);
    changed_artist = 1;
  }

  for(index=0;index<disc.disc_total_tracks;index++)
  {
    changed_track[index]=0;
    if(strcmp(data.data_track[index].track_name,"Unknown")==0)
    {
      snprintf(data.data_track[index].track_name,256,"Track %d",index);
      changed_track[index]=1;
    }
  }

  cddb_write_data(cd_desc,&data);

  if(cddb_submit_method==CDDB_SUBMIT_EMAIL)
  {
    snprintf(outbuffer,512,"cat %s/.cddb/%s/%08lx | mail -s \"cddb %s %08lx\" %s",home,cddb_genre(data.data_genre),data.data_id,cddb_genre(data.data_genre),data.data_id,cddb_submit_email_address);
    if(system(outbuffer) != 0)
      return -1;
    return 0;
  }

  if(proxy!=NULL)
    snprintf(outbuffer,512,"POST http://%s:%d%s HTTP/1.0\n",host->host_server.server_name,host->host_server.server_port,CDDB_HTTP_SUBMIT_CGI);
  else
    snprintf(outbuffer,512,"POST %s HTTP/1.0\n",CDDB_HTTP_SUBMIT_CGI);
  write(sock,outbuffer,strlen(outbuffer));

  snprintf(outbuffer,512,"Category: %s\n",cddb_genre(data.data_genre));
  write(sock,outbuffer,strlen(outbuffer));

  snprintf(outbuffer,512,"Discid: %08lx\n",data.data_id);
  write(sock,outbuffer,strlen(outbuffer));

  snprintf(outbuffer,512,"User-Email: %s\n",email_address);
  write(sock,outbuffer,strlen(outbuffer));

  snprintf(outbuffer,512,"Submit-Mode: %s\n",CDDB_SUBMIT_MODE?"submit":"test");
  write(sock,outbuffer,strlen(outbuffer));

  strncpy(outbuffer,"X-Cddbd-Note: Submission problems?  E-mail libcdaudio@gjhsnews.mesa.k12.co.us\n",512);
  write(sock, outbuffer, strlen(outbuffer));

  snprintf(cddb_file,PATH_MAX,"%s/.cddb/%s/%08lx",home,cddb_genre(data.data_genre),data.data_id);
  stat(cddb_file,&st);

  snprintf(outbuffer,512,"Content-Length: %d\n\n",(int)st.st_size);
  write(sock,outbuffer,strlen(outbuffer));

  cddb_entry=fopen(cddb_file,"r");
  while(!feof(cddb_entry))
  {
    fgets(outbuffer,512,cddb_entry);
    write(sock,outbuffer,strlen(outbuffer));
  }

  cddb_read_line(sock,inbuffer,512);
  if(strncmp(inbuffer+9,"200",3)!=0)
  {
    if(use_cddb_message)
      strncpy(cddb_message,inbuffer,256);
    return -1;
  }

  cddb_skip_http_header(sock);

  if(cddb_read_token(sock,token)<0)
    error = 1;

  if(token[0]!=2)
    error = 1;

  close(sock);

  if(changed_artist)
    strncpy(data.data_artist,"Unknown",256);

  for(index=0;index<disc.disc_total_tracks;index++)
  {
    if(changed_track[index])
      strncpy(data.data_track[index].track_name,"Unknown",256);
  }

  data.data_revision++;
  cddb_write_data(cd_desc, &data);

  if(error)
    return -1;

  return 0;
}
