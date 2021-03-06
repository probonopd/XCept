# directories:
OWNER= root
GROUP= sys
EXECMODE= 6750

BINDIR= /usr/local/bin# 		directory for the executable ceptd
MANDIR= /usr/local/man/manl# 	directory for the ceptds manpage 
ETCDIR= /usr/local/etc# 	directory for 'init file' and 'users file'

# defaults for the ceptd :

DEFAULTINITFILENAME= 	$(ETCDIR)/init.cept# 	modems default init file
DEFAULTUSERSFILENAME= 	$(ETCDIR)/users.cept# 	file with allowed users
DEFAULTLOGFILENAME= 	/tmp/log.cept#		logfile for user online times
DEFAULTMODEM= 		/dev/modem#		char. special file for modem
DEFAULTSPEED= 		2400# 			modem baud rate
DEFAULTSOCKETPORT= 	20005#			socket port for connections
LOCKDIR=		/var/spool/locks#       lock dir for uucp-locks

# define these flags in $(DEFS):

# if your system has ISDN:
# -DISDN
# if using uucp LOCK files:
# -DUSE_LOCK=\"$(LOCKDIR)\"		
# allways define this for the modem to initalize right:
# -DDEFAULTINITFILENAME=\"$(DEFAULTINITFILENAME)\"
# if you want user online times to be logged in a file
# -DDEFAULTLOGFILENAME=\"$(DEFAULTLOGFILENAME)\"
# if you want to check users access rights:
# -DDEFAULTUSERSFILENAME=\"$(DEFAULTUSERSFILENAME)\" 
# allways define this:
# -DDEFAULTMODEM=\"$(DEFAULTMODEM)\"
# allways define this:
# -DDEFAULTSPEED=$(DEFAULTSPEED)
# allways define this:
# -DDEFAULTSOCKETPORT=$(DEFAULTSOCKETPORT)
# try: (if one of these don't work, see config.h)
# -DSUN
# For sun-os (Not Solaris)
# -DSVR4
# For System V (Arghh..)
# -DLINUX
# For Linux (Loosers Unix)
# -DSCO
# For SCO-Unix
# -DBSD
# For BSD Systems

DEFS=	-DDEFAULTSOCKETPORT=$(DEFAULTSOCKETPORT)\
	-DUSE_LOCK=\"$(LOCKDIR)\"\
	-DDEFAULTMODEM=\"$(DEFAULTMODEM)\"\
	-DDEFAULTSPEED=$(DEFAULTSPEED)\
 	-DDEFAULTINITFILENAME=\"$(DEFAULTINITFILENAME)\"\
	-DDEFAULTUSERSFILENAME=\"$(DEFAULTUSERSFILENAME)\"\
	-DDEFAULTLOGFILENAME=\"$(DEFAULTLOGFILENAME)\"\
	-DSVR4
#	-DISDN\

# the GNU C-compiler:
CC	= gcc
CCOPTS	= -O
CCFLAGS = -Wswitch -Wcomment  -Wshadow -Wpointer-arith -Wcast-qual\
          -Wtrigraphs# -Wunused

# ... the lex is ...
LEX	= flex

##############################################################################

all:	ceptd ceptd.8

OBJS = ceptd.o level2.o modeminit.o socket.o term.o lex.yy.o isdn.o
SRC  = ceptd.c level2.c modeminit.c socket.c term.c lex.yy.c isdn.c
INC  = config.h cept.h ../xcept/protocol.h

.c.o:		makefile config.h cept.h  ../xcept/protocol.h 
		@echo
		@echo '########################## $@ ########################'
		$(CC) -c $(CCOPTS) $(CCFLAGS) $(DEFS) $*.c

lex.yy.c:       lex_yy
		@echo
		@echo '########################## $@ ########################'
		$(LEX) -s -i -p lex_yy
		

ceptd:	$(OBJS)
		@echo
		@echo '########################## $@ ########################'
		$(CC) $(OBJS) -L. -o ceptd -lnsl -lsocket
		@echo "$@ finished"

install:	ceptd ceptd.8
	@echo
	@echo '########################## $@ #################################'
	strip ceptd
	mv ceptd $(BINDIR)
	chown $(OWNER) $(BINDIR)/ceptd
	chgrp $(GROUP) $(BINDIR)/ceptd
	chmod $(EXECMODE) $(BINDIR)/ceptd
	mv ceptd.8 $(MANDIR)
	chown $(OWNER) $(MANDIR)/ceptd.8
	chgrp $(GROUP) $(MANDIR)/ceptd.8
	chmod 644 $(MANDIR)/ceptd.8
	@echo
	@echo '**************************************************************'
	@echo '**************************************************************'
	@echo "------>>> Now do the following steps:"
	@echo 
	@echo '1)' add the following line to your /etc/inetd.conf file:
	@echo "   cept stream tcp nowait $(OWNER)"\
		$(BINDIR)/ceptd ceptd 
	@echo 
	@echo '2)' add the following line to your /etc/services file:
	@echo "   cept    $(DEFAULTSOCKETPORT)/tcp    ceptd"
	@echo
	@echo '3)' copy the ../etc/users.cept file to $(DEFAULTUSERSFILENAME)\
		  and edit it.
	@echo
	@echo '4)' copy the ../etc/init.cept file to $(DEFAULTINITFILENAME)\
		  and edit it.
	@echo
	@echo '5)' read the ceptd manual.
	@echo '***************************************************************'
	@echo '***************************************************************'

ceptd.8:	ceptd.8.template
		sed -e 's|--- The permissions file.|$(DEFAULTUSERSFILENAME)|g'\
		   -e 's|--- The initalization file.|$(DEFAULTINITFILENAME)|g'\
		   -e 's|--- The log file.|$(DEFAULTLOGFILENAME)|g'\
		   ceptd.8.template > ceptd.8


clean:		
		rm -f ceptd.8 *.o TAGS ceptd

veryclean:      
		rm -f ceptd.8 *.o TAGS ceptd lex.yy.c *.zoo

TAGS:		$(SRC) $(INC)
		etags -e *.[ch]



