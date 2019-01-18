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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include "scripts.h"


struct script scripts[ MAXSCRIPTS+1 ];
int num_scripts = 0;


static void *Lalloc( size )
    size_t size;
{
    void *p;

    if( (p = malloc( size )) == NULL ) {
	perror( NULL );
	exit(1);
    }
    return p;
}

static check_magic( f )
    char *f;
{
    FILE *fp;
    int stat;
    char magic[ MAGIC_STRLEN ];

    stat = (fp = fopen( f, "r" )) != NULL
           &&  fread( magic, sizeof(char), MAGIC_STRLEN, fp) == MAGIC_STRLEN
    	   &&  strncmp( MAGIC_STRING, magic, MAGIC_STRLEN) == 0;
    if( fp != NULL ) fclose(fp);
    return stat;
}


static int expand_path( path )
    char *path;
{
    int path_len;
    int base_len;
    int len;
    char dirbuf[PATH_BUFLEN];
    char *dir;
    char *pathp;
    struct script *s;
    DIR *dp;
    struct dirent *ent;

    if( path == NULL )
	return 0;

    for( pathp = path; *pathp != '\0'; )
    {
	dir = pathp;
	while( *pathp != ':' && *pathp != '\0' )
	    pathp++;

	if( pathp > dir )
	{
	    dirbuf[0] = '\0';
	    strncat( dirbuf, dir, (pathp - dir) );
	    if( (dp = opendir( dirbuf )) == NULL )
		return -1;				/* skip silently */

	    for( s = &scripts[ num_scripts ]; (ent = readdir(dp)) != NULL; )
	    {
	        len = strlen(ent->d_name); /* <- use this (SYSV has no
					      d_namelen) */
		path_len = (pathp - dir) + 1 + len + 1;
		base_len = len - XBTXSCRIPTS_SUFFIXLENGTH;

		s->basename = Lalloc( len+1 );
		strcpy( s->basename, ent->d_name );

		s->path = Lalloc( path_len );
		strcpy( s->path, dirbuf );
		strcat( s->path, "/" );
		strcat( s->path, ent->d_name );

		if( base_len > 0
		    &&  strcmp(&s->basename[base_len], XBTXSCRIPTS_SUFFIX) == 0
		    &&  (s->basename[base_len] = 0) == 0
			/* line above wanted for side effect only (always true!) */
		||  check_magic( s->path ) )
		{
		    if( num_scripts >= MAXSCRIPTS ) {
			perror( "Too many script files ... ignoring some" );
		    } else {
			s++;
			num_scripts++;
		    }
		} else {
		    free( s->path );
		    free( s->basename );
		}
	    }

	    closedir(dp);
	}

	if( *pathp == ':' ) {
	    while( *++pathp == ':' );
	}
    }
    return 0;
}


void collect_xbtxscripts( more_pathes )
    char *more_pathes;
{
    char *envvar;

    if( (envvar = getenv( XBTXSCRIPTS_ENVVAR )) == NULL
    &&  more_pathes == NULL ) {
	expand_path( XCEPTSCRIPTS_DEFAULTPATH );
    }
    else {
	expand_path( more_pathes );
	expand_path( envvar );
    }
}


char *lookup_script( basename )
    char *basename;
{
    int i;

    for( i=0; i<num_scripts; i++ ) {
	if( strcmp( basename, scripts[i].basename ) == 0 ) {
	    return scripts[i].path;
	}
    }
    return NULL;
}


#ifdef ALONE

int main( argc, argv )
    int argc;
    char *argv[];
{
    int i;

    collect_xbtxscripts( argc >= 1 ? argv[1] : NULL );

    printf( "# scripts: %d\n", num_scripts );
    for( i=0; i<num_scripts; i++ )
	printf( "%20s : %s\n", scripts[i].basename, scripts[i].path );

    return 0;
}

#endif
