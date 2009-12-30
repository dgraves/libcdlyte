/*
Irix CD-ROM interface for libcdlyte
Copyright (C)1999 David Rose <David.R.Rose@disney.com>

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

#include <config.h>

/* The implementation of the Cdlyte functions under Irix are
   sufficiently different from everything else that it warrented created
   a totally separate version of this file.  */

/* We assume that an Irix compiler will always be ANSI, so we don't
   bother to mess around with the K&R-compatibility stuff in here. */

#ifdef IRIX_CDLYTE

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <dmedia/cdaudio.h>
#include "cdlyte.h"

/* Eliminated collisions with some macros defined in dmedia/cdaudio.h. */
#undef cd_open
#undef cd_play
#undef cd_play_track
#undef cd_play_track_abs
#undef cd_play_abs
#undef cd_readda
#undef cd_seek
#undef cd_seek_track
#undef cd_stop
#undef cd_eject
#undef cd_close
#undef cd_get_status
#undef cd_toggle_pause
#undef cd_get_track_info
#undef cd_get_volume
#undef cd_set_volume

/* We have to map ints to CDLYTE pointers, since the libcdlyte
   specs require an int cd device handle, but the Irix library gives us
   CDLYTE *. */

typedef struct
{
  CDLYTE *p;
  struct disc_info *disc;
} CDInfo;

#define MAXOPEN_CDDEVS 10
static CDInfo cddevs[MAXOPEN_CDDEVS];

/* Lock the disc in the tray and fill in its disc information.  This
   is an internal function. */
static int get_info(cddesc_t cd_desc)
{
  struct disc_info *disc;
  CDSTATUS status;
  CDTRACKINFO track;
  int num_tracks;
  int i;

  assert(cd_desc>=0&&cd_desc<MAXOPEN_CDDEVS);
  assert(cddevs[cd_desc].p!=NULL);

  if(cddevs[cd_desc].disc!=NULL)
  {
    /* The disc information is already available.  Simply return. */
    return 1;
  }

  disc=(struct disc_info *)malloc(sizeof(struct disc_info));

  CDpreventremoval(cddevs[cd_desc].p);

  if(!CDgetstatus(cddevs[cd_desc].p,&status))
  {
    free(disc);
    return 0;
  }

  num_tracks=status.last-status.first+1;

  disc->disc_present=(status.state!=CD_NODISC);
  switch (status.state)
  {
    case CD_READY:
      disc->disc_mode=CDLYTE_COMPLETED;
      break;
    case CD_PLAYING:
      disc->disc_mode=CDLYTE_PLAYING;
      break;
    case CD_PAUSED:
    case CD_STILL:
      disc->disc_mode=CDLYTE_PAUSED;
      break;
    case CD_NODISC:
    case CD_ERROR:
    case CD_CDROM:
    default:
      disc->disc_mode=CDLYTE_NOSTATUS;
      num_tracks=0;
      break;
  }

  disc->disc_track_time.minutes=status.min;
  disc->disc_track_time.seconds=status.sec;
  disc->disc_time.minutes=status.abs_min;
  disc->disc_time.seconds=status.abs_sec;
  disc->disc_length.minutes=status.total_min;
  disc->disc_length.seconds=status.total_sec;
  /*
  disc->disc_current_frame=
    CDmsftoframe(status.abs_min,status.abs_sec,status.abs_frame);
   */
  disc->disc_current_track=status.track;
  disc->disc_first_track=status.first;
  disc->disc_total_tracks=num_tracks;
  disc->disc_track=(struct track_info*)malloc(disc->disc_total_tracks*sizeof(struct track_info));

  /* Now fill in all the per-track data. */
  for(i=0;i<num_tracks;i++)
  {
    if(CDgettrackinfo(cddevs[cd_desc].p,i+status.first,&track))
    {
      disc->disc_track[i].track_length.minutes=track.total_min;
      disc->disc_track[i].track_length.seconds=track.total_sec;
      disc->disc_track[i].track_pos.minutes=track.start_min;
      disc->disc_track[i].track_pos.seconds=track.start_sec;
      disc->disc_track[i].track_pos.frames=track.start_frame;
      disc->disc_track[i].track_lba=(track.start_min*60+track.start_sec)*75+track.start_frame-150;
      CDmsftoframe(track.start_min,track.start_sec,track.start_frame);
      disc->disc_track[i].track_type=CDLYTE_TRACK_AUDIO;
    }
    else
    {
      /* Some error reading the track. */
      free(disc->disc_track);
      free(disc);
      return 0;
    }
  }

  cddevs[cd_desc].disc=disc;
  return 1;
}

/* Return the track number that contains the indicated frame on the
   disc.  This is an internal function. */
static int find_track(cddesc_t cd_desc,int frame)
{
  int i;
  struct disc_info *disc;

  get_info(cd_desc);
  disc=cddevs[cd_desc].disc;
  assert(disc!=NULL);

  for(i=1;i<disc->disc_total_tracks;i++)
  {
    if(disc->disc_track[i].track_pos.minutes*4500+disc->disc_track[i].track_pos.seconds*75+disc->disc_track[i].track_pos.frames>frame)
      return i;
  }

  return disc->disc_total_tracks;
}

/* Initialize the CD-ROM for playing audio CDs. */
cddesc_t cd_init_device(char *device_name)
{
  /* First, find the next available cd device handle.  This isn't
     thread safe. */

  cddesc_t next_cddev=0;
  while(next_cddev<MAXOPEN_CDDEVS&&cddevs[next_cddev].p!=NULL)
    next_cddev++;

  if(next_cddev>=MAXOPEN_CDDEVS)
  {
    /* No available devices. */
    return INVALID_CDDESC;
  }

  if(strcmp(device_name,"default")==0)
  {
    device_name=NULL;
  }

  cddevs[next_cddev].p=CDopen(device_name,"r");
  if(cddevs[next_cddev].p==NULL)
  {
    /* Failure opening. */
    return INVALID_CDDESC;
  }

  return next_cddev;
}

/* Close a device handle and free its resources. */
int cd_finish(cddesc_t cd_desc)
{
  assert(cd_desc>=0&&cd_desc<MAXOPEN_CDDEVS);
  assert(cddevs[cd_desc].p!=NULL);

  if(cddevs[cd_desc].disc!=NULL)
  {
    free(cddevs[cd_desc].disc->disc_track);
    free(cddevs[cd_desc].disc);
    cddevs[cd_desc].disc=NULL;
  }

  CDclose(cddevs[cd_desc].p);
  cddevs[cd_desc].p=NULL;
  return 0;
}

/* Update a CD status structure. */
int cd_stat(cddesc_t cd_desc,struct disc_info *disc)
{
  int size;

  if(cddevs[cd_desc].disc!=NULL)
  {
    free(cddevs[cd_desc].disc->disc_track);
    free(cddevs[cd_desc].disc);
    cddevs[cd_desc].disc=NULL;
  }

  if(!get_info(cd_desc))
  {
    /* We couldn't read the disc information for some reason. */
    return -1;
  }

  size=disc->disc_total_tracks*sizeof(struct track_info);
  disc->disc_track=(struct track_info*)malloc(size);
  memcpy(disc,cddevs[cd_desc].disc,sizeof(struct disc_info) - sizeof(struct track_info *));
  memcpy(disc.disc_track,cddevs[cd_desc].disc->disc_track,size);

  return 0;
}

/* Play frames from CD */
int cd_play_frames(cddesc_t cd_desc,int startframe,int endframe)
{
  int starttrack, endtrack;
  struct track_info *track;

  if (!get_info(cd_desc))
  {
    /* We couldn't read the disc information for some reason. */
    return -1;
  }

  /* Get the track that contains the indicating starting frame and
     ending frame. */
  starttrack=find_track(cd_desc,startframe);
  endtrack=find_track(cd_desc, endframe);

  track=&cddevs[cd_desc].disc->disc_track[starttrack-1];
  return cd_play_track_pos(cd_desc,starttrack,endtrack,
			   startframe-(track->track_pos.minutes*4500+
				       track->track_pos.seconds*75+
				       track->track_pos.frames));
}

/* Play starttrack at position pos to endtrack */
int cd_play_track_pos(cddesc_t cd_desc,int starttrack,int endtrack,int startpos)
{
  int m,s,f;

  if(!get_info(cd_desc))
  {
    /* We couldn't read the disc information for some reason. */
    return -1;
  }

  /* If we're playing strictly within a track, use CDplaytrackabs.
     Otherwise, use CDplayabs to play till the end of the CD.  Neither
     is exactly right, but we do the best we can. */

  if(starttrack==endtrack)
  {
    CDframetomsf(startpos,&m,&s,&f);
    if(CDplaytrackabs(cddevs[cd_desc].p,starttrack,m,s,f,1))
    {
      return 0;
    }
  }
  else
  {
    CDframetomsf(startpos+cddevs[cd_desc].disc->disc_track[starttrack-1].track_pos.minutes*4500+cddevs[cd_desc].disc->disc_track[starttrack-1].track_pos.seconds*75+cddevs[cd_desc].disc->disc_track[starttrack-1].track_pos.frames,&m,&s,&f);
    if(CDplayabs(cddevs[cd_desc].p,m,s,f,1))
    {
      return 0;
    }
  }

  return -1;
}

/* Play starttrack to endtrack */
int cd_play_track(cddesc_t cd_desc,int starttrack,int endtrack)
{
  return cd_play_track_pos(cd_desc,starttrack,endtrack,0);
}

/* Play starttrack at position pos to end of CD */
int cd_play_pos(cddesc_t cd_desc,int track,int startpos)
{
  return cd_play_track_pos(cd_desc,track,cddevs[cd_desc].disc->disc_total_tracks-1,startpos);
}

/* Play starttrack to end of CD */
int cd_play(cddesc_t cd_desc,int track)
{
  return cd_play_pos(cd_desc,track,0);
}

/* Stop the CD, if it is playing */
int cd_stop(cddesc_t cd_desc)
{
  assert(cd_desc>=0&&cd_desc<MAXOPEN_CDDEVS);
  assert(cddevs[cd_desc].p!=NULL);

  if(CDstop(cddevs[cd_desc].p))
  {
    return 0;
  }
  return -1;
}

/* Pause the CD */
int cd_pause(cddesc_t cd_desc)
{
  assert(cd_desc>=0&&cd_desc<MAXOPEN_CDDEVS);
  assert(cddevs[cd_desc].p!=NULL);

  if(CDtogglepause(cddevs[cd_desc].p))
  {
    return 0;
  }
  return -1;
}

/* Resume playing */
int cd_resume(cddesc_t cd_desc)
{
  assert(cd_desc>=0&&cd_desc<MAXOPEN_CDDEVS);
  assert(cddevs[cd_desc].p!=NULL);

  if(CDtogglepause(cddevs[cd_desc].p))
  {
    return 0;
  }
  return -1;
}

/* Eject the tray */
int cd_eject(cddesc_t cd_desc)
{
  assert(cd_desc>=0&&cd_desc<MAXOPEN_CDDEVS);
  assert(cddevs[cd_desc].p!=NULL);

  CDallowremoval(cddevs[cd_desc].p);
  if(cddevs[cd_desc].disc!=NULL)
  {
    free(cddevs[cd_desc].disc->disc_track);
    free(cddevs[cd_desc].disc);
    cddevs[cd_desc].disc=NULL;
  }

  if(CDeject(cddevs[cd_desc].p))
  {
    return 0;
  }
  return -1;
}

/* Close the tray */
int cd_close(cddesc_t cd_desc)
{
  errno=ENOSYS;
  return -1;
}

/* Return the current volume setting */
int cd_get_volume(cddesc_t cd_desc,struct disc_volume *vol)
{
  CDVOLUME volume;

  assert(cd_desc >= 0 && cd_desc < MAXOPEN_CDDEVS);
  assert(cddevs[cd_desc].p != NULL);

  if(CDgetvolume(cddevs[cd_desc].p,&volume))
  {
    vol->vol_front.left=volume.chan0;
    vol->vol_front.right=volume.chan1;
    vol->vol_back.left=volume.chan2;
    vol->vol_back.right=volume.chan3;
    return 0;
  }
  return -1;
}

/* Set the volume */
int cd_set_volume(cddesc_t cd_desc,struct disc_volume vol)
{
  CDVOLUME volume;

  assert(cd_desc>=0&&cd_desc<MAXOPEN_CDDEVS);
  assert(cddevs[cd_desc].p!=NULL);

  volume.chan0=vol.vol_front.left;
  volume.chan1=vol.vol_front.right;
  volume.chan2=vol.vol_back.left;
  volume.chan3=vol.vol_back.right;
  if(CDsetvolume(cddevs[cd_desc].p,&volume))
  {
    return 0;
  }
  return -1;
}

#endif  /* IRIX_CDLYTE */
