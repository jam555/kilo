/* Mila -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
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

#include "kilo.h"
#include "coroutine/coro.h"
#include "statview.h"
#include "msgs.h"
#include "utility.h"

#include <stddef.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>



static size_t debug_off;
static char *debug_text = "DEBUG debug DEBUG";
static time_t last_time = 0, test_time = 0;



	/* Baring strlen() and time(), this should be MORE than needed. */
static const size_t allocation = /* 8 */ 1024 * 1024;


static void statview_ontime( signal_links *sl, int sig );

int statview_updatetime( statstate *stats );
int statview_updatetime( statstate *stats )
{
	/* E.display_pointer = stats; */
	time_t t = time( (time_t*)0 );
	/* Using stats->last_time doesn't work. It runs once, and never again. */
	E.display_time = *localtime( &( t /* stats->last_time */ ) );
	
	
	if( 1 )
	{
		if( last_time > t || last_time + 3 <=  t )
		{
			/* THIS WORKS, so why isn't the text scrolling? */
			
			last_time = t;
			
			++debug_off;
			if( debug_off >= sizeof( debug_text ) )
			{
				debug_off = 0;
			}
		}
			/* Let's just turn this off for now. */
		if( 0 )
		{
			return( 1 );
		}
	}
	/* Fix this function so that we can deactivate the debugging code above. */
	
	
	
	
	
	
	/* E.display_test = ( !!stats ); */
	
	if( stats )
	{
		static time_t old_t = 0;
		time_t t = time( (time_t*)0 );
		time_t dtime = stats->last_time - t;
		/* double dtime = difftime( stats->last_time, t ); */
		if( dtime < 0.0 )
		{
			dtime = -dtime;
		}
		test_time = t;
		E.old_time = *localtime( &( dtime ) );
		
		/* E.display_test = dtime; */
		/* E.display_test = dtime * 1 >= MILA_MESSAGESLOTH; */
		if( dtime * 1 >= MILA_MESSAGESLOTH )
		{
			stats->off += 1;
			stats->last_time = t;
		}
		/* E.display_test = stats->off; */
		
			/* ... WHY does ->off suddenly jump? */
		/* E.display_test = stats->off; */
			/* ... WHY IS THIS CHANGING? */
		/* E.display_test = stats->last_size; */
		/*
		E.display_test =
			( &( stats ) == E.statusinterface ) ?
				1 : -1;
		*/
		/*
		E.vptr = stats;
		E.display_test = E.statusinterface;
		*/
			/* No, not "why is this changing?", instead "what are we touching?" */
		
		/* E.display_test = E.statusinterface->last_size; */
		if( stats->off >= stats->last_size )
		{
			/* Note: forcing this to run does nothing useful. */
			
				/* This never executes... why? */
			/* E.display_test = 1; */
			stats->off = 0;
			
				/* Cycle to the next message. */
			if( msgs_rotate() < 0 )
			{
				msgs_build_fatal
				(
					(msgs**)0,
						"\tmsgs_rotate() failed in statview_fetchmsg_inner().\n"
				);
				exit( 1 );
			}
			
			return( 1 );
		}
		
		return( 0 );
	}
	
	return( -1 );
}
/*
	!!!
		Let's break statview_fetchmsg_inner() into multiple pieces, according to purpose!
		Why doesn't stats->off change? Why does it stay as 0?
	!!!
*/
static void statview_fetchmsg_inner( void *v_ )
{
	/* Runs inside the coro. */
	/* printf( "\nstatview_fetchmsg_inner() entered.\n" ); */
	
	msgs_view msgsv = { 0 };
	
	statstate *stats = (statstate*)( coro_getaux() );
	statview_view *sv = (statview_view*)v_;
	size_t usewid = sv->len;
	int loop = 0;
	/* printf( "\tusewid: %zu", usewid ); */
	/* E.display_test = E.statusinterface->last_size; */
	
		/* Mark the progress for debugging. */
	/* E.display_test =
		1
		(int)( stats->off )
		; */
	
	afterloop:
	while( !( msgsv.buf ) )
	{
		msgsv = msgs_peek();
		if( !msgsv.buf || !( msgsv.buf->b ) )
		{
			msgs_build_fatal
			(
				(msgs**)0,
					"statview.c : statview_fetchmsg_inner() couldn't get a source buf."
			);
			exit( 1 );
		}
		if
		(
			( msgsv.msgsflags & msgs_flags_discard ) ==
			msgs_flags_discard
		)
		{
			int res = msgs_rotate();
			if( !res )
			{
				msgs_build_fatal
				(
					(msgs**)0,
						"statview.c : msgs_rotate() returned error: %d",
						res
				);
				exit( 1 );
			}
		}
	}
	/* E.display_test = E.statusinterface->last_size; */
	size_t slen = strlen( msgsv.buf->b );
		/* This makes the screen jiggle a lot? */
	/* E.display_test = E.statusinterface->last_size; */
	/* E.display_test = slen; */
		/* Ok, ->last_size is being modified anomylously... */
	/*
	if( slen != stats->last_size )
	{
		E.display_test = stats->last_size;
	}
	*/
	stats->last_size = slen;
	/* E.display_test = stats->last_size; */
	/* E.display_test = E.statusinterface->last_size; */
	
	/* Increment per time. */
	if( 0 /* !loop */ )
	{
		/* E.display_test = stats->last_size; */
		loop = statview_updatetime( stats );
		if( loop < 0 )
		{
			msgs_build_fatal
			(
				(msgs**)0,
					"statview.c : statview_updatetime() returned error: %d",
					loop
			);
			exit( 1 );
		}
		if( loop == 1 )
		{
			goto afterloop;
		}
	}
	/* E.display_test = stats->last_size; */
	if( !loop )
	{
		loop = 1;
		goto afterloop;
	}
	
	
	/* "Output" the effective string && flags. */
	if( slen <= usewid )
	{
		/* Never currently happens. */
		
		sv->start = msgsv.buf->b;
		sv->len = msgsv.buf->len;
		
	} else {
		
		/* Using debug_off instead of stats->off works. */
		
		sv->start = ( msgsv.buf->b ) + /* stats->off */ debug_off ;
		
			slen -= /* stats->off */ debug_off ;
		sv->len = ( slen > usewid ) ? usewid : slen ;
		/* E.display_test = test_time */ /* ( slen - usewid ) */ /* off */ ;
	}
	/* E.display_test = stats->last_size; */
	sv->msgsflags = msgsv.msgsflags;
	
	/* printf( "\tstatview_fetchmsg_inner() returning.\n" ); */
	/* Just fall back to the coro-main() loop, that'll handle the rest. */
}


static void statview_coromain( corohead *head, void *data )
{
	/* printf( "\nEntering statview_coromain" );
	printf
	(
		"\tCalculated footer: %p\n",
			(void*)( ( (uintptr_t)head ) - allocation )
	); */
	
	if( head && data )
	{
		
		statstate stats;
		corohead *tmp = 0;
		
		stats.head = head;
			/* Should this really be 0? Nope, we need to bootstrap. */
		stats.ret_dest = *( (corohead**)data );
		stats.last_time = time( (time_t*)0 );
		stats.last_size = 0;
		stats.time_hook = (signal_links){ 0 };
			stats.time_hook.handler = &statview_ontime;
		head->auxiliary = (uintptr_t)&stats;
		
		int loop = 1 /*CORO_WORKING*/ ;
		while( loop == 1 /*CORO_WORKING*/ )
		{
			/* E.display_test = stats.last_size; */
			/* printf( "\tcalling coyield()\n" ); fflush( stdout ); */
			tmp = (corohead*)stats.ret_dest;
			if( !tmp )
			{
				printf( "\n\ttmp was null in statview_coromain()!\n" );
				exit( 1 );
			}
			stats.ret_dest = 0;
				loop = coyield( tmp );
			tmp = 0;
			/* E.display_test = E.statusinterface->last_size; */
			/* stats.ret_dest has already been set elsewhere. */
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
			stats->ret_dest = 0;
			stats->off = 0;
			stats->data = 0;
			stats->func = 0;
		
		return( 1 );
	}
	
	return( -1 );
	
	/* printf( "\nExiting statview_conclude()\n" ); fflush( stdout ); */
}



static void statview_fetchmsg_inner_test( void *v_ )
{
	statview_view *data = (statview_view*)v_;
	
	
	E.vptr = (void*)"statview_fetchmsg() default text.";
	/* E.display_test = 2; */
	/* E.vptr = (void*)( debug_text + debug_off ); */
	/* E.display_test = ( (statstate*)( coro_getaux() ) )->last_size; */
	if( data )
	{
		if( strlen( debug_text ) > 12 )
		{
			data->len = strlen( debug_text ) - debug_off;
			data->len =
				( data->len <= 12 ) ?
					( data->len ) :
					12;
			data->start = debug_text + debug_off;
			/* E.vptr = (void*)( debug_text / * + debug_off * / ); */
			
		} else {
			
			data->len = strlen( debug_text );
			data->start = debug_text;
		}
		
		/* E.display_test = data->len; */
		/* E.display_test = 0; */
		/* E.display_test = stats->last_size; */
		E.vptr = (void*)( data->start );
	}
}
#warning "Remove all the debug cruft from statview_fetchmsg()."
int statview_fetchmsg( statstate *stats, size_t usable_width,  statview_view *data )
{
	/* Using stats->last_time doesn't work. It runs once, and never again. */
	/* E.display_time = *localtime( &( stats->last_time ) ); */
	/* E.display_test = stats->last_size; */
	if( 0 )
	{
		E.vptr = (void*)"statview_fetchmsg() default text.";
		/* E.display_test = 2; */
		/* E.vptr = (void*)( debug_text + debug_off ); */
		if( data )
		{
			/* Note: THIS TEST WORKS. */
			
			coyield2( stats->head, &( stats->ret_dest ),  (void*)data, &statview_fetchmsg_inner_test );
			return( 1 );
			
			
			
			if( strlen( debug_text ) > usable_width /* 8 */ )
			{
				data->len = strlen( debug_text ) - debug_off;
				data->len =
					( data->len <= usable_width ) ?
						( data->len ) :
						usable_width;
				data->start = debug_text + debug_off;
				/* E.vptr = (void*)( debug_text / * + debug_off * / ); */
				
			} else {
				
				data->len = strlen( debug_text );
				data->start = debug_text;
			}
			
			/* E.display_test = data->len; */
			/* E.display_test = !!stats; */
			E.vptr = (void*)( data->start );
			
			return( 1 );
		}
		
		return( -1 );
	}
	
	
	
	
	
	/* Runs outside the coro. */
	/* printf( "\nEntering statview_fetchmsg()\n" ); */
	
	if( stats && data )
	{
		if( 0 && stats->ret_dest != 0 )
		{
			/* Should probably add some error reporting here. */
			return( -2 );
		}
		
		/* printf( "\tstats && data.\n" );
		fflush( stdout ); */
		data->len = usable_width;
		/*
		stats.ret_dest = ;
		*/
		
		/* E.display_test = stats->last_size; */
		coyield2( stats->head, &( stats->ret_dest ),  (void*)data, &statview_fetchmsg_inner );
		/* E.display_test = stats->last_size; */
		
		/* printf( "\tstatview_fetchmsg() successful exit.\n" ); */
		return( 1 );
	}
	
	/* printf( "\tstatview_fetchmsg() error exit.\n" ); */
	return( -1 );
}
statstate* statview_build( signal_links **sl )
{
	/* printf( "\nEntering statview_build()\n" ); fflush( stdout ); */
	
	corohead *head = 0, *tmp;
	
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
			(void*)&tmp, &statview_coromain, 0,
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
	
	
	/* ... Don't we need to store straight into ->ret_dest? */
	/*  NO, because we hand a pointer to tmp to statview_coromain() via cobuild(). */
	coyield2( head, &tmp,  0, 0 );
	/* E.display_test = ( (statstate*)( head->auxiliary ) )->last_size; */
	
	if( sl )
	{
		*sl = &( ( (statstate*)( head->auxiliary ) )->time_hook );
	}
	/* printf( "\t\tstatview_build() returning.\n" );
	fflush( stdout ); */
	/* E.display_pointer = head->auxiliary; */
	/* E.vptr = head->auxiliary; */
	/* E.vptr = (void*)&( ( (statstate*)( head->auxiliary ) )->off ); */
	return( (statstate*)( head->auxiliary ) );
}
static void statview_ontime( signal_links *sl, int sig )
{
		/* We DO reliably reach here. */
	/* E.display_test = (int)time( 0 ); */
		/* ... and this is true from very early. */
	/* E.display_test = ( sig == SIGVTALRM ); */
		/* ... and so is this. */
	/* E.display_test = ( !!sl ); */
	
	if( sl && sig == SIGVTALRM )
	{
			/* This also gets reliable accessed. */
		/* E.display_test = (int)time( 0 ); */
		
		/* E.display_test = E.statusinterface->last_size; */
			/* This was counting the wrong direction, so stats pointed on */
			/*  the opposite side of sl from where it should have. */
		statstate *stats = CALCADDR_FROMMEMBER( statstate, time_hook, sl );
		/* E.display_test = stats->last_size; */
		/* E.display_test = E.statusinterface->last_size; */
		
			/* TODO: pay attention to the return type. */
		/*
		E.display_pointer = stats;
		E.display_test = E.display_pointer & 0xFFFFFFFF;
		*/
		/*
		E.display_test = (char*)( &( stats->time_hook ) ) - (char*)stats;
		E.display_pointer = &( stats->time_hook );
		*/
		/*
		E.vptr =
			&(
				( (statstate*)0 )[ 1 ].time_hook
			);
		E.display_pointer =
			CALCADDR_FROMMEMBER( statstate, time_hook, E.vptr );
		E.display_test =
			&(
				( (statstate*)(E.display_pointer) )->time_hook
			);
		*/
		
		/*
			(
				(dest_type*)
				(
					(char*)( refaddr ) +
					(
						(char*)
						(
							&( ( (dest_type*)0 )->member )
						) -
						(char*)( (dest_type*)0 )
					)
				)
			)
		*/
		/*
		E.vptr = &( ( ( (statstate*)0 )[ 1 ] ).time_hook );
		E.display_pointer = &( ( (statstate*)0 )[ 1 ] );
		E.display_test =
			( (char*)E.display_pointer ) +
			(
				( (char*)E.vptr ) - ( (char*)E.display_pointer )
			);
		*/
		
		/*
		E.vptr = &( ( ( (statstate*)0 )[ 1 ] ).time_hook );
		E.display_pointer = &( ( (statstate*)0 )[ 1 ] );
		E.display_test =
			( (char*)E.vptr ) - ( (char*)E.display_pointer );
		*/
		/* Calc display values. */
			/* The sign was wrong, it was adding when it needed to be subtracting! */
		/* E.display_pointer = stats */ /* ( (char*)sl ) - E.display_test */ ;
		/* E.vptr = E.statusinterface */ /* &( E.statusinterface->time_hook ) */ ;
		
			/* THIS IS PROVIDING THE WRONG ARGUMENT VALUE! */
		statview_updatetime( stats );
			/* This DOES show the bad value. */
		/* E.display_test = stats->last_size; */
		/* E.display_test = E.statusinterface->last_size; */
	}
}
