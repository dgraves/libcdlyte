/*
This is part of the audio CD player library
Copyright (C)1998-99 Tony Arcieri
Copyright (C)2001 Dustin Graves <dgraves@computer.org>

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

#ifdef __cplusplus
extern "C" {
#endif

#define LIBCDLYTE_VERSION_MAJOR 0
#define LIBCDLYTE_VERSION_MINOR 9
#define LIBCDLYTE_VERSION_MICRO 5

#define LIBCDLYTE_VERSION \
        ((LIBCDLYTE_VERSION_MAJOR<<16)| \
         (LIBCDLYTE_VERSION_MINOR<< 8)| \
         (LIBCDLYTE_VERSION_MICRO))

/* Used with disc_info */
#define CDLYTE_PLAYING				0
#define CDLYTE_PAUSED				1
#define CDLYTE_COMPLETED			2
#define CDLYTE_NOSTATUS				3
#define CDLYTE_INVALID				4
#define CDLYTE_ERROR				5

#define CDLYTE_TRACK_AUDIO 			0
#define CDLYTE_TRACK_DATA 			1

#define MAX_TRACKS				100
#define MAX_SLOTS				100 /* For CD changers */

/* CDDB defaults */
#define CDDB_PROTOCOL_LEVEL 			5
#define CDDBP_DEFAULT_PORT			8880
#define HTTP_DEFAULT_PORT			80
#define CDDB_HTTP_QUERY_CGI			"~cddb/cddb.cgi"
#define CDINDEX_QUERY_CGI			"cgi-bin/cdi/get.pl"
#define CDDB_EMAIL_SUBMIT_ADDRESS		"freedb-submit@freedb.org"
#define CDDB_HTTP_SUBMIT_CGI 			"~cddb/submit.cgi"
#define CDINDEX_SUBMIT_CGI			"cgi-bin/cdi/xsubmit.pl"
#define CDDB_SUBMIT_HTTP			0
#define CDDB_SUBMIT_EMAIL			1
#define CDDB_SUBMIT_MODE 			0
#define CDDB_MAX_SERVERS			128
#define MAX_INEXACT_MATCHES			16
#define EXTENDED_DATA_SIZE 			4096
#define DISC_ART_SIZE				32768
#define CDINDEX_ID_SIZE				30

/* Connection modes */
#define CDDB_MODE_CDDBP 			0
#define CDDB_MODE_HTTP 				1
#define CDINDEX_MODE_HTTP			2
#define COVERART_MODE_HTTP			3
#define CDDB_ACCESS_LOCAL 			0
#define CDDB_ACCESS_REMOTE 			1
#define CDDB_PROXY_DISABLED 			0
#define CDDB_PROXY_ENABLED 			1

/* CDDB genres */
#define CDDB_UNKNOWN				0
#define CDDB_BLUES				1
#define CDDB_CLASSICAL				2
#define CDDB_COUNTRY				3
#define CDDB_DATA				4
#define CDDB_FOLK				5
#define CDDB_JAZZ				6
#define CDDB_MISC				7
#define CDDB_NEWAGE				8
#define CDDB_REGGAE				9
#define CDDB_ROCK				10
#define CDDB_SOUNDTRACK				11

/* CD Index artist types */
#define CDINDEX_SINGLE_ARTIST			0
#define CDINDEX_MULTIPLE_ARTIST			1

/* Play function options */
#define PLAY_START_TRACK			0
#define PLAY_END_TRACK				1
#define PLAY_START_POSITION			2
#define PLAY_END_POSITION			4

/* CD error return values */
#ifndef WIN32
#define INVALID_CDDESC				-1
#else
#define INVALID_CDDESC				(~0)
#endif


#ifndef __LIBCDLYTE_INTERNAL
#define cdindex_write_disc_data(cd_desc, data)	cdindex_write_data(cd_desc, &data)
#define cddb_write_disc_data(cd_desc, data)	cddb_write_data(cd_desc, &data)
#define cddb_erase_entry(data)			cddb_erase_data(&data)

extern char cddb_message[256];
extern int parse_disc_artist;
extern int parse_track_artist;
extern int cddb_submit_method;
extern char *cddb_submit_email_address;
#endif


/* Library typedefs */

#ifndef WIN32
/** The type descriptor for a CD device handle.  */
typedef int cddesc_t;
#else
/** The type descriptor for a CD device handle.  */
#include <windows.h>
#include <mmsystem.h>
typedef MCIDEVICEID cddesc_t;
#endif


/* Structure definitions */

/** CDDB entry */
struct cddb_entry {
   int entry_present;				/* Is there an entry? */
   long entry_timestamp;                        /* Last modification time */
   unsigned long entry_id;                      /* CDDB ID of entry */
   char entry_cdindex_id[CDINDEX_ID_SIZE];      /* CD Index ID of entry */
   int entry_genre;                             /* CDDB genre of entry */
};

/** CDDB configuration */
struct cddb_conf {
   int conf_access;                             /* CDDB_ACCESS_LOCAL or CDDB_ACCESS_REMOTE */
   int conf_proxy;                              /* Is a proxy being used? */
};
	
/** Server retrieval structure */
struct cddb_server {
   char server_name[256]; 			/* Server name */
   int server_port; 				/* Server port */
};

/** CDDB server list structure */
struct cddb_host {
   struct cddb_server host_server;
   char host_addressing[256];
   int host_protocol;
};

/** CDDB hello structure */
struct cddb_hello {
   char hello_program[256]; 			/* Program name*/
   char hello_version[256]; 			/* Version */
};

/** An entry in the query list */
struct query_list_entry {
   int list_genre;                              /* CDDB genre of entry */
   unsigned long list_id;                       /* CDDB ID of entry */
   char list_title[64];                         /* Title of entry */
   char list_artist[64];                        /* Entry's artist */
};

/** TOC data for CDDB query */
struct cddb_direct_query_toc {
   unsigned long toc_id;
   int tracks;
   int positions[MAX_TRACKS];
};

/** CDDB query structure */
struct cddb_query {
#define QUERY_NOMATCH				0
#define QUERY_EXACT				1
#define QUERY_INEXACT				2
   int query_match;				/* Uses above definitions */
   int query_matches;                           /* Number of matches */
   struct query_list_entry query_list[MAX_INEXACT_MATCHES];
};

/** Used for keeping track of times */
struct disc_timeval {
   int minutes;
   int seconds;
   int frames;
};

/** Brief disc information */
struct disc_status {
   int status_present;				/* Is disc present? */
   int status_mode;				/* Disc mode */
   struct disc_timeval status_disc_time;	/* Current disc time */
   struct disc_timeval status_track_time; 	/* Current track time */
   int status_current_track;			/* Current track */
};

/** Track specific information */
struct track_info {
   struct disc_timeval track_length;		/* Length of track */
   struct disc_timeval track_pos;		/* Position of track */
   int track_lba;				/* Logical Block Address */
   int track_type;				/* CDLYTE_TRACK_AUDIO or CDLYTE_TRACK_DATA */
};

/** Disc information such as current track, amount played, etc */
struct disc_info {
   int disc_present;				/* Is disc present? */
   int disc_mode;				/* Current disc mode */
   struct disc_timeval disc_track_time;		/* Current track time */
   struct disc_timeval disc_time;		/* Current disc time */
   struct disc_timeval disc_length;		/* Total disc length */
   int disc_current_track;			/* Current track */
   int disc_first_track;			/* First track on the disc */
   int disc_total_tracks;			/* Number of tracks on disc */
   struct track_info disc_track[MAX_TRACKS];	/* Track specific information */
};

/** Invisible volume structure */
struct __volume { 
   int left;
   int right;
};

/** Volume structure */
struct disc_volume {
   struct __volume vol_front;			/* Normal volume settings */
   struct __volume vol_back;			/* Surround sound volume settings */
};

/** Track database structure */
struct track_data {
   char track_name[256];			/* Track name */
   char track_artist[256];			/* Track specific artist */
   char track_extended[EXTENDED_DATA_SIZE];	/* Extended information */
};

/** Disc database structure */
struct disc_data {
   unsigned long data_id;			/* CDDB ID */
   char data_cdindex_id[CDINDEX_ID_SIZE];	/* CD Index ID */
   int data_revision; 				/* CDDB revision (incremented with each submit) */
   char data_title[256];			/* Disc title */
   char data_artist[256];			/* Album artist */
   char data_extended[EXTENDED_DATA_SIZE];	/* Extended information */
   int data_genre;				/* Disc genre */
   int data_artist_type;			/* Single or multiple artist CD */
   struct track_data data_track[MAX_TRACKS];	/* Track names */
};

/** Compact track database structure */
struct track_mc_data {
   int track_name_len;
   char *track_name;
   int track_artist_len;
   char *track_artist;
   int track_extended_len;
   char *track_extended;
};

/** Compact disc database structure */
struct disc_mc_data {
   unsigned long data_id;
   char data_cdindex_id[CDINDEX_ID_SIZE];
   int data_title_len;
   char *data_title;
   int data_artist_len;
   char *data_artist;
   int data_extended_len;
   char *data_extended;
   int data_genre;
   int data_revision;
   int data_artist_type;
   int data_total_tracks;
   struct track_mc_data **data_track;
};

/** Summary of a single disc in the changer */
struct disc_summary {
   int disc_present;				/* Is disc present? */
   struct disc_timeval disc_length;		/* Length of disc */
   int disc_total_tracks;			/* Total tracks */
   unsigned long disc_id;			/* CDDB ID */
   char data_cdindex_id[CDINDEX_ID_SIZE];	/* CDI ID */
   char disc_info[128];				/* Artist name / Disc name */
};

/** Disc changer structure */
struct disc_changer {
   int changer_slots;
   struct disc_summary changer_disc[MAX_SLOTS];
};

/* Compact disc summary */
struct disc_mc_summary {
   int disc_present;
   struct disc_timeval disc_length;
   int disc_total_tracks;
   unsigned long disc_id;
   char data_cdindex_id[CDINDEX_ID_SIZE];
   int disc_info_len;
   char *disc_info;
};

/** Compact disc changer structure */
struct disc_mc_changer {
   int changer_slots;
   struct disc_mc_summary **changer_disc;
};

/** CDDB server list structure */
struct cddb_serverlist {
   int list_len;
   struct cddb_host list_host[CDDB_MAX_SERVERS];
};


/* CD function declarations */

/* Return the name and version number of libcdlyte as a string.  */
void cd_version(char *buffer,int len);

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

/* Read CD table of contents and get current status.  */
int cd_stat(cddesc_t cd_desc,struct disc_info *disc);

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

long cddb_discid(int cd_desc);
int cddb_process_url(struct cddb_host *host, const char *url);
int cddb_read_serverlist(struct cddb_conf *conf, struct cddb_serverlist *list, struct cddb_server *proxy);
int cddb_write_serverlist(const struct cddb_conf *conf,const  struct cddb_serverlist *list,const struct cddb_server *proxy);
char *cddb_genre(int genre);
int cddb_direct_mc_alloc(struct disc_mc_data *data, int tracks);
int cddb_mc_alloc(int cd_desc, struct disc_mc_data *data);
void cddb_mc_free(struct disc_mc_data *data);
int cddb_mc_copy_from_data(struct disc_mc_data *outdata,const struct disc_data *indata);
int cddb_data_copy_from_mc(struct disc_data *outdata,const struct disc_mc_data *indata);
int cddb_connect(const struct cddb_server *server);
int cddb_connect_server(const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello,...);
int cddb_skip_http_header(int sock);
int cddb_read_token(int sock, int token[3]);
int cddb_query(int cd_desc, int sock, int mode, struct cddb_query *query, ...);
int cddb_read(int cd_desc, int sock, int mode,const struct cddb_entry *entry, struct disc_data *data, ...);
int cddb_quit(int sock);
int cddb_sites(int sock, int mode, struct cddb_serverlist *list, ...);
int cddb_read_data(int desc, struct disc_data *data);
int cddb_generate_unknown_entry(int cd_desc, struct disc_data *data);
int cddb_read_disc_data(int cd_desc, struct disc_data *data);
int cddb_write_data(int cd_desc,const struct disc_data *data);
int cddb_erase_data(const struct disc_data *data);
int cddb_stat_disc_data(int cd_desc, struct cddb_entry *entry);
int cddb_http_query(int cd_desc,const struct cddb_host *host,const struct cddb_hello *hello, struct cddb_query *query);
int cddb_http_proxy_query(int cd_desc,const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello, struct cddb_query *query);
int cddb_http_read(int cd_desc,const struct cddb_host *host,const struct cddb_hello *hello,const struct cddb_entry *entry, struct disc_data *data);
int cddb_http_proxy_read(int cd_desc,const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello,const struct cddb_entry *entry, struct disc_data *data);
int cddb_http_sites(int cd_desc,const struct cddb_host *host,const struct cddb_hello *hello, struct cddb_serverlist *list);
int cddb_http_proxy_sites(int cd_desc,const struct cddb_host *host,const struct cddb_server *proxy,const struct cddb_hello *hello, struct cddb_serverlist *list);
int cddb_http_submit(int cd_desc,const struct cddb_host *host, struct cddb_server *proxy, char *email_address);

int cdindex_discid(int cd_desc, char *discid, int len);
int cdindex_connect_server(const struct cddb_host *host, struct cddb_server *proxy, char *http_string, int len);
int cdindex_read(int cd_desc, int sock, struct disc_data *data, char *http_string);
int cdindex_write_data(int cd_desc, struct disc_data *data);

int cd_changer_select_disc(int cd_desc, int disc);
int cd_changer_slots(int cd_desc);
int cd_changer_stat(int cd_desc, struct disc_changer *changer);

#ifdef __cplusplus
}
#endif

#endif
