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

#include <stdlib.h>
#include "coro.h"


__thread corohead main_fiber = { 0 };

static volatile __thread corohead *current_fiber = 0;
static volatile corohead *dead_fiber = 0;


static void debug_marker()
{
	/* Do nothing. */
	return;
}


#ifdef __GNUC__
	/* This is actually a length defined by the linker, but this is how it */
	/*  appears. */
/* extern void *__stack_size; */
/*
	__size_of_stack_reserve__
*/
#else
#error "coro.c encountereed an unsupported compiler!\n"
	/* MSVC++ stuff: what's the C version? Might need to just wrap it. */
/* property int StackReserveSize { int get(); void set(int value); }; */
#endif

	/* This only works for CC & similar. */
uintptr_t get_defaultstacksize()
{
		/* Just blindly allocate 1 meg. */
	return( 1024 * 1024 );
	/* return( (uintptr_t)__stack_size ); */
}


	/* This exists to be used on the main-coroutine as it's conclude, purely */
	/*  because that should NEVER run, and thus any time it runs is a major */
	/*  error. */
static int co_conclude( corohead *ign1, uintptr_t ign2 )
{
	/* THis SHOULD literally never run. SHOULD... */
	
	(void)ign1;
	(void)ign2;
	
	exit( 1 );
}

	/* Not needed here, but maybe on other platforms? */
int cocontext( void *data, int (*func)( void* ) )
{
		/* Required by GCC. */
	current_fiber = &main_fiber;
	main_fiber =
	(corohead)
	{
			/* It shouldn't be possible for this to get used. */
		(void*)&exit,
		
		&main_fiber,
		0,
		&co_conclude,
		
		&exit,
		0,
		
		0,
		
		{ 0 }
	};
	
	return( func( data ) );
}

/* These two functions actually build and initialize coroutines. */
#if 1
static void coro_boot
(
	corohead *head,
	
	void *coro_data,
	void (*coro_main)( corohead*, void* )
)
{
	if( setjmp( head->state ) )
	{
		coro_main( head, coro_data );
		
	} else {
		
			/* ONLY jumps back into cobuild(). */
		longjmp( ( (corohead*)current_fiber )->state, 1 );
	}
}
#define set_sp(p) \
  asm volatile( "mov %0, %%rsp" : : "r"(p) )
static void coro_bootcaller( void **alloc )
{
	/* SysV (including Linux) version. The 64-bit MSVC must be in */
	/*  an assembly file.  */
	
	set_sp( alloc );
	/* The setjmp() should be enough to undo the low-level */
	/*  modifications above. */
	
		/* Note that despite some online samples implying otherwise, */
		/*  registers DO require TWO leading percent signs, NOT just */
		/*  one! */
		/* ... WHY DOES THE EXTENDED SYNTAX CALL FOR DIFFERENT NUMBERS */
		/*  OF PERCENT SIGNS?!? */
		/* Override the previous frame pointer, essentially to hide */
		/*  it. */
	asm volatile ( "movq %rsp,  %rbp\n" );
	asm volatile
	(
		/* Load the arguments: this isn't needed elsewhere. */
		"popq %rcx\n" /* head */
		"popq %rdx\n" /* coro_data */
		"popq %r8\n" /* coro_main */
	);
		/* Restore alignment. */
	/* asm volatile ( "pushq %rcx\n" ); */
		/* Call coro_main, the args are already ready. DO NOT WRAP */
		/*  rsp IN PARENS! THAT CAUSES AN ERROR (presumably due to */
		/*  excessive indirections)! Asterick seems fine. */
	asm volatile ( "call *%0\n" : : "r"(&coro_boot) );
		/* Pop both align padding, & MSVC ret addr. */
	asm volatile ( "add $16,  %rsp\n" );
	asm volatile
	(
		/* Note that for an MSVC version, the add above would be */
		/*  just 8, and these would be "movq"s, but wouldn't be in */
		/*  a C file. */
		"pop %%rcx\n" /* ch / rcx */
		"pop %%rdx\n" /* cb / rdx */
		"pop %%r8\n" /* conclude / r8 */
		
		/* Tail-call into cocollapse(). Could technically be the */
		/*  wrong name. */
		"jmp *(%0)\n"
		: : "r"(&cocollapse)
	);
}
#else
	#error "Only GCC & Clang can reliably be supported without extension."
#endif
int cobuild
(
	size_t stacksize,
	
	void *coro_data,
	void (*coro_main)( corohead*, void* ),
		/* This is optional, as coro_main() can always just set corohead-> */
		/*  ->auxiliary manually. */
	uintptr_t coro_auxiliarydata,
	
		/* This gets used as a flag during destruction, so it CANNOT be null. */
		/* conclude() is the deinitializer for the coroutine. */
	int (*conclude)( corohead*, uintptr_t ),
	
	
	corohead **ret
)
{
	if( sizeof( void* ) > 8 )
	{
		/* We know nothing about this void* format, thus we know nothing */
		/*  about the target! */
		return( -2 );
	}
	
	if
	(
		stacksize > sizeof( dummyframe ) * 2 &&
		ret && current_fiber && conclude
	)
	{
		stacksize +=
			sizeof( corohead ) * 2 +
			sizeof( dummyframe ) * 2 +
			sizeof( corobody ) +
				/* x86-64 red-zone, used in Linux apps as a safe zone for */
				/*  temporaries by funcs. */
			128;
		
		void **alloc = (void**)malloc( stacksize );
		if( !alloc )
		{
			return( -2 );
		}
		
		/* Find the corohead. */
		corohead *head;
		{
			/* Move above the stack, since we grow downwards. */
			uintptr_t head_ = (uintptr_t)alloc, mask;
			head_ += stacksize;
			
			/* Calculate the location of the header. */
			head_ -= sizeof( corohead );
			static unsigned char masks[ 8 ] =
				{
#if 1
						/* Linux currently uses 16-byte alignment on x86-32 */
						/*  too, so let's just do it. */
					0, 1, 3, 16,
	/* MSWin x64 & Linux AMD64 alignment version. */
					15, 15, 15, 15
#else
	/* Simple alignment version. */
					0, 1, 3, 3,
					7, 7, 7, 7
#endif
				};
			debug_marker();
			/* This code was broken into pieces to track down an alignment */
			/*  bug, it can be returned to normal now. */
			mask = sizeof( void* );
			mask -= 1; /* Shift fdrom index to offset mode. */
			mask = (uintptr_t)masks[ mask ];
			mask = ~( mask );
				/* Adjust into alignment. */
			head_ &= mask;
				/* Adjust downwards for ->exit_retaddr, because that's meant */
				/*  to be 8 bytes OUT of 16-byte alignment.  */
			head_ -= sizeof( void* );
			
			head = (corohead*)head_;
			/* Basic header initialization. */
				/* If this EVER gets called, something went VERY wrong. Note */
				/*  that depending on the size of int, exit MIGHT receive */
				/*  ->lastbyte as an argument. */
			head->exit_retaddr_MSVC = &exit;
			head->exit_retaddr_SysV = &exit;
			/* For cocollapse(), these are meant to act as it's args. */
			head->here = head;
			head->lastbyte_a = (corobody*)alloc;
			head->lastbyte_b = (corobody*)alloc;
			head->conclude = conclude;
			head->auxiliary = coro_auxiliarydata;
			/* head->state gets initialized a bit later. */
			
			/* Update alloc. */
			alloc = (void**)head_;
			/* Note that although alloc won't be 16-byte aligned anymore, */
			/*  it's actually SUPPOSED to be like that. */
		}
		
		/* Pack the args for coro_boot(). */
		*( --alloc ) = coro_main;
		*( --alloc ) = coro_data;
		*( --alloc ) = head;
		/* alloc should now be aligned again. This simplifies later math. */
		
			/* ONLY jumped to by coro_boot(). */
		if( !setjmp( ( (corohead*)current_fiber )->state ) )
		{
			coro_bootcaller( alloc );
		}
		
		/* We are done spawning the coroutine/fiber, return it. */
		
		*ret = head;
		return( 1 );
	}
	
	return( -1 );
}

uintptr_t coro_getaux()
{
	if( current_fiber )
	{
		return( current_fiber->auxiliary );
	}
	
	return( 0 );
}

int coyield( corohead *dest )
{
	if( !current_fiber )
	{
		exit( 1 );
	}
	if( dest )
	{
		if( !( dest->conclude ) )
		{
			return( CORO_DONE );
		}
		
		int res = setjmp( ( (corohead*)current_fiber )->state );
		if( !res )
		{
			current_fiber = dest;
			longjmp( dest->state, CORO_WORKING );
		}
		
			/* Let's take advantage to get rid of any accumulated debris. */
		coclean();
		
		return( res );
	}
	return( CORO_BADARGS );
}

void cocollapse
(
	corohead *head,
	corobody *body,
	int (*conclude)( corohead*, uintptr_t )
)
{
	coclean();
	
	if( !( head->conclude ) || !conclude || head == &main_fiber )
	{
		/* Use ->conclude as a marker for "already running this elsewhere". */
		
		return;
	}
	
		/* We actually want this test & null to be an atomic cmp-and-swap, */
		/*  even for signals. */
	if( conclude != current_fiber->conclude )
	{
		exit( 1 );
	}
	current_fiber->conclude = 0;
	
	/* Test result. */
	if( !( conclude( head, current_fiber->auxiliary ) ) )
	{
		exit( 1 );
	}
	
	if( head == current_fiber )
	{
		/* DON'T delete the fiber while we're using it as our stack, stick it */
		/*  in a cleanup stack instead. We really want to use atomics here. */
		
		head->here = (corohead*)dead_fiber;
		dead_fiber = head;
		if( coyield( &main_fiber ) )
		{
			/* Welp, this is bad, we weren't supposed to see that return. */
			
			exit( 1 );
		}
		
	} else {
		
		free( head->lastbyte_b );
	}
}
void coclean()
{
	/* We really want to use atomics with dead_fiber. */
	
	corohead *tmp = (corohead*)dead_fiber;
	while( dead_fiber )
	{
		dead_fiber = tmp->here;
		tmp->here = 0;
		free( tmp );
	}
}
