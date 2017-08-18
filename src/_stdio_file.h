/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#ifndef _STLP_STDIO_FILE_H
#define _STLP_STDIO_FILE_H

/* This file provides a low-level interface between the internal
 * representation of struct FILE, from the C stdio library, and
 * the C++ I/O library. */

#ifndef _STLP_CSTDIO
#  include <cstdio>
#endif
#ifndef _STLP_CSTDDEF
#  include <cstddef>
#endif

#if defined (__MSL__)
#  include <unix.h>  /* get the definition of fileno */
#endif

//SWISTART : Needed  definition of FILE here
__BEGIN_DECLS

/*
 * NB: to fit things in six character monocase externals, the stdio
 * code uses the prefix `__s' for stdio objects, typically followed
 * by a three-character attempt at a mnemonic.
 */

/* stdio buffers */
#if defined(__LP64__)
struct __sbuf {
  unsigned char* _base;
  size_t _size;
};
#else
struct __sbuf {
	unsigned char *_base;
	int	_size;
};
#endif

/*
 * stdio state variables.
 *
 * The following always hold:
 *
 *	if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *		_lbfsize is -_bf._size, else _lbfsize is 0
 *	if _flags&__SRD, _w is 0
 *	if _flags&__SWR, _r is 0
 *
 * This ensures that the getc and putc macros (or inline functions) never
 * try to write or read from a file that is in `read' or `write' mode.
 * (Moreover, they can, and do, automatically switch from read mode to
 * write mode, and back, on "r+" and "w+" files.)
 *
 * _lbfsize is used only to make the inline line-buffered output stream
 * code as compact as possible.
 *
 * _ub, _up, and _ur are used when ungetc() pushes back more characters
 * than fit in the current _bf, or when ungetc() pushes back a character
 * that does not match the previous one in _bf.  When this happens,
 * _ub._base becomes non-nil (i.e., a stream has ungetc() data iff
 * _ub._base!=NULL) and _up and _ur save the current values of _p and _r.
 *
 * NOTE: if you change this structure, you also need to update the
 * std() initializer in findfp.c.
 */
typedef	struct __sFILE {
	unsigned char *_p;	/* current position in (some) buffer */
	int	_r;		/* read space left for getc() */
	int	_w;		/* write space left for putc() */
#if defined(__LP64__)
	int	_flags;		/* flags, below; this FILE is free if 0 */
	int	_file;		/* fileno, if Unix descriptor, else -1 */
#else
	short	_flags;		/* flags, below; this FILE is free if 0 */
	short	_file;		/* fileno, if Unix descriptor, else -1 */
#endif
	struct	__sbuf _bf;	/* the buffer (at least 1 byte, if !NULL) */
	int	_lbfsize;	/* 0 or -_bf._size, for inline putc */

	/* operations */
	void	*_cookie;	/* cookie passed to io functions */
	int	(*_close)(void *);
	int	(*_read)(void *, char *, int);
	fpos_t	(*_seek)(void *, fpos_t, int);
	int	(*_write)(void *, const char *, int);

	/* extension data, to avoid further ABI breakage */
	struct	__sbuf _ext;
	/* data for long sequences of ungetc() */
	unsigned char *_up;	/* saved _p when _p is doing ungetc data */
	int	_ur;		/* saved _r when _r is counting ungetc data */

	/* tricks to meet minimum requirements even when malloc() fails */
	unsigned char _ubuf[3];	/* guarantee an ungetc() buffer */
	unsigned char _nbuf[1];	/* guarantee a getc() buffer */

	/* separate buffer for fgetln() when line crosses buffer boundary */
	struct	__sbuf _lb;	/* buffer for fgetln() */

	/* Unix stdio files get aligned to block boundaries on fseek() */
	int	_blksize;	/* stat.st_blksize (may be != _bf._size) */
	fpos_t	_offset;	/* current lseek offset */
} FILE;

__END_DECLS

//SWISTOP

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_WCE)

inline int _FILE_fd(const FILE *__f) {
  /* Check if FILE is one of the three standard streams
     We do this check first, because invoking _fileno() on one of them
     causes a terminal window to be created. This also happens if you do
     any IO on them, but merely retrieving the filedescriptor shouldn't
     already do that.

     Obviously this is pretty implementation-specific because it requires
     that indeed the first three FDs are always the same, but that is not
     only common but almost guaranteed. */
  for (int __fd = 0; __fd != 3; ++__fd) {
    if (__f == _getstdfilex(__fd))
      return __fd;
  }

  /* Normal files. */
  return (int)::_fileno((FILE*)__f); 
}
struct __sFILE;
# elif defined (_STLP_SCO_OPENSERVER) || defined (__NCR_SVR)

inline int _FILE_fd(const FILE *__f) { return __f->__file; }

# elif defined (__sun) && defined (_LP64)

inline int _FILE_fd(const FILE *__f) { return (int) __f->__pad[2]; }

#elif defined (__hpux) /* && defined(__hppa) && defined(__HP_aCC)) */ || \
      defined (__MVS__) || \
      defined (_STLP_USE_UCLIBC) /* should be before _STLP_USE_GLIBC */

inline int _FILE_fd(const FILE *__f) { return fileno(__CONST_CAST(FILE*, __f)); }

#elif defined (_STLP_USE_GLIBC)

inline int _FILE_fd(const FILE *__f) { return __f->_fileno; }

#elif defined (__BORLANDC__)

inline int _FILE_fd(const FILE *__f) { return __f->fd; }

#elif defined (__MWERKS__)

/* using MWERKS-specific defines here to detect other OS targets
 * dwa: I'm not sure they provide fileno for all OS's, but this should
 * work for Win32 and WinCE

 * Hmm, at least for Novell NetWare __dest_os == __mac_os true too..
 * May be both __dest_os and __mac_os defined and empty?   - ptr */
#  if __dest_os == __mac_os
inline int _FILE_fd(const FILE *__f) { return ::fileno(__CONST_CAST(FILE*, __f)); }
#  else
inline int _FILE_fd(const FILE *__f) { return ::_fileno(__CONST_CAST(FILE*, __f)); }
#  endif

#elif defined (__QNXNTO__) || defined (__WATCOMC__) || defined (__EMX__)

inline int _FILE_fd(const FILE *__f) { return __f->_handle; }

#elif defined (__Lynx__)

/* the prototypes are taken from LynxOS patch for STLport 4.0 */
inline int _FILE_fd(const FILE *__f) { return __f->_fd; }

#else  /* The most common access to file descriptor. */

struct __sFILE;
inline int _FILE_fd(const FILE *__f) { return __f->_file; }

#endif

_STLP_END_NAMESPACE

#endif /* _STLP_STDIO_FILE_H */

/* Local Variables:
 * mode:C++
 * End: */
