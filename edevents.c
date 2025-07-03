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
#include "msgs.h"


/* ========================= Editor events handling  ======================== */

/* Handle cursor position change because arrow keys were pressed. */
void editorMoveCursor( int key )
{
	size_t filerow = E.rowoff + E.cy;
    size_t filecol = E.coloff + E.cx;
    erow *row = ( filerow >= E.numrows ) ? NULL : &E.row[ filerow ];

    switch( key )
	{
	    case ARROW_LEFT:
	        if( E.cx == 0 )
			{
	            if( E.coloff )
				{
	                E.coloff--; /* Changes displayed column. */
					
	            } else {
	                
					if( filerow > 0 )
					{
#warning "Verify that this properly handles vertical movement."
	                    E.cy--;
	                    E.cx = E.row[ filerow - 1 ].size;
	                    if( E.cx > E.screencols - 1 )
						{
	                        E.coloff = E.cx - E.screencols + 1;
	                        E.cx = E.screencols - 1;
	                    }
	                }
	            }
				
	        } else {
	            
				E.cx -= 1;
	        }
	        break;
			
	    case ARROW_RIGHT:
	        if( row && filecol < row->size )
			{
	            if( E.cx == E.screencols - 1 )
				{
	                E.coloff++;
					
	            } else {
	                
					E.cx += 1;
	            }
				
	        } else if( row && filecol == row->size )
			{
	            E.cx = 0;
	            E.coloff = 0;
	            if( E.cy == E.screenrows - 1 )
				{
	                E.rowoff++;
					
	            } else {
	                
					E.cy += 1;
	            }
	        }
	        break;
			
	    case ARROW_UP:
	        if( E.cy == 0 )
			{
	            if( E.rowoff )
				{
					E.rowoff--;
				}
				
	        } else {
	            
				E.cy -= 1;
	        }
	        break;
			
	    case ARROW_DOWN:
	        if( filerow < E.numrows )
			{
	            if( E.cy == E.screenrows - 1 )
				{
	                E.rowoff++;
					
	            } else {
	                
					E.cy += 1;
	            }
	        }
	        break;
	    
		default:
			io_unknownkey_message( "editorMoveCursor", key );
			break;
    }
	
	
	/* TODO: THIS is where the cursor position gets modified to fit in the */
	/*  length of the current line. Add more logic (and members in E) to */
	/*  seperate the "rear position" from the position displayed on screen, */
	/*  so that the column gets maintained even if moving through a line */
	/*  that's too short, BUT still displays no further from the "origin */
	/*  column" than a character can next be added. */
	
    /* Fix cx if the current line has not enough chars. */
    filerow = E.rowoff + E.cy;
    filecol = E.coloff + E.cx;
    row =
		( filerow < E.numrows ) ?
			&E.row[ filerow ] :
			NULL;
    size_t rowlen = ( row ? row->size : 0 );
    if( filecol > rowlen )
	{
        E.cx -= ( filecol - rowlen );
        if( E.cx < 0 )
		{
            E.coloff += E.cx;
            E.cx = 0;
			
        }
		
    }
}

/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */
void editorProcessKeypress( int fd )
{
    /* When the file is modified, requires Ctrl-q to be pressed N times
     * before actually quitting. */
    static int quit_times = KILO_QUIT_TIMES;
	
    int c = editorReadKey( fd );
    switch( c )
	{
	    case ENTER:         /* Enter */
	        editorInsertNewline();
	        break;
	    case CTRL_C:        /* Ctrl-c */
	        /* We ignore ctrl-c, it can't be so simple to lose the changes
	         * to the edited file. */
			/* Instead of ignoring Ctrl-C, let's treat it ALMOST like */
			/*  Ctrl-Q. */
	        if( E.dirty && quit_times )
			{
#warning "Add a dedicated mode-message to msgs.c"
				msgs_build_alert
				(
					(msgs**)0,
						"WARNING!!! File has unsaved changes. "
						"To exit, press Ctrl-Q %d times to quit.",
						quit_times + 1
				);
	            return;
	        }
				/* Not a real error, but we DO want to prompt proper usage. */
			msgs_build_fatal
			(
				(msgs**)0,
					"\n\tKilo exited via Ctrl-C.\n"
					"\t!!! Warning !!!\n"
						"\t\tKilo is meant to exit via Ctrl-Q, not Ctrl-C!\n"
			);
			exit( 0 );
	        break;
	    case CTRL_Q:        /* Ctrl-q */
	        /* Quit if the file was already saved. */
	        if( E.dirty && quit_times )
			{
#warning "Add a dedicated mode-message to msgs.c"
				msgs_build_alert
				(
					(msgs**)0,
						"WARNING!!! File has unsaved changes. "
						"Press Ctrl-Q %d more times to quit.",
						quit_times
				);
	            quit_times--;
	            return;
	        }
	        exit( 0 );
	        break;
	    case CTRL_S:        /* Ctrl-s */
	        editorSave();
	        break;
	    case CTRL_F:
	        editorFind( fd );
	        break;
	    case BACKSPACE:     /* Backspace */
	    case CTRL_H:        /* Ctrl-h */
	    case DEL_KEY:
	        editorDelChar();
	        break;
	    case PAGE_UP:
	    case PAGE_DOWN:
	        if( c == PAGE_UP && E.cy != 0 )
			{
				E.cy = 0;
				
	        } else if( c == PAGE_DOWN && E.cy != E.screenrows-1 )
			{
				E.cy = E.screenrows - 1;
	        }
			{
		        size_t times = E.screenrows;
		        while( times-- )
				{
		            editorMoveCursor
						( c == PAGE_UP ? ARROW_UP: ARROW_DOWN );
		        }
			}
	        break;
	
	    case ARROW_UP:
	    case ARROW_DOWN:
	    case ARROW_LEFT:
	    case ARROW_RIGHT:
	        editorMoveCursor( c );
	        break;
	    case CTRL_L: /* ctrl+l, clear screen */
	        /* Just refresht the line as side effect. */
	        break;
	    case ESC:
	        /* Nothing to do for ESC in this mode. */
	        break;
	    default:
	        	/* This alerts for unfamiliar characters. */
			editorInsertChar( c );
	        break;
    }

    quit_times = KILO_QUIT_TIMES; /* Reset it to the original value. */
}

int editorFileWasModified( void )
{
    return E.dirty;
}

	/* Leandro Pereira */
void updateWindowSize( void )
{
    if
	(
		getWindowSize
		(
			STDIN_FILENO, STDOUT_FILENO,
			&E.screenrows, &E.screencols
		) == -1
	)
	{
		msgs_build_fatal( (msgs**)0,  "\tupdateWindowSize() was unable to query the screen for size (columns / rows)\n" );
        exit( 1 );
    }
    
	E.screenrows -= MILA_UTILITYLINES; /* Get room for utility area. */
}

	/* Leandro Pereira */
void handleSigWinCh2( int unused __attribute__((unused)) )
{
    updateWindowSize();
    if( E.cy > E.screenrows )
	{
		E.cy = E.screenrows - 1;
	}
    if( E.cx > E.screencols )
	{
		E.cx = E.screencols - 1;
	}
    editorRefreshScreen();
}

void initEditor( void )
{
#warning "initEditor() needs to split into terminal and editor -half sections, "
#warning "since the editor will be turned into just a mode."
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.screenrows = 0;
    E.screencols = 0;
    E.numrows = 0;
    E.rawmode = 0;
    E.altscr = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.syntax = NULL;
	/* Members of E below here aren't currently used. */
	/* E.orig_termios ; */
	E.utilrow = 0;
    /* The utility zone currently just holds the status lines. */
	/*  The "exten" section is for "auxiliary display" options, */
	/*  like a character LCD hanging off of a serial port. */
	E.extenrow = 0;
	/* These two describe e.g. the character dimensions of an */
	/*  auxiliary display, such as a character display hanging */
	/*  off of a serial port. Thus, they're differently sized */
	/*  than the ones above. */
	/* A decent minimum for conventional units is 8 wide by 1 */
	/*  tall, but 12*2, 16*2, and 20*4 are all semi-common, */
	/*  beware though, that custom LED-based displays can go */
	/*  MUCH lower in character counts, commercially available */
	/*  displays exist up to AT LEAST  40*4. */
	E.externx = 0;
	E.externy = 0;
	E.externrows = 0;
	E.externcols = 0;
	E.statusinterface = statview_build();
	E.modemsg = 0;
	E.deathrattle = 0;
	
    if( !E.altscr && !E.no_altscr )
    {
        if( !mila_initterm_xterm() )
		{
			msgs_build_fatal( (msgs**)0,  "\tXTerm initialization failed. If alt-screen in enabled, use ESC [?1049l.\n" );
			exit( 1 );
		}
    }
	if( !E.statusinterface )
	{
		msgs_build_fatal( (msgs**)0,  "\tstatview_build() failed in initEditor().\n" );
		exit( 1 );
	}
		/* Leandro Pereira */
    updateWindowSize();
}

/* Load the specified program in the editor memory and returns 0 on success
 * or 1 on error. */
int editorOpen( char *filename )
{
    FILE *fp;

    E.dirty = 0;
    free( E.filename );
    size_t fnlen = strlen( filename ) + 1;
    E.filename = malloc( fnlen );
    memcpy( E.filename, filename, fnlen );

    fp = fopen( filename, "r" );
    if( !fp )
	{
        if( errno != ENOENT )
		{
            perror( "Opening file" );
            exit( 1 );
        }
        return 1;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while( ( linelen = getline( &line, &linecap, fp ) ) > -1 )
	{
        if
		(
			line[ linelen - 1 ] == '\n' ||
			line[ linelen - 1 ] == '\r'
		)
		{
            line[ --linelen ] = '\0';
        }
			/* We've already verified the range of linelen, */
			/*  so we can safely cast. */
		editorInsertRow( E.numrows, line, (size_t)linelen );
    }
    free( line );
    fclose( fp );
    E.dirty = 0;
    return 0;
}

/* Save the current file on disk. Return 0 on success, 1 on error. */
int editorSave( void )
{
    size_t len;
    char *buf = editorRowsToString( &len );
    int fd = open( E.filename, O_RDWR | O_CREAT, 0644 );
    if( fd == -1 )
	{
		goto writeerr;
	}

    /* Use truncate + a single write(2) call in order to make saving
     * a bit safer, under the limits of what we can do in a small editor. */
	{
		off_t tmp = (off_t)len;
	/* Maybe https://stackoverflow.com/questions/4514572/c-question-off-t-and-other-signed-integer-types-minimum-and-maximum-values ? */
#warning "Find a better approach to limits than this."
		if( (size_t)tmp < len || ftruncate( fd, (off_t)len ) == -1 )
		{
			goto writeerr;
		}
	}
	{
		ssize_t res = write( fd, buf, len );
	    if( res < 0 || (size_t)res != len )
		{
			goto writeerr;
		}
	}

    close( fd );
    free( buf );
    E.dirty = 0;
	{
    	msgs *msg;
		if( msgs_build_note( &msg,  "%d bytes written on disk", len ) && msg )
		{
			int res = msgs_mark_timelife( msg, 30 /* Seconds? */ );
			(void)res;
			
		} else {
		}
	}
	return 0;

writeerr:
    free( buf );
    if( fd != -1 ) close( fd );
		/* Just in case strerror() does something janky. */
	int eerr = errno;
    /* editorSetStatusMessage( "Can't save! I/O error: %s", strerror( errno ) ); */
	msgs_build_error( (msgs**)0,  "Can't save! I/O error: %s", strerror( errno ) );
	msgs_build_fatal( (msgs**)0,  "\teditorSave() couldn't save! I/O error: %s\n", strerror( eerr ) );
    return 1;
}
