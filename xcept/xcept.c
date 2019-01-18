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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <sys/time.h>
#include "version.h"
#include "font.h"
#include "attrib.h"
#include "control.h"
#include "buttons.h"
#include "buttondefs.h"
#include "scripts.h"
#include "protocol.h"

#define BTXHOSTENV          "XCEPTHOST"
#define READ_BUFSZ	    (2*1024)

#define MIN(a, b)  ((a) > (b) ? (b) : (a))
#define MAX(a, b)  ((a>b) ? (a) : (b))

extern Display *dpy;
extern Window  btxwin, ctrlwin;
extern int rows, fontheight, reachedEOF;
extern BUTT buttons[];
extern MBUTT menus[];

/* exported variables */
int visible=0, pixel_size=1, have_color=1, use_backingstore=1, xsync=0;
int playback=0, connected=0, tia=0, reveal=0, server_infd, server_outfd;
FILE *textlog=NULL;
unsigned char *memimage=NULL;
char playbackfilename[100];
int connection_status=UNSET;

/* local variables */
static int port = DEFAULTSOCKETPORT;
static char btxhostname[100], tmpstr[100], textfilename[100], **script_argv;
static int script_argc, script=0, stop_page=0, jump=0, showtitle=1;
static char *more_script_pathes = NULL;
static int want_playback=0, want_connect=0, want_script=0, want_window=1;
static char *cancel_ok_str[] = { "Cancel", "Ok" };
static FILE *datalog=NULL;

/* local functions */
static process_X_events(), expose_event(), buttonpress_event();
static keypress_event(), selection(), toggle_tia(), toggle_reveal();
static parse_options(), usage(), menu_server(), menu_protocol(), menu_page();
static menu_project(), menu_script(), expand_homedir();

/* stuff to speed up read1 */
static char read1_buffer[READ_BUFSZ];
static int read1_bufend=0, read1_bufind=0;


main(argc, argv)
int argc;
char **argv;
{
   fd_set fdset;
   int maxfd, dct=1;
   char *btxhostenv;
   
   /* for remote control of xcept by scripts set stdin and stdout
      to line buffering !! (This is normally the default) */
   setvbuf(stdin, NULL, _IOLBF, 0);
   setvbuf(stdout, NULL, _IOLBF, 0);

   /* initialize default hostname for server */
   if (btxhostenv = getenv(BTXHOSTENV))
   	strncpy(btxhostname, btxhostenv, MIN(strlen(btxhostenv), 100));
   else strncpy(btxhostname, DEFAULTCEPTHOSTNAME, 100);

   textfilename[0] = 0;
   
   parse_options(argc, argv);
   collect_xbtxscripts( more_script_pathes );

   /* try to look up script via script pathes */
   if( want_script && script_argv[0] != NULL && script_argv[0][0] != '/' ) {
      char *p;

      if( (p = lookup_script(script_argv[0])) != NULL ) {
	 script_argv[0] = p;
      }
   }

   init_colormap();
   init_fonts();
   init_layer6();

   if(want_window) open_X(NULL);  /* NULL=default display (env DISPLAY) */
   
   if(want_connect) set_connect();
   else if(want_playback) {
      set_playback();
      if(playback) {
	 /* skip the first pages */
	 visible = 0;
	 while(jump--) {
	    while( !process_BTX_data() )  if(reachedEOF) break;
	    if(reachedEOF) {
	       close(server_infd);
	       playback = 0;
	       break;
	    }
	 }
	 visible = want_window;
      }
   }
   else set_disconnect();

   if(want_script) set_script();

   /* script terminated immediately */
   if(!visible && !script) exit(0);

   /* main event loop */
   while(1) {
      FD_ZERO(&fdset);
      maxfd = -1;
      
      if(visible) {
	 FD_SET(ConnectionNumber(dpy), &fdset);
	 maxfd = MAX(maxfd, ConnectionNumber(dpy));
	 
	 /* purge X event queue before selecting, so select() really
	  * will detect new incoming events from the socket. Otherwise the
	  * next event may be already queued and so missed by select().
	  */
	 if(QLength(dpy)) process_X_events();
      }
      
      if( connected || (playback && !stop_page) ) {
         FD_SET(server_infd, &fdset);
 	 maxfd = MAX(maxfd, server_infd);
      }

      if( read1_bufind<read1_bufend && !stop_page) {
	  FD_SET(server_infd, &fdset);
      }
      else {
	  if(select(maxfd+1, &fdset, NULL, NULL, NULL) == -1) {
	     perror("XCEPT: select()");
	     connected = playback = 0;
	     continue;
	  }
	  
	  if(visible && FD_ISSET(ConnectionNumber(dpy), &fdset)) {
	     process_X_events();
	  }
      }

      if((connected || playback) && FD_ISSET(server_infd, &fdset)) {
         stop_page = dct = process_BTX_data();
         if(visible) XFlush(dpy);

         if(reachedEOF)
	    if(visible) set_disconnect();
	    else        quit(0);

	 if(script && dct && !script_next_dct()) {
	    script = 0;
	    if(visible) update_status();
	    else        quit(0);
	 }
      }

   } /* while 1 */

}


/*
 * blocks until 1 byte read from BTX server, X events are porcessed
 */
int read1(fd, buf, n)
int fd, n;
char *buf;
{
   fd_set fdset;
   int maxfd;

   if(n== -1) {
      if(read1_bufind)  read1_bufind--;
      else if(!reachedEOF)
              fprintf(stderr, "internal error: ungetc buffer overflow\n");
      return;
   }
      
   if(n!=1) {
      fprintf(stderr, "read1() can only read 1 byte !!");
      return -1;
   }

   if(read1_bufind<read1_bufend) {
      *buf = read1_buffer[read1_bufind++];
      return 1;
   }
   
   if(reachedEOF) return 0;

   /* loop until we read at least 1 byte */
   while(1) {
      FD_ZERO(&fdset);
      FD_SET(fd, &fdset);
      maxfd = fd;
      
      if(visible) {
	 FD_SET(ConnectionNumber(dpy), &fdset);
	 maxfd = MAX(fd, ConnectionNumber(dpy));
	 if(QLength(dpy)) process_X_events();
      }

      if(select(maxfd+1, &fdset, NULL, NULL, NULL) == -1) {
	 perror("XCEPT: read1(): select()");
	 return -1;
      }
   
      if(visible && FD_ISSET(ConnectionNumber(dpy), &fdset)) {
	 process_X_events();
	 if(reachedEOF) return 0;
      }

      if(FD_ISSET(fd, &fdset)) {
	 read1_bufend = read(fd, read1_buffer, sizeof(read1_buffer));
	 if(read1_bufend <= 0)  return read1_bufend;
	 else {
	    if(datalog && visible) {
	       fwrite(read1_buffer, 1, read1_bufend, datalog);
	       fflush(datalog);
	    }
	    *buf = read1_buffer[0];
	    read1_bufind = 1;
	    return 1;
	 }
      }
   }
}

   
/* predicate procedure used by XCheckIfEvent(), match ALL events !! */
static int all_event_proc()
{
   return 1;
}


static process_X_events()
{
   XEvent event;

   while(XCheckIfEvent(dpy, &event, all_event_proc, NULL))  {

      switch(event.type) {
         case Expose:
     	    expose_event(&event);
	    break;

	 case KeyPress:
	    keypress_event(&event);
	    break;
	 
	 case ButtonPress:
	    buttonpress_event(&event);
	    break;
	 
	 case ButtonRelease:
	    if(event.xbutton.window==btxwin) {
	       if(event.xbutton.button==Button1)
	          selection(1, event.xbutton.x, event.xbutton.y);
	       if(event.xbutton.button==Button3)  update_status();
	    }
	    break;
	    
	 case GraphicsExpose:
	    fprintf(stderr, "XCEPT: EVENT: GraphicsExpose: %d %d %d %d\n",
		    event.xgraphicsexpose.x, event.xgraphicsexpose.y,
		    event.xgraphicsexpose.width, event.xgraphicsexpose.height);
	    break;
	 
	 case NoExpose:
	    fprintf(stderr, "XCEPT: EVENT: NoExpose\n");
	    break;

         case SelectionClear:
	    fprintf(stderr, "XCEPT: EVENT: SelectionClear\n");
	    break;

         case SelectionRequest:
	    fprintf(stderr, "XCEPT: EVENT: SelectionRequest\n");
	    break;

         case SelectionNotify:
	    fprintf(stderr, "XCEPT: EVENT: SelectionNotify\n");
	    break;
      }
   }
}


static expose_event(event)
XEvent *event;
{
   int x1, y1, x2, y2;
   
   if(event->xexpose.window==btxwin) {
      if(showtitle)  titlescreen();
      else {
	 x1 = event->xexpose.x/FONT_WIDTH/pixel_size;
	 y1 = event->xexpose.y/fontheight/pixel_size;
	 x2 = (event->xexpose.x+event->xexpose.width-1)/FONT_WIDTH/pixel_size;
	 y2 = (event->xexpose.y+event->xexpose.height-1)/fontheight/pixel_size;
         redraw_screen_rect(x1, y1, x2, y2);
      }
   }

   if(event->xexpose.window==ctrlwin && event->xexpose.count==0) {
      redraw_ctrlwin();
   }
   
   XFlush(dpy);
}


static buttonpress_event(event)
XEvent *event;
{
   /*  B T X  window  */
   if(event->xbutton.window==btxwin) {
      if(event->xbutton.button==Button1)
         selection(0, event->xbutton.x, event->xbutton.y);
      if(event->xbutton.button==Button2)  selection(2);
      if(event->xbutton.button==Button3) {
	 sprintf(tmpstr, "mouse position: x=%d,  y=%d",
		 event->xbutton.x/FONT_WIDTH/pixel_size+1,
		 event->xbutton.y/fontheight/pixel_size+1);
	 set_status(tmpstr);
      }
   }

   /*  C O N T R O L  window  */
   if(event->xbutton.window==ctrlwin) {
      if(event->xbutton.button==Button1) {
	 /* CONNECT button */
	 if(BTClick(&buttons[CONNECT], event->xbutton.x, event->xbutton.y)) {
	    if(BTTrack(&buttons[CONNECT]))  set_connect();
	 }
	 /* DISCONNECT button */
	 if(BTClick(&buttons[DISCONNECT],event->xbutton.x, event->xbutton.y)) {
	    if(BTTrack(&buttons[DISCONNECT])) {
	       connection_status = ABORT;
	       set_disconnect();
	    }
	 }
	 /* TIA button */
	 if(BTClick(&buttons[TIA], event->xbutton.x, event->xbutton.y)) {
	    if(BTTrack(&buttons[TIA]))  toggle_tia();
	 }
	 /* REVEAL button */
	 if(BTClick(&buttons[REVEAL], event->xbutton.x, event->xbutton.y)) {
	    if(BTTrack(&buttons[REVEAL]))  toggle_reveal();
	 }
	 /* CHSIZE button */
	 if(BTClick(&buttons[CHSIZE], event->xbutton.x, event->xbutton.y)) {
	    if(BTTrack(&buttons[CHSIZE])) {
	       pixel_size = 3-pixel_size;
	       resize_windows();
	    }
	 }
	 /* QUIT button */
	 if(BTClick(&buttons[QUIT], event->xbutton.x, event->xbutton.y)) {
	    if(BTTrack(&buttons[QUIT]))  quit(1);
	 }
	 /* PROTOCOL menu */
	 if(MBClick(&menus[PROTOCOL], event->xbutton.x, event->xbutton.y)) {
	    if(MBTrack(&menus[PROTOCOL]))  menu_protocol();
	 }
	 /* SERVER menu */
	 if(MBClick(&menus[SERVER], event->xbutton.x, event->xbutton.y)) {
	    if(MBTrack(&menus[SERVER]))  menu_server();
	 }
	 /* PROJECT menu */
	 if(MBClick(&menus[PROJECT], event->xbutton.x, event->xbutton.y)) {
	    if(MBTrack(&menus[PROJECT]))  menu_project();
	 }
	 /* PAGE menu */
	 if(MBClick(&menus[PAGE], event->xbutton.x, event->xbutton.y)) {
	    if(MBTrack(&menus[PAGE]))  menu_page();
	 }
	 /* SCRIPT menu */
	 if(MBClick(&menus[SCRIPT], event->xbutton.x, event->xbutton.y)) {
	    if(MBTrack(&menus[SCRIPT]))  menu_script();
	 }
      } /* Button 1 */

   } /* ctrlwin */

}


static keypress_event(event)
XEvent *event;
{
   static char quote_chars[3];
   static int quote=0;
   KeySym keysym;
   char string[10];
   int n;

   n = XLookupString((XKeyEvent *) event, string, 10, &keysym, NULL);
	 
   switch(keysym) {
      case XK_Escape:
         exit(0);
	 break;
      case XK_l:
	 if(event->xkey.state & ControlMask) {
	    redraw_screen_rect(0, 0, 39, rows-1);
	    XFlush(dpy);
	    n = 0;
	 }
	 break;
      case XK_q:
	 if(event->xkey.state & ControlMask) {
	    quote = 1;
	 }
	 break;
      case XK_w:
	 if(event->xkey.state & ControlMask) {
	    if(!textfilename[0])
	       popup("XCEPT: save page as ASCII", 0,
			"Please enter filename:", cancel_ok_str, 2,
			textfilename, 100, 1);
	    if(textfilename[0]) {
	       expand_homedir(textfilename);
	       watch_cursor();
	       save_ASCII(textfilename);
	       normal_cursor();
	    }
	    n = 0;
	 }
	 break;
      case XK_Home:
	 string[0] = APH;
	 n = 1;
	 break;
      case XK_BackSpace:
      case XK_Delete:
      case XK_Left:
	 string[0] = APB;
	 n = 1;
	 break;
      case XK_Up:
	 string[0] = APU;
	 n = 1;
	 break;
      case XK_Right:
	 string[0] = APF;
	 n = 1;
	 break;
      case XK_Down:
	 string[0] = APD;
	 n = 1;
	 break;
      case XK_asterisk:
      case XK_KP_Multiply:
	 string[0] = INI;
	 n = 1;
	 break;
      case XK_numbersign:
      case XK_Return:
      case XK_KP_Enter:
	 string[0] = TER;
	 n = 1;
	 break;
      case XK_KP_0:
	 string[0] = '0';
	 n = 1;
	 break;
      case XK_KP_1:
	 string[0] = '1';
	 n = 1;
	 break;
      case XK_KP_2:
	 string[0] = '2';
	 n = 1;
	 break;
      case XK_KP_3:
	 string[0] = '3';
	 n = 1;
	 break;
      case XK_KP_4:
	 string[0] = '4';
	 n = 1;
	 break;
      case XK_KP_5:
	 string[0] = '5';
	 n = 1;
	 break;
      case XK_KP_6:
	 string[0] = '6';
	 n = 1;
	 break;
      case XK_KP_7:
	 string[0] = '7';
	 n = 1;
	 break;
      case XK_KP_8:
	 string[0] = '8';
	 n = 1;
	 break;
      case XK_KP_9:
	 string[0] = '9';
	 n = 1;
         break;
      case XK_Adiaeresis:
         n = 2;
         string[1] = 'A';
         string[0] = 0xc8;
         break;
      case XK_adiaeresis:
         n = 2;
         string[1] = 'a';
         string[0] = 0xc8;
         break;
      case XK_Odiaeresis:
         n = 2;
         string[1] = 'O';
         string[0] = 0xc8;
         break;
      case XK_odiaeresis:
         n = 2;
         string[1] = 'o';
         string[0] = 0xc8;
         break;
      case XK_Udiaeresis:
         n = 2;
         string[1] = 'U';
         string[0] = 0xc8;
         break;
      case XK_udiaeresis:
         n = 2;
         string[1] = 'u';
         string[0] = 0xc8;
         break;
      case XK_ssharp:
         n = 1;
         string[0] = 0xfb;
	 break;
      case XK_a:
      case XK_A:
      case XK_o:
      case XK_O:
      case XK_u:
      case XK_U:
	 if(event->xkey.state & Mod1Mask) {
	    string[1] = string[0];
	    string[0] = 0xc8;
	    n = 2;
	 }
	 break;
      case XK_s:
	 if(event->xkey.state & Mod1Mask) {
	    string[0] = 0xfb;
	    n = 1;
	 }
	 break;
      case XK_KP_Decimal:
	 string[0] = '1';
	 string[1] = '9';
	 n = 2;
	 break;
   } /* switch */
   
   if(n && quote) {     /* ^Q: 3 digit octal quote */
      if(++quote==2)  n = 0;
      else {
	 if(string[0]>='0' && string[0]<='9') {
	    quote_chars[quote-3] = string[0];
	    if(quote==5) {
	       string[0] = (quote_chars[0]-'0')*8*8 +
	       (quote_chars[1]-'0')*8 + quote_chars[2]-'0';
	       quote = 0;
	       n = 1;
	    }
	    else n = 0;
	 }
	 else quote = 0;
      }
   }

   if(n) {
      if(connected) write(server_outfd, string, n);
      if(playback)  stop_page = 0;
   }
}


static selection(op, x, y)
int op, x, y;
{
   static char cut_buffer[24*40];
   static int x1, y1, n;
   int x2, y2, i, xs, xe;
   char *data;
   
   switch(op) {

      case 0:  /* START */
	 x1 = x/FONT_WIDTH/pixel_size;
	 y1 = y/fontheight/pixel_size;
	 n = 0;
	 xbell();
	 XFlush(dpy);
	 break;

      case 1:  /* STOP */
	 x2 = x/FONT_WIDTH/pixel_size;
	 y2 = y/fontheight/pixel_size;

	 if( y2<y1 || (y1==y2 && x2<x1) ) {
	    i=x2; x2=x1; x1=i;
	    i=y2; y2=y1; y1=i;
	 }

	 for(y=y1; y<=y2; y++) {
	    xs = (y==y1) ? x1 : 0;
	    xe = (y==y2) ? x2 : 40;
	    for(x=xs; x<xe; x++) {
	       cut_buffer[n++] = map_iso_char(x, y);
	       if(x==39)  cut_buffer[n++] = '\n';
	    }
	 }
	 XStoreBytes(dpy, cut_buffer, n);
	 /* Clear the selection owner, so that other applications
	    will use the cut buffer rather than a selection.  */
	 XSetSelectionOwner(dpy, XA_PRIMARY, None, CurrentTime);
	 xbell();
	 XFlush(dpy);
	 break;

      case 2:  /* PASTE */
	 data = XFetchBytes(dpy, &n);
	 if(n)  write(server_outfd, data, n);
	 XFree(data);
	 break;
	 
   }
}
	    
	 
static toggle_reveal()
{
   reveal ^= 1;
   redraw_screen_rect(0, 0, 39, rows-1);
   XFlush(dpy);
}


static toggle_tia()
{
   tia ^= 1;
   if(tia) {
      reveal = 0;
      buttons[REVEAL].lit = 0;
   }
   update_status();
   redraw_screen_rect(0, 0, 39, rows-1);
   XFlush(dpy);
}


set_connect()
{
   if(connected || playback)  return;

   connection_status = UNSET;
   
   if( (server_infd = server_outfd = callbtx(btxhostname, port)) == -1 ) {
      xbtxerror(1, "can't connect to server '%s' (%d)", btxhostname, port);
      set_disconnect();
      return;
   }
   read1_bufind = read1_bufend = 0;
   connected = 1;
   reachedEOF = 0;
   showtitle = 0;
   update_status();
}


set_disconnect()
{
   if(connected || playback) close(server_infd);
   connected = playback = script = 0;
   reachedEOF = 1;
   read1_bufind = read1_bufend = 0;
   update_status();
}


set_playback()
{
   if(connected || playback)  return;
   
   connection_status = UNSET;

   if(strcmp(playbackfilename, "-") == 0)  server_infd = 0;
   else
      if( (server_infd = open(playbackfilename, O_RDONLY)) == -1) {
	 xbtxerror(1, "can't open '%s'", playbackfilename);
	 set_disconnect();
	 return;
      }
   read1_bufind = read1_bufend = 0;
   server_outfd = 1;
   playback = 1;
   reachedEOF = stop_page = 0;
   showtitle = 0;
   update_status();
}   


set_script()
{
   if(initscript(script_argc, script_argv) < 0) {
      script = 0;
      return;
   }
   /* start script, even if not yet connected */
   script = script_next_dct();
   update_status();
}


update_status()
{
   if(connected) {
      if(connection_status==ISDNCONNECT)  strcpy(tmpstr, "ONLINE (ISDN)");
      else if(connection_status==CONNECT) strcpy(tmpstr, "ONLINE (Modem)");
      else                                strcpy(tmpstr, "CONNECTED");
   }
   else if(playback)  strcpy(tmpstr, "PLAYBACK");
   else {
      strcpy(tmpstr, "DISCONNECTED");
      switch(connection_status) {
         case UNSET:                                                  break;
         case NOCARRIER:      strcat(tmpstr, " (carrier lost)");      break;
         case NODIALTONE:     strcat(tmpstr, " (no dialtone)");       break;
         case NOPERMISSION:   strcat(tmpstr, " (no permission)");     break;
         case TIMEOUT:        strcat(tmpstr, " (dial timed out)");    break;
         case HANGUP:         strcat(tmpstr, " (modem hangup)");      break;
         case ABORT:          strcat(tmpstr, " (aborted)");           break;
         case BUSY:           strcat(tmpstr, " (modem line busy)");   break;
         case OPENFAIL:       strcat(tmpstr, " (modem device busy)"); break;
	 case ISDNBUSY:       strcat(tmpstr, " (ISDN line busy)");    break;
	 case ISDNOPENFAIL:   strcat(tmpstr, " (ISDN device busy)");  break;
	 case ISDNDISCONNECT: strcat(tmpstr, " (ISDN line dropped)"); break;
         default:             strcat(tmpstr, " (???)");               break;
      }
   }
      
   if(script)  strcat(tmpstr, " / SCRIPT");
   if(datalog) strcat(tmpstr, " / LOG");
   set_status(tmpstr);

   if(visible) {
      BTSetActive(&buttons[CONNECT], !connected && !playback);
      BTSetActive(&buttons[DISCONNECT], connected);
      BTSetActive(&buttons[REVEAL], !tia);
      XFlush(dpy);
   }
}


serverstatus(code)
int code;
{
   if(code==US) return;
   
   connection_status = code;
   if(code==UNSET || code==CONNECT || code==ISDNCONNECT)  update_status();
   else  set_disconnect();  /* autom. update */
}

   
static parse_options(argc, argv)
int argc;
char **argv;
{
   int i;
   
   for(i=1; i<argc; i++) {
      if(argv[i][0]=='-' && argv[i][2]==0) {
	 switch(argv[i][1]) {
	    case 'h':
	       if(i >= argc-1)  usage(argv[0]);
	       strncpy(btxhostname, argv[++i], 100);
	       break;
	    case 'p':
	       if(i >= argc-1)  usage(argv[0]);
	       port = atoi(argv[++i]);
	       break;
	    case 'r':
	       if(i >= argc-1)  usage(argv[0]);
	       strncpy(playbackfilename, argv[++i], 100);
	       want_playback = 1;
	       break;
	    case 'S':
	       if(i >= argc-1)  usage(argv[0]);
	       more_script_pathes = argv[++i];
	       break;
	    case 's':
	       script_argc = argc - (i+1);
	       script_argv = &argv[i+1];
	       want_script = 1;
	       i = argc;  /* break for() */
	       break;
	    case 'j':
	       if(i >= argc-1)  usage(argv[0]);
	       jump = atoi(argv[++i]);
	       break;
	    case 't':
	       if(i >= argc-1)  usage(argv[0]);
	       if( !(textlog = fopen(argv[++i], "w")) ) {
		  perror("XCEPT: can't open text log file\n");
	       }
	       break;
	    case 'l':
	       if(i >= argc-1)  usage(argv[0]);
	       if( !(datalog = fopen(argv[++i], "w")) ) {
		  perror("XCEPT: can't open data log file\n");
	       }
	       break;
	    case 'c':
	       want_connect = 1;
	       break;
	    case 'x':
	       want_window = 0;
	       break;
	    case 'X':
	       xsync = 1;
	       break;
	    case 'b':
	       use_backingstore = 0;
	       break;
	    case '2':
	       pixel_size = 2;
	       break;
	    case 'm':
	       have_color = 0;
	       break;
	    default:
	       usage(argv[0]);
	 }
      }
      else usage(argv[0]);
   }

   if(!want_window && !want_script) {
      fprintf(stderr, "\nXCEPT: you can't use xcept without the windows and");
      fprintf(stderr, " no script running !!\n\n");
      exit(1);
   }
}


static usage(name)
char *name;
{  fprintf(stderr, "xcept Version %s", XCEPTVERSION);
   fprintf(stderr, "\nusage: %s [options]\n\n", name);
   fprintf(stderr, "valid options are:\n");
   fprintf(stderr, "\t-h host   connect to ceptd@'host' (%s)\n",
	   btxhostname);
   fprintf(stderr, "\t-p port   use socket port 'port' (%d)\n", port);
   fprintf(stderr, "\t-l file   log session data to 'file'\n");
   fprintf(stderr, "\t-t file   log debug text to 'file'\n");
   fprintf(stderr, "\t-r file   replay session from 'file' on startup\n");
   fprintf(stderr, "\t-j count  jump over 'count' pages (at playback)\n");
   fprintf(stderr, "\t-c        connect to server on startup\n");
   fprintf(stderr, "\t-x        don't use window system on startup\n");
   fprintf(stderr, "\t-X        set synchronous mode for the X protocol\n");
   fprintf(stderr, "\t-b        don't use the X server's backing store\n");
   fprintf(stderr, "\t-2        display each pixel as 2x2 pixels\n");
   fprintf(stderr, "\t-m        force monochrome display\n");
   fprintf(stderr, "\t-S pathes colon seperated pathes where to look for script files\n");
   fprintf(stderr, "\t-s file   execute script 'file' on startup\n");
   fprintf(stderr, "\t          (last option followed by parameters)\n");
   fprintf(stderr, "\n");
   exit(1);
}


quit(confirm)
int confirm;
{
   if(!confirm || popup("XCEPT: Quit", 1, "Do you really want to quit ??",
		    cancel_ok_str, 2, NULL, 0, 1) >= 1) {
      exit(0);
   }
}


xbtxerror(perr, str, p1, p2, p3, p4, p5, p6, p7, p8)
char *str;
int perr, p1, p2, p3, p4, p5, p6, p7, p8;
{
   extern int errno;
   extern char *sys_errlist[];
   static char errstr[200];

   if(!visible) {
      fprintf(stderr, "\nXCEPT: ");
      fprintf(stderr, str, p1, p2, p3, p4, p5, p6, p7, p8);
      if(perr) fprintf(stderr, ": %s", sys_errlist[errno]);
      fprintf(stderr, "\n\n");
      exit(1);
   }
   else {
      xbell();
      sprintf(errstr, str, p1, p2, p3, p4, p5, p6, p7, p8);
      if(perr) {
	 strcat(errstr, ": ");
	 strcat(errstr, sys_errlist[errno]);
      }
      popup("XCEPT: Error", 1, errstr, cancel_ok_str+1, 1, NULL, 0, 1);
   }
}


static menu_server()
{
   switch(menus[SERVER].selected) {
      case 0:
         set_connect();
	 break;
      case 1:
	 if(connected) {
	    connection_status = ABORT;
	    set_disconnect();
	 }
	 break;
      case 2:
         strcpy(tmpstr, btxhostname);
	 if(popup("XCEPT Server", 0, "Please enter hostname:",
		  cancel_ok_str, 2, tmpstr, 100, 1) >= 1) {
	    strcpy(btxhostname, tmpstr);
	 }
	 break;
      case 3:
	 sprintf(tmpstr, "%d", port);
	 if(popup("XCEPT: Server", 0,
		  "Please enter socket port number:",
		  cancel_ok_str, 2, tmpstr, 100, 1) >= 1) {
	    port = atoi(tmpstr);
	 }
	 break;
      }
   menus[SERVER].selected = -1;
}


static menu_protocol()
{
   switch(menus[PROTOCOL].selected) {
      case 0:
         if(!datalog) {
	    tmpstr[0] = 0;
	    if(popup("XCEPT: Protocol", 0, "Enter session log filename:",
	       cancel_ok_str, 2, tmpstr, 100, 1) >= 1 && strlen(tmpstr)) {
	       expand_homedir(tmpstr);
	       if( !(datalog = fopen(tmpstr, "w")) ) {
		  xbtxerror(1, "can't open session log file '%s'", tmpstr);
	       }
	       update_status();
	    }
	 }
	 break;
      case 1:
	 if(datalog) {
	    fclose(datalog);
	    datalog = NULL;
	    update_status();
	 }
	 break;
      case 2:
	 if(!playback && !connected) {
	    tmpstr[0] = 0;
	    if(popup("XCEPT: Protocol", 0,
		     "Please enter session playback filename:", cancel_ok_str,
		     2, tmpstr, 100, 1) >= 1 && strlen(tmpstr)) {
	       expand_homedir(tmpstr);
	       strcpy(playbackfilename, tmpstr);
	       set_playback();
	    }
	 }
	 break;
      case 3:
	 if(playback)  set_disconnect();
	 break;
      case 4:
         if(!textlog) {
	    tmpstr[0] = 0;
	    if(popup("XCEPT: Protocol", 0, "Please enter debug log filename:",
	       cancel_ok_str, 2, tmpstr, 100, 1) >= 1 && strlen(tmpstr)) {
	       expand_homedir(tmpstr);
	       if( !(textlog = fopen(tmpstr, "w")) ) {
		  xbtxerror(1, "can't open debug log file '%s'", tmpstr);
	       }
	    }
	 }
	 break;
      case 5:
	 if(textlog) {
	    fclose(textlog);
	    textlog = NULL;
	 }
	 break;
   }
   menus[PROTOCOL].selected = -1;
}
   

static menu_project()
{
   switch(menus[PROJECT].selected) {
      case 0:
	 titlescreen();
         break;
      case 1:
	 quit(1);
	 break;
   }
   menus[PROJECT].selected = -1;
}


static menu_page()
{
   switch(menus[PAGE].selected) {
      case 0:
         if(popup("XCEPT: save page as ASCII", 0, "Please enter filename:",
		  cancel_ok_str, 2, textfilename, 100, 1) >= 1 &&
	    strlen(textfilename))
         {
	    expand_homedir(textfilename);
	    watch_cursor();
	    save_ASCII(textfilename);
	    normal_cursor();
	 }
         break;
      case 1:
         tmpstr[0] = 0;
         if(popup("XCEPT: save page as PPM", 0, "Please enter filename:",
		  cancel_ok_str, 2, tmpstr, 100, 1) >= 1 && strlen(tmpstr)) {
	    expand_homedir(tmpstr);
	    watch_cursor();
	    save_PPM(tmpstr);
	    normal_cursor();
	 }
	 break;
   }
   menus[PAGE].selected = -1;
}


static menu_script()
{
   static char *args[50];
   char *p;
   int i;
   
   switch(menus[SCRIPT].selected) {
      case 0:
         if(script) break;
         tmpstr[0] = 0;
         if(popup("XCEPT: Script", 0, "Please enter filename (+ parameter):",
		  cancel_ok_str, 2, tmpstr, 100, 1) >= 1 && strlen(tmpstr)) {
	    expand_homedir(tmpstr);

	    /* XXX one arg cannot contain a SPACE -> quote !?! */
	    for(i=0, p=tmpstr; *p; i++) {
	       while(!isgraph(*p)) p++;
	       args[i] = p;
	       while(isgraph(*p))  p++;
	       if(*p) *p++ = 0;
	    }
	    
	    script_argc = i;
	    script_argv = args;
	    set_script();
	 }
         break;
      default:
	 args[0] = scripts[ menus[SCRIPT].selected - 2 ].path;
	 script_argc = 1;
	 script_argv = args;
	 set_script();
	 break;
      case 1:
	 script = 0;
	 update_status();
	 break;
   }
   menus[SCRIPT].selected = -1;
}


save_ASCII(name)
char *name;
{
   FILE *fp;
   int x, y;

   if( !(fp = fopen(name, "a")) ) {
      xbtxerror(1, "can't open page save file '%s'", name);
      return;
   }

   for(y=0; y<rows; y++) {
      for(x=0; x<40; x++)  fputc(map_iso_char(x, y), fp);
      fputc('\n', fp);
   }
   fclose(fp);
}


save_PPM(name)
char *name;
{
   FILE *fp;
   
   if( !(fp = fopen(name, "w")) ) {
      xbtxerror(1, "can't open page save file '%s'", name);
      return;
   }
   
   if(!(memimage = (unsigned char *)malloc(480*240*3)) ) {
      xbtxerror(1, "can't malloc() image data");
      return;
   }

   redraw_screen_rect(0, 0, 39, rows-1);

   fprintf(fp, "P6 480 240 255 ");

   fwrite(memimage, 480*3*240, 1, fp);
   
   fclose(fp);
   free(memimage);
   memimage = NULL;
}


static int expand_homedir(path)
char *path;
{
   static char tmppath[500];
   struct passwd *pwd;
   int i;

   if(path[0] != '~')  return 1;
   strncpy(tmppath, path, 500);
   for(i=1; isalpha(path[i]) && i<498; i++);

   if(i==1) {  /* MY home dir */
      if( !(pwd = getpwuid(getuid())) )  return 0;
   }
   else {
      path[i] = 0;
      if( !(pwd = getpwnam(path+1)) )  return 0;
   }

   strcpy(path, pwd->pw_dir);
   strcat(path, tmppath+i);
   return 1;
}


int map_iso_char(x, y)
int x, y;
{
   static unsigned char supp_map[96] =
      { ' ', 0xa1, 0xa2, 0xa3, '$', 0xa5, '#', 0xa7, 0xa4, '`', '\"', 0xab,
	0, 0, 0, 0, 0xb0, 0xb1, 0xb2, 0xb3, 0xd7, 0xb5, 0xb6, 0xb7, 0xf7,
	'\'', '\"', 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0, 0x60, 0x27, 0, '~',
	0xaf, 0, 0, 0x22, 0x22, 0xb0, 0, 0, 0x22, 0xb8, 0, 0xad, 0xb9, 0xae,
	0xa9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xc6, 0, 0, 0, 0, 0,
	0, 0, 0xd8, 0, 0, 0xfe, 0, 0, 0, 0, 0xe6, 0, 0, 0, 0, 0, 0, 0, 0xf8,
	0, 0xdf, 0xde, 0, 0, 0 };
   
   static unsigned char diacritical_map[26*2][16] = {
    { 0, 0xc0, 0xc1, 0xc2, 0xc3, 0, 0, 0, 0xc4, 0xc4, 0xc5, 0, 0, 0xc4, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     /* B */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xc7, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  /* D */
    { 0, 0xc8, 0xc9, 0xca, 0, 0, 0, 0, 0xcb, 0xcb, 0, 0, 0, 0xcb, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  /* H */
    { 0, 0xcc, 0xcd, 0xce, 0, 0, 0, 0, 0xcf, 0xcf, 0, 0, 0, 0xcf, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0xd1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  /* N */
    { 0, 0xd2, 0xd3, 0xd4, 0xd5, 0, 0, 0, 0xd6, 0xd6, 0, 0, 0, 0xd6, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* T */
    { 0, 0xd9, 0xda, 0xdb, 0, 0, 0, 0, 0xdc, 0xdc, 0, 0, 0, 0xdc, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* X */
    { 0, 0, 0xdd, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0xe0, 0xe1, 0xe2, 0xe3, 0, 0, 0, 0xe4, 0xe4, 0xe5, 0, 0, 0xe4, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xe7, 0, 0, 0, 0 },  /* c */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0xe8, 0xe9, 0xea, 0, 0, 0, 0, 0xeb, 0xeb, 0, 0, 0, 0xeb, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* h */
    { 0, 0xec, 0xed, 0xee, 0, 0, 0, 0, 0xef, 0xef, 0, 0, 0, 0xef, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0xf1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* n */
    { 0, 0xf2, 0xf3, 0xf4, 0xf5, 0, 0, 0, 0xf6, 0xf6, 0, 0, 0, 0xf6, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* t */
    { 0, 0xf9, 0xfa, 0xfb, 0, 0, 0, 0, 0xfc, 0xfc, 0, 0, 0, 0xfc, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0xfd, 0, 0, 0, 0, 0, 0xff, 0xff, 0, 0, 0, 0xff, 0, 0 },  /* y */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };    
    
   extern struct screen_cell screen[24][40];
   int c, d, s;
   
   c = screen[y][x].chr & 0x7f;
   d = (screen[y][x].chr>>8) & 0x7f;
   s = screen[y][x].set;

   /* supplementary set of graphic characters */
   if(s==SUPP && supp_map[c-0x20])  return supp_map[c-0x20];
   
   /* composed characters, diacritical marks (page 123) */
   if(s==PRIM) {
      if(!d)  return c;  /* ASCII character */
      if(d>=0x40 && d<=0x4f) {
	 if(c>=0x41 && c<=0x5a && diacritical_map[c-0x41][d&0xf])
            return diacritical_map[c-0x41][d&0xf];
	 if(c>=0x61 && c<=0x7a && diacritical_map[c-0x61+26][d&0xf])
            return diacritical_map[c-0x61+26][d&0xf];
      }
   }

   /* ASCII out of  1st supplementary set of mosaic characters */
   if(s==SUP1 && c>=0x40 && c<=0x5f)  return c;
   
   return ' ';
}
