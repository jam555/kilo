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

















#if GCC
	/* THis is actually a length defined by the linker, but this is how it appears. */
extern void *__STACK_SIZE;
#elif 0
/* MSVC++ stuff: what's the C version? Might need to just wrap it. */
property int StackReserveSize { int get(); void set(int value); };
#endif

	/* This only works for CC & similar. */
uintptr_t get_defaultstacksize()
{
	return( (uintptr_t)__STACK_SIZE );
}







	/* This represents the layout of a minimal stack frame. Though I don't think it's really minimal. */
struct dummyframe
{
	/* For at least MSVC targeting Win64, this layout should be correct. */
	
	void *ret;
		/* Just some rando number of slots. */
		/* Note: for MSWin x64, this must be AT LEAST 4, while on LInux & co. */
		/*  this is only used for args past the first 6, so we go with the MS */
		/*  approach. */
	void *reg[ 4 ];
};
	/* Slightly less minimal, assumes a base-pointer. */
struct dummyframe_bp
{
	/* For at least MSVC targeting Win64, this layout should be correct. */
	
	void *bp;
	void *ret;
		/* Just some rando number of slots. */
		/* Note: for MSWin x64, this must be AT LEAST 4, while on LInux & co. */
		/*  this is only used for args past the first 6, so we go with the MS */
		/*  approach. */
	void *reg[ 4 ];
};


typedef struct corohead corohead;

	/* Pointers to this mark the lowest address of a coroutine stack. */
typedef struct corobody
{
	char corostack[];
	
} corobody;

	/* Normally the highest-addressed occupant of an individual stack. */
struct corohead
{
	void *exit_retaddr_MSVC;
	
	corohead *this; /* TODO: rename this variable. */
	corobody *lastbyte_a;
	int (*conclude)( uintptr_t );
	
	void *exit_retaddr_SysV;
	corobody *lastbyte_b;
	
	uintptr_t auxiliary;
	
	volatile jmp_buf state;
};
volatile thread corohead main_fiber = { 0 }, *current_fiber = 0;

	/* Note that the stack frame for this SHOULD perfectly overlap this */
	/*  argument list with the correspondingly typed members of corohead{}. */
	/*  This may seem odd, but stack elements are allocated to growing */
	/*  addresses as encountered, while arguments are ALSO allocated to */
	/*  growing addresses as encountered... but the other way around, they're */
	/*  allocated to FALLING addresses, but in the REVERSE of of the order */
	/*  that they are encountered, negative * negative == positive, thus the */
	/*  direction is the same despite sorta being the opposite. */
void cocollapse( corohead *head, corobody *body, int (*conclude)( uintptr_t ) );
uintptr_t coro_getaux()
{
	if( current_fiber )
	{
		return( current_fiber->auxiliary );
	}
	
	return( 0 );
}




/*
Stuff to still build:
	cocollapse()
	A "build main_fiber" function:
		Note that this should ACTUALLY involve using cobuild() to build a "dummy target"
		for the jmp_buf.
		Also, do-nothing funcs to initialize it to.
	A "free coroutine" function (or more likely, function-complex), to deallocate the coroutine allocations cleanly.
	;
*/





#define get_sp(p) \
  asm volatile("movq %%rsp, %0" : "=r"(p))
#define get_fp(p) \
  asm volatile("movq %%rbp, %0" : "=r"(p))
#define set_sp(p) \
  asm volatile("movq %0, %%rsp" : : "r"(p))
#define set_fp(p) \
  asm volatile("movq %0, %%rbp" : : "r"(p))
#define copy_sp2fp() \
	asm volatile("movq %%rsp,  %%rfp")
#define jump_to(p) \
	asm volatile("jmp %0" : : "r"(p))




/* TODO: See what else needs to use this. */
enum { BADARGS = -1, INVALID = 0, WORKING=1, DONE };

int coyield( corohead *dest )
{
	if( dest )
	{
		int res = setjmp( current_fiber->state );
		if( !res )
		{
			current_fiber = dest;
			longjmp( dest->state, WORKING );
		}
		return( res );
	}
	return( BADARGS );
}


/* These two functions actually build and initialize coroutines. */
#if 1
static void coro_boot( corohead *head,  void *coro_data, void (*coro_main)( corohead*, void* ) )
{
		/* ONLY jumps back into cobuild(). */
	longjmp( current_fiber->state, 1 );
	
	coro_main( head, coro_data );
}
#endif
int cobuild
(
	size_t stacksize,
	
	void *coro_data,
	void (*coro_main)( corohead*, void* ),
	uintptr_t coro_auxiliarydata,
	
	int (*conclude)( uintptr_t ),
	
	
	corohead **ret
)
{
	if( sizeof( void* ) > 8 )
	{
		/* We know nothing about this void* format, thus we know nothing about the target! */
		return( -2 );
	}
	
	if( stacksize > sizeof( dummyframe ) * 2 && ret && current_fiber )
	{
		stacksize +=
			sizeof( corohead ) * 2 +
			sizeof( dummyframe ) * 2 +
			sizeof( corobody ) +
			128; /* x86-64 red-zone, used in Linux apps as a safe zone for temporaries by funcs. */
		
		void **alloc = (void**)malloc( stacksize );
		if( !alloc )
		{
			return( -2 );
		}
		
		/* Find the corohead. */
		corohead *head;
		{
			/* Move above the stack, since we grow downwards. */
			uintptr_t head_ = (uintptr_t)alloc;
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
				/* Adjust into alignment. */
			head_ &= ~( (uintptr_t)masks[ sizeof( void* ) ] );
				/* Adjust downwards for ->exit_retaddr, because that's meant */
				/*  to be 8 bytes OUT of 16-byte alignment.  */
			head_ -= sizeof( void* );
			
			head = (corohead*)head_;
			/* Basic header initialization. */
				/* If this EVER gets called, something went VERY wrong. Note that */
				/*  depending on the size of int, exit MIGHT receive ->lastbyte */
				/*  as an argument. */
			head->exit_retaddr_MSVC = &exit;
			head->exit_retaddr_SysV = &exit;
			/* For cocollapse(), these are actually meant to act as it's args. */
			head->this = head;
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
		if( !setjmp( current_fiber->state ) )
		{
			set_sp( alloc );
			/* The setjmp() should be enough to undo the low-level */
			/*  modifications above. */
			
				/* SysV (including Linux) version. The 64-bit MSVC must be in */
				/*  an assembly file.  */
			asm volatile
			(
				/* Override the previous frame pointer, essentially to hide */
				/*  it. */
				"mov $rsp,  %rfp\n"
				
				/* Load the arguments: this isn't needed elsewhere. */
				"popq %rcx\n" /* head */
				"popq %rdx\n" /* coro_data */
				"popq %r8\n" /* coro_main */
				
				/* Restore alignment. */
				"pushq %rcx\n"
				
				/* Call coro_main, the args are already ready. */
				"call %0\n"
				
				/* Pop align pad, & MSVC ret addr. */
				"add $16,  %rsp\n"
				
				/* Note that for an MSVC version, the add above would be */
				/*  just 8, and these would be "movq"s, but wouldn't be in */
				/*  a C file. */
				"pop %rcx\n" /* ch / rcx */
				"pop %rdx\n" /* cb / rdx */
				"pop %r8\n" /* conclude / r8 */
				
				/* Tail-call into cocollapse(). Could technically be the */
				/*  wrong name. */
				"jmp _cocollapse\n"
				: : "r"(&coro_boot)
			);
		}
		
		/* We are done spawning the coroutine/fiber, return it. */
		
		*ret = head;
		return( 1 );
	}
	
	return( -1 );
}

void cocollapse( corohead *head, corobody *body, int (*conclude)( uintptr_t ) )
{
	if( !conclude || !( conclude( head ) ) )
	{
		exit( 1 );
	}
	
	/* Call into the coroutine code that switches to main & deallocates a coroutine: we're ready. */
	??? ( body );
}
