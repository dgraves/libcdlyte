/*
This is part of the audio CD player library
Copyright (C)1998-99 Tony Arcieri
Copyright (C)2001,2004 Dustin Graves <dgraves@computer.org>

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

#ifndef _CDVER_H
#define _CDVER_H

#define LIBCDLYTE_PACKAGE "libcdlyte"
#define LIBCDLYTE_VERSION "0.9.8"

#define LIBCDLYTE_MAJOR_VERSION 0
#define LIBCDLYTE_MINOR_VERSION 9
#define LIBCDLYTE_PATCH_LEVEL   8

#define LIBCDLYTE_VERSION_NUMBER \
        ((LIBCDLYTE_MAJOR_VERSION<<16)| \
         (LIBCDLYTE_MINOR_VERSION<< 8)| \
         (LIBCDLYTE_PATCH_LEVEL))

#endif
