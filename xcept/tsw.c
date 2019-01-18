/*-----------------------------------------------------------------------------
 *
 *    tsw.c	Modul fuer das Empfangen von TSW-TRANSMISSION	
 *							annex 7.4, page 4
 *
 *    implementiert von Dieter Kasper
 *
------------------------------------------------------------------------------
S I E M E N S   Dieter Kasper           Phone: (..89) 636 - 40593 <09:00-18:00>
- ------------- SNI HE SW 441           Phone: (..89) 6929597     <18:00-22:00>
N I X D O R F   81739 Munich            email: Dieter.Kasper@mch.sni.de
------------------------------------------------------------------------------
*/


#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include "control.h"


/*  globals
 *---------------------------------------------------------------------------*/
unsigned char	tsw_mode;
int	dz_tsw;		/* Datenzeiger  TSW-Datei	*/
int	ll_tsw;		/* Laenge der   TSW-Datei	*/
int	ii_tsw;		/* Index in der TSW-Datei	*/
char	*cp_tsw, *calloc();	/* Zeiger auf   TSW-Daten	*/
char	msg[BUFSIZ];	

/*  functions
 *---------------------------------------------------------------------------*/
int	l2getc();
int	get_sq();
int	get_LI();
int	do_PI();


#define	DDU_D_SET	0x27
#define	DDU_D_END	0x33

#define	TDU_T_ASSOCIATE	0x23
#define	TDU_T_FILESPEC	0x63
#define	TDU_T_WR_START	0x43
#define	TDU_T_WRITE	0x45
#define	TDU_T_WR_END	0x47

#define	MODE1_3IN4	0x42
#define	MODE2_8BIT	0x41

#define	PI_CHECK	0x22
#define	PI_STREAM	0x31
#define	PI_APPL_NAME	0x45
#define	PI_TRANS_ID	0x4F
#define	PI_FILE_NAME	0x65
#define	PI_FILE_LENG	0x67
#define	PI_DATE_TIME	0x7F


/*
 *-------------------------------------------------------------- 05.10.93 ---*/
void
do_TSW()
{
  int		DDU, TDU, sq, LI, PV, i, rc;
  int us; /* boolean_t */

  while( (DDU = layer2getc()) != US ) {
	switch( DDU ) {				/* Dialog Data Unit */
		case DDU_D_SET:
			log("DDU_D_SET\n");
			sq = get_sq(     tsw_mode );
			LI = get_LI( sq, tsw_mode );

			for( i=0; i<LI; ) {
				if( (rc=do_PI( sq, tsw_mode )) == -1 )
					goto end_do_TSW;
				i += rc;
			}
			break;
		case DDU_D_END:
			log("DDU_D_END\n");
			sq = get_sq(     tsw_mode );

			if( layer2getc() == US )	goto end_do_TSW;
			log("PV:               0x%02x\n", sq );

			tsw_mode = 0x00;
			break;
		default:
			sq = DDU;
			log("DDU_D_DATA, sq:   0x%02x\n", sq );
			while( (TDU = l2getc(tsw_mode,&us)) != US &&
			      us == 0 ) {
				switch( TDU ) {	/* Telesoftware Data Unit */
					case TDU_T_ASSOCIATE:
						log("CI TDU_T_ASSOCIATE\n");
						break;
					case TDU_T_FILESPEC:
						log("CI TDU_T_FILESPEC\n");
						break;
					case TDU_T_WR_START:
						log("CI TDU_T_WR_START\n");
						break;
					case TDU_T_WRITE:
						log("CI TDU_T_WRITE\n");
						break;
					case TDU_T_WR_END:
						log("CI TDU_T_WR_END\n");
						if( write( dz_tsw, cp_tsw, ll_tsw ) != ll_tsw ) {
							sprintf( msg, "write(,,%d)", ll_tsw );
							perror( msg );
						}
						free(  cp_tsw );
						close( dz_tsw );
						break;
					default:
						log("unknown TDU       0x%02x\n", TDU );
						break;
				}
				LI = get_LI( sq, tsw_mode );

				for( i=0; i<LI; ) {
					if( (rc=do_PI( sq, tsw_mode )) == -1 )
						goto end_do_TSW;
					i += rc;
				}
				if( TDU == TDU_T_WR_START
				 || TDU == TDU_T_WRITE    ) {	/* DATEN */
					do {
						PV = l2getc( tsw_mode, &us );
						if( us  ) {
							log("\n" );
							goto end_do_TSW;
						}
						log("PV DATA\n");
						*(cp_tsw+ii_tsw) = PV;
					} while( ++ii_tsw < ll_tsw );
				}
			}
			goto end_do_TSW;
	}
  }
end_do_TSW:
  log("\n");
  layer2ungetc();
}

/*
 *-------------------------------------------------------------- 05.10.93 ---*/
int
l2getc(mode, us)
int mode;
int *us;
{
  static unsigned char	nr, b1, b2, b3, b4;
  unsigned char	rc;

  *us = 0;

  if( mode == MODE1_3IN4 ) {
	switch( nr ) {
		case 0:					/*--- Byte 1+2 ---*/
			b1 = layer2getc();	log("\n");
			if( b1 == US ) {
				nr = 0;	rc = US; *us = 1;
				break;
			}
			b2 = layer2getc();
			if( b2 == US ) {
				nr = 0;	rc = US; *us = 1;
			}
			else {
				nr = 2;	rc = ( (0x30 & b1)<<2 )|( 0x3F & b2 );
			}
			break;
		case 2:					/*--- Byte 3 ---*/
			b3 = layer2getc();
			if( b3 == US ) {
				nr = 0;	rc = US; *us = 1;
			}
			else {
				nr = 3;	rc = ( (0x0C & b1)<<4 )|( 0x3F & b3 );
			}
			break;
		case 3:					/*--- Byte 4 ---*/
			b4 = layer2getc();
			if( b4 == US ) {
				nr = 0;	rc = US; *us = 1;
			}
			else {
				nr = 0;	rc = ( (0x03 & b1)<<6 )|( 0x3F & b4 );
			}
			break;
	}
  }
  else
	rc = layer2getc();

  log("l2getc(0x%02X)  ", rc );
  if( isprint(rc) )	log("'%c'  ", rc );
  else			log("     ");

  return( rc );
}
/*
 *-------------------------------------------------------------- 05.10.93 ---*/
int
get_sq(mode)
int mode;
{
  int 	us;
  int		sq;

  sq = l2getc( mode, &us );
  log("sequence code:    0x%02x\n", sq );

  return( sq );
}

/*
 *--------------------------------------------------------------- 05.10.93 ---*/
int
get_LI( sq, mode )
int sq, mode;
{
  int 	us;
  int		LI;

  LI = l2getc( mode, &us );
  LI = LI % sq;
  log("LI=%3d\n", LI );

  return( LI );
}

/*
 *-------------------------------------------------------------- 05.10.93 ---*/
int
do_PI( sq, mode )
int sq, mode;
{
  int	us;
  unsigned char	u_PV;
  int		PI, LI, PV, rc=-1, i;
  char		name[BUFSIZ], date[BUFSIZ];

  if( (PI=l2getc(mode,&us)) == US || us == 1 )
	goto end_do_PI;

  switch( PI ) {
	case PI_STREAM:	log("Str\n");	rc = 1;	break;
	case PI_CHECK:	log("PI Checksum use & mode\n");	
			LI = get_LI( sq, tsw_mode );
			for( i=0; i<LI; i++ ) {
				PV = l2getc( mode, &us );
				log("PV\n");
			}
			tsw_mode = PV;
			rc = LI+2;	break;
	case PI_APPL_NAME:	log("PI Application-name\n");
			LI = get_LI( sq, tsw_mode );
			for( i=0; i<LI; i++ ) {
				PV = l2getc( mode, &us );
				log("PV\n");
			}
			rc = LI+2;	break;
	case PI_FILE_NAME:	log("PI Filename\n");
			LI = get_LI( sq, tsw_mode );
			for( i=0; i<LI; i++ ) {
				PV = l2getc( mode, &us );
				log("PV\n");
				name[i] = PV;
			}
			name[LI] = 0x00;
			if( (dz_tsw=open( name, O_RDWR|O_CREAT, 0600)) == -1) {
				sprintf( msg, "open(%s,rw)", name );
				perror( msg );
			}
			rc = LI+2;	break;
	case PI_FILE_LENG:	log("PI File-length\n");
			LI = get_LI( sq, tsw_mode );
			ll_tsw = 0;
			for( i=0; i<LI; i++ ) {
				u_PV = l2getc( mode, &us );
				log("PV\n");
				PV = u_PV;
				ll_tsw += ( PV<<((LI-i-1)*8) );
			}
			if( (cp_tsw = calloc( 1, ll_tsw )) == NULL ) {
				sprintf( msg, "calloc(1,%d)", ll_tsw );
				perror( msg );
			}
			ii_tsw = 0;
			rc = LI+2;	break;
	case PI_DATE_TIME:	log("PI Date/time\n");
			LI = get_LI( sq, tsw_mode );
			for( i=0; i<LI; i++ ) {
				PV = l2getc( mode, &us );
				log("PV\n");
				date[i] = PV;
			}
			date[LI] = 0x00;
			rc = LI+2;	break;
	case PI_TRANS_ID:	log("PI Transfer Identifier\n");
			LI = get_LI( sq, tsw_mode );
			for( i=0; i<LI; i++ ) {
				PV = l2getc( mode, &us );
				log("PV\n");
			}
			rc = LI+2;	break;
	default:	log("PI unknown\n");
			LI = get_LI( sq, tsw_mode );
			for( i=0; i<LI; i++ ) {
				PV = l2getc( mode, &us );
				log("PV\n");
			}
			rc = LI+2;	break;
  }

end_do_PI:
  return( rc );
}
