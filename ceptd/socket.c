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
#include 	<sys/types.h>
#include 	<sys/socket.h>
#include 	<netinet/in.h>
#include        <netdb.h>
#include        <stdio.h>

#define 	MAXCLIENTS 0
#ifndef 	NULL
#define 	NULL 0
#endif

static int 	comsock = -1;

int createsocket(port)				/* returns socket id */
int port;
{
   struct 	sockaddr_in sockaddr;

   if(-1 == (comsock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) ) 
      return -1;
   sockaddr.sin_port 		= htons( port );
   sockaddr.sin_addr.s_addr 	= INADDR_ANY;
   sockaddr.sin_family 		= AF_INET;
   if( bind(comsock, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) )  
      return -1;
   if( listen(comsock, MAXCLIENTS) ) 		/* max. MAXCLIENTS */
      return -1;
   return comsock;
}


char *getcallersname(sockfd)
int sockfd;
{
   struct sockaddr_in sockaddr;
   int addrlen = sizeof(struct sockaddr_in);
   struct hostent *hostent;

   
   if(-1 == getpeername(sockfd,(struct sockaddr *)&sockaddr,&addrlen))
      return 0;
   if(!(hostent = gethostbyaddr((char *) &sockaddr.sin_addr,
				sizeof(sockaddr.sin_addr), AF_INET)))
      return 0;
   return hostent->h_name;
}


checksecurity(usersfile,sockfd,who)
FILE *usersfile; int sockfd; char **who;
{
   char callerusername[40], *callerhostname;
   static char entry[120];
   char *hostname, *username,*c;
   int hostok = 0, nameok = 0;

   sprintf(entry,"error getting callers name !\000");
   *who=entry;
   if(!(callerhostname = getcallersname(sockfd))) return -1;
   rewind(usersfile);
   if(-1 == recv(sockfd,callerusername, 40, 0));

   if(usersfile){
      while((!nameok || !hostok) && fgets(entry,120,usersfile)){
	 for(c=entry; *c && !isgraph(*c); c++);
	 if(*c == '#' || *c == 0) continue;
	 if(*c == '@') {               /* this is an hostentry: '@hostname'*/
	    if(hostok) continue;
	    for(hostname = ++c; isgraph(*c); c++);
	    *c = 0; hostok = !strcmp(callerhostname,hostname);
	    continue;
	 }
	 else {
	    for(username = c; isgraph(*c) && *c != '@'; c++);
	    if(*c == '@') {            /* this is a name@hostname entry */
	       *c = 0;
	       if(strcmp(username,callerusername)) continue;
	       for(hostname = ++c; isgraph(*c) ; c++);
	       *c = 0;
	       if(strcmp(hostname,callerhostname)) continue;
	       hostok = nameok =1; break;       /* break while if entry found */
	    }
	    else {                     /* this is a name entry: 'name' */
	       if(nameok) continue;
	       *c = 0;
	       nameok = !strcmp(username,callerusername);
	       continue;
	    }
	 }
      }
   } else {
      hostok =1; nameok =1;
   }
   sprintf(entry,"%s@%s\000",callerusername,callerhostname);
   *who = entry;
   return (hostok && nameok)-1;
}
	 

int getclient()     /* wait for client and return socket file fd */
{
   int sockfd;
   
   if( (sockfd = accept(comsock,NULL,NULL)) == -1) return -1;
   return sockfd;
}

socketbye()
{
   return shutdown(comsock,2);
}


issock(s)
int s;
{
   int len;
   int val;

   len = sizeof(val);
   if(-1 == getsockopt(s,SOL_SOCKET, SO_TYPE, &val, &len)
      || val != SOCK_STREAM) return 0;
   return 1;
}
   




