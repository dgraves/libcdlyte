#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "cdlyte.h"

int main(int argc,char **argv)
{
  int i,len;
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

  printf ("Connecting to server: %s %d\n",host.host_server.server_name,host.host_server.server_port);
  len=BUFSIZ;
  if((sock=cddb_connect(&host,NULL,&hello,http_string,&len))==INVALID_CDSOCKET)
  {
    printf("%s\n",cddb_message);
    cddb_free_cddb_host(&host);
    cddb_free_cddb_hello(&hello);
    return 0;
  }

  /* Get sites */
  printf("\nRequesting site list\n");
  cddb_init_cddb_serverlist(&list);
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

  cddb_free_cddb_host(&host);
  cddb_free_cddb_hello(&hello);
  cddb_free_cddb_serverlist(&list);

  return 0;

quit:
  printf("%s\n",cddb_message);
  cddb_quit(sock,host.host_protocol);

  cddb_free_cddb_host(&host);
  cddb_free_cddb_hello(&hello);
  cddb_free_cddb_serverlist(&list);

  return 0;
}
