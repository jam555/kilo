/* Thou -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * walter.h - A timer facility, named after *nix Cron via Walter Cronkite
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

#ifndef WALTER_WALTER_H
# define WALTER_WALTER_H
	
	#include <time.h>
	#include <stdint.h>
	
	
	typedef enum
	{
		walter_flag_head = -2,
		walter_flag_invalid = -1,
		walter_flag_null = 0,
		
		walter_flag_repeatmask = 3,
		walter_flag_dontrepeat = 1,
		walter_flag_dorepeat = 2,
		walter_flag_illegalrepeat = 3,,
		
		walter_flag__PASTEND
		
	} walter_flags;
	
	
	typedef struct walter_handler walter_handler;
	
		/* The first time is the PLANNED time, the second is the MEASURED time. */
	typedef void (*walter_handlertype)( walter_handler*,  time_t, time_t, uintmax_t );
	typedef void (*walter_cleanertype)( walter_handler* );
	
	void walter_dummyhandler( walter_handler *link,  time_t plan, time_t act, uintmax_t count );
	void walter_dummycleaner( walter_handler *link );
	
	/* These initialize & deinitialize the walter system. Note that they */
	/*  may/will interfere with/depend on with signals: walter will chain */
	/*  to the previous handler, BUT NOT ALL SYSTEMS WILL, so beware. */
	int walter_init( walter_handlertype );
	int walter_deinit();
	
		/* Only SIGWINCH & SIGVTALRM are currently supported. */
	int walter_addlink( walter_handler *link );
	int walter_droplink( walter_handler *link );
	
	struct walter_handler
	{
		walter_handler *prev, *next;
		
		walter_handlertype handler;
		walter_cleanertype destructor;
		walter_flags flags;
		time_t
				/* A time for *_dontrepeat, a time difference for *_dorepeat. */
			time_measure;
	};
	
#endif
#if 0
	
	#include <sys/time.h>
	#include <signal.h>
	
	typedef void (*walter_signalhandlertype)( int );
	
	
#warning "This may require some threading/interrupt protection."
	
	
	
	static walter_handler volatile head = { 0 };
	static walter_signalhandlertype old_handler = 0;
	static struct itimerval tsigtime;
	
	void walter_dummyhandler( walter_handler *link,  time_t plan, time_t act, uintmax_t count )
	{
		(void)link;
		(void)plan;
		(void)act;
		(void)count;
		
		/* This is just a dummy, so do nothing. */
	}
	void walter_dummycleaner( walter_handler *link );
	
	
	
	/* Specific implementations. */
	static void walter_timesig( int sig )
	{
		time_t
			plan = head.time_measure,
			act = time( (time_t*)0 );
		walter_handler *cur = &head, *next = head.next;
		
		
		/* Visit ALL registered handlers. */
		while( cur )
		{
			if( cur->handler )
			{
				cur->handler( cur, plan, act, 1 );
			}
			
			cur = next;
			next = cur->next;
		}
		
		
		/* Loop in our predecessor. */
		if( old_handler )
		{
			old_handler( sig );
		}
			/* Maintain the handler chain. Note that this is required */
			/*  REGARDLESS, as the registered signal handler is always */
			/*  unregistered by the signal() system when called anyways. */
		old_handler = signal( sig, &walter_timesig );
	}
	int walter_init( walter_handlertype pulse, walter_cleanertype conclude )
	{
		if
		(
			head.prev ||
			head.next ||
			head.flags != walter_flag_null ||
			old_handler
		)
		{
			return( -2 );
		}
		
		
		/* Basic head initialization. */
		head.handler = pulse;
		head.destructor = conclude;
		head.flags = walter_flag_head;
		/* head.time_measure will be changed in a bit. */
		
		
#warning "Switch to sigaction() on at least some platforms."
		old_handler = signal( SIGVTALRM, &walter_timesig );
		if( old_handler == SIG_ERR )
		{
			walter_deinit();
			
			return( -3 );
		}
		
		{
			/* ITIMER_VIRTUAL == Only counts process's direct execution time. */
			int res = getitimer( ITIMER_VIRTUAL, &tsigtime );
			if( res != 0 )
			{
				/* Pay attention to errno! Will be EFAULT or EINVAL */
				return( -4 );
			}
			
			if( old_handler )
			{
	#warning "Check to see if the old timer's values are compatible with our own."
				/* ??? ; */
			}
			
			tsigtime.it_interval.tv_sec = 1;
				/* Micro-seconds, so 1s == 1000 * 1000 */
			tsigtime.it_interval.tv_usec = 0;
			tsigtime.it_value = tsigtime.it_interval;
			
			head.time_measure = time( (time_t*)0 );
				/* FIX THIS! time_t might not measure seconds! */
			head.time_measure += tsigtime.it_interval.tv_sec;
			res = setitimer( ITIMER_VIRTUAL, &tsigtime,  (struct itimerval*)0 );
			if( res != 0 )
			{
				/* Pay attention to errno! Will be EFAULT or EINVAL */
				return( -5 );
			}
		}
		
		return( 1 );
	}
	int walter_deinit()
	{
#warning "Improve this testing."
		if
		(
			old_handler != 0 ||
			( old_handler = signal( SIGVTALRM, &walter_timesig ) )
		)
		{
				/* Handled failed *_init(). */
			if( old_handler == SIG_ERR )
			{
				head.handler = 0;
				head.destructor = 0;
				head.flags = walter_flag_null;
				
				return( -2 );
			}
				/* Handle non-clean *_deinit() call. */
			if( old_handler != &walter_timesig )
			{
				signal( SIGVTALRM, old_handler );
				return( -3 );
			}
			
			
			walter_handler *next = head.next;
			int res;
			
			signal( SIGVTALRM, old_handler );
			old_handler = 0;
			
			while( next )
			{
				res = walter_droplink( walter_handler *link );
				if( res < 0 )
				{
					return( -3 );
				}
			}
			
			if( head.destructor )
			{
				( head.destructor )( (walter_handler*)0 );
			}
			
			return( 1 );
		}
		
		/* Nothing to do. */
		return( 0 );
	}
	
	
	int walter_addlink( walter_handler *link )
	{
		if( link )
		{
			link->next = head.next;
			link->prev = &head;
			if( link->next )
			{
				link->next->prev = link;
			}
			head.next = link;
			
			return( 1 );
		}
		
		return( -1 );
	}
	int walter_droplink( walter_handler *link )
	{
		if( link )
		{
			if( link == &head )
			{
				return( -2 );
			}
			if( !( link->prev ) )
			{
				return( -3 );
			}
			
			link->prev->next = link->next;
			if( link->next )
			{
				link->prev->next = link->next;
			}
			link->prev = 0;
			link->next = 0;
			
			if( link->destructor )
			{
				( link->destructor )( link );
			}
			
			return( 1 );
		}
		
		return( -1 );
	}
	
#endif
