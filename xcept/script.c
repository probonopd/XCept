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

#include 	<sys/types.h>
#include 	<fcntl.h>
#include	<string.h>
#include 	<stdio.h>
#include 	<stdlib.h>
#include 	<ctype.h>
#include 	<sys/stat.h>
#if 0
#include	<sys/termio.h>
#endif
#include 	"attrib.h"
#include 	"font.h"
#include 	"control.h"
#include        "protocol.h"

#define INSTRUCTION 	0
#define VARIABLE    	1
#define LABEL       	2
#define MAXSTRING       256
#define MAXVAR          2500
#define MAXLABEL        1000
#define get()           getc(sfile)
#define unget(c)        ungetc(c,sfile) 

#define dp(c)		((void)(debug && fprintf c))

static int
    s_set(),		/* assign value to variable */
    s_write(),		/* write value of variable to file */
    s_read(),		/* assign value to variable from file */
    s_append(),		/* append value of variable to file */
    s_strcat(),		/* like C strcat */
    s_send(),		/* send value of variable to btx center */
    s_get(),		/* assign value to variable from rectangle of page */
    s_waitdct(),	/* wait for DCT */
    s_if(),		/* do if */
    s_info(),		/* print message to user */
    s_debug(),		/* print debug message to stderr */
    s_goto(),		/* do goto */
    s_inc(),		/* increment variable */
    s_dec(),		/* decrement variable */
    s_setpart(),	/* assign value to variable from a part of another v.*/
    s_system(),		/* execute a system command */
    s_sleep(),		/* do sleep */
    s_onx(),		/* switch on X11 */
    s_offx(),		/* switch off X11 */
    s_input(),          /* request user input */
    s_connect(),        /* connect to btx server */
    s_playback(),       /* playback session */
    s_disconnect(),     /* disconnect */
    s_ppmsave(),        /* save page as PPM */
    s_getstate(),       /* get terminal status */
    s_quit();           /* quit XBTX */

static struct instruction {     
   char *name;
   char *args;
   int  (*function)();
} instructions[]={

	/* %a = variable or string, %v = variable, %l = label */

	"set",		"%v=%a",	  	s_set,         
	"write",	"%a to %a",	  	s_write,  
	"read",		"%v from %a",	  	s_read,   
	"append",	"%v to %a",	  	s_append, 
	"strcat",	"%v,%a",	  	s_strcat, 
	"send",		"%a",		  	s_send,   
	"get",	        "%v=%a,%a,%a,%a", 	s_get,    
	"waitdct",	" ",		  	s_waitdct,
	"if",		"%a == %a goto %l", 	s_if,          
	"info",		"%a",		  	s_info,        
	"debug",	"%a",			s_debug,
	"goto",		"%l",		  	s_goto,   
	"inc",		"%v",		  	s_inc,    
	"dec",		"%v",		  	s_dec,    
	"setpart",	"%v=%a,%a,%a",	  	s_setpart,
	"system",	"%a",		  	s_system, 
	"sleep",        "%a",              	s_sleep,
	"onx",          " ",              	s_onx,
 	"offx",         " ",              	s_offx,
	"input",        "%v:%a,%a",             s_input,
	"connect",      " ",                    s_connect,
	"disconnect",   " ",                    s_disconnect,
	"playback",     "%a",                   s_playback,
	"ppmsave",      "%a",                   s_ppmsave,
	"getstate",     "%v",                   s_getstate,
	"quit",         " ",                    s_quit,
	0,		  0,		  0         
};

typedef struct {
   char type;
   union {
      int (*function )();
      int variable;
      int label;
   } data;
} ADDRESS;

typedef  struct {
   char *name;
   char *value;
} VAR;

static struct label {
   char *name;
   int pc;
} label[MAXLABEL];

static labelcnt = 0;
static VAR var[MAXVAR];
static varcnt = 0;

static ADDRESS *programm = 0;
static int pc = 0,endpc = 0;
static int maxprog = 0;
static linenum = 0;
static FILE *sfile;
static char *display = NULL;
static int debug = 0;

static char 	*getstring();
static char 	*getidentifier();
static 		freescript(), addlabel();
static 		makevar(), setidentifier(), skip();
static 		initnext(), initargs();
static          error(), errorp();
static          nextinstr();
static void     *ralloc();
static          initargc();

static int wait_for_dct;


/************************    extern called   ********************************/

/* Initscript():
 * this has to be called first with the filename of the script as argv[0]
 * RETURN VALUES:   -1 on error in script file
 *                   0 on success
 */

initscript(argc, argv)
int argc;
char *argv[];
{
   int status,i;

   freescript();
   if((sfile = strcmp(argv[0], "-") ? fopen(argv[0],"r")
				    : stdin		) == NULL)
	return errorp("filename: %s\n", argv[0]);
   pc = 0;
   linenum = 1;

   initargc(argc);
   for(i=0; i<argc; i++)  if(-1 == initargs(i,argv[i])) goto err;
   
   while(EOF !=  (status = initnext()) ) {
      if(!status) goto err;
   }
   fclose(sfile);
   endpc = pc;
   linenum = pc = 0;
   wait_for_dct = 0;
   return 0;

 err:
   fclose(sfile);
   freescript();
   return -1;
}


/* script_next_dct():
 * executes the script until script asks waiting for the next DCT.
 * returns 1 when it wants to be called again (after DCT),
 *         0 on completion or error
 */

int script_next_dct()
{
   while(!wait_for_dct) {
      if(nextinstr() < 0)  return 0;
   }
   wait_for_dct = 0;
   return 1;
}


/**************************************************************************/

/* nextinstr():
 * executes the next instruction to be executed.
 * RETURN VALUES:        0 on succes
 *			 EOF on end of programm;
 *                     	 -2 on programm error;
 */

static nextinstr()
{
   int pcc=pc;
   
   if(pc >= endpc){
      freescript();
      return EOF;
   }
   if(programm[pc].type != INSTRUCTION){
      error("pc: %d - Programm error !\n",pc);
      freescript();
      return -2;
   }
   pc++;
   if(-1 == (*programm[pcc].data.function)()) return -2;
   return 0;
}


/* freescript:
 * frees all memory used by the script and is called by nextinstr()
 * if the end of the script is reached or on error during execution
 */

static freescript()
{
   int i;
   
   for(i=0; i<varcnt; i++){
      if(var[i].name)  free(var[i].name);
      if(var[i].value) free(var[i].value);
   }
   for(i=0; i<labelcnt; i++) if(label[i].name) free(label[i].name);
   if(programm) free(programm);
   programm = 0;
   maxprog = 0;
   pc = endpc = 0;
   varcnt = 0;
   labelcnt = 0;
   linenum = 0;
}
   

static skipspace()
{
   int c;
   
   while((c = get()) == ' ' || c== '\t');
   unget(c);
}


static getarg()
{
   char str[MAXSTRING];
   int c;
   
   skipspace();
   if((c = get()) == '"') {
      unget(c);
      if(!getstring(str)) return -1;
      dp((stderr," \"%s\" ",str));
      return makevar(str);
   } else {
      unget(c);
      if(!getidentifier(str)) return -1;
      dp((stderr," %s ",str));
      return setidentifier(str);
   }
}

static getlabel()
{
   char str[MAXSTRING];
   int i,ok;
   
   if(!getidentifier(str)) return -1;
   for(i = 0, ok = 0; i < labelcnt; i++)
      if(! strcmp(str,label[i].name)) return i;
   return addlabel(str,-1);
}


static addlabel(str,addr)
char *str; int addr;
{
   if(! (label[labelcnt].name = malloc(strlen(str)+1)))
      return errorp("malloc\n");
   strcpy(label[labelcnt].name,str);
   label[labelcnt].pc = addr;
   labelcnt++;
   return labelcnt -1;
}


static setlabel(str,addr)
char *str; int addr;
{
   int i,ok;
   
   for(i=0,ok=0; i<labelcnt; i++)
      if(! strcmp(str,label[i].name)){
	 label[i].pc = addr;
	 return addr;
      }
   return addlabel(str,addr);
   
}


static char *getstring(str)     /* get a c string from file */
char *str;
{
   int c,cnt=0,beg=0;

   for(;;) {
      c = get();
      if(!isprint(c)) goto err;
      switch(c){
       case EOF:  goto err; 
       case '\"':         /* skip beg. and end. '"' */
	 if(beg++) {str[cnt] = 0; return str;}
	 else break;
       case '\\':
	 str[cnt++] = c;
	 if(EOF == (c = get()) || (!isprint(c) && c != '\n')) goto err;
       default: str[cnt++] = c;
      }
      if(cnt >= MAXSTRING) {
	 error("string too long !\n");
	 goto err;
      }
   }
 err:
   error("line: %d - error reading string !\n",linenum);
   return 0;
   
}


/* changestring():
 * converts a C - ASCII string in a normal string
 */

static char *changestring(str)   
char *str;
{
   int number;
   char *retstr,*s;

   retstr = s = str;
   
   while(*str) {
      switch(*str){
       case '\\':
	 str++;
	 if(isdigit(*str)) {            /* octal number */
	    number = (*str++ - '0') & 7;
	    if(!isdigit(*str)){
	       *s++ = number; break;
	    }
	    number <<= 3; number |= (*str++ - '0') & 7;
	    if(!isdigit(*str)){
	       *s++ = number; break;
	    }
	    number <<= 3; number |= (*str++ - '0') & 7;
	    *s++ = number; break;
	 }
	 else {
	    switch(*str){
	     case 'a' :
	     case 'A' :
	     case 'o' :
	     case 'O' :
             case 'u' :
             case 'U' :  *s++ = 0xc8;  *s++ = *str;  break;
	     case 's' :  *s++ = 0xfb;  break;
	     case 'r' :  *s++ =  '\r'; break;
	     case 't' :  *s++ =  '\t'; break;
	     case 'n' :  *s++ =  '\n'; break;
/*	     case 'a' :  *s++ =  '\a'; break;  alert (bell) char */
	     case 'b' :  *s++ =  '\b'; break;
	     case 'f' :  *s++ =  '\f'; break;
	     case 'v' :  *s++ =  '\v'; break;
	     case '#' :  *s++ =  TER ; break;
	     case '*' :  *s++ =  INI ; break;		 
	     case '\'':  *s++ =  '\''; break;
	     case '\\':  *s++ =  '\\'; break;
	     case '\n':   	       break;
	     case '\0':    continue;   break;  /* break while loop */
	     default  :  *s++ = *str;
	    }
	    str++;
	 }
	 break;
       default:
	 *s++ = *str++;
      }
   }
   *s = 0;
   return retstr;
}


static char *getidentifier(str)
char *str;
{
   int c;
   char *retstr = str;;
   
   for(;;){
      switch((c = get())){
       case    ' ':  continue;
       case   '\t':  continue;
       case    EOF:  return 0;
       default:
	 while(isalnum(c) || c == '_') {
	    *str = c;
	    c = get();
	    str++;
	 }
	 *str = 0;
	 unget(c);
	 if(str == retstr){
	    error("syntax error line %d !\n",linenum);
	    return 0;
	 }
	 return retstr;
      }
   }  
}


/* makevar():
 * initializes an unnamed variable with the Value 'string'
 * 'string' is a C - string.
 * RETURN:   -1 on error;
 */

static makevar(string)
char *string;
{
   char *vari;

   if(varcnt >= MAXVAR) {
      error("line: %d - too many variables\n",linenum);
      return -1;
   }
   if(!(vari = malloc(strlen(string)+1))) {
      errorp("malloc\n");
      return -1;
   }
   strcpy(vari,string);
   changestring(vari);     /* this is a waste of some bytes - don't worry */
   var[varcnt].value = vari;
   var[varcnt].name = 0;
   varcnt++;
   return varcnt -1;
}


static setidentifier(str)
char *str;
{
   int i,ok;

   for(i=0,ok=0; i<varcnt; i++) {
      if(var[i].name && !strcmp(str,var[i].name)) { ok =1; break;}
   }
   if(ok) return i;
   else {
      if(!(var[varcnt].name = malloc (strlen(str)+1)))
         return errorp("malloc\n");
      strcpy(var[varcnt].name,str);
      var[varcnt].value = 0;
      if(++varcnt >= MAXVAR) {
	 error("line: %d - too many identifiers\n",linenum);
	 return -1;
      }
      return varcnt -1;
   }
}


/* initargs():
 * initalize arguments given to script (see initscript() )
 */

static initargs(num, string)
char *string;
int num;
{
   int varnum;
   char name[32];

   if(-1 == (varnum = makevar(string)))  return -1;
   sprintf(name, "argv_%d\000", num);
   if(!(var[varnum].name = malloc(strlen(name)+1)))  return errorp("malloc\n");
   strcpy(var[varnum].name, name);
   return 0;
}


static initargc(argc)
int argc;
{
   char *name= "argc";
   char argcstr[10]; int varnum;
   
   sprintf(argcstr, "%d\000", argc);
   if(-1 == (varnum = makevar(argcstr))) return -1;
   if(!(var[varnum].name = malloc(strlen(name)+1)))  return errorp("malloc\n");
   strcpy(var[varnum].name, name);
   return 0;
}

   
/* setcommand():
 * docodes one command from file and precompiles it for further use
 * 'str' contains the argument description and
 * 'function' is the pointer to the procedure later called
 */

static setcommand(function,str)
int (*function)(); char *str;
{
   if(pc >= maxprog) {
      if(maxprog == 0) {programm =0; pc =0;}
      maxprog += 1000;
      if( !(programm = (ADDRESS *)ralloc(programm, maxprog*sizeof(ADDRESS)))) 
	      return errorp("realloc\n");
   }
   programm[pc].data.function = function;
   programm[pc].type = INSTRUCTION;
   pc++;
	 
   while(*str){
      switch(*str){
       case '%':
	 ++str;
	 switch (*str){
	  case 'a': 
	    if(-1 == (programm[pc].data.variable = getarg())) return -1;
	    programm[pc++].type = VARIABLE;
	    break;
	 
	  case 'v':
	    if(-1 == (programm[pc].data.variable = getarg())) return -1;
	    programm[pc++].type = VARIABLE;
	    break;
	    
	  case 'l':
	    if(-1 == (programm[pc].data.label  = getlabel())) return -1;
	    programm[pc++].type = LABEL;
	    break;
	 }
	 break;

       case ' ': break;
	 
       default: if(-1 == skip(*str)) return -1; break;
      }
      str++;
   }
   return 0;
}


static skip(c)
char c;
{
   int cc;

   for(;;){
      cc = get();
      switch(cc){
       case ' ':  break;
       case '\t': break;
       default:
	 if(cc == c) return 0;
/*	 error("char:%x \'%c\' expected\n",c,c); */
	 return -1;
      }
   }
}


char *gethome(filename)
char *filename;
{
   char *home,*name;
   
   if(filename[0] != '~' || !(home = getenv("HOME"))) return 0;
   if(!(name = malloc(strlen(home)+strlen(filename)+1))){
      errorp("getenv HOME:");
      return 0;
   }
   strcpy(name, home);
   strcat(name, filename+1);
   return name;
}


static VAR *gc()
{
   VAR *c = &var[programm[pc].data.variable];
   if(programm[pc].type != VARIABLE){
      error("pc: %d - variable error !\n",pc);
      return 0;
   }
   pc++;
   return c;
}


int gl()
{
   int c = label[programm[pc].data.label].pc;
   if(programm[pc].type != LABEL || c == -1){
      error("pc: %d - label error !\n",pc);
      return -1;
   }
   pc++;
   return c;
}


static isinitialized(b)
VAR *b;
{

   if(!b->value){
      error("pc: %d - uninizalized variable %s!\n",pc,b->name);
      return 0;
   }
   return 1;
}
   

static specials(n, v)
   char *n;
   char *v;
{
   /* special variables */
   if( strcmp(n, "DISPLAY") == 0 ) {
      display = v[0] ? v : NULL;
   }
   else if( strcmp(n, "DEBUG") == 0 ) {
      debug = ( strcmp(v, "on") == 0 );
   }
}


/***************************** functions *********************************/

static s_set()
{
   VAR *a,*b;
   
   if(!(a=gc()) || !(b=gc())) return -1;
   if(a->value == b->value)
      return error("pc: %d - variables don't differ!\n",pc);
   dp((stderr,"set %s=\"%s\"\n",a->name,b->value));
   if(!isinitialized(b)) return -1;

   if(a->value) free(a->value);
   if(!(a->value = malloc(strlen(b->value)+1)))  return errorp(" ");
   strcpy(a->value,b->value);

   specials(a->name, a->value);

   return 0;
}


static s_write()
{
   VAR *a, *b;
   int fd=-1,len;
   char *filename,*name;
   
   if(!(a=gc()) || !(b=gc())) return -1;

   dp((stderr,"write \"%s\",\"%s\"\n",a->value,b->value));
   if(!isinitialized(b) || !isinitialized(a)) return -1;

      
   name = gethome(b->value);
   filename = name ? name : b->value;
   len = strlen(a->value); 
   if(-1 == (fd = open(filename,O_WRONLY | O_CREAT | O_TRUNC,0666)) ||
      len  != write(fd,a->value,len)){
      errorp("write %s -",filename);
      if(fd != -1) close(fd);
      if(name) free(name);
      return -1;
   }
   if(name) free(name);
   close(fd);
   
   return 0;
}


static s_append()
{
   VAR *a, *b;
   int fd = -1,len;
   char *filename,*name;
   
   if(!(a=gc()) || !(b=gc())) return -1;
   dp((stderr,"append \"%s\",\"%s\"\n",a->value,b->value));
   len = strlen(a->value);
   if(!isinitialized(b) || !isinitialized(a)) return -1;

   name = gethome(b->value);
   filename = name ? name : b->value;
   
   if(-1 == (fd = open(filename,O_APPEND | O_WRONLY | O_CREAT,0666)) ||
      -1  == write(fd,a->value,len)) {
      error("append %s - ",filename);
      if(fd != -1) close(fd);
      if(name) free(name);
      return -1;
   }
   if(name) free(name);
   close(fd);
   
   return 0;
}
   

static s_read()
{
   VAR *a, *b;
   int fd = -1;
   struct stat sstat;
   char *name, *filename;
   
   if(!(a=gc()) || !(b=gc())) return -1;
   dp((stderr,"read %s,\"%s\"\n",a->name,b->value));
   if(!isinitialized(b)) return -1;
   
   name = gethome(b->value);
   filename = name ? name : b->value;
   if(-1 == (fd = open(filename,O_RDONLY)) || -1 == fstat(fd,&sstat))
      goto err;
   if(a->value) free(a);
   if(!(a->value = malloc(sstat.st_size + 1))) goto err;
   if(-1 == read(fd,a->value,sstat.st_size)) goto err;
   a->value[sstat.st_size]=0;
   close(fd);
   if(name) free(name);
   
   return 0;
   
 err:
   if(fd != -1) close(fd);
   if(name) free(name);
   return errorp("%s\n",filename);
}     


static s_strcat()
{
   VAR *a,*b;
   int len;
   

   if(!(a=gc()) || !(b=gc())) return -1;

   dp((stderr,"strcat \"%s\",\"%s\"\n",a->value,b->value));

   if(!isinitialized(b) || !isinitialized(a)) return -1;
   
   if(a->value == b->value)
      return error("pc: %d - variables don't differ!\n",pc);
   len = strlen(a->value) + strlen(b->value) + 1;
   free(a->value);
   if(!(a->value = ralloc(a->value,len))) return errorp("realloc\n");
   strcat(a->value,b->value);
   
   return 0;
}


static s_send()
{
   VAR *a;
   extern int server_outfd;
   
   if(!(a=gc())) return -1;

   dp((stderr,"send \"%s\"\n",a->value));

   if(!isinitialized(a) || server_outfd == -1) return -1;
   if(-1 == write(server_outfd,a->value,strlen(a->value)))
      return errorp("send\n");
   return 0;
}


static s_get()  /* coordinates now 1 based !! */
{
   VAR *a, *b, *c, *d, *e;
   int x1, y1, x2, y2, y, x, mem, n;
   extern int rows;
   char *endptr;
   
   if(!(a=gc()) || !(b=gc()) || !(c=gc()) || !(d=gc()) || !(e=gc()))
      return -1;
   dp((stderr,"get %s=\"%s\",\"%s\",\"%s\",\"%s\"\n",
       a->name,b->value,c->value,d->value,e->value));

   if(!isinitialized(b) || !isinitialized(c) || !isinitialized(d) ||
      !isinitialized(e))
      return -1;
   
   if(a->value) free(a->value);
   x1 = strtol(b->value, &endptr, 0) - 1;
   if(*endptr) return error("get: numeric argument expected!\n");
   y1 = strtol(c->value, &endptr, 0) - 1;
   if(*endptr) return error("get: numeric argument expected!\n");
   x2 = strtol(d->value, &endptr, 0) - 1;
   if(*endptr) return error("get: numeric argument expected!\n");
   y2 = strtol(e->value, &endptr, 0) - 1;
   if(*endptr) return error("get: numeric argument expected!\n");
   mem  = x2 - x1 + 3;  /* x diff + 2 reserved per line */
   mem *= y2 - y1 + 1;  /* y diff */
   if(y1 > y2 || x1 > x2 || x1 < 0 || x2 > 39 || y1 < 0 || y2 > rows-1)
      return error("illegal values for get\n");
   if(!(a->value = malloc(mem))) return errorp("malloc\n");

   for(n=0, y=y1; y<=y2; y++) {
      for(x=x1; x<=x2; x++)  a->value[n++] = map_iso_char(x, y);
      if(y2>y1) a->value[n++] = '\n';
   }
   a->value[n++]=0;
   return 0;
}


static s_waitdct()
{
   dp((stderr,"waitdct\n"));
   wait_for_dct = 1;
   return 0;
}


static s_if()
{
   VAR *a, *b;
   int c;

   if(!(a=gc()) || !(b=gc()) || -1 == (c = gl())) return -1;
   dp((stderr,"if \"%s\"==\"%s\" goto %d\n",a->value,b->value,c));
   if(!isinitialized(b) || !isinitialized(a)) return -1;
   
   if(!strcmp(a->value,b->value)) pc = c;
   
   return 0;
}


static info_or_debug( fp )
FILE *fp;
{
   VAR *a;
   
   if(!(a=gc())) return -1;
   dp((stderr,"info \"%s\"\n",a->value));
   if(!isinitialized(a)) return -1;
   /* xbtxinfo(a->value); */
   
   fprintf(fp,"%s",a->value);
   
   return 0;
}


static s_info()
{
    return info_or_debug(stdout);
}


static s_debug()
{
    return info_or_debug(stderr);
}


static s_goto()
{
   int c;
   
   if((c=gl()) == -1) return -1;
   dp((stderr,"goto %d\n",c));
   
   pc = c;
   
   return 0;
}


static s_inc()
{
   VAR *a;
   char *endptr;
   long x;
   
   if(!(a=gc())) return -1;
   dp((stderr,"inc \"%s\"\n",a->value));
   if(!isinitialized(a)) return -1;
   
   if(!(a->value = ralloc(a->value,16))) return errorp("realloc\n");
   x = strtol(a->value, &endptr, 0);
   if(*endptr) return error("inc: numeric argument expected!\n");
   sprintf(a->value,"%ld\000", x+1);
   
   return 0;
}


static s_dec()
{
   VAR *a;
   long x;
   char *endptr;
   
   if(!(a=gc())) return -1;
   dp((stderr,"dec \"%s\"\n",a->value));
   if(!isinitialized(a)) return -1;
   
   if(!(a->value = ralloc(a->value,16)))  return errorp("realloc\n");
   x = strtol(a->value, &endptr, 0);
   if(*endptr) return error("dec: numeric argument expected!\n");
   sprintf(a->value, "%ld\000", x-1);
   
   return 0;
}


static s_setpart()
{
   VAR *a,*b,*c,*d;
   int from,to,len,i;
   char *endptr;
   
   if(!(a=gc()) || !(b=gc()) || !(c=gc()) || !(d=gc())) return -1;
   dp((stderr,"setpart %s=\"%s\",\"%s\",\"%s\"\n",
       a->name,b->value,c->value,d->value));
   if(a->value == b->value)
      return error("pc: %d - variables don't differ!\n",pc);
   
   if(!isinitialized(b) || !isinitialized(c) || !isinitialized(d))  return -1;
   
   if(a->value) free(a->value);
   from = strtol(c->value, &endptr, 0);
   if(*endptr) return error("setpart: numeric argument expected!\n");
   to =   strtol(d->value, &endptr, 0);
   if(*endptr) return error("setpart: numeric argument expected!\n");
   len = strlen(b->value);
   if(from < 0 || from >= len || to < from || to < 0 || to >= len)
      return error("illegal values for setpart\n");
   if(!(a->value = malloc(len))) errorp("malloc\n");
   for(i=0; from<=to; i++){
      a->value[i] = b->value[from++];
   }
   a->value[i]=0;
   
   return 0;
}


static s_system()
{
   VAR *a;
   
   if(!(a=gc())) return -1;
   if(!isinitialized(a)) return -1;
   dp((stderr,"system \"%s\"\n",a->value));
   
   if(system(a->value)) return error("system \'%s\' failed !\n",a->value);
   
   return 0;
}


static s_sleep()
{
   VAR *a;
   int x;
   char *endptr;
   
   if(!(a=gc())) return -1;
   if(!isinitialized(a)) return -1;
   dp((stderr,"sleep \"%s\"\n",a->value));

   x = strtol(a->value, &endptr, 0);
   if(*endptr) return error("sleep: numeric argument expected!\n");
   sleep(x);

   return 0;
}


static s_onx()
{
   dp((stderr,"onx\n"));
   open_X(display);

   return 0;
}


static s_offx()
{
   dp((stderr,"offx\n"));
   close_X();

   return 0;
}


/*
 * if the preset string is the string constant "*NO ECHO*", it is considered
 *  to be empty and the user input shouldn't be echoed !!
 * if the preset string is "*STDIN*" the input is read from stdin no matter
 *  if the X display is on or not
 */
static s_input()
{
   extern int visible;
   VAR *a, *b, *c;
   char tmpstr[300], *ok_str = "Ok", *ptr, *getpass();
   int echo;
   
   if( !(a=gc()) || !(b=gc()) || !(c=gc()) ) return -1;
   dp((stderr,"input %s:\"%s\",\"%s\"\n", a->name, b->value, c->value));
   if(!isinitialized(b) || !isinitialized(c)) return -1;

   tmpstr[0] = 0;	/* prepare for eof (gets transfers no chars!) */
   echo      = strcmp(c->value, "*NO ECHO*");

   if( strcmp(c->value, "*STDIN*") == 0 ) {
	 if( b->value[0] ) {
	    printf( "%s", b->value );
	    fflush(stdout);
	 }
	 gets(tmpstr); 
   }
   else if(visible) {
      if(echo) {
	 strncpy(tmpstr, c->value, sizeof(tmpstr)-1);
      }
   
      popup("XCEPT: Script Input", 0, b->value, &ok_str, 1, tmpstr, 300, echo);
   }
   else {   /* normal input, no xbtx window */
      printf("XCEPT INPUT: %s  ", b->value);
      fflush(stdout);

      if( !echo && isatty(fileno(stdout)) ) {
	 ptr = getpass("?  ");
	 strcpy(tmpstr, ptr);
      }
      else {
	 if(c->value[0] != '\0') {
	    printf("   (Type [Return] to accept default '%s')\n", c->value);
	 }
	 printf("?  ");
	 fflush(stdout);
	 gets(tmpstr); 
	 if( !strlen(tmpstr) ) strncpy( tmpstr, c->value, sizeof(tmpstr)-1 );
      }
   }
   
   changestring(tmpstr);
   
   if(a->value) free(a->value);
   if(!(a->value = malloc(strlen(tmpstr)+1)))  return errorp(" ");
   strcpy(a->value, tmpstr);

   specials(a->name, a->value);

   return 0;
}   
   

static s_connect()
{
   set_connect();

   return 0;
}


static s_disconnect()
{
   set_disconnect();

   return 0;
}


static s_ppmsave()
{
   VAR *a;

   if(!(a=gc())) return -1;
   if(!isinitialized(a)) return -1;
   dp((stderr,"ppmsave \"%s\"\n",a->value));

   save_PPM(a->value);
   
   return 0;
}


static s_playback()
{
   extern char playbackfilename[];
   VAR *a;

   if(!(a=gc())) return -1;
   if(!isinitialized(a)) return -1;
   dp((stderr,"playback \"%s\"\n", a->value));

   strcpy(playbackfilename, a->value);
   set_playback();
   
   return 0;
}


static s_getstate()
{
   VAR *a;
   extern int connected, playback, connection_status;
   char tmpstr[100];
   
   if( !(a=gc()) ) return -1;
   dp((stderr,"getstate %s\n", a->name));

   if(connected &&
      (connection_status==CONNECT || connection_status==ISDNCONNECT) )
			   strcpy(tmpstr, "ONLINE");
   else if(connected)      strcpy(tmpstr, "CONNECTED");
   else if(playback)       strcpy(tmpstr, "PLAYBACK");
   else                    strcpy(tmpstr, "DISCONNECTED");
   
   if(a->value) free(a->value);
   if(!(a->value = malloc(strlen(tmpstr)+1)))  return errorp(" ");

   strcpy(a->value, tmpstr);

   return 0;
}   


static s_quit()
{
   quit(0);  /* exit without confirmation */
}


/*************************************************************************/

static skiplc()
{
   int c;
   
   for(;;){
      
      switch((c =get())){
       case ' ' : continue;
       case '\n': linenum++; continue;
       case '\t': continue;
       default  : break;
      }
      
      switch(c){
       case '#':
	 while((c =get()) != '\n' && c != EOF);
	 if(c == EOF) return EOF;
	 linenum++;
	 break;
       case EOF: return EOF;
       case '\n': linenum++; break;
       default: unget(c); return 0;
      }
      
   }
}
	 

static initnext()
{

   char str[MAXSTRING];
   int i;
   
   if(EOF == skiplc()) return EOF;
   if(!getidentifier(str)) return 0;
   dp((stderr,"instr: %-10s ",str));
   
   for(i=0; instructions[i].name; i++){
      if(!strcmp(instructions[i].name,str)){
	 if(-1 == setcommand(instructions[i].function,instructions[i].args)){
	    error("line: %d - error in syntax !\n",linenum);
	    return 0;
	 }
	 dp((stderr,"\n"));
	 return 1;
      }
   }
   if(-1 == skip(':')){
      error("line: %d - missing for \':\' for label !\n",linenum);
      return 0;
   }
   setlabel(str,pc);
   dp((stderr,"\n"));
   
   return 1;
}


static errorp(fmt,a,b,c,d,e,f)
char *fmt;
void *a,*b,*c,*d,*e,*f;
{
/*
   error(fmt,a,b,c,d,e,f);
   perror("errno: ");
*/
   xbtxerror(1, fmt, a, b, c, d, e, f);
   return -1;
}


static error(fmt,a,b,c,d,e,f)
char *fmt;
void *a,*b,*c,*d,*e,*f;
{
/*
   fprintf(stderr,"xbtx (script) - ");
   fprintf(stderr,fmt,a,b,c,d,e,f);
*/
   xbtxerror(0, fmt, a, b, c, d, e, f);
   return -1;
}


static void *ralloc(ptr, size)
char *ptr;
int size;
{
   if(ptr)  return realloc(ptr, size);
   else     return malloc(size);
}
