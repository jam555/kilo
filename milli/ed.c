/* Thou:Milli -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *     a text editor in less than 1-kilo lines of code (as counted by "cloc").
 *     Does not depend on libcurses, directly emits VT100 escapes on the
 *     terminal.
 *
 * -----------------------------------------------------------------------
 *
 * ed.c
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


#include "../kilo.h"
#include "../coroutine/coro.h"


	/* Just a stand-in for a future "real" version of pane{}. */
typedef struct pane
{
	int dummy_var;
	
} pane;
	/* This is now going to be defined in kilo.h, it can move */
	/*  here later. */
/*
typedef struct millistate
{
	corohead *head;
	char *filename;
		/ * The target is volatile, not the pointer. * /
	struct editorConfig volatile *Econf;
		/ * The pointer is volatile, not the target. * /
	corohead *volatile ret_dest;
	pane here;
	
} millistate;
*/
#define build_millistate( chead, Eptr, data, pane ) \
	(millistate){ \
		(chead), \
		(char*)0, /* Filename gets set seperately. */ \
		(Eptr), *( (corohead**)( data ) ), (pane) }

	/* Don't skimp... hopefully this isn't skimping. */
static const size_t allocation = 8 * 1024 * 1024;


static void milli_coromain( corohead *head, void *data );
static int milli_conclude( corohead *head, uintptr_t aux );
millistate* milli_build( char *filename );

void milli_refresh( millistate *state );
void milli_prockey( millistate *state );





static void milli_coromain( corohead *head, void *data )
{
	if( head && data )
	{
		
		millistate state = build_millistate( head, &E, data, (pane){ 0 } );
		corohead *tmp = 0;
		head->auxiliary = (uintptr_t)&state;
		
		// initEditor();
		
		// coyield(  );
	    
		// editorSelectSyntaxHighlight( args[ 1 ] );
	    // editorOpen( args[ 1 ] );
		
		int loop = 1 /*CORO_WORKING*/ ;
		while( loop == 1 /*CORO_WORKING*/ )
		{
			tmp = (corohead*)state.ret_dest;
			if( !tmp )
			{
				printf( "\n\ttmp was null in milli_coromain()!\n" );
				exit( 1 );
			}
			// stats.ret_dest = 0; Note: obsolete.
				loop = coyield( tmp );
			tmp = 0;
			
			// editorProcessKeypress( STDIN_FILENO );
			
			/* state.ret_dest MUST already have been set elsewhere. */
		}
	}
	/* printf( "\tExiting milli_coromain().\n" ); */
}
static int milli_conclude( corohead *head, uintptr_t aux )
{
	(void)aux;
	
	/* printf( "\nEntering milli_conclude()\n" ); fflush( stdout ); */
	
	if( head )
	{
		millistate *state = (millistate*)( head->auxiliary );
			head->auxiliary = 0;
			*state = build_millistate( 0, &E, 0, (pane){ 0 } );
		
		return( 1 );
	}
	
	return( -1 );
	
	/* printf( "\nExiting milli_conclude()\n" ); fflush( stdout ); */
}
millistate* milli_build( char *filename )
{
	/* printf( "\nEntering milli_build()\n" ); fflush( stdout ); */
	
	corohead *head = 0, *tmp;
	
	int res =
		cobuild
		(
			allocation,
			(void*)&tmp, &milli_coromain, 0,
			&milli_conclude,
			
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
	/*  NO, because we hand a pointer to tmp to milli_coromain() via cobuild(). */
	coyield2( head, &tmp,  0, 0 );
	
	/* It's set! Except we should use a COPY, then deallocate that. */
	( (millistate*)( head->auxiliary ) )->filename = filename;
#warning "We should issue a second coyield() here, to do some file opening stuff."
	/* Was that really ok? Is that one of those "unforeseen segfault" cases? */
	
	/* printf( "\t\tmilli_build() returning.\n" );
	fflush( stdout ); */
	return( (millistate*)( head->auxiliary ) );
}



static void milli_prockey_inner( void *v_ )
{
	/* This runs inside the coroutine. */
	
	(void)v_;
	
	editorProcessKeypress( STDIN_FILENO );
}
void milli_prockey( millistate *state )
{
	/* This bit runs outside the coroutine. */
	
	coyield2( state->head, &( state->ret_dest ),  (void*)0, &milli_prockey_inner );
}

static void milli_refresh_inner( void *v_ )
{
	/* This runs inside the coroutine. */
	
	(void)v_;
	
	editorRefreshScreen();
}
void milli_refresh( millistate *state )
{
	/* This bit runs outside the coroutine. */
	
	coyield2( state->head, &( state->ret_dest ),  (void*)0, &milli_refresh_inner );
}
