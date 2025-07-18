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

#include "../kilo.h"
#include "edtools.h"

	/* Calculate the on-screen position of the cursor position specified in */
	/*  the relevant kilo.h:editorConfig{} */
	/* BE AWARE! This ALTERS the provided ints, but DOES NOT CLEAR THEM, so */
	/*  the value stored in those ints WILL alter the final result. ALSO, the */
	/*  ONLY protection against null-pointers is that if the int pointers are */
	/*  null, then they'll be redirected to an internal int. */
void editorCalc_CurScreenPos( size_t *x, size_t *y )
{
	/* TODO: Alter this to take VTAB into account for *y */
	
	size_t x_ = 1, y_ = E.cy + 1;
	if( !x )
	{
		x = &x_;
	}
	if( !y )
	{
		y = &y_;
	}
	
    size_t j;
    size_t filerow = E.rowoff + E.cy;
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
	size_t cx = 1, cy = E.cy + 1;
	
	editorCalc_CurScreenPos( &cx, &cy );
	mila_ab_curseek( ab, 2,   cy, cx, "" ); /* Move cursor. */
}





/* ========================== Editor display update ========================= */


/* This function writes the whole screen using VT100 escape characters
 * starting from the logical state of the editor in the global state 'E'. */
void editorRefreshScreen( void )
{
    size_t y;
    erow *r;
    struct abuf ab = ABUF_INIT;
	/* E.display_test = E.statusinterface->last_size; */
	
    mila_ab_curvis_hide( &ab );
    mila_ab_curseek_home( &ab );
    for( y = 0; y < E.screenrows; y++ )
	{
        size_t filerow = E.rowoff + y;
		
        if( filerow >= E.numrows )
		{
            if( E.numrows == 0 && y == E.screenrows / 3 )
			{
                char welcome[ 80 ];
				mila_term_printWelcomeMessage
					( &ab,  welcome, sizeof( welcome ) );
				
            } else {
                
				mila_ab_cleartoend( &ab, "\r\n" );
            }
            continue;
        }

        r = &E.row[ filerow ];

        size_t len = r->rsize - E.coloff;
        int current_color = -1;
        if( len > 0 )
		{
            if( len > E.screencols )
			{
				len = E.screencols;
			}
            char *c = r->render + E.coloff;
            unsigned char *hl = r->hl + E.coloff;
            size_t j;
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
                        mila_term_setcolor
						(
							&ab,  buf, sizeof( buf ),
							color, &current_color
						);
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
	
	E.stale = 0;
}
