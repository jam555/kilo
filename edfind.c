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


/* =============================== Find mode ================================ */

void editorFind( int fd )
{
    char query[ KILO_QUERY_LEN + 1 ] = {0};
    int qlen = 0;
    int last_match = -1; /* Last line where a match was found. -1 for none. */
    int find_next = 0; /* if 1 search next, if -1 search prev. */
    int saved_hl_line = -1;  /* No saved HL */
    char *saved_hl = NULL;

#define FIND_RESTORE_HL do { \
    if( saved_hl ) { \
        memcpy( E.row[ saved_hl_line ].hl,saved_hl, E.row[ saved_hl_line ].rsize ); \
        free( saved_hl ); \
        saved_hl = NULL; \
    } \
} while (0)

    /* Save the cursor position in order to restore it later. */
    int saved_cx = E.cx, saved_cy = E.cy;
    int saved_coloff = E.coloff, saved_rowoff = E.rowoff;

    while( 1 ) {
        editorSetStatusMessage(
            "Search: %s (Use ESC/Arrows/Enter)", query );
        editorRefreshScreen();

        int c = editorReadKey( fd );
        if( c == DEL_KEY || c == CTRL_H || c == BACKSPACE ) {
            if( qlen != 0 ) query[ --qlen ] = '\0';
            last_match = -1;
        } else if( c == ESC || c == ENTER ) {
            if( c == ESC ) {
                E.cx = saved_cx; E.cy = saved_cy;
                E.coloff = saved_coloff; E.rowoff = saved_rowoff;
            }
            FIND_RESTORE_HL;
            editorSetStatusMessage( "" );
            return;
        } else if( c == ARROW_RIGHT || c == ARROW_DOWN ) {
            find_next = 1;
        } else if( c == ARROW_LEFT || c == ARROW_UP ) {
            find_next = -1;
        } else if( isprint( c ) ) {
            if( qlen < KILO_QUERY_LEN ) {
                query[ qlen++ ] = (char)c; /* Trust isprint() */
                query[ qlen ] = '\0';
                last_match = -1;
            }
        }

        /* Search occurrence. */
        if( last_match == -1 ) find_next = 1;
        if( find_next ) {
            char *match = NULL;
            int match_offset = 0;
            int i, current = last_match;

            for( i = 0; i < E.numrows; i++ ) {
                current += find_next;
                if( current == -1 ) current = E.numrows-1;
                else if( current == E.numrows ) current = 0;
                match = strstr( E.row[ current ].render, query );
                if( match ) {
                    match_offset = match-E.row[ current ].render;
                    break;
                }
            }
            find_next = 0;

            /* Highlight */
            FIND_RESTORE_HL;

            if( match ) {
                erow *row = &E.row[ current ];
                last_match = current;
                if( row->hl ) {
                    saved_hl_line = current;
                    saved_hl = malloc( row->rsize );
                    memcpy( saved_hl, row->hl, row->rsize );
                    memset( row->hl + match_offset, HL_MATCH, qlen );
                }
                E.cy = 0;
                E.cx = match_offset;
                E.rowoff = current;
                E.coloff = 0;
                /* Scroll horizontally as needed. */
                if( E.cx > E.screencols ) {
                    ptrdiff_t diff = (ptrdiff_t)( E.cx - E.screencols );
                    E.cx -= diff;
                    E.coloff += diff;
                }
            }
        }
    }
}
