#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "cdlyte.h"

int main(int argc,char **argv)
{
  int i;
  char http_string[BUFSIZ],version[BUFSIZ],*prog,*ver;
  cdsock_t sock;
  struct cddb_host host;
  struct cddb_hello hello;
  struct cddb_serverlist list;

  if(argc>1)
  {
    printf("usage: %s\n",argv[0]);
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

  printf ("Connecting to server: %s %d\n",host.host_server.server_name,host.host_server.server_port);
  if((sock=cddb_connect(&host,NULL,&hello,http_string,BUFSIZ))==INVALID_CDSOCKET)
  {
    printf("%s\n",cddb_message);
    return 0;
  }

  /* Get sites */
  printf("\nRequesting site list\n");
  if(cddb_sites(sock,host.host_protocol,&list,http_string)!=1)
  {
    goto quit;
  }
  printf("%s\n",cddb_message);

  printf("\nSites:\n");
  for(i=0;i<list.list_len;i++)
  {
    printf("%s %d %s %s %s %s\n",
           list.list_host[i].host_server.server_name,
           list.list_host[i].host_server.server_port,
           (list.list_host[i].host_protocol==CDDB_MODE_CDDBP)?"CDDBP":"HTTP",
           list.list_host[i].host_latitude,
           list.list_host[i].host_longitude,
           list.list_host[i].host_description);
  }

  cddb_quit(sock,host.host_protocol);

  return 0;

quit:
  printf("%s\n",cddb_message);
  cddb_quit(sock,host.host_protocol);

  return 0;
}
