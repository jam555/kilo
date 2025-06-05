/* Mila -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * msgs.h : A message-handling system
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
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "msgs.h"



static msgs_queue messages, errors, fatalities;
static msgs_queue
	messages = { 0, &( messages.head ) },
	errors = { 0, &( errors.head ) },
	fatalities = { 0, &( fatalities.head ) };



int msgs_mark_discard( msgs *msg )
{
	if( msg )
	{
		msg->msgsflags |= (unsigned char)( msgs_flags_discard );
		
		return( 1 );
	}
	
	return( -1 );
}
	/* Note that you should just use *_alert() in many cases. */
int msgs_mark_unimportant( msgs *msg )
{
	if( msg )
	{
		msg->msgsflags &= (unsigned char)( ~msgs_flags_important );
		
		return( 1 );
	}
	
	return( -1 );
}
int msgs_mark_important( msgs *msg )
{
	if( msg )
	{
		msg->msgsflags |= (unsigned char)( msgs_flags_important );
		
		return( 1 );
	}
	
	return( -1 );
}
int msgs_mark_plainlife( msgs *msg )
{
	if( msg )
	{
		msg->msgsflags &= (unsigned char)( ~msgs_flags_timecalced );
		/* We don't HAVE a "plain" valoue, we just leave it blank. */
		
		return( 1 );
	}
	
	return( -1 );
}
int msgs_mark_hardlife( msgs *msg )
{
	if( msg )
	{
		msg->msgsflags &= (unsigned char)( ~msgs_flags_timecalced );
		msg->msgsflags |= msgs_flags_hardwired;
		
		return( 1 );
	}
	
	return( -1 );
}
int msgs_mark_timelife( msgs *msg, time_t relative )
{
	if( msg )
	{
		msg->msgsflags &= (unsigned char)( ~msgs_flags_timecalced );
		msg->msgsflags |= msgs_flags_timepending;
		msg->reftime = relative;
		
		return( 1 );
	}
	
	return( -1 );
}



int msgs_queue_init( msgs_queue *queue )
{
	if( queue )
	{
		queue->head = 0;
		queue->tail = &( queue->head );
		
		return( 1 );
	}
	
	return( -1 );
}
int msgs_queue_pop( msgs_queue *queue,  msgs **recip )
{
	if( queue && recip )
	{
		*recip = queue->head;
		
		if( *recip )
		{
			/* Delink. */
			
			queue->head = ( *recip )->next;
			( *recip )->next = 0;
			
			if( &( ( *recip )->next ) == queue->tail )
			{
				/* Reset. */
				
				queue->tail = &( queue->head );
			}
			
			return( 1 );
		}
		
		return( 0 );
	}
	
	return( -1 );
}
int msgs_queue_append( msgs_queue *queue, msgs *val )
{
	if( queue )
	{
		if( !val )
		{
			return( 0 );
		}
		
		*( queue->tail ) = val;
		queue->tail = &( val->next );
		
		return( 1 );
	}
	
	return( -1 );
}
int msgs_queue_rotate( msgs_queue *queue )
{
	if( queue )
	{
		msgs *tmp = queue->head;
		
		if( tmp && tmp->next )
		{
			if( !msgs_queue_pop( queue,  &tmp ) )
			{
				return( -2 );
			}
			
			if
			(
				( tmp->msgsflags & msgs_flags_discard ) == msgs_flags_discard ||
				(
					( tmp->msgsflags & msgs_flags_timecalced ) == msgs_flags_timecalced &&
					tmp->reftime < time( 0 )
				)
			)
			{
				if( ( tmp->msgsflags & msgs_flags_timecalced ) == msgs_flags_hardwired )
				{
					/* Hardwired, so delinking is all we're allowed to do. */
					
					return( 1 );
				}
				
				*tmp = (msgs){ 0 };
				free( tmp );
				tmp = 0;
				
				return( 1 );
			}
			
			return( msgs_queue_append( queue, tmp ) );
		}
		
		return( 0 );
	}
	
	return( -1 );
}
int msgs_queue_deinit( msgs_queue *queue )
{
	if( queue )
	{
		msgs *tmp = 0;
		int i = msgs_queue_pop( queue,  &tmp );
		if( i < 0 )
		{
			return( -2 );
		}
		if( i == 0 )
		{
			return( 0 );
		}
		
		while( i )
		{
			if( !tmp )
			{
				return( -3 );
			}
			
			if( ( tmp->msgsflags & msgs_flags_timecalced ) != msgs_flags_hardwired )
			{
				*tmp = (msgs){ 0 };
				free( tmp );
				tmp = 0;
			}
			
			i = msgs_queue_pop( queue,  &tmp );
		}
		
		if( i < 0 )
		{
			return( -4 );
		}
		msgs_queue_init( queue );
		return( 1 );
	}
	
	return( -1 );
}



static int msgs_innerbuild( msgs **ret,  const char *format, va_list *args )
{
	if( ret && format && args )
	{
		va_list args2;
		va_copy( args2, ( *args ) );
		
			/*
				See: https://en.cppreference.com/w/c/io/vfprintf.html
				
				Writes the results to a character string buffer. At most
					(bufsz - 1)
				characters are written. The resulting character string will be
				terminated with a null character, unless bufsz is zero. If
				bufsz is zero, nothing is written and buffer may be a null
				pointer, however the return value (number of bytes that would
				be written not including the null terminator) is still
				calculated and returned.
			*/
			/*
			Return:
				The number of characters written if successful or negative
				value if an error occurred. If the resulting string gets
				truncated due to buf_size limit, function returns the total
				number of characters (not including the terminating null-byte)
				which would have been written, if the limit was not imposed.
			*/
			/*
			Notes:
				vsnprintf() is C99.
				
				All these functions invoke va_arg at least once, the value of
				arg is indeterminate after the return. These functions do not
				invoke va_end, and it must be done by the caller.
				
				vsnprintf_s, unlike vsprintf_s, will truncate the result to
				fit within the array pointed to by buffer.
				
				The implementation of vsnprintf_s in the Microsoft CRT does
				not conform to the C standard. Microsoft's version has an
				extra parameter size_t count in third position that contains
				the maximum amount of characters to be written, not including
				the null terminator. This parameter is possibly distinct from
				the buffer size provided via the parameter size_t bufsz.
			*/
		int i =
			vsnprintf
			(
				(char*)0, 0,
				
				format, *args
			);
		if( i < 0 )
		{
			return( -2 );
		}
		
			/* We've already checked the range, so we can reliably cast 'i'. */
		*ret =
			(msgs*)malloc
			(
				sizeof( msgs ) +
				sizeof( char ) * ( (unsigned)i + 1 )
			);
		if( !( *ret ) )
		{
			return( -3 );
		}
		
		**ret = (msgs){ 0 };
		
			/* We've already checked the range, so we can reliably cast 'i'. */
		( *ret )->buf.len = (unsigned)i;
		( *ret )->buf.b = (char*)( ( *ret ) + 1 );
		
		int i2 =
			vsnprintf
			(
				( *ret )->buf.b,
				( *ret )->buf.len,
				
				format, args2
			);
		if( i2 > i || i2 < 0 )
		{
			return( -4 );
		}
		
		
		va_end( args2 );
		return( 1 );
	}
	
	return( -1 );
}



	/* Uses common vsnprintf() to parse it's args into a message string. */
int msgs_build( msgs **ret,  const char *format, ... )
{
	if( ret && format )
	{
		va_list args;
		va_start( args, format );
		
		int i = msgs_innerbuild( ret,  format, &( args ) );
		
		va_end( args );
		
		return( i );
	}
	
	return( -1 );
}

/* 'error' overrides the normal circulation until all of it's entries */
/*  have been displayed at least once (and either deleted or movede into */
/*  'alert'), 'fatal' gets printed by an on_exit() handler, 'alert' and */
/*  'note' are distinguished from each other visually.*/
/* Note that 'fatal' messages never get displayed in the TUI, just in */
/*  e.g. the XTerm terminal. */
	/* These do the same thing as msgs_build(), but they push the */
	/*  message into the relevant queue/set the relevant flag. */
int msgs_build_note( msgs **ret,  const char *format, ... )
{
	if( format )
	{
		va_list args;
		va_start( args, format );
		
		msgs *msg = 0;
		
		int i = msgs_innerbuild( &msg,  format, &( args ) );
		if( !msg )
		{
			return( -2 );
		}
		if( i )
		{
			if( ret )
			{
				*ret = msg;
			}
			
			if( !msgs_queue_append( &messages,  msg ) )
			{
				*ret = 0;
				free( msg );
				return( -3 );
			}
			
			/* Fall-through. */
		}
		
		va_end( args );
		
		return( i );
	}
	
	return( -1 );
}
int msgs_build_alert( msgs **ret,  const char *format, ... )
{
	if( format )
	{
		va_list args;
		va_start( args, format );
		
		msgs *msg = 0;
		
		int i = msgs_innerbuild( &msg,  format, &( args ) );
		if( !msg )
		{
			return( -2 );
		}
		if( i )
		{
			if( ret )
			{
				*ret = msg;
			}
			
			if( !msgs_queue_append( &messages,  msg ) )
			{
				*ret = 0;
				free( msg );
				return( -3 );
			}
			
				/* Visually distinguish this message as important. */
			msgs_mark_important( msg );
			
			/* Fall-through. */
		}
		
		va_end( args );
		
		return( i );
	}
	
	return( -1 );
}
int msgs_build_error( msgs **ret,  const char *format, ... )
{
	if( format )
	{
		va_list args;
		va_start( args, format );
		
		msgs *msg = 0;
		
		int i = msgs_innerbuild( &msg,  format, &( args ) );
		if( !msg )
		{
			return( -2 );
		}
		if( i )
		{
			if( ret )
			{
				*ret = msg;
			}
			
				/* Messages from this queue override the normal queue for */
				/*  display, and if they aren't "expired" upon rotate then */
				/*  they then get moved to the end of the normal message */
				/*  queue for at least one more round of display. */
			if( !msgs_queue_append( &errors,  msg ) )
			{
				*ret = 0;
				free( msg );
				return( -3 );
			}
			
				/* Visually distinguish this message as important. */
			msgs_mark_important( msg );
			
			/* Fall-through. */
		}
		
		va_end( args );
		
		return( i );
	}
	
	return( -1 );
}
int msgs_build_fatal( msgs **ret,  const char *format, ... )
{
	if( format )
	{
		va_list args;
		va_start( args, format );
		
		msgs *msg = 0;
		
		int i = msgs_innerbuild( &msg,  format, &( args ) );
		if( !msg )
		{
			return( -2 );
		}
		if( i )
		{
			if( ret )
			{
				*ret = msg;
			}
			
				/* These only get displayed if there's a fatal error that */
				/*  triggers exit(). */
			if( !msgs_queue_append( &fatalities,  msg ) )
			{
				*ret = 0;
				free( msg );
				return( -3 );
			}
			
				/* Visually distinguish this message as important. */
			msgs_mark_important( msg );
			
			/* Fall-through. */
		}
		
		va_end( args );
		
		return( i );
	}
	
	return( -1 );
}


	/* Automatically chooses between notes/alerts, vs errors. Note that */
	/*  this will also "initialize" any time-pending message that it */
	/*  returns into a time-calculated message, but DOES NOT release any. */
struct abuf* msgs_peek()
{
	msgs *tmp = errors.head;
	
	if( !tmp )
	{
		tmp = messages.head;
	}
	
	if( tmp )
	{
		if( ( tmp->msgsflags & msgs_flags_timecalced ) == msgs_flags_timepending )
		{
			/* Process timer messages. */
			
			tmp->reftime += time( 0 );
			tmp->msgsflags |= msgs_flags_timecalced;
		}
		
		return( &( tmp->buf ) );
	}
	
	return( 0 );
}

	/* Shift msgs_peek() from it's current target, to it's next. This can */
	/*  result in the "current" message being deallocated after */
	/*  replacement (notes, alerts, or errors), or to be moved into the */
	/*  "note/alert" queue (for errors). */
int msgs_rotate()
{
	msgs *tmp;
	
	msgs_peek();
	
	if( errors.head )
	{
		if( !msgs_queue_pop( &errors,  &tmp ) )
		{
			return( -2 );
		}
		
		if
		(
			( tmp->msgsflags & msgs_flags_discard ) == msgs_flags_discard ||
			(
				( tmp->msgsflags & msgs_flags_timecalced ) == msgs_flags_timecalced &&
				tmp->reftime < time( 0 )
			)
		)
		{
			if( ( tmp->msgsflags & msgs_flags_timecalced ) == msgs_flags_hardwired )
			{
				/* Hardwired, so delinking is all we're allowed to do. */
				
				return( 1 );
			}
			
			*tmp = (msgs){ 0 };
			free( tmp );
			tmp = 0;
			
			return( 1 );
		}
		
		if( !msgs_queue_append( &messages, tmp ) )
		{
			return( -3 );
		}
		
		return( 1 );
	}
	
	if( !msgs_queue_rotate( &messages ) )
	{
		return( -4 );
	}
	
	return( 1 );
}

	/* Forcibly deletes the current target of msgs_peek(): note that this */
	/*  DOES NOT update the peek until AFTER delinking the target, so */
	/*  messages added since the last call to *_peek() will NEVER be */
	/*  deleted by this function. */
int msgs_freecurrent()
{
	msgs *tmp = 0;
	
	if( errors.head )
	{
		if( !msgs_queue_pop( &errors,  &tmp ) )
		{
			return( -2 );
		}
		
	} else {
		
		if( !msgs_queue_pop( &messages,  &tmp ) )
		{
			return( -3 );
		}
	}
	
	if( ( tmp->msgsflags & msgs_flags_timecalced ) == msgs_flags_hardwired )
	{
		/* Hardwired, so delinking is all we're allowed to do. */
		
		return( 1 );
	}
	
	*tmp = (msgs){ 0 };
	free( tmp );
	tmp = 0;
	
	return( 1 );
}

	/* Prints the 'fatal' messages. This MUST be called after returning */
	/*  to the normal state of the terminal. No free()s attemted. */
void msgs_atexit( void )
{
	msgs *tmp = 0;
	
	while( fatalities.head )
	{
		if( !msgs_queue_pop( &fatalities,  &tmp ) )
		{
			printf( "\nERROR: msgs_atexit(): msgs_queue_pop() returned a failure!\n" );
			return;
		}
		
		printf( tmp->buf.b );
	}
	
	/* Done. */
}

