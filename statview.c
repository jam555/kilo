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

#include <stddef.h>
#include <time.h>



struct statstate
{
	corohead *head;
	size_t off;
	time_t last_time;
	
	volatile void *data;
	volatile void (*func)();
};
static void statview_fetchmsg_inner();



static statstate* get_stats( corohead *dest )
{
	if( dest )
	{
		return
		(
			(statstate*)
			(
				dest->auxiliary
			)
		);
	}
	
	return( 0 );
}
static int inneryield( corohead *dest,  void *data, void (*func)() )
{
	if( dest && func )
	{
		statstate *stats = get_stats( dest );
			stats->data = data;
			stats->func = func;
		
		coyield( dest );
		
		if( stats->func )
		{
			func = stats->func;
			data = stats->data;
			
			stats->func = 0;
				func();
			stats->data = 0;
		}
		
		return( 1 );
	}
	
	return( -1 );
}


static void statview_fetchmsg_inner()
{
	/* Runs inside the coro. */
	
	
	statstate *stats = (statstate*)( coro_getaux() );
	statview_view *sv = (statview_view*)( stats->data );
	size_t usewid = sv->len;
	size_t strlen = strlen( E.statusmsg );
	time_t t = time( (time_t*)0 );
	
	double dtime = difftime( t, stats->last_time );
	if( dtime * 10 >= MILA_MESSAGESLOTH )
	{
		stats->off += 1;
		stats->last_time = t;
	}
	if( stats->off >= strlen )
	{
		stats->off = 0;
	}
	
	if( strlen <= usewid )
	{
		sv->start = E.statusmsg;
		sv->len = strlen;
		
	} else {
		
		sv->start = E.statusmsg + stats->off;
		
			strlen -= stats->off;
		sv->len = ( strlen > usewid ) ? usewid : strlen ;
	}
	
	/* Just fall back to the coro-main() loop, that'll handle the rest. */
}


static void statview_coromain( corohead *head, void *data )
{
	if( head )
	{
		statstate stats = { 0 };
			stats.head = head;
			stats.last_time = time( (time_t*)0 );
			head->auxiliary = (uintptr_t)&stats;
		
		int loop = CORO_WORKING;
		while( loop == CORO_WORKING )
		{
			loop = yield( &main_fiber );
		}
	}
}
static int statview_conclude( corohead *head, uintptr_t aux )
{
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
}



int statview_fetchmsg( statstate *stats, size_t usable_width,  statview_view *data )
{
	/* Runs outside the coro. */
	
	if( stats && data )
	{
		data->len = usable_width;
		
		inneryield( stats->head,  (void*)data, &statview_fetchmsg_inner );
		
		return( 1 );
	}
	
	return( -1 );
}
statstate* statview_build()
{
	corohead *head = 0;
	int res =
		cobuild
		(
				/* Baring strlen() and time(), this should be MORE than needed. */
			1024 * 8,
			(void*)0, &statview_coromain, 0,
			&statview_conclude,
			
			&head
		);
	if( !res || !head )
	{
		return( 0 );
	}
	
	return( (statstate*)( head->auxiliary ));
}
