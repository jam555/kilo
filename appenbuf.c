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

/* TODO: Find all of the "warning" directives, and fix them. */

 /* TODO: Move these into a header and wrap in ifdef()s for */
 /*  override support. */
#define KILO_QUERY_LEN 256
#define KILO_QUIT_TIMES 3
#define MILA_TABSIZE 8
	/* This is the number of lines for the status lines. */
#define MILA_UTILITYLINES 2

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

#include "appenbuf.h"

#define MILA_TERMCODES_11 "\x1b[7m"



void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b,ab->len+len);

    if (new == NULL) return;
    memcpy(new+ab->len,s,len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

void mila_ab_curseek( struct abuf *ab, int argn,   int x, int y, char *tail )
{
	char buf[32];
	
#define MILA_TERMCODES_8 "\x1b[H%s" /* Go home. */
#define mila_ab_curseek_ONEARG "\x1b[%dH%s"
#define MILA_TERMCODES_21 "\x1b[%d;%dH%s"
	
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
		snprintf (buf, sizeof(buf), MILA_TERMCODES_21, x, y,  tail );
	}
	
	abAppend( ab,buf,strlen(buf));
}
void mila_ab_curvis_hide( struct abuf *ab )
{
#define MILA_TERMCODES_7 "\x1b[?25l"
    abAppend( ab,MILA_TERMCODES_7,6); /* Hide cursor. */
}
void mila_ab_curvis_show( struct abuf *ab )
{
#define MILA_TERMCODES_22 "\x1b[?25h"
    abAppend( ab,MILA_TERMCODES_22,6); /* Show cursor. */
}
void mila_ab_curseek_home( struct abuf *ab )
{
	mila_ab_curseek( ab, 0,   0, 0, "" );
}
void mila_ab_clearall( struct abuf *ab )
{
#define mila_ab_clearall_TERMCODE "\x1b[2K\r\n"
	abAppend( ab,mila_ab_clearall_TERMCODE,7);
}
void mila_ab_cleartostart( struct abuf *ab )
{
#define mila_ab_cleartostart_TERMCODE "\x1b[1K\r\n"
	abAppend( ab,mila_ab_cleartostart_TERMCODE,7);
}
void mila_ab_cleartoend( struct abuf *ab, char *tail )
{
	char buf[32];
	
	if( !tail )
	{
		tail = "";
	}
	
#define mila_ab_cleartoend_TERMCODE "\x1b[0K%s"
	snprintf( buf, sizeof(buf), mila_ab_cleartoend_TERMCODE,  tail );
	
	abAppend( ab,buf,strlen(buf));
}
void mila_ab_swapFgBg( struct abuf *ab )
{
	abAppend( ab,MILA_TERMCODES_11,4);
}
void mila_ab_resetAttribs( struct abuf *ab, char *tail )
{
	char buf[32];
	
	if( !tail )
	{
		tail = "";
	}
	
#define MILA_TERMCODES_12 "\x1b[0m%s"
	snprintf( buf, sizeof(buf), MILA_TERMCODES_12,  tail );
	
	abAppend( ab,buf,strlen(buf));
}
void mila_ab_defaultFg( struct abuf *ab )
{
#define MILA_TERMCODES_13 "\x1b[39m"
	abAppend( ab,MILA_TERMCODES_13,5);
}
