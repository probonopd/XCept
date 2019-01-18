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
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <pwd.h>


int callbtx(hostname, port)
char *hostname;
int port;
{
   struct passwd *passwd;
   struct sockaddr_in sockaddr;
   struct hostent *host;
   int sock;
   
   if(-1 == (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)))  return -1;
   if( !(host = gethostbyname(hostname)) )   return -1;
   bcopy(host->h_addr, &sockaddr.sin_addr, host->h_length);
   sockaddr.sin_port = htons(port);
   sockaddr.sin_family = AF_INET;         /* internet */
   if( -1 == connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr))) 
      return -1;

   if(!(passwd = getpwuid(getuid())))  return -1;
   send(sock, passwd->pw_name, strlen(passwd->pw_name)+1, 0);

   return sock;
}



