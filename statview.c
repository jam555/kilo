/* Thou -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
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



struct statstate
{
	corohead *head;
	corohead *volatile ret_dest;
	size_t volatile off;
	time_t volatile last_time;
	
	void *volatile data;
	void (*volatile func)( void );
	
		/* Both of these are for signal-handler based time tracking. */
	size_t volatile last_size;
#warning "Adding a single extra element BEFORE HERE causes a segfault, but adding two doesn't!"
	signal_links time_hook;
};



	/* Baring strlen() and time(), this should be MORE than needed. */
static const size_t allocation = /* 8 */ 1024 * 1024;


static void statview_ontime( signal_links *sl, int sig );

int statview_updatetime( statstate *stats );
int statview_updatetime( statstate *stats )
{
	/* E.display_test = ( !!stats ); */
	
	if( stats )
	{
#warning "Start using nanotime() from nanotime.h/.c"
		time_t t = time( (time_t*)0 );
		double dtime = difftime( t, stats->last_time );
		if( dtime < 0.0 )
		{
			dtime = -dtime;
		}
		
		if( dtime * 1 >= MILA_MESSAGESLOTH )
		{
			stats->off += 1;
			stats->last_time = t;
		}
		
			/* ... WHY IS THIS CHANGING? */
		/* E.display_test = stats->last_size; */
			/* No, not "why is this changing?", instead "what are we touching?" */
			/* "The wrong side of stats->time_hook" is what we were touching. */
		
		if( stats->off >= stats->last_size )
		{
			/* Note: forcing this to run does nothing useful. */
			
				/* This never executes... why? */
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
			msgsv = (msgs_view){ 0 };
		}
	}
	size_t slen = strlen( msgsv.buf->b );
	stats->last_size = slen;
	
	/* Increment per time. */
	if( 0 /* !loop */ )
	{
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
	if( !loop )
	{
		loop = 1;
		goto afterloop;
	}
	
	
	/* "Output" the effective string. */
	if( slen <= usewid )
	{
		/* Never currently happens. */
		
		sv->start = msgsv.buf->b;
		sv->len = msgsv.buf->len;
		
	} else {
		
		sv->start = ( msgsv.buf->b ) + stats->off;
		
			slen -= stats->off;
		sv->len = ( slen > usewid ) ? usewid : slen ;
	}
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
			tmp = (corohead*)stats.ret_dest;
			if( !tmp )
			{
				printf( "\n\ttmp was null in statview_coromain()!\n" );
				exit( 1 );
			}
			stats.ret_dest = 0;
				loop = coyield( tmp );
			tmp = 0;
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



int statview_fetchmsg( statstate *stats, size_t usable_width,  statview_view *data )
{
	/* Runs outside the coro. */
	/* printf( "\nEntering statview_fetchmsg()\n" ); */
	
	if( stats && data )
	{
		if( 0 && stats->ret_dest != 0 )
		{
			/* Should probably add some error reporting here. */
			return( -2 );
		}
		
		data->len = usable_width;
		
		coyield2( stats->head, &( stats->ret_dest ),  (void*)data, &statview_fetchmsg_inner );
		
		if( 0 )
		{
			E.display_test = data->len;
			E.display_pointer = data->start;
			E.display_text = data->start;
		}
		
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
	
	if( sl )
	{
		*sl = &( ( (statstate*)( head->auxiliary ) )->time_hook );
	}
	/* printf( "\t\tstatview_build() returning.\n" );
	fflush( stdout ); */
	return( (statstate*)( head->auxiliary ) );
}
static void statview_ontime( signal_links *sl, int sig )
{
	if( sl && sig == SIGVTALRM )
	{
			/* This was counting the wrong direction, so stats pointed on */
			/*  the opposite side of sl from where it should have. */
		statstate *stats = CALCADDR_FROMMEMBER( statstate, time_hook, sl );
		
		statview_updatetime( stats );
	}
}
