#
# Copyright (c) 1992, 1993 Arno Augustin, Frank Hoering, University of
# Erlangen-Nuremberg, Germany.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the University of
#	Erlangen-Nuremberg, Germany.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
# EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#

# Makefile for xcept client.

#define ceptserver's hostname (Only used when not overridden by
#   environment variable XCEPTHOST or by command line optin '-h').
SERVERHOST = localhost


#define ceptserver's socketport (Only used when not overridden by
#   command line option '-p').
SERVERPORT = 20005


#define directory pathname where the scripts are located which
#   should be displayed in the scripts menu. (Only used when not
#   overridden by enironment variable XCEPTSCRIPTS or by command line
#   option '-S').
SCRIPTSDIR = /usr/local/btx


#define compiler and X11 include+library pathes 
CC	= gcc
XINC	= -I/usr/X386/include
XLIB	= -L/usr/X386/lib
#WARN	= -W -Wunused -Wswitch -Wcomment -Wshadow -Wpointer-arith -Wcast-qual


#define location of the xcept binary and online manual (for 'make install').
OWNER	= bin
GROUP	= uucp
MODE	= 755
BINDIR	= /usr/local/bin
MANDIR	= /usr/local/man/manl




######### definitions below should not need to be modified ###############

DEFS	= -DDEFAULTCEPTHOSTNAME=\"$(SERVERHOST)\"      \
	  -DDEFAULTSOCKETPORT=$(SERVERPORT)           \
	  -DXCEPTSCRIPTS_DEFAULTPATH=\"$(SCRIPTSDIR)\"		
CFLAGS	= $(XINC) $(WARN) $(DEFS)


SRCS = 	xcept.c xfont.c rawfont.c xwin.c layer6.c socket.c buttons.c \
	script.c scripts.c tsw.c

OBJS =	xcept.o xfont.o rawfont.o xwin.o layer6.o socket.o buttons.o \
	script.o scripts.o tsw.o



all:		xcept

xcept:		$(OBJS)
		@echo 'linking $@'
		@$(CC) $(OBJS) -o xcept $(XLIB) -lX11 -lnsl -lsocket /usr/ucblib/libucb.a

$(OBJS):
		@echo 'compiling $*.c'
		@$(CC) $(CFLAGS) -c $*.c


tags:		$(SRCS)
		etags -e $(SRCS)

install:	xcept
		strip xcept
		mv xcept $(BINDIR)
		-chown $(OWNER) $(BINDIR)/xcept
		-chgrp $(GROUP) $(BINDIR)/xcept
		chmod $(MODE)   $(BINDIR)/xcept
		cp xcept.1 $(MANDIR)
		-chown $(OWNER) $(MANDIR)/xcept.1
		-chgrp $(GROUP) $(MANDIR)/xcept.1
		chmod 644       $(MANDIR)/xcept.1


clean:
		@rm -f $(OBJS) xcept



# DO NOT DELETE THIS LINE -- make depend depends on it.

buttons.o: buttons.h
layer6.o: control.h font.h attrib.h protocol.h
script.o: attrib.h font.h control.h
scripts.o: scripts.h
xcept.o: font.h attrib.h control.h buttons.h buttondefs.h scripts.h protocol.h
xfont.o: font.h attrib.h
xwin.o: font.h bitmaps.h buttons.h buttondefs.h scripts.h
