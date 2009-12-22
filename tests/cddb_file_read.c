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
  int i,len;
  char genre[BUFSIZ];
  cddesc_t cd_desc;
  struct disc_info disc;
  struct disc_data data;

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

  /* Get data */
  printf("\nReading file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));
  cddb_init_disc_data(&data);
  if(cddb_read_local(".",cddb_discid(cd_desc),&data)<0)
  {
    printf("Error reading file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));
    cd_finish(cd_desc);
    cd_free_disc_info(&disc);
    cddb_free_disc_data(&data);
    return 0;
  }

  printf("Read file .%c%08lx\n",PATHSEP,cddb_discid(cd_desc));

  printf("\nRead data:\n");
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

  cd_finish(cd_desc);
  cd_free_disc_info(&disc);
  cddb_free_disc_data(&data);
  return 0;
}
