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

#include <stdlib.h>
#include <stdio.h>
#include "coro.h"


void altmain( corohead*, void* );
int altconclude( corohead*, uintptr_t );
corohead *altfiber;

const char *linepadding = "  ";



int bulk( void* );



void cotest_print( int depth, int val )
{
	while( depth )
	{
		puts( linepadding );
		--depth;
	}
	printf( "%d\n",  val );
}
void cotest_printaux()
{
	cotest_print( 2, coro_getaux() );
}

int main( int argn, char *args[] )
{
	printf( "\nCoroutine testing beginning.\n" );
	
	cocontext( (void*)0, &bulk );
	
	printf( "%sCoroutine testing exiting.\n", linepadding );
	return( 1 );
}



int bulk( void* )
{
	printf( "%s%sAllocating alternate coroutine.\n", linepadding,linepadding );
	if
	(
		cobuild
		(
			1024 /* 1k */ * 1024 /* 1M */,
			(void*)0, &altmain, 0,
			&altconclude,
			
			&altfiber
		)
	)
	{
		exit( 1 );
	}
	
	cotest_print( 4, 1 );
	cotest_print( 4, 3 );
	altfiber->auxiliary = 1;
	if( !coyield( altfiber ) )
	{
		printf( "%s%sYield to alternate coroutine failed (1).\n", linepadding,linepadding );
		exit( 1 );
	}
	
	cotest_print( 4, 5 );
	cotest_print( 4, 7 );
	altfiber->auxiliary= 2;
	if( !coyield( altfiber ) )
	{
		printf( "%s%sYield to alternate coroutine failed (1).\n", linepadding,linepadding );
		exit( 1 );
	}
	
	cotest_print( 4, 9 );
	cotest_print( 4, 11 );
	altfiber->auxiliary= 3;
	if( !coyield( altfiber ) )
	{
		printf( "%s%sYield to alternate coroutine failed (1).\n", linepadding,linepadding );
		exit( 1 );
	}
	
	cotest_print( 4, 13 );
	cotest_print( 4, 15 );
	altfiber->auxiliary= 4;
	if( !coyield( altfiber ) )
	{
		printf( "%s%sYield to alternate coroutine failed (1).\n", linepadding,linepadding );
		exit( 1 );
	}
	
	cotest_print( 4, 17 );
	cotest_print( 4, 19 );
	
	return( 1 );
}

void altmain( corohead *ch, void *v )
{
	(void)ch;
	(void)v;
	
	printf( "%s%saltmain() entered (1).\n" );
	cotest_printaux();
	cotest_print( 4, 2 );
	cotest_print( 4, 4 );
	if( !coyield( &main_fiber ) )
	{
		printf( "%s%sYield to main coroutine failed (1).\n", linepadding,linepadding );
		exit( 1 );
	}
	
	printf( "%s%saltmain() entered (2).\n" );
	cotest_printaux();
	cotest_print( 4, 6 );
	cotest_print( 4, 8 );
	if( !coyield( &main_fiber ) )
	{
		printf( "%s%sYield to main coroutine failed (2).\n", linepadding,linepadding );
		exit( 1 );
	}
	
	printf( "%s%saltmain() entered (3).\n" );
	cotest_printaux();
	cotest_print( 4, 10 );
	cotest_print( 4, 12 );
	if( !coyield( &main_fiber ) )
	{
		printf( "%s%sYield to main coroutine failed (3).\n", linepadding,linepadding );
		exit( 1 );
	}
	
	printf( "%s%saltmain() entered (4).\n" );
	cotest_printaux();
	cotest_print( 4, 14 );
		void cocollapse
		(
			corohead *head,
			corobody *body,
			int (*conclude)( corohead*, uintptr_t )
		);
	/* This SHOULDN'T get executed. */
	cotest_print( 4, 16 );
	printf( "%s%scotest_print() after cocollapse() somehow ran!.\n", linepadding,linepadding );
	if( !coyield( &main_fiber ) )
	{
		printf( "%s%sYield to main coroutine failed (4).\n", linepadding,linepadding );
		exit( 1 );
	}
}

int altconclude( corohead *ch, uintptr_t aux )
{
	(void)ch;
	
	cotest_print( 4, aux );
}
