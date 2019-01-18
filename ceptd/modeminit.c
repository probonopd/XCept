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


#include "config.h"
#include "cept.h"
#include "../xcept/protocol.h"
#include 	<sys/types.h>
#include	<sys/time.h>
#include 	<unistd.h>
#include        <stdio.h>
#include        <errno.h>
#include        <fcntl.h>
#include        <signal.h>
#include        <syslog.h>
#include        <string.h>
#ifdef HASTERMIOS
#   include        <sys/termios.h>
#else
#   include        <sys/termio.h>
#endif /* HASTERMIOS */

#ifdef SVR4
#	include      <sys/stat.h>
#	include      <sys/mkdev.h>
#endif

#if LINUX
#	include        <sgtty.h>
#endif

#define MIMAXCHARS 80 
static char          hangupstring[MIMAXCHARS]= "~+++~ATH0\r";
static char          nocarrierstring[MIMAXCHARS]= "NO CARRIER\r";
static char          busystring[MIMAXCHARS]= "BUSY\r";
static char          nodialtonestring[MIMAXCHARS]= "NO DIALTONE\r";
static char          connectstring[MIMAXCHARS]= "CONNECT 2400\r";
static char          devicename[MIMAXCHARS]  = DEFAULTMODEM;
static int           speed                   = DEFAULTSPEED;          

static struct TERMIO oldpar;
static int modem = -1;
static dial_btx(), changestring(), sendmodem();

extern int errno;
extern int connectstatus, isdnflag;

#ifdef USE_LOCK
#include <string.h>
#include <sys/stat.h>

static char    lock[256];
static int      makelock();
static int 	 readlock();
static int      checklock();
static void     rmlocks();
#ifndef TRUE
#define TRUE -1
#define FALSE 0
#endif
#define FAIL -1
#define SUCCESS 0
#endif

/* modeminit: set up modem and try to call btx center, if btx not busy and 
 * on successful completition returns modem fd (-1 on error)
 */


modeminit(initfile)
FILE *initfile;
{

   modem = -1;
   isdnflag = 0;
   if( -1 == dial_btx(initfile) ) {
      log(LOG_DEBUG,"Dial failed");
      return -1;
   }
   if(isdnflag) return modem;
   if(modem == -1) {
      log(LOG_NOTICE,"No device !");
      info("No device !");
      return -1;
   }
   sleep(1); /* time to wait for carrier line from modem */
   unsetcflags(modem, CLOCAL);

   /* showlamps(modem); *//* this is for debug only */
   return modem;
}


/* modemclose: hangup ,reset device parameters and close modem fd
 *
 */
modemclose()
{
   (void) setsid ();   /* give up controling terminal modem */
   if(modem != -1){
      sendmodem(hangupstring);
      termflush(modem);  /* get rid of garbage */
      /* ioctl(modem, TIOCCDTR, 0); */ /* clear DTR */
      termset(modem,&oldpar);
      if(-1 == close(modem))
         log(LOG_NOTICE,"Unable to close device %s : %m",devicename);
      modem = -1;
   }
#ifdef USE_LOCK
      rmlocks();
#endif
}


/* modemopen: open device (devicename) with speed (speed) and return
 * modem fileds. on success
 */

int modemopen(devname,devspeed)
char *devname; int devspeed;
{
   struct TERMIO newpar;
#ifdef USE_LOCK
   char *device;   
#endif
   int s;

   if(-1 == setsid())
      log(LOG_ERR,"Can't setsid : %m");
#ifdef USE_LOCK
   device = strrchr(devname, '/');
   device++;
#  ifdef SVR4
   {
      struct stat dstat;
      if(stat(devname,&dstat) != 0) {
	 log(LOG_INFO, "Stat %s failed", device);
	 status(OPENFAIL);
	 goto error;
      }
      sprintf(lock,"%s/LK.%03lu.%03lu.%03lu",USE_LOCK,
	      (unsigned long) major(dstat.st_dev),
	      (unsigned long) major(dstat.st_rdev),
	      (unsigned long) minor(dstat.st_rdev));
   }
#  else   
   strcpy(lock,USE_LOCK);
   strcat(lock,"/LCK..");
   strcat(lock,device);
#  endif /* SVR4 */
   log(LOG_DEBUG,"lock: %s\n",lock);
   if (checklock(lock) == TRUE) {
      log(LOG_INFO, "Open %s failed --- already locked", device);
      status(OPENFAIL);
      goto error;
   } else if (makelock(lock) == FAIL) {
      log(LOG_INFO, "Locking %s failed: %m", device);
      status(OPENFAIL);
      goto error;
   }
#endif /* USE_LOCKS */


   if( -1 == (modem = open(devname,/* O_EXCL  |*/ O_RDWR | O_NDELAY))){
      log(LOG_INFO,"Open %s failed: %m",devname);
      goto error;
   }

   if(-1 == termget(modem,&oldpar)) goto error;
   oldpar.c_cflag |= HUPCL;
   newpar=oldpar;
   newpar.c_cflag  = CS8 | HUPCL | CREAD | CLOCAL; 

   newpar.c_iflag = IGNBRK|IGNPAR;      /* ignore ^C and parity errors */
   newpar.c_oflag = 0; 
   newpar.c_lflag = 0; 
   newpar.c_cc[VMIN] = 1;               /* read satisfied with 1 char */
   newpar.c_cc[VTIME] = 0;              /* no timeout on read */

   if(-1 == termset(modem,&newpar))   goto error;
   if(-1 == setspeed(modem,devspeed)) goto error;
   termflush(modem);
   termctty(modem);      /* modem get controling tty (to get HANGUP-signal) */

   /* showlamps(modem); */
   return modem;    
 error:
  log(LOG_INFO,"Initalize modem (%s) failed : %m",devname);
  status(OPENFAIL);
  /* info("Modem: (%s) :\n%m",devname); */
  return -1;
}

setcflags(fd,flags)
int fd,flags;
{
   struct TERMIO newpar;

   if(-1 == termget(fd,&newpar)) goto error;
   newpar.c_cflag  |= flags;                     /* add flags */
   if(-1 == termset(fd,&newpar)) goto error;
   return 1;
 error:
  log(LOG_INFO,"set line (cflags) failed : %m");
  status(OPENFAIL);
  return -1;
}   

unsetcflags(fd,flags)
int fd,flags;
{
   struct TERMIO newpar;

   if(-1 == termget(fd,&newpar)) goto error;
   newpar.c_cflag  &= ~flags;                    /* clear flags */
   if(-1 == termset(fd,&newpar)) goto error;
   return 1;
 error:
  log(LOG_INFO,"set line (cflags) failed : %m");
  status(OPENFAIL);
  return -1;
}   




      

static int dial_btx(initfile)
FILE *initfile;
{
   extern FILE *yyin;
   int line;

   yyin = initfile;
   rewind(yyin);
   if(line = dolex()){
      log(LOG_INFO,"Initfile line %d",line);
      return -1;
   }
}

/* ****** caled from lex.yy.c **********/
sethangupstring(str)
char *str;
{
   log(LOG_DEBUG,"hangup: %s",str);
   strncpy(hangupstring,str,MIMAXCHARS);
   changestring(hangupstring);
   
}


setnocarrierstring(str)
char *str;
{
   log(LOG_DEBUG,"nocarrier: %s",str);
   strncpy(nocarrierstring,str,MIMAXCHARS);
   changestring(nocarrierstring);
   
}

setconnectstring(str)
char *str;
{
   log(LOG_DEBUG,"connect: %s",str);
   strncpy(connectstring,str,MIMAXCHARS);
   changestring(connectstring);
   
}

setbusystring(str)
char *str;
{
   log(LOG_DEBUG,"busy: %s",str);
   strncpy(busystring,str,MIMAXCHARS);
   changestring(busystring);
   
}

setnodialtonestring(str)
char *str;
{
   log(LOG_DEBUG,"nodialtone: %s",str);
   strncpy(nodialtonestring,str,MIMAXCHARS);
   changestring(nodialtonestring);
   
}

int setbaud(baud)  /* setbaud with real baud rate (as int) */
int baud;
{
   log(LOG_DEBUG,"Baud: %d",baud);
   speed = baud;
   if(-1 != setspeed(modem,speed)) return 0;
   log(LOG_NOTICE,"Set baud rate (%s) %m",devicename);
   return -1;
}

opendevice(name,baud)
char *name; int baud;
{
   strncpy(devicename,name,MIMAXCHARS);
   changestring(devicename);
   speed = baud;
   if(modem == -1) modem = modemopen(devicename,speed);
   return modem;
}

sendstring(str)
char *str;
{
   changestring(str);
   log(LOG_DEBUG,"Send: %s",str);
   return sendmodem(str);
}

wsleep(waittime)
int waittime;
{
   log(LOG_DEBUG,"Sleep: %d.%d sec",waittime/10,waittime%10);
#if defined(SCO) || defined(SVR4)
   sleep(waittime/10);
#else    
   usleep(waittime*100000);
#endif   
}

isdn(device,port) /* this initializes modem fd as isdn fd if possible */
char *device, *port;
{
  isdnflag=1;
# ifdef ISDN
  log(LOG_DEBUG,"%s %s",device,port);
  changestring(port);
  changestring(device);
  modem = openisdn(device,port);
  return modem;
# else
  log(LOG_ERR,"ISDN not configured in this server (%s) !\n",port);
  return -1;
# endif
}

/*************************************************************************/

#define NSTRINGS 4

waitconnect(tmout)
int tmout;
{
   char s[MIMAXCHARS], *str[NSTRINGS];
   register char *st;
   int pos =0, len[NSTRINGS], maxlen = 0;
   register int k,j,i;
   
   log(LOG_DEBUG,"Waitconnect: wait %2ds for connection",tmout);

   str[0]=nocarrierstring;
   str[1]=busystring;
   str[2]=nodialtonestring;
   str[3]=connectstring;

   for(i=0; i<NSTRINGS; i++){
      len[i] = strlen(str[i]);
      if(len[i] > maxlen) maxlen = len[i];
   }
   memset(s,0,maxlen);
   alarm(tmout);

   
   for(pos=0;;){
      if(readmodem(s+pos,1) <1){
	 alarm(0);
	 log(LOG_ERR,"Wait: Error: %m ");
	 return -1;
      }
      for(j=0; j<NSTRINGS; j++){
	 st=str[j];
	 for(k=0, i=len[j]-1; i >= 0; i--, k++)
	    if(s[(maxlen+pos-k)%maxlen] != st[i]) break;
	 if(i == -1){
	    alarm(0);
	    switch(j){
	     case 0:   status(NOCARRIER); 	return -1;
	     case 1:   status(BUSY); 		return -1;
	     case 2:   status(NODIALTONE);	return -1;
	     case 3:   status(CONNECT);		return 1;
	    }
	    return 1;
	 }
      }
      if(++pos == maxlen) pos = 0;
   }
}
   
   

static changestring(str)
char *str;
{
   register char *newstr = str;

   while(*newstr = *str++){
      switch(*newstr){
         case '"': break;           /* skip '"' */
         case '^':  if(*str) *newstr++ = *str++ - '@'; break; /* quote */
         case '\\': if(*str) *newstr++ = *str++; break;
         default:   newstr++;
      }
   }
}

static sendmodem(str)
char *str;
{
   while(*str){
      switch(*str){
         case '~': sleep(1); break;  /* wait 1 sec */
         default:  if(modem != -1) write(modem,str,1); 
      }
      str++;
   }
   return 1;
}

/* read from modem and flush input from socket */

readmodem(buffer,len)
int len;  char *buffer;
{
   extern int sockfd;
   int ret;
   int 		max;
   char 	dummy[100];
   fd_set 	fdset;

   max = MAX( modem, sockfd ) + 1;
   for(;;) {

      FD_ZERO( &fdset );
      FD_SET( modem,  &fdset );
      FD_SET( sockfd, &fdset );
      if( -1 == (ret = select( max, &fdset, NULL, NULL, NULL )) ){
	 connectstatus = ABORT;
	 return -1;
      }
      if( FD_ISSET( modem,  &fdset) ) return read(modem,buffer,len);
      if( FD_ISSET( sockfd, &fdset) ){
	 if(read(sockfd, dummy, 100) <= 0) {
	    connectstatus = ABORT;
	    return -1;
	 }
	 continue;
      }
   }  
}

   
#if 0
showlamps(m)
int m;
{
   int s;
   ioctl(m,TIOCMGET,&s);  /* get status of modem */
   log(LOG_DEBUG,"DTR DSR RTS CTS CD  ST  SR  RI  LE");
   log(LOG_DEBUG,"%1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d",
       !!(s&TIOCM_DTR), !!(s&TIOCM_DSR), !!(s&TIOCM_RTS), !!(s&TIOCM_CTS),
       !!(s&TIOCM_CD ), !!(s&TIOCM_ST ), !!(s&TIOCM_SR ),  !!(s&TIOCM_RI),
       !!(s&TIOCM_LE ));
}
#endif


/******************************* uucp file locking *************************/

#ifdef USE_LOCK
/* taken from agetty.c */
/*
**	makelock() - attempt to create a lockfile
**
**	Returns FAIL if lock could not be made (line in use).
*/
static char *tempname;

static int makelock(name)
char *name;
{
   int fd, pid;
   char buf[256+1];
#ifdef	ASCIIPID
   char apid[16];
#endif	/* ASCIIPID */
   int getpid();
   char *mktemp();
   
   /* first make a temp file
    */
   strcpy(buf,USE_LOCK);
   strcat(buf,"/LCK..");
   strcat(buf,"TM.YXXXXXX");
   if ((fd = creat((tempname=mktemp(buf)), 0444)) == FAIL) {
      return(FAIL);
   }
   
   /* put my pid in it
    */
#ifdef	ASCIIPID
   (void) sprintf(apid, "%09d", getpid());
   (void) write(fd, apid, strlen(apid));
#else
   pid = getpid();
   (void) write(fd, (char *)&pid, sizeof(pid));
#endif	/* ASCIIPID */
   (void) close(fd);
   
   /* link it to the lock file
    */
   alarm(100); /* to avoid that process stucks */
   while (link(tempname, name) == FAIL) {
      if (errno == EEXIST) {		/* lock file already there */
	 if ((pid = readlock(name)) == FAIL){
	    sleep(5);
	    continue;
	 }
	 if ((kill((pid_t)pid, 0) == FAIL) && errno == ESRCH) {
	    /* pid that created lockfile is gone */
	    (void) unlink(name);
	    sleep(5);
	    continue;
	 }
      }
      (void) unlink(tempname);
      alarm(0);
      return(FAIL);
   }
   (void) unlink(tempname);
   alarm(0);
   return(SUCCESS);
}

/*
**	checklock() - test for presense of valid lock file
**
**	Returns TRUE if lockfile found, FALSE if not.
*/

static int checklock(name)
char *name;
{
   int pid;
   struct stat st;
   
   if ((stat(name, &st) == FAIL) && errno == ENOENT) {
      return(FALSE);
   }

   if ((pid = readlock(name)) == FAIL) {
      return(FALSE);
   }
   
   if ((kill((pid_t)pid, 0) == FAIL) && errno == ESRCH) {
      (void) unlink(name);
      return(FALSE);
   }
   
   return(TRUE);
}

/*
**	readlock() - read contents of lockfile
**
**	Returns pid read or FAIL on error.
*/

static int readlock(name)
char *name;
{
   int fd, pid;
#ifdef	ASCIIPID
   char apid[16];
#endif	/* ASCIIPID */

   if ((fd = open(name, O_RDONLY)) == FAIL) return(FAIL);

#ifdef	ASCIIPID
   (void) read(fd, apid, sizeof(apid));
   (void) sscanf(apid, "%d", &pid);
#else
   (void) read(fd, (char *)&pid, sizeof(pid));
#endif	/* ASCIIPID */
   
   (void) close(fd);
   return(pid);
}

/*
**	rmlocks() - remove lockfile(s)
*/

static void rmlocks()
{
   if(getpid() == readlock(lock)) (void) unlink(lock);
   (void) unlink(tempname);
}


#endif




