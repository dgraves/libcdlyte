#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "cdlyte.h"

#ifdef WIN32
#define PATHSEP '\\'
#else
#define PATHSEP '/'
#endif

int main(int argc,char **argv)
{
  char version[BUFSIZ],*prog,*ver;
  cddesc_t cd_desc;
  struct disc_info disc;
  struct disc_data data;
  struct cddb_host host;
  struct cddb_hello hello;

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

  /* Get data */
  strncpy(host.host_server.server_name,"freedb.freedb.org",256);
  host.host_server.server_port=8880;
  host.host_protocol=CDDB_MODE_CDDBP;

  cd_version(version,BUFSIZ);
  prog=version;
  ver=strchr(version,' ');
  *ver++='\0';
  strncpy(hello.hello_user,"anonymous",64);
  strncpy(hello.hello_hostname,"localhost",256);
  strncpy(hello.hello_program,prog,256);
  strncpy(hello.hello_version,ver,256);

  /* Retrieve data for disc in device */
  printf("\nRetrieveing data for disc id: %08lx\n",cddb_discid(cd_desc));
  if(cddb_read_data(cd_desc,&host,NULL,&hello,&data)!=1)
  {
    goto quit;
  }
  printf("Retrieved data\n",cddb_message);
  
  /* Write to current directory; disc_info was retrieved above */
  printf("\nWriting file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));
  if(cddb_write_local(".",&hello,&disc,&data,"[cdlyte write test]")<0)
    printf("Error writing file ./%c%08lx\n",PATHSEP,cddb_discid(cd_desc));
  else
    printf("Wrote file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));

  cd_finish(cd_desc);
  return 0;

quit:
  printf("%s\n",cddb_message);
  cd_finish(cd_desc);

  return 0;
}
