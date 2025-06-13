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



/*
	Interesting key combo: ctrl-/ to turn the current line into a
	single-line comment.
*/
#define KILO_VERSION "0.0.1"

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>

#include "statview.h"
#include "appenbuf.h"
#include "term.h"
#include "msgs.h"


/* 6/June/2025: I've decided (several days ago) to rename the system in */
/*  general Thou (maybe it'll get memed on, but more importantly it */
/*  refereces Kilo via "Thousand"), and the text-editor sub-tool Milli (to */
/*  reference Kilo via SI prefixes, but to clearly be "smaller"). */
	/* TODO: Rename stuff appropriately. */

/* TODO: Find all of the "warning" directives, and fix them. */

/*
	
	!!!
	TODO: Move the "execute inside coroutine" stuff from statview to coro.
	!!!
		Do this one first!
	!!!
	
	TODO: Start using the flags returned from the statview stuff to draw "markup text".
		Note: The markup text is used to mark status line sub-fields, and is mostly important for adaptive layout
		stuff. There should PROBABLY be a bg/fg swap to distinguish them, AND they should exist at both the start
		AND end of a sub-field, not just one or the other. They should also be distinctive from each other, as
		their purpose is mostly to distinguish sub-fields when only a subset can be displayed at a time.
	TODO: Start using the assert stuff below.
	TODO: Coroutine-based non-blocking I/O routines.
	TODO: Seperate code to do updates from code to do renders, for e.g. better marques.
	TODO: Improve the Responsive/Adaptive handling of the status line.
		Note: This includes cycling between fields when compact- try to use the msgs_queue stuff.
	TODO: Build a "metaterm" to represent concepts (e.g. marques, and markup-text surrounding sub-fields).
	TODO: While testing the status-message stuff, the cursor somehow got stuck inside the status message area:
		hunt this down asnd fix (entered C-f, then exited, was doing lots of scrolling the whole time).
*/
/*
	Wishlist:
		Multi-document Milli
		Hex Editor (call it Runes, include multi-doc)
		CLI (not a mode, always at the bottom, controls the program rather than OS, maybe scriptable)
			Hooks for TinyC, Lua, TCL, maybe more? Forth? There's a mini-Javascript that might be useful. Is
			there a BASIC that can be used? Anything else?
			Select/Insert/Copy/Cut/Delete for Milli & Runes, and CLI.
			Better/heavier-duty text-file edit scheme (we aren't using ropes yet, are we?) for Milli & Runes
		
		Minesweeper
		
		"Menu/button mode"; configurable, to allow TUI execution of e.g. arbitrary Makefile targets, meant
			to improve usefullness as IDE/project-manager
		Calculator (scriptable? graphing? RPN & Infix switchable? arbitrary-length numbers? !!!Base
			Conversion & Programmer's commands!!! Sign-bit/1s-complement/2s-complement conversion!
			If ALN, then BCD/Packed-BCD/ASCII/EBCDIC/Binary conversions.
			If scriptable, then SEPERATE EXECUTABLE for crash-resistance, and struct{} + C-like operators +
			Meta-C -like operator-overloading + gc + coroutines; DO NOT attempt to make a "solver")
		Contacts management
			Use a common format, use same format for e.g. CONTRIBUTORS
		
		Extended keyboard support (XTerm, Kitty, that thing Kitty is based on)
		Unix-Ed over remote connection as I/O option (to deal with remote servers)
			Vi-mode for the same?
		Multi-terminal mode (use a sub-program to simplify I/O, 1 instance per terminal: should be fine to
			have a variant that uses NCurses, another for PDCurses, maybe another for SDL, maybe another for
			bare-metal MDA/CGA/EGA/VGA, preferably another for RS-232 character-LCD)
		
		Hierarchal config files (go looking all the way to root, include a "additional configs" option for e.g.
			/etc configs); "leaf-most" (as opposed to root-most) files are dominant
		Extend config file syntax all the way to "project management" & "IDE" level (even if just slightly)
*/


#define THOU_SEV_SEVERITY_0  ( 0x200 )  /* 512 */
#define THOU_SEV_SEVERITY_1  ( 0x400 ) /* 1024 */
#define THOU_SEV_SEVERITY_2  ( 0x600 ) /* 1536 */
#define THOU_SEV_SEVERITY_3  ( 0x800 ) /* 2048 */

#define THOU_SEV_SEVERITY_4  ( 0xA00 ) /* 2560 */
#define THOU_SEV_SEVERITY_5  ( 0xC00 ) /* 3072 */
#define THOU_SEV_SEVERITY_6  ( 0xE00 ) /* 3584 */
#define THOU_SEV_SEVERITY_7 ( 0x1000 ) /* 4096 */

#define THOU_SEV_SEVERITY_8 ( 0x1200 ) /* 4608 */
#define THOU_SEV_SEVERITY_9 ( 0x1400 ) /* 5120 */
#define THOU_SEV_SEVERITY_A ( 0x1600 ) /* 5632 */
#define THOU_SEV_SEVERITY_B ( 0x1800 ) /* 6144 */

#define THOU_SEV_SEVERITY_C ( 0x1A00 ) /* 6656 */
#define THOU_SEV_SEVERITY_D ( 0x1C00 ) /* 7168 */
#define THOU_SEV_SEVERITY_E ( 0x1E00 ) /* 7680 */
#define THOU_SEV_SEVERITY_F ( 0x2000 ) /* 8192 */


#ifndef THOU_SEV_STEP
	#define THOU_SEV_STEP ( 1024 )
#endif


#ifndef THOU_SEV_NOTE
	#define THOU_SEV_NOTE ( 1024 )
#endif
#ifndef THOU_SEV_ALERT
	#define THOU_SEV_ALERT ( THOU_SEV_NOTE + THOU_SEV_STEP /* commonly 2048 */ )
#endif
#ifndef THOU_SEV_ERROR
	#define THOU_SEV_ERROR ( THOU_SEV_ALERT + THOU_SEV_STEP /* commonly 3072 */ )
#endif
#ifndef THOU_SEV_FATAL
	#define THOU_SEV_FATAL ( THOU_SEV_ERROR + THOU_SEV_STEP /* commonly 4096 */ )
#endif


#ifndef NDEBUG
	
	#define THOU_ASSERT( severity, comment, allowexit, exitval, condition ) \
			( ( ( (severity) >= THOU_SEV_NOTE ) ? \
				( \
					( !( (int)( condition ) ) ) ? \
						( \
							( ( (severity) >= THOU_SEV_FATAL ) ? \
								( msgs_build_fatal( (msgs**)0,  (comment) ) ) : \
								( \
									( (severity) >= THOU_SEV_ERROR ) ? \
										( msgs_build_error( (msgs**)0,  (comment) ) ) : \
										( \
											( (severity) >= THOU_SEV_ALERT ) ? \
												( msgs_build_alert( (msgs**)0,  (comment) ) ) : \
												( msgs_build_note( (msgs**)0,  (comment) ) ) \
										) \
								) ), \
							( (allowexit) && ( (severity) >= THOU_SEV_FATAL ) ) ? \
								( exit( exitval ), 0 ) \
						) \
				) ), \
			(void) )
	
#else
	
	#define THOU_ASSERT( severity, comment, allowexit, exitval, condition ) \
		/* Discarded assert. */
	
#endif


 /* TODO: Move these into a header and wrap in ifdef()s for */
 /*  override support. */
#define KILO_QUERY_LEN 256
#define KILO_QUIT_TIMES 3

#define MILA_TABSIZE 8
	/* This is the number of lines for the status lines. */
#define MILA_UTILITYLINES 2
	/* Whether the utility-area messages should timeout or not. */
#define MILA_MESSAGETIMEOUTS ( 0 )
	/* In deci-seconds. The time to scroll by one character. */
#define MILA_MESSAGESLOTH ( 3 )

/* Syntax highlight types */
#define HL_NORMAL 0
#define HL_NONPRINT 1
#define HL_COMMENT 2   /* Single line comment. */
#define HL_MLCOMMENT 3 /* Multi-line comment. */
#define HL_KEYWORD1 4
#define HL_KEYWORD2 5
#define HL_STRING 6
#define HL_NUMBER 7
#define HL_MATCH 8      /* Search match. */

#define HL_HIGHLIGHT_STRINGS (1<<0)
#define HL_HIGHLIGHT_NUMBERS (1<<1)

struct editorSyntax {
    char **filematch;
    char **keywords;
    char singleline_comment_start[2];
    char multiline_comment_start[3];
    char multiline_comment_end[3];
    int flags;
};

/* This structure represents a single line of the file we are editing. */
typedef struct erow {
    size_t idx;            /* Row index in the file, zero-based. */
    size_t size;           /* Size of the row, excluding the null term. */
    size_t rsize;          /* Size of the rendered row. */
    char *chars;        /* Row content. */
    char *render;       /* Row content "rendered" for screen (for TABs). */
    unsigned char *hl;  /* Syntax highlight type for each character in render.*/
    int hl_oc;          /* Row had open comment at end in last syntax highlight
                           check. */
} erow;

typedef struct hlcolor {
    int r,g,b;
} hlcolor;

	/* TODO: Break this into separate window & pane (frame & glass?) */
	/*  sections, to support multi-document capability. */
	/* This gets initialized in initEditor() in edevents.c */
struct editorConfig {
    int no_altscr;  /* Forbid usage of the alternate-screen. */

    size_t cx,cy;  /* Cursor x and y position in characters */
    size_t rowoff;     /* Offset of row displayed. */
    size_t coloff;     /* Offset of column displayed. */
    size_t screenrows; /* Number of rows that we can show */
    size_t screencols; /* Number of cols that we can show */
    size_t numrows;    /* Number of rows */
    int rawmode;    /* Is terminal raw mode enabled? */
    int altscr;     /* Is terminal alternate-screen selected? */
    erow *row;      /* Rows */
    int dirty;      /* File modified but not saved. */
    char *filename; /* Currently open filename */
    char statusmsg[80];
    time_t statusmsg_time;
	
    struct editorSyntax *syntax;    /* Current syntax highlight, or NULL. */
	
	
	/* The things below have been added to convert this into a window-tracker. They are currently unused. */
	
	struct termios orig_termios;
	
    /* The utility zone currently just holds the status lines. */
	/*  The "exten" section is for "auxiliary display" options, */
	/*  like a character LCD hanging off of a serial port. */
	erow *utilrow;      /* Utility zone rows */
	char *extenrow;	/* Storage space for the "extern text". Probably still contains escapes. */
	/* Extern x & y specifically is to allow for screen scrolling: */
	/*  there's no telling if it'll match the terminal size, so */
	/*  it's better to prepare for a mismatch. */
    size_t externx, externy;
	size_t externrows; /* Number of rows that we can show */
    size_t externcols; /* Number of cols that we can show */
	
		/* This is where our progress-target currently lies. We need to move the appenbuf.c */
		/*  stuff to using this instead of accessing the status directly. In particular, it */
		/*  should implement ticker-tape behavior, when the status-message is larger than the */
		/*  allocated space! */
	statstate *statusinterface;
		/* Replaces statusmsg. */
	msgs *modemsg;
	char *deathrattle;
};

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
        PAGE_DOWN
};



/* MILA_TERMCODES_11 was defined here.*/





extern struct editorConfig E;

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
extern char *C_HL_extensions[];
extern char *C_HL_keywords[];

/* Here we define an array of syntax highlights by extensions, keywords,
 * comments delimiters and flags. */
extern struct editorSyntax HLDB[];
extern size_t HLBD_entrycount;

#define HLDB_ENTRIES ( HLBD_entrycount )































/* ======================= Low level terminal handling ====================== */

extern struct termios orig_termios; /* In order to restore at exit.*/

void mila_term_cursseek_setpos( int alter, int ofile, size_t row, size_t col );
int mila_term_cursseek_finalchar( int alter );

void disableRawMode( int fd );
void mila_term_altscreen_disable( void );

/* Called at exit to avoid remaining in raw mode. */
void editorAtExit( void );

/* Raw mode: 1960 magic shit. */
int enableRawMode( int fd );

void mila_term_printWelcomeMessage
(
	struct abuf *ab,
	char *buf, size_t buflen
);
void mila_term_setcolor
(
	struct abuf *ab,
	char *buf, size_t buflen,
	
	int color, int *curcolor
);

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
int editorReadKey( int fd );

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned. */
int getCursorPosition( int ifd, int ofd, size_t *rows, size_t *cols );

/* Try to get the number of columns in the current terminal. If the ioctl()
 * call fails the function will try to query the terminal itself.
 * Returns 0 on success, -1 on error. */
int getWindowSize( int ifd, int ofd, size_t *rows, size_t *cols );

/* ====================== Syntax highlight color scheme  ==================== */

int is_separator(int c);

/* Return true if the specified row last char is part of a multi line comment
 * that starts at this row or at one before, and does not end at the end
 * of the row but spawns to the next row. */
int editorRowHasOpenComment(erow *row);

/* Set every byte of row->hl (that corresponds to every character in the line)
 * to the right syntax highlight type (HL_* defines). */
void editorUpdateSyntax(erow *row);

/* Maps syntax highlight token types to terminal colors. */
int editorSyntaxToColor(int hl);

/* Select the syntax highlight scheme depending on the filename,
 * setting it in the global state E.syntax. */
void editorSelectSyntaxHighlight(char *filename);

/* ======================= Editor rows implementation ======================= */

/* Update the rendered version and the syntax highlight of a row. */
void editorUpdateRow(erow *row);

/* Insert a row at the specified position, shifting the other rows on the bottom
 * if required. */
void editorInsertRow( size_t at, char *s, size_t len );

/* Free row's heap allocated stuff. */
void editorFreeRow( erow *row );

/* Remove the row at the specified position, shifting the remainign on the
 * top. */
void editorDelRow( size_t at );

/* Turn the editor rows into a single heap-allocated string.
 * Returns the pointer to the heap-allocated string and populate the
 * integer pointed by 'buflen' with the size of the string, escluding
 * the final nulterm. */
char *editorRowsToString( size_t *buflen );

/* Insert a character at the specified position in a row, moving the remaining
 * chars on the right if needed. */
void editorRowInsertChar(erow *row, size_t at, int c);

/* Append the string 's' at the end of a row */
void editorRowAppendString(erow *row, char *s, size_t len);

/* Delete the character at offset 'at' from the specified row. */
void editorRowDelChar(erow *row, size_t at);

/* Insert the specified char at the current prompt position. */
void editorInsertChar(int c);

/* Inserting a newline is slightly complex as we have to handle inserting a
 * newline in the middle of a line, splitting the line as needed. */
void editorInsertNewline(void);

/* Delete the char at the current prompt position. */
void editorDelChar(void);

/* Load the specified program in the editor memory and returns 0 on success
 * or 1 on error. */
int editorOpen(char *filename);

/* Save the current file on disk. Return 0 on success, 1 on error. */
int editorSave(void);

/* ============================= Terminal update ============================ */

#include "appenbuf.h"

/* Set an editor status message for the second line of the status, at the
 * end of the screen. Note that this will build the "full" message INTO the
 * E.statusmsg[] member: it SHOULD be properly length-restricted. */
void editorSetStatusMessage(const char *fmt, ...);

/* =============================== Find mode ================================ */

void editorFind(int fd);

/* ========================= Editor events handling  ======================== */

/* Handle cursor position change because arrow keys were pressed. */
void editorMoveCursor(int key);

/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */
void editorProcessKeypress(int fd);

int editorFileWasModified(void);

void updateWindowSize(void);

void handleSigWinCh(int unused __attribute__((unused)));

void initEditor(void);





void editorCalc_CurScreenPos( size_t *x, size_t *y );
void editorUpdateCurPos( struct abuf *ab );





/* Draws the status line. Pulled out of editorRefreshScreen() for */
/*  modularity. */
	/* ab: the primary buffer, will get drawn to the conventional terminal. */
	/* util: the utility-zone buffer, will ONLY get drawn to an auxiliary */
	/*  display (such as a character LCD), which might not even exist. */
	/* fstatus & fstat_len: storage for info about the file. */
	/* rstatus & rstat_len: storage for info about... the display, what does */
	/*  'r' stand for? Row? */
void editorStatusLine
(
	struct abuf *ab, struct abuf *util,
	
	char *fstatus, size_t fstat_len,
	char *rstatus, size_t rstat_len
);
	/* Renders the message line. The message will eventually move to the */
	/*  status line, and be replaced with a CLI area. */
		/* ab: the primary buffer, will get drawn to the conventional */
		/*  terminal. */
		/* util: the utility-zone buffer, will ONLY get drawn to an auxiliary */
		/*  display (such as a character LCD), which might not even exist. */
void editorMessageLine( struct abuf *ab, struct abuf *util );

    /* The following code draws the utility area. At the current time it only */
    /*  handles status lines, but I intend to throw other stuff in too. */
		/* ab: the primary buffer, will get drawn to the conventional */
		/*  terminal. */
		/* util: the utility-zone buffer, will ONLY get drawn to an auxiliary */
		/*  display (such as a character LCD), which might not even exist. */
void editorUtilityArea( struct abuf *ab, struct abuf *util );
