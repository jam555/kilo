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

#include <stddef.h>


/* All functions including "mila" in their name were added for Mila. */

/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf {
    char *b;
    size_t len;
};

#define ABUF_INIT {NULL,0}

void abAppend( struct abuf *ab, const char *s, int len );

void abFree( struct abuf *ab );


/* These "mila" functions are actually found in term.c */

void mila_ab_curseek( struct abuf *ab, int argn,   int x, int y, char *tail );
void mila_ab_curseek_home( struct abuf *ab );

void mila_ab_curvis_hide( struct abuf *ab );
void mila_ab_curvis_show( struct abuf *ab );

void mila_ab_clearall( struct abuf *ab );
void mila_ab_cleartostart( struct abuf *ab );
void mila_ab_cleartoend( struct abuf *ab, char *tail );

void mila_ab_defaultFg( struct abuf *ab );
void mila_ab_swapFgBg( struct abuf *ab );
void mila_ab_resetAttribs( struct abuf *ab, char *tail );



void abStatusLine
(
	struct abuf *ab, struct abuf *util,
	
	char *fstatus, int fstat_len,
	char *rstatus, int rstat_len
);
void abMessageLine( struct abuf *ab, struct abuf *util );
void abUtilityArea( struct abuf *ab, struct abuf *util );


/* This function writes the whole screen using VT100 escape characters
 * starting from the logical state of the editor in the global state 'E'. */
void editorRefreshScreen(void);
