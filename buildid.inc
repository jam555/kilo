/* Thou -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *         a text editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * buildid.inc : A file to generate build info in conjunction with make
 *
 * Copyright (C) 2025 Jam555 <3349478+jam555@users.noreply.github.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  *  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/* !!! WARNING !!! DO NOT INCLUDE !!! */
/* This file is meant to be processed by the C preprocessor to generate a .c */
/*  file, which can then be built as any other source file. CHASH, CDATE, and */
/*  WDIRS are meant to be provided to the preprocessor on the command-line: */
/*  their backup values are informative to their individual purposes. */


#ifndef CHASH
# define CHASH "No commit hash proovided."
#endif

#ifndef CDATE
# define CDATE "No commit date proovided."
#endif

#ifndef WDIRS
# define WDIRS "No working-dir status information proovided."
#endif


#define INNER_STRINGIFY( val ) #val
#define STRINGIFY( val ) INNER_STRINGIFY( val )

#define CVERSTR( val ) "C standard version : " STRINGIFY( val )
#define GCCVERSTR( major, minor, patch ) \
	"GCC version # " \
		STRINGIFY( major ) " . " \
		STRINGIFY( minor ) " . " \
		STRINGIFY( patch ) " , release: " \
		__VERSION__
#define WIDTHSSTR( scwid, iwid, lwid, ipwid, imwid ) \
	"Signed Char width: " STRINGIFY( scwid ) \
	";  Integer width: " STRINGIFY( iwid ) \
	";  Long Integer width: " STRINGIFY( lwid ) \
	";  Integer Pointer width: " STRINGIFY( ipwid ) \
	";  Max Integer width: " STRINGIFY( imwid )



char
	*const thou_commithash = "Commit hash: " STRINGIFY( CHASH ),
	*const thou_commitdate = "Commit date: " STRINGIFY( CDATE ),
	*const thou_workdirstate = "Working directory state: " STRINGIFY( WDIRS ),
	
	*const thou_buildstamp = "CPP Date: " __DATE__ "; Time: " __TIME__,
	*const thou_stdcver = CVERSTR( __STDC_VERSION__ ),
	*const thou_gccver = GCCVERSTR( __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__ ),
	*const thou_typewidths = WIDTHSSTR( __SCHAR_WIDTH__, __INT_WIDTH__, __LONG_WIDTH__, __INTPTR_WIDTH__, __INTMAX_WIDTH__ ),
	*const thou_filestamp = __TIMESTAMP__;

#ifndef __STDC_VERSION__
	#ifdef __cplusplus
		#error "THis file is being treated as C++, but is C!"
	#endif
	
	#error "__STDC_VERSION__ undefined!"
#endif
