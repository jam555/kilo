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


#include <limits.h>

#include "../kilo.h"
#include "edrows.h"
#include "../msgs.h"


/* ======================= Editor rows implementation ======================= */

/* Update the rendered version and the syntax highlight of a row. */
void editorUpdateRow( erow *row )
{
    unsigned int tabs = 0, nonprint = 0;
    size_t j, idx;

   /* Create a version of the row we can directly print on the screen,
     * respecting tabs, substituting non printable characters with '?'. */
    free( row->render );
    for( j = 0; j < row->size; j++ )
	{
        if( row->chars[ j ] == TAB ) tabs++;
	}
	
    unsigned long long allocsize =
        (unsigned long long) row->size + tabs * MILA_TABSIZE + nonprint * 9 + 1;
    if( allocsize > UINT32_MAX )
	{
        	/* TODO: Print WHICH row it is. */
		msgs_build_fatal( (msgs**)0,  "\tSome line of the edited file is too long for kilo\n" );
        exit( 1 );
    }

    row->render = malloc( row->size + tabs * MILA_TABSIZE + nonprint * 9 + 1 );
    idx = 0;
    for( j = 0; j < row->size; j++ )
	{
        if( row->chars[ j ] == TAB )
		{
            row->render[ idx++ ] = ' ';
            while( ( idx + 1 ) % MILA_TABSIZE != 0 )
			{
				row->render[ idx++ ] = ' ';
			}
			
        } else {
            
			row->render[ idx++ ] = row->chars[ j ];
        }
    }
    row->rsize = idx;
    row->render[ idx ] = '\0';

    /* Update the syntax highlighting attributes of the row. */
    editorUpdateSyntax( row );
}

/* Insert a row at the specified position, shifting the other rows on the bottom
 * if required. */
void editorInsertRow( size_t at, char *s, size_t len )
{
    if( at > E.numrows ) return;
    E.row = realloc( E.row, sizeof( erow ) * ( E.numrows + 1 ) );
    if( at != E.numrows ) {
        memmove( E.row + at + 1, E.row + at, sizeof( E.row[ 0 ] ) * ( E.numrows - at ) );
        for( size_t j = at + 1; j <= E.numrows; j++ ) E.row[ j ].idx++;
    }
    E.row[ at ].size = len;
    E.row[ at ].chars = malloc( len + 1 );
    memcpy( E.row[ at ].chars, s, len + 1 );
    E.row[ at ].hl = NULL;
    E.row[ at ].hl_oc = 0;
    E.row[ at ].render = NULL;
    E.row[ at ].rsize = 0;
    E.row[ at ].idx = at;
    editorUpdateRow( E.row + at );
    E.numrows++;
    E.dirty++;
}

/* Free row's heap allocated stuff. */
void editorFreeRow( erow *row )
{
    free(row->render);
    free(row->chars);
    free(row->hl);
}

/* Remove the row at the specified position, shifting the remaining onto the
 * top. */
void editorDelRow( size_t at )
{
    erow *row;

    if( at >= E.numrows )
	{
		return;
    }
	row = E.row+at;
    editorFreeRow( row );
    memmove( E.row + at, E.row + at + 1, sizeof( E.row[ 0 ] ) * ( E.numrows - at - 1 ) );
    for( size_t j = at; j < E.numrows-1; j++ )
	{
		E.row[ j ].idx++;
    }
	E.numrows--;
    E.dirty++;
}

/* Turn the editor rows into a single heap-allocated string.
 * Returns the pointer to the heap-allocated string and populate the
 * integer pointed by 'buflen' with the size of the string, escluding
 * the final nulterm. */
char *editorRowsToString( size_t *buflen )
{
    char *buf = NULL, *p;
    size_t totlen = 0;
    size_t j;

    /* Compute count of bytes */
    for( j = 0; j < E.numrows; j++ )
	{
        totlen += E.row[ j ].size + 1; /* +1 is for "\n" at end of every row */
    }
	if( totlen > INT_MAX )
	{
		exit( 1 );
	}
	*buflen = totlen;
    totlen++; /* Also make space for nulterm */

    p = buf = malloc( totlen );
    for( j = 0; j < E.numrows; j++ )
	{
        memcpy( p, E.row[ j ].chars, E.row[ j ].size );
        p += E.row[ j ].size;
        *p = '\n';
        p++;
    }
    *p = '\0';
    return buf;
}

/* Insert a character at the specified position in a row, moving the remaining
 * chars on the right if needed. */
void editorRowInsertChar( erow *row, size_t at, int c )
{
    if( at > row->size )
	{
        /* Pad the string with spaces if the insert location is outside the
         * current length by more than a single character. */
        size_t padlen = at-row->size;
        /* In the next line +2 means: new char and null term. */
        row->chars = realloc( row->chars, row->size + padlen + 2 );
        memset( row->chars + row->size, ' ', padlen );
        row->chars[ row->size + padlen + 1 ] = '\0';
        row->size += padlen + 1;
		
    } else {
        
		/* If we are in the middle of the string just make space for 1 new
         * char plus the (already existing) null term. */
        row->chars = realloc( row->chars, row->size + 2 );
        memmove( row->chars + at + 1, row->chars + at, row->size - at + 1 );
        row->size++;
    }
    if( CHAR_MIN > c || c < CHAR_MAX )
	{
		msgs_build_fatal
		(
			(msgs**)0,
				"\teditorRowInsertChar() encountered an out-of-bounds character: %x\n",
				(int)c
		);
		exit( 1 );
	}
	row->chars[ at ] = (char)c;
    editorUpdateRow( row );
    E.dirty++;
}

/* Append the string 's' at the end of a row */
void editorRowAppendString( erow *row, char *s, size_t len )
{
    row->chars = realloc( row->chars, row->size + len + 1 );
    memcpy( row->chars + row->size, s, len );
    row->size += len;
    row->chars[ row->size ] = '\0';
    editorUpdateRow( row );
    E.dirty++;
}

/* Delete the character at offset 'at' from the specified row. */
void editorRowDelChar( erow *row, size_t at )
{
    if( row->size <= at )
	{
		return;
	}
    memmove( row->chars + at, row->chars + at + 1, row->size - at );
    editorUpdateRow( row );
    row->size--;
    E.dirty++;
}

/* Insert the specified char at the current prompt position. */
void editorInsertChar( int c )
{
    size_t filerow = E.rowoff+E.cy;
    size_t filecol = E.coloff+E.cx;
    erow *row = ( filerow >= E.numrows ) ? NULL : &E.row[ filerow ];

    /* If the row where the cursor is currently located does not exist in our
     * logical representaion of the file, add enough empty rows as needed. */
    if( !row )
	{
        while( E.numrows <= filerow )
		{
            editorInsertRow( E.numrows, "", 0 );
		}
    }
    row = &E.row[ filerow ];
    editorRowInsertChar( row, filecol, c );
    if( E.cx == E.screencols - 1 )
	{
        E.coloff++;
		
    } else {
        
		E.cx++;
    }
	E.dirty++;
}

/* Inserting a newline is slightly complex as we have to handle inserting a
 * newline in the middle of a line, splitting the line as needed. */
void editorInsertNewline( void )
{
    size_t filerow = E.rowoff + E.cy;
    size_t filecol = E.coloff + E.cx;
    erow *row = ( filerow >= E.numrows ) ? NULL : &E.row[ filerow ];

    if( !row )
	{
        if( filerow == E.numrows )
		{
            editorInsertRow( filerow, "", 0 );
            goto fixcursor;
        }
        return;
    }
    /* If the cursor is over the current line size, we want to conceptually
     * think it's just over the last character. */
    if( filecol >= row->size )
	{
		filecol = row->size;
	}
    if( filecol == 0 )
	{
        editorInsertRow( filerow, "", 0 );
		
    } else {
        
		/* We are in the middle of a line. Split it between two rows. */
        editorInsertRow( filerow + 1, row->chars + filecol, row->size - filecol );
        row = &E.row[ filerow ];
        row->chars[ filecol ] = '\0';
        row->size = filecol;
        editorUpdateRow( row );
    }
fixcursor:
    if( E.cy == E.screenrows - 1 )
	{
        E.rowoff++;
		
    } else {
        
		E.cy++;
    }
    E.cx = 0;
    E.coloff = 0;
}

/* Delete the char at the current prompt position. */
void editorDelChar( void )
{
	size_t filerow = E.rowoff + E.cy;
	size_t filecol = E.coloff + E.cx;
	erow *row = ( filerow >= E.numrows ) ? NULL : &E.row[ filerow ];
	
	if( !row || ( filecol == 0 && filerow == 0 ) )
	{
		return;
	}
	if( filecol == 0 )
	{
		/* Handle the case of column 0, we need to move the current line
		 * on the right of the previous one. */
		filecol = E.row[ filerow - 1 ].size;
		editorRowAppendString( &E.row[ filerow - 1 ], row->chars, row->size );
		editorDelRow( filerow );
		row = NULL;
		if( E.cy == 0 )
		{
			E.rowoff--;
			
		} else {
			
			E.cy--;
		}
		E.cx = filecol;
		if( E.cx >= E.screencols )
		{
			size_t shift = ( E.screencols-E.cx ) + 1;
			E.cx -= shift;
			E.coloff += shift;
		}
		
	} else {
		
		editorRowDelChar( row, filecol - 1 );
		if( E.cx == 0 && E.coloff )
		{
			E.coloff--;
			
		} else {
			
			E.cx--;
		}
	}
	if( row )
	{
		editorUpdateRow( row );
	}
	E.dirty++;
}
