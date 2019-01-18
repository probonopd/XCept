/* 
 * modified  'xv.h'  from John Bradley
 */

typedef struct { Window win;            /* window ID */
		 int len;               /* length of major axis */
		 int vert;              /* true if vertical, else horizontal */
		 int active;            /* true if scroll bar can do anything*/
		 int min,max;           /* min/max values 'pos' can take */
		 int val;               /* 'value' of scrollbar */
		 int page;              /* amt val change on pageup/pagedown */
		 int tpos;              /* thumb pos. (pixels from tmin) */
		 int tmin,tmax;         /* min/max thumb offsets (from 0,0) */
		 int tsize;             /* size of thumb (in pixels) */
		 void (*drawobj)();     /* redraws obj controlled by scrl*/
		 int uplit, dnlit;      /* true if up&down arrows are lit */
	       } SCRL;

typedef struct { Window win;            /* window ID */
		 int w,h;               /* size of window */
		 int active;            /* true if can do anything*/
		 int min,max;           /* min/max values 'pos' can take */
		 int val;               /* 'value' of dial */
		 int page;              /* amt val change on pageup/pagedown */
		 char *title;           /* title for this guage */
		 char *units;           /* string appended to value */
		 int rad, cx, cy;       /* internals */
		 int bx[4], by[4];      /* more internals */
		 void (*drawobj)();     /* redraws obj controlled by dial */
	       } DIAL;

typedef struct { Window win;            /* parent window */
		 int x,y,w,h;           /* size of button rectangle */
		 int lit;               /* if true, invert colors */
		 int active;            /* if false, stipple gray */
		 int toggle;            /* if true, clicking toggles state */
		 char *str;             /* string in button */
		 Pixmap pix;            /* use pixmap instead of string */
		 int pw,ph;             /* size of pixmap */
		 int style;             /* ... */
		 int fwidth;            /* width of frame */
	       } BUTT;


typedef struct rbutt { Window        win;      /* parent window */
		       int           x,y;      /* position in parent */
		       char         *str;      /* the message string */
		       int           selected; /* selected or not */
		       int           active;   /* selectable? */
		       struct rbutt *next;     /* pointer to next in group */
		     } RBUTT;


typedef struct cbutt { Window        win;      /* parent window */
		       int           x,y;      /* position in parent */
		       char         *str;      /* the message string */
		       int           val;      /* 1=selected, 0=not */
		       int           active;   /* selectable? */
		     } CBUTT;


typedef struct mbutt { Window        win;      /* parent window */
		       int           x,y,w,h;  /* position in parent */
		       char         *title;    /* title string in norm state */
		       int           selected; /* item# selected, or -1 */
		       int           active;   /* selectable? */
		       char        **list;     /* list of strings in menu */
		       int           nlist;    /* # of strings in menu */
		       int           check;    /* mark last selected item ? */
		       Pixmap pix;             /* use pixmap instd of string */
		       int pw,ph;              /* size of pixmap */
		       Window        mwin;     /* popup menu window */
		     } MBUTT;


typedef struct { Window win;            /* window */
		 int x,y,w,h;           /* size of window */
		 char **str;            /* ptr to list of strings */
		 int   nstr;            /* number of strings */
		 int   selected;        /* number of 'selected' string */
		 int   nlines;          /* number of lines shown at once */
		 SCRL  scrl;            /* scrollbar that controls list */
		 int   filetypes;       /* true if filetype icons to be drawn*/
		 int   dirsonly;        /* if true, only dirs selectable */
	       } LIST;


#define MAX_GHANDS 16   /* maximum # of GRAF handles */

#define N_GFB 6
#define GFB_SPLINE 0
#define GFB_LINE   1
#define GFB_ADDH   2
#define GFB_DELH   3
#define GFB_RESET  4
#define GFB_GAMMA  5

#define GVMAX 8

typedef struct {  Window win;          /* window ID */
		  Window gwin;         /* graph subwindow */
		  int    spline;       /* spline curve or lines? */
		  int    entergamma;   /* currently entering gamma value */
		  int    gammamode;    /* currently using gamma function */
		  double gamma;        /* gamma value (if gammamode) */
		  int    nhands;       /* current # of handles */
		  XPoint hands[MAX_GHANDS];   /* positions of handles */
		  unsigned char func[256];    /* output function of GRAF */
		  BUTT   butts[N_GFB]; /* control buttons */
		  char   *str;         /* title string */
		  char   gvstr[GVMAX+1];    /* gamma value input string */
		} GRAF;

typedef struct {  int    spline;
		  int    entergamma;
		  int    gammamode;
		  double gamma;
		  int    nhands;
		  XPoint hands[MAX_GHANDS];
		  char   gvstr[GVMAX+1];
		} GRAF_STATE;
