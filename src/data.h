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

#ifndef _DATA_H
#define _DATA_H

#include <cdlyte.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_EXTEMPORANEOUS_LINES                  6
#define MAX_EXTENDED_LINES                        64

/* Internal CDDB structures used for preprocessing */
struct __unprocessed_track_data {
  int track_name_index;
  char track_name[MAX_EXTEMPORANEOUS_LINES][80];
  int track_extended_index;
  char track_extended[MAX_EXTENDED_LINES][80];
};

struct __unprocessed_disc_data {
  unsigned long data_id;
  char data_cdindex_id[CDINDEX_ID_SIZE];
  int data_revision;
  int data_title_index;
  char data_title[MAX_EXTEMPORANEOUS_LINES][80];
  int data_extended_index;
  char data_extended[MAX_EXTENDED_LINES][80];
  int data_genre;
  struct __unprocessed_track_data data_track[MAX_TRACKS];
};

int data_format_input(struct disc_data *outdata,const struct __unprocessed_disc_data *indata,int tracks);
int data_format_output(struct __unprocessed_disc_data *outdata,const struct disc_data *indata,int tracks);
unsigned long __internal_cddb_discid(struct disc_info *disc);

#ifdef __cplusplus
}
#endif

#endif   
