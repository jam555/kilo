/* Thou:Milli -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *     a text editor in less than 1-kilo lines of code (as counted by "cloc").
 *     Does not depend on libcurses, directly emits VT100 escapes on the
 *     terminal.
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

#ifndef MSGS_H
# define MSGS_H
	
	#include "appenbuf.h"
	
	
	typedef struct msgs msgs;
	
	
	
	typedef struct msgs_queue
	{
		msgs *head, **tail;
		
	} msgs_queue;
	
	
		/* Note that the e.g. error queue are statically initialized. */
	int msgs_queue_init( msgs_queue *queue );
	int msgs_queue_pop( msgs_queue *queue,  msgs **recip );
	int msgs_queue_append( msgs_queue *queue, msgs *val );
	int msgs_queue_rotate( msgs_queue *queue );
	int msgs_queue_deinit( msgs_queue *queue );
	
	
	
	typedef enum
	{
			/* This message isn't needed anymore, free it. */
		msgs_flags_discard = 1,
			/* This message is an alert, so needs a visual distinction. */
		msgs_flags_important = 2,
		
		/* These next three use a 2-bit field. */
			/* Used for things like statics, which MUST NOT be given to */
			/*  free(). */
		msgs_flags_hardwired = 4,
			/* reftime stores a delay, NOT a time: use it to calculate */
			/*  the needed time. */
		msgs_flags_timepending = 8,
			/* Pay attention to the time. */
		msgs_flags_timecalced = 12
		
	} msgs_flags;
	struct msgs
	{
		msgs *next;
		
		struct abuf buf;
		time_t reftime;
			/* Use values from msgs_flags. */
		unsigned char msgsflags;
	};
	typedef struct msgs_view
	{
		struct abuf *buf;
			/* Use values from msgs_flags. */
		unsigned char msgsflags;
		
	} msgs_view;
	
	
		/* For initializing a preallocated message. */
	int msgs_initmsg( msgs *recip,  unsigned char flags, char *text, size_t len );
	
	
		/* Uses common vsnprintf() to parse it's args into a message string, */
		/*  uses malloc() to allocate it's return message (which is returned */
		/*  via the first argument). Note that this ONLY exists so that this */
		/*  general system can be used for other purposes, the messages it */
		/*  assembles CANNOT be directly provided back for further use in */
		/*  this system. */
	int msgs_build( msgs**,  const char*, ... );
	
	/* 'error' overrides the normal circulation until all of it's entries */
	/*  have been displayed at least once (and either deleted or movede into */
	/*  'alert'), 'fatal' gets printed by an on_exit() handler, 'alert' and */
	/*  'note' are distinguished from each other visually.*/
	/* Note that 'fatal' messages never get displayed in the TUI, just in */
	/*  e.g. the XTerm terminal. */
		/* These do the same thing as msgs_build(), but they push the */
		/*  message into the relevant queue/set the relevant flag. */
	/* !!! WARNING !!! */
	/*  !!! NEVER deallocate returned msgs instances yourself, just mark them */
	/*  as msgs_flags_discard and let it happen "naturally" !!! */
	int msgs_build_note( msgs**,  const char*, ... );
	int msgs_build_alert( msgs**,  const char*, ... );
	int msgs_build_error( msgs**,  const char*, ... );
	int msgs_build_fatal( msgs**,  const char*, ... );
	
	int msgs_mark_discard( msgs* );
	int msgs_mark_unimportant( msgs* );
		/* Note that you should just use *_alert() in almost all cases. */
	int msgs_mark_important( msgs* );
	int msgs_mark_plainlife( msgs* );
	int msgs_mark_hardlife( msgs* );
	int msgs_mark_timelife( msgs *msg, time_t relative );
	
		/* Automatically chooses between notes/alerts, vs errors. Note that */
		/*  this will also "initialize" any time-pending message that it */
		/*  returns into a time-calculated message, but DOES NOT release any. */
	msgs_view msgs_peek( void );
	
		/* Shift msgs_peek() from it's current target, to it's next. This can */
		/*  result in the "current" message being deallocated after */
		/*  replacement (notes, alerts, or errors), or to be moved into the */
		/*  "note/alert" queue (for errors). */
	int msgs_rotate( void );
	
		/* Forcibly deletes the current target of msgs_peek(): note that this */
		/*  DOES NOT update the peek until AFTER delinking the target, so */
		/*  messages added since the last call to *_peek() will NEVER be */
		/*  deleted by this function. */
	int msgs_freecurrent( void );
	
		/* Prints the 'fatal' messages. This MUST be called after returning */
		/*  to the normal state of the terminal. No free()s attemted. */
	void msgs_atexit( void );
	
	
	
	#define MODEMSGS_MILLI_MAIN 1
	#define MODEMSGS_MILLI_FIND 2
	
	int modemsgs_setmodal( int id );
	
#endif
