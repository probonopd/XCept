/* 
 * modified  'xvbutt.c'  from John Bradley
 *
 * callable functions:
 *
 *   BTCreate()             -  create a button
 *   BTSetActive()          -  change 'active' status of button
 *   BTRedraw()             -  redraw button
 *   BTClick()              -  returns true if given BT was clicked on
 *   BTTrack()              -  clicked in button.  track until mouse up
 *
 *   RBCreate()             -  create an RBUTT and append to supplied list
 *   RBRedraw()             -  redraw one or all RBUTTs in a list
 *   RBSelect()             -  change selected item in list of RBUTTs
 *   RBWhich()              -  returns index of selected RBUTT in list
 *   RBCount()              -  returns # of RBUTTs in list
 *   RBSetActive()          -  sets active status of an RBUTT
 *   RBClick()              -  finds clicked-on rb in a list
 *   RBTrack()              -  tracks rb after click, until release
 * 
 *   CBCreate()             -  create a CBUTT (checkbox button)
 *   CBRedraw()             -  redraw a CBUTT
 *   CBSetActive()          -  change active status of a CBUTT
 *   CBClick()              -  returns true if given CB was clicked on
 *   CBTrack()              -  tracks CBUTT after click, until release
 *
 *   MBCreate()             -  create a MBUTT (menu button)
 *   MBRedraw()             -  redraw a MBUTT
 *   MBSetActive()          -  change active status of a MBUTT
 *   MBClick()              -  returns true if given MB was clicked on
 *   MBTrack()              -  tracks MBUTT after click, until release
 */


/*
 * Copyright 1989, 1990, 1991, 1992 by John Bradley and
 *                       The University of Pennsylvania
 *
 * Permission to use, copy, and distribute for non-commercial purposes,
 * is hereby granted without fee, providing that the above copyright
 * notice appear in all copies and that both the copyright notice and this
 * permission notice appear in supporting documentation. 
 *
 * The software may be modified for your own purposes, but modified versions
 * may not be distributed.
 *
 * This software is provided "as is" without any expressed or implied warranty.
 *
 * The author may be contacted via:
 *    US Mail:   John Bradley
 *               GRASP Lab, Room 301C
 *               3401 Walnut St.  
 *               Philadelphia, PA  19104
 *
 *    Phone:     (215) 898-8813
 *    EMail:     bradley@cis.upenn.edu       
 */

/**************************************************************************/

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "buttons.h"

#define FONT1 "-*-lucida-medium-r-*-*-12-*"
#define FONT2 "-*-helvetica-medium-r-*-*-12-*"
#define FONT3 "-*-helvetica-medium-r-*-*-11-*"
#define FONT4 "6x13"
#define FONT5 "fixed"

/* random string-placing definitions */
#define SPACING 3      /* vertical space between strings */
#define ASCENT   (mfinfo->ascent)
#define DESCENT  (mfinfo->descent)
#define CHIGH    (ASCENT + DESCENT)
#define LINEHIGH (CHIGH + SPACING)

/* MACROS */
#define CENTERX(f,x,str) ((x)-XTextWidth(f,str,strlen(str))/2)
#define CENTERY(f,y) ((y)-((f->ascent+f->descent)/2)+f->ascent)

/* RANGE forces a to be in the range b..c (inclusive) */
#define RANGE(a,b,c) { if (a < b) a = b;  if (a > c) a = c; }

/* PTINRECT returns '1' if x,y is in rect (inclusive) */
#define PTINRECT(x,y,rx,ry,rw,rh) \
           ((x)>=(rx) && (y)>=(ry) && (x)<=(rx)+(rw) && (y)<=(ry)+(rh))

/* MONO returns total intensity of r,g,b components */
#define MONO(rd,gn,bl) (((rd)*11 + (gn)*16 + (bl)*5) >> 5)  /*.33R+ .5G+ .17B*/


/***************************  B I T M A P S ******************************/
#define cboard50_width 8
#define cboard50_height 8
static char cboard50_bits[] = {
   0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};

#define mb_chk_width 8
#define mb_chk_height 8
static char mb_chk_bits[] = {
   0x00, 0x80, 0xc0, 0x60, 0x33, 0x1f, 0x0e, 0x04};

#define gray50_width 8
#define gray50_height 8
static char gray50_bits[] = {
   0x33, 0xcc, 0x33, 0xcc, 0x33, 0xcc, 0x33, 0xcc};

#define rb_off_width 15
#define rb_off_height 15
static char rb_off_bits[] = {
   0xe0, 0x03, 0x18, 0x0c, 0x04, 0x10, 0x02, 0x20, 0x02, 0x20, 0x01, 0x40,
   0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x02, 0x20, 0x02, 0x20,
   0x04, 0x10, 0x18, 0x0c, 0xe0, 0x03};

#define rb_on_width 15
#define rb_on_height 15
static char rb_on_bits[] = {
   0xe0, 0x03, 0x18, 0x0c, 0x04, 0x10, 0xc2, 0x21, 0xf2, 0x27, 0xf1, 0x47,
   0xf9, 0x4f, 0xf9, 0x4f, 0xf9, 0x4f, 0xf1, 0x47, 0xf2, 0x27, 0xc2, 0x21,
   0x04, 0x10, 0x18, 0x0c, 0xe0, 0x03};

#define rb_off1_width 15
#define rb_off1_height 15
static char rb_off1_bits[] = {
   0xe0, 0x03, 0xf8, 0x0f, 0x1c, 0x1c, 0x06, 0x30, 0x06, 0x30, 0x03, 0x60,
   0x03, 0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x60, 0x06, 0x30, 0x06, 0x30,
   0x1c, 0x1c, 0xf8, 0x0f, 0xe0, 0x03};

#define rb_on1_width 15
#define rb_on1_height 15
static char rb_on1_bits[] = {
   0xe0, 0x03, 0xf8, 0x0f, 0x1c, 0x1c, 0xc6, 0x31, 0xf6, 0x37, 0xf3, 0x67,
   0xfb, 0x6f, 0xfb, 0x6f, 0xfb, 0x6f, 0xf3, 0x67, 0xf6, 0x37, 0xc6, 0x31,
   0x1c, 0x1c, 0xf8, 0x0f, 0xe0, 0x03};

#define cb_off_width 15
#define cb_off_height 15
static char cb_off_bits[] = {
   0xff, 0x7f, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40,
   0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40,
   0x01, 0x40, 0x01, 0x40, 0xff, 0x7f};

#define cb_on_width 15
#define cb_on_height 15
static char cb_on_bits[] = {
   0xff, 0x7f, 0x01, 0x40, 0x01, 0x40, 0x01, 0x58, 0x01, 0x4c, 0x01, 0x46,
   0x0d, 0x43, 0x8d, 0x41, 0xd9, 0x40, 0x79, 0x40, 0x31, 0x40, 0x11, 0x40,
   0x01, 0x40, 0x01, 0x40, 0xff, 0x7f};

#define cb_off1_width 15
#define cb_off1_height 15
static char cb_off1_bits[] = {
   0xff, 0x7f, 0xff, 0x7f, 0x03, 0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x60,
   0x03, 0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x60,
   0x03, 0x60, 0xff, 0x7f, 0xff, 0x7f};

#define cb_on1_width 15
#define cb_on1_height 15
static char cb_on1_bits[] = {
   0xff, 0x7f, 0xff, 0x7f, 0x03, 0x60, 0x03, 0x78, 0x03, 0x6c, 0x03, 0x66,
   0x0f, 0x63, 0x8f, 0x61, 0xdb, 0x60, 0x7b, 0x60, 0x33, 0x60, 0x13, 0x60,
   0x03, 0x60, 0xff, 0x7f, 0xff, 0x7f};

#define Excl_width 40
#define Excl_height 32
static char Excl_bits[] = {
   0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00,
   0x7e, 0x00, 0x00, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x00,
   0x00, 0x00, 0x80, 0xc3, 0x01, 0x00, 0x00, 0xc0, 0x99, 0x03, 0x00, 0x00,
   0xc0, 0x34, 0x03, 0x00, 0x00, 0xe0, 0x6a, 0x07, 0x00, 0x00, 0x70, 0x76,
   0x0e, 0x00, 0x00, 0x70, 0x6a, 0x0e, 0x00, 0x00, 0x38, 0x76, 0x1c, 0x00,
   0x00, 0x18, 0x6a, 0x18, 0x00, 0x00, 0x1c, 0x76, 0x38, 0x00, 0x00, 0x0e,
   0x6a, 0x70, 0x00, 0x00, 0x06, 0x76, 0x60, 0x00, 0x00, 0x07, 0x6a, 0xe0,
   0x00, 0x80, 0x03, 0x76, 0xc0, 0x01, 0x80, 0x03, 0x6a, 0xc0, 0x01, 0xc0,
   0x01, 0x76, 0x80, 0x03, 0xc0, 0x00, 0x6a, 0x00, 0x03, 0xe0, 0x00, 0x34,
   0x00, 0x07, 0x70, 0x00, 0x18, 0x00, 0x0e, 0x70, 0x00, 0x00, 0x00, 0x0e,
   0x38, 0x00, 0x3c, 0x00, 0x1c, 0x1c, 0x00, 0x76, 0x00, 0x38, 0x1c, 0x00,
   0x6a, 0x00, 0x38, 0x0e, 0x00, 0x76, 0x00, 0x70, 0x0e, 0x00, 0x3c, 0x00,
   0x70, 0x1e, 0x00, 0x00, 0x00, 0x78, 0xfe, 0xff, 0xff, 0xff, 0x7f, 0xfc,
   0xff, 0xff, 0xff, 0x3f};

/**************************************************************************/

static Display *theDisp;
static unsigned long fg, bg, hicol, locol;
static int theScreen, dispDEEP, have_color, bwidth=2;
static Window rootW;
static GC theGC;
static XFontStruct *mfinfo;
static Font mfont;


static int rbpixmade, cbpixmade, mbpixmade;
static Pixmap grayStip, exclpix, cboard50, mbchk;
static Pixmap rbon, rboff, rbon1, rboff1;
static Pixmap cbon, cboff, cbon1, cboff1;

char *malloc();

/**************************************************************************/


init_buttons(dpy, fore, back, top, bot)
Display *dpy;
unsigned long fore, back, top, bot;
{
   fg        = fore;
   bg        = back;
   hicol     = top;
   locol     = bot;
   if(fore==top && back==bot)  have_color = 0;
   else                        have_color = 1;
   
   theDisp   = dpy;
   theScreen = DefaultScreen(theDisp);
   theGC     = DefaultGC(theDisp, theScreen);
   rootW     = RootWindow(theDisp, theScreen);
   dispDEEP  = DefaultDepth(theDisp, theScreen);

   /* try to load fonts */
   if( (mfinfo = XLoadQueryFont(theDisp,FONT1))==NULL && 
       (mfinfo = XLoadQueryFont(theDisp,FONT2))==NULL && 
       (mfinfo = XLoadQueryFont(theDisp,FONT3))==NULL && 
       (mfinfo = XLoadQueryFont(theDisp,FONT4))==NULL && 
       (mfinfo = XLoadQueryFont(theDisp,FONT5))==NULL) {
      FatalError("couldn't open the following fonts:\n%s\n%s\n%s\n%s\n%s",
		 FONT1, FONT2, FONT3, FONT4, FONT5);
   }
   mfont=mfinfo->fid;
   XSetFont(theDisp, theGC, mfont);

   XSetGraphicsExposures(theDisp, theGC, False);

   exclpix = cboard50 = NULL;
   rbpixmade = cbpixmade = mbpixmade = 0;

   if( !(grayStip = XCreatePixmapFromBitmapData(theDisp, rootW, gray50_bits,
		       gray50_width, gray50_height, 1, 0, 1)) )
      FatalError("Unable to create gray50 bitmap\n");
}


FatalError(str)
char *str;
{
   fprintf(stderr, str);
   exit(1);
}


/******************* BUTT ROUTINES ************************/

/**********************************************/
BTCreate(bp,win,x,y,w,h,str)
BUTT         *bp;
Window        win;
int           x,y,w,h;
char         *str;
{
  bp->win = win;
  bp->x = x;  bp->y = y;  bp->w = w;  bp->h = h;
  bp->str = str;
  bp->lit = 0;
  bp->active = 1;
  bp->toggle = 0;
  bp->pix = None;
  bp->style = 0;
  bp->fwidth = 3;

  if (!cboard50) {
    cboard50 = XCreatePixmapFromBitmapData(theDisp, rootW, cboard50_bits,
		       cboard50_width, cboard50_height, 1, 0, 1);
    if (!cboard50) FatalError("Unable to create cboard50 bitmap\n");
  }
}



/**********************************************/
BTSetActive(bp,act)
BUTT         *bp;
int           act;
{
  if(bp->active != act) {
    bp->active = act;
    BTRedraw(bp);
    XFlush(theDisp);
 }
}



/**********************************************/
BTRedraw(bp)
BUTT *bp;
{
  int i,x,y,w,h,r,x1,y1;
  XPoint tpts[10], bpts[10], ipts[5];

  x = bp->x;  y=bp->y;  w=bp->w;  h=bp->h;  r=bp->fwidth;

  if (!bp->active) bp->lit = 0;
  if (bp->lit) {
    r -= 1;
    if (r<0) r = 0;
  }

  /* set up 'ipts' */
  ipts[0].x = x+r;        ipts[0].y = y+r;         /* topleft */
  ipts[1].x = x+r;        ipts[1].y = y+h-r;       /* botleft */
  ipts[2].x = x+w-r;      ipts[2].y = y+h-r;       /* botright */
  ipts[3].x = x+w-r;      ipts[3].y = y+r;         /* topright */
  ipts[4].x = ipts[0].x;  ipts[4].y = ipts[0].y;   /* close path */

  /* top left polygon */
  tpts[0].x = x;            tpts[0].y = y;
  tpts[1].x = x;            tpts[1].y = y+h;
  tpts[2].x = ipts[1].x;    tpts[2].y = ipts[1].y;
  tpts[3].x = ipts[0].x;    tpts[3].y = ipts[0].y;
  tpts[4].x = ipts[3].x;    tpts[4].y = ipts[3].y;
  tpts[5].x = x+w;          tpts[5].y = y;
  tpts[6].x = x;            tpts[6].y = y;

  /* bot left polygon */
  bpts[0].x = x;            bpts[0].y = y+h;
  bpts[1].x = ipts[1].x;    bpts[1].y = ipts[1].y;
  bpts[2].x = ipts[2].x;    bpts[2].y = ipts[2].y;
  bpts[3].x = ipts[3].x;    bpts[3].y = ipts[3].y;
  bpts[4].x = x+w;          bpts[4].y = y;
  bpts[5].x = x+w;          bpts[5].y = y+h;
  bpts[6].x = x;            bpts[6].y = y+h;


  if (!have_color) {
    /* clear button and draw frame */
    XSetForeground(theDisp, theGC, bg);
    XFillRectangle(theDisp, bp->win, theGC, x, y, w, h);
    XSetForeground(theDisp, theGC, fg);
    XDrawRectangle(theDisp, bp->win, theGC, x, y, w, h);

    XSetForeground(theDisp, theGC, fg);
    XSetFillStyle(theDisp, theGC, FillStippled);
    XSetStipple(theDisp, theGC, cboard50);
    XFillPolygon(theDisp, bp->win, theGC, bpts, 7, Nonconvex, CoordModeOrigin);
    XSetFillStyle(theDisp,theGC,FillSolid);

    XSetForeground(theDisp, theGC, fg);
    XDrawLines(theDisp, bp->win, theGC, ipts, 5, CoordModeOrigin);  /* inset */

    XDrawLine(theDisp, bp->win, theGC, x+1,   y+1,  ipts[0].x,ipts[0].y);
    XDrawLine(theDisp, bp->win, theGC, x+1,   y+h-1,ipts[1].x,ipts[1].y);
    XDrawLine(theDisp, bp->win, theGC, x+w-1, y+h-1,ipts[2].x,ipts[2].y);
    XDrawLine(theDisp, bp->win, theGC, x+w-1, y+1,  ipts[3].x,ipts[3].y);

    if (bp->lit) {
      XDrawRectangle(theDisp, bp->win, theGC, x+2, y+2, w-4, h-4);
      XDrawRectangle(theDisp, bp->win, theGC, x+1, y+1, w-2, h-2);
    }
  }
    
  else {   /* have_color */
    XSetForeground(theDisp, theGC, bg);
    XFillRectangle(theDisp, bp->win, theGC, x+1, y+1, w-1, h-1);

    XSetForeground(theDisp, theGC, hicol);
    for (i=1; i<=r; i++) {
      XDrawLine(theDisp, bp->win, theGC, x+i, y+i, x+w, y+i);
      XDrawLine(theDisp, bp->win, theGC, x+i, y+i, x+i, y+h);
    }

    XSetForeground(theDisp, theGC, locol);
    for (i=1; i<=r; i++) {
      XDrawLine(theDisp, bp->win, theGC, x+i, y+h-i, x+w, y+h-i);
      XDrawLine(theDisp, bp->win, theGC, x+w-i, y+h, x+w-i, y+i);
    }

    XSetForeground(theDisp, theGC, fg);
    XDrawRectangle(theDisp, bp->win, theGC, x, y, w, h);

    if (bp->lit)
      XDrawRectangle(theDisp, bp->win, theGC, x+1, y+1, w-2, h-2);
  }
    
  XSetForeground(theDisp, theGC, fg);

  if (bp->pix != None) {                    /* draw pixmap centered in butt */
    x1 = x+(1+w-bp->pw)/2;
    y1 = y+(1+h-bp->ph)/2;

    XSetBackground(theDisp, theGC, bg);
    XCopyPlane(theDisp, bp->pix, bp->win, theGC, 0,0,bp->pw,bp->ph, x1,y1, 1);
    if (!bp->active) DimRect(bp->win, x1,y1, bp->pw, bp->ph, bg);
  }
  else {                                    /* draw string centered in butt */
    x1 = CENTERX(mfinfo, x + w/2, bp->str);
    y1 = CENTERY(mfinfo, y + h/2);

    if (bp->active) {
      XDrawString(theDisp, bp->win, theGC, x1,y1, bp->str, strlen(bp->str));
    }
    else {  /* stipple if not active */
      XSetFillStyle(theDisp, theGC, FillStippled);
      XSetStipple(theDisp, theGC, grayStip);
      XDrawString(theDisp, bp->win, theGC, x1,y1, bp->str, strlen(bp->str));
      XSetFillStyle(theDisp,theGC,FillSolid);
    }
  }

}


/**********************************************/
int BTClick(bt, mx, my)
BUTT *bt;
int    mx,my;
{
  if (PTINRECT(mx, my, bt->x, bt->y, bt->w, bt->h)) return 1;
  return 0;
}


/**********************************************/
int BTTrack(bp)
BUTT *bp;
{
  /* called when we've gotten a click inside 'bp'.  returns 1 if button
     was still selected lit when mouse was released. */

  Window       rW, cW;
  int          x, y, rx, ry, rval, inval;
  unsigned int mask;

  if (!bp->active) return 0;   /* inactive button */

  inval = bp->lit;
  bp->lit = !bp->lit;

  BTRedraw(bp);  XFlush(theDisp);
  /* sleep(1);  Timer(120);  long enough for turn on to be visible */

  while (XQueryPointer(theDisp,bp->win,&rW,&cW,&rx,&ry,&x,&y,&mask)) {
    if (!(mask & Button1Mask)) break;    /* button released */

    if (bp->lit==inval && PTINRECT(x, y, bp->x, bp->y, bp->w, bp->h)) {
      bp->lit = !inval;  BTRedraw(bp);  XFlush(theDisp);
    }
    
    if (bp->lit!=inval && !PTINRECT(x, y, bp->x, bp->y, bp->w, bp->h)) {
      bp->lit = inval;  BTRedraw(bp);  XFlush(theDisp);
    }
  }

  rval = (bp->lit != inval);
  
  if (bp->lit && !bp->toggle) 
    { bp->lit = 0;  BTRedraw(bp);  XFlush(theDisp); }

  return(rval);
}




/******************* RBUTT ROUTINES ************************/




/***********************************************/
RBUTT *RBCreate(rblist, win, x,y,str)
      RBUTT        *rblist;
      Window        win;
      int           x,y;
      char         *str;
{
  /* mallocs an RBUTT, fills in the fields, and appends it to rblist
     if rblist is NULL, this is the first rb in the list.  It will
     be made the 'selected' one 

     Note: no need to check return status.  It'll fatal error if it 
     can't malloc */

  RBUTT *rb, *rbptr;

  rb = (RBUTT *) malloc(sizeof(RBUTT));
  if (!rb) FatalError("couldn't malloc RBUTT");

  /* fill in the fields of the structure */
  rb->win      = win;
  rb->x        = x;
  rb->y        = y;
  rb->str      = str;
  rb->selected = 0;
  rb->active   = 1;
  rb->next     = (RBUTT *) NULL;

  if (rblist) {            /* append to end of list */
    rbptr = rblist;
    while (rbptr->next) rbptr = rbptr->next;
    rbptr->next = rb;
  }
  else {                   /* this is the first one in the list.  select it */
    rb->selected = 1;
  }


  /* and, on an unrelated note, if the RB pixmaps haven't been created yet,
     do so.  We'll be needing them, y'see... */

  if (!rbpixmade) {
    rbon  = XCreatePixmapFromBitmapData(theDisp, rootW, rb_on_bits,
	     rb_on_width, rb_on_height, fg, bg, dispDEEP);

    rboff = XCreatePixmapFromBitmapData(theDisp, rootW, rb_off_bits,
	     rb_off_width, rb_off_height, fg, bg, dispDEEP);
    rbon1 = XCreatePixmapFromBitmapData(theDisp, rootW, rb_on1_bits,
	     rb_on1_width, rb_on1_height, fg, bg, dispDEEP);
    rboff1= XCreatePixmapFromBitmapData(theDisp, rootW, rb_off1_bits,
	     rb_off1_width, rb_off1_height, fg, bg, dispDEEP);

    rbpixmade = 1;
  }

  return(rb);
}
  



/***********************************************/
RBRedraw(rblist, num)
RBUTT *rblist;
int    num;
{
  /* redraws the 'num-th' RB in the list.  if num < 0, redraws entire list */

  RBUTT *rb;
  int    i;

  /* point 'rb' at the appropriate RBUTT, *if* we're not drawing entire list */
  if (num>=0) {
    i=0;  rb=rblist;
    while (i!=num && rb) { rb = rb->next;  i++; }
    if (!rb) return;                     /* num is out of range.  do nothing */
    drawRB(rb);
  }

  else {                                 /* draw entire list */
    rb = rblist;
    while (rb) {
      drawRB(rb);
      rb = rb->next;
    }
  }
}


drawRB(rb)
RBUTT *rb;
{
  /* draws the rb being pointed at */

  if (!rb) return;  /* rb = NULL */

  XSetForeground(theDisp, theGC, fg);
  XSetBackground(theDisp, theGC, bg);

  if (rb->selected) 
    XCopyArea(theDisp, rbon, rb->win, theGC, 0, 0, rb_on_width, rb_on_height, 
	      rb->x, rb->y);
  else
    XCopyArea(theDisp, rboff, rb->win, theGC, 0, 0, rb_on_width, rb_on_height, 
	      rb->x, rb->y);

  XDrawString(theDisp, rb->win, theGC, rb->x+rb_on_width+4, 
	      rb->y+rb_on_height/2 - CHIGH/2 + ASCENT,rb->str,strlen(rb->str));

  if (!rb->active) {  /* if non-active, dim button and string */
    DimRect(rb->win, rb->x, rb->y, rb_on_width, rb_on_height, bg);
    DimRect(rb->win, rb->x + rb_on_width+4, rb->y+rb_on_height/2 - CHIGH/2, 
	    StringWidth(rb->str),CHIGH, bg);
  }
}


/***********************************************/
RBSelect(rblist, n)
RBUTT *rblist;
int    n;
{
  RBUTT *rbold, *rb;
  int    i;

  /* makes rb #n the selected rb in the list.  Does all redrawing.  Does
     nothing if rb already selected */

  /* get pointers to the currently selected rb and the desired rb */
  rbold = rblist;
  while (rbold && !rbold->selected) rbold = rbold->next;
  if (!rbold) return;    /* no currently selected item.  shouldn't happen */

  rb = rblist;  i=0;
  while (rb && i!=n) {rb = rb->next;  i++; }
  if (!rb) return;    /* 'n' is out of range */


  if (rb == rbold) return;   /* 'n' is already selected.  do nothing */

  rbold->selected = 0;
  rb->selected    = 1;
  drawRB(rbold);
  drawRB(rb);
}


	      
/***********************************************/
int RBWhich(rblist)
RBUTT *rblist;
{
  int i;

  /* returns index of currently selected rb.  if none, returns -1 */

  i = 0;
  while (rblist && !rblist->selected) { rblist = rblist->next;  i++; }

  if (!rblist) return -1;             /* didn't find one */
  return i;
}


/***********************************************/
int RBCount(rblist)
RBUTT *rblist;
{
  int i;

  /* returns # of rb's in the list */

  i = 0;
  while (rblist) { rblist = rblist->next; i++; }
  return i;
}


/***********************************************/
RBSetActive(rblist, n, act)
RBUTT *rblist;
int n,act;
{
  RBUTT *rb;
  int    i;

  /* sets 'active' status of rb #n.  does redrawing */

  rb=rblist;  i=0;
  while (rb && i!=n) { rb = rb->next; i++; }
  if (!rb) return;                         /* n out of range.  do nothing */

  if (rb->active != act) {
    rb->active = act;
    drawRB(rb);
  }
}


/***********************************************/
int RBClick(rblist, mx, my)
RBUTT *rblist;
int    mx,my;
{
  int i;

  /* searches through rblist to see if mouse click at mx,my is in the
     clickable region of any of the rb's.  If it finds one, it returns 
     it's index in the list.  If not, returns -1 */

  i = 0;
  while (rblist) {
    if (PTINRECT(mx, my, rblist->x, rblist->y, rb_on_width, rb_on_height)) 
      break;
    rblist = rblist->next;
    i++;
  }

  if (!rblist) return -1;
  return(i);
}


/***********************************************/
int RBTrack(rblist, n)
RBUTT *rblist;
int    n;
{
  RBUTT       *rb;
  Window       rW, cW;
  int          i, x, y, rx, ry, lit, rv;
  unsigned int mask;
  Pixmap litpix, darkpix;

  /* returns '1' if selection changed */

  rb=rblist;  i=0;
  while (rb && i!=n) { rb = rb->next; i++; }
  if (!rb) return 0;                    /* n out of range */

  /* called once we've figured out that the mouse clicked in 'rb' */

  if (!rb->active) return 0;

  if (rb->selected) { litpix = rbon1;   darkpix = rbon; }
               else { litpix = rboff1;  darkpix = rboff; }

  lit = 1;
  XCopyArea(theDisp, litpix, rb->win, theGC, 0, 0, rb_on_width, rb_on_height, 
	      rb->x, rb->y);
  XFlush(theDisp);
  /* sleep(1);  Timer(75);    give chance for 'turn on' to become visible */

  while (XQueryPointer(theDisp,rb->win,&rW,&cW,&rx,&ry,&x,&y,&mask)) {
    if (!(mask & Button1Mask)) break;    /* button released */

    if (!lit && PTINRECT(x, y, rb->x, rb->y, rb_on_width, rb_on_height)) {
      lit=1;
      XCopyArea(theDisp, litpix, rb->win, theGC, 0,0,rb_on_width,rb_on_height, 
	      rb->x, rb->y);
      XFlush(theDisp);
    }
    
    if (lit && !PTINRECT(x, y, rb->x, rb->y, rb_on_width, rb_on_height)) {
      lit=0;
      XCopyArea(theDisp, darkpix,rb->win,theGC, 0,0,rb_on_width,rb_on_height, 
	      rb->x, rb->y);
      XFlush(theDisp);
    }
  }

  rv = 0;

  if (lit) {
    XCopyArea(theDisp, darkpix, rb->win, theGC, 0, 0, 
	      rb_on_width, rb_on_height, rb->x, rb->y);
    if (RBWhich(rblist) != n) rv = 1;
    RBSelect(rblist, n);
  }

  XFlush(theDisp);
  return rv;
}




/******************* CBUTT ROUTINES ************************/




/***********************************************/
CBCreate(cb, win, x,y, str)
      CBUTT        *cb;
      Window        win;
      int           x,y;
      char         *str;
{
  /* fill in the fields of the structure */
  cb->win      = win;
  cb->x        = x;
  cb->y        = y;
  cb->str      = str;
  cb->val      = 0;
  cb->active   = 1;

  /* and, on an unrelated note, if the CB pixmaps haven't been created yet,
     do so.  We'll be needing them, y'see... */

  if (!cbpixmade) {
    cbon  = XCreatePixmapFromBitmapData(theDisp, rootW, cb_on_bits,
	     cb_on_width, cb_on_height, fg, bg, dispDEEP);

    cboff = XCreatePixmapFromBitmapData(theDisp, rootW, cb_off_bits,
	     cb_off_width, cb_off_height, fg, bg, dispDEEP);
    cbon1 = XCreatePixmapFromBitmapData(theDisp, rootW, cb_on1_bits,
	     cb_on1_width, cb_on1_height, fg, bg, dispDEEP);
    cboff1= XCreatePixmapFromBitmapData(theDisp, rootW, cb_off1_bits,
	     cb_off1_width, cb_off1_height, fg, bg, dispDEEP);

    cbpixmade = 1;
  }
}
  



/***********************************************/
CBRedraw(cb)
CBUTT *cb;
{
  /* draws the cb being pointed at */

  XSetForeground(theDisp, theGC, fg);
  XSetBackground(theDisp, theGC, bg);

  if (cb->val) 
    XCopyArea(theDisp, cbon, cb->win, theGC, 0, 0, cb_on_width, cb_on_height, 
	      cb->x, cb->y);
  else
    XCopyArea(theDisp, cboff, cb->win, theGC, 0, 0, cb_on_width, cb_on_height, 
	      cb->x, cb->y);

  XDrawString(theDisp, cb->win, theGC, cb->x+cb_on_width+4, 
	      cb->y+cb_on_height/2 - CHIGH/2 + ASCENT,cb->str,strlen(cb->str));

  if (!cb->active) {  /* if non-active, dim button and string */
    DimRect(cb->win, cb->x, cb->y, cb_on_width, cb_on_height, bg);
    DimRect(cb->win, cb->x + cb_on_width+4, cb->y+cb_on_height/2 - CHIGH/2, 
	    StringWidth(cb->str),CHIGH, bg);
  }
}


/**********************************************/
CBSetActive(cb,act)
CBUTT        *cb;
int           act;
{
  if (cb->active != act) {
    cb->active = act;
    CBRedraw(cb);
  }
}


/***********************************************/
int CBClick(cb, mx, my)
CBUTT *cb;
int    mx,my;
{
  if (PTINRECT(mx, my, cb->x, cb->y, cb_on_width, cb_on_height)) return 1;
  return 0;
}


/***********************************************/
int CBTrack(cb)
CBUTT *cb;
{
  Window       rW, cW;
  int          x, y, rx, ry, lit;
  unsigned int mask;
  Pixmap litpix, darkpix;

  /* called once we've figured out that the mouse clicked in 'cb' */

  if (!cb->active) return 0;

  if (cb->val) { litpix = cbon1;   darkpix = cbon; }
          else { litpix = cboff1;  darkpix = cboff; }

  lit = 1;
  XCopyArea(theDisp, litpix, cb->win, theGC, 0, 0, cb_on_width, cb_on_height, 
	      cb->x, cb->y);
  XFlush(theDisp);
  /* sleep(1);  Timer(75);      give chance for 'turn on' to become visible */

  while (XQueryPointer(theDisp,cb->win,&rW,&cW,&rx,&ry,&x,&y,&mask)) {
    if (!(mask & Button1Mask)) break;    /* button released */

    if (!lit && PTINRECT(x, y, cb->x, cb->y, cb_on_width, cb_on_height)) {
      lit=1;
      XCopyArea(theDisp, litpix, cb->win, theGC, 0,0,cb_on_width,cb_on_height, 
	      cb->x, cb->y);
      XFlush(theDisp);
    }
    
    if (lit && !PTINRECT(x, y, cb->x, cb->y, cb_on_width, cb_on_height)) {
      lit=0;
      XCopyArea(theDisp, darkpix,cb->win,theGC, 0,0,cb_on_width,cb_on_height, 
	      cb->x, cb->y);
      XFlush(theDisp);
    }
  }

  if (lit) {
    XCopyArea(theDisp, darkpix, cb->win, theGC, 0, 0, 
	      cb_on_width, cb_on_height, cb->x, cb->y);
    cb->val = !cb->val;
    CBRedraw(cb);
  }

  XFlush(theDisp);

  return(lit);
}




/******************* MBUTT ROUTINES ************************/




/***********************************************/
MBCreate(mb, win, x,y,w,h, str, list, nlist, check)
      MBUTT        *mb;
      Window        win;
      int           x,y,w,h;
      char         *str;
      char        **list;
      int           nlist;
      int           check;
{
  XSetWindowAttributes xswa;
  unsigned long        xswamask;

  if (!mbpixmade) {
    mbchk = XCreatePixmapFromBitmapData(theDisp, rootW, mb_chk_bits,
	     mb_chk_width, mb_chk_height, fg, bg, dispDEEP);
    mbpixmade = 1;
  }


  /* fill in the fields of the structure */
  mb->win      = win;
  mb->x        = x;
  mb->y        = y;
  mb->w        = w;
  mb->h        = h;
  mb->title    = str;
  mb->selected = -1;
  mb->active   = 1;
  mb->list     = list;
  mb->nlist    = nlist;
  mb->check    = check;

  mb->pix      = (Pixmap) NULL;
  mb->pw = mb->ph = 0;


  /* create popup window (it gets mapped, pos'd and sized later) */
  xswa.background_pixel = bg;
  xswa.border_pixel     = fg;
  xswa.save_under       = True;
  xswamask = CWBackPixel | CWBorderPixel | CWSaveUnder;

  mb->mwin = XCreateWindow(theDisp, mb->win, x, y, w, h, 
			bwidth, dispDEEP, InputOutput,
			CopyFromParent, xswamask, &xswa);

  if (!mb->mwin) FatalError("can't create popup menu window!");

  XSelectInput(theDisp, mb->mwin, ExposureMask | VisibilityChangeMask);
  XSetTransientForHint(theDisp, mb->mwin, mb->win);
}
  



/***********************************************/
MBRedraw(mb)
MBUTT *mb;
{
  /* draws a menu button in it's normal state.  (When it's actively being
     used (to select an item), all drawing is handled in MBTrack) */

  int x,y,w,h,i,r,x1,y1;

  r = 2;  /* amt of shadow */
  x = mb->x;  y = mb->y;  w = mb->w;  h = mb->h;

  XSetForeground(theDisp, theGC, bg);
  XFillRectangle(theDisp, mb->win, theGC, x+1, y+1, w-1, h-1);

  XSetForeground(theDisp, theGC, fg);
  XDrawRectangle(theDisp, mb->win, theGC, x, y, w, h);

  /* draw shadow */
  for (i=1; i<=r; i++) {
    XDrawLine(theDisp, mb->win, theGC, x+r, y+h+i, x+w+i, y+h+i);
    XDrawLine(theDisp, mb->win, theGC, x+w+i, y+h+i, x+w+i, y+r);
  }

  if (mb->pix != None) {                    /* draw pixmap centered in butt */
    x1 = x + (1+w - mb->pw)/2;
    y1 = y + (1+h - mb->ph)/2;

    XSetForeground(theDisp, theGC, fg);
    XSetBackground(theDisp, theGC, bg);
    XCopyPlane(theDisp, mb->pix, mb->win, theGC, 0,0,mb->pw,mb->ph, x1,y1, 1);
    if (!mb->active) DimRect(mb->win, x1,y1, mb->pw, mb->ph, bg);
  }

  else {                                    /* draw string centered in butt */
    char *str;

    if (mb->title) str = mb->title;
    else {
      if (mb->selected>=0 && mb->selected<mb->nlist) 
	str = mb->list[mb->selected];
      else str = "";
    }

    x1 = CENTERX(mfinfo, x + w/2, str);
    y1 = CENTERY(mfinfo, y + h/2);

    if (mb->active) {
      XDrawString(theDisp, mb->win, theGC, x1,y1, str, strlen(str));
    }
    else {  /* stipple if not active */
      XSetFillStyle(theDisp, theGC, FillStippled);
      XSetStipple(theDisp, theGC, grayStip);
      XDrawString(theDisp, mb->win, theGC, x1,y1, str, strlen(str));
      XSetFillStyle(theDisp,theGC,FillSolid);
    }
  }
}


/**********************************************/
MBSetActive(mb,act)
MBUTT        *mb;
int           act;
{
  if (mb->active != act) {
    mb->active = act;
    MBRedraw(mb);
    XFlush(theDisp);
 }
}


/***********************************************/
int MBClick(mb, mx, my)
MBUTT *mb;
int    mx,my;
{
  if (PTINRECT(mx, my, mb->x, mb->y, mb->w, mb->h)) return 1;
  return 0;
}


/***********************************************/
int MBTrack(mb)
MBUTT *mb;
{
  Window       rW, cW, win;
  int          i, x, y, rx, ry, extratop;
  unsigned int mask;
  int          mwide, mhigh, mx, my, j, lit, lastlit, changed=0;
  XSizeHints   hints;
  XEvent       event;


  /* called once we've figured out that the mouse clicked in 'mb'
     returns '1' if selection changed */

  if (!mb->active || !mb->nlist) return 0;

  /* invert the button, for visual feedback */
  XSetFunction(theDisp, theGC, GXinvert);
  XSetPlaneMask(theDisp, theGC, fg ^ bg);
  XFillRectangle(theDisp, mb->win, theGC, mb->x+1, mb->y+1, mb->w-1, mb->h-1);
  XSetFunction(theDisp, theGC, GXcopy);
  XSetPlaneMask(theDisp, theGC, ~0L);

  extratop = (mb->title) ? LINEHIGH+3 : 1-SPACING; /*add extra line for title*/


  mwide = 1;                              /* compute maximum width */
  for (i=0; i<mb->nlist; i++) {
    j = StringWidth(mb->list[i]);
    if (j > mwide) mwide = j;
  }
  mwide += 8;                             /* extra room at edges */
  if (mb->selected >=0 && mb->check) mwide += 8;
  if (mwide < (mb->w+1)) mwide = mb->w+1; /* at least as wide as button */
    
  mhigh = mb->nlist * LINEHIGH + 2 + extratop;

  mx = mb->x-1;  my = mb->y - 1;
  if (mb->check && mwide > mb->w) mx -= ((mwide - mb->w)/2);

  /* create/map window, and warp mouse if we had to move the window */
  win = mb->mwin;
  XMoveResizeWindow(theDisp, win, mx, my, mwide, mhigh);

  hints.width  = hints.min_width  = hints.max_width  = mwide;
  hints.height = hints.min_height = hints.max_height = mhigh;
  hints.x = mx;  hints.y = my;
  hints.flags  = (USSize | PMinSize | PMaxSize | PPosition);
  XSetNormalHints(theDisp, win, &hints);

  XMapRaised(theDisp, win);

  /* wait for window to become mapped */
  XWindowEvent(theDisp, win, VisibilityChangeMask, &event);


  /* draw the menu */
  XSetForeground(theDisp, theGC, fg);
  x = (mb->selected >= 0 && mb->check) ? 12 : 4;
  if (mb->title) {                /* draw a title on this menu */
    CenterString(win, mb->title, mwide/2-1, (extratop-2)/2);
    XDrawLine(theDisp, win, theGC, 0, extratop-2, mwide, extratop-2);
    XDrawLine(theDisp, win, theGC, 0, extratop,   mwide, extratop);
  }

  y = ASCENT + SPACING + extratop;
  for (i=0; i<mb->nlist; i++) {
    if (i == mb->selected && mb->check) {
      XCopyArea(theDisp, mbchk, win, theGC, 0, 0, mb_chk_width, mb_chk_height, 
	      x - 10, y - 8);
    }
      
    XDrawString(theDisp, win, theGC, x, y, mb->list[i], strlen(mb->list[i]));
    y += LINEHIGH;
  }

  XFlush(theDisp);


  /* track the mouse */
  XSetFunction(theDisp, theGC, GXinvert);         /* go in to 'invert' mode */
  XSetPlaneMask(theDisp, theGC, fg ^ bg);

  lit = lastlit = -1;
  while (XQueryPointer(theDisp,win,&rW,&cW,&rx,&ry,&x,&y,&mask)) {
    if (!(mask & Button1Mask)) break;    /* button released */

    /* determine which choice the mouse is in.  -1 if none */
    j = extratop+2;
    if (x < 0 || x > mwide) lit = -1;
    else {
      for (i=0; i<mb->nlist; i++, j+=LINEHIGH) {
	if (y>=j && y < j+LINEHIGH) { lit = i; break; }
      }
      if (i == mb->nlist) lit = -1;
    }

    if (lit != lastlit) {
      if (lit >= 0) {
	y = extratop + 2 + lit*LINEHIGH;
	XFillRectangle(theDisp, win, theGC, 0, y, mwide, LINEHIGH);
      }
      if (lastlit >= 0) {
	y = extratop + 2 + lastlit*LINEHIGH;
	XFillRectangle(theDisp, win, theGC, 0, y, mwide, LINEHIGH);
      }
      lastlit = lit;
    }
  }

  XSetFunction(theDisp, theGC, GXcopy);   /* back to 'normal' mode */
  XSetPlaneMask(theDisp, theGC, ~0L);

  /* could try eating all remaining events for 'win' before unmapping */

  XSync(theDisp, False);
  XUnmapWindow(theDisp, win);

  if (lit >= 0 && lit != mb->selected) {
    mb->selected = lit;
    changed = 1;
  }

  MBRedraw(mb);
  XFlush(theDisp);

  return changed;
}


DimRect(win, x, y, w, h, backg)
Window win;
int x,y,w,h;
u_long backg;
{
  /* stipple a rectangular region by drawing 'backg' where there's 1's 
     in the stipple pattern */

  XSetFillStyle(theDisp, theGC, FillStippled);
  XSetStipple(theDisp, theGC, grayStip);
  XSetForeground(theDisp, theGC, backg);
  XFillRectangle(theDisp,win,theGC,x,y,w,h);
  XSetFillStyle(theDisp, theGC, FillSolid);
}


int StringWidth(str)
char *str;
{
  return(XTextWidth(mfinfo, str, strlen(str)));
}


CenterString(win,str,x,y)
Window win;
char *str;
int x,y;
{
  XDrawString(theDisp, win, theGC, CENTERX(mfinfo, x, str),
	    CENTERY(mfinfo, y), str, strlen(str));
}





/************************ POPUP ROTINES ***************************/


/*
 * pops up a requester near the mouse with 'title'.
 * displays an exclamation mark if excl is true, the 'message' string
 * and 'nbts' buttons labeled 'btnames[0...nbts-1]'.
 * when 'retstr' is not NULL a text input box is used with the string at
 * 'retstr' as the default input. 'retmaxb' specifies the maximum byte count
 * which is to be returned.
 * popup() returns 0...nbts-1 when a button was pressed or nbts when using
 * a text box and CR was typed.
 */

#define BTW 70
#define BTH 20
#define STH 20

static Window popwin;
static int popwidth, popheight, boxwidth;
static BUTT *popbuts;
static int curPos, stPos, enPos, echo;
static char dashstr[] = "----------------------------------------------------";

int popup(title, excl, message, btnames, nbts, retstr, retmaxb, do_echo)
char *title, *message, *retstr, **btnames;
int nbts, retmaxb, excl, do_echo;
{
   XSizeHints hints;
   XEvent event;
   Window rw, cw;
   int wx, wy, x, y, i;
   unsigned int mask;

   /* compute window geometry */
   x = XTextWidth(mfinfo, message, strlen(message)) + excl*Excl_width + 80;
   y = 100 + nbts*BTW + 10;
   popwidth  = (x>y) ? x : y;
   popheight = 10+BTH+10+(retstr!=NULL)*(STH+10)+(excl?Excl_height:15)+10;
   
   XQueryPointer(theDisp, rootW, &rw, &cw, &x, &y, &wx, &wy, &mask);
   x -= popwidth/2 ;  if(x<0) x=0;
   y -= popheight/2;  if(y<0) y=0;
   
   popwin = XCreateSimpleWindow(theDisp, rootW, x, y,
				popwidth, popheight, 5, fg, bg );

   /* inform window manager */
   hints.width  = hints.min_width  = hints.max_width  = popwidth;
   hints.height = hints.min_height = hints.max_height = popheight;
   hints.x = x;
   hints.y = y;
   hints.flags  = USPosition | PSize | PMinSize | PMaxSize;
   XSetStandardProperties(theDisp, popwin, title, NULL, None,
			  NULL, 0, &hints);

   if(excl && !exclpix) {
      if( !(exclpix = XCreatePixmapFromBitmapData(theDisp, popwin, Excl_bits,
			 Excl_width, Excl_height, 1, 0, 1)) ) {
	 fprintf(stderr, "can't create Excl pixmap\n");
	 exit(1);
      }
   }

   if(!(popbuts = (BUTT *) malloc(nbts*sizeof(BUTT))) ) {
      fprintf(stderr, "can't malloc() popup buttons !\n");
      exit(1);
   }
   for (i=0; i<nbts; i++)
      BTCreate(&popbuts[i], popwin, popwidth - (nbts-i)*(BTW+10),
	       popheight-10-BTH, BTW, BTH, btnames[i]);

   if(retstr) {
      boxwidth = popwidth-10-10;
      retstr[retmaxb-1] = 0;
      curPos = strlen(retstr);
      stPos = 0;
      enPos = curPos;
      echo = do_echo;
   }
   
   XMapRaised(theDisp, popwin);

   XSelectInput(theDisp, popwin, ExposureMask|KeyPressMask|ButtonPressMask);


   while(1) {
      XWindowEvent(theDisp, popwin,
		   ExposureMask|KeyPressMask|ButtonPressMask, &event);
      /* XNextEvent(theDisp, &event);
	 if(event.xany.window != popwin) continue;
      */
      
      switch(event.type) {
         case Expose:
	    XSetForeground(theDisp, theGC, fg);
	    XSetBackground(theDisp, theGC, bg);
            if(excl) XCopyPlane(theDisp, exclpix, popwin, theGC, 0, 0,
				Excl_width, Excl_height, 10, 10, 1L);
	    XDrawString(theDisp, popwin, theGC, excl ? 10+Excl_width+10 : 10,
			excl ? 10+Excl_height : 10+10,
			message, strlen(message));
	    if(retstr) redraw_strbox(retstr);
	    for(i=0; i<nbts; i++)  BTRedraw(&popbuts[i]);
	    break;
         case KeyPress:
	    if(retstr && popup_key(&event, retstr, retmaxb)) {
	       free_popup();
	       return nbts;
	    }
	    break;
         case ButtonPress:
	    if(event.xbutton.button != Button1)  continue;
	    for(i=0; i<nbts; i++)
	       if(BTClick(&popbuts[i], event.xbutton.x, event.xbutton.y))
		  if(BTTrack(&popbuts[i])) {
		     free_popup();
		     return i;
		  }
	    break;
      }
   }

}


int popup_key(event, filename, max)
XEvent *event;
char *filename;
int max;
{
   char buf[128];
   KeySym ks;
   int stlen;
	
   stlen = XLookupString((XKeyEvent *) event, buf, 128, &ks,
			 (XComposeStatus *) NULL);

   if(ks==XK_Home)   return DirKey('\001', filename, max);
   if(ks==XK_End)    return DirKey('\005', filename, max);
   if(ks==XK_Left)   return DirKey('\002', filename, max);
   if(ks==XK_Right)  return DirKey('\006', filename, max);
   if(stlen)         return DirKey(buf[0], filename, max);
   return 0;
}


int DirKey(c, filename, max)
char *filename;
int c, max;
{
  int len = strlen(filename);
  
  if (c>=' '    && c<'\177'       /* printable characters */
  ||  c>='\200' && c<='\377' ) {  /* STUPID AM... - There *are* printable characters beyond DEL! */
    if (len >= max-1) return 0;  /* max length of string */
    bcopy(&filename[curPos], &filename[curPos+1], len-curPos+1);
    filename[curPos]=c;  curPos++;
  }

  else if (c=='\010' || c=='\177') {    /* BS or DEL */
    if (curPos==0) return 0;          /* at beginning of str */
    bcopy(&filename[curPos], &filename[curPos-1], len-curPos+1);
    curPos--;
  }

  else if (c=='\013') {                 /* ^K: clear to end of line */
    filename[curPos] = '\0';
  }

  else if (c=='\001') {                 /* ^A: move to beginning */
    curPos = 0;
  }

  else if (c=='\005') {                 /* ^E: move to end */
    curPos = len;
  }

  else if (c=='\004') {                 /* ^D: delete character at curPos */
    if (curPos==len) return 0;
    bcopy(&filename[curPos+1], &filename[curPos], len-curPos);
  }

  else if (c=='\002') {                 /* ^B: move backwards char */
    if (curPos==0) return 0;
    curPos--;
  }

  else if (c=='\006') {                 /* ^F: move forwards char */
    if (curPos==len) return 0;
    curPos++;
  }

  else if (c=='\012' || c=='\015') {    /* CR or LF */
     return 1;
  }

  else return 0;                      /* unhandled character */

  redraw_strbox(filename);
  return 0;
}


redraw_strbox(filename)
char *filename;
{
  int len=strlen(filename), cpos, x=10, y=popheight-(10+BTH+10+STH);

  if(!echo)  filename = dashstr;
  
  if (curPos<stPos) stPos = curPos;
  if (curPos>enPos) enPos = curPos;

  if (stPos>len) stPos = (len>0) ? len-1 : 0;
  if (enPos>len) enPos = (len>0) ? len-1 : 0;

  /* while substring is shorter than window, inc enPos */

  while (XTextWidth(mfinfo, &filename[stPos], enPos-stPos) < boxwidth-6
	 && enPos<len) { enPos++; }

  /* while substring is longer than window, dec enpos, unless enpos==curpos,
     in which case, inc stpos */

  while (XTextWidth(mfinfo, &filename[stPos], enPos-stPos) > boxwidth-6) {
    if (enPos != curPos) enPos--;
    else stPos++;
  }


  /* draw substring filename[stPos:enPos] and cursor */

  XSetForeground(theDisp, theGC, bg);
  XFillRectangle(theDisp, popwin, theGC, x+1, y+1, boxwidth-2, STH-2);
  XSetForeground(theDisp, theGC, fg);
  XDrawRectangle(theDisp, popwin, theGC, x, y, boxwidth-1, STH-1);

  if (stPos>0)   /* draw a "there's more over here" doowah */
     XFillRectangle(theDisp, popwin, theGC, x, y, 3, STH);

  if (enPos<len)   /* draw a "there's more over here" doowah */
     XFillRectangle(theDisp, popwin, theGC, x+boxwidth-3, y, 3, STH);

  XDrawString(theDisp, popwin, theGC, x+3, y+ASCENT+3,
	      filename+stPos, enPos-stPos);

  cpos = XTextWidth(mfinfo, &filename[stPos], curPos-stPos);
  XDrawLine(theDisp, popwin, theGC, x+3+cpos, y+2, x+3+cpos, y+2+STH-7);
  XDrawLine(theDisp, popwin, theGC, x+3+cpos, y+2+STH-6, x+5+cpos, y+2+STH-4);
  XDrawLine(theDisp, popwin, theGC, x+3+cpos, y+2+STH-6, x+1+cpos, y+2+STH-4);
}


free_popup()
{
   free(popbuts);
   XDestroyWindow(theDisp, popwin);
   XSync(theDisp, False);
}
