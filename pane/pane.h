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

#ifndef PANE_H
# define PANE_H
	
	#include "../kilo.h"
	#include "../appenbuf.h"
	#include "../coroutine/coro.h"
	#include "../dynarr.h"
	
	
	
	/* Panes are used to hold data that allows something to be displayed */
	/*  without explicitly running code to recompute it. */
	typedef struct pane pane;
	struct pane
	{
		/* panenotes and coro should both be elsewhere. */
		/*
		corohead *coro;
		*/
		
		dynarr *a, *b, *c, **dest;
		
			/* Used by the display code to identify the active pane's type in */
			/*  the status line. Each type of pane uses the same string. */
		const char *const modename;
	};
	
		/* This should be run INSIDE the related coroutine, *p should be */
		/*  allocated on it's stack. */
	int pane_init
	(
		pane *p,
			size_t start_size,
			const char *modename
	);
	
	
	typedef struct modepane modepane;
	struct modepane
	{
		pane p;
		
			/* Holds pane-specific state info, meant to be displayed with */
			/*  a pane's modename. */
		char panenotes[ 16 ];
	};
	
#endif
