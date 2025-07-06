/* Thou:Milli -- A very simple editor derived from Salvatore Sanfilippo's Kilo,
 *     a text editor in less than 1-kilo lines of code (as counted by "cloc").
 *     Does not depend on libcurses, directly emits VT100 escapes on the
 *     terminal.
 *
 * -----------------------------------------------------------------------
 *
 * io.h
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

#ifndef IO_IO_H
# define IO_IO_H
	
		/* This needs to become uint32_t at some point in the future. */
	typedef char io_chara;
	
	
	int io_unknownkey_message( const io_chara *func, int key );
	
	
	typedef struct io io;
	typedef struct io_closure io_closure;
	
	typedef enum
	{
		io_flag_invalid = -1,
		
		io_flag_null = 0,
		
		io_flag_onSuccess = 1,
		io_flag_wouldWait = 2,
		io_flag_onEOF = 4,
		io_flag_onFailure = 8,
		
		
		io_flag__PAST_END
		
	} io_flags;
	
	typedef
		int (*io_sendchar)
		(
			io *stream,
			io_chara val, io_flags flags,
			io_closure *on_err
		);
	typedef
		int (*io_fetchchar)
		(
			io *stream,
			io_chara *dest, io_flags flags,
			io_closure *on_err
		);
	typedef
		int (*io_genericfunc)
		(
			io *stream,
			io_flags flags,
			io_closure *on_err
		);
	
	
	
		/* This is a convenience type, to allow error values via callback. It */
		/*  MUST ONLY be called from DIRECTLY within Thou I/O code, not from */
		/*  within anything else that it calls, so that the behavior of */
		/*  coroutine usage is more predictable. */
	struct io_closure
	{
		void *data;
		void (*func)( io_closure*, io*,  intmax_t );
	};
	
	
	
		/* THe actual stream type... except it's really a mask, but close enough. */
	struct io
	{
		uintptr_t id;
		
		io_fetchchar getc;
		io_sendchar putc;
		
		io_genericfunc close;
	};
	
	inline int io_putc
	(
		io *stream,
		io_chara val, io_flags flags,
		io_closure *on_err
	)
	{
		if( stream )
		{
			if( !( stream->putc ) )
			{
				return( -2 );
			}
			
			return( stream->putc( stream,  val, flags,  on_err ) );
		}
		
		return( -1 );
	}
	inline int io_getc
	(
		io *stream,
		io_chara *dest, io_flags flags,
		io_closure *on_err
	)
	{
		if( stream && dest )
		{
			if( !( stream->getc ) )
			{
				return( -2 );
			}
			
			return( stream->getc( stream,  dest, flags,  on_err ) );
		}
		
		return( -1 );
	}
	
	inline int io_close
	(
		io *stream,
		io_flags flags,
		io_closure *on_err
	)
	{
		if( stream )
		{
			if( !( stream->close ) )
			{
				return( -2 );
			}
			
			return( stream->close( stream,  flags,  on_err ) );
		}
		
		return( -1 );
	}
	
	
	
	
	
	/**************************************************************************/
	/**************************************************************************/
	/** The following are predefined I/O types: *******************************/
	/*** Callisto: "local" common file (and similar) I/O. *********************/
	/*** Europa: "local" "smart" I/O, such as ANSI terminals. *****************/
	/*** Ganymede: remote "smart" I/O, such as accessing a remote system via **/
	/***  Unix "ed". **********************************************************/
	/**************************************************************************/
	/**************************************************************************/
	
	
		/* Wraps fopen(). */
	io* io_callisto1
	(
		const char *filename, const char *mode,
		io_closure *on_err
	);
	io* io_callisto2( FILE *src, FILE *dest,  io_closure *on_err );
	
	
	
	typedef enum
	{
		io_europa_invalid = -1,
		
		io_europa_null = 0,
		
		io_europa_noaltscr = 1,
		io_europa_isaltscr = 2,
		io_europa_israw = 4
		
	} io_europa_flags;
		/* Wraps stdin/stdout. */
	io* io_europa1( void );
	
	
	
	/*
	io* io_ganymede( ??? );
	*/
	
/*  5    0    5    0    5    0    5    0    5    0    5    0    5    0    5    0  */
	/*
	int coyield2
	(
		corohead *dest,
		corohead **volatile old,
		void *data, void (*func)( void* )
	);
	*/
	
	
	
	
	
	/**************************************************************************/
	/**************************************************************************/
	/** The following are predefined & generic implementation functions. ******/
	/*** io_genericnull() : Mean for io{} insdtances not meant to be closed. **/
	/**************************************************************************/
	/**************************************************************************/
	
	inline int io_genericnull
	(
		io *stream,
		io_flags flags,
		io_closure *on_err
	)
	{
		(void)flags;
		(void)on_err;
		
		if( stream )
		{
			return( 1 );
		}
		
		return( -1 );
	}
	
#endif
