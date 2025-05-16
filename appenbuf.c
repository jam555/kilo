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



void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b,ab->len+len);

    if (new == NULL) return;
    memcpy(new+ab->len,s,len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

void mila_ab_curseek( struct abuf *ab, int argn,   int x, int y, char *tail )
{
	char buf[32];
	
#define MILA_TERMCODES_8 "\x1b[H%s" /* Go home. */
#define mila_ab_curseek_ONEARG "\x1b[%dH%s"
#define MILA_TERMCODES_21 "\x1b[%d;%dH%s"
	
	if( !tail )
	{
		tail = "";
	}
	
	/* Populate the buffer. */
	if( argn == 0 )
	{
		snprintf( buf, sizeof(buf), MILA_TERMCODES_8,  tail );
		
	} else if( argn == 1 )
	{
		snprintf( buf, sizeof(buf), mila_ab_curseek_ONEARG, x,  tail );
		
	} else if( argn == 2 )
	{
		snprintf (buf, sizeof(buf), MILA_TERMCODES_21, x, y,  tail );
	}
	
	abAppend( ab,buf,strlen(buf));
}
void mila_ab_curvis_hide( struct abuf *ab )
{
#define MILA_TERMCODES_7 "\x1b[?25l"
    abAppend( ab,MILA_TERMCODES_7,6); /* Hide cursor. */
}
void mila_ab_curvis_show( struct abuf *ab )
{
#define MILA_TERMCODES_22 "\x1b[?25h"
    abAppend( ab,MILA_TERMCODES_22,6); /* Show cursor. */
}
void mila_ab_curseek_home( struct abuf *ab )
{
	mila_ab_curseek( ab, 0,   0, 0, "" );
}
void mila_ab_clearall( struct abuf *ab )
{
#define mila_ab_clearall_TERMCODE "\x1b[2K\r\n"
	abAppend( ab,mila_ab_clearall_TERMCODE,7);
}
void mila_ab_cleartostart( struct abuf *ab )
{
#define mila_ab_cleartostart_TERMCODE "\x1b[1K\r\n"
	abAppend( ab,mila_ab_cleartostart_TERMCODE,7);
}
void mila_ab_cleartoend( struct abuf *ab, char *tail )
{
	char buf[32];
	
	if( !tail )
	{
		tail = "";
	}
	
#define mila_ab_cleartoend_TERMCODE "\x1b[0K%s"
	snprintf( buf, sizeof(buf), mila_ab_cleartoend_TERMCODE,  tail );
	
	abAppend( ab,buf,strlen(buf));
}
void mila_ab_swapFgBg( struct abuf *ab )
{
	abAppend( ab,MILA_TERMCODES_11,4);
}
void mila_ab_resetAttribs( struct abuf *ab, char *tail )
{
	char buf[32];
	
	if( !tail )
	{
		tail = "";
	}
	
#define MILA_TERMCODES_12 "\x1b[0m%s"
	snprintf( buf, sizeof(buf), MILA_TERMCODES_12,  tail );
	
	abAppend( ab,buf,strlen(buf));
}
void mila_ab_defaultFg( struct abuf *ab )
{
#define MILA_TERMCODES_13 "\x1b[39m"
	abAppend( ab,MILA_TERMCODES_13,5);
}


/* ============================= Terminal update ============================ */

/* This function writes the whole screen using VT100 escape characters
 * starting from the logical state of the editor in the global state 'E'. */
void editorRefreshScreen(void) {
    int y;
    erow *r;
    char buf[32];
    struct abuf ab = ABUF_INIT;

    mila_ab_curvis_hide( &ab );
    mila_ab_curseek_home( &ab );
    for (y = 0; y < E.screenrows; y++) {
        int filerow = E.rowoff+y;

        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows/3) {
                char welcome[80];
                int welcomelen = snprintf(welcome,sizeof(welcome),
#define MILA_TERMCODES_9 "\x1b[0K"
                    "Kilo editor -- verison %s%s\r\n", KILO_VERSION,MILA_TERMCODES_9);
                int padding = (E.screencols-welcomelen)/2;
                if (padding) {
                    abAppend( &ab,"~",1);
                    padding--;
                }
                while(padding--) abAppend(&ab," ",1);
                abAppend( &ab,welcome,welcomelen);
            } else {
                mila_ab_cleartoend(  &ab, "\r\n" );
            }
            continue;
        }

        r = &E.row[filerow];

        int len = r->rsize - E.coloff;
        int current_color = -1;
        if (len > 0) {
            if (len > E.screencols) len = E.screencols;
            char *c = r->render+E.coloff;
            unsigned char *hl = r->hl+E.coloff;
            int j;
            for (j = 0; j < len; j++) {
                if (hl[j] == HL_NONPRINT) {
                    char sym;
                    mila_ab_swapFgBg( &ab );
                    if (c[j] <= 26)
                        sym = '@'+c[j];
                    else
                        sym = '?';
                    abAppend( &ab,&sym,1);
                    mila_ab_resetAttribs( &ab, "" );
                } else if (hl[j] == HL_NORMAL) {
                    if( current_color != -1 ) {
                        mila_ab_defaultFg( &ab );
                        current_color = -1;
                    }
                    abAppend( &ab,c+j,1);
                } else {
                    int color = editorSyntaxToColor( hl[j] );
                    if (color != current_color) {
                        char buf[16];
#warning "We can't do this, we need to print ints!"
#define MILA_TERMCODES_14 "\x1b[%dm"
                        int clen = snprintf( buf,sizeof(buf),MILA_TERMCODES_14,color );
                        current_color = color;
                        abAppend( &ab,buf,clen );
                    }
                    abAppend( &ab,c+j,1 );
                }
            }
        }
        mila_ab_defaultFg( &ab );
        mila_ab_cleartoend( &ab, "\r\n" );
    }


    /* The following code draws the utility area. At the current time it only */
    /*  handles status lines, but I intend to throw other stuff in too. */


    /* Create a two rows status. First row: */
    mila_ab_cleartoend( &ab, "" );
	mila_ab_swapFgBg( &ab );
    char status[80], rstatus[80];
	int len =
		snprintf
		(
			status, sizeof( status ),
			"%.20s - %d lines %s",  E.filename, E.numrows, E.dirty ? "(modified)" : ""
		);
    int rlen =
		snprintf
		(
			rstatus, sizeof(rstatus),
			"%d : %d/%d",  E.cx+1, E.rowoff+E.cy+1, E.numrows
		);
    if( len > E.screencols )
	{
		len = E.screencols;
	}
    abAppend( &ab,status,len );
    while( len < E.screencols ) {
        if( E.screencols - len == rlen ) {
            abAppend( &ab, rstatus, rlen );
            break;
        } else {
            abAppend( &ab, " ", 1 );
            len++;
        }
    }
	mila_ab_resetAttribs( &ab, "\r\n" );

    /* Second row depends on E.statusmsg and the status message update time. */
    mila_ab_cleartoend( &ab, "" );
    int msglen = strlen( E.statusmsg );
    if( msglen && time(NULL)-E.statusmsg_time < 5 )
        abAppend( &ab, E.statusmsg, msglen <= E.screencols ? msglen : E.screencols );

    /* Put cursor at its current position. Note that the horizontal position
     * at which the cursor is displayed may be different compared to 'E.cx'
     * because of TABs. */
	 /* TODO: split this code so that the tab-corrected location can be used */
	 /*  as the text-column value. */
	 /* Also, add configurability to the tab size. */
    int j;
    int cx = 1;
    int filerow = E.rowoff+E.cy;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    if( row )
	{
        for( j = E.coloff; j < (E.cx+E.coloff); j++ )
		{
            if( j < row->size && row->chars[j] == TAB )
			{
				cx += (MILA_TABSIZE-1)-((cx)%MILA_TABSIZE);
			}
            cx++;
        }
    }
    mila_ab_curseek( &ab, 2,   E.cy+1,cx, "" ); /* Move cursor. */
	mila_ab_curvis_show( &ab ); /* Show cursor. */
	
    write(STDOUT_FILENO,ab.b,ab.len);
    abFree( &ab );
}

/* Set an editor status message for the second line of the status, at the
 * end of the screen. */
void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start( ap,fmt );
    vsnprintf( E.statusmsg,sizeof(E.statusmsg),fmt,ap );
    va_end( ap );
    E.statusmsg_time = time( NULL );
}
