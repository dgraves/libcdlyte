#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "cdlyte.h"

int main(int argc,char **argv)
{
  int i;
  char version[BUFSIZ],*prog,*ver;
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
  host.host_server.server_name=strdup("freedb.freedb.org");
  host.host_server.server_port=8880;
  host.host_protocol=CDDB_MODE_CDDBP;

  printf ("Connecting to server: %s %d\n",host.host_server.server_name,host.host_server.server_port);
  if((sock=cddb_connect(&host,NULL))==INVALID_CDSOCKET)
  {
    printf("%s\n",cddb_message);
    cddb_free_cddb_host(&host);
    return 0;
  }
  printf("%s\n",cddb_message);

  /* Initiaite */
  cd_version(version,BUFSIZ);
  prog=version;
  ver=strchr(version,' ');
  *ver++='\0';

  /* Initialize data objects */
  cddb_init_cddb_hello(&hello);
  cddb_init_cddb_serverlist(&list);

  hello.hello_user=strdup("anonymous");
  hello.hello_hostname=strdup("localhost");
  hello.hello_program=strdup(prog);
  hello.hello_version=strdup(ver);

  printf ("\nInitiating connection: %s %s %s %s\n",hello.hello_user,hello.hello_hostname,hello.hello_program,hello.hello_version);
  if(cddb_handshake(sock,&hello)!=1)
  {
    goto quit;
  }
  printf("%s\n",cddb_message);

  printf("\nSetting protocol level to %d\n",CDDB_PROTOCOL_LEVEL);
  if(cddb_proto(sock)!=1)
  {
    goto quit;
  }
  printf("%s\n",cddb_message);

  /* Get sites */
  printf("\nRequesting site list\n");
  if(cddb_sites(sock,host.host_protocol,&list)!=1)
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
  printf("%s\n",cddb_message);
  cddb_free_cddb_host(&host);
  cddb_free_cddb_hello(&hello);
  cddb_free_cddb_serverlist(&list);

  return 0;

quit:
  printf("%s\n",cddb_message);
  cddb_quit(sock,host.host_protocol);
  printf("%s\n",cddb_message);
  cddb_free_cddb_host(&host);
  cddb_free_cddb_hello(&hello);
  cddb_free_cddb_serverlist(&list);

  return 0;
}
