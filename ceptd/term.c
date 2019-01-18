/*
 * Copyright (c) 1992, 1993 Arno Augustin, Frank Hoering, University of
 * Erlangen-Nuremberg, Germany.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	Erlangen-Nuremberg, Germany.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This software has not been validated by the ``Bundesamt fuer Zulassungen in
 * der Telekommunikation'' of the ``Deutsche Bundepost Telekom'' and thus
 * must not be used for accessing the BTX-Network of the Telekom in Germany.
 *
 * Diese Software hat keine Zulassung durch das Bundesamt fuer Zulassungen in
 * der Telekommunikation der Deutschen Bundespost Telekom und darf daher nicht
 * am Netz der Deutschen Bundespost Telekom in Deutschland betrieben werden.
 */


#include 	"config.h"
#include 	"cept.h"
#if LINUX
#	include        <sys/time.h>
#	include        <sgtty.h>
#endif
#ifdef 	HASTERMIOS
#	include 	<sys/termios.h>
#	define BBAUD 	speed_t
#else
#	include 	<sys/termio.h>
#	define BAUDMASK B0 | B50 |  B75 | B110 | B134 | B200 | B300 | B600 |\
			B1200 | B1800 | B2400 | B4800 | B9600 | B19200 | B38400
#	define BBAUD int
#endif  /* HASTERMIOS */

#ifndef SUN
#include <sys/ioctl.h>
#endif



static BBAUD gspeed();

/* get termio or termios structure for filedes. fd */
termget(fd,termio)
struct TERMIO 	*termio;
int 		fd;
{
#ifdef HASTERMIOS
   return tcgetattr(fd,termio);
#else
   return ioctl(fd, TCGETA, termio);
#endif
}

/* set termio or termios structure for filedes. fd */
termset(fd,termio)
struct TERMIO *termio; int fd;
{
#ifdef HASTERMIOS
   return tcsetattr(fd,TCSANOW,termio);
#else
   return ioctl(fd, TCSETA, termio);
#endif
}

/* set speed for filedes. fd, speed is the real speed value (int) */
setspeed(fd,speed)
int fd; int speed;
{
   struct TERMIO termio;
   BBAUD bbaud;

   if(-1 == (bbaud = gspeed(speed)) || -1 == termget(fd, &termio)) return -1;
#ifdef HASTERMIOS
   if(-1 == cfsetospeed(&termio, bbaud) || -1 == cfsetispeed(&termio, bbaud))
      return -1;
#else
   termio.c_cflag &= ~BAUDMASK;
   termio.c_cflag |= bbaud;
#endif
   if(-1 == termset(fd, &termio)) return -1;
   return 0;
}

termflush(fd)
int fd;
{
#ifdef HASTERMIOS
   tcflush(fd,TCIOFLUSH);
#else
   ioctl(fd, TIOCFLUSH, 0);
#endif
}

termctty(fd)      /* sets conrolling tty to get hangup signal */
int fd;
{
#ifdef TIOCSCTTY
     ioctl(fd, TIOCSCTTY,1);         /* modem gets controlling tty */
#else 
#  ifdef TIOCSPGRP 
     int s;
     s = getpgrp();		     /* i think this patch dosn't work */
     ioctl(fd, TIOCSPGRP, &s);       /* if someone has better ideas .... */
#  endif			     /* else nothing: hope that fd gets
				        controlling tty on that machine */
#endif
}

static BBAUD gspeed(baud)
int baud;
{
   switch(baud){

#ifdef B50
      case 50: return B50;
#endif

#ifdef B50
      case 75: return B75;
#endif

#ifdef B110
      case 110: return B110;
#endif

#ifdef B134
      case 134: return B134;
#endif

#ifdef B150
      case 150: return B150;
#endif

#ifdef B200
      case 200: return B200;
#endif

#ifdef B300
      case 300: return B300;
#endif
 
#ifdef B600
      case 600: return B600;
#endif
 
#ifdef B1200
      case 1200: return B1200;
#endif
 
#ifdef B1800
      case 1800: return B1800;
#endif

#ifdef B2400
      case 2400: return B2400;
#endif
 
#ifdef B4800
      case 4800: return B4800;
#endif
 
#ifdef B9600
      case 9600: return B9600;
#endif

#ifdef B19200
  case 19200: return B19200;
#else 
#ifdef EXTA
      case 19200: return EXTA;
#endif
#endif

#ifdef B38400
      case 38400: return B38400;
#else 
#ifdef EXTB
      case 38400: return EXTB;
#endif
#endif
   }
   return -1;
}      

