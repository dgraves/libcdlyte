#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "cdlyte.h"

#ifdef WIN32
#define PATHSEP '\\'
#define CAT     "TYPE"
#else
#define PATHSEP '/'
#define CAT     "cat"
#endif

int main(int argc,char **argv)
{
  char version[BUFSIZ],*prog,*ver,cmd[BUFSIZ];
  cddesc_t cd_desc;
  struct disc_info disc;
  struct disc_data data;
  struct cddb_host host;
  struct cddb_hello hello;
  struct timeval pause;

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
    cd_finish(cd_desc);
    cd_free_disc_info(&disc);
    return 0;
  }

  if(!disc.disc_present)
  {
    printf("CD-ROM drive does not contain a disc\n");
    cd_finish(cd_desc);
    cd_free_disc_info(&disc);
    return 0;
  }

  cd_free_disc_info(&disc);

  /* Get data */
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

  /* Retrieve data for disc in device */
  printf("\nRetrieving data for disc id: %08lx\n",cddb_discid(cd_desc));
  cddb_init_disc_data(&data);
  if(cddb_read_data(cd_desc,&host,NULL,&hello,&data)!=1)
  {
    printf("%s\n",cddb_message);
    cd_finish(cd_desc);
    cddb_free_cddb_host(&host);
    cddb_free_cddb_hello(&hello);
    cddb_free_disc_data(&data);
    return 0;
  }
  printf("Retrieved data\n",cddb_message);

  /* Write to current directory; disc_info was retrieved above */
  printf("\nWriting file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));
  if(cddb_write_local(".",&hello,&disc,&data,"[cdlyte write test]")<0)
  {
    printf("Error writing file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));
    cd_finish(cd_desc);
    cddb_free_cddb_host(&host);
    cddb_free_cddb_hello(&hello);
    cddb_free_disc_data(&data);
    return 0;
  }
  else
    printf("Wrote file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));

  snprintf(cmd,BUFSIZ,"%s .%c%08lx",CAT,PATHSEP,cddb_discid(cd_desc));
  printf("\nExecuting \"%s\"\n",cmd);

  /* Slight pause. */
  pause.tv_sec=2;
  select(0,NULL,NULL,NULL,&pause);

  system(cmd);

  cd_finish(cd_desc);
  cddb_free_cddb_host(&host);
  cddb_free_cddb_hello(&hello);
  cddb_free_disc_data(&data);
  return 0;
}
