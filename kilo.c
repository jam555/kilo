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

#include "kilo.h"
#include "coroutine/coro.h"



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
char *C_HL_extensions[] = {".c",".h",".cpp",".hpp",".cc",NULL};
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


#warning "Move error-exit messages to something that an atexit() handler will print."



int main_coro( void *ign );
int argn;
char **args;
int main( int argn_, char **args_ )
{
	argn = argn_;
	args = args_;
	
		/* Wrap, and continue with main(). */
		/* NOte that the void pointer will probably need to be non-null */
		/*  at some point in the future. */
	argn_ = cocontext( (void*)0, &main_coro );
	/* Let's just trash the return for now. */
}
int main_coro( void *ign )
{
    (void)ign;
	
	const char noaltscr_opt[] = "--no-alt-screen";
    if( argn < 2 || argn > 3 )
	{
        fprintf( stderr, "Usage: kilo <filename> [%s]\n", noaltscr_opt );
        exit( 1 );
    }

    if( argn == 3 ) {
        /* Surpress usage of the alternate screen: useful if you */
        /*  want to keep info displayed on exit. */
        if( strcmp( noaltscr_opt, args[ 2 ] ) != 0 ) {
            perror( "Unfamiliar command-line option:" );
            fprintf( stderr, "  %s", args[ 2 ] );
            exit( 1 );
        }
        E.no_altscr = 1;
		
    } else {
        
		E.no_altscr = 0;
    }

    initEditor();
    editorSelectSyntaxHighlight( args[ 1 ] );
    editorOpen( args[ 1 ] );
    enableRawMode( STDIN_FILENO );
		/* TODO: This message needs to be displayed by default! */
		/* Note that the max length for a line is currentlt UINT32_MAX stored characters (NOT displayed characters). */
    editorSetStatusMessage( "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find" );
    while( 1 )
	{
        editorRefreshScreen();
			/* TODO: Subject this to a mode switch! */
			/*  If mode != notepad, then run input through CLI mode! */
			/*  For CLI mode, try to use "linenoise" from the same author. */
        editorProcessKeypress( STDIN_FILENO );
    }
    return 0;
}
