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


/* =============================== Find mode ================================ */

#warning "editorFind() needs to roughly be a mode."

void editorFind( int fd )
{
	char query[ KILO_QUERY_LEN + 1 ] = { 0 };
	axis_type qlen = 0;
	axis_type
		last_match = 0, /* Last line where a match was found. See "has_match" for none. */
		saved_hl_line = 0;  /* Current highlighted line. See "has_match" for none. */
	int has_match = 0; /* 0 if there is no match, else 1. */
	int find_next = 0; /* if 1 search next, if -1 search prev. */
	char *saved_hl = NULL;
	
#define FIND_RESTORE_HL do { \
    if( saved_hl ) { \
        memcpy( E.row[ saved_hl_line ].hl, saved_hl, E.row[ saved_hl_line ].rsize ); \
        free( saved_hl ); /* Vuong Hoang */ \
        saved_hl = NULL; \
    } \
} while (0)
	
	/* Save the cursor position in order to restore it later. */
	axis_type saved_cx = E.cx, saved_cy = E.cy;
	axis_type saved_coloff = E.coloff, saved_rowoff = E.rowoff;
	/* msgs *msgtmp = 0; */ /* Was used to track msgs{} for later deactivation maybe? */
	
	while( 1 )
	{
		int res = modemsgs_setmodal( MODEMSGS_MILLI_FIND );
#warning "Add some method to display the current search string."
		switch( res )
		{
			case 0:
			case 1:
				break;
			case -1:
				/* .deathrattle is already set. */
				exit( 1 );
			case -2:
				E.deathrattle = "\neditorFind() : modemsgs_setmodal() : msgs_queue_append() failure.\n";
				exit( 1 );
			default:
				E.deathrattle = "\neditorFind unforeseen failure 1.\n";
				exit( 2 );
		}
		editorRefreshScreen();
		
		int c = editorReadKey( fd );
		if( c == ESC || c == ENTER )
		{
			/* Done with find. */
			
			if( c == ESC )
			{
				E.cx = saved_cx;
				E.cy = saved_cy;
				E.coloff = saved_coloff;
				E.rowoff = saved_rowoff;
			}
			FIND_RESTORE_HL;
			if( modemsgs_setmodal( MODEMSGS_MILLI_MAIN ) < 0 )
			{
					/* Note: do something to pass on a message... maybe pre-specced exit values? */
				E.deathrattle = "\neditorFind unforeseen failure 2.\n";
				exit( 2 );
			}
			return;
			
			

		} else if( c == DEL_KEY || c == CTRL_H || c == BACKSPACE )
		{
			/* Truncate query string. */
			
			if( qlen > 0 )
			{
				query[ --qlen ] = '\0';
				
			} else {
				
				/* Throw some sort of error. */
			}
			last_match = 0;
			has_match = 0;
			
		} else if( isprint( c ) )
		{
			/* Grow query string. */
			
			if( qlen < KILO_QUERY_LEN )
			{
				query[ qlen++ ] = (char)c; /* Trust isprint() */
				query[ qlen ] = '\0';
				last_match = 0;
				has_match = 0;
				
			} else {
				
				/* Throw some sort of error. */
			}
			
			
			
		} else if( c == ARROW_RIGHT || c == ARROW_DOWN )
		{
			find_next = 1;
			
		} else if( c == ARROW_LEFT || c == ARROW_UP )
		{
			find_next = -1;
			
		} else {
			
			io_unknownkey_message( "editorFind", c );
			continue;
		}
		
		/* Search for occurrence. */
		if( has_match == 0 )
		{
			find_next = 1;
		}
		if( find_next )
		{
			char *match = NULL;
			ptrdiff_t match_offset = 0;
			axis_type
				i,
				currow = (axis_type)( has_match ? last_match : 0 ),
				curneg = !has_match;
			
			/* Actually search. */
			for( i = 0; i && (unsigned)i < E.numrows; i++ )
			{
				/* The iteration is this ENTIRE conditional cascade. */
				if( find_next < 0 && currow <= 0 )
				{
					if( E.numrows >= 1 )
					{
						currow = E.numrows - 1;
						curneg = 0;
						
					} else {
						
						currow = 0;
						curneg = 1;
					}
					
				} else if( find_next > 0 && currow + 1 == E.numrows )
				{
					currow = 0;
					curneg = 0;
					
				} else {
					
#pragma GCC diagnostic push
	/* Silence the conversion complaint, we've already verified the range. */
# pragma GCC diagnostic ignored "-Wsign-conversion"
					currow += find_next;
#pragma GCC diagnostic pop
					curneg = 0;
				}
				
				
					/* Actual comparison. */
				match = strstr( E.row[ currow ].render, query );
				if( match )
				{
					match_offset = match - E.row[ currow ].render;
					if( match_offset < 0 )
					{
						/* Throw some sort of error. */
					}
					break;
				}
			}
			find_next = 0;
			
			/* Highlight */
			FIND_RESTORE_HL;
			
			/* Update position info. */
			if( match )
			{
                /* If we have a match, then ( !curneg ). */
				
				erow *row = &E.row[ currow ];
                last_match = currow;
				has_match = !curneg;
				
				if( row->hl )
				{
					saved_hl_line = currow;
					saved_hl = malloc( row->rsize );
					memcpy( saved_hl, row->hl, row->rsize );
					memset( row->hl + match_offset, HL_MATCH, qlen );
				}
				
				E.cy = 0;
				E.cx = (axis_type)match_offset;
				E.rowoff = currow;
				E.coloff = 0;
				
				/* Scroll horizontally as needed. */
				if( E.cx > E.screencols )
				{
					ptrdiff_t diff = (ptrdiff_t)( E.cx - E.screencols );
					
					if( diff && E.cx < (size_t)( diff ) )
					{
						msgs_build_fatal( (msgs**)0,  "\teditorFind() err 1. diff: %d; E.cx: %zu\n", (int)diff, E.cx );
						exit( 1 );
					}
					E.cx -= (size_t)diff;
					if( !diff && E.coloff < (size_t)( -diff ) )
					{
						msgs_build_fatal( (msgs**)0,  "\teditorFind err 2. diff: $d; E.coloff: %zu\n", (int)diff, E.coloff );
						exit( 1 );
					}
#pragma GCC diagnostic push
	/* Silence the conversion complaint, we've already verified the range. */
# pragma GCC diagnostic ignored "-Wsign-conversion"
					E.coloff += diff;
#pragma GCC diagnostic pop
                }
            }
        }
    }
}
