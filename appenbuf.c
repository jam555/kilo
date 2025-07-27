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
#include "msgs.h"



int abRespodapt_fullinner
(
	struct abuf *buf,
	size_t extra,
	
	char **stati,
	size_t *lens,
	size_t count
);



void abAppend( struct abuf *ab, const char *s, size_t len )
{
    char *new = realloc( ab->b, ab->len + len );

    if( new == NULL ) return;
    memcpy( new + ab->len, s, len );
    ab->b = new;
    ab->len += len;
}

void abFree( struct abuf *ab )
{
    free( ab->b );
}



/* ============================= Terminal update ============================ */

	int abRespodapt_fullinner
	(
		struct abuf *buf,
		size_t extra,
		
		char **stati,
		size_t *lens,
		size_t count
	)
	{
		if( buf )
		{
			if( !count )
			{
				return( 0 );
			}
			if( !( stati && lens ) )
			{
				return( -2 );
			}
			
			size_t step = 0;
			if( count )
			{
				step = extra % ( count - 1 );
				step = ( extra - step ) / ( count - 1 );
			}
			
			while( count )
			{
				abAppend( buf,  *stati, *lens );
				
					/* Handle padding. */
				if( count > 1 )
				{
					size_t tmp = 0;
					while( tmp < step )
					{
						abAppend( buf,  " ", 1 );
						tmp += 1;
					}
					
					if
					(
						(double)( extra - ( step * ( count - 1 ) ) ) / 2.0 >= 0.4999
					)
					{
						abAppend( buf,  " ", 1 );
						extra -= 1;
					}
					
					extra -= tmp;
				}
				
				/* Increment. */
				--count;
				++stati;
				++lens;
			}
			
			return( 1 );
		}
		
		return( -1 );
	}
	
		/* This is a hack job, so mark it as deprecated from the start. */
	int abRespodapt_skimpinner
	(
		struct abuf *buf,
		size_t width,
		
		char **stati,
		size_t *lene,
		size_t *lens,
		size_t count
	) __attribute__ ((deprecated));
	int abRespodapt_skimpinner
	(
		struct abuf *buf,
		size_t width,
		
		char **stati,
		size_t *lene,
		size_t *lens,
		size_t count
	)
	{
		if( buf )
		{
			if( !count )
			{
				return( 0 );
			}
			if( !( stati && lene && lens ) )
			{
				return( -2 );
			}
			
			size_t i = 0;
			
			abAppend( buf,  stati[ 0 ], lens[ 0 ] );
			i += lene[ 0 ];
			
			if( count > 1 )
			{
				while( i + lene[ count - 1 ] < width )
				{
					abAppend( buf,  " ", 1 );
					i += 1;
				}
				
				if( width - i )
				{
					abAppend( buf,  stati[ count - 1 ], width - i );
				}
			}
			
			return( 1 );
		}
		
		return( -1 );
	}
	
	/* The Progressive/Responsive partitioning code. */
#warning "The message area of the status line jumps around when moving to row 10 from 9: fix that."
	int abRespodapt
	(
		struct abuf *ab,
		size_t abwide,
		
		struct abuf *util,
		size_t utilwide,
		
		char **stati,
			/* Effective length, only considers space consumed. */
		size_t *lene,
			/* Memory length, includes escape sequences. */
		size_t *lens,
		size_t count
	)
	{
		if( ab )
		{
			if( count < 1 )
			{
					/* Nothing to do. */
				return( 0 );
			}
			if( !( stati && lene && lens ) )
			{
				return( -2 );
			}
			
			int abfull = 0, utilfull = 0;
			size_t ablen = 0, utillen = 0;
			
			size_t loopsz = 0;
			while( loopsz < count )
			{
				ablen += lene[ loopsz ];
				utillen += lene[ loopsz ];
				++loopsz;
			}
			
			if( ablen <= abwide )
			{
				abRespodapt_fullinner
				(
					ab, abwide - ablen,
					
					stati, lens, count
				);
				
			} else {
				
				abRespodapt_skimpinner
				(
					ab, abwide,
					
					stati, lene, lens, count
				);
			}
			if( util )
			{
				if( utillen <= utilwide )
				{
					abRespodapt_fullinner
					(
						util, utilwide - utillen,
						
						stati, lens, count
					);
				} else {
					
					abRespodapt_skimpinner
					(
						util, utilwide,
						
						stati, lene, lens, count
					);
				}
			}
			
			return( 1 );
		}
		
		return( -1 );
	}

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
	
		/* These two status bits are just buffers: the contents are added IN */
		/*  THIS FUNCTION. */
	char *fstatus, size_t fstat_len,
	char *rstatus, size_t rstat_len
)
{
	(void)util;
	
	
	/* Prepare the editor status info. */
	int tmp =
		snprintf
		(
			fstatus, (size_t)fstat_len,
			
			"%.20s - %zu lines  |  %zu : %zu/%zu  | %s%s",
				/* File ID. Should a scrolling directory path be appended? */
				E.filename,
				E.numrows,
				
				/* Position. */
				E.cx + 1,
				E.rowoff + E.cy + 1,
				E.numrows,
				
				/* Edit status. */
				( E.dirty ? " (modified)" : "" ),
				( 0 ? " (read-only)" : " (read/write)" )
		);
	if( tmp < 0 )
	{
		msgs_build_fatal
		(
			(msgs**)0,
				"\tFirst snprintf() in abStatusLine() had a negative return: %d\n",
				tmp
		);
		exit( 1 );
	}
	fstat_len = (size_t)tmp;
	if( fstat_len > E.screencols )
	{
		fstat_len = E.screencols;
	}
	
	
	/* Prepare the mode info. */
	tmp =
		snprintf
		(
			rstatus, (size_t)rstat_len,
			
			/* Note that this should really indicate the active pane. */
			"Text-editor Mode, file: %s ",
				/* TODO: Change this to include the directory path too? Or just remove? */
				E.filename
		);
	if( tmp < 0 )
	{
		msgs_build_fatal( (msgs**)0, "Second snprintf() in abStatusLine() failed." );
		exit( 1 );
	}
	rstat_len = (size_t)tmp;
	
	
	/* Calc & render the final status line. */
	statview_view sv = { 0 };
	if( !statview_fetchmsg( E.statusinterface, /* Should calc this instead. */ 24,  &sv ) )
	{
		msgs_build_fatal( (msgs**)0,  "\tstatview_fetchmsg() failed in abStatusLine().\n" );
		exit( 1 );
	}
	char *stati[ 3 ] = { fstatus, sv.start, rstatus };
	size_t lens[ 3 ] = { fstat_len, sv.len, rstat_len };
	abRespodapt
	(
		ab, E.screencols,
		
		0, 0,
		
		stati, lens, lens, 3
	);
}
void editorStatusLine
(
	struct abuf *ab, struct abuf *util,
	
		/* These are just used as buffers, abStatusLine both fills AND uses the resulting contents. */
	char *fstatus, size_t fstat_len,
	char *rstatus, size_t rstat_len
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
#warning "Remove all the debug cruft from abMessageLine()."
void abMessageLine( struct abuf *ab, struct abuf *util )
{
	(void)util;
	
	abAppend( ab,  " ", 1 );
	abAppend( util,  " ", 1 );
	/* return; */
	
	size_t msglen;
	msgs_view msgsv = msgs_peek();
	if( !msgsv.buf || !( msgsv.buf->b ) )
	{
		msglen = 0;
		
	} else {
		
		msglen = strlen( msgsv.buf->b );
	}
	
	
	if( MILA_DISPLAYTEST_MESSAGE )
	{
		/*
		typedef struct statview_view
		{
			char *start;
			size_t len;
			unsigned char msgsflags;
			
		} statview_view;
		*/
		
		char status[ 160 ];
		int tmp;
		size_t len;
		
		/* Prepare the test info. */
		tmp =
			snprintf
			(
				status, sizeof( status ),
				
				// "time == %d:%d; "
				// "old time == %d:%d; "
				
				"E.d test == %d; "
				// "E.d test (off) == 0x%x; "
				"E.d ptr (base) = 0x%jx, "
				
				// "E.vptr (memb&) = %p; "
				// "*( E.vptr ) == %d;"
				// "E.vptr(string) == %s"
				
				" text: %20.20s "
				,
					// E.display_time.tm_min,
					// E.display_time.tm_sec,
					// E.old_time.tm_min,
					// E.old_time.tm_sec,
					
					(int)( E.display_test ),
					// (int)( E.display_test ),
					(intmax_t)( E.display_pointer ),
					
					// (void*)( E.vptr ),
					// *( (int*)E.vptr ),
					// (void*)( E.vptr ),
					
					E.display_text,
					
					
					(int)0 /* Dummy arg, scares away comma errors. */
			);
		if( tmp < 0 )
		{
			msgs_build_fatal( (msgs**)0, "snprintf() in abMessageLine() failed." );
			exit( 1 );
		}
		len = (size_t)tmp;
		
		abAppend( ab, status, len );
		
	} else if
	(
		MILA_MESSAGETIMEOUTS ?
			( time( NULL ) - E.statusmsg_time < 5 ) :
			1
	)
	{
		statview_view sv = { 0 };
		
		if( !statview_fetchmsg( E.statusinterface, /* E.screencols */ 12,  &sv ) )
		{
			msgs_build_fatal( (msgs**)0,  "\tstatview_fetchmsg() failed in abMessageLine().\n" );
			exit( 1 );
		}
		
		abAppend( ab, sv.start, sv.len );
		abAppend( util, sv.start, sv.len );
		
		
		/*
		abAppend( ab, msgsv.buf->b, msglen <= E.screencols ? msglen : E.screencols );
		abAppend( util, msgsv.buf->b, msglen <= E.screencols ? msglen : E.screencols );
		*/
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
