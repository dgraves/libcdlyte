/*
Windows CD-ROM interface for libcdlyte
Copyright (C)2001,2004,2010 Dustin Graves <dgraves@computer.org>

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

#ifdef WIN32

#include "cdlyte.h"
#include <windows.h>
#include <mmsystem.h>
#include <memory.h>
#include <stdlib.h>
#include <ctype.h>

/* Information required for proper operation.  */
#define MAXDRIVES 26 /* All the letters in the alphabet.  */
#define MSF_TO_FRAMES(m,s,f) ((m*4500)+(s*75)+f)

static struct cdrom_t
{
  MCIDEVICEID cdrom_id;
  int cdrom_paused;
} *cdrom[MAXDRIVES]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                     NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

/* Mixer info for volume control.  */
static int mixref=0;
static HMIXER mixer;
static MIXERCONTROL mxc;
static MIXERLINE mxl;
static MIXERLINECONTROLS mxlc;

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
  int cd_desc;
  MCI_OPEN_PARMS params;
  MCI_SET_PARMS set;

  params.lpstrDeviceType=(LPCSTR)MCI_DEVTYPE_CD_AUDIO;
  params.lpstrElementName=device_name;
  if(mciSendCommand(0,MCI_OPEN,MCI_OPEN_SHAREABLE|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID|MCI_OPEN_ELEMENT,(DWORD)(LPVOID)&params)!=0)
    return INVALID_CDDESC;

  cd_desc=tolower(device_name[0])-'a';
  cdrom[cd_desc]=(struct cdrom_t*)malloc(sizeof(struct cdrom_t));
  cdrom[cd_desc]->cdrom_id=params.wDeviceID;
  cdrom[cd_desc]->cdrom_paused=0;

  set.dwTimeFormat=MCI_FORMAT_MSF;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_SET,MCI_SET_TIME_FORMAT,(DWORD)(LPVOID)&set)!=0)
  {
    mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_CLOSE,0,0);
    return INVALID_CDDESC;
  }

  // All CD devices will share the same mixer
  if(mixer==NULL)
  {
    if(mixerOpen(&mixer,0,0,0,0)==MMSYSERR_NOERROR)
    {
      memset(&mxl,0,sizeof(mxl));
      mxl.cbStruct=sizeof(mxl);
      mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;
      if(mixerGetLineInfo((HMIXEROBJ)mixer,&mxl,MIXER_GETLINEINFOF_COMPONENTTYPE)==MMSYSERR_NOERROR)
      {
        memset(&mxc,0,sizeof(mxc));
        mxc.cbStruct = sizeof(mxc);

        memset(&mxlc,0,sizeof(mxlc));
        mxlc.cbStruct=sizeof(mxlc);
        mxlc.dwLineID=mxl.dwLineID;
        mxlc.dwControlType=MIXERCONTROL_CONTROLTYPE_VOLUME;
        mxlc.cControls=1;
        mxlc.cbmxctrl=sizeof(mxc);
        mxlc.pamxctrl=&mxc;
        if(mixerGetLineControls((HMIXEROBJ)mixer,&mxlc,MIXER_GETLINECONTROLSF_ONEBYTYPE)!=MMSYSERR_NOERROR)
        {
          mixerClose(mixer);
          mixer=NULL;
        }
      }
      else
      {
        mixerClose(mixer);
        mixer=NULL;
      }
    }
    else
      mixer=NULL;
  }

  if(mixer)
    mixref++;

  return cd_desc;
}

/**
 * Close a device handle and free its resources.
 * @param cd_desc the handle to the cd device to close.
 * @return 0 on success, -1 on failure.
 */
int cd_finish(cddesc_t cd_desc)
{
  if(cdrom[cd_desc]==NULL)
    return -1;

  if(mixer&&--mixref==0)
  {
    mixerClose(mixer);
    mixer=NULL;
  }

  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_CLOSE,0,0)!=0)
    return -1;

  free(cdrom[cd_desc]);
  cdrom[cd_desc]=NULL;

  return 0;
}

/** Read CD table of contents and get current status.
 * @param cd_desc the handle to the device to stat.
 * @param disc a disc_info structured to be filled with the current
 *        CD info and status values.
 * @return 0 on success, -1 on failure.
 */
int cd_stat(cddesc_t cd_desc,struct disc_info *disc)
{
  int readtracks;
  struct disc_status status;
  MCI_STATUS_PARMS msp;

  if(cd_poll(cd_desc,&status)<0)
    return -1;

  if(!status.status_present)
  {
    disc->disc_present=0;
    return 0;
  }

  /* Read the number of tracks.  */
  msp.dwItem=MCI_STATUS_NUMBER_OF_TRACKS;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT,(DWORD)(LPVOID)&msp)!=0)
    return -1;
  disc->disc_first_track=1;
  disc->disc_total_tracks=msp.dwReturn;
  if(disc->disc_track!=NULL)
    free(disc->disc_track);
  disc->disc_track=(struct track_info*)malloc((disc->disc_total_tracks+1)*sizeof(struct track_info));

  for(readtracks=0;readtracks<disc->disc_total_tracks;readtracks++)
  {
    /* Get offsets.  */
    msp.dwTrack=readtracks+1;
    msp.dwItem=MCI_STATUS_POSITION;
    if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT|MCI_TRACK,(DWORD)(LPVOID)&msp)!=0)
      return -1;

    disc->disc_track[readtracks].track_pos.minutes=MCI_MSF_MINUTE(msp.dwReturn);
    disc->disc_track[readtracks].track_pos.seconds=MCI_MSF_SECOND(msp.dwReturn);
    disc->disc_track[readtracks].track_pos.frames=MCI_MSF_FRAME(msp.dwReturn);
    disc->disc_track[readtracks].track_lba=cd_msf_to_lba(&disc->disc_track[readtracks].track_pos);

    /* Get length.  */
    msp.dwTrack=readtracks+1;
    msp.dwItem=MCI_STATUS_LENGTH;
    if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT|MCI_TRACK,(DWORD)(LPVOID)&msp)!=0)
      return -1;

    if(readtracks+1==disc->disc_total_tracks)
    {
      /* Fix broken MCI last track length.  */
      cd_frames_to_msf(&disc->disc_track[readtracks].track_length,MSF_TO_FRAMES(MCI_MSF_MINUTE(msp.dwReturn),MCI_MSF_SECOND(msp.dwReturn),MCI_MSF_FRAME(msp.dwReturn))+1);
    }
    else
    {
      disc->disc_track[readtracks].track_length.minutes=MCI_MSF_MINUTE(msp.dwReturn);
      disc->disc_track[readtracks].track_length.seconds=MCI_MSF_SECOND(msp.dwReturn);
      disc->disc_track[readtracks].track_length.frames=MCI_MSF_FRAME(msp.dwReturn);
    }

    msp.dwTrack=readtracks+1;
    msp.dwItem=MCI_CDA_STATUS_TYPE_TRACK;
    if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT|MCI_TRACK,(DWORD)(LPVOID)&msp)!=0)
      return -1;
    disc->disc_track[readtracks].track_type=(msp.dwReturn==MCI_CDA_TRACK_AUDIO)?CDLYTE_TRACK_AUDIO:CDLYTE_TRACK_DATA;
  }

  /* Now calculate leadout. */
  cd_frames_to_msf(&disc->disc_track[disc->disc_total_tracks].track_pos,
                   cd_msf_to_frames(&disc->disc_track[disc->disc_total_tracks-1].track_pos)+
                   cd_msf_to_frames(&disc->disc_track[disc->disc_total_tracks-1].track_length));

  disc->disc_track[disc->disc_total_tracks].track_length.minutes=0;
  disc->disc_track[disc->disc_total_tracks].track_length.seconds=0;
  disc->disc_track[disc->disc_total_tracks].track_length.frames=0;

  disc->disc_track[disc->disc_total_tracks].track_lba=cd_msf_to_lba(&disc->disc_track[readtracks].track_pos);
  disc->disc_track[disc->disc_total_tracks].track_type=CDLYTE_TRACK_AUDIO;

  cd_frames_to_msf(&disc->disc_length,cd_msf_to_frames(&disc->disc_track[disc->disc_total_tracks].track_pos));

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
  int position;
  MCI_STATUS_PARMS msp;

  memset(&msp,0,sizeof(msp));

  /* Check for media in drive.  */
  msp.dwItem=MCI_STATUS_MEDIA_PRESENT;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT,(DWORD)(LPVOID)&msp)!=0)
    return -1;

  if(!msp.dwReturn)
  {
    status->status_present=0;
    return 0;
  }

  status->status_present=1;

  /* Get device state.  */
  msp.dwItem=MCI_STATUS_MODE;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT,(DWORD)(LPVOID)&msp)!=0)
    return -1;
  switch(msp.dwReturn)
  {
    case MCI_MODE_PLAY:
      status->status_mode=CDLYTE_PLAYING;
      break;
    case MCI_MODE_PAUSE:
      status->status_mode=CDLYTE_PAUSED;
      break;
    case MCI_MODE_STOP:
      status->status_mode=cdrom[cd_desc]->cdrom_paused?CDLYTE_PAUSED:CDLYTE_NOSTATUS;
      break;
    default:
      status->status_mode=CDLYTE_NOSTATUS;
  }

  /* Get current position.  */
  msp.dwItem=MCI_STATUS_POSITION;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT,(DWORD)(LPVOID)&msp)!=0)
    return -1;
  position=MSF_TO_FRAMES(MCI_MSF_MINUTE(msp.dwReturn),MCI_MSF_SECOND(msp.dwReturn),MCI_MSF_FRAME(msp.dwReturn));

  /* Get current track.  */
  msp.dwItem=MCI_STATUS_CURRENT_TRACK;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT,(DWORD)(LPVOID)&msp)!=0)
    return -1;
  status->status_current_track=msp.dwReturn;

  /* Get offset for current track.  */
  msp.dwTrack=status->status_current_track;
  msp.dwItem=MCI_STATUS_POSITION;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT|MCI_TRACK,(DWORD)(LPVOID)&msp)!=0)
    return -1;

  /* Current disc and track times.  */
  cd_frames_to_msf(&status->status_disc_time,position);
  cd_frames_to_msf(&status->status_track_time,position-MSF_TO_FRAMES(MCI_MSF_MINUTE(msp.dwReturn),MCI_MSF_SECOND(msp.dwReturn),MCI_MSF_FRAME(msp.dwReturn)));

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
  int tracks, leadout;
  MCI_STATUS_PARMS msp;
  MCI_PLAY_PARMS mciPlayParms;

  if(cdrom[cd_desc]==NULL)
    return -1;

  /* Calculate end position to fix broken leadout */
  msp.dwItem=MCI_STATUS_NUMBER_OF_TRACKS;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT,(DWORD)(LPVOID)&msp)!=0)
    return -1;
  tracks=msp.dwReturn;

  msp.dwTrack=tracks;
  msp.dwItem=MCI_STATUS_POSITION;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT|MCI_TRACK,(DWORD)(LPVOID)&msp)!=0)
    return -1;
  leadout=MSF_TO_FRAMES(MCI_MSF_MINUTE(msp.dwReturn),MCI_MSF_SECOND(msp.dwReturn),MCI_MSF_FRAME(msp.dwReturn));

  msp.dwTrack=tracks;
  msp.dwItem=MCI_STATUS_LENGTH;
  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT|MCI_TRACK,(DWORD)(LPVOID)&msp)!=0)
    return -1;
  leadout+=MSF_TO_FRAMES(MCI_MSF_MINUTE(msp.dwReturn),MCI_MSF_SECOND(msp.dwReturn),MCI_MSF_FRAME(msp.dwReturn))+1;

  if(leadout==endframe)
    endframe--;

  mciPlayParms.dwFrom=MCI_MAKE_MSF(startframe/4500,(startframe%4500)/75,startframe%75);
  mciPlayParms.dwTo=MCI_MAKE_MSF(endframe/4500,(endframe%4500)/75,endframe%75);

  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_PLAY,MCI_FROM|MCI_TO,(DWORD)(LPVOID)&mciPlayParms)!=0)
    return -1;

  cdrom[cd_desc]->cdrom_paused=0;
  return 0;
}

/**
 * Stop the CD.
 * @param cd_desc the handle to the device to stop.
 * @return 0 on success, -1 on failure.
 */
int cd_stop(cddesc_t cd_desc)
{
  if(cdrom[cd_desc]==NULL)
    return -1;

  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_STOP,0,0)!=0)
    return -1;

  cdrom[cd_desc]->cdrom_paused=0;
  return 0;
}

/**
 * Pause the CD.
 * @param cd_desc the handle to the device to pause.
 * @return 0 on success, -1 on failure.
 */
int cd_pause(cddesc_t cd_desc)
{
  if(cdrom[cd_desc]==NULL)
    return -1;

  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_PAUSE,0,0)!=0)
    return -1;

  cdrom[cd_desc]->cdrom_paused=1;
  return 0;
}

/**
 * Resume a paused CD.
 * @param cd_desc the handle to the device to resume.
 * @return 0 on success, -1 on failure.
 */
int cd_resume(int cd_desc)
{
  if(cdrom[cd_desc]==NULL)
    return -1;

  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_PLAY,0,0)!=0)
    return -1;

  cdrom[cd_desc]->cdrom_paused=0;
  return 0;
}

/**
 * Eject the tray.
 * @param cd_desc the handle to the device to eject.
 * @return 0 on success, -1 on failure.
 */
int cd_eject(cddesc_t cd_desc)
{
  if(cdrom[cd_desc]==NULL)
    return -1;

  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_SET,MCI_SET_DOOR_OPEN,0)!=0)
    return -1;

  cdrom[cd_desc]->cdrom_paused=0;
  return 0;
}

/**
 * Close the tray.
 * @param cd_desc the handle to the device to close.
 * @return 0 on success, -1 on failure.
 */
int cd_close(int cd_desc)
{
  if(cdrom[cd_desc]==NULL)
    return -1;

  if(mciSendCommand(cdrom[cd_desc]->cdrom_id,MCI_SET,MCI_SET_DOOR_CLOSED,0)!=0)
    return -1;

  cdrom[cd_desc]->cdrom_paused=0;
  return 0;
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
  MIXERCONTROLDETAILS mxcd;
  MIXERCONTROLDETAILS_UNSIGNED volStruct[2];

  if(cdrom[cd_desc]==NULL)
    return -1;

  if(mixer!=NULL)
  {
    memset(&mxcd,0,sizeof(mxcd));
    mxcd.cbStruct=sizeof(mxcd);
    mxcd.cChannels=(mxl.cChannels==1) ? 1 : 2;
    mxcd.cbDetails=mxcd.cChannels*sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.dwControlID=mxc.dwControlID;
    mxcd.paDetails=&volStruct;
    if(mixerGetControlDetails((HMIXEROBJ)mixer,&mxcd,MIXER_GETCONTROLDETAILSF_VALUE)==MMSYSERR_NOERROR)
    {
      vol->vol_back.left=vol->vol_front.left=((double)(volStruct[0].dwValue-mxc.Bounds.dwMinimum))/((double)(mxc.Bounds.dwMaximum-mxc.Bounds.dwMinimum));
      if(mxl.cChannels==1)
        vol->vol_back.right=vol->vol_front.right=vol->vol_back.left;
      else
        vol->vol_back.right=vol->vol_front.right=((double)(volStruct[1].dwValue-mxc.Bounds.dwMinimum))/((double)(mxc.Bounds.dwMaximum-mxc.Bounds.dwMinimum));
      return 0;
    }
  }

  return -1;
}

/**
 * Set the volume level.
 * @param cd_desc the handle to the device to modify.
 * @param vol a disc_volume structure specifying the new volume level.
 *            as a percentage (scale 0.0-1.0).
 * @return 0 on success, -1 on failure.
 */
int cd_set_volume(cddesc_t cd_desc,const struct disc_volume *vol)
{
  MIXERCONTROLDETAILS mxcd;
  MIXERCONTROLDETAILS_UNSIGNED volStruct[2];

  if(cdrom[cd_desc]==NULL)
    return -1;

  if(vol->vol_front.left>1.0f||vol->vol_front.left<0.0f||vol->vol_front.right>1.0f||vol->vol_front.right<0.0f||vol->vol_back.left>1.0f||vol->vol_back.left<0.0f||vol->vol_back.right>1.0f||vol->vol_back.right<0.0f)
    return -1;

  if(mixer!=NULL)
  {
    long result;
    /* The windows mixer does not support front and back.  Use the larger of the two.  */
    double left=(vol->vol_back.left>vol->vol_front.left)?vol->vol_back.left:vol->vol_front.left;
    double right=(vol->vol_back.right>vol->vol_front.right)?vol->vol_back.right:vol->vol_front.right;

    memset(&mxcd,0,sizeof(mxcd));
    mxcd.cbStruct=sizeof(mxcd);
    mxcd.hwndOwner = 0;
    mxcd.dwControlID=mxc.dwControlID;
    mxcd.paDetails=&volStruct;

    if(mxl.cChannels==1)
    {
      mxcd.cChannels=1;
      mxcd.cbDetails=sizeof(MIXERCONTROLDETAILS_UNSIGNED);

      /* When left/right have different values for a single channel, use the larger */
      volStruct[0].dwValue=mxc.Bounds.dwMinimum+((DWORD)((left>right?left:right)*(mxc.Bounds.dwMaximum-mxc.Bounds.dwMinimum)));
    }
    else
    {
      mxcd.cChannels=2;
      mxcd.cbDetails=2*sizeof(MIXERCONTROLDETAILS_UNSIGNED);
      volStruct[0].dwValue=mxc.Bounds.dwMinimum+((DWORD)(left*(mxc.Bounds.dwMaximum-mxc.Bounds.dwMinimum)));
      volStruct[1].dwValue=mxc.Bounds.dwMinimum+((DWORD)(right*(mxc.Bounds.dwMaximum-mxc.Bounds.dwMinimum)));
    }

    result=mixerSetControlDetails((HMIXEROBJ)mixer,&mxcd,MIXER_SETCONTROLDETAILSF_VALUE);
    if(result==MMSYSERR_NOERROR)
      return 0;
  }
  return -1;
}

#endif
