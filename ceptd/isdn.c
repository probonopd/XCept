int isdn_fd = -1;

#ifdef ISDN    /* this module only if ISDN is defined */

#include "config.h"
#include "cept.h"
#include "../xcept/protocol.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <syslog.h>
#include <tiuser.h>
#include <errno.h>
#define IOBUFSIZE 1024

extern char *t_errlist[];
extern int t_errno;

static int socket_to_isdn(), isdn_to_socket();

int openisdn(device,port)
char *device, *port;
{
   struct t_call call, rcall, *pcall;
   extern int t_errno;
   struct t_info info;
   struct t_bind *tbind;

   alarm(80);    /* wait 80 sec for connection */
   if(-1 == (isdn_fd = t_open(device, O_RDWR, &info))){
      log(LOG_INFO,"Unable to open %s - %s !\n",device, t_errlist[t_errno]);
      status(ISDNOPENFAIL);
      return -1;
   }
   if(t_bind(isdn_fd, NULL, NULL) < 0){
      log(LOG_INFO,"Unable to bind ISDN (%s) - %s !\n",device,
	  t_errlist[t_errno]);
      closeisdn();
      status(ISDNOPENFAIL);
      return -1;
   }
   memset(&call,0,sizeof(call));
   call.addr.buf = port;
   call.addr.len = call.addr.maxlen = strlen(port)+1;
   if(t_connect(isdn_fd, &call, &rcall) <0 ){
      log(LOG_INFO,"Unable to connect ISDN (%s) to port %s - %s !\n"
	  ,device, port, t_errlist[t_errno]);
      closeisdn();
      status(ISDNBUSY);
      return -1;
   }
   log(LOG_DEBUG,"port: %s fd: %d\n",port,isdn_fd);
   alarm(0);
   status(ISDNCONNECT);
   return isdn_fd;
}

closeisdn()
{
   if(isdn_fd != -1){
      t_snddis(isdn_fd,0); /* send disconnect */
      t_close(isdn_fd);
   }
   /* status(ISDNDISCONNECT); */
   return 0;
}

doisdn(isdnfd,sockfd)
int    isdnfd,sockfd;
{
   int 		max;
   fd_set 	fdset;
   max = MAX( isdnfd, sockfd ) + 1;
   for(;;) {
      FD_ZERO( &fdset );
      FD_SET( isdnfd,  &fdset );
      FD_SET( sockfd, &fdset );
      if( -1 == select( max, &fdset, NULL, NULL, NULL ) ) {
	 log(LOG_DEBUG," select %m\n"); return 1;
      }
      if( FD_ISSET( isdnfd,  &fdset) )
      if( -1 == isdn_to_socket(isdnfd, sockfd) ) {
	 log(LOG_INFO," isdn_sock %m\n"); return 1;
      }
      if( FD_ISSET( sockfd, &fdset) )
      if(socket_to_isdn(isdnfd, sockfd) <= 0){
	 log(LOG_INFO," sock_isdn %m\n"); return 1;
      }
   }
   
}


static socket_to_isdn(isdnfd, sockfd)
int isdnfd, sockfd;
{
   char 	 		buffer[IOBUFSIZE];
   register int  		cnt;
   
   if(-1 == (cnt = read( sockfd, buffer+2, IOBUFSIZE-2))) return -1;
   buffer[0]=1; buffer[1]=0;         /* Header bytes */
   if(cnt && t_snd(isdnfd, buffer, cnt+2, 0) < 0){
      log(LOG_DEBUG,"t_snd - %s !\n", t_errlist[t_errno]);
      return -1;
   }
   
   return cnt;
}


static isdn_to_socket(isdnfd, sockfd)
int isdnfd, sockfd;
{
   unsigned char buffer[IOBUFSIZE], *ptr=buffer;
   int i=0, j=0, cnt;

   if((cnt = t_rcv(isdnfd, buffer, IOBUFSIZE, &i)) <0){
      log(LOG_DEBUG,"t_rcv - %s !\n", t_errlist[t_errno]);
      return -1;
   }

   while(j<cnt) {
      while(buffer[j]<8 && j<cnt) j++;
      i = j;
      while(buffer[j]>=8 && j<cnt) j++;
      if(i != j && write(sockfd, buffer+i, j - i) <0) return -1;
   }

   return cnt;
}


#endif ISDN
