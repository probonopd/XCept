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
#include        "../xcept/protocol.h"
#include 	"../xcept/version.h"
#include 	<sys/types.h>
#include 	<sys/wait.h>
#include 	<sys/stat.h>
#include        <stdlib.h>
#include 	<unistd.h>
#include 	<signal.h>
#include 	<stdio.h>
#include        <syslog.h>
#include 	<string.h>
#include 	<fcntl.h>
#include 	<time.h>
#include 	<sys/resource.h>
#include        <sys/errno.h>
#include	<sys/file.h>

FILE		*openinitfile();
static       	ceptd();
static void  	disconnect(),childsig();  /* signal routine for child  */
static void     hang_child(), waitchild();
static void	starttime(), initlog();
static int      endtime();
static void  	exit_btx_server();        /* signal routine for parent */
static pid_t   	childpid = -1;            /* pid of child process      */
static int      modem = -1, inetd = 0;
int             debug =0;
static char     *userlogfilename = DEFAULTLOGFILENAME;
static int	connected;
static char 	*caller;
int             connectstatus, isdnflag = 0;

main(argc,argv)
int argc;char *argv[];
{
   FILE 	*initfile;       /* fileptr for DEFAULTINITFILENAME*/
   FILE         *usersfile;	 /* fileptr for DEFAULTUSERSFILENAME*/
   char         *initfilename =   DEFAULTINITFILENAME;
   char         *usersfilename = DEFAULTUSERSFILENAME;
   int          port = DEFAULTSOCKETPORT;
   int  	i,userlog;
   extern int sockfd;

   umask(022);
   if(issock(0) && issock(1))  inetd =1,sockfd=0;
   
   initlog(argv[0]);

   for(i=1; i<argc; i++){
      if(argv[i][0] == '-') {
	 switch(argv[i][1]){
	  case 'd': debug = 1; break;
	  case 'p': if(i++ < argc) port = atoi(argv[i]);
	            else usage(argv[0]); break;
	  case 'f': if(i++ < argc) initfilename = argv[i];
	            else usage(argv[0]); break;
	  case 's': if(i++ < argc) usersfilename = argv[i];
	            else usage(argv[0]); break;
	  case 'u': if(i++ < argc) userlogfilename = argv[i];
	            else usage(argv[0]); break;
	  default:  usage(argv[0]);
	 }
      } else  usage(argv[0]);
   }
   
   if(!(initfile = fopen(initfilename,"r"))){
      log(LOG_CRIT,"Unable to open file '%s': %m - exiting !",initfilename);
      if(inetd) info("Unable to open file '%s':\n%m - exiting !",initfilename);
      usage(argv[0]);
   }
   if(!usersfilename) {
      usersfile=0;
   } else {
      if(!(usersfile = fopen(usersfilename,"r"))){
	 log(LOG_CRIT,"Unable to open file '%s': %m - exiting !",
	     usersfilename);
	 if(inetd) info("Unable to open file '%s':\n%m - exiting !",
			usersfilename);
	 usage(argv[0]);
      }
   }

   
   if(userlogfilename){
     if(-1 ==
	(userlog = open(userlogfilename,O_WRONLY | O_APPEND | O_CREAT,0666))){
	log(LOG_CRIT,"Unable to open file '%s': %m - exiting !",
	    userlogfilename);
	if(inetd)
	   info("Unable to open file '%s':\n%m - exiting !",userlogfilename);
	usage(argv[0]);
     }
     close(userlog);
   }
   
   /* if inetd starts server, do only what child normally does - dobtx never
    * returns
    */
   if(inetd) dobtx(initfile,usersfile,0); 
   else      ceptd(initfile,usersfile,port);
   exit(0);
}

usage(name)
char *name;
{
   if(inetd) info("Your server is wrong installed !");
   log(LOG_INFO,"\n*** %s Version %s ***\n",name,XCEPTVERSION);
   log(LOG_INFO,
   "Usage: %s [-p port] [-d] [-f filename] [-s filename] [-u filename]",
	   name);
   log(LOG_INFO,"\n-d \t\t\tadditional debug output");
   log(LOG_INFO,"-p port\t\t\tuse port number 'port'");
   log(LOG_INFO,"-f filename\t\tuse init filename 'filename'");
   log(LOG_INFO,"-s filename\t\tuse users filename 'filename'");
   log(LOG_INFO,"-u filename\t\tuse filename 'filename' to log clients");
   log(LOG_INFO,"\nDefaults:\nusers filename:\t %s",
       DEFAULTUSERSFILENAME ?
       DEFAULTUSERSFILENAME : "-- No permission check ! --");
   log(LOG_INFO,"init filename:\t %s\nport:\t\t %d",
	   DEFAULTINITFILENAME,DEFAULTSOCKETPORT);
   log(LOG_INFO,"log filename:\t %s",
       DEFAULTLOGFILENAME ? DEFAULTLOGFILENAME : "-- Not defined ! --");
   exit(1);
}
   

static void  exit_btx_server()
{
   log(LOG_DEBUG,"Kill child...");
   if(childpid != -1) kill(childpid,SIGTERM);
   log(LOG_DEBUG,"shutdown socket...");
   if (-1 == socketbye()) log(LOG_ERR,"Shutdown socket: %m");
   log(LOG_INFO,"Exiting");
   _exit(0);
}

static void childsig()
{
   wait(0);
}

static void hang_childs()
{
   int pid;
   do pid = waitpid(-1,(int *)0,WNOHANG);   /* POSIX */
   while(pid && pid != -1);
#if defined(SVR4) || defined(SCO)
   signal(SIGCHLD, hang_childs);
#endif
   
}

static void waitchild()                     /* wait for child to terminate */
{
   int 	st;
   
   do wait(&st);      
   while(!WIFEXITED(st) && !WIFSIGNALED(st));

}

static ceptd(initfile,usersfile,port)
FILE *initfile,*usersfile; int port;
{
   extern int 	sockfd;

   if(-1 == createsocket(port)) {
      log(LOG_ERR,"Create socket: %m");
      exit(1);
   }   
   signal(SIGINT,  exit_btx_server);
   signal(SIGTERM, exit_btx_server);
#if defined(SCO)
   signal(SIGCHLD, hang_childs);
#else
   {
      struct sigaction act;
      act.sa_handler = hang_childs;
      act.sa_mask.sa_sigbits[0] = 0;
      act.sa_mask.sa_sigbits[1] = 0;
      act.sa_mask.sa_sigbits[2] = 0;
      act.sa_mask.sa_sigbits[3] = 0;
      act.sa_flags =0;
      sigaction(SIGCHLD, &act, NULL);	 /* POSIX */
   }
#endif
   
   for(;;){
      if(-1 == (sockfd = getclient())){   /* wait for new client */
	 if(errno == EINTR) continue;
	 log(LOG_ERR,"Accept socket: %m");
	 exit_btx_server();
      }
      log(LOG_DEBUG,"Client accepted ");
      childpid = -1;
      switch (childpid = fork()) {
         case  0:
	    signal(SIGINT,  SIG_DFL);
	    signal(SIGTERM, SIG_DFL);
	    dobtx(initfile,usersfile,sockfd);  /* never returns (child) */
         case -1:
	    log(LOG_CRIT,"Fork failed: %m");
	    break;
/*         default:
	    waitchild();
	    childpid = -1; */
      }
      close(sockfd); 
      sleep(1);
   }
}


/*****************************************************************************
 *                               CHILD PROCESS                               *
 *****************************************************************************/
   
static void disconnect(sig,code)
int sig,code;
{
   if(!sig)
      log(LOG_DEBUG,"Disconnect pid: %d",getpid());
   else
       log(LOG_DEBUG,"Disconnect pid: %d signal: %d code: %d",
	      getpid(),sig,code);

   /* ignore 2nd signal when modem gets closed (SIGHUP) */
   signal(SIGPIPE, SIG_IGN);
   signal(SIGALRM, SIG_IGN);
   signal(SIGHUP,  SIG_IGN);
   signal(SIGTERM, SIG_IGN);
   signal(SIGINT,  SIG_IGN);
   switch(sig){
    case SIGALRM: status(TIMEOUT); break;
    case SIGHUP:  status(HANGUP); break;
    case SIGTERM: 
    case SIGINT:  info("Server killed !\n"); break;
   }
   closelogfile();
#ifdef ISDN
   if(isdnflag){
      closeisdn();
      if(connected) status(ISDNDISCONNECT);
   }
   else
#endif   
   modemclose();                  /* hangup and close modem fd */
   endtime();                          /* end of session time */
   exit(0);
}

dobtx(initfile, usersfile, sockfd)    /* child process only */
FILE *initfile, *usersfile; int sockfd;
{
   if(-1 == checksecurity(usersfile,sockfd,&caller)){
      log(LOG_NOTICE,"%s: - no permission -",caller);
      status(NOPERMISSION);
      /* info("%s:\n- no permission -",caller); */
      exit(1);
   } else {
      log(LOG_DEBUG,"%s - accepted",caller);
   }
   starttime();
   signal(SIGPIPE, disconnect);
   signal(SIGALRM, disconnect);
   signal(SIGTERM, disconnect);
   signal(SIGINT,  disconnect);
   connected = 0;
   if(-1 != (modem = modeminit(initfile)) ) {/* open modem and dial */
      connected =1;
      signal(SIGHUP,  disconnect);
      starttime();
#ifdef ISDN      
      if(isdnflag) doisdn(modem,sockfd); 
      else
#endif      
      level2(modem,sockfd);         /* btx data link level protocol */
      
   }
   else starttime();
   
   disconnect(0,0);
}

/*****************************************************************************
 *                               USER LOG                                    *
 *****************************************************************************/
static time_t   start_time, end_time;


static void starttime()
{
   start_time = time(NULL);
}


static int endtime()
{
   char 	*timestr,*str,tstr[40];
   time_t 	diff;
   char statusstr[40];
   char line[256];
   int userlog;
   
   if(!userlogfilename) return 0;
   end_time = time(NULL);
   timestr = ctime(&start_time);
   str = tstr;
   while(*str = *timestr){ if(*timestr != '\n') str++; timestr++; }
      
   diff = end_time - start_time;
   switch(connectstatus){
    case NOCARRIER: strcpy(statusstr,"NO CARRIER"); break;
    case BUSY: strcpy(statusstr,"LINE BUSY"); break;
    case NODIALTONE: strcpy(statusstr,"NO DIALTONE"); break;
    case NOPERMISSION: strcpy(statusstr,"NO PERMISSION"); break;
    case TIMEOUT: strcpy(statusstr,"TIMEOUT"); break;
    case OPENFAIL: strcpy(statusstr,"MODEM BUSY"); break;
    case ISDNOPENFAIL: strcpy(statusstr,"ISDN OPENFAIL"); break;
    case ISDNBUSY: strcpy(statusstr,"ISDN BUSY"); break;
    case HANGUP: strcpy(statusstr,"HANGUP"); break;
    case ABORT:  strcpy(statusstr,"ABORTED"); break;
    default: strcpy(statusstr,"???"); break;
   }
   
   sprintf(line,"%s  %14s  %4d:%02d:%02d   %s\n\000",
	   tstr,connected ? "CONNECTED" : statusstr,diff/3600,
	   (diff/60)%60,diff%60,caller);

   if(-1 == (userlog = open(userlogfilename,O_WRONLY | O_APPEND |
			    O_CREAT/* | O_EXLOCK */))){
      log(LOG_CRIT,"Unable to open file '%s': %m - exiting !",userlogfilename);
      info("Unable to open file '%s':\n%m - exiting !",userlogfilename);
      return -1;
   }
#ifdef SVR4
   lockf(userlog,F_LOCK,0);
#else
   flock(userlog, LOCK_EX); /* to avoid problems with other server processes */
#endif
   lseek(userlog, 0, SEEK_END); 
   write(userlog,line,strlen(line));
#ifdef SVR4
   lockf(userlog,F_ULOCK,0);
#endif
   close(userlog);
   return 0;
}

/*****************************************************************************
 *                               ERROR LOG                                   *
 *****************************************************************************/

static int      debugfd = -1;             /* fd to DEBUG file */

/*

#ifdef NO_STRERROR
char *strerror(err)
int err;
{
   static char array[30];
   sprintf(array,"Errno: %d",err); 
   return array;
}
#endif
*/

#ifdef NO_STRERROR
char *strerror(err)
int err;
{
   extern char *sys_errlist[];
   return sys_errlist[err];
}
#endif

static void initlog(name)
char *name;
{
   if(inetd){
      openlog(name,LOG_CONS | LOG_NDELAY | LOG_PID, LOG_DAEMON);
      setlogmask(LOG_UPTO(LOG_DEBUG));
   }
   else{
      debugfd = 2;
   }
}

log(priority,fmt,a,b,c,d,e,f,g,h,i,j)
int priority,a,b,c,d,e,f,g,h,i,j;
char *fmt;
{
   if(!debug && priority == LOG_DEBUG) return;

   if(inetd) syslog(priority,fmt,a,b,c,d,e,f,g,h,i,j);
   else
   {
      char fmtstr[256],strout[256];
      char *s, *er;

      fmtstr[0]=0;
      /*sprintf(fmtstr,"ceptd[%d]: ",getpid()); */
      s = fmtstr+strlen(fmtstr);
      while(*fmt) {
	 if(*fmt != '%') *s++ = *fmt++;
	 else {
	    if(*(fmt+1) == 'm'){
	       er = strerror(errno);
	       while(*s = *er++) s++;
	       fmt += 2;
	    } else {
	       *s++ = *fmt++; 
	       if(*(fmt+1)) *s++ = *fmt++;
	    }
	 } 
      }
      *s = 0;
      sprintf(strout,fmtstr,a,b,c,d,e,f,g,h,i,j);
      strcat(strout,"\n");
      write(debugfd,strout,strlen(strout));
   }
}


