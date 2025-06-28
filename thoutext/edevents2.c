/* Thou -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * edevents2.c : A workspace for moving edevents onto coroutines.
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


/*
TODO:
	Everything.
	Build the I/O system.
*/

		/* Holds assorted things from kilo.h:editorConfig{} : Only */
		/*  three lines actually have some code from Kilo, but carry */
		/*  over credit anyways. */
	typedef struct milli_pane milli_pane;
		typedef enum
		{
			milli_pane_invalid = -1,
			
			milli_pane_null = 0,
			
			milli_pane_dirty = 1,
			milli_pane_wordwrap = 2
			
		} milli_pane_flags;
	struct milli_pane
	{
		modepane pane;
		
#warning "Do we need to track whether raw-mode is set?"
			/* All measurements in terms of display cells. */
		pane_dim
			cursor, /* Location on screen. */
			display; /* Available display space. */
		
			/* This should eventually be replaced with something less limited. */
		uintmax_t
			numcols, numrows, /* File extents in display cells. */
				/* These two are for scrolling: */
			offcols, offrows; /* Display's view into the document in display cells. */
		
		erow *row; /* Row contents. */
		struct editorSyntax *syntax; /* Current syntax highlight, or NULL. */
		milli_pane_flags flags;
		
		char *filepath, *filename; /* Currently open file. */
		
		
		/* We need to expand this with it's i/o route. */
	};
	#define MILLI_PANE_NULL() ( (milli_pane){ 0 } )
	int milli_pane_init
	(
		milli_pane *p,
			size_t start_size,
			const pane_calls *vtab
	)
	{
		if( p )
		{
			int res =
				modepane_init
				(
					&( p->pane ),
						start_size,
						"Milli text ed.",
						vtab
				);
			
			p->cursor = { 0 };
			p->display = { 0 };
			
			p->numcols = 0;
			p->numrows = 0;
			p->offcols = 0;
			p->offrows = 0;
			
			p->row = 0;
			p->syntax = 0;
			p->flags = milli_pane_invalid;
			
			p->filename = 0;
			p->filepath = 0;
			
			return( res );
		}
		
		return( -1 );
	}
	milli_pane milli_pane_init2
	(
			size_t start_size,
			const pane_calls *vtab
	)
	{
		milli_pane mp;
		
		if( !milli_pane_init( &mp,  start_size, vtab ) )
		{
			exit( 1 );
		}
		
		return( mp );
	}
	#error "milli_pane needs a deinitializer!"
	
	
	
	/* The europa stuff was here, now its in ../io/europa.c */



	typedef struct
	{
		milli_pane pane;
		
		corohead *coro;
		corohead *volatile ret_dest;
		
	} milli_inner;
static size_t allocation = 1024 * 1024; /* Stack size. */
static void milli_spawn_coromain( corohead *head, void *data );
static int milli_spawn_conclude( corohead *head, uintptr_t aux );
modepane* milli_spawn()
{
	corohead *coro = 0, *tmp;
	
	
		/* This stuff needs to migrate to milli_spawn_coromain() */
		/*  as the edevents stuff transitions to "more natively" */
		/*  a coroutine system. */
	initEditor();
	
	
	int res =
		cobuild
		(
			allocation,
			(void*)&tmp, &milli_spawn_coromain, 0,
			&milli_spawn_conclude,
			
			&coro
		);
	if( !res )
	{
		return( 0 );
	}
	if( !coro )
	{
		return( 0 );
	}
	
	coyield2( coro, &tmp,  0, 0 );
	
	return( (modepane*)( head->auxiliary ) );
}
static void milli_spawn_coromain( corohead *coro, void *data )
{
	if( coro && data )
	{
		volatile milli_inner milli;
		
		int
			loop = 1 /*CORO_WORKING*/ ,
			res =
				milli_pane_init
				(
					(milli_pane*)( &( milli.pane ) ),
						1024, /* Starting buffer. */
						const pane_calls *vtab /* Need to build this. */
				);
		milli.coro = coro;
		milli.ret_dest = *( (corohead**)data );
		coro->auxiliary = (uintptr_t)&( milli.pane );
		
		while( loop == 1 /*CORO_WORKING*/ )
		{
			coro = (corohead*)( milli.ret_dest );
			if( !coro )
			{
				printf( "\n\tmilli.ret_dest was null in milli_spawn_coromain()!\n" );
				exit( 1 );
			}
			milli.ret_dest = 0;
				loop = coyield( coro );
			coro = 0;
			/* milli.ret_dest has already been set elsewhere. */
		}
	}
}
static int milli_spawn_conclude( corohead *head, uintptr_t aux )
{
	(void)aux;
	
	if( head )
	{
		/* Stick something here. */
		
		return( 1 );
	}
	
	return( -1 );
}




static void milli_processKeypress_inner( void *v_ )
{
	if( v_ )
	{
		int fd = *( (int*)v_ );
		
		editorProcessKeypress( fd );
	}
}
	/* This is meant to replace editorProcessKeypress(). */
void milli_processKeypress( milli_pane *mp, int fd )
{
	if( mp )
	{
		milli_inner *mpi =
				/* Looks like freaking LISP. */
			(milli_inner*)(
				( (char*)mp ) +
				(
					(
						(char*)
						&(
							(
								( (milli_inner*)0 ) + 1
							)->pane
						)
					) -
					(
						(char*)( ( (milli_inner*)0 ) + 1 )
					)
				)
			);
		
		coyield2
		(
			mpi->coro,
			&( mpi->ret_dest ),
			
			(void*)&fd,
			&milli_processKeypress_inner
		);
		
	} else {
		
		printf( "\n\tmilli_processKeypress() was given a null pane pointer!\n" );
		exit( 1 );
	}
}
