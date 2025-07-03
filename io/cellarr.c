/* Thou -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * cellarr.c : A dynamic-array system
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

#include "cellarr.h"

#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>


int cellarrOverwrite( cellarr **da,  const celltype *str, size_t len )
{
	if( da && str )
	{
		if( *da )
		{
			/* Truncate. */
			
			( *da )->real = 0;
			memset( ( *da )->arr,  0,  sizeof( celltype ) * ( *da )->total );
		}
		
		return( cellarrAppend( da,  str, len ) );
	}
	
	return( -1 );
}
int cellarrAppend( cellarr **da,  const celltype *str, size_t len )
{
	if( da && str )
	{
		if( !( *da ) )
		{
			*da = malloc( sizeof( cellarr ) + sizeof( celltype ) );
			if( !( *da ) )
			{
				return( -2 );
			}
			
			( *da )->real = 0;
			( *da )->total = 0;
			( *da )->arr =
				(celltype*)
				(
					(char*)( ( *da ) + 1 )
				);
		}
		
			/* Resize. */
		if( len > ( *da )->total || ( *da )->real > ( *da )->total - len )
		{
			if
			(
				SIZE_MAX == ( *da )->total ||
				( SIZE_MAX - ( ( *da )->total + 1 ) ) / 2 < len
			)
			{
				return( -3 );
			}
			
			celltype *mark = ( *da )->arr;
			cellarr *tmp =
				realloc
				(
					*da,
					
					sizeof( cellarr )
					+ sizeof( celltype ) *
						(
							( *da )->total
							+ ( len * 2 )
							+ 1 /* Add a cell of padding. */
						)
						
						/* We don't need a full cell of padding, so drop one */
						/*  byte. */
					- 1
				);
			if( !tmp )
			{
				return( -4 );
			}
				/* Update the available length. */
			tmp->total += ( len * 2 );
			
				/* Patch alignment/placement. */
			if( sizeof( celltype ) > 1 )
			{
				uintptr_t t2;
				
				tmp->arr = (celltype*)( tmp + 1 );
				t2 =
					(uintptr_t)( sizeof( celltype ) - 1 ) &
					(uintptr_t)( tmp->arr );
				tmp->arr =
					(celltype*)
					(
						(uintptr_t)( tmp->arr ) +
						( t2 ? ( sizeof( celltype ) - t2 ) : 0 )
					);
				
					/* Double-check, in case alignment has changed. */
				if( tmp->arr != (celltype*)( (uintptr_t)tmp + ( (uintptr_t)mark - (uintptr_t)( *da ) ) ) )
				{
						/* Adjust placement of the array. */
					memmove
					(
						tmp->arr,
						(celltype*)
						(
							(uintptr_t)tmp +
							( (uintptr_t)mark - (uintptr_t)( *da ) )
						),
						
						sizeof( celltype ) * ( tmp->real )
					);
				}
			}
			
			*da = tmp;
			/* The dyn-array is now large enough. */
		}
		
		memcpy( ( *da )->arr + ( *da )->real,  str, sizeof( celltype ) * len );
		( *da )->real += len;
		
		return( 1 );
	}
	
	return( -1 );
}
	/* Adds the provided prefix & postfix to EVERY line. */
int cellarrSuffixLines
(
	cellarr *src,
	cellarr **dest,
	
	const celltype *pre, size_t prelen,
	const celltype *post, size_t postlen
)
{
	if( src && dest && pre && post )
	{
		if( !( prelen || postlen ) )
		{
			return( 0 );
		}
		
		size_t src_off = 0, src_mark = 0;
		unsigned nl_masks = 0;
		int res = 0;
		
		while( src->real > src_off )
		{
#warning "cellarrSuffixLines() & should never receive VTab or FF!"
			if
			(
				(
					src->arr[ src_off ].val == 0xA || /* LF */
					src->arr[ src_off ].val == 0xB || /* VTab */
					src->arr[ src_off ].val == 0xC || /* FF */
					src->arr[ src_off ].val == 0xD /* CR */
				) &&
				(
					( !nl_masks ) ||
					(
						nl_masks &
						(
							1 << ( src->arr[ src_off ].val - 0xA )
						)
					)
				)
			)
			{
				/* We have a full line, emit it with prefix & postfix. */
				
				if( prelen )
				{
					res = cellarrAppend( dest,  pre, prelen );
					if( res < 0 )
					{
						return( -2 );
					}
				}
				if( src_off && src_off - 1 > src_mark )
				{
					res =
						cellarrAppend
						(
							dest,
							
							src->arr + src_mark,
							( src_off - 1 ) - src_mark
						);
					if( res < 0 )
					{
						return( -3 );
					}
				}
				if( postlen )
				{
					res = cellarrAppend( dest,  post, postlen );
					if( res < 0 )
					{
						return( -4 );
					}
				}
				/* ALWAYS emit whatever the newline seperately, so that they */
				/*  don't screw up the prefix or postfix. */
				res = cellarrAppend( dest,  src->arr + src_off, 1 );
				if( res < 0 )
				{
					return( -5 );
				}
				
				src_mark = src_off;
				if
				(
					nl_masks &
					(
						1 << ( src->arr[ src_off ].val - 0xA )
					)
				)
				{
					/* Repetition, so clear others. */
					
					nl_masks = 1 << ( src->arr[ src_off ].val - 0xA );
					
				} else {
					
					/* No repetition, so just mark. */
					
					nl_masks |= 1 << ( src->arr[ src_off ].val - 0xA );
				}
				
			} else if
			(
				src->arr[ src_off ].val == 0xA || /* LF */
				src->arr[ src_off ].val == 0xB || /* VTab */
				src->arr[ src_off ].val == 0xC || /* FF */
				src->arr[ src_off ].val == 0xD /* CR */
			)
			{
				/* ALWAYS emit whatever the newline is, so that they don't */
				/*  disappear in contexts where that matters. */
				res = cellarrAppend( dest,  src->arr + src_off, 1 );
				if( res < 0 )
				{
					return( -5 );
				}
				
				/* We NEVER want to leave this for the normal newline match, */
				/*  SO THAT it can't screw up the addition of prefixes & */
				/*  postfixes. */
				src_mark = src_off;
				
				if
				(
					nl_masks &
					(
						1 << ( src->arr[ src_off ].val - 0xA )
					)
				)
				{
					/* Repetition, so clear others. */
					
					nl_masks = 1 << ( src->arr[ src_off ].val - 0xA );
					
				} else {
					
					/* No repetition, so just mark. */
					
					nl_masks |= 1 << ( src->arr[ src_off ].val - 0xA );
				}
				
			} else {
				
				/* Not a newline, so clear any we've been tracking. */
				
				nl_masks = 0;
			}
			
			
			src_off += 1;
		}
		
			/* Handle any lingering line. */
		if( src->real && src->real - 1 > src_mark )
		{
			/* We have a final line, emit it with prefix & postfix. */
			
				/* Figure out if we need to skip an ending newline. */
			if
			(
				src->arr[ src_off - 1 ].val == 0xA || /* LF */
				src->arr[ src_off - 1 ].val == 0xB || /* VTab */
				src->arr[ src_off - 1 ].val == 0xC || /* FF */
				src->arr[ src_off - 1 ].val == 0xD /* CR */
			)
			{
				nl_masks = 1;
				
			} else {
				
				nl_masks = 0;
			}
			
			
			if( prelen )
			{
				res = cellarrAppend( dest,  pre, prelen );
				if( res < 0 )
				{
					return( -6 );
				}
			}
			if( src_off && src_off - 1 > src_mark )
			{
				res =
					cellarrAppend
					(
						dest,
						
						src->arr + src_mark,
						( src_off - nl_masks ) - src_mark
					);
				if( res < 0 )
				{
					return( -7 );
				}
			}
			if( postlen )
			{
				res = cellarrAppend( dest,  post, postlen );
				if( res < 0 )
				{
					return( -8 );
				}
			}
			if( nl_masks )
			{
				res = cellarrAppend( dest,  src->arr + ( src_off - 1 ), 1 );
				if( res < 0 )
				{
					return( -9 );
				}
			}
		}
		
		return( 1 );
	}
	
	return( -1 );
}
int cellarrFree( cellarr *ca )
{
	if( ca )
	{
		free( ca );
	}
	
	return( -1 );
}
