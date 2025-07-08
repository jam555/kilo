/* Thou -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * virttimer.c : A test program to figure out freaking timer signals
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <errno.h>



volatile int flag = 0;

void callback( int sig )
{
	flag = sig;
}

time_t inittimer()
{
	/* Setup a timer to be delivered via signal(). */
	
	int res, e;
	struct itimerval tsigtime;
	
	
	signal( SIGVTALRM, &callback );
	
	/* ITIMER_VIRTUAL == Only counts process's direct execution time. */
	res = getitimer( ITIMER_VIRTUAL, &tsigtime );
	if( res != 0 )
	{
		/* Pay attention to errno! Will be EFAULT or EINVAL */
		e = errno;
		
		printf( "\tmain_coro():getitimer( ITIMER_VIRTUAL ) failed with %d.\n", e );
		exit( 1 );
	}
	
	
	tsigtime.it_interval.tv_sec = 3;
	tsigtime.it_interval.tv_usec = 0;
	
	tsigtime.it_value = tsigtime.it_interval;
	
	
	res = setitimer( ITIMER_VIRTUAL, &tsigtime,  (struct itimerval*)0 );
	if( res != 0 )
	{
		/* Pay attention to errno! Will be EFAULT or EINVAL */
		e = errno;
		
		printf( "\tmain_coro():setitimer( ITIMER_VIRTUAL ) failed with %d.\n", e );
		exit( 1 );
	}
	
	
	return( time( (time_t*)0 ) );
}
int main( int argn, char *args[] )
{
	int res;
	time_t first_time, last_time, new_time;
	double dtime, etime;
	
	printf( "\nVirtual timer test entered.\n" );
	
	first_time = inittimer();
	last_time = first_time;
	
	
	while( difftime( new_time, first_time ) < 60 )
	{
		new_time = time( (time_t*)0 );
		dtime = difftime( new_time, last_time );
		etime = difftime( new_time, first_time );
		
		if( dtime >= 4 )
		{
			printf( "\n\tDirect loop time: %d.", (int)etime );
			last_time = new_time;
		}
		if( flag )
		{
			printf( "\n\tIndirect loop time: %d, sig: %d.", (int)etime, flag );
			flag = 0;
		}
	}
	
	
	printf( "\n\nVirtual timer test exiting.\n" );
}
