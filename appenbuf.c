/* Mila -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * Copyright (C) 2016 Salvatore Sanfilippo <antirez at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "kilo.h"



void abAppend( struct abuf *ab, const char *s, int len )
{
    char *new = realloc( ab->b, ab->len+len );

    if( new == NULL ) return;
    memcpy( new + ab->len,s,len );
    ab->b = new;
    ab->len += len;
}

void abFree( struct abuf *ab )
{
    free( ab->b );
}



/* ============================= Terminal update ============================ */

/* Draws the status line. Pulled out of editorRefreshScreen() for */
/*  modularity. */
	/* ab: the primary buffer, will get drawn to the conventional terminal. */
	/* util: the utility-zone buffer, will ONLY get drawn to an auxiliary */
	/*  display (such as a character LCD), which might not even exist. */
	/* fstatus & fstat_len: storage for info about the file. */
	/* rstatus & rstat_len: storage for info about... the display, what does */
	/*  'r' stand for? Row? */
void abStatusLine
(
	struct abuf *ab, struct abuf *util,
	
	char *fstatus, int fstat_len,
	char *rstatus, int rstat_len
)
{
	(void)util;
	
	fstat_len =
		snprintf
		(
			fstatus, fstat_len,
			"%.20s - %d lines %s",  E.filename, E.numrows, E.dirty ? "(modified)" : ""
		);
	rstat_len =
		snprintf
		(
			rstatus, rstat_len,
			"%d : %d/%d",  E.cx+1, E.rowoff + E.cy + 1, E.numrows
		);
	if( fstat_len > E.screencols )
	{
		fstat_len = E.screencols;
	}
	
	abAppend( ab, fstatus, fstat_len );
	abAppend( util, fstatus, fstat_len );
	while( fstat_len < E.screencols )
	{
		if( E.screencols - fstat_len == rstat_len )
		{
			abAppend( ab, rstatus, rstat_len );
			abAppend( util, rstatus, rstat_len );
			break;
			
		} else {
			
			abAppend( ab, " ", 1 );
			abAppend( util, " ", 1 );
			fstat_len++;
		}
	}
}
void editorStatusLine
(
	struct abuf *ab, struct abuf *util,
	
	char *fstatus, int fstat_len,
	char *rstatus, int rstat_len
)
{
	abStatusLine
	(
		ab, util,
		
		fstatus, fstat_len,
		rstatus, rstat_len
	);
}

	/* Renders the message line. The message will eventually move to the status line, and be replaced with a CLI area. */
	/* See editorStatusLine() for argument info. */
void abMessageLine( struct abuf *ab, struct abuf *util )
{
	(void)util;
	
	int msglen = strlen( E.statusmsg );
	if
	(
		msglen &&
		MILA_MESSAGETIMEOUTS ?
			( time( NULL ) - E.statusmsg_time < 5 ) :
			1
	)
	{
		/*
			statview_view sv = { 0 };
			
				/ * Where do we get stats* from? The 'E' global? Is there a source? * /
			if( !statview_fetchmsg( statstate *stats, E.screencols,  &sv ) )
			{
				exit( 1 );
			}
			
			abAppend( ab, sv.start, sv.len );
			abAppend( util, sv.start, sv.len );
		*/
		
		
		abAppend( ab, E.statusmsg, msglen <= E.screencols ? msglen : E.screencols );
		abAppend( util, E.statusmsg, msglen <= E.screencols ? msglen : E.screencols );
	}
}
void editorMessageLine( struct abuf *ab, struct abuf *util )
{
	abMessageLine( ab, util );
}

    /* The following code draws the utility area. At the current time it only */
    /*  handles status lines, but I intend to throw other stuff in too. */
		/* ab: the primary buffer, will get drawn to the conventional */
		/*  terminal. */
		/* util: the utility-zone buffer, will ONLY get drawn to an auxiliary */
		/*  display (such as a character LCD), which might not even exist. */
void abUtilityArea( struct abuf *ab, struct abuf *util )
{
    /* Prepare for the utility area: */
    char status[ 80 ], rstatus[ 80 ];
	
	/* First row: */
		/* We'll use reversed-color, both calls the line out and serves as a divider. */
	mila_ab_swapFgBg( ab );
	mila_ab_cleartoend( ab,  "" );
	mila_ab_swapFgBg( util );
	mila_ab_cleartoend( util,  "" );
	editorStatusLine( ab, util,   status, sizeof( status ),  rstatus, sizeof( rstatus ) );
	
	/* Second row depends on E.statusmsg and the status message update time. */
		/* Return foreground/background to normal. */
	mila_ab_resetAttribs( ab,  "\r\n" );
	mila_ab_cleartoend( ab,  "" );
	mila_ab_resetAttribs( util,  "\r\n" );
	mila_ab_cleartoend( util,  "" );
	editorMessageLine( ab, util );
	
#if MILA_UTILITYLINES != 2
	#error "MILA_UTILITYLINES doesn't match editorUtilityArea()."
#endif
}
void editorUtilityArea( struct abuf *ab, struct abuf *util )
{
	abUtilityArea( ab, util );
}
