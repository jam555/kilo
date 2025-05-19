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

	/* Calculate the on-screen position of the cursor position specified in */
	/*  the relevant kilo.h:editorConfig{} */
	/* BE AWARE! This ALTERS the provided ints, but DOES NOT CLEAR THEM, so */
	/*  the value stored in those ints WILL alter the final result. ALSO, the */
	/*  ONLY protection against null-pointers is that if the int pointers are */
	/*  null, then they'll be redirected to an internal int. */
void editorCalc_CurScreenPos( int *x, int *y )
{
	/* TODO: Alter this to take VTAB into account for *y */
	
	int x_ = 1, y_ = E.cy + 1;
	if( !x )
	{
		x = &x_;
	}
	if( !y )
	{
		y = &y_;
	}
	
    int j;
    int filerow = E.rowoff + E.cy;
    erow *row = ( filerow >= E.numrows ) ? NULL : &E.row[ filerow ];
    if( row )
	{
        for( j = E.coloff; j < ( E.cx + E.coloff ); j++ )
		{
            if( j < row->size && row->chars[ j ] == TAB )
			{
				*x += ( MILA_TABSIZE - 1 ) - ( ( *x ) % MILA_TABSIZE );
			}
            ( *x )++;
        }
    }
}

    /* Update the cursor position to reflect it's "official" position. Note */
	/*  that e.g. TABs may cause the on-screen position to be different than */
	/*  e.g. E.cx */
void editorUpdateCurPos( struct abuf *ab )
{
	int cx = 1, cy = E.cy + 1;
	
	editorCalc_CurScreenPos( &cx, &cy );
	mila_ab_curseek( ab, 2,   cy, cx, "" ); /* Move cursor. */
}


/* TODO: Start using the util* variables. */

/* Draws the status line. Pulled out of editorRefreshScreen() for */
/*  modularity. */
void editorStatusLine( struct abuf *ab, struct abuf *util,   char *status, int stat_len,  char *rstatus, int rstat_len )
{
	(void)util;
	
	stat_len =
		snprintf
		(
			status, stat_len,
			"%.20s - %d lines %s",  E.filename, E.numrows, E.dirty ? "(modified)" : ""
		);
	rstat_len =
		snprintf
		(
			rstatus, rstat_len,
			"%d : %d/%d",  E.cx+1, E.rowoff + E.cy + 1, E.numrows
		);
	if( stat_len > E.screencols )
	{
		stat_len = E.screencols;
	}
	
	abAppend( ab, status, stat_len );
	abAppend( util, status, stat_len );
	while( stat_len < E.screencols )
	{
		if( E.screencols - stat_len == rstat_len )
		{
			abAppend( ab, rstatus, rstat_len );
			abAppend( util, rstatus, rstat_len );
			break;
			
		} else {
			
			abAppend( ab, " ", 1 );
			abAppend( util, " ", 1 );
			stat_len++;
		}
	}
}
void editorMessageLine( struct abuf *ab, struct abuf *util )
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
		abAppend( ab, E.statusmsg, msglen <= E.screencols ? msglen : E.screencols );
		abAppend( util, E.statusmsg, msglen <= E.screencols ? msglen : E.screencols );
	}
}

    /* The following code draws the utility area. At the current time it only */
    /*  handles status lines, but I intend to throw other stuff in too. */
void editorUtilityArea( struct abuf *ab, struct abuf *util )
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


/* This function writes the whole screen using VT100 escape characters
 * starting from the logical state of the editor in the global state 'E'. */
void editorRefreshScreen( void )
{
    int y;
    erow *r;
    struct abuf ab = ABUF_INIT;

    mila_ab_curvis_hide( &ab );
    mila_ab_curseek_home( &ab );
    for( y = 0; y < E.screenrows; y++ )
	{
        int filerow = E.rowoff + y;

        if( filerow >= E.numrows )
		{
            if( E.numrows == 0 && y == E.screenrows / 3 )
			{
                char welcome[ 80 ];
#define MILA_TERMCODES_9 "\x1b[0K"
                int welcomelen =
					snprintf
					(
						welcome, sizeof( welcome ),
                    	
						"Kilo editor -- verison %s%s\r\n",
						KILO_VERSION, MILA_TERMCODES_9
					);
                int padding = ( E.screencols - welcomelen ) / 2;
                if( padding )
				{
                    abAppend( &ab, "~", 1 );
                    padding--;
                }
                while( padding-- )
				{
					abAppend( &ab," ",1 );
				}
                abAppend( &ab, welcome, welcomelen );
				
            } else {
                
				mila_ab_cleartoend( &ab, "\r\n" );
            }
            continue;
        }

        r = &E.row[ filerow ];

        int len = r->rsize - E.coloff;
        int current_color = -1;
        if( len > 0 )
		{
            if( len > E.screencols )
			{
				len = E.screencols;
			}
            char *c = r->render + E.coloff;
            unsigned char *hl = r->hl + E.coloff;
            int j;
            for( j = 0; j < len; j++ )
			{
                if( hl[ j ] == HL_NONPRINT )
				{
                    char sym;
                    mila_ab_swapFgBg( &ab );
                    if( c[ j ] <= 26 )
					{
                        sym = '@' + c[ j ];
						
                    } else {
                        
						sym = '?';
                    }
					abAppend( &ab, &sym, 1 );
                    mila_ab_resetAttribs( &ab, "" );
					
                } else if( hl[ j ] == HL_NORMAL )
				{
                    if( current_color != -1 )
					{
                        mila_ab_defaultFg( &ab );
                        current_color = -1;
                    }
                    abAppend( &ab, c + j, 1 );
					
                } else {
                    
					int color = editorSyntaxToColor( hl[ j ] );
                    if( color != current_color )
					{
                        char buf[ 16 ];
#warning "We can't do this, we need to print ints!"
#define MILA_TERMCODES_14 "\x1b[%dm"
                        int clen =
							snprintf
							(
								buf, sizeof( buf ),
								MILA_TERMCODES_14, color
							);
                        current_color = color;
                        abAppend( &ab, buf, clen );
                    }
                    abAppend( &ab, c + j, 1 );
                }
            }
        }
        mila_ab_defaultFg( &ab );
        mila_ab_cleartoend( &ab, "\r\n" );
    }
	
	
	struct abuf util = ABUF_INIT;

		/* Render the utility area. */
	editorUtilityArea( &ab, &util );

		/* Restore the cursor to it's "official" position. */
	editorUpdateCurPos( &ab );
	mila_ab_curvis_show( &ab ); /* Show cursor. */
	
    /* Render the display. */
	write( STDOUT_FILENO, ab.b, ab.len );
	/* TODO: We need to copy a "window" from util into... wherever in */
	/*  E that we write it to. */
    abFree( &util );
    abFree( &ab );
}

/* Set an editor status message for the second line of the status, at the
 * end of the screen. */
void editorSetStatusMessage( const char *fmt, ... )
{
    va_list ap;
    va_start( ap,fmt );
    vsnprintf( E.statusmsg, sizeof( E.statusmsg ), fmt, ap );
    va_end( ap );
    E.statusmsg_time = time( NULL );
}
