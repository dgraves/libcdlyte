#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "cdlyte.h"

#ifdef WIN32
#define PATHSEP '\\'
#else
#define PATHSEP '/'
#endif

void print_usage(char *name)
{
  printf("usage: %s [-d device] [-e email_address] [server[:port]]\n",name);
}

int main(int argc,char **argv)
{
  int i;
  short port=8080;
  char device[BUFSIZ]="/dev/cdrom";
  char hostname[BUFSIZ]="127.0.0.1";
  char emailaddr[BUFSIZ]="user@localhost";
  char http_string[BUFSIZ],querystr[BUFSIZ],version[BUFSIZ],genre[BUFSIZ],*prog,*ver,*ptr;
  unsigned long discid;
  cddesc_t cd_desc;
  cdsock_t sock;
  struct disc_info disc;
  struct disc_data data;
  struct cddb_host host;
  struct cddb_hello hello;
  struct cddb_query query;

  if(argc>6)
  {
    print_usage(argv[0]);
    return 0;
  }

  for(i=1;i<argc;i++)
  {
    if(strcmp(argv[i],"-d")==0)
    {
      if(argv[++i][0]=='-')
      {
        print_usage(argv[0]);
        return 0;
      }
      strncpy(device,argv[i],BUFSIZ);
    }
    else if(strcmp(argv[i],"-e")==0)
    {
      if(argv[++i][0]=='-')
      {
        print_usage(argv[0]);
        return 0;
      }
      strncpy(emailaddr,argv[i],BUFSIZ);
    }
    else if(argv[i][0]!='-')
    {
      strncpy(hostname,argv[i],BUFSIZ);
      if((ptr=strrchr(hostname,':'))!=NULL)
      {
        *ptr++='\0';
        port=atoi(ptr);
      }
      else
        port=80;
    }
    else
    {
      print_usage(argv[0]);
      return 0;
    }

  }
  printf("Submitting as \"%s\" to \"%s\" on port %d\n",emailaddr,hostname,port);
      
  if((cd_desc=cd_init_device(device))==INVALID_CDDESC)
  {
     printf("Error opening device \"%s\": %d\n",device,errno);
      return 0;
  }
  printf("Processing disc in \"%s\"\n",device);

  /* Check for disc */
  if(cd_stat(cd_desc,&disc)<0)
  {
    printf("Error accessing CD-ROM drive: %d\n",errno);
    cd_finish(cd_desc);
    return 0;
  }

  if(!disc.disc_present)
  {
    printf("CD-ROM drive does not contain a disc\n");
    cd_finish(cd_desc);
    return 0;
  }


  /* First get data for disc from freedb server */
  printf("Retrieving data to be used in test submission:\n\n");

  if((discid=cddb_discid(cd_desc))==-1)
  {
    printf("Error computing CD ID\n");
    cd_finish(cd_desc);
    return 0;
  }

  /* Check for a file in current directory */
  if(cddb_read_local(".",discid,&data)==0)
  {
    printf("Read file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));

    printf("\nRead data:\n");
    printf("\tID\t\t= %08lx\n",data.data_id);
    printf("\tCategory\t= %s\n",cddb_category(data.data_category,genre,BUFSIZ));
    printf("\tRevision\t= %d\n",data.data_revision);
    printf("\tArtist\t\t= %s\n",data.data_artist);
    printf("\tTitle\t\t= %s\n",data.data_title);
    printf("\tYear\t\t= %s\n",data.data_year);
    printf("\tGenre\t\t= %s\n",data.data_genre);
    printf("\tExt Data\t= %s\n",data.data_extended);
    for(i=0;i<data.data_total_tracks;i++)
    {
      printf("\tTrack Title\t= %s\n",data.data_track[i].track_title);
      printf("\tTrack Ext Data\t= %s\n",data.data_track[i].track_extended);
    }
  }
  else
  {
    /* No file found, get data from server */
    if(cddb_query_string(cd_desc,querystr,BUFSIZ)==NULL)
    {
      printf("Error computing CDDB Query string\n");
      cd_finish(cd_desc);
      return 0;
    }

    /* Connect */
    strncpy(host.host_server.server_name,"freedb.freedb.org",256);
    host.host_server.server_port=80;
    host.host_protocol=CDDB_MODE_HTTP;
    strncpy(host.host_addressing,CDDB_HTTP_QUERY_CGI,256);

    cd_version(version,BUFSIZ);
    prog=version;
    ver=strchr(version,' ');
    *ver++='\0';
    strncpy(hello.hello_user,"anonymous",64);
    strncpy(hello.hello_hostname,"localhost",256);
    strncpy(hello.hello_program,prog,256);
    strncpy(hello.hello_version,ver,256);

    /* One transaction per socket.  Use cddb_http_query to avoid dealing with sockets */
    printf ("\nConnecting to server: %s %d\n",host.host_server.server_name,host.host_server.server_port);
    if((sock=cddb_connect(&host,NULL,&hello,http_string,BUFSIZ))==INVALID_CDSOCKET)
    {
      printf("%s\n",cddb_message);
      cd_finish(cd_desc);
      return 0;
    }

    /* Query */
    printf("\nSending query for disc id: %08lx\n",discid);
    if(cddb_query(querystr,sock,host.host_protocol,&query,http_string)!=1)
    {
      goto quit;
    }
    printf("%s\n",cddb_message);

    /* One transaction per socket.  Use cddb_http_query to avoid dealing with sockets */
    cddb_quit(sock,host.host_protocol);

    printf("\nRetrieved query:\n");
    printf("\tFound\t= %s\n",(query.query_match==QUERY_NOMATCH)?"No match":(query.query_match==QUERY_EXACT)?"Exact match(es)":"Inexact match(es)");
    printf("\tTotal\t= %d\n",query.query_matches);
    for(i=0;i<query.query_matches;i++)
      printf("\tMatch\t= %s %08lx %s %s\n",cddb_category(query.query_list[i].list_category,genre,BUFSIZ),query.query_list[i].list_id,query.query_list[i].list_artist,query.query_list[i].list_title);

    if(query.query_match!=QUERY_NOMATCH)
    {
      /* One transaction per socket.  Use cddb_http_query to avoid dealing with sockets */
      printf ("\nConnecting to server: %s %d\n",host.host_server.server_name,host.host_server.server_port);
      if((sock=cddb_connect(&host,NULL,&hello,http_string,BUFSIZ))==INVALID_CDSOCKET)
      {
        printf("%s\n",cddb_message);
        cd_finish(cd_desc);
        return 0;
      }

      printf("\nRetrieving first match for disc id: %08lx\n",query.query_list[0].list_id);
      if(cddb_read(query.query_list[0].list_category,query.query_list[0].list_id,sock,host.host_protocol,&data,http_string)!=1)
      {
        goto quit;
      }
      printf("%s\n",cddb_message);

      printf("\nRetrieved data:\n");
      printf("\tID\t\t= %08lx\n",data.data_id);
      printf("\tCategory\t= %s\n",cddb_category(data.data_category,genre,BUFSIZ));
      printf("\tRevision\t= %d\n",data.data_revision);
      printf("\tArtist\t\t= %s\n",data.data_artist);
      printf("\tTitle\t\t= %s\n",data.data_title);
      printf("\tYear\t\t= %s\n",data.data_year);
      printf("\tGenre\t\t= %s\n",data.data_genre);
      printf("\tExt Data\t= %s\n",data.data_extended);
      for(i=0;i<data.data_total_tracks;i++)
      {
        printf("\tTrack Title\t= %s\n",data.data_track[i].track_title);
        printf("\tTrack Ext Data\t= %s\n",data.data_track[i].track_extended);
      }
    }

    cddb_quit(sock,host.host_protocol);
  }
  cd_finish(cd_desc);

  /* Now try submitting the data */

  printf("\nExecuting test submission:\n\n");

  strncpy(host.host_server.server_name,hostname,256);
  host.host_server.server_port=port;
  host.host_protocol=CDDB_SUBMIT_HTTP;
  strncpy(host.host_addressing,CDDB_HTTP_SUBMIT_CGI,256);

  cd_version(version,BUFSIZ);
  prog=version;
  ver=strchr(version,' ');
  *ver++='\0';
  strncpy(hello.hello_user,"anonymous",64);
  strncpy(hello.hello_hostname,"localhost",256);
  strncpy(hello.hello_program,prog,256);
  strncpy(hello.hello_version,ver,256);

  printf("Connecting to \"%s\" on port %d\n",host.host_server.server_name,host.host_server.server_port);

  /* Run program in test mode.  */
  cddb_test_submit=1;
  cddb_submit(&host,NULL,&hello,&disc,&data,"CDDB submission test",emailaddr);

  printf("%s\n",cddb_message);

  return 0;

quit:
  printf("%s\n",cddb_message);
  cddb_quit(sock,host.host_protocol);
  cd_finish(cd_desc);

  return 0;
}
