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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "font.h"
#include "bitmaps.h"
#include "buttons.h"
#include "buttondefs.h"
#include "scripts.h"
#include "version.h"

#define BWIDTH 5
#define CTRLTOP 100
#define CTRLBOT 10

extern int have_color, pixel_size, visible;

/* exported variables */
Display	*dpy;
Window ctrlwin, btxwin;
GC gc, cursorgc, ctrlgc;
Colormap cmap;
int scr, btxwidth, btxheight, ctrlwidth, ctrlheight;
BUTT buttons[NBUTTS];
MBUTT menus[NMENUS];

/* local variables */
static XFontStruct *xfont0, *xfont1, *xfont2, *xfont3, *xfont4;
static unsigned long fg, bg, top, bot;
static Pixmap connectpix, disconnectpix, tiapix, revealpix, chsizepix;
static char statstr[200];
static int statx, staty, statw, stath;
static Cursor btxmousecursor, ctrlmousecursor, watchcursor;

/* local functions */
static create_buttons(), xcenterstr(), xerrorhandler();


/*
 * connect to X-server, load x-fonts, create window & icon,
 * print title screen, allocate colors and load btx-fonts.
 */

open_X(display)
   char *display;
{
   extern int use_backingstore, xsync;
   XSizeHints hints;
   XSetWindowAttributes attr;
   XColor col;
   Pixmap cursorpixmap, iconpixmap, backpixmap, borderpixmap;
   char *getenv();
   int i=0;
   
   if ( dpy != NULL )	/* display already open ... forget it */
     return;

   if ( !(dpy = XOpenDisplay(display)) ) {
     fprintf(stderr, "XCEPT: can't open display '%s'!!\n", getenv("DISPLAY"));
     exit(1);
   }

   XSetErrorHandler(xerrorhandler);
   
   scr    = DefaultScreen(dpy);
   cmap   = DefaultColormap(dpy, scr);
   
   if(xsync) XSynchronize(dpy, True);

   /* set up colors */
   fg = BlackPixel(dpy, scr);
   bg = WhitePixel(dpy, scr);
   alloc_colors();
   if(have_color) {
      if( XParseColor(dpy, cmap, "#000000", &col) &&
	 XAllocColor(dpy, cmap, &col) )   fg = col.pixel;
      if( XParseColor(dpy, cmap, "#B2C0DC", &col) &&
	 XAllocColor(dpy, cmap, &col))    bg = col.pixel;
      if(XParseColor(dpy, cmap, "#C6D5E2", &col) &&
	 XAllocColor(dpy, cmap, &col))  { top = col.pixel; i|=1; }
      if(XParseColor(dpy, cmap, "#8B99B5", &col) &&
	 XAllocColor(dpy, cmap, &col))  { bot = col.pixel; i|=2; }
      if(i<3) { /* only got some of them */
	 if (i&1) XFreeColors(dpy, cmap, &top, 1, 0L);
	 if (i&2) XFreeColors(dpy, cmap, &bot, 1, 0L);
	 have_color = 0;
      }
   }
   
   init_buttons(dpy, fg, bg, have_color ? top : fg, have_color ? bot : bg);

   iconpixmap   = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
		     icon_bits, icon_width, icon_height,
                     WhitePixel(dpy, scr), BlackPixel(dpy, scr), 1);

   backpixmap   = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
		     dimple1_bits, dimple1_width, dimple1_height,
                     fg, bg, DefaultDepth(dpy, scr) );

   borderpixmap = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
		     dimple1_bits, dimple1_width, dimple1_height,
                     BlackPixel(dpy, scr), WhitePixel(dpy, scr),
		     DefaultDepth(dpy, scr) );

   set_windowsize();

   /* create control window */
   attr.background_pixmap = backpixmap;
   attr.backing_store = use_backingstore ? WhenMapped : NotUseful;
   
   ctrlwin = XCreateWindow(dpy, RootWindow(dpy, scr), 0, 0,
			   ctrlwidth, ctrlheight, 0, CopyFromParent,
			   InputOutput, CopyFromParent,
			   CWBackPixmap | CWBackingStore, &attr);

   /* create btx window */
   attr.background_pixel = WhitePixel(dpy, scr);
   attr.border_pixmap = borderpixmap;

   btxwin = XCreateWindow(dpy, ctrlwin, (ctrlwidth-btxwidth-2*BWIDTH)/2,
			  ctrlheight-btxheight-2*BWIDTH-CTRLBOT,
			  btxwidth, btxheight, BWIDTH, CopyFromParent,
			  InputOutput, CopyFromParent,
			  CWBorderPixmap|CWBackPixel|CWBackingStore, &attr);

   cursorpixmap  = XCreateBitmapFromData(dpy, btxwin, cursor_bits,
					 cursor_width, cursor_height);
   connectpix    = XCreateBitmapFromData(dpy, ctrlwin, connect_bits,
					 connect_width, connect_height);
   disconnectpix = XCreateBitmapFromData(dpy, ctrlwin, disconnect_bits,
					 disconnect_width, disconnect_height);
   tiapix        = XCreateBitmapFromData(dpy, ctrlwin, tia_bits,
					 tia_width, tia_height);
   revealpix     = XCreateBitmapFromData(dpy, ctrlwin, reveal_bits,
					 reveal_width, reveal_height);
   chsizepix     = XCreateBitmapFromData(dpy, ctrlwin, chsize_bits,
					 chsize_width, chsize_height);

   /* load X fonts */
   xfont0 = XLoadQueryFont(dpy, "5x8");
   xfont1 = XLoadQueryFont(dpy, "fixed");
   xfont2 = XLoadQueryFont(dpy, "-adobe-courier-bold-r-normal--12-*");
   xfont3 = XLoadQueryFont(dpy, "-adobe-courier-bold-r-*-*-24-*-*-*-*-*-*-*");
   xfont4 = XLoadQueryFont(dpy,
	       "-adobe-new century schoolbook-bold-r-*-*-34-*-*-*-*-*-*-*");
   
   ctrlgc = XCreateGC(dpy, ctrlwin, 0, 0);
   
   /* set up GC for btx window */
   gc = XCreateGC(dpy, btxwin, 0, 0);
   XSetForeground(dpy, gc, BlackPixel(dpy, scr) );
   XSetBackground(dpy, gc, WhitePixel(dpy, scr) );
   XSetGraphicsExposures(dpy, gc, False);
   
   /* set up GC for btx cursor manipulation */
   cursorgc = XCreateGC(dpy, btxwin, 0, 0);
   XSetForeground(dpy, cursorgc, AllPlanes);
   XSetFunction(dpy, cursorgc, GXxor);
   XSetFillStyle(dpy, cursorgc, FillStippled);
   XSetStipple(dpy, cursorgc, cursorpixmap);
   
   /* inform window manager */
   hints.width  = hints.min_width  = hints.max_width  = ctrlwidth;
   hints.height = hints.min_height = hints.max_height = ctrlheight;
   hints.flags  = PSize | PMinSize | PMaxSize;
   XSetStandardProperties(dpy, ctrlwin, "XCEPT", "XCEPT",
			  iconpixmap, NULL, 0, &hints);

   /* create mouse cursors: xterm, arrow, watch */
   btxmousecursor  = XCreateFontCursor(dpy, XC_xterm);
   ctrlmousecursor = XCreateFontCursor(dpy, XC_top_left_arrow);
   watchcursor     = XCreateFontCursor(dpy, XC_watch);

   normal_cursor();
   
   /* input from ctrl window & btx window, select before mapping ! */
   XSelectInput(dpy, btxwin, ExposureMask | KeyPressMask |
		ButtonPressMask | ButtonReleaseMask );
   XSelectInput(dpy, ctrlwin, ExposureMask | KeyPressMask |
		ButtonPressMask | ButtonReleaseMask );

   XMapRaised(dpy, ctrlwin);
   XMapSubwindows(dpy, ctrlwin);
   
   create_buttons();  /* create after mapping subwindows (menu windows !) */
   
   visible = 1;
   update_status();
}


resize_windows()
{
   XSizeHints hints;

   free_font_pixmaps();
   set_windowsize();
   XResizeWindow(dpy, ctrlwin, ctrlwidth, ctrlheight);
   XResizeWindow(dpy, btxwin,  btxwidth,  btxheight);

   /* inform window manager */
   hints.width  = hints.min_width  = hints.max_width  = ctrlwidth;
   hints.height = hints.min_height = hints.max_height = ctrlheight;
   hints.flags  = PSize | PMinSize | PMaxSize;
   XSetWMNormalHints(dpy, ctrlwin, &hints);
   
   XFlush(dpy);
}


set_windowsize()
{
   /* compute sizes */
   btxwidth   = 40*FONT_WIDTH*pixel_size;
   btxheight  = 20*FONT_HEIGHT*pixel_size;
   ctrlwidth  = btxwidth  + 2*BWIDTH + 2*CTRLBOT;
   ctrlheight = btxheight + 2*BWIDTH + CTRLTOP + CTRLBOT;
}  


static create_buttons()
{
   static char *protocolmenu[] = { "open session log",
				   "close session log",
				   "start playback",
				   "stop playback",
				   "open debug log",
				   "close debug log" };
   
   static char *servermenu[] = { "connect",
				 "disconnect",
				 "set host",
				 "set port" };

   static char *projectmenu[] = { "info",
				  "quit" };

   static char *pagemenu[] = { "save as ASCII",
			       "save as PPM" };
   
   static char *raw_scriptmenu[] = { "start execution",
				     "stop execution" };
   static char **scriptmenu;


   /* buttons */
   BTCreate(&buttons[CONNECT], ctrlwin, CTRLBOT, 5, 50, 42, NULL);
   buttons[CONNECT].pix = connectpix;
   buttons[CONNECT].pw  = connect_width;
   buttons[CONNECT].ph  = connect_height;

   BTCreate(&buttons[DISCONNECT], ctrlwin, CTRLBOT, 5+47, 50, 42, NULL);
   buttons[DISCONNECT].pix = disconnectpix;
   buttons[DISCONNECT].pw  = disconnect_width;
   buttons[DISCONNECT].ph  = disconnect_height;

   BTCreate(&buttons[TIA], ctrlwin, CTRLBOT+50+5, 5, 50, 42, NULL);
   buttons[TIA].toggle = 1;
   buttons[TIA].pix    = tiapix;
   buttons[TIA].pw     = tia_width;
   buttons[TIA].ph     = tia_height;

   BTCreate(&buttons[REVEAL], ctrlwin, CTRLBOT+50+5, 5+47, 50, 42, NULL);
   buttons[REVEAL].toggle = 1;
   buttons[REVEAL].pix    = revealpix;
   buttons[REVEAL].pw     = reveal_width;
   buttons[REVEAL].ph     = reveal_height;

   BTCreate(&buttons[CHSIZE], ctrlwin, CTRLBOT+55+55, 5, 50, 42, NULL);
   buttons[CHSIZE].pix    = chsizepix;
   buttons[CHSIZE].pw     = chsize_width;
   buttons[CHSIZE].ph     = chsize_height;

   BTCreate(&buttons[QUIT], ctrlwin, CTRLBOT+55+55, 5+47, 50, 42, "Quit");


   /* menus */
   MBCreate(&menus[PROJECT], ctrlwin, CTRLBOT+180, 10, 80, 20,
	    "Project", projectmenu, 2, 0);
   MBCreate(&menus[PAGE], ctrlwin, CTRLBOT+180, 40, 80, 20,
	    "Page", pagemenu, 2, 0);
   MBCreate(&menus[PROTOCOL], ctrlwin, CTRLBOT+180+100, 10, 80, 20,
	    "Protocol", protocolmenu, 6, 0);

   /* add more script menu lines */
   {  int i;

      if( (scriptmenu =
	   (char **) malloc( (2 + num_scripts) * sizeof(char *))) == NULL )
	perror( NULL ), exit(1);
      scriptmenu[0] = raw_scriptmenu[0];
      scriptmenu[1] = raw_scriptmenu[1];
      for( i=0; i < num_scripts; i++ )
	scriptmenu[i+2] = scripts[i].basename;
   }
   MBCreate(&menus[SCRIPT], ctrlwin, CTRLBOT+180+100, 40, 80, 20,
	    "Script", scriptmenu, 2+num_scripts, 0);

   MBCreate(&menus[SERVER], ctrlwin, CTRLBOT+180+200, 10, 80, 20,
	    "Server", servermenu, 4, 0);

   /* status line */
   statx = CTRLBOT+180;
   stath = 20;
   staty = 5+42+5+42-stath;
   statw = 100+100+80;
}


redraw_ctrlwin()
{
   int i;

   for (i=0; i<NBUTTS; i++)  BTRedraw(buttons+i);
   for (i=0; i<NMENUS; i++)  MBRedraw(menus+i);
   redraw_status();
}


titlescreen()
{
   char ver[256];
   if(have_color) {
      XSetForeground(dpy, gc, BlackPixel(dpy, scr));
      XFillRectangle(dpy, btxwin, gc, 0, 0, btxwidth, btxheight);
      XSetForeground(dpy, gc, WhitePixel(dpy, scr));
   }
   else XClearWindow(dpy, btxwin);

   strcpy(ver,"Version ");
   strcat(ver,XCEPTVERSION);
   xcenterstr("X C E P T ", 60*pixel_size, xfont4);
   xcenterstr(ver,80*pixel_size, xfont2);
   xcenterstr("\251 1992, 1993   Arno Augustin, Frank H\366ring",
	      100*pixel_size, xfont2);
   xcenterstr("University of Erlangen-Nuremberg, Germany",
	      120*pixel_size, xfont0);
}   


static xcenterstr(str, y, f)
char *str;
int y;
XFontStruct *f;
{
   int w;

   if(f) {
      XSetFont(dpy, gc, f->fid);
      w = XTextWidth(f, str, strlen(str) );
      XDrawString(dpy, btxwin, gc, (btxwidth-w)/2,  y, str, strlen(str) );
   }
}


set_status(str)
char *str;
{
   strncpy(statstr, str, 200);
   redraw_status();
}


redraw_status()
{
   XFontStruct *fs = xfont2;

   if(visible) {
      XSetForeground(dpy, ctrlgc, bg);
      XFillRectangle(dpy, ctrlwin, ctrlgc, statx+1, staty+1, statw-2, stath-2);
      XSetForeground(dpy, ctrlgc, fg);
      XDrawRectangle(dpy, ctrlwin, ctrlgc, statx, staty, statw-1, stath-1);
      XSetFont(dpy, ctrlgc, fs->fid);
      XDrawString(dpy, ctrlwin, ctrlgc,
		  (statx+statw/2)-XTextWidth(fs, statstr, strlen(statstr))/2,
		  (staty+stath/2)-((fs->ascent+fs->descent)/2)+fs->ascent,
		  statstr, strlen(statstr));
   }
}


normal_cursor()
{
   if(visible) {
      XDefineCursor(dpy, ctrlwin, ctrlmousecursor);
      XDefineCursor(dpy, btxwin, btxmousecursor);
      XFlush(dpy);
   }
}


watch_cursor()
{
   if(visible) {
      XDefineCursor(dpy, ctrlwin, watchcursor);
      XDefineCursor(dpy, btxwin, watchcursor);
      XFlush(dpy);
   }
}


xflush()
{
   if(visible)  XFlush(dpy);
}


xbell()
{
   if(visible)  XBell(dpy, -80);
}


close_X()
{
   if(dpy == NULL)	/* no display open at all */
      return;
   free_font_pixmaps();
   XCloseDisplay(dpy);  /* free all X resources (windows, fonts, ....) */
   dpy = NULL;
   visible = 0;
}


static xerrorhandler(d, err)
Display *d;
XErrorEvent *err;
{
   char str[100];

   XGetErrorText(d, err->error_code, str, 100);
   fprintf(stderr, "XCEPT: XERROR: %s\n", str);
   fprintf(stderr, "XCEPT: XERROR: ID=0x%x\n", err->resourceid);
   fprintf(stderr, "XCEPT: XERROR: ignore & continue ? (y/n) ");
   scanf("%s", str);
   if(str[0]!='y') exit(1);
}
