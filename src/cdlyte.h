/*
This is part of the audio CD player library
Copyright (C)1998-99 Tony Arcieri
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

#ifndef _CDLYTE_H
#define _CDLYTE_H

#include "cdver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Used with disc_info */
#define CDLYTE_PLAYING			0
#define CDLYTE_PAUSED			1
#define CDLYTE_STOPPED			2
#define CDLYTE_COMPLETED		3
#define CDLYTE_NOSTATUS			4
#define CDLYTE_INVALID			5
#define CDLYTE_ERROR			6

#define CDLYTE_TRACK_AUDIO 		0
#define CDLYTE_TRACK_DATA 		1

/* CDDB defaults */
#define CDDB_PROTOCOL_LEVEL 		5
#define CDDBP_DEFAULT_PORT		8880
#define HTTP_DEFAULT_PORT		80
#define CDDB_HTTP_QUERY_CGI		"/~cddb/cddb.cgi"
#define CDDB_HTTP_SUBMIT_CGI 		"/~cddb/submit.cgi"
#define CDDB_LINE_SIZE 	                256

/* Connection and submission modes */
#define CDDB_MODE_CDDBP 		0
#define CDDB_MODE_HTTP 			1
#define CDDB_SUBMIT_SMTP		0
#define CDDB_SUBMIT_HTTP		1

/* CDDB genres */
#define CDDB_UNKNOWN			0
#define CDDB_BLUES			1
#define CDDB_CLASSICAL			2
#define CDDB_COUNTRY			3
#define CDDB_DATA			4
#define CDDB_FOLK			5
#define CDDB_JAZZ			6
#define CDDB_MISC			7
#define CDDB_NEWAGE			8
#define CDDB_REGGAE			9
#define CDDB_ROCK			10
#define CDDB_SOUNDTRACK			11

/* Play function options */
#define PLAY_START_TRACK		0
#define PLAY_END_TRACK			1
#define PLAY_START_POSITION		2
#define PLAY_END_POSITION		4


/* External declarations */
extern char cddb_message[CDDB_LINE_SIZE];


/* Library typedefs */

/** The type descriptor for a CD device handle.  */
typedef int cddesc_t;

/* CD error return values */
#define INVALID_CDDESC			-1

#ifndef WIN32
/** The type descriptor for a socket used to communicate with cddb and cdindex.  */
typedef int cdsock_t;

/* Socket error codes */
#define INVALID_CDSOCKET                -1
#define CDSOCKET_ERROR                  -1

#else

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>

/** The type descriptor for a socket used to communicate with cddb and cdindex.  */
typedef SOCKET cdsock_t;

/* Socket error codes */
#define INVALID_CDSOCKET                INVALID_SOCKET
#define CDSOCKET_ERROR                  SOCKET_ERROR
#endif


/* Structure definitions */

/** Server retrieval structure */
struct cddb_server
{
   char *server_name; 			        /* Server name */
   int   server_port; 			        /* Server port */
};

/** CDDB server list structure */
struct cddb_host
{
   struct cddb_server host_server;
   int    host_protocol;
   char  *host_addressing;
   char  *host_latitude;
   char  *host_longitude;
   char  *host_description;
};

/** CDDB server list structure */
struct cddb_serverlist
{
   int               list_len;
   struct cddb_host *list_host;
};

/** CDDB hello structure */
struct cddb_hello
{
   char *hello_user;                            /* User's name */
   char *hello_hostname;                        /* Workstation's name */
   char *hello_program; 			/* Program name */
   char *hello_version; 			/* Version */
};

/** An entry in the query list */
struct query_list_entry
{
   int           list_category;                  /* CDDB category of entry */
   unsigned long list_id;                        /* CDDB ID of entry */
   char         *list_title;                     /* Title of entry */
   char         *list_artist;                    /* Entry's artist */
};

/** CDDB query structure */
struct cddb_query
{
#define QUERY_NOMATCH				0
#define QUERY_EXACT				1
#define QUERY_INEXACT				2
   int                      query_match;	/* Uses above definitions */
   int                      query_matches;      /* Number of matches */
   struct query_list_entry *query_list;
};

/** Used for keeping track of times */
struct disc_timeval
{
   int minutes;
   int seconds;
   int frames;
};

/** Brief disc information */
struct disc_status
{
   int                 status_present;		/* Is disc present? */
   int                 status_mode;		/* Disc mode */
   struct disc_timeval status_disc_time;	/* Current disc time */
   struct disc_timeval status_track_time; 	/* Current track time */
   int                 status_current_track;	/* Current track */
};

/** Track specific information */
struct track_info
{
   struct disc_timeval track_length;		/* Length of track */
   struct disc_timeval track_pos;		/* Position of track */
   int                 track_lba;		/* Logical Block Address */
   int                 track_type;              /* CDLYTE_TRACK_AUDIO or CDLYTE_TRACK_DATA */
};

/** Disc information such as current track, amount played, etc */
struct disc_info
{
   int                 disc_present;		/* Is disc present? */
   int                 disc_mode;		/* Current disc mode */
   struct disc_timeval disc_track_time;		/* Current track time */
   struct disc_timeval disc_time;		/* Current disc time */
   struct disc_timeval disc_length;		/* Total disc length */
   int                 disc_current_track;	/* Current track */
   int                 disc_first_track;	/* First track on the disc */
   int                 disc_total_tracks;	/* Number of tracks on disc */
   struct track_info  *disc_track;	        /* Track specific information */
};

/** "Invisible" volume structure */
struct __volume
{
   float left;
   float right;
};

/** Volume structure */
struct disc_volume
{
   struct __volume vol_front;			/* Normal volume settings */
   struct __volume vol_back;			/* Surround sound volume settings */
};

/** Track database structure */
struct track_data
{
   char *track_artist;			        /* Track specific artist (will be empty if value was not present in database, eg. all tracks on disc are by same artist) */
   char *track_title;                           /* Track name */
   char *track_extended;                        /* Extended information */
};

/** Disc database structure */
struct disc_data
{
   unsigned long      data_id;			/* CDDB ID */
   int                data_category;		/* Disc category */
   int                data_revision; 		/* CDDB revision (incremented with each submit) */
   char              *data_artist;		/* Album artist */
   char              *data_title;		/* Disc title */
   char               data_year[5];             /* Year of publicarion */
   char              *data_genre;		/* Disc genre */
   int                data_total_tracks;
   struct track_data *data_track;	        /* Track names */
   char              *data_extended;	        /* Extended information */
};

/** Summary of a single disc in the changer */
struct disc_summary
{
   int                 disc_present;		/* Is disc present? */
   struct disc_timeval disc_length;		/* Length of disc */
   int                 disc_total_tracks;	/* Total tracks */
   unsigned long       disc_id;			/* CDDB ID */
   char               *disc_info;		/* Artist name / Disc name */
};

/** Disc changer structure */
struct disc_changer
{
   int                  changer_slots;
   struct disc_summary *changer_disc;
};


/* CD function declarations */

/* Return the name and version number of libcdlyte as a string.  */
char* cd_version(char *buffer,int len);

/* Return the version number of libcdlyte as an integer.  */
long cd_getversion();

/* Convert frames to a logical block address. */
int cd_frames_to_lba(int frames);

/* Convert a logical block address to frames.  */
int cd_lba_to_frames(int lba);

/* Convert disc_timeval to frames.  */
int cd_msf_to_frames(const struct disc_timeval *time);

/* Convert disc_timeval to a logical block address.  */
int cd_msf_to_lba(const struct disc_timeval *time);

/* Convert frames to disc_timeval.  */
void cd_frames_to_msf(struct disc_timeval *time,int frames);

/* Convert frames to disc_timeval.  */
void cd_lba_to_msf(struct disc_timeval *time,int frames);

/* Initialize the CD-ROM for playing audio CDs.  */
cddesc_t cd_init_device(char *device_name);

/* Close a device handle and free its resources.  */
int cd_finish(cddesc_t cd_desc);

/* Initialize disc_info structure for use with cd_stat.  */
void cd_init_disc_info(struct disc_info *disc);

/* Read CD table of contents and get current status.  */
int cd_stat(cddesc_t cd_desc,struct disc_info *disc);

/* Free resources allocated ro disc_info structure by cd_stat.  */
void cd_free_disc_info(struct disc_info *disc);

/* Get current CD status.  */
int cd_poll(cddesc_t cd_desc,struct disc_status *status);

/* Update information in a disc_info structure using a disc_status structure.  */
int cd_update(struct disc_info *disc,const struct disc_status *status);

/* Universal play control function.  */
int cd_playctl(cddesc_t cd_desc,int options,int starttrack,...);

/* Play from a selected track to end of CD.  */
int cd_play(cddesc_t cd_desc,int track);

/* Play from specified position within a selected track to end of CD.  */
int cd_play_pos(cddesc_t cd_desc,int track,int startpos);

/* Play a range of tracks from a CD.  */
int cd_play_track(cddesc_t cd_desc,int starttrack,int endtrack);

/* Play a range of tracks, starting at a specified position within a
   track, from a CD.  */
int cd_play_track_pos(int cd_desc,int starttrack,int endtrack,int startpos);

/* Play a range of frames from a CD.  */
int cd_play_frames(cddesc_t cd_desc,int startframe,int endframe);

/* Advance the position within a track while preserving an end track.  */
int cd_track_advance(cddesc_t cd_desc,int endtrack,const struct disc_timeval *time);

/* Advance the position within a track without preserving the end track.  */
int cd_advance(cddesc_t cd_desc,const struct disc_timeval *time);

/* Stop the CD.  */
int cd_stop(cddesc_t cd_desc);

/* Pause the CD.  */
int cd_pause(cddesc_t cd_desc);

/* Resume a paused CD.  */
int cd_resume(cddesc_t cd_desc);

/* Eject the tray.  */
int cd_eject(cddesc_t cd_desc);

/* Close the tray.  */
int cd_close(cddesc_t cd_desc);

/* Return the current volume level.  */
int cd_get_volume(cddesc_t cd_desc,struct disc_volume *vol);

/* Set the volume level.  */
int cd_set_volume(cddesc_t cd_desc,const struct disc_volume *vol);


/* CDDB function declarations */

/* Return CDDB ID for CD in device with handle cd_desc.  */
unsigned long cddb_discid(cddesc_t cd_desc);

/* Create a CDDB query string for the CD in device with handle cd_desc.  */
char* cddb_query_string(cddesc_t cd_desc, char *query, int *len);

/* Create a generic entry for an unknown disc.  */
int cddb_gen_unknown_entry(int cd_desc,struct disc_data *data);

/* Extract CDDB protocol, server address and port, and CGI script from URL.  */
int cddb_process_url(struct cddb_host *host,const char *url);

/* Convert numerical category identifier to text descriptor.  */
char *cddb_category(int category,char *buffer,int *len);

/* Convert category text descriptor to numerical identifier.  */
int cddb_category_value(const char *category);

/* Connect to a specified CDDB server.  */
cdsock_t cddb_connect(const struct cddb_host *host,const struct cddb_server *proxy,...);

/* Initiate a CDDB CDDBP session.  */
int cddb_handshake(cdsock_t sock,const struct cddb_hello *hello);

/* Set the CDDB protocol level for a CDDBP session.  */
int cddb_proto(cdsock_t sock);

/* Query CDDB for an entry corresponding to a specified CDDB id.  */
int cddb_query(const char *querystr,cdsock_t sock,int mode,struct cddb_query *query,...);

/* Read CDDB data for a specified CDDB entry.  */
int cddb_read(int category,unsigned long discid,cdsock_t sock,int mode,struct disc_data *data,...);

/* Retrieve CDDB server list.  */
int cddb_sites(cdsock_t sock,int mode,struct cddb_serverlist *list,...);

/* Terminate CDDB connection.  */
int cddb_quit(cdsock_t sock,int mode);

/* Read CDDB data from a local directory.  */
int cddb_read_local(const char *path,unsigned long discid,struct disc_data *data);

/* Write CDDB data to a local directory.  */
int cddb_write_local(const char *path,const struct cddb_hello *hello,const struct disc_info *info,const struct disc_data *data,const char *comment);

/* Erase CDDB data from local directory.  */
int cddb_erase_local(const char* path,unsigned long discid);

/* Set CDDB submission mode to test.  */
void cddb_set_test_submit(int test);

/* Get current CDDB submission mode.  */
int cddb_get_test_submit();

/* Submit CDDB data to a CDDB server.  */
int cddb_submit(const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello,const struct disc_info *info,const struct disc_data *data,const char *comment,const char *email_address,...);

/* Query CDDB for an entry corresponding to a specified CDDB id via HTTP.  */
int cddb_http_query(const char *querystr,const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello* hello,struct cddb_query *query);

/* Read CDDB data for a specified CDDB entry via HTTP.  */
int cddb_http_read(int category,unsigned long discid,const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello* hello,struct disc_data *data);

/* Retrieve CDDB server list via http.  */
int cddb_http_sites(const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello* hello,struct cddb_serverlist *list);

/* Do connect, handshake, and proto.  */
cdsock_t cddb_initiate(const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello,...);

/* Do full process to read entry.  */
int cddb_read_data(cddesc_t cd_desc,const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello,struct disc_data *data);

/* Do full sites read.  */
int cddb_read_sites(const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello,struct cddb_serverlist *list);


/* CD Changer function declarations */

int cd_changer_select_disc(int cd_desc,int disc);
int cd_changer_slots(int cd_desc);
int cd_changer_stat(const char *path,int cd_desc,struct disc_changer *changer);

#ifdef __cplusplus
}
#endif

#endif
