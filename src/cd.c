/*
This is part of the audio CD player library
Copyright (C)1998-99 Tony Arcieri <bascule@inferno.tusculum.edu>
Parts Copyright (C)1999 Quinton Dolan <q@OntheNet.com.au>
Copyright (C)2001-03 Dustin Graves <dgraves@computer.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <netinet/in.h>
#endif

#include "cdver.h"
#include "cdlyte.h"

#ifndef WIN32
/* We can check to see if the CD-ROM is mounted if this is available */
#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif

#ifdef HAVE_SYS_MNTENT_H
#include <sys/mntent.h>
#endif

#ifdef SOLARIS_GETMNTENT
#include <sys/mnttab.h>
#endif

#ifdef HAVE_SYS_UCRED_H
#include <sys/ucred.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

/* For Linux */
#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#define NON_BLOCKING
#endif

#ifdef HAVE_LINUX_UCDROM_H
#include <linux/ucdrom.h>
#endif

/* For FreeBSD, OpenBSD, and Solaris */
#ifdef HAVE_SYS_CDIO_H
#include <sys/cdio.h>
#endif

/* For Digital UNIX */
#ifdef HAVE_IO_CAM_CDROM_H
#include <io/cam/cdrom.h>
#endif

#endif

#include "compat.h"

/**
 * Return the name and version number of libcdlyte as a string.
 * @param buffer a character array to be filled with a ' ' separated string
 *        specifying the name and version of libcdlyte.
 * @param len an integer specifying the length of 'buffer'
 * @return a pointer to 'buffer'.
 */
char* cd_version(char *buffer,int len)
{
#ifndef WIN32
  snprintf(buffer,len,"%s %s",LIBCDLYTE_PACKAGE,LIBCDLYTE_VERSION);
#else
  _snprintf(buffer,len,"%s %s",LIBCDLYTE_PACKAGE,LIBCDLYTE_VERSION);
#endif
  return buffer;
}

/**
 * Return the version number of libcdlyte as an integer.
 * @return a long integer specifying the version of libcdlyte.
 */
long cd_getversion()
{
  return LIBCDLYTE_VERSION_NUMBER;
}

#if !defined IRIX_CDLYTE && !defined WIN32
/*
Because of Irix's different interface, most of this program is
completely ignored when compiling under Irix.
*/

/**
 * Initialize the CD-ROM for playing audio CDs.
 * @param device_name a string indicating the name of the device
 *        to be opened.  On UNIX systmes this is a device name (eg
 *        "/dev/cdrom") and on Windows systems this is the drive
 *        letter for the device (eg "D:").
 * @return a handle to the opened device if successful,
 *         INVALID_CDDESC if failed.  On failure, an error code of EBUSY
 *         indicates that the requested device can not be opened because it
 *         is currently mounted.
 */
cddesc_t cd_init_device(char *device_name)
{
  cddesc_t cd_desc;

#ifdef SOLARIS_GETMNTENT
  FILE *mounts;
  struct mnttab mnt;
#elif defined(HAVE_GETMNTENT)
  FILE *mounts;
  struct mntent *mnt;
#endif
#ifdef HAVE_GETMNTINFO
  int mounts;
  struct statfs *mnt;
#endif
  char devname[PATH_MAX];
  struct stat st;
  int len = 0;

  if(lstat(device_name,&st)<0)
    return INVALID_CDDESC;

  if(S_ISLNK(st.st_mode))
  {
    /* Some readlink implementations don't put a \0 on the end */
    len=readlink(device_name,devname,PATH_MAX);
    devname[len]='\0';
  }
  else
  {
    strncpy(devname,device_name,PATH_MAX);
    len=strlen(devname);
  }
#ifdef SOLARIS_GETMNTENT
  if((mounts=fopen(MNTTAB,"r"))==NULL)
    return INVALID_CDDESC;

  while(getmntent(mounts,&mnt)==0)
  {
    if(strncmp(mnt.mnt_fstype,devname,len)==0)
    {
      errno=EBUSY;
      return INVALID_CDDESC;
    }
  }

  if(fclose(mounts)!=0)
    return INVALID_CDDESC;
#elif defined(HAVE_GETMNTENT)
  if((mounts=setmntent(MOUNTED,"r"))==NULL)
    return INVALID_CDDESC;

  while((mnt=getmntent(mounts))!=NULL)
  {
    if(strncmp(mnt->mnt_fsname,devname,len)==0)
    {
      endmntent(mounts);
      errno=EBUSY;
      return INVALID_CDDESC;
    }
  }
  endmntent(mounts);
#endif
#ifdef HAVE_GETMNTINFO
  for((mounts=getmntinfo(&mnt,0));mounts>0;)
  {
    mounts--;
    if(strncmp(mnt[mounts].f_mntfromname,devname,len)==0)
    {
      errno=EBUSY;
      return INVALID_CDDESC;
    }
  }
#endif

#ifdef NON_BLOCKING
  if((cd_desc=open(device_name,O_RDONLY|O_NONBLOCK))<0)
#else
  if((cd_desc=open(device_name,O_RDONLY))<0)
#endif
    return INVALID_CDDESC;

  return cd_desc;
}

/**
 * Close a device handle and free its resources.
 * @param cd_desc the handle to the cd device to close.
 * @return 0 on success, -1 on failure.
 */
int cd_finish(cddesc_t cd_desc)
{
  close(cd_desc);
  return 0;
}

/** Read CD table of contents and get current status.
 * @param cd_desc the handle to the device to stat.
 * @param disc a disc_info structured to be filled with the current
 *        CD info and status values.  Memory resources allocated for
 *        disc must be freed with cd_free_disc_info.
 * @return 0 on success, -1 on failure.
 */
int cd_stat(cddesc_t cd_desc,struct disc_info *disc)
{
  /* Since every platform does this a little bit differently this gets pretty
     complicated... */

  struct CDLYTE_TOCHEADER cdth;
  struct CDLYTE_TOCENTRY cdte;
#ifdef CDLYTE_TOCENTRY_DATA
  struct CDLYTE_TOCENTRY_DATA cdte_buffer[MAX_TRACKS];
#endif

  struct disc_status status;
  int readtracks,pos;

  if(cd_poll(cd_desc,&status)<0)
    return -1;

  if(!status.status_present)
  {
    disc->disc_present=0;
    return 0;
  }

  /* Read the Table Of Contents header */
  if(ioctl(cd_desc,CDLYTE_READTOCHEADER,&cdth)<0)
    return -1;

  disc->disc_first_track=cdth.CDTH_STARTING_TRACK;
  disc->disc_total_tracks=cdth.CDTH_ENDING_TRACK;
  if(disc->disc_track!=NULL)
    free(disc->disc_track);
  disc->disc_track=(struct track_info*)calloc(disc->disc_total_tracks+1,sizeof(struct track_info));

#ifdef CDLYTE_READTOCENTRYS

  /* Read the Table Of Contents */
  cdte.CDTE_ADDRESS_FORMAT=CDLYTE_MSF_FORMAT;
  cdte.CDTE_STARTING_TRACK=0;
  cdte.CDTE_DATA=cdte_buffer;
  cdte.CDTE_DATA_LEN=sizeof(cdte_buffer);

  if(ioctl(cd_desc,CDLYTE_READTOCENTRYS,&cdte)<0)
  {
    free(disc->disc_track);
    disc->disc_track=NULL;
    return -1;
  }

  for(readtracks=0;readtracks<=disc->disc_total_tracks;readtracks++)
  {
    disc->disc_track[readtracks].track_pos.minutes=cdte.CDTE_DATA[readtracks].CDTE_MSF_M;
    disc->disc_track[readtracks].track_pos.seconds=cdte.CDTE_DATA[readtracks].CDTE_MSF_S;
    disc->disc_track[readtracks].track_pos.frames=cdte.CDTE_DATA[readtracks].CDTE_MSF_F;
    disc->disc_track[readtracks].track_type=(cdte.CDTE_DATA[readtracks].CDTE_CONTROL & CDROM_DATA_TRACK) ? CDLYTE_TRACK_DATA : CDLYTE_TRACK_AUDIO;
    disc->disc_track[readtracks].track_lba=cd_msf_to_lba(disc->disc_track[readtracks].track_pos);
  }

#else /* CDLYTE_READTOCENTRYS */

  for(readtracks=0;readtracks<=disc->disc_total_tracks;readtracks++)
  {
    cdte.CDTE_STARTING_TRACK=(readtracks==disc->disc_total_tracks)?CDROM_LEADOUT:readtracks+1;
    cdte.CDTE_ADDRESS_FORMAT=CDLYTE_MSF_FORMAT;
    if(ioctl(cd_desc,CDLYTE_READTOCENTRY,&cdte)<0)
    {
      free(disc->disc_track);
      disc->disc_track=NULL;
      return -1;
    }

    disc->disc_track[readtracks].track_pos.minutes=cdte.CDTE_MSF_M;
    disc->disc_track[readtracks].track_pos.seconds=cdte.CDTE_MSF_S;
    disc->disc_track[readtracks].track_pos.frames=cdte.CDTE_MSF_F;
    disc->disc_track[readtracks].track_type=(cdte.CDTE_CONTROL&CDROM_DATA_TRACK)?CDLYTE_TRACK_DATA:CDLYTE_TRACK_AUDIO;
    disc->disc_track[readtracks].track_lba=cd_msf_to_lba(&disc->disc_track[readtracks].track_pos);
  }

#endif /* CDLYTE_READTOCENTRY */

  for(readtracks=1;readtracks<=disc->disc_total_tracks;readtracks++)
  {
    pos=cd_msf_to_frames(&disc->disc_track[readtracks].track_pos)-
        cd_msf_to_frames(&disc->disc_track[readtracks - 1].track_pos);
    cd_frames_to_msf(&disc->disc_track[readtracks-1].track_length,pos);
  }

  disc->disc_length.minutes=disc->disc_track[disc->disc_total_tracks].track_pos.minutes;
  disc->disc_length.seconds=disc->disc_track[disc->disc_total_tracks].track_pos.seconds;
  disc->disc_length.frames=disc->disc_track[disc->disc_total_tracks].track_pos.frames;

  cd_update(disc,&status);

  return 0;
}

/** Get current CD status.
 * @param cd_desc the handle to the device to poll.
 * @param status a disc_status structured to be filled with the current
 *        CD status values.
 * @return 0 on success, -1 on failure.
 */
int cd_poll(cddesc_t cd_desc,struct disc_status *status)
{
  struct CDLYTE_SUBCHANNEL cdsc;
#ifdef CDLYTE_SUBCHANNEL_DATA
  struct CDLYTE_SUBCHANNEL_DATA cdsc_data;
#endif


#ifdef CDLYTE_SUBCHANNEL_DATA
  memset(&cdsc_data,'\0',sizeof(cdsc_data));
  cdsc.CDSC_DATA=&cdsc_data;
  cdsc.CDSC_DATA_LEN=sizeof(cdsc_data);
  cdsc.CDSC_DATA_FORMAT=CDLYTE_CDSC_DATA_FORMAT;
#endif
  cdsc.CDSC_ADDRESS_FORMAT=CDLYTE_MSF_FORMAT;

  if(ioctl(cd_desc,CDLYTE_READSUBCHANNEL,&cdsc)<0)
  {
    status->status_present=0;
    return 0;
  }

  status->status_present=1;

#ifdef CDLYTE_SUBCHANNEL_DATA
  status->status_disc_time.minutes=cdsc_data.CDSC_DATA_ABS_MSF_M;
  status->status_disc_time.seconds=cdsc_data.CDSC_DATA_ABS_MSF_S;
  status->status_disc_time.frames=cdsc_data.CDSC_DATA_ABS_MSF_F;
  status->status_track_time.minutes=cdsc_data.CDSC_DATA_REL_MSF_M;
  status->status_track_time.seconds=cdsc_data.CDSC_DATA_REL_MSF_S;
  status->status_track_time.frames=cdsc_data.CDSC_DATA_REL_MSF_F;
  status->status_current_track=cdsc_data.CDSC_DATA_TRK;
  switch(cdsc_data.CDSC_AUDIO_STATUS)
#else
  status->status_disc_time.minutes = cdsc.CDSC_ABS_MSF_M;
  status->status_disc_time.seconds = cdsc.CDSC_ABS_MSF_S;
  status->status_disc_time.frames = cdsc.CDSC_ABS_MSF_F;
  status->status_track_time.minutes = cdsc.CDSC_REL_MSF_M;
  status->status_track_time.seconds = cdsc.CDSC_REL_MSF_S;
  status->status_track_time.frames = cdsc.CDSC_REL_MSF_F;
  status->status_current_track = cdsc.CDSC_TRK;
  switch(cdsc.CDSC_AUDIO_STATUS)
#endif
  {
    case CDLYTE_AS_PLAY_IN_PROGRESS:
      status->status_mode=CDLYTE_PLAYING;
      break;
    case CDLYTE_AS_PLAY_PAUSED:
      status->status_mode=CDLYTE_PAUSED;
      break;
    case CDLYTE_AS_PLAY_COMPLETED:
      status->status_mode=CDLYTE_COMPLETED;
      break;
    default:
      status->status_mode=CDLYTE_NOSTATUS;
  }

  return 0;
}

/**
 * Play a range of frames from a CD.
 * @param cd_desc the handle to the device to play.
 * @param startfram an integer specifying the frame address at which to begin play.
 * @param endframe an integer specifying the frame address at which to end play.
 * @return 0 on success, -1 on failure.
 */
int cd_play_frames(int cd_desc,int startframe,int endframe)
{
  struct CDLYTE_MSF cdmsf;

#ifdef BROKEN_SOLARIS_LEADOUT
  endframe-=1;
#endif

  cdmsf.CDMSF_START_M=startframe/4500;
  cdmsf.CDMSF_START_S=(startframe%4500)/75;
  cdmsf.CDMSF_START_F=startframe%75;
  cdmsf.CDMSF_END_M=endframe/4500;
  cdmsf.CDMSF_END_S=(endframe%4500)/75;
  cdmsf.CDMSF_END_F=endframe%75;

  if(ioctl(cd_desc,CDLYTE_PLAY_MSF,&cdmsf)<0)
    return -1;

  return 0;
}

/**
 * Stop the CD.
 * @param cd_desc the handle to the device to stop.
 * @return 0 on success, -1 on failure.
 */
int cd_stop(cddesc_t cd_desc)
{
  if(ioctl(cd_desc,CDLYTE_STOP)<0)
    return -1;

  return 0;
}

/**
 * Pause the CD.
 * @param cd_desc the handle to the device to pause.
 * @return 0 on success, -1 on failure.
 */
int cd_pause(cddesc_t cd_desc)
{
  if(ioctl(cd_desc,CDLYTE_PAUSE)<0)
    return -1;

  return 0;
}

/**
 * Resume a paused CD.
 * @param cd_desc the handle to the device to resume.
 * @return 0 on success, -1 on failure.
 */
int cd_resume(int cd_desc)
{
  if(ioctl(cd_desc,CDLYTE_RESUME)<0)
    return -1;

  return 0;
}

/**
 * Eject the tray.
 * @param cd_desc the handle to the device to eject.
 * @return 0 on success, -1 on failure.
 */
int cd_eject(cddesc_t cd_desc)
{
  if(ioctl(cd_desc,CDLYTE_EJECT)<0)
    return -1;

  return 0;
}

/**
 * Close the tray.
 * @param cd_desc the handle to the device to close.
 * @return 0 on success, -1 on failure.
 */
int cd_close(int cd_desc)
{
#ifdef CDLYTE_CLOSE
  if(ioctl(cd_desc,CDLYTE_CLOSE)<0)
    return -1;

  return 0;
#else
  errno=ENOTTY;
  return -1;
#endif
}

/* Convert volume level from a byte to a percentage.  */
static float __internal_cd_get_volume_percentage(unsigned char level)
{
  return (float)level/255.0f;
}

/**
 * Return the current volume level.
 * @param cd_desc the handle to the device to query.
 * @param vol a disc_volume structure to fill with current volume level.
 *            The volume level is represented as a percentage (scale 0.0-1.0).
 * @return 0 on success, -1 on failure.
 */
int cd_get_volume(cddesc_t cd_desc,struct disc_volume *vol)
{
#ifdef CDLYTE_GET_VOLUME
  struct CDLYTE_VOLSTAT cdvol;
#ifdef CDLYTE_VOLSTAT_DATA
  struct CDLYTE_VOLSTAT_DATA cdvol_data;
  cdvol.CDVOLSTAT_DATA=&cdvol_data;
  cdvol.CDVOLSTAT_DATA_LEN=sizeof(cdvol_data);
#endif

  if(ioctl(cd_desc,CDLYTE_GET_VOLUME,&cdvol)<0)
    return -1;

  vol->vol_front.left=__internal_cd_get_volume_percentage(cdvol.CDVOLSTAT_FRONT_LEFT);
  vol->vol_front.right=__internal_cd_get_volume_percentage(cdvol.CDVOLSTAT_FRONT_RIGHT);
  vol->vol_back.left=__internal_cd_get_volume_percentage(cdvol.CDVOLSTAT_BACK_LEFT);
  vol->vol_back.right=__internal_cd_get_volume_percentage(cdvol.CDVOLSTAT_BACK_RIGHT);

#ifdef CDLYTE_VOLSTAT_DATA
  vol->vol_front.left=__internal_cd_get_volume_percentage(cdvol_data.CDVOLSTAT_FRONT_LEFT);
  vol->vol_front.right=__internal_cd_get_volume_percentage(cdvol_data.CDVOLSTAT_FRONT_RIGHT);
  vol->vol_back.left=__internal_cd_get_volume_percentage(cdvol_data.CDVOLSTAT_BACK_LEFT);
  vol->vol_back.right=__internal_cd_get_volume_percentage(cdvol_data.CDVOLSTAT_BACK_RIGHT);
#endif

  return 0;
#else
  errno = ENOTTY;
  return -1;
#endif
}

/* Convert volume level from a percentage to a byte.  */
static char __internal_cd_get_volume_val(float ratio)
{
  return (char)(255.0f*ratio);
}

/**
 * Set the volume level.
 * @param cd_desc the handle to the device to modify.
 * @param vol a disc_volume structure specifying the new volume level
 *            as a percentage (scale 0.0-1.0).
 * @return 0 on success, -1 on failure.
 */
int cd_set_volume(cddesc_t cd_desc,const struct disc_volume *vol)
{
#ifdef CDLYTE_SET_VOLUME
  struct CDLYTE_VOLCTRL cdvol;
#ifdef CDLYTE_VOLCTRL_DATA
  struct cd_playback cdvol_data;
  cdvol.CDVOLCTRL_DATA=&cdvol_data;
  cdvol.CDVOLCTRL_DATA_LEN=sizeof(cdvol_data);
#endif

  if(vol->vol_front.left>1.0f||vol->vol_front.left<0.0f||vol->vol_front.right>1.0f||vol->vol_front.right<0.0f||vol->vol_back.left>1.0f||vol->vol_back.left<0.0f||vol->vol_back.right>1.0f||vol->vol_back.right<0.0f)
    return -1;

  cdvol.CDVOLCTRL_FRONT_LEFT=__internal_cd_get_volume_val(vol->vol_front.left);
  cdvol.CDVOLCTRL_FRONT_RIGHT=__internal_cd_get_volume_val(vol->vol_front.right);
  cdvol.CDVOLCTRL_BACK_LEFT=__internal_cd_get_volume_val(vol->vol_back.left);
  cdvol.CDVOLCTRL_BACK_RIGHT=__internal_cd_get_volume_val(vol->vol_back.right);

#ifdef CDLYTE_VOLCTRL_SELECT
  cdvol_data.CDVOLCTRL_FRONT_LEFT_SELECT=CDLYTE_MAX_VOLUME;
  cdvol_data.CDVOLCTRL_FRONT_RIGHT_SELECT=CDLYTE_MAX_VOLUME;
  cdvol_data.CDVOLCTRL_BACK_LEFT_SELECT=CDLYTE_MAX_VOLUME;
  cdvol_data.CDVOLCTRL_BACK_RIGHT_SELECT=CDLYTE_MAX_VOLUME;
#endif

  if(ioctl(cd_desc,CDLYTE_SET_VOLUME,&cdvol)<0)
    return -1;

  return 0;
#else
  errno = ENOTTY;
  return -1;
#endif
}

#endif  /* IRIX_CDLYTE || WIN32*/

/*
 * Because all these functions are solely mathematical and/or only make
 * callbacks to previously existing functions they can be used for any
 * platform.
 */

/**
 * Convert frames to a logical block address.
 * @params frames an integer specifying the frame value to be converted.
 * @return the logical block address derived from 'frames'.
 */
int cd_frames_to_lba(int frames)
{
  if(frames>=150) return frames-150;
  return 0;
}

/**
 * Convert a logical block address to frames.
 * @param lba an integer specifying the lba value to be converted.
 * @return the frame value derived from 'lba'.
 */
int cd_lba_to_frames(int lba)
{
  return lba+150;
}

/**
 * Convert disc_timeval to frames.
 * @param time a disc_timeval structure to be coverted.
 * @return the frame value derived from 'time'.
 */
int cd_msf_to_frames(const struct disc_timeval *time)
{
  return time->minutes*4500+time->seconds*75+time->frames;
}

/**
 * Convert disc_timeval to a logical block address.
 * @param time a disc_timeval structure to be converted.
 * @return the logical block address derived from 'time'.
 */
int cd_msf_to_lba(const struct disc_timeval *time)
{
  if(cd_msf_to_frames(time)>150)
    return cd_msf_to_frames(time)-150;
  return 0;
}

/**
 * Convert frames to disc_timeval.
 * @param time a disc_timeval structure to be filled with time data
          derived from 'frames'.
 * @param frames an integer specifying the frame value to be converted.
 */
void cd_frames_to_msf(struct disc_timeval *time,int frames)
{
  time->minutes=frames/4500;
  time->seconds=(frames%4500)/75;
  time->frames=frames%75;
}

/**
 * Convert a logical block address to disc_timeval.
 * @param time a disc_timeval structure to be filled with time data
          derived from 'lba'.
 * @param lba  an integer specifying the logical block address to be converted.
 */
void cd_lba_to_msf(struct disc_timeval *time,int lba)
{
  cd_frames_to_msf(time, lba + 150);
}

/** Initialize disc_info structure for use with cd_stat.
 * @param disc a disc_info structure to be initialized for use with cd_stat.
 */
void cd_init_disc_info(struct disc_info *disc)
{
  disc->disc_total_tracks=0;
  disc->disc_track=NULL;
}

/** Free resources allocated for disc_info structure by cd_stat.
 * @param disc a disc_info structure with memory resources allocated by
 *        cd_stat to be freed.
 */
void cd_free_disc_info(struct disc_info *disc)
{
  if(disc->disc_track!=NULL)
  {
    free(disc->disc_track);
    disc->disc_track=NULL;
    disc->disc_total_tracks=0;
  }
}

/* Internal advance function.  */
static int __internal_cd_track_advance(cddesc_t cd_desc,const struct disc_info *disc,int endtrack,const struct disc_timeval *time)
{
  struct disc_info internal_disc;
  internal_disc.disc_track_time.minutes=disc->disc_track_time.minutes+time->minutes;
  internal_disc.disc_track_time.seconds=disc->disc_track_time.seconds+time->seconds;
  internal_disc.disc_track_time.frames=disc->disc_track_time.frames+time->frames;
  internal_disc.disc_current_track=disc->disc_current_track;

  if(internal_disc.disc_track_time.frames>74)
  {
    internal_disc.disc_track_time.frames-=74;
    internal_disc.disc_track_time.seconds++;
  }

  if(internal_disc.disc_track_time.frames<0)
  {
    internal_disc.disc_track_time.frames+=75;
    internal_disc.disc_track_time.seconds--;
  }

  if(internal_disc.disc_track_time.seconds>59)
  {
    internal_disc.disc_track_time.seconds-=59;
    internal_disc.disc_track_time.minutes++;
  }

  if(internal_disc.disc_track_time.seconds<0)
  {
    internal_disc.disc_track_time.seconds+=60;
    internal_disc.disc_track_time.minutes--;
  }

  if(internal_disc.disc_track_time.minutes<0)
  {
    internal_disc.disc_current_track--;
    if(internal_disc.disc_current_track==0)
      internal_disc.disc_current_track=1;

    return cd_play_track(cd_desc,internal_disc.disc_current_track,endtrack);
  }

  if((internal_disc.disc_track_time.minutes==internal_disc.disc_track[internal_disc.disc_current_track].track_pos.minutes&&internal_disc.disc_track_time.seconds>internal_disc.disc_track[internal_disc.disc_current_track].track_pos.seconds)||internal_disc.disc_track_time.minutes>internal_disc.disc_track[internal_disc.disc_current_track].track_pos.minutes)
  {
    internal_disc.disc_current_track++;
    if(internal_disc.disc_current_track>endtrack)
      internal_disc.disc_current_track=endtrack;

    return cd_play_track(cd_desc,internal_disc.disc_current_track,endtrack);
  }

  return cd_play_track_pos(cd_desc,internal_disc.disc_current_track,endtrack,internal_disc.disc_track_time.minutes*60+internal_disc.disc_track_time.seconds);
}

/**
 * Advance the position within a track while preserving an end track.
 * @param cd_desc the handle to the device to advance.
 * @param endtrack an integer specifying the track at which to end play.
 * @param time a disc_timeval structure specifying the time to advance
 *        to within the current track.
 * @return 0 on success, -1 on failure.
 */
int cd_track_advance(cddesc_t cd_desc,int endtrack,const struct disc_timeval *time)
{
  struct disc_info disc;

  cd_init_disc_info(&disc);
  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if(!disc.disc_present)
    return -1;

  if(__internal_cd_track_advance(cd_desc,&disc,endtrack,time)<0)
    return -1;

  cd_free_disc_info(&disc);
  return 0;
}

/**
 * Advance the position within a track without preserving the end track.
 * @param cd_desc the handle to the device to advance.
 * @param time a disc_timeval structure specifying the time to advance
 *        to within the current track.
 * @return 0 on success, -1 on failure.
 */
int cd_advance(cddesc_t cd_desc,const struct disc_timeval *time)
{
  struct disc_info disc;

  cd_init_disc_info(&disc);
  if(cd_stat(cd_desc,&disc)<0)
    return -1;

  if(__internal_cd_track_advance(cd_desc,&disc,disc.disc_total_tracks,time)<0)
    return -1;

  cd_free_disc_info(&disc);
  return 0;
}

/** Update information in a disc_info structure using a disc_status structure.
 * @param disc a disc_info structure to be updated.
 * @param status a disc_status structure containing current data to be copied
 *        into 'disc'.
 * @return 0 on success, -1 on failure.
 */
int cd_update(struct disc_info *disc,const struct disc_status *status)
{
  if(!(disc->disc_present=status->status_present))
    return -1;

  disc->disc_mode=status->status_mode;
  memcpy(&disc->disc_time,&status->status_disc_time,sizeof(struct disc_timeval));
  memcpy(&disc->disc_track_time,&status->status_track_time,sizeof(struct disc_timeval));

  disc->disc_current_track = 0;
  while((disc->disc_current_track<disc->disc_total_tracks)&&
        (cd_msf_to_frames(&disc->disc_time)>=cd_msf_to_frames(&disc->disc_track[disc->disc_current_track].track_pos)))
  {
    disc->disc_current_track++;
  }

  return 0;
}

/**
 * Play from a selected track to end of CD.
 * @param cd_desc the handle to the device to play.
 * @param track an integer indicating the track with which to begin play.
 * @return 0 on success, -1 on failure.
 */
int cd_play(int cd_desc,int track)
{
  return cd_playctl(cd_desc,PLAY_START_TRACK,track);
}

/**
 * Play from specified position within a selected track to end of CD.
 * @param cd_desc the handle to the device to play.
 * @param track an integer indicating the track with which to begin play.
 * @param startpos an integer specifying the position within 'track', in seconds, at which to begin play.
 * @return 0 on success, -1 on failure.
 */
int cd_play_pos(cddesc_t cd_desc,int track,int startpos)
{
  struct disc_timeval time;

  time.minutes=startpos/60;
  time.seconds=startpos%60;
  time.frames=0;

  return cd_playctl(cd_desc,PLAY_START_POSITION,track,&time);
}

/**
 * Play a range of tracks from a CD.
 * @param cd_desc the handle to the device to play.
 * @param starttrack an integer indicating the track with which to begin play.
 * @param endtrack an integer indicating the track with which to end play.
 * @return 0 on success, -1 on failure.
 */
int cd_play_track(cddesc_t cd_desc,int starttrack,int endtrack)
{
  return cd_playctl(cd_desc,PLAY_END_TRACK,starttrack,endtrack);
}

/**
 * Play a range of tracks, starting at a specified position within a
   track, from a CD.
 * @param cd_desc the handle to the device to play.
 * @param starttrack an integer indicating the track with which to begin play.
 * @param endtrack an integer indicating the track with which to end play.
 * @param startpos an integer specifying the position within 'starttrack',
          in seconds, at which to begin play.
 * @return 0 on success, -1 on failure.
 */
int cd_play_track_pos(cddesc_t cd_desc,int starttrack,int endtrack,int startpos)
{
  struct disc_timeval time;

  time.minutes=startpos/60;
  time.seconds=startpos%60;
  time.frames=0;

  return cd_playctl(cd_desc,PLAY_END_TRACK|PLAY_START_POSITION,starttrack,endtrack,&time);
}

/**
 * Universal play control function.
 * @param cd_desc the handle to the device to play.
 * @param options an integer specifying play options.  Options may be a
 *        combination of the following:
 *        <ul>
 *          <li> PLAY_START_TRACK - This is implied. If no other options
 *               are given this may be used in place of 0.
 *          <li> PLAY_END_TRACK - This is used to specify a track at which
 *               play shoule end.
 *          <li> PLAY_START_POSITION - This specifies a position (in the
 *               form of a struct disc_timeval address) after the
 *               start_track's beginning at which to begin play.
 *          <li> PLAY_END_POSITION - This specifies a position struct
 *               disc_timeval address) after the end_track's beginning
 *               at which to end play.
 *        </ul>
 * @param starttrack an integer specifying the track at which to begin play.
 * @param ... a variable length list of arguments specific to the options
 *        specified.
 * @return 0 on success,-1 on failure.
 */
int cd_playctl(cddesc_t cd_desc,int options,int starttrack,...)
{
  int end_track;
  struct disc_info disc;
  struct disc_timeval *startpos, *endpos, start_position, end_position;
  va_list arglist;

  cd_init_disc_info(&disc);
  
  if(cd_stat(cd_desc,&disc)<0)
  {
    cd_free_disc_info(&disc);
    return -1;
  }

  va_start(arglist,starttrack);

  if(options&PLAY_END_TRACK)
    end_track=va_arg(arglist,int);
  else
    end_track=disc.disc_total_tracks;

  if(options&PLAY_START_POSITION)
    startpos=va_arg(arglist,struct disc_timeval *);
  else
    startpos=0;

  if(options&PLAY_END_POSITION)
    endpos=va_arg(arglist,struct disc_timeval *);
  else
    endpos=0;

  va_end(arglist);

  if(options&PLAY_START_POSITION)
  {
    start_position.minutes=disc.disc_track[starttrack-1].track_pos.minutes+startpos->minutes;
    start_position.seconds=disc.disc_track[starttrack-1].track_pos.seconds+startpos->seconds;
    start_position.frames=disc.disc_track[starttrack-1].track_pos.frames+startpos->frames;
  }
  else
  {
    start_position.minutes=disc.disc_track[starttrack-1].track_pos.minutes;
    start_position.seconds=disc.disc_track[starttrack-1].track_pos.seconds;
    start_position.frames=disc.disc_track[starttrack-1].track_pos.frames;
  }

  if(options&PLAY_END_TRACK)
  {
    if(options&PLAY_END_POSITION)
    {
      end_position.minutes=disc.disc_track[end_track].track_pos.minutes+endpos->minutes;
      end_position.seconds=disc.disc_track[end_track].track_pos.seconds+endpos->seconds;
      end_position.frames=disc.disc_track[end_track].track_pos.frames+endpos->frames;
    }
    else
    {
      end_position.minutes=disc.disc_track[end_track].track_pos.minutes;
      end_position.seconds=disc.disc_track[end_track].track_pos.seconds;
      end_position.frames=disc.disc_track[end_track].track_pos.frames;
    }
  }
  else
  {
    end_position.minutes=disc.disc_track[disc.disc_total_tracks].track_pos.minutes;
    end_position.seconds=disc.disc_track[disc.disc_total_tracks].track_pos.seconds;
    end_position.frames=disc.disc_track[disc.disc_total_tracks].track_pos.frames;
  }

  cd_free_disc_info(&disc);

  return cd_play_frames(cd_desc,cd_msf_to_frames(&start_position),cd_msf_to_frames(&end_position));
}
