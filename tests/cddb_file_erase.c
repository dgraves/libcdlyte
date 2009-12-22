#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "cdplayer.h"

#ifdef WIN32
#define PATHSEP '\\'
#define CAT     "TYPE"
#else
#define PATHSEP '/'
#define CAT     "cat"
#endif

int main(int argc,char **argv)
{
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
    cd_finish(cd_desc);
    cd_free_disc_info(&disc);
    return 0;
  }

  if(!disc.disc_present)
  {
    printf("CD-ROM drive does not contain a disc\n");
    cd_free_disc_info(&disc);
    cd_finish(cd_desc);
    return 0;
  }

  /* Remove file.  */
  printf("\nRemoving file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));
  if(cddb_erase_local(".",cddb_discid(cd_desc))<0)
    printf("Error removing file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));
  else
    printf("Successfully Removed file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));

  cd_finish(cd_desc);
  cd_free_disc_info(&disc);
  return 0;
}
