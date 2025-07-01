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
	
		/* Will often contain escapes. Eventually needs to be replaced with a */
		/*  uint32_t-based version. */
	dynarr
		*cells,
/*  5    0    5    0    5    0    5    0    5    0    5    0    5    0    5    0  */
			/* The terminal should HOPEFULLY have a distinct title bar. Add */
			/*  something to store it. */
		*title;
	
	struct termios orig_termios;
	
	io_europa_flags flags;
	
		/* The last-chanve choice for error messages. */
	char *deathrattle;
};



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
	europa_stdio.src = STDIN;
	europa_stdio.dest = STDOUT;
	/*
	europa_stdio.size.width = ??? ;
	europa_stdio.size.height = ??? ;
	*/
	if( !( europa_stdio.cells ) )
	{
		europa_stdio.cells = (dynarr*){};
	}
	if( !( europa_stdio.title ) )
	{
	europa_stdio.title = (dynarr*){};
	}
	/*
	europa_stdio.orig_termios = (termios){};
	europa_stdio.flags = (io_europa_flags){};
	*/
	europa_stdio.deathrattle = (char*)0;
	
	return( &europa_stdio );
}

int io_deathrattle( io *stream,  char *deathrattle )
{
	if( io && io->id == (uintptr_t)&id )
	{
		( (europa*)io )->deathrattle = deathrattle;
		
		return( 1 );
	}
	
	/* Use a shared one as a fall-back! */
	
	return( -1 );
}
