/* Mila -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * coro.h : A coroutine system
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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "coro.h"


void altmain( corohead*, void* );
int altconclude( corohead*, uintptr_t );
corohead *altfiber;
size_t stackspec = 1024 /* 1k */ * 1024 /* 1M */;

const char *linepadding = "  \0";



int bulk( void* );



void cotest_print( int depth, int val )
{
	while( depth )
	{
		printf( linepadding );
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
		printf( "%sbulk() prints odd numbers, and increments the auxiliary value for altmain().\n",  linepadding );
			printf( "%s%sWhen yielding, bulk() includes the address of it's yield target in the print.\n",  linepadding,linepadding );
		printf( "%saltmain() prints it's aux value, then prints even numbers.\n",  linepadding );
	
	cocontext( (void*)0, &bulk );
	
	printf( "\n%sCoroutine testing exiting.\n", linepadding );
	
	fflush( stdout );
	return( 1 );
}



int bulk( void* )
{
	int tmp;
	
	printf( "\n%sbulk() allocating alternate coroutine.\n", linepadding,linepadding );
	tmp = cobuild
		(
			stackspec,
			(void*)0, &altmain, 0,
			&altconclude,
			
			&altfiber
		);
	if( !tmp )
	{
		printf( "%s%scobuild() == %d\n", linepadding,linepadding,  tmp );
		exit( 2 );
	}
	
	cotest_print( 4, 1 );
	cotest_print( 4, 3 );
	altfiber->auxiliary = 1;
	printf( "%s%sbulk(): yielding( %p ) (1).\n", linepadding,linepadding,  altfiber );
	if( !coyield( altfiber ) )
	{
		printf( "%s%sYield to alternate coroutine failed (1).\n", linepadding,linepadding );
		exit( 3 );
	}
	
	printf( "\n%s%sbulk():yield returned (1->2).\n", linepadding,linepadding );
	cotest_print( 4, 5 );
	cotest_print( 4, 7 );
	altfiber->auxiliary= 2;
	printf( "%s%sbulk(): yielding( %p ) (2).\n", linepadding,linepadding,  altfiber );
	if( !coyield( altfiber ) )
	{
		printf( "%s%sYield to alternate coroutine failed (1).\n", linepadding,linepadding );
		exit( 4 );
	}
	
	printf( "\n%s%sbulk():yield returned (2->3).\n", linepadding,linepadding );
	cotest_print( 4, 9 );
	cotest_print( 4, 11 );
	altfiber->auxiliary= 3;
	printf( "%s%sbulk(): yielding( %p ) (3).\n", linepadding,linepadding,  altfiber );
	if( !coyield( altfiber ) )
	{
		printf( "%s%sYield to alternate coroutine failed (1).\n", linepadding,linepadding );
		exit( 5 );
	}
	
	printf( "\n%s%sbulk():yield returned (3->4).\n", linepadding,linepadding );
	cotest_print( 4, 13 );
	cotest_print( 4, 15 );
	altfiber->auxiliary= 4;
	printf( "%s%sbulk(): yielding( %p ) (4).\n", linepadding,linepadding,  altfiber );
	if( !coyield( altfiber ) )
	{
		printf( "%s%sYield to alternate coroutine failed (1).\n", linepadding,linepadding );
		exit( 6 );
	}
	printf( "\n%s%sbulk():yield returned (4).\n", linepadding,linepadding );
	
	cotest_print( 4, 17 );
	cotest_print( 4, 19 );
	
	return( 1 );
}

void altmain( corohead *ch, void *v )
{
	(void)ch;
	(void)v;
	
	printf( "\n%saltmain( %p, %p ) entered (1).\n", linepadding,  (void*)ch, v );
	cotest_printaux();
	cotest_print( 4, 2 );
	cotest_print( 4, 4 );
	printf( "%s%saltmain(): yielding (1).\n", linepadding,linepadding );
	if( !coyield( &main_fiber ) )
	{
		printf( "%s%sYield to main coroutine failed (1).\n", linepadding,linepadding );
		exit( 7 );
	}
	
	printf( "\n%s%saltmain():yield returned (1->2).\n", linepadding,linepadding );
	cotest_printaux();
	cotest_print( 4, 6 );
	cotest_print( 4, 8 );
	printf( "%s%saltmain(): yielding (2).\n", linepadding,linepadding );
	if( !coyield( &main_fiber ) )
	{
		printf( "%s%sYield to main coroutine failed (2).\n", linepadding,linepadding );
		exit( 8 );
	}
	
	printf( "\n%s%saltmain():yield returned (2->3).\n", linepadding,linepadding );
	cotest_printaux();
	cotest_print( 4, 10 );
	cotest_print( 4, 12 );
	printf( "%s%saltmain(): yielding (3).\n", linepadding,linepadding );
	if( !coyield( &main_fiber ) )
	{
		printf( "%s%sYield to main coroutine failed (3).\n", linepadding,linepadding );
		exit( 9 );
	}
	
	printf( "\n%s%saltmain():yield returned (3->4).\n", linepadding,linepadding );
	cotest_printaux();
	cotest_print( 4, 14 );
	
	printf( "\n%s%saltmain(): running cocollapse() on self.", linepadding,linepadding );
		/* The following printf()s exist to track down the seg-fault in cocollapse(). */
	/*
		printf
		(
			"\n%s%s%s head: %p, body(a): %p, body(b): %p, conclude(): %p",
				linepadding,linepadding,linepadding,
				(void*)ch,
				(void*)( ch->lastbyte_a ), (void*)( ch->lastbyte_b ),
				(void*)( ch->conclude )
		);
		printf
		(
			"\n%s%s%s stack supplement: %x, stackspec: %x, head - stackspec: %p\n",
				linepadding,linepadding,linepadding,
				(int)
				(
					sizeof( corohead ) * 2 +
					sizeof( dummyframe ) * 2 +
					sizeof( corobody ) +
					128
				),
				(int)stackspec,
				(void*)( ( (uintptr_t)ch ) - stackspec )
		);
	*/
	cocollapse( ch, ch->lastbyte_b,  ch->conclude );
	
	/* This SHOULDN'T get executed. */
	cotest_print( 4, 16 );
	printf( "%s%scotest_print() after cocollapse() somehow ran!.\n", linepadding,linepadding );
	if( !coyield( &main_fiber ) )
	{
		printf( "%s%sYield to main coroutine failed (4).\n", linepadding,linepadding );
		exit( 10 );
	}
}

int altconclude( corohead *ch, uintptr_t aux )
{
	(void)ch;
	
	cotest_print( 4, aux );
}
