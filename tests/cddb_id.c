#include <stdio.h>
#include <errno.h>
#include "cdlyte.h"

int main(int argc,char **argv)
{
  int len;
  char buffer[BUFSIZ];
  unsigned long discid;
  cddesc_t cd_desc;
  struct disc_info disc;

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
    return 0;
  }

  if(!disc.disc_present)
  {
    printf("CD-ROM drive does not contain a disc\n");
    cd_free_disc_info(&disc);
    return 0;
  }

  cd_free_disc_info(&disc);

  if((discid=cddb_discid(cd_desc))==0)
  {
    printf("Error computing CD ID\n");
    return 0;
  }

  len=BUFSIZ;
  if(cddb_query_string(cd_desc,buffer,&len)==NULL)
  {
    printf("Error computing CDDB Query string\n");
    return 0;
  }

  printf("\tCD ID: %08lx\n",discid);
  printf("\tCDDB Query string: %s\n",buffer);

  cd_finish(cd_desc);

  return 0;
}
