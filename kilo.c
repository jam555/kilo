/* Kilo -- A very simple editor in less than 1-kilo lines of code (as counted
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

#include <sys/time.h>
#include <signal.h>

#include "kilo.h"
#include "coroutine/coro.h"
#include "msgs.h"



char *const kilodesc_string = "Kilo text-editor version: " KILO_VERSION;
char *const thoudesc_string = "Thou terminal-environment version: " THOU_VERSION;



struct editorConfig E;

/* =========================== Syntax highlights DB =========================
 *
 * In order to add a new syntax, define two arrays with a list of file name
 * matches and keywords. The file name matches are used in order to match
 * a given syntax with a given file name: if a match pattern starts with a
 * dot, it is matched as the last past of the filename, for example ".c".
 * Otherwise the pattern is just searched inside the filenme, like "Makefile").
 *
 * The list of keywords to highlight is just a list of words, however if they
 * a trailing '|' character is added at the end, they are highlighted in
 * a different color, so that you can have two different sets of keywords.
 *
 * Finally add a stanza in the HLDB global variable with two two arrays
 * of strings, and a set of flags in order to enable highlighting of
 * comments and numbers.
 *
 * The characters for single and multi line comments must be exactly two
 * and must be provided as well (see the C language example).
 *
 * There is no support to highlight patterns currently. */

/* C / C++ */
	/* Property404 */
char *C_HL_extensions[] = {".c",".h",".cpp",".hpp",".cc",NULL};
	/* Property404 */
char *C_HL_keywords[] = {
	/* C Keywords */
	"auto","break","case","continue","default","do","else","enum",
	"extern","for","goto","if","register","return","sizeof","static",
	"struct","switch","typedef","union","volatile","while","NULL",

	/* C++ Keywords */
	"alignas","alignof","and","and_eq","asm","bitand","bitor","class",
	"compl","constexpr","const_cast","deltype","delete","dynamic_cast",
	"explicit","export","false","friend","inline","mutable","namespace",
	"new","noexcept","not","not_eq","nullptr","operator","or","or_eq",
	"private","protected","public","reinterpret_cast","static_assert",
	"static_cast","template","this","thread_local","throw","true","try",
	"typeid","typename","virtual","xor","xor_eq",

	/* C types */
        "int|","long|","double|","float|","char|","unsigned|","signed|",
        "void|","short|","auto|","const|","bool|",NULL
};

/* Here we define an array of syntax highlights by extensions, keywords,
 * comments delimiters and flags. */
struct editorSyntax HLDB[] =
{
    {
        /* C / C++ */
        C_HL_extensions,
        C_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    }
};
size_t HLBD_entrycount = ( sizeof( HLDB ) / sizeof( HLDB[ 0 ] ) );


typedef void (*sig_handlertype)(int);

static sig_handlertype oldSigVtAlrm = 0 /* , rollingtest */ ;
static volatile int hadVtAlrm = 0;


static void handleSigWinCh( int sig );
	static void delayedHandleSigVtAlrm( int sig );
static void handleSigVtAlrm( int sig );
static signal_links
	sigVtAlrm_hooks = { 0 }, /* Virtual Alarm, a timer that tracks direct execution time. */
	sigWinch_hooks = { 0 }; /* Window CHange,m changes to a window. */

int register_signallink( int sig, signal_links *link )
{
	if( link )
	{
		signal_links *host = 0;
		
		switch( sig )
		{
			case SIGWINCH:
				host = &sigWinch_hooks;
				break;
			case SIGVTALRM:
				host = &sigVtAlrm_hooks;
				break;
			
			default:
				return( -2 );
		}
		
		link->next = host->next;
		link->prev = host;
		if( link->next )
		{
			link->next->prev = link;
		}
		host->next = link;
		
		return( 1 );
	}
	
	return( -1 );
}
int delink_signallink( signal_links *sl )
{
	if( sl )
	{
		if( !( sl->prev ) )
		{
			return( -2 );
		}
		
		sl->prev->next = sl->next;
		if( sl->next )
		{
			sl->next->prev = sl->prev;
		}
		sl->prev = 0;
		sl->next = 0;
		
		return( 1 );
	}
	
	return( -1 );
}

void signallink_dummyhandler( signal_links *sl, int i )
{
	(void)sl;
	(void)i;
}


/* Generic implementation. */
static void handleSigGeneric( int sig, signal_links *link )
{
	signal_links *next = 0;
	
	while( link )
	{
		next = link->next;
		
		if( link->handler )
		{
			link->handler( link, sig );
		}
		
		link = next;
	}
}

/* Specific implementations. */
static void handleSigWinCh( int sig )
{
	signal( SIGWINCH, &handleSigWinCh );
	
	handleSigGeneric( sig, sigWinch_hooks.next );
	
		/* In edevents.c */
	handleSigWinCh2( sig );
}
	static void delayedHandleSigVtAlrm( int sig )
	{
		handleSigGeneric( sig, sigVtAlrm_hooks.next );
	}
static void handleSigVtAlrm( int sig ) /* sig == SIGVTALRM */
{
	signal( SIGVTALRM, &handleSigVtAlrm );
	
		/* Just mark for later handling. */
	hadVtAlrm = 1;
		/* Let's start clearing things for clarity. */
	E.display_test = 0;
	
	if( oldSigVtAlrm )
	{
		oldSigVtAlrm( sig );
	}
}



void main_atexit( void );
int main_coro( void *ign );
void main_noargs_print( void );
void main_args( void );

int argn;
char **args;
int main( int argn_, char **args_ )
{
	argn = argn_;
	args = args_;
	
	fprintf( stdout,  "\nKilo is starting.\n" );
	fflush( stdout );
	
	if( atexit( &main_atexit ) != 0 )
	{
		fprintf( stderr,  "\tatexit() failed to register main_atexit.\n" );
		exit( 1 );
	}
	if( atexit( &msgs_atexit ) != 0 )
	{
		fprintf( stderr,  "\tatexit() failed to register msgs_atexit.\n" );
		exit( 1 );
	}
	
		/* Wrap, and continue with main(). */
		/* Note that the void pointer will probably need to be non-null */
		/*  at some point in the future. */
	argn_ = cocontext( (void*)0, &main_coro );
	/* Let's just trash the return for now. */
}

const char noaltscr_opt[] = "--no-alt-screen";
void main_noargs_print( void )
{
	static char **builddesc_strs;
	if( !builddesc_strs )
	{
		builddesc_strs =
			(char*[])
			{
				kilodesc_string,
				thoudesc_string,
				
			/* These are all from buildid.hpp/.c, and declared in kilo.h. The .c file */
			/*  is preprocessed from the .hpp; for correct values, use the makefile. */
				
				/* Commit-tool info. */
				thou_commithash,
				thou_commitdate,
				thou_workdirstate,
				
				/* Build/Compile info. */
				thou_buildstamp,
				thou_stdcver,
				thou_gccver,
				thou_typewidths,
				
				/* Strings-file info. */
				thou_filestamp,
				
				(char*)0
			};
	}
	
	
	fprintf( stderr, "Usage: kilo <filename> [%s]\n", noaltscr_opt );
			
	fprintf( stderr, "\nKilo / Thou info:\n" );
	char **iter = builddesc_strs;
	while( iter && *iter )
	{
		fprintf( stderr, "\n\t%s\n", *iter );
		
		++iter;
	}
	
	exit( 1 );
}
void main_args( void )
{
	if( argn < 2 || argn > 3 )
	{
		main_noargs_print();
	}
	
	if( argn == 3 )
	{
		/* Surpress usage of the alternate screen: useful if you */
		/*  want to keep info displayed on exit. */
		if( strcmp( noaltscr_opt, args[ 2 ] ) != 0 )
		{
			perror( "Unfamiliar command-line option:" );
			fprintf( stderr, "  %s", args[ 2 ] );
			
			exit( 1 );
		}
		E.no_altscr = 1;
		
	} else {
		
		E.no_altscr = 0;
	}
}

int main_coro( void *ign )
{
    (void)ign;
	
	int res;
	
	main_args();
	
    initEditor();
		/* Leandro Pereira */
		/* Was in initEditor() */
#warning "Switch to sigaction() on at least some platforms."
    signal( SIGWINCH, handleSigWinCh );
	
    editorSelectSyntaxHighlight( args[ 1 ] );
    editorOpen( args[ 1 ] );
    enableRawMode( STDIN_FILENO );
    /* editorSetStatusMessage( "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find" ); */
	/* msgs_build_note( &( E.modemsg ),  "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find" ); */
	if( !modemsgs_setmodal( MODEMSGS_MILLI_MAIN ) )
	{
		msgs_build_fatal( (msgs**)0,  "\tmain_coro():modemsgs_setmodal() failed.\n" );
	}
	
	{
		/* Setup a timer to be delivered via signal(). */
		
		oldSigVtAlrm = signal( SIGVTALRM, &handleSigVtAlrm );
		
		struct itimerval tsigtime;
		
		/* ITIMER_VIRTUAL == Only counts process's direct execution time. */
		res = getitimer( ITIMER_VIRTUAL, &tsigtime );
		if( res != 0 )
		{
			/* Pay attention to errno! Will be EFAULT or EINVAL */
			int e = errno;
			
			msgs_build_fatal
			(
				(msgs**)0,
					
					"\tmain_coro():getitimer( ITIMER_VIRTUAL ) failed with %d.\n",
					e
			);
			exit( 1 );
		}
		
		if( oldSigVtAlrm )
		{
#warning "Check to see if the old timer's values are compatible with our own."
			msgs_build_fatal
			(
				(msgs**)0,
					
					"\tmain_coro() old timer values: interval( %d . %d ), time( %d . %d ).\n",
						(int)tsigtime.it_interval.tv_sec,
						(int)tsigtime.it_interval.tv_usec,
						
						(int)tsigtime.it_value.tv_sec,
						(int)tsigtime.it_value.tv_usec
			);
		}
		
		tsigtime.it_interval.tv_sec =
				/* Remember: MILA_MESSAGESLOTH is in deci-seconds. */
			( MILA_MESSAGESLOTH - ( MILA_MESSAGESLOTH % 10 ) ) / 10;
		tsigtime.it_interval.tv_usec = ( MILA_MESSAGESLOTH % 10 ) * 100000;
		
		tsigtime.it_value = tsigtime.it_interval;
		
		res = setitimer( ITIMER_VIRTUAL, &tsigtime,  (struct itimerval*)0 );
		if( res != 0 )
		{
			/* Pay attention to errno! Will be EFAULT or EINVAL */
			int e = errno;
			
			msgs_build_fatal
			(
				(msgs**)0,
					
					"\tmain_coro():setitimer( ITIMER_VIRTUAL ) failed with %d.\n",
					e
			);
			exit( 1 );
		}
	}
	
	E.stale = 1;
	while( 1 )
	{
		time_t t = time( (time_t*)0 );
			/* This advances, BUT the statview doesn't. */
		/* E.display_test = (int)t; */
		
			/* For whatever reason, this JUST blocks screen draw. Meanwhile, */
			/*  with or without there seems to be a soft-crash. */
			/* CORRECTION, it PROBABLY isn't a soft-crash, just bad lag. */
		if( 1 /* E.stale */ )
		{
	        editorRefreshScreen();
			
			if( E.stale )
			{
				msgs_build_fatal( (msgs**)0,  "\tmain_coro():editorRefreshScreen() didn't clear stale flag.\n" );
				exit( 1 );
			}
		}
		
			/* TODO: Subject this to a mode switch! */
			/*  If mode != notepad, then run input through CLI mode! */
			/*  For CLI mode, try to use "linenoise" from the same author. */
        editorProcessKeypress( STDIN_FILENO );
		
		if( hadVtAlrm )
		{
			hadVtAlrm = 0;
			
			delayedHandleSigVtAlrm( SIGVTALRM );
		}
    }
    return 0;
}
void main_atexit( void )
{
	/* This should be the VERY LAST of the "normal" functions that exit() */
	/*  runs before finishing the exit sequence. */
	
	fflush( stderr );
	fflush( stdout );
	if( E.deathrattle )
	{
		printf( E.deathrattle );
	}
	fprintf( stdout,  "\nKilo is exiting.\n" );
	fflush( stdout );
}
