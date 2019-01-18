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
#include 	<unistd.h>
#include	<sys/types.h>
#include	<signal.h>
#include	<stdio.h>
#include	<sys/time.h>
#include        <syslog.h>


static int     	modem;
int             sockfd;

#define NOBLOCK    0        /* not receiving a block */
#define TEXTBLOCK  1        /* normal text block     */
#define IMAGEBLOCK 2        /* transparent block     */
#define ERROR      0        /* status error          */

static unsigned short 	crc=0,getcheck();
static 		        calccrc(), socketinput(), modeminput();
static 		        putmodem(), getmodem();

static FILE             *logfile = 0;
static FILE             *outlogfile = 0;
static unsigned int     readbyte;
static int              logged = 0;

level2(modemd,sockd)
int modemd,sockd;
{
   int 		max;
   fd_set 	fdset;

   modem  = modemd;
   sockfd = sockd;
   logged = 0;
   readbyte = 0; openlogfile();
   max = MAX( modem, sockfd ) + 1;
   for(;;) {
      FD_ZERO( &fdset );
      FD_SET( modem,  &fdset );
      FD_SET( sockfd, &fdset );
      if( -1 == select( max, &fdset, NULL, NULL, NULL ) ) return 1;
      if( FD_ISSET( modem,  &fdset) )  if( -1 == modeminput() ) return 1;
      if( FD_ISSET( sockfd, &fdset) )  if(socketinput() <= 0)   return 1;
   }
      
}

static socketinput()
{
   static char 	 		buffer[100];
   register int  		cnt;
   
   cnt = read( sockfd, buffer, 100 ); 
   if(cnt >0){
      write( modem, buffer, cnt);
   }
      
   return cnt;
}

/* modeminput: this is data link layer (level 2) for getting blocks from
 * btx, crc check is done and characters are send to client
 */
#define 	BLOCKBUFSIZ 2048
static modeminput()
{
   static unsigned int		block = NOBLOCK, bufferlen = 0;
   static unsigned int		tfi = 0;
   static unsigned char 	ack01 = '0',st = EOT;
   static unsigned char 	buffer[BLOCKBUFSIZ];
   static unsigned char 	c = 0;
   static unsigned char         lastchar;
   int 				c1;
   
   if(-1 == (c1 = getmodem()) ) return -1;
   c = c1;

   switch( block ) {
      case TEXTBLOCK:
         calccrc(c);
         switch(c) {
            case DLE:
            case NAK:
            case ACK:
            case SOH: log(LOG_DEBUG,"Bad char:%x",c1); break;
            case STX:
	      if(st == ERROR || st == ETB){
		 crc = 0; bufferlen = 0;
		 st = EOT;
	      }
	      break;
            case EOT: ack01 = '0'; block = NOBLOCK; break;
            case ITB: 
            case ETB:
            case ETX:
               if(getcheck() || st == ERROR){ 
                  bufferlen = 0; crc = 0; putmodem(NAK); st = ERROR;
		  break;
               }
	       if(bufferlen){
		  if(-1 == write( sockfd, buffer, bufferlen )) return -1;
#                 ifdef LOGFILE
		  write(fileno(logfile),buffer,bufferlen);
#                 endif
		  bufferlen = 0;
	       }
               st = c;
               if(c == ITB) putmodem(ACK);
               else {
                  ack01 ^= 1;  putmodem(DLE);  putmodem(ack01);
                  if(c == ETX) block = NOBLOCK;  /* end of textblock */
               }
               break;
            case ENQ:
               switch( st ) {
                  case ERROR: putmodem(NAK);  break;
		  case EOT:   putmodem(NAK);  break;
                  case ITB:   putmodem(ACK);  putmodem(NAK); break;
                  case ETX: 
                  case ETB:   putmodem(DLE);  putmodem(ack01);
		              putmodem(NAK); break;
               }
   	       break;
            default: 
               if(bufferlen < BLOCKBUFSIZ-1) buffer[bufferlen++] = c;
               else log(LOG_NOTICE,"Blockbuffer overflow !");
               break;
         }
	 break;
       default:     /* NOBLOCK */
         st = EOT;
	 if(tfi){
	    if(tfi == SOH) { lastchar = c; tfi = ENQ; }
	    else           { if(c == ENQ) send_TFI(); tfi=0;}
	 }
         switch(c) {
	  case EOT: 	ack01 = '0'; break;
	  case SOH:     tfi = SOH;
	  case STX: 	crc = 0;  block = TEXTBLOCK; bufferlen = 0;  break;
	  default: 	if(-1 == putsocket(c)) return -1;
         }
         break;
   }
   return 1;
}

send_TFI()
{
   putmodem(SOH);
   putmodem('@');  /* @ = intermediate blocksize */
   putmodem('5');  /* 5 = max. 256 bytes         */
   putmodem(ETX);
}
   
/* get 2 bytes from modem and calccrc, return value is crc (should be 0 if
 * no crc error), during check handle socket inputs
 */ 
static unsigned short getcheck()
{
   int 			max,i=0;
   unsigned char 	c;
   fd_set 		fdset;

   max = MAX( modem, sockfd ) + 1;
   
   while(i<2) {                /* get 2 checkbytes */
      FD_ZERO( &fdset );
      FD_SET( modem,  &fdset );
      FD_SET( sockfd, &fdset );
      if(-1 == select( max, &fdset, NULL, NULL, NULL) ) return -1;
      if( FD_ISSET( modem,  &fdset) )  {
	 c=getmodem();
	 calccrc(c); i++;
      }
      if( FD_ISSET( sockfd, &fdset) )  socketinput();
   }
   if(crc) log(LOG_DEBUG,"CRC error %x",(int)crc) ;
   return crc;
      
}
   

static calccrc(c)
unsigned int c;
{

   register unsigned int 	cr = crc, ch = c;
   register unsigned int 	i,bit;

   for(i=0; i<8; i++){           /* for all bits in character c */
      bit = (cr & 1) ^ (ch & 1);
      cr >>= 1;                  /* bit 15 now cleared */
      if(bit) cr ^= 0xa001;      /* xor for polynom: x^16+x^15+x^2+x^0 */
      ch >>= 1;                  /* shift character for next bit */
   }
   crc=cr;
}

/******************************* I/O *****************************************/

int putsocket(c)
char c;
{
#  ifdef LOGFILE
   write(fileno(logfile), &c, 1 );
#  endif
   
   return write(sockfd, &c, 1 );
}

static int putmodem(c)
char c;
{
#ifdef LOGFILE
   if(outlogfile) fprintf(outlogfile,"%08x: %02x\n",readbyte,c);
#endif
   return write( modem, &c, 1 );

}

static int getmodem()
{
   unsigned char 	c;

   if( read( modem, &c, 1 ) <1) return -1;
#ifdef LOGFILE
/*   if(logfile) putc(c,logfile); */
   readbyte++;
#endif
   return c;
}


#if 0

static int getmodem()
{
   static unsigned char c[100];
   unsigned char retc;
   static bufferp=0, bufferl=0;
   
   if(!bufferl){
      if((bufferl = read(modem,c,100)) <1) return -1;
   }
   retc = c[bufferp++];
   if(bufferp == bufferl) { bufferp = 0;  bufferl =0;}
#ifdef LOGFILE
 /*  if(logfile) putc(retc,logfile); */
#endif
   return retc;

}
#endif

openlogfile()
{

#ifdef LOGFILE
   char outfilename[100];
#endif
   logfile = 0;
   outlogfile =0;
#ifdef LOGFILE
   if(!(logfile = fopen(LOGFILE,"w"))) {
      log(LOG_NOTICE,"Unable to open logfile \'%s\' : %m", LOGFILE);
   } else {
      setbuf(logfile,NULL);
   }
   strcpy(outfilename,LOGFILE); strcat(outfilename,".OUT");
   if(!(outlogfile = fopen(outfilename,"w"))) {
      log(LOG_NOTICE,"Unable to open logfile \'%s%s\' : %m",LOGFILE,".OUT");
   } else {
      setbuf(outlogfile,NULL);
   }
#endif
}

closelogfile()
{
   if(logfile) fclose(logfile);
   if(outlogfile) fclose(outlogfile);
   logfile =0;
   outlogfile =0;
}


status(number)
int number;
{
   unsigned char seq[3];
   extern connectstatus;

   connectstatus=number;
   seq[0] = STX; seq[1] = STX; seq[2] = (unsigned char)number;
   write(sockfd,seq,3);
   log(LOG_DEBUG,"status: %d",number);
}  


info(fmt,a,b,c,d,e,f)
char *fmt; int a,b,c,d,e,f;
{
   char fmtstr[256],strout[256];
   char *s, *s1, *er, *strerror();

   sprintf(fmtstr,"ceptd[%d]:",getpid());
   s = fmtstr;
   while(*fmt) {                        /* insert strerror for %m */
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
   sprintf(strout,fmtstr,a,b,c,d,e,f);
   s = fmtstr;
   s1 = strout;
   *s++ = US;   *s++ = 0x2f; *s++ = 0x42; /* reset */
   for(;;){
      if(*s1 == '\n' || !(*s1)) {
	 *s++ = APR;                      /* like cr/lf */
	 *s++ = APD;
	 if(! (*s1)) { *s =0; break; }
	 s1++;
      } else *s++ = *s1++;
   }
   write(sockfd,fmtstr,strlen(fmtstr));
   sleep(6);
   s=fmtstr;
   *s++ = US;   *s++ = 0x2f; *s++ = 0x42; /* reset */
   *s =0;
   write(sockfd,fmtstr,4);
   
   
}
   
