/*
This is part of the audio CD player library
Copyright (C)1998-99 Tony Arcieri <bascule@inferno.tusculum.edu>
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
xxFree Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

/* (C) Copyright 1993,1994 by Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Carnegie
 * Mellon University not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  Carnegie Mellon University makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)

Permission to use, copy, modify, and distribute this material
for any purpose and without fee is hereby granted, provided
that the above copyright notice and this permission notice
appear in all copies, and that the name of Bellcore not be
used in advertising or publicity pertaining to this
material without the specific, prior written permission
of an authorized representative of Bellcore.  BELLCORE
MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY
OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS",
WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.  
 */

/*
 * Program:     RFC-822 routines (originally from SMTP)
 *
 * Author:      Mark Crispin
 *              Networks and Distributed Computing
 *              Computing & Communications
 *              University of Washington
 *              Administration Building, AG-44
 *              Seattle, WA  98195
 *              Internet: MRC@CAC.Washington.EDU
 *
 * Date:        27 July 1988
 * Last Edited: 10 September 1998
 *
 * Sponsorship: The original version of this work was developed in the
 *              Symbolic Systems Resources Group of the Knowledge Systems
 *              Laboratory at Stanford University in 1987-88, and was funded
 *              by the Biomedical Research Technology Program of the National
 *              Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1998 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* Strange base64 encoding set used by the CD index */
static unsigned char oddity_64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-";

int cd_basis_encode64(unsigned char *outbuffer,unsigned char *inbuffer,int inlen,int outlen,unsigned char *basis)
{
  unsigned long outdex=0,s_count=inlen,index;
 
  for(index=0;s_count;inbuffer+=3)
  {   
    outbuffer[outdex++]=basis[inbuffer[0]>>2];
    outbuffer[outdex++]=basis[((inbuffer[0]<<4)+(--s_count?(inbuffer[1]>>4):0))&0x3f];
    outbuffer[outdex++]=s_count?basis[((inbuffer[1]<<2)+(--s_count?(inbuffer[2]>>6):0))&0x3f]:basis[64];
    outbuffer[outdex++]=s_count?basis[inbuffer[2]&0x3f]:basis[64];
    if(s_count)s_count--;
    if((++index)==15)
    {
      index=0;
      outbuffer[outdex++]='\n';
    }
  }
  outbuffer[outdex++]='\0';
  return outdex;
}

int cdindex_encode64(unsigned char *outbuffer,unsigned char *inbuffer,int inlen,int outlen)
{
  return cd_basis_encode64(outbuffer,inbuffer,inlen,outlen,oddity_64);
}
