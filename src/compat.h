/*
This is part of the audio CD player library
Copyright (C)1998-99 Tony Arcieri
Copyright (C)2001 Dustin Graves <dgraves@computer.org

Compatibility abstraction contributed by Quinton Dolan

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

#ifndef _COMPAT_H
#define _COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef  CDIOCREADSUBCHANNEL
#define CDIOREADSUBCHANNEL CDIOCREADSUBCHANNEL
#endif

#ifndef CDROM_DATA_TRACK
#define CDROM_DATA_TRACK 0x04
#endif

#ifndef CDROM_LEADOUT
#define CDROM_LEADOUT           0xAA
#endif

#ifdef CD_MSF_FORMAT
#define CDAUDIO_MSF_FORMAT CD_MSF_FORMAT
#endif

#ifdef CD_LBA_FORMAT
#define CDAUDIO_LBA_FORMAT CD_LBA_FORMAT
#endif

#ifdef CDROM_MSF
#define CDAUDIO_MSF_FORMAT CDROM_MSF
#endif

#ifdef CDROM_MSF_FORMAT
#define CDAUDIO_MSF_FORMAT CDROM_MSF_FORMAT
#endif

#ifdef  CDIOREADTOCHEADER
#define CDAUDIO_READTOCHEADER CDIOREADTOCHEADER

#define CDAUDIO_TOCHEADER ioc_toc_header
#define CDTH_STARTING_TRACK starting_track
#define CDTH_ENDING_TRACK ending_track
#endif /* CDIOREADTOCHEADER */

#ifdef  CDROMREADTOCHDR
#define CDAUDIO_READTOCHEADER CDROMREADTOCHDR

#define CDAUDIO_TOCHEADER cdrom_tochdr
#define CDAUDIO_READTOCHEADER CDROMREADTOCHDR
#define CDTH_STARTING_TRACK cdth_trk0
#define CDTH_ENDING_TRACK cdth_trk1
#endif /* CDROMREADTOCHDR */

#ifdef  CDROM_TOC_HEADER
#define CDAUDIO_READTOCHEADER CDROM_TOC_HEADER

#define CDAUDIO_READTOCHEADER CDROM_TOC_HEADER
#define CDAUDIO_TOCHEADER cd_toc_header
#define CDTH_STARTING_TRACK th_starting_track
#define CDTH_ENDING_TRACK th_ending_track
#endif /* CDROM_TOC_HEADER */

#ifdef  CDIOREADTOCENTRYS
#define CDAUDIO_READTOCENTRYS CDIOREADTOCENTRYS

#define CDAUDIO_TOCENTRY ioc_read_toc_entry
#define CDAUDIO_TOCENTRY_DATA cd_toc_entry 

#define CDTE_ADDRESS_FORMAT address_format
#define CDTE_STARTING_TRACK starting_track
#define CDTE_DATA data
#define CDTE_DATA_LEN data_len
#define CDTE_MSF_M addr.msf.minute
#define CDTE_MSF_S addr.msf.second
#define CDTE_MSF_F addr.msf.frame
#define CDTE_LBA addr.lba
#define CDTE_CONTROL control
#endif /* CDIOREADTOCENTRYS */

#ifdef  CDROMREADTOCENTRY
#define CDAUDIO_READTOCENTRY CDROMREADTOCENTRY

#define CDAUDIO_TOCENTRY cdrom_tocentry

#define CDTE_ADDRESS_FORMAT cdte_format
#define CDTE_STARTING_TRACK cdte_track
#define CDTE_MSF_M cdte_addr.msf.minute
#define CDTE_MSF_S cdte_addr.msf.second
#define CDTE_MSF_F cdte_addr.msf.frame
#define CDTE_LBA cdte_addr.lba
#define CDTE_CONTROL cdte_ctrl
#endif /* CDROMREADTOCENTRY */

#ifdef  CDROM_TOC_ENTRYS
#define CDAUDIO_READTOCENTRYS CDROM_TOC_ENTRYS

#define CDAUDIO_TOCENTRY cd_toc
#define CDAUDIO_TOCENTRY_DATA cd_toc_entry

#define CDTE_ADDRESS_FORMAT toc_address_format
#define CDTE_STARTING_TRACK toc_starting_track
#define CDTE_DATA toc_buffer
#define CDTE_DATA_LEN toc_alloc_length
#define CDTE_MSF_M te_absaddr.m_units
#define CDTE_MSF_S te_absaddr.s_units
#define CDTE_MSF_F te_absaddr.f_units
#define CDTE_CONTROL te_control
#endif /* CDROM_TOC_ENTRYS */

#ifdef  CDIOREADSUBCHANNEL
#define CDAUDIO_READSUBCHANNEL CDIOREADSUBCHANNEL

#define CDAUDIO_SUBCHANNEL ioc_read_subchannel
#define CDAUDIO_SUBCHANNEL_DATA cd_sub_channel_info
#define CDAUDIO_CDSC_DATA_FORMAT CD_CURRENT_POSITION

#define CDSC_DATA data
#define CDSC_DATA_LEN data_len
#define CDSC_DATA_FORMAT data_format
#define CDSC_ADDRESS_FORMAT address_format

#define CDSC_DATA_ABS_MSF_M what.position.absaddr.msf.minute
#define CDSC_DATA_ABS_MSF_S what.position.absaddr.msf.second
#define CDSC_DATA_ABS_MSF_F what.position.absaddr.msf.frame
#define CDSC_DATA_REL_MSF_M what.position.reladdr.msf.minute
#define CDSC_DATA_REL_MSF_S what.position.reladdr.msf.second
#define CDSC_DATA_REL_MSF_F what.position.reladdr.msf.frame
#define CDSC_DATA_TRK what.position.track_number
#define CDSC_AUDIO_STATUS header.audio_status

#define CDAUDIO_AS_PLAY_IN_PROGRESS CD_AS_PLAY_IN_PROGRESS
#define CDAUDIO_AS_PLAY_PAUSED CD_AS_PLAY_PAUSED
#define CDAUDIO_AS_PLAY_COMPLETED CD_AS_PLAY_COMPLETED
#endif /* CDIOREADSUBCHANNEL */

#ifdef CDROMSUBCHNL
#define CDAUDIO_READSUBCHANNEL CDROMSUBCHNL

#define CDAUDIO_SUBCHANNEL cdrom_subchnl
#define CDSC_ADDRESS_FORMAT cdsc_format

#define CDSC_ABS_MSF_M cdsc_absaddr.msf.minute
#define CDSC_ABS_MSF_S cdsc_absaddr.msf.second
#define CDSC_ABS_MSF_F cdsc_absaddr.msf.frame
#define CDSC_REL_MSF_M cdsc_reladdr.msf.minute
#define CDSC_REL_MSF_S cdsc_reladdr.msf.second
#define CDSC_REL_MSF_F cdsc_reladdr.msf.frame
#define CDSC_TRK cdsc_trk;
#define CDSC_AUDIO_STATUS cdsc_audiostatus

#define CDAUDIO_AS_PLAY_IN_PROGRESS CDROM_AUDIO_PLAY
#define CDAUDIO_AS_PLAY_PAUSED CDROM_AUDIO_PAUSED
#define CDAUDIO_AS_PLAY_COMPLETED CDROM_AUDIO_COMPLETED

#endif /* CDROMSUBCHNL */

#ifdef CDROM_READ_SUBCHANNEL
#define CDAUDIO_READSUBCHANNEL CDROM_READ_SUBCHANNEL

#define CDAUDIO_SUBCHANNEL cd_sub_channel
#define CDAUDIO_SUBCHANNEL_DATA cd_sub_channel_data
#define CDAUDIO_CDSC_DATA_FORMAT CDROM_SUBQ_DATA

#define CDSC_DATA sch_buffer
#define CDSC_DATA_LEN sch_alloc_length
#define CDSC_DATA_FORMAT sch_data_format
#define CDSC_ADDRESS_FORMAT sch_address_format

#define CDSC_DATA_ABS_MSF_M scd_position_data.scp_absaddr.m_units
#define CDSC_DATA_ABS_MSF_S scd_position_data.scp_absaddr.s_units
#define CDSC_DATA_ABS_MSF_F scd_position_data.scp_absaddr.f_units
#define CDSC_DATA_REL_MSF_M scd_position_data.scp_reladdr.m_units
#define CDSC_DATA_REL_MSF_S scd_position_data.scp_reladdr.s_units
#define CDSC_DATA_REL_MSF_F scd_position_data.scp_reladdr.f_units
#define CDSC_DATA_TRK scd_position_data.scp_track_number
#define CDSC_AUDIO_STATUS scd_header.sh_audio_status

#define CDAUDIO_AS_PLAY_IN_PROGRESS AS_PLAY_IN_PROGRESS
#define CDAUDIO_AS_PLAY_PAUSED AS_PLAY_PAUSED
#define CDAUDIO_AS_PLAY_COMPLETED AS_PLAY_COMPLETED

#endif /* CDROM_READ_SUBCHANNEL */

#ifdef  CDIOCPLAYMSF
#define CDAUDIO_PLAY_MSF CDIOCPLAYMSF

#define CDAUDIO_MSF	ioc_play_msf
#define CDMSF_START_M	start_m
#define CDMSF_START_S	start_s
#define CDMSF_START_F	start_f
#define CDMSF_END_M	end_m
#define CDMSF_END_S	end_s
#define CDMSF_END_F	end_f
#endif /* CDIOCPLAYMSF */

#ifdef  CDROMPLAYMSF
#define CDAUDIO_PLAY_MSF CDROMPLAYMSF

#define CDAUDIO_MSF	cdrom_msf
#define CDMSF_START_M	cdmsf_min0
#define CDMSF_START_S	cdmsf_sec0
#define CDMSF_START_F	cdmsf_frame0
#define CDMSF_END_M	cdmsf_min1
#define CDMSF_END_S	cdmsf_sec1
#define CDMSF_END_F	cdmsf_frame1
#endif /* CDROMPLAYMSF */

#ifdef  CDROM_PLAY_AUDIO_MSF
#define CDAUDIO_PLAY_MSF CDROM_PLAY_AUDIO_MSF

#define CDAUDIO_MSF cd_play_audio_msf
#define CDMSF_START_M	msf_starting_M_unit
#define CDMSF_START_S	msf_starting_S_unit
#define CDMSF_START_F	msf_starting_F_unit
#define CDMSF_END_M	msf_ending_M_unit
#define CDMSF_END_S	msf_ending_S_unit
#define CDMSF_END_F	msf_ending_F_unit
#endif /* CDROM_PLAY_AUDIO_MSF */

#ifdef CDIOCSTART
#define CDAUDIO_START CDIOCSTART
#endif

#ifdef CDROMSTART
#define CDAUDIO_START CDROMSTART
#endif

#ifdef CDIOCSTOP
#define CDAUDIO_STOP CDIOCSTOP
#endif

#ifdef CDROMSTOP
#define CDAUDIO_STOP CDROMSTOP
#endif

#ifdef CDIOCPAUSE
#define CDAUDIO_PAUSE CDIOCPAUSE
#endif

#ifdef CDROMPAUSE
#define CDAUDIO_PAUSE CDROMPAUSE
#endif

#ifdef CDROM_PAUSE_PLAY
#define CDAUDIO_PAUSE CDROM_PAUSE_PLAY
#endif

#ifdef CDIOCRESUME
#define CDAUDIO_RESUME CDIOCRESUME
#endif

#ifdef CDROMRESUME
#define CDAUDIO_RESUME CDROMRESUME
#endif

#ifdef CDROM_RESUME_PLAY
#define CDAUDIO_RESUME CDROM_RESUME_PLAY
#endif

#ifdef CDIOCEJECT
#define CDAUDIO_EJECT CDIOCEJECT
#endif

#ifdef CDROMEJECT
#define CDAUDIO_EJECT CDROMEJECT
#endif

#ifdef CDIOCCLOSE
#define CDAUDIO_CLOSE CDIOCCLOSE
#endif

#ifdef CDROMCLOSETRAY
#define CDAUDIO_CLOSE CDROMCLOSETRAY
#endif

#ifdef CDIOCGETVOL
#define CDAUDIO_GET_VOLUME CDIOCGETVOL

#define CDAUDIO_VOLSTAT ioc_vol
#define CDVOLSTAT_FRONT_LEFT vol[0]
#define CDVOLSTAT_FRONT_RIGHT vol[1]
#define CDVOLSTAT_BACK_LEFT vol[2]
#define CDVOLSTAT_BACK_RIGHT vol[3]
#endif /* CDIOCGETVOL */

#ifdef CDROMVOLREAD
#define CDAUDIO_GET_VOLUME CDROMVOLREAD

#define CDAUDIO_VOLSTAT cdrom_volctrl
#define CDVOLSTAT_FRONT_LEFT channel0
#define CDVOLSTAT_FRONT_RIGHT channel1
#define CDVOLSTAT_BACK_LEFT channel2
#define CDVOLSTAT_BACK_RIGHT channel3
#endif /* CDROMVOLREAD */

#ifdef CDROM_PLAYBACK_STATUS
#define CDAUDIO_GET_VOLUME CDROM_PLAYBACK_STATUS

#define CDAUDIO_VOLSTAT cd_playback
#define CDAUDIO_VOLSTAT_DATA cd_playback_status
#define CDVOLSTAT_DATA pb_buffer
#define CDVOLSTAT_DATA_LEN pb_alloc_length
#define CDVOLSTAT_FRONT_LEFT ps_chan0_volume
#define CDVOLSTAT_FRONT_RIGHT ps_chan1_volume
#define CDVOLSTAT_BACK_LEFT ps_chan2_volume
#define CDVOLSTAT_BACK_RIGHT ps_chan3_volume
#endif /* CDROM_PLAYBACK_STATUS */

#ifdef CDIOCSETVOL
#define CDAUDIO_SET_VOLUME CDIOCSETVOL

#define CDAUDIO_VOLCTRL ioc_vol
#define CDVOLCTRL_FRONT_LEFT vol[0]
#define CDVOLCTRL_FRONT_RIGHT vol[1]
#define CDVOLCTRL_BACK_LEFT vol[2]
#define CDVOLCTRL_BACK_RIGHT vol[3]
#endif

#ifdef CDROMVOLCTRL
#define CDAUDIO_SET_VOLUME CDROMVOLCTRL

#define CDAUDIO_VOLCTRL cdrom_volctrl
#define CDVOLCTRL_FRONT_LEFT channel0
#define CDVOLCTRL_FRONT_RIGHT channel1
#define CDVOLCTRL_BACK_LEFT channel2
#define CDVOLCTRL_BACK_RIGHT channel3
#endif

#ifdef CDROM_PLAYBACK_CONTROL
#define CDAUDIO_SET_VOLUME CDROM_PLAYBACK_CONTROL
#define CDAUDIO_MAX_VOLUME CDROM_MAX_VOLUME

#define CDAUDIO_VOLCTRL cd_playback_control
#define CDAUDIO_VOLCTRL_DATA cd_playback
#define CDAUDIO_VOLCTRL_SELECT 1
#define CDVOLCTRL_DATA pb_buffer
#define CDVOLCTRL_DATA_LEN pb_alloc_length
#define CDVOLCTRL_FRONT_LEFT pc_chan0_volume
#define CDVOLCTRL_FRONT_RIGHT pc_chan1_volume
#define CDVOLCTRL_BACK_LEFT pc_chan2_volume
#define CDVOLCTRL_BACK_RIGHT pc_chan3_volume
#define CDVOLCTRL_FRONT_LEFT_SELECT pc_chan0_select
#define CDVOLCTRL_FRONT_RIGHT_SELECT pc_chan1_select
#define CDVOLCTRL_BACK_LEFT_SELECT pc_chan2_select
#define CDVOLCTRL_BACK_RIGHT_SELECT pc_chan3_select
#endif

#ifdef __cplusplus
}
#endif

#endif
