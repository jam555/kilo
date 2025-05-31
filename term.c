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

#include "kilo.h"



#define MILA_TERMCODES_11 "\x1b[7m"


/* ======================= Low level terminal handling ====================== */

struct termios orig_termios; /* In order to restore at exit.*/



int mila_initterm_xterm()
{
    char *termstr = getenv( "TERM" );
    if( termstr && strstr( termstr, "xterm" ) )
    {
        /* Activate alternate screen. To disable, use 'l' instead of 'h'. */
		
#define MILA_TERMCODES_23 "\x1b[?1049h\n"
		
		const char altscren[] = MILA_TERMCODES_23;
		const size_t altscren_len = sizeof( altscren );
		ssize_t res = write( STDOUT_FILENO, altscren, altscren_len );
		if( res < 0 || (size_t)res != altscren_len )
		{
#warning "This should be expanded to handle incomplete writes."
			perror( "Unable to select the alternate screen display buffer" );
			exit( 1 );
		}
		E.altscr = 1;
		
		return( 1 );
	}
	
	return( -1 );
}
void mila_ab_curseek( struct abuf *ab, int argn,   size_t x, size_t y, char *tail )
{
	char buf[ 32 ];
	
#define MILA_TERMCODES_8 "\x1b[H%s" /* Go home. */
#define mila_ab_curseek_ONEARG "\x1b[%zuH%s"
#define MILA_TERMCODES_21 "\x1b[%zu;%zuH%s"
	
	if( !tail )
	{
		tail = "";
	}
	
	/* Populate the buffer. */
	if( argn == 0 )
	{
		snprintf( buf, sizeof(buf), MILA_TERMCODES_8,  tail );
		
	} else if( argn == 1 )
	{
		snprintf( buf, sizeof(buf), mila_ab_curseek_ONEARG, x,  tail );
		
	} else if( argn == 2 )
	{
		snprintf( buf, sizeof(buf), MILA_TERMCODES_21, x, y,  tail );
	}
	
	abAppend( ab, buf, strlen( buf ) );
}
void mila_ab_curseek_home( struct abuf *ab )
{
	mila_ab_curseek( ab, 0,   0, 0, "" );
}

void mila_ab_curvis_hide( struct abuf *ab )
{
#define MILA_TERMCODES_7 "\x1b[?25l"
    abAppend( ab, MILA_TERMCODES_7, 6 ); /* Hide cursor. */
}
void mila_ab_curvis_show( struct abuf *ab )
{
#define MILA_TERMCODES_22 "\x1b[?25h"
    abAppend( ab, MILA_TERMCODES_22, 6 ); /* Show cursor. */
}

void mila_ab_clearall( struct abuf *ab )
{
#define mila_ab_clearall_TERMCODE "\x1b[2K\r\n"
	abAppend( ab, mila_ab_clearall_TERMCODE, 7 );
}
void mila_ab_cleartostart( struct abuf *ab )
{
#define mila_ab_cleartostart_TERMCODE "\x1b[1K\r\n"
	abAppend( ab, mila_ab_cleartostart_TERMCODE, 7 );
}
void mila_ab_cleartoend( struct abuf *ab, char *tail )
{
	char buf[ 32 ];
	
	if( !tail )
	{
		tail = "";
	}
	
#define mila_ab_cleartoend_TERMCODE "\x1b[0K%s"
	snprintf( buf, sizeof( buf ), mila_ab_cleartoend_TERMCODE,  tail );
	
	abAppend( ab, buf, strlen( buf ) );
}

void mila_ab_defaultFg( struct abuf *ab )
{
#define MILA_TERMCODES_13 "\x1b[39m"
	abAppend( ab, MILA_TERMCODES_13, 5 );
}
void mila_ab_swapFgBg( struct abuf *ab )
{
	abAppend( ab, MILA_TERMCODES_11, 4 );
}
void mila_ab_resetAttribs( struct abuf *ab, char *tail )
{
	char buf[ 32 ];
	
	if( !tail )
	{
		tail = "";
	}
	
#define MILA_TERMCODES_12 "\x1b[0m%s"
	snprintf( buf, sizeof( buf ), MILA_TERMCODES_12,  tail );
	
	abAppend( ab, buf, strlen( buf ) );
}

void mila_term_printWelcomeMessage( struct abuf *ab,  char *buf, size_t buflen )
{
	/* buf == welcome, buflen == sizeof( welcome ) */
#define MILA_TERMCODES_9 "\x1b[0K"
	int welcomelen =
		snprintf
		(
			buf, buflen,
			
			"Kilo editor -- verison %s%s\r\n",
			KILO_VERSION, MILA_TERMCODES_9
		);
	size_t padding;
	if( welcomelen < 0 )
	{
		fprintf
		(
			stderr,
				"welcomelen in mila_term_printWelcomeMessage() had a strange value: %d\n",
				welcomelen
		);
		exit( 1 );
		
	} else {
		
		padding = ( E.screencols - (size_t)welcomelen ) / 2;
	}
	if( padding )
	{
		abAppend( ab, "~", 1 );
		padding--;
	}
	while( padding-- )
	{
		abAppend( ab, " ", 1 );
	}
	abAppend( ab, buf, (size_t)welcomelen );
}
void mila_term_setcolor
(
	struct abuf *ab,
	char *buf, size_t buflen,
	
	int color, int *curcolor
)
{
#define MILA_TERMCODES_14 "\x1b[%dm"
	int clen =
		snprintf
		(
			buf, buflen,
			MILA_TERMCODES_14, color
		);
	if( clen < 0 )
	{
		exit ( 1 );
	}
	*curcolor = color;
	abAppend( ab, buf, (size_t)clen );
}



void mila_term_cursseek_setpos( int alter, int ofile, size_t row, size_t col )
{
	char seq[ 32 ];
	
#warning "Numeric results haven't been verified: note the \"row < 0\" and \"col < 0 \" cases."
		/* Normalize coordinate. */
	if( (int)row < 0 )
	{
		row = E.screenrows - row;
	}
	if( (int)col < 0 )
	{
		col = E.screencols - col;
	}
	
	if( alter >= 0 )
	{
		/* Restore position. */
		
#define MILA_TERMCODES_6 "\x1b[%zu;%zuH"
		snprintf( seq, 32, MILA_TERMCODES_6, row, col );
		if( write( ofile, seq, strlen( seq ) ) == -1 )
		{
			/* Can't recover... */
		}
		
	} else if( alter == -1 )
	{
		row -= E.cx;
		col -= E.cy;
		
		CU_jumptarget: ;
		
#warning "This set of if()s should be modified to allow all cases to be used."
		if( (int)row < 0 )
		{
			row = -row;
			snprintf( seq, 32, "\x1b[%zuA", row );
			if( write( ofile, seq, strlen( seq ) ) == -1 )
			{
				/* Can't recover... */
			}
			
		} else if( (int)row > 0 )
		{
			snprintf( seq, 32, "\x1b[%zuB", row );
			if( write( ofile, seq, strlen(seq ) ) == -1 )
			{
				/* Can't recover... */
			}
		}
		if( (int)col < 0 )
		{
			col = -col;
			snprintf( seq, 32, "\x1b[%zuD", col );
			if( write( ofile, seq, strlen( seq ) ) == -1 )
			{
				/* Can't recover... */
			}
			
		} else if( (int)col > 0 )
		{
			snprintf( seq, 32, "\x1b[%zuC", col );
			if( write( ofile, seq, strlen( seq ) ) == -1 )
			{
				/* Can't recover... */
			}
		}
		
	} else if( alter == -2 )
	{
			/* No reason to duplicate that... */
		goto CU_jumptarget;
	}
}
int mila_term_cursseek_finalchar( int alter )
{
	if( alter >= 0 )
	{
		/* (+1,+1) because the screen size is described with C indexing on our */
		/*  side, but 1-based indexing on the terminal side. */
		mila_term_cursseek_setpos
		(
			alter,
			STDOUT_FILENO,
			E.screenrows + 1,
			E.screencols + 1
		);
		return( 1 );
		
	} else if( alter == -1 )
	{
		/* Alternate case: seek to some insane point. */
#define MILA_TERMCODES_5 "\x1b[999C\x1b[999B"
		mila_term_cursseek_setpos( -1, STDOUT_FILENO, 999, 999 );
		return( 1 );
	}
	
	return( -1 );
}

void disableRawMode( int fd )
{
    /* Don't even check the return value as it's too late. */
    if( E.rawmode )
	{
        tcsetattr( fd, TCSAFLUSH, &orig_termios );
        E.rawmode = 0;
    }
}
void mila_term_altscreen_disable( void )
{
	if( E.altscr )
	{
        /* "?47l" ~1978 VT100 DECSET magic. "?1049l" is similar xterm magic */
		/*  from... some indeterminate time, possibly even before X Windows */
		/*  existed. */
            /* To instead enable, use 'h' instead of 'l': note that */
            /*  initEditor() does the enabling already. */
#define MILA_TERMCODES_1 "\x1b[?1049l"
        const char altscren[] = MILA_TERMCODES_1;
        const size_t altscren_len = sizeof( altscren );
		ssize_t res = write( STDOUT_FILENO, altscren, altscren_len );
        if( res && (size_t)res != altscren_len )
		{
            perror( "Unable to deselect the alternate screen display buffer" );
            perror( "please type" );
#define MILA_TERMCODES_2 "\"\\e[?1049l\""
            fprintf( stderr, "  echo -e %s", MILA_TERMCODES_2 );
            perror( "and then hit your enter key" );
            exit( 1 );
        }
        E.altscr = 0;
	}
}

/* Called at exit to avoid remaining in raw mode. */
void editorAtExit( void )
{
    disableRawMode( STDIN_FILENO );

    if( E.altscr )
	{
       /* Disable alternate screen. */
       mila_term_altscreen_disable();
	   
    } else if( E.no_altscr )
	{
        /* If we aren't using the alternate-screen, move the cursor to the */
        /*  end of the screen and force a line-advance instead, to prepare */
        /*  for the return to the CLI. */
        mila_term_cursseek_finalchar( 0 );
		printf( "\n\n" );
    }
}

/* Raw mode: 1960 magic shit. */
int enableRawMode( int fd )
{
    struct termios raw;

    if( E.rawmode ) return 0; /* Already enabled. */
    if( !isatty( STDIN_FILENO ) ) goto fatal;
    atexit( editorAtExit );
    if( tcgetattr( fd, &orig_termios ) == -1 ) goto fatal;
		/* To support the move to multi-doc capabilities. */
	E.orig_termios = orig_termios;

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
    E.rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
int editorReadKey( int fd )
{
    ssize_t nread;
    char c, seq[ 3 ];
#warning "This is a prime candidate for a yield-based IO routine."
    while
	(
		(
			nread = read( fd, &c, 1 )
		) == 0
	)
	{
		;
	}
    if( nread == -1 ) exit( 1 );

    while( 1 )
	{
        switch( c )
		{
	        case ESC:    /* escape sequence */
	            /* If this is just an ESC, we'll timeout here. */
	            if( read( fd, seq, 1 ) == 0 ) return ESC;
	            if( read( fd, seq + 1, 1 ) == 0 ) return ESC;
	
	            /* ESC [ sequences. */
	            if( seq[ 0 ] == '[' )
				{
	                if( seq[ 1 ] >= '0' && seq[ 1 ] <= '9' )
					{
	                    /* Extended escape, read additional byte. */
	                    if( read( fd, seq + 2, 1 ) == 0 ) return ESC;
	                    if( seq[ 2 ] == '~')
						{
	                        switch( seq[ 1 ] )
							{
		                        case '3': return DEL_KEY;
		                        case '5': return PAGE_UP;
		                        case '6': return PAGE_DOWN;
								default:
									perror( "Numeric \"ESC [\" in editorReadKey() had a strange value: " );
									fputc( seq[ 1 ], stderr );
									exit( 1 );
	                        }
	                    }
						
	                } else {
	                    
						switch( seq[ 1 ] )
						{
		                    case 'A': return ARROW_UP;
		                    case 'B': return ARROW_DOWN;
		                    case 'C': return ARROW_RIGHT;
		                    case 'D': return ARROW_LEFT;
		                    case 'H': return HOME_KEY;
		                    case 'F': return END_KEY;
							default:
								perror( "Non-numeric \"ESC [\" in editorReadKey() had a strange value: " );
								fputc( seq[ 1 ], stderr );
								exit( 1 );
	                    }
	                }
	            }
	
	            /* ESC O sequences. */
	            else if( seq[ 0 ] == 'O' )
				{
	                switch( seq[ 1 ] )
					{
		                case 'H': return HOME_KEY;
		                case 'F': return END_KEY;
		                default:
							perror( "\"ESC O\" in editorReadKey() had a strange value: " );
							fputc( seq[ 1 ], stderr );
							exit( 1 );
	                }
	            }
	            break;
	        default:
	            return c;
        }
    }
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned. */
int getCursorPosition( int ifd, int ofd, size_t *rows, size_t *cols )
{
    char buf[ 32 ];
    unsigned int i = 0;

    /* Report cursor location */
#define MILA_TERMCODES_4 "\x1b[6n"
    if( write( ofd, MILA_TERMCODES_4, 4 ) != 4 ) return -1;

    /* Read the response: ESC [ rows ; cols R */
    while( i < sizeof( buf ) - 1 )
	{
        if( read( ifd, buf + i, 1 ) != 1 ) break;
        if( buf[ i ] == 'R') break;
        i++;
    }
    buf[ i ] = '\0';

    /* Parse it. */
    if( buf[ 0 ] != ESC || buf[ 1 ] != '[' ) return -1;
    if( sscanf( buf + 2, "%zu;%zu", rows, cols ) != 2 ) return -1;
    return 0;
}

/* Try to get the number of columns in the current terminal. If the ioctl()
 * call fails the function will try to query the terminal itself.
 * Returns 0 on success, -1 on error. */
int getWindowSize( int ifd, int ofd, size_t *rows, size_t *cols )
{
    struct winsize ws;

    if( ioctl( 1, TIOCGWINSZ, &ws ) == -1 || ws.ws_col == 0 )
	{
        /* ioctl() failed. Try to query the terminal itself. */
        size_t orig_row, orig_col;
		int retval;

        /* Get the initial position so we can restore it later. */
        retval = getCursorPosition( ifd, ofd, &orig_row, &orig_col );
        if( retval == -1 ) goto failed;

        /* Go to right/bottom margin and get position. */
		if( !mila_term_cursseek_finalchar( -1 ) ) goto failed;
        retval = getCursorPosition( ifd, ofd, rows, cols );
        if( retval == -1 ) goto failed;

        /* Restore position. */
        mila_term_cursseek_setpos( 0, ofd, orig_row, orig_col );
        return 0;
		
    } else {
        
		*cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
	
failed:
    return -1;
}
