/* Thou -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * europa.c : A terminal-focused implementation of the IO system.
 *
 * Copyright (C) 2025 Jam555 <3349478+jam555@users.noreply.github.com>
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


#include <stdio.h>
#include <stdint.h>

#include "../kilo.h"
#include "io.h"
#include "cellarr.h"
#include "../dynarr.h"
#include "../utility.h"



typedef struct europa europa;
	/* The terminal. */
struct europa
{
	io header;
	
		
	FILE *src, *dest;
	struct
	{
		size_t width, height;
		
	} size; /* Available terminal area. */
	
		/* Will often contain escapes. */
	cellarr *cells;
		/* The terminal should HOPEFULLY have a distinct title bar. */
	dynarr *title;
	
	struct termios orig_termios;
	
	io_europa_flags flags;
	
		/* The last-chanve choice for error messages. */
	char *deathrattle;
	
#warning "europa{} needs to have a target for msgs{} fatal messages to target."
};

	/* Originally by Leandro Pereira */
void europa_updateWindowSize( europa *eu );



static uintptr_t id;

static europa europa_stdio = { 0 };



static int europa_sendchar
(
	io *stream,
	io_chara val, io_flags flags,
	io_closure *on_err
)
{
#warning "europa_sendchar() needs to pay attention to it's flags!"
	if( stream )
	{
		if( stream->id != (uintptr_t)( &id ) )
		{
			return( -2 );
		}
		
		europa *eu = CALCADDR_FROMMEMBER( europa, header, stream );
		if( !( eu->dest ) )
		{
			return( -3 );
		}
		
		int res = fputc( val,  eu->src );
		if( res == EOF )
		{
			res = ferror( eu->src );
			if( res != 0 )
			{
				/* Yes, an error. */
				clearerr( eu->src );
				
				if( on_err )
				{
					if( !( on_err->func ) )
					{
						return( -4 );
					}
					on_err->func( on_err, stream,  res );
				}
				return( -5 );
			}
			/* This shouldn't ever be reached. */
			
			res = feof( eu->src );
			if( res != 0 )
			{
				/* A limit was reached? */
				
				if( on_err )
				{
					if( !( on_err->func ) )
					{
						return( -6 );
					}
					on_err->func( on_err, stream,  0 );
				}
				
				clearerr( eu->src );
				return( -7 );
			}
			/* How? Just how? */
			
			clearerr( eu->src );
			return( -8 );
		}
		
		return( 1 );
	}
	
	return( -1 );
}
static int europa_fetchchar
(
	io *stream,
	io_chara *dest, io_flags flags,
	io_closure *on_err
)
{
#warning "europa_fetchchar() needs to pay attention to it's flags!"
	if( stream && dest )
	{
		if( stream->id != (uintptr_t)( &id ) )
		{
			return( -2 );
		}
		
		europa *eu = CALCADDR_FROMMEMBER( europa, header, stream );
		if( !( eu->dest ) )
		{
			return( -3 );
		}
		
		int res = fgetc( eu->dest );
		if( res == EOF )
		{
			res = ferror( eu->dest );
			if( res != 0 )
			{
				/* Yes, an error. */
				clearerr( eu->dest );
				
				if( on_err )
				{
					if( !( on_err->func ) )
					{
						return( -4 );
					}
					on_err->func( on_err, stream,  res );
				}
				return( -5 );
			}
			/* This shouldn't ever be reached. */
			
			res = feof( eu->dest );
			if( res != 0 )
			{
				/* End of file. */
				
				if( on_err )
				{
					if( !( on_err->func ) )
					{
						return( -6 );
					}
					on_err->func( on_err, stream,  0 );
				}
				
				clearerr( eu->dest );
				return( -7 );
			}
			/* How? Just how? */
			
			clearerr( eu->dest );
			return( -8 );
		}
		
		*dest = (char)res;
		return( 1 );
	}
	
	return( -1 );
}
io* io_europa1()
{
	europa_stdio.header =
		(io)
		{
			(uintptr_t)( &id ),
			
			&europa_fetchchar,
			&europa_sendchar,
			
			&io_genericnull
		};
	europa_stdio.src = stdin;
	europa_stdio.dest = stdout;
	/*
	europa_stdio.size.width = ??? ;
	europa_stdio.size.height = ??? ;
	*/
	if( !( europa_stdio.cells ) )
	{
		europa_stdio.cells = (cellarr*){ 0 };
	}
	if( !( europa_stdio.title ) )
	{
		europa_stdio.title = (dynarr*){ 0 };
	}
	/*
	europa_stdio.orig_termios = (termios){};
	europa_stdio.flags = io_europa_null;
	europa_stdio.deathrattle = (char*)0;
	*/
	
		/* Need to update this bit. */
	if( !E.altscr && !E.no_altscr )
	{
		if( !mila_initterm_xterm() )
		{
			msgs_build_fatal
			(
				(msgs**)0,
				"\tXTerm initialization failed. If alt-screen in enabled, use ESC [?1049l.\n"
			);
			exit( 1 );
		}
	}
		/* Leandro Pereira */
	europa_updateWindowSize( &europa_stdio );
		/* Leandro Pereira */
	/* signal( SIGWINCH, handleSigWinCh ); */
#warning "The SIGWINCH handler needs to move into main() or related."
	
	return( &( europa_stdio.header ) );
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position */
/*  and return it. On error -1 is returned, on success the position of the */
/*  cursor is stored at *rows and *cols and 0 is returned. */
/*  5    0    5    0    5    0    5    0    5    0    5    0    5    0    5    0  */
int europa_getCursorPosition( europa *eu,  size_t *rows, size_t *cols )
{
	char buf[ 32 ];
	unsigned int i = 0;
	
	/* Report cursor location */
#define THOU_TERMCODES_4 "\x1b[6n"
	if( write( fileno( eu->dest ), THOU_TERMCODES_4, 4 ) != 4 )
	{
		return( -1 );
	}
	
	/* Read the response: ESC [ rows ; cols R */
	while( i < sizeof( buf ) - 1 )
	{
		if( read( fileno( eu->src ), buf + i, 1 ) != 1 )
		{
			break;
		}
		if( buf[ i ] == 'R')
		{
			break;
		}
		i++;
	}
	buf[ i ] = '\0';
	
	/* Parse it. */
	if( buf[ 0 ] != ESC || buf[ 1 ] != '[' )
	{
		return( -1 );
	}
	if( sscanf( buf + 2, "%zu;%zu", rows, cols ) != 2 )
	{
		return( -1 );
	}
	return( 0 );
}
	/* Try to get the number of columns in the current terminal. If the */
	/*  ioctl() * call fails the function will try to query the terminal */
	/*  itself. Returns 0 on success, -1 on error. */
int europa_getWindowSize
(
	europa *eu,
	
	size_t *rows, size_t *cols
)
{
	struct winsize ws;
	
	if
	(
		ioctl( /* 1 */ fileno( eu->dest ), TIOCGWINSZ, &ws ) == -1 ||
		ws.ws_col == 0
	)
	{
		/* ioctl() failed. Try to query the terminal directly. */
		
		size_t orig_row, orig_col;
		int retval;
		
		/* Get the initial position so we can restore it later. */
		retval = europa_getCursorPosition( eu,  &orig_row, &orig_col );
		if( retval == -1 )
		{
			goto failed;
		}
		
		/* Go to right/bottom margin and get position. */
		if( !mila_term_cursseek_finalchar( -1 ) )
		{
			goto failed;
		}
		retval = europa_getCursorPosition( eu,  rows, cols );
		if( retval == -1 )
		{
			goto failed;
		}
		
		/* Restore position. */
		mila_term_cursseek_setpos( 0, fileno( eu->dest ), orig_row, orig_col );
		return( 0 );
		
	} else {
		
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return( 0 );
	}
	
failed:
	return( -1 );
}
	/* Originally by Leandro Pereira */
void europa_updateWindowSize( europa *eu )
{
    if
	(
		europa_getWindowSize
		(
			eu,
			&( eu->size.height ),
			&( eu->size.width )
		) == -1
	)
	{
		msgs_build_fatal
		(
			(msgs**)0,
			"\n\teuropa_updateWindowSize() was unable to query the screen for "
				"size (columns / rows)\n"
		);
        exit( 1 );
    }
}

int io_deathrattle( io *stream,  char *deathrattle )
{
	if( stream && stream->id == (uintptr_t)&id )
	{
		( (europa*)stream )->deathrattle = deathrattle;
		
		return( 1 );
	}
	
	/* Use a shared one as a fall-back! */
	
	return( -1 );
}
