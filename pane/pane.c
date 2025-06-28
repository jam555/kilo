/* Mila -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * pane.c : A display-source pane system
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


#include "pane.h"



extern inline int pane_on_resize( pane *pn,  size_t rows, size_t cols );
extern inline int pane_on_refresh( pane *pn );
extern inline int pane_on_orphan( pane *pn );



int pane_init
(
	pane *p,
		size_t start_size,
		const char *modename,
		const pane_calls *vtab
)
{
	if( p && modename )
	{
		static const char filler[ 16 ] =
			"    "
			"    "
			"    "
			"    ";
		
		int res;
		
		
		p->a = p->b = p->c = 0;
		p->dest = &( p->a );
		
		*( (const char**)&( p->modename ) ) = modename;
		*( (const pane_calls**)&( p->vtab ) ) = vtab;
		
		while( start_size )
		{
			if( start_size > 16 )
			{
				res = dynarrAppend( &( p->a ),  filler, 16 );
				if( !res )
				{
					return( -2 );
				}
				
				start_size -= 16;
				
			} else {
				
				res = dynarrAppend( &( p->a ),  filler, start_size );
				if( !res )
				{
					return( -3 );
				}
				
				start_size = 0;
			}
		}
		
		if( p->a )
		{
			res = dynarrAppend( &( p->b ),  p->a->arr, p->a->real );
			if( !res )
			{
				return( -4 );
			}
			
			res = dynarrAppend( &( p->c ),  p->a->arr, p->a->real );
			if( !res )
			{
				return( -5 );
			}
		}
		
		return( 1 );
	}
	
	return( -1 );
}

int modepane_init
(
	modepane *p,
		size_t start_size,
		const char *modename,
		const pane_calls *vtab
)
{
	if( p )
	{
		size_t i = 0;
		while( i < sizeof( p->panenotes ) )
		{
			p->panenotes[ i ] = 0;
			++i;
		}
		
		return
		(
			pane_init( &( p->p ),  start_size, modename, vtab )
		);
	}
	
	return( -1 );
}



int dummypane_on_resize( pane *pn,  size_t rows, size_t cols )
{
	(void)pn;
	(void)rows;
	(void)cols;
	
	return( 0 );
}
int dummypane_on_refresh( pane *pn )
{
	(void)pn;
	
	return( 0 );
}
int dummypane_on_orphan( pane *pn )
{
	(void)pn;
	
	return( 0 );
}
/*  5    0    5    0    5    0    5    0    5    0    5    0    5    0    5    0 */
