/* Mila -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * Note that this is actually an I/O test.
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



#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>



size_t tablen = 8;
void print_tool( char *text, ... )
{
	static char buf[ 161 ];
	va_list args;
	va_start( args, text );
	
		vsnprintf
		(
			buf, sizeof( buf ),
			
			text, args
		);
		
		size_t len = strlen( buf ), iter = 0;
		
		while( buf[ iter ] != '\0' )
		{
			if( buf[ iter ] == '\t' )
			{
				len += ( tablen - 1 );
			}
			++iter;
		}
		
		printf( buf );
		printf( "\n\x1b[%zuD", len );
	
	va_end( args );
}
void print_antitab()
{
	printf( "\n\x1b[%zuD", tablen );
}



enum KEY_ACTION{
        KEY_NULL = 0,       /* NULL */
        CTRL_C = 3,         /* Ctrl-c */
        CTRL_D = 4,         /* Ctrl-d */
        CTRL_F = 6,         /* Ctrl-f */
        CTRL_H = 8,         /* Ctrl-h */
        TAB = 9,            /* Tab */
        CTRL_L = 12,        /* Ctrl+l */
        ENTER = 13,         /* Enter */
        CTRL_Q = 17,        /* Ctrl-q */
        CTRL_S = 19,        /* Ctrl-s */
        CTRL_U = 21,        /* Ctrl-u */
        ESC = 27,           /* Escape */
        BACKSPACE =  127,   /* Backspace */
        /* The following are just soft codes, not really reported by the
         * terminal directly. */
        ARROW_LEFT = 1000,
        ARROW_RIGHT,
        ARROW_UP,
        ARROW_DOWN,
        DEL_KEY,
        HOME_KEY,
        END_KEY,
        PAGE_UP,
        PAGE_DOWN,
		KEYBOARD_TIMEOUT
};



int fd = STDIN_FILENO, display_test;
intptr_t display_pointer;
struct termios raw, orig_termios;
struct tm display_time, t2, t3;


/* Raw mode: 1960 magic shit. */
int enableRawMode()
{
    if( !isatty( fd ) ) goto fatal;
    if( tcgetattr( fd, &orig_termios ) == -1 ) goto fatal;
		/* To support the move to multi-doc capabilities. */
	
    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~( (tcflag_t)( BRKINT | ICRNL | INPCK | ISTRIP | IXON ) );
    /* output modes - disable post processing */
    raw.c_oflag &= ~( (tcflag_t)( OPOST ) );
    /* control modes - set 8 bit chars */
    raw.c_cflag |= ( CS8 );
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~( (tcflag_t)( ECHO | ICANON | IEXTEN | ISIG ) );
    /* control chars - set return condition: min number of bytes and timer. */
    raw.c_cc[ VMIN ] = 0; /* Return each byte, or zero for timeout. */
    raw.c_cc[ VTIME ] = 1; /* 100 ms timeout (unit is tens of second). */

    /* put terminal in raw mode after flushing */
    if( tcsetattr( fd, TCSAFLUSH, &raw ) < 0 ) goto fatal;
	
	/* Prefer non-blocking behavior. */
	if( 1 )
	{
		int res = fcntl( fd, F_GETFL ), tmp;
		if( res < 0 )
		{
			tmp = errno;
			print_tool
			(
				"\n\tenableRawMode()::fcntl()1 failed: res == %d, errno == %d.\n",
				res,
				tmp
			);
			exit( 1 );
		}
		
		res = fcntl( fd, F_SETFL, res | O_NONBLOCK );
		if( res < 0 )
		{
			tmp = errno;
			print_tool
			(
				"\n\tenableRawMode()::fcntl()2 failed: res == %d, errno == %d.\n",
				res,
				tmp
			);
			exit( 1 );
		}
	}
	
    return 1;
	
fatal:
    errno = ENOTTY;
    return -1;
}
void disableRawMode()
{
	tcsetattr( fd, TCSAFLUSH, &orig_termios );
	
	if( 1 )
	{
		int res = fcntl( fd, F_GETFL ), tmp;
		if( res < 0 )
		{
			tmp = errno;
			print_tool
			(
				"\n\tdisableRawMode()::fcntl()1 failed: res == %d, errno == %d.\n",
					res,
					tmp
			);
			exit( 1 );
		}
		
		res = fcntl( fd, F_SETFL, res & ~O_NONBLOCK );
		if( res < 0 )
		{
			tmp = errno;
			print_tool
			(
				"\n\tdisableRawMode()::fcntl()2 failed: res == %d, errno == %d.\n",
					res,
					tmp
			);
			exit( 1 );
		}
	}
}

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
int editorReadKey()
{
	static char text[ 18 ] =
		{
			'\0', '\0', ' ', ' ',
			'\0', '\0', ' ', ' ',
			'\0', '\0', ' ', ' ',
			'\0', '\0', ' ', ' ',
			
			'\0', '\0'
		};
    ssize_t nread;
    char c, seq[ 4 ] = { '\0', '\0', '\0', '\0' };
	time_t t;
    int res, ret = EOF, e;
#define editorReadKey_ONRET( val ) { ret = (val); goto onret; }
	
	errno = 0;
	
	if( ret != EOF )
	{
		onret:
		
		if( 1 )
		{
			t = time( (time_t*)0 );
			t3 = *localtime( &t );
		}
		
		if( seq[ res ] != 0 )
		{
			text[ 12 ] = '\0';
			
			res = 0;
			while( res < 4 )
			{
				text[ ( res * 4 ) + 1 ] =
					"0123456789ABCDEF"[ seq[ res ] & 15 ];
				text[ res * 4 ] =
					"0123456789ABCDEF"[ ( ( seq[ res ] & ~(char)15 ) / 16 ) & 15 ];
				if( isprint( seq[ res ] ) )
				{
					text[ ( res * 4 ) + 2 ] = seq[ res ];
					
				} else {
					
					text[ ( res * 4 ) + 2 ] = ' ';
				}
				
				++res;
			}
		}
		if( ret != KEYBOARD_TIMEOUT )
		{
			print_tool
			(
				"\ttime == %2.2d:%2.2d, %2.2d:%2.2d, %2.2d:%2.2d; "
				"editorReadKey(): %s; ret: %x; "
				"d test: %d; nread: %d; errno: %d",
					display_time.tm_min,
					display_time.tm_sec,
					
					t2.tm_min,
					t2.tm_sec,
					
					t3.tm_min,
					t3.tm_sec,
					
					text,
					ret,
					
					display_test,
					nread,
					e
			);
		}
		return( ret );
	}
	
	if( 1 )
	{
		t = time( (time_t*)0 );
		display_time = *localtime( &t );
	}
	while
	(
		(
			nread = read( fd, seq, 1 )
		) == 0 ||
		( res = errno ) == EAGAIN ||
		( res = errno ) == EWOULDBLOCK
	)
	{
		errno = 0;
		if( res == EAGAIN || res == EWOULDBLOCK )
		{
			editorReadKey_ONRET( KEYBOARD_TIMEOUT );
		}
	}
    if( nread == -1 )
	{
		int e = errno;
		print_tool
		(
			"\n\teditorReadKey()::read() failed: res == %d, err == %d.\n",
			(int)nread, e
		);
		exit( 1 );
	}
	// display_test = nread;
	
	if( 0 )
	{
		t = time( (time_t*)0 );
		t2 = *localtime( &t );
	}
	
    while( 1 )
	{
		if( 0 )
		{
			t = time( (time_t*)0 );
			display_time = *localtime( &t );
		}
        switch( seq[ 0 ] )
		{
	        case ESC:    /* escape sequence */
				if( 0 )
				{
					t = time( (time_t*)0 );
					display_time = *localtime( &t );
				}
				seq[ 2 ] = '\0';
				seq[ 3 ] = '\0';
				
	            /* If this is just an ESC, we'll timeout here. */
					/* This DOESN'T seem to timeout. */
					/*
						Do we want a wrapper for the character read? Use w/ the first while?
					*/
				/* Timeout seems random? And some key presses seem to get missed? */
				/*
					One or another of these read()s causes a lock-up, figure out how to hunt for it.
				*/
				if( 1 )
				{
					t = time( (time_t*)0 );
					t2 = *localtime( &t );
				}
				errno = 0;
				if( ( nread = read( fd, seq + 1, 1 ) ) == 0 || seq[ 1 ] == ESC )
				{
					/* For some reason it's ALWAYS this that provides ESC... */
					/* How ARE we supposed to do single-escape detection? */
					e = errno;
					
					if( 1 )
					{
						t = time( (time_t*)0 );
						t3 = *localtime( &t );
					}
					
					display_test = 1;
					
					editorReadKey_ONRET( ESC );
					
				} else if( ( nread = read( fd, seq + 1, 1 ) ) == 0 )
				{
					e = errno;
					display_test = 2;
					
					if( 1 )
					{
						t = time( (time_t*)0 );
						t3 = *localtime( &t );
					}
					
					/* Do we REALLY want multiple read()s? Should we have ALL of them be the same in the coroutine version? */
					
					editorReadKey_ONRET( ESC );
				}
				e = errno;
				display_test = 3;
				if( 1 )
				{
					t = time( (time_t*)0 );
					t3 = *localtime( &t );
				}
				
	            /* ESC [ sequences. */
	            if( seq[ 1 ] == '[' )
				{
	                if( seq[ 2 ] >= '0' && seq[ 1 ] <= '9' )
					{
	                    /* Extended escape, read additional byte. */
	                    if( read( fd, seq + 3, 1 ) == 0 )
						{
							editorReadKey_ONRET( ESC );
						}
	                    if( seq[ 3 ] == '~')
						{
	                        switch( seq[ 2 ] )
							{
		                        case '3':
									editorReadKey_ONRET( DEL_KEY );
		                        case '5':
									editorReadKey_ONRET( PAGE_UP );
		                        case '6':
									editorReadKey_ONRET( PAGE_DOWN );
								default:
									print_tool
									(
										"\tNumeric \"ESC [\" in editorReadKey() had a strange value: %c\n",
										seq[ 2 ]
									);
									exit( 1 );
	                        }
	                    }
						
	                } else {
	                    
						switch( seq[ 2 ] )
						{
		                    case 'A':
								editorReadKey_ONRET( ARROW_UP );
		                    case 'B':
								editorReadKey_ONRET( ARROW_DOWN );
		                    case 'C':
								editorReadKey_ONRET( ARROW_RIGHT );
		                    case 'D':
								editorReadKey_ONRET( ARROW_LEFT );
		                    case 'H':
								editorReadKey_ONRET( HOME_KEY );
		                    case 'F':
								editorReadKey_ONRET( END_KEY );
							default:
								print_tool
								(
									"\tNon-numeric \"ESC [\" in editorReadKey() had a strange value: %c\n",
									seq[ 2 ]
								);
								exit( 1 );
	                    }
	                }
					
	            } else if( seq[ 1 ] == 'O' )
				{
	            	/* ESC O sequences. */
	                
					switch( seq[ 2 ] )
					{
		                case 'H':
							editorReadKey_ONRET( HOME_KEY );
		                case 'F':
							editorReadKey_ONRET( END_KEY );
		                default:
							print_tool
							(
								"\t\"ESC O\" in editorReadKey() had a strange value: %c\n",
								seq[ 2 ]
							);
							exit( 1 );
	                }
	            }
	            break;
	        default:
				if( 0 )
				{
					t = time( (time_t*)0 );
					display_time = *localtime( &t );
					
					print_tool
					(
						"\t\tdefault value: %d",
						(int)( seq[ 0 ] )
					);
				}
				editorReadKey_ONRET( seq[ 0 ] );
        }
    }
}



int main( int argn, char *args[] )
{
	int res = ' ';
	if( !enableRawMode() )
	{
		disableRawMode();
	}
	
	printf( "\n" );
	print_tool( "Entering kilo::rawinput.c" );
	print_tool( "\tTo exit, press q.\n" );
	
	while( 1 && res != 'q' && res != 'Q' )
	{
		res = editorReadKey();
		if( res != KEYBOARD_TIMEOUT )
		{
			// printf( "\t\tLoop print: %d\n", res );
		}
	}
	if( 0 )
	{
		print_tool( "\tPrint test 2." );
		print_tool( "\tPrint test 2." );
		print_tool( "\tPrint test 2." );
		print_tool( "\tPrint test 2." );
	}
	
	print_antitab();
	print_tool( "\nExiting kilo::rawinput.c\n" );
	
	disableRawMode();
}
