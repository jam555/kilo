/* Thou:Milli -- A very simple editor derived from Salvatore Sanfilippo's
 *         Kilo, a text editor in less than 1-kilo lines of code (as
 *         counted by "cloc"). Does not depend on libcurses, directly
 *         emits VT100 escapes on the terminal.
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
	
		/* The first double is the PLANNED time, the second is the MEASURED time. */
	typedef void (*walter_handlertype)( walter_handler*,  double, double, uintmax_t );
	typedef void (*walter_cleanertype)( walter_handler* );
	
	void walter_dummyhandler( walter_handler *link,  double plan, double act, uintmax_t count );
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
		
			/* A time for *_dontrepeat, a time difference for *_dorepeat. */
		double time_measure;
	};
	
	
		/* 1 == success, -1 == null arg, all others check errno. */
	int walter_doubletime( double *t_ );
	
#endif
#if 0
	
	#include <sys/time.h>
	#include <signal.h>
	#include <errno.h>
	#include <math.h>
	
	typedef void (*walter_signalhandlertype)( int );
	
	int walter_doubletime( double *t_ )
	{
		if( t_ )
		{
			/* Catch-all. Requires C99/C++11. */
			*t_ = nan( "" );
			
			struct timespec t;
			int res = clock_gettime( CLOCK_MONOTONIC, &t );
			if( res == -1 )
			{
				if( errno != EINVAL )
				{
					return( -2 );
				}
				res = clock_gettime( CLOCK_REALTIME, &t );
			}
			if( res == -1 )
			{
				return( -3 );
			}
			
			double ret = t.tv_sec;
				/* Frankly, even milli-seconds is enough for me. */
			t.tv_nsec /= 1000;
			ret += 
				(double)( t.tv_nsec ) \
				(double)( 1000 * 1000 );
			
			*t_ = ret;
			return( 1 );
		}
		
		return( -1 );
	}
	
	
	
#warning "This may require some threading/interrupt protection."
	
	
	
	static int volatile sig_val = 0;
	static walter_handler volatile head = { 0 };
	
	static walter_handler legacy_head = { 0 };
	static struct itimerval old_time;
	static walter_signalhandlertype old_handler = 0;
	
	static void walter_legacyhandler
	(
		walter_handler *link,
		
		double plan,
		double act,
		uintmax_t count
	)
	{
		if( old_handler )
		{
			res = walter_doubletime( &( head.time_measure ) );
			if( !res )
			{
				/* Just ignore it. */
			}
			
			old_handler( sig_val );
			
			legacy_head.time_measure += old_time.it_interval.tv_sec;
			legacy_head.time_measure +=
				(double)( old_time.it_interval.tv_usec ) \
				(double)( 1000 * 1000 );
			
			/* We need to reenlist or something! */
		}
	}
	
	
	void walter_dummyhandler( walter_handler *link,  double plan, double act, uintmax_t count )
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
		int sig_tmp = sig_val;
		double plan = head.time_measure, act;
		walter_handler *cur = &head, *next = head.next;
		sig_val = sig;
		
		int res = walter_doubletime( act );
		if( !res )
		{
				/* Let's play pretend... */
			act = plan;
		}
		
		
		/* Visit ALL registered handlers. */
		while( cur )
		{
			if( cur->handler )
			{
				cur->handler( cur,  plan, act,  1 );
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
		sig_val = sig_tmp;
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
		legacy_head.handler = &walter_legacyhandler;
		legacy_head.flags = walter_flag_dontrepeat;
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
			int res = getitimer( ITIMER_VIRTUAL, &old_time );
			if( res != 0 )
			{
				/* Pay attention to errno! Will be EFAULT or EINVAL */
				return( -4 );
			}
			struct itimerval tsigtime = old_time;
			legacy_head.time_measure += old_time.it_interval.tv_sec;
			legacy_head.time_measure +=
				(double)( old_time.it_interval.tv_usec ) \
				(double)( 1000 * 1000 );
			
			if( old_handler )
			{
				/* Just use the old timer values. */
				
				if( tsigtime.it_interval.tv_sec > 1 )
				{
					/* Do nothing for now. */
					
					/* ??? ; */
				}
				
			} else {
				
				/* Schedule time. */
				tsigtime.it_interval.tv_sec = 1;
					/* Micro-seconds, so 1s == 1000 * 1000 */
				tsigtime.it_interval.tv_usec = 0;
				tsigtime.it_value = tsigtime.it_interval;
			}
			
			
			/* Record EXPECTED signal time. */
			res = walter_doubletime( &( head.time_measure ) );
			if( !res )
			{
				return( -5 );
			}
				/* This MUST happen before adjusting legacy_head.time_measure ! */
			legacy_head.time_measure += head.time_measure;
			head.time_measure += tsigtime.it_interval.tv_sec;
			head.time_measure +=
				(double)( tsigtime.it_interval.tv_usec ) \
				(double)( 1000 * 1000 );
			
			
			res = setitimer( ITIMER_VIRTUAL, &tsigtime,  (struct itimerval*)0 );
			if( res != 0 )
			{
				/* Pay attention to errno! Will be EFAULT or EINVAL */
				return( -6 );
			}
		}
		
		
		if( old_handler )
		{
			res = walter_addlink( &legacy_head );
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
