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
#include "coroutine/coro.h"
#include "statview.h"
#include "msgs.h"

#include <stddef.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>



struct statstate
{
	corohead *head;
	size_t off;
	time_t last_time;
	
	void *volatile data;
	void (*volatile func)( void );
};



	/* Baring strlen() and time(), this should be MORE than needed. */
static const size_t allocation = 8 * 1024;


static void statview_fetchmsg_inner( void );
static int inneryield( corohead *dest,  void *data, void (*func)( void ) );



static statstate* get_stats( corohead *dest )
{
	/* printf( "\nEntering get_stats()\n" ); fflush( stdout ); */
	
	if( dest )
	{
		statstate *stat = (statstate*)( dest->auxiliary );
		
		/* printf( "\tSuccess return == %p\n", (void*)stat ); fflush( stdout ); */
		return( stat );
	}
	
	/* printf( "\tFailure return.\n" ); fflush( stdout ); */
	return( 0 );
}
static int inneryield( corohead *dest,  void *data, void (*func)( void ) )
{
	/* printf( "\nEntering inneryield" ); fflush( stdout );
		printf( "( %p,  %p, %p )\n", (void*)dest, data, (void*)func ); fflush( stdout );  */
	
	if( dest )
	{
		/* printf( "\tget_stats( %p ) == ", (void*)dest ); fflush( stdout ); */
		statstate *stats = get_stats( dest );
			/* printf( "%p\n", (void*)stats ); fflush( stdout ); */
		if( stats )
		{
			/* printf( "\tdata == %p", data ); fflush( stdout ); */
			stats->data = data;
			/* printf( ", func == %p\n", (void*)func ); fflush( stdout ); */
			stats->func = func;
			
		} else {
			
			/* printf( "\tNo valid initializations.\n" ); fflush( stdout ); */
		}
		
			/* printf( "\tinneryield(): calling coyield().\n" ); fflush( stdout ); */
		coyield( dest );
			/* printf( "\n\tinneryield(): returned from coyield().\n" ); fflush( stdout ); */
		
		if
		(
			(statstate*)( coro_getaux() ) &&
			( (statstate*)( coro_getaux() ) )->func
		)
		{
			stats = (statstate*)( coro_getaux() );
			
			func = stats->func;
			data = stats->data;
			
			stats->func = 0;
				func();
			stats->data = 0;
		}
		
		return( 1 );
	}
	
	/* printf( "\tbad args. dest == %p, func == %p\n", (void*)dest, (void*)func ); fflush( stdout ); */
	return( -1 );
}


static void statview_fetchmsg_inner( void )
{
	/* Runs inside the coro. */
	/* printf( "\nstatview_fetchmsg_inner() entered.\n" ); fflush( stdout ); */
	
	msgs_view msgsv;
	
	/* printf( "\tGetting aux:" );
		fflush( stdout ); */
	statstate *stats = (statstate*)( coro_getaux() );
		/* printf( " %p\n", (void*)stats ); */
	statview_view *sv = (statview_view*)( stats->data );
	/* printf( "\tstatview_view: %p", (void*)sv );
		fflush( stdout ); */
	size_t usewid = sv->len;
	/* printf( "\tusewid: %zu", usewid );
		fflush( stdout ); */
	msgsv = msgs_peek();
	if( !msgsv.buf || !( msgsv.buf->b ) )
	{
		msgs_build_fatal( (msgs**)0,  "statview.c : statview_fetchmsg_inner() couldn't get a source buf." );
		exit( 1 );
	}
	size_t slen = strlen( msgsv.buf->b );
	/* printf( "\tstring length: %zu", slen );
		fflush( stdout ); */
	time_t t = time( (time_t*)0 );
	
	double dtime = difftime( t, stats->last_time );
	if( dtime * 10 >= MILA_MESSAGESLOTH )
	{
		stats->off += 1;
		stats->last_time = t;
	}
	if( stats->off >= slen )
	{
		stats->off = 0;
	}
	
#warning "Do something to perpetuate the flags to later stages!"
	if( slen <= usewid )
	{
		sv->start = msgsv.buf->b;
		sv->len = msgsv.buf->len;
		
	} else {
		
		sv->start = ( msgsv.buf->b ) + stats->off;
		
			slen -= stats->off;
		sv->len = ( slen > usewid ) ? usewid : slen ;
	}
	
	/* printf( "\tstatview_fetchmsg_inner() returning.\n" ); */
	/* Just fall back to the coro-main() loop, that'll handle the rest. */
}


static void statview_coromain( corohead *head, void *data )
{
	(void)data;
	
	/* printf( "\nEntering statview_coromain" ); fflush( stdout );
		printf
		(
			"( %p, %p )\n",
				(void*)head, (void*)data
		);
	printf
	(
		"\tCalculated footer: %p\n",
			(void*)( ( (uintptr_t)head ) - allocation )
	); */
	
	if( head )
	{
		/* printf( "\tThird printf.\n" ); */
		
		statstate stats;
		
		/* printf
		(
			"\tstatview_coromain():&stats == %p, head->aux == %d\n",
				(void*)&stats,
				(int)( head->auxiliary )
		); */
		
		stats.head = head;
		stats.last_time = time( (time_t*)0 );
		head->auxiliary = (uintptr_t)&stats;
		
		int loop = 1 /*CORO_WORKING*/ ;
		while( loop == 1 /*CORO_WORKING*/ )
		{
			/* printf( "\tcalling inneryield()\n" ); fflush( stdout ); */
			loop = inneryield( &main_fiber,  (void*)0, (void (*)())0 );
		}
	}
	/* printf( "\tExiting statview_coromain().\n" ); */
}
static int statview_conclude( corohead *head, uintptr_t aux )
{
	(void)aux;
	
	/* printf( "\nEntering statview_conclude()\n" ); fflush( stdout ); */
	
	if( head )
	{
		statstate *stats = (statstate*)( head->auxiliary );
			head->auxiliary = 0;
			stats->head = 0;
			stats->off = 0;
			stats->data = 0;
			stats->func = 0;
		
		return( 1 );
	}
	
	return( -1 );
	
	/* printf( "\nExiting statview_conclude()\n" ); fflush( stdout ); */
}



int statview_fetchmsg( statstate *stats, size_t usable_width,  statview_view *data )
{
	/* Runs outside the coro. */
	/* printf( "\nEntering statview_fetchmsg()\n" ); */
	
	if( stats && data )
	{
		/* printf( "\tstats && data.\n" ); */
		fflush( stdout );
		data->len = usable_width;
		
		inneryield( stats->head,  (void*)data, &statview_fetchmsg_inner );
		
		/* printf( "\tstatview_fetchmsg() successful exit.\n" ); */
		return( 1 );
	}
	
	/* printf( "\tstatview_fetchmsg() error exit.\n" ); */
	return( -1 );
}
statstate* statview_build()
{
	/* printf( "\nEntering statview_build()\n" ); fflush( stdout ); */
	
	corohead *head = 0;
	
	/* printf
	(
		"\nstatview(): calling cobuild(\n"
				"\t\t%zu,\n"
				"\t\t%p, %p, %d\n"
				"\t\t%p\n\n"
				"\t\t%p\n"
			")\n",
		
			allocation,
			(void*)0, (void*)&statview_coromain, 0,
			(void*)&statview_conclude,
			
			(void*)&head
	); */
	
	int res =
		cobuild
		(
			allocation,
			(void*)0, &statview_coromain, 0,
			&statview_conclude,
			
			&head
		);
	if( !res )
	{
		/* printf( "\t\t!res == true; returning 0\n" ); */
		return( 0 );
	}
	if( !head )
	{
		/* printf( "\t\t!head == true; returning 0\n" ); */
		return( 0 );
	}
	
	/* printf
	(
		"\t\t&head == %p, head == %p, head->aux == %d;\n\t\tCalling coyield( head ).\n",
			(void*)&head,
			(void*)head,
			(int)( head->auxiliary )
	); */
	coyield( head );
	/* printf
	(
		"\t\tstatview_build():coyield() returned.\n"
	);
		printf
		(
			"\t\t&head == %p, head == %p, &( head->aux ) == %p,",
				(void*)&head,
				(void*)head,
				(void*)&( head->auxiliary )
		);
		printf
		(
			" head->aux == %d\n",
				(unsigned)( head->auxiliary )
		);
	printf( "\t\tstatview_build() returning.\n" );
	fflush( stdout ); */
	return( (statstate*)( head->auxiliary ));
}
