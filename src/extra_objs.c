/*
This is part of the audio CD player library
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

#ifndef WIN32

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifndef HAVE_SNPRINTF
/* The system doesn't have a snprintf() function.  Punt, and map it to
   sprintf(). */
int snprintf(char *dest,int size,const char *format,...)
{
  va_list ap;
  int result;

  va_start(ap,format);
  result=vsprintf(dest,format,ap);
  va_end(ap);
  return result;
}
#endif  /* HAVE_SNPRINTF */

#endif  /* WIN32 */
