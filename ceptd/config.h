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



/* Do your configuration in this file ! */


/*****************************************************************************/
#if defined(__bsdi__) || defined(BSD)

#undef NO_STRERROR    /* define NO_STRERROR if you don't have strerror func. */
#define HASTERMIOS    /* define HASTERMIOS if system has termios.h */
#undef	ASCIIPID      /* define if your system writes pid in ascii to
			 uucp-lock */

#endif /* __bsdi__ */
/*****************************************************************************/
#if defined(SUN)

#define NO_STRERROR   /* define NO_STRERROR if you don't have strerror func. */
#define HASTERMIOS    /* define HASTERMIOS if system has termios.h */
#define	ASCIIPID      /* define if your system writes pid in ascii to
			 uucp-lock */
#endif /* sun */
/*****************************************************************************/
#if defined(SVR4)
  
#undef NO_STRERROR    /* define NO_STRERROR if you don't have strerror func. */
#undef HASTERMIO    /* define HASTERMIOS if system has termios.h */
#define ASCIIPID      /* define if your system writes pid in ascii to
			 uucp-lock */

#endif /* SVR4 */
/*****************************************************************************/
#if defined(SCO) || defined(LINUX)
  
#undef NO_STRERROR    /* define NO_STRERROR if you don't have strerror func. */
#define HASTERMIOS    /* define HASTERMIOS if system has termios.h */
#define ASCIIPID      /* define if your system writes pid in ascii to
			 uucp-lock */

#endif /* SCO || LINUX */
/*****************************************************************************/











/*****************************************************************************/

#undef  LOGFILE "/tmp/btxlog" /* (un)define LOGFILE for session log */

#ifdef HASTERMIOS               /* For TERMIOS */
#define TERMIO termios
#else
#define TERMIO termio
#endif

#ifndef DEFAULTUSERSFILENAME
#define DEFAULTUSERSFILENAME NULL
#endif
#ifndef DEFAULTLOGFILENAME
#define DEFAULTLOGFILENAME NULL
#endif













/*****************************************************************************/


#ifdef HASTERMIOS               /* For TERMIOS */
#define TERMIO termios
#else
#define TERMIO termio
#endif

#ifndef DEFAULTUSERSFILENAME
#define DEFAULTUSERSFILENAME NULL
#endif
#ifndef DEFAULTLOGFILENAME
#define DEFAULTLOGFILENAME NULL
#endif
