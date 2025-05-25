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

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>


/* This file, and coro.c, implement a coroutine (also called a fiber (akin to */
/*  "thread" from multi-threading), a go-routine (from the programming */
/*  language Go), a green-thread, a user-level thread, a protothread (though */
/*  those techincally are a more-limited switch-and-goto based mechanism), or */
/*  stackful coroutines) system, currently targetting 64-bit *nix platforms */
/*  only, but it should be practical to port it to e.g. 64-bit Windows and */
/*  various 32-bit platforms with few changes to the existing files. This */
/*  system was originally written for the Kilo-derived Mila or Thou text */
/*  editor, but should be generally useful. */
/* The system is divided into a few values, types, and functions. Of these, */
/*  the various dummyframe{} structures are only intended to be useful for */
/*  other low-level manipulations, and the other structure types are meant to */
/*  ONLY be allocated by cobuild(), and thus to only be refered to via */
/*  pointers. */
/* Be aware that for the purposes of this system, every thread is considered */
/*  to have a single coroutine running on it when it first starts: this is */
/*  refered to as the initial-coroutine or initial-coro: this coroutine is */
/*  allocated a per-thread corohead, and treated as a yield-target of last */
/*  resort: if this isn't desired, then transfer execution to some other */
/*  coroutine that does the "triggering action" (major example: */
/*  cocollapse()), so that this "last resort" behavior won't be incurred. */
/* Of the functions: */
	/* get_defaultstacksize() and cocontext() can absolutely be run */
	/*  seperately from any of the other functions, but all other functions */
	/*  can only reliably be invoked on a thread where cocontext() has been */
	/*  called, but HAS NOT returned, so that the system can function */
	/*  reliably even on platforms where OS coordination is necessary. */
	/*  */
	/* get_defaultstacksize() : A general utility function, meant to tell */
	/*  user code what the linked-in default stack allocation size is- this */
	/*  is meant to be used to predict a reliable stack size to specify to */
	/*  cobuild(). Note that while A relevant bit of code has been found for */
	/*  MSVC, only GCC (and others with a sufficiently similar linker) is */
	/*  actually currently supported. */
	/* cocontext() : Establishes a per-thread context that allows the other */
	/*  coroutine functions to reliably be used. On some platforms this will */
	/*  be utterly unneeded, on others it will be vital, thus it's "mandated" */
	/*  for all platforms. */
	/* cobuild() : Creates (& initializes) a new coroutine stack from the */
	/*  heap. The stacksize, coro_main, conclude, and ret values MUST be */
	/*  given relevant values, but all other values can be given irrelevant */
	/*  values, as they can simply be alterd in the corohead{} that is */
	/*  returned on success. */
	/* coro_getaux() : Returns the auxiliary value stored inside the */
	/*  currently active corohead. It is generally assumed that this will be */
	/*  a pointer, but anything that the user finds useful is in general */
	/*  appropriate. Note that since the user-provided coroutine-main must */
	/*  not exit before the coroutine is ready to be released, this value can */
	/*  reasonably be a pointer to a structure that is allocated on the stack */
	/*  by that entry-point. */
	/* coyield() : Switches from one coroutine to another. It doesn't take */
	/*  any arguments, as that facility is expected to be implemented by the */
	/*  user. NOte that this CAN fail, so pay attention to the return value: */
	/*  it will be one of the CORO_* values. coyield() will deallocate any */
	/*  coroutine instanmces that have been enqueued for such by calling */
	/*  coclean() right before it's "successful return" route actually */
	/*  results in a return. */
	/* cocollapse() : This is automatically stored into a corohead{} upon */
	/*  it's construction, but can also be called explicitly, BUT BEWARE, as */
	/*  it doesn't directly perform ANY deinitialization of the actual */
	/*  coroutine stack: that must be provided by the conclude() function */
	/*  that is provided to cobuild(), and is thus a burden on the user */
	/*  instead of on the coroutine system. Having said that, the conclude() */
	/*  pointer will be nulled once the function has been entered, so that */
	/*  cocollapse() can use that as a flag to prevent iotself from running */
	/*  twice on the same corohead{}. The conclude() function will be called */
	/*  by the invocation of cocollapse(), which will then either directly */
	/*  deallocate the coroutine (which is NOT done by deallocating the */
	/*  corohead{}!), or will be added to a linked-list-stack for later */
	/*  deallocation and coyield() will then be called to return control to */
	/*  the initial-coro (which in turn will result in all of the stacked */
	/*  dead-coroutines being deallocated via coclean()). */
	/*  Note that cocollapse() WILL NOT deallocate or call conclude() on the */
	/*  thread's initial-coro, for various reasons including because that */
	/*  would call for the thread's primary stack to be deallocated, which */
	/*  might not be possible for free(), and the coroutine system doesn't */
	/*  attempt to find the relevant address regardless... besides which, */
	/*  that would very likely cause some sort of major malfunction. */
	/* coclean() : Deallocates entries from the singly-linked list of "dead" */
	/*  coroutines that is populated by cocollapse(). This list is likely to */
	/*  usually have no more than one entry, but TECHNICALLY may have more in */
	/*  situations that are at risk of producing data races. */
/*
	TODO: Figure out what missing functionality there is- I keep thinking of
		SOMETHING, but haven't spent the time to nail it down.
*/

uintptr_t get_defaultstacksize();


	/* Not immediately sure how many of these are used. */
enum
{
	CORO_BADSTATE = -2,
	CORO_BADARGS = -1,
	CORO_INVALID = 0,
	CORO_WORKING = 1,
	CORO_DONE
};

/* These two represent the bare-minimum of a stack frame. The memory */
/*  layout SHOULD be correct. They aren't used by the coro system, but users */
/*  might find some value in them as templates. */
	/* This represents the layout of a minimal stack frame. Though I don't */
	/*  think it's really minimal. */
typedef struct dummyframe
{
	/* For at least MSVC targeting Win64, this layout should be correct. Note */
	/*  that reg[ 0 ] should correspond to the location of the first argument */
	/*  on 64-bit MSVC, and most or all 32-bit calling conventions, but for */
	/*  SysV 64-bit purposes (including Linux) reg[ 0 ] would instead */
	/*  correspond to the SEVENTH argument, as the earlier arguments are ONLY */
	/*  stored in registers instead of having shadow space like they do in */
	/*  the MSVC calling convention. Regardless, IF you can correctly equate */
	/*  an argument address to reg[ 0 ], then that is enough to allow you to */
	/*  find the return address, and to work up & down from that, especially */
	/*  if your compilation options force the use of a frame pointer (and */
	/*  thus a copy of an old frame pointer on the stack). */
	
	void *ret;
		/* Just some rando number of slots. */
		/* Note: for MSWin x64, this must be AT LEAST 4, while on LInux & co. */
		/*  this is only used for args past the first 6, so we go with the MS */
		/*  approach. */
	void *reg[ 4 ];
	
} dummyframe;
	/* Slightly less minimal, assumes a base-pointer / frame-pointer. */
typedef struct dummyframe_bp
{
	/* For at least MSVC targeting Win64, this layout should be correct. */
	
	void *bp;
	void *ret;
		/* Just some rando number of slots. */
		/* Note: for MSWin x64, this must be AT LEAST 4, while on LInux & co. */
		/*  this is only used for args past the first 6, so we go with the MS */
		/*  approach. */
	void *reg[ 4 ];
	
} dummyframe_bp;



typedef struct corohead corohead;
	/* Pointers to this mark the lowest address of a coroutine stack. */
typedef struct corobody
{
	char corostack[ 1 ];
	
} corobody;

	/* Normally the highest-addressed occupant of an individual stack. */
struct corohead
{
	void *exit_retaddr_MSVC;
	
	corohead *here;
	corobody *lastbyte_a;
		/* Marked volatile for the sake of the destruction code: conclude() */
		/*  should null itself out upon completion of it's task. */
	volatile int (*conclude)( corohead*, uintptr_t );
	
	void *exit_retaddr_SysV;
	corobody *lastbyte_b;
	
	uintptr_t auxiliary;
	
	jmp_buf state;
};
	/* Note that the stack frame for this SHOULD perfectly overlap this */
	/*  argument list with the correspondingly typed members of corohead{}. */
	/*  This may seem odd, but stack elements are allocated to growing */
	/*  addresses as encountered, while arguments are ALSO allocated to */
	/*  growing addresses as encountered... but the other way around, they're */
	/*  allocated to FALLING addresses, but in the REVERSE of of the order */
	/*  that they are encountered, negative * negative == positive, thus the */
	/*  direction is the same despite sorta being the opposite. */
void cocollapse
(
	corohead *head,
	corobody *body,
	int (*conclude)( corohead*, uintptr_t )
);

int cocontext( void *data, int (*func)( void* ) );
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
	);
		uintptr_t coro_getaux();
		int coyield( corohead *dest );
	void coclean();


	/* The initial-coroutine itself. This is provided so that it can act as a */
	/*  target for coyield(). */
extern __thread corohead main_fiber;
extern volatile char *coro_errmsg;
