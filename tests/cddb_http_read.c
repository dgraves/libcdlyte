#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "cdplayer.h"

int main(int argc,char **argv)
{
  int i,len;
  char http_string[BUFSIZ],querystr[BUFSIZ],version[BUFSIZ],genre[BUFSIZ],*prog,*ver;
  unsigned long discid;
  cddesc_t cd_desc;
  cdsock_t sock;
  struct disc_info disc;
  struct disc_data data;
  struct cddb_host host;
  struct cddb_hello hello;
  struct cddb_query query;

  if(argc>2)
  {
    printf("usage: %s [device]\n",argv[0]);
    return 0;
  }

  if(argc==2)
  {
    if((cd_desc=cd_init_device(argv[1]))==INVALID_CDDESC)
    {
      printf("Error opening device \"%s\": %d\n",argv[1],errno);
      return 0;
    }
    printf("Processing disc in \"%s\"\n",argv[1]);
  }
  else
  {
    if((cd_desc=cd_init_device("/dev/cdrom"))==INVALID_CDDESC)
    {
      printf("Error opening device \"/dev/cdrom\": %d\n",errno);
      return 0;
    }
    printf("Processing disc in \"/dev/cdrom\"\n");
  }

  /* Check for disc */
  cd_init_disc_info(&disc);
  if(cd_stat(cd_desc,&disc)<0)
  {
    printf("Error accessing CD-ROM drive: %d\n",errno);
    cd_free_disc_info(&disc);
    cd_finish(cd_desc);
    return 0;
  }

  if(!disc.disc_present)
  {
    printf("CD-ROM drive does not contain a disc\n");
    cd_free_disc_info(&disc);
    cd_finish(cd_desc);
    return 0;
  }

  /* Done with disc_info */
  cd_free_disc_info(&disc);

  if((discid=cddb_discid(cd_desc))==0)
  {
    printf("Error computing CD ID\n");
    cd_finish(cd_desc);
    return 0;
  }

  len=BUFSIZ;
  if(cddb_query_string(cd_desc,querystr,&len)==NULL)
  {
    printf("Error computing CDDB Query string\n");
    cd_finish(cd_desc);
    return 0;
  }

  /* Connect */
  cddb_init_cddb_host(&host);
  host.host_server.server_name=cddb_strdup("freedb.freedb.org");
  host.host_server.server_port=80;
  host.host_protocol=CDDB_MODE_HTTP;
  host.host_addressing=cddb_strdup(CDDB_HTTP_QUERY_CGI);

  cd_version(version,BUFSIZ);
  prog=version;
  ver=strchr(version,' ');
  *ver++='\0';

  cddb_init_cddb_hello(&hello);
  hello.hello_user=cddb_strdup("anonymous");
  hello.hello_hostname=cddb_strdup("localhost");
  hello.hello_program=cddb_strdup(prog);
  hello.hello_version=cddb_strdup(ver);

  /* One transaction per socket.  Use cddb_http_query to avoid dealing with sockets */
  printf ("\nConnecting to server: %s %d\n",host.host_server.server_name,host.host_server.server_port);
  len=BUFSIZ;
  if((sock=cddb_connect(&host,NULL,&hello,http_string,&len))==INVALID_CDSOCKET)
  {
    printf("%s\n",cddb_message);
    cddb_free_cddb_hello(&hello);
    cddb_free_cddb_host(&host);
    cd_finish(cd_desc);
    return 0;
  }

  /* Initialize data objects */
  cddb_init_cddb_query(&query);
  cddb_init_disc_data(&data);

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
  {
    len=BUFSIZ;
    printf("\tMatch\t= %s %08lx %s %s\n",cddb_category(query.query_list[i].list_category,genre,&len),query.query_list[i].list_id,query.query_list[i].list_artist,query.query_list[i].list_title);
  }

  if(query.query_match!=QUERY_NOMATCH)
  {
    /* One transaction per socket.  Use cddb_http_query to avoid dealing with sockets */
    printf ("\nConnecting to server: %s %d\n",host.host_server.server_name,host.host_server.server_port);
    len=BUFSIZ;
    if((sock=cddb_connect(&host,NULL,&hello,http_string,&len))==INVALID_CDSOCKET)
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
    len=BUFSIZ;
    printf("\tCategory\t= %s\n",cddb_category(data.data_category,genre,&len));
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
  cddb_free_cddb_hello(&hello);
  cddb_free_cddb_host(&host);
  cddb_free_cddb_query(&query);
  cd_finish(cd_desc);

  return 0;

quit:
  printf("%s\n",cddb_message);
  cddb_quit(sock,host.host_protocol);
  cddb_free_cddb_hello(&hello);
  cddb_free_cddb_host(&host);
  cddb_free_cddb_query(&query);
  cd_finish(cd_desc);

  return 0;
}
