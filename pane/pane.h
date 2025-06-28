/* Mila -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * pane.h : A display-source pane system
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

#ifndef PANE_PANE_H
# define PANE_PANE_H
	
	#include "../kilo.h"
	#include "../appenbuf.h"
	#include "../coroutine/coro.h"
	#include "../dynarr.h"
	
	
	
	/*
		Need a pane derivative to be defined that takes e.g. the messages &
			errors msgs_queue{}s for itself. Note that related functions will
			need to take pointers to a pane.
	*/
	
	
	
	typedef struct pane_dim
	{
		size_t col /* x */, row /* y */ ;
		
	} pane_dim;
	
	/* Panes are used to hold data that allows something to be displayed */
	/*  without explicitly running code to recompute it. */
	typedef struct pane_calls pane_calls;
	typedef struct pane
	{
		/* panenotes and coro should both be elsewhere. */
		/*
		corohead *coro;
		*/
		
		dynarr *a, *b, *c, **dest;
		
			/* Used by the display code to identify the active pane's type in */
			/*  the status line. Each type of pane uses the same string. */
		const char *const modename;
		const pane_calls *const vtab;
		
	} pane;
	struct pane_calls
	{
		/* Note that each reference to pane (or modepane) must be at a known */
		/*  offset within a larger structure holding whatever info the */
		/*  function actually needs. The implementation of that falls on the */
		/*  programmer, NOT on the pane system. */
		
		/* Note that for ALL funcs, a "0" return should mean "no action", */
		/*  positive is success, and negative is error. */
/*  5    0    5    0    5    0    5    0    5    0    5    0    5    0    5    0 */
		
		int (*on_resize)( pane*,  size_t rows, size_t cols );
		int (*on_refresh)( pane* );
		int (*on_orphan)( pane* );
		/*
			Will also need a dusk/dawn func(s), but that needs the "io" system. Use
			the kilo.h:gaianphase{} values to indicate the "destination behavior",
			so that the code will know whether to e.g. deallocate buffers.
		*/
	};
	
		/* This should be run INSIDE the related coroutine, *p should be */
		/*  allocated on it's stack. */
	int pane_init
	(
		pane *p,
			size_t start_size,
			const char *modename,
			const pane_calls *vtab
	);
	#error "De-initializers for pane{} and modepane{} are required!"
	
	inline int pane_on_resize( pane *pn,  size_t rows, size_t cols )
	{
		if( pn )
		{
			if( !( pn->vtab ) )
			{
				return( -2 );
			}
			if( !( pn->vtab->on_resize ) )
			{
				return( -3 );
			}
			
			return( pn->vtab->on_resize( pn,  rows, cols ) );
		}
		
		return( -1 );
	}
	inline int pane_on_refresh( pane *pn )
	{
		if( pn )
		{
			if( !( pn->vtab ) )
			{
				return( -2 );
			}
			if( !( pn->vtab->on_refresh ) )
			{
				return( -3 );
			}
		}
			
			return( pn->vtab->on_refresh( pn ) );
		
		return( -1 );
	}
	inline int pane_on_orphan( pane *pn )
	{
		if( pn )
		{
			if( !( pn->vtab ) )
			{
				return( -2 );
			}
			if( !( pn->vtab->on_orphan ) )
			{
				return( -3 );
			}
			
			return( pn->vtab->on_orphan( pn ) );
		}
		
		return( -1 );
	}
	
	/* These are just fillers for if you don't want to do anything. They */
	/*  always return 0 and do nothing else. */
	int dummypane_on_resize( pane *pn,  size_t rows, size_t cols );
	int dummypane_on_refresh( pane *pn );
	int dummypane_on_orphan( pane *pn );
	
	
	typedef struct modepane modepane;
	struct modepane
	{
		pane p;
		
			/* Holds pane-specific state info, meant to be displayed with */
			/*  a pane's modename. */
		char panenotes[ 16 ];
	};
	
	int modepane_init
	(
		modepane *p,
			size_t start_size,
			const char *modename,
			const pane_calls *vtab
	);
	
#endif
