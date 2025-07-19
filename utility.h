/* Thou -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * utility.h
 *
 * Copyright (C) 2025 Jam555 <3349478+jam555@users.noreply.github.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  *  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UTILITY_H
# define UTILITY_H
	
	#include <limits.h>
	
	/* Calculate the address of a structure instance, based on the address */
	/*  of one of it's known member elements. */
	#define CALCADDR_FROMMEMBER( dest_type, member, refaddr ) \
		( (dest_type*)( \
			(char*)( refaddr ) - ( \
				(char*)( &( ( (dest_type*)0 )->member ) ) - \
				(char*)( (dest_type*)0 ) ) ) )
	
	
	/* !!!BEWARE!!! Optimizations can absolutely thrash all of this logic!!! */
	/* Note that much of this was sourced via: */
		/*
			https://stackoverflow.com/questions/4514572/
				c-question-off-t-and-other-signed-integer-
				types-minimum-and-maximum-values
		*/
	
		/* Calculate the maximum value of an unsigned integer. */
	#define CALCVAL_UNSIGNEDMAX( type ) ( ( (type)0 ) - 1 )
	/* The logic for *HIGHBIT() & *GENERICMAX() is apparently credit to */
	/*  Christian Biere via a nabble.com page that seems non-existent */
	/*  (couldn't find it on wayback machine). *HASSIGNED() and *GENERICMIN() */
	/*  are my own formulations. */
	#define CALCVAL_GENERICHIGHBIT( type ) \
		( (type)( ( \
			(uintmax_t)1 ) << \
			( CHAR_BIT * sizeof( type ) - \
				( 1 + ( (type)-1 < 1 ) ) ) ) )
	#define CALCVAL_GENERICMAX( type ) \
		( ( CALCVAL_GENERICHIGHBIT( type ) - 1 ) + \
			CALCVAL_GENERICHIGHBIT( type ) )
	#define CALCVAL_HASSIGNED( type ) \
		( ( (type)-1 < 1 ) ? 1 : 0 )
			/* *_GENERICMIN() is expected to work for sign-magnitude, */
			/*  one's-complement, AND two's-complement. Odder variants of */
			/*  signed probably WON'T work, but aren't particularly existent */
			/*  either. */
	#define CALCVAL_GENERICMIN( type ) \
		( CALCVAL_HASSIGNED( type ) ? ( 0 ) : \
			( -CALCVAL_GENERICMAX( type ) + \
				( ( -CALCVAL_GENERICMAX( type ) ) - 1 < 1 ) ? \
					( -1 ) : ( 0 ) ) )
	
		/* Originally from Hallvard B Furuseth in: */
			/* https://groups.google.com/g/comp.lang.c/c/NfedEFBFJ0k */
			/* Note: Usenet comp.lang.c, Dec 30 of 2003, "Portability question" */
		/* Number of bits in inttype_MAX, or: */
			/* in any ( (1<<b)-1 ) where: */
				/* 0 <= b < 3E+10 */
		/* Apparently covers up to "4-gigabyte integers". */
	#define CALCVAL_MAXtoBITS( m ) \
		( ( m ) / \
			( ( m ) % 0x3fffffffL + 1 ) / \
			0x3fffffffL % \
			0x3fffffffL * \
			30 + ( m ) % \
			0x3fffffffL / \
			( ( m ) % 31 + 1 ) / \
			31 % 31 * 5 + 4 - 12 / \
			( ( m ) % 31 + 3 ) )
		/* Note that the original Usenet posting contains a brief commentary on */
		/*  the algorithm. */
	
#endif
