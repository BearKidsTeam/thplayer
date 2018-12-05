#ifndef THTK_CONFIG_H_
#define THTK_CONFIG_H_

#ifdef _MSC_VER
# define PRAGMA(x) __pragma(x)
#else
# define PRAGMA(x) _Pragma(#x)
#endif

#define HAVE_OFF_T
#define HAVE_SIZE_T
#define HAVE_SSIZE_T
#define HAVE_SYS_TYPES_H
#define HAVE_STDINT_H
#define HAVE_STDDEF_H
#define HAVE_SYS_STAT_H
#define HAVE_SYS_MMAN_H
#define HAVE_UNISTD_H
#ifndef HAVE_UNISTD_H
# define YY_NO_UNISTD_H
#endif
#define HAVE__SPLITPATH
#define HAVE_MMAP
#define HAVE_MUNMAP
#define HAVE_FEOF
#define HAVE_FILENO
#define HAVE_FREAD
#define HAVE_FWRITE
#define HAVE_GETC
#define HAVE_PUTC

#ifdef _MSC_VER
# ifdef HAVE_FEOF
#  define feof_unlocked feof
# endif
# ifdef HAVE_FILENO
#  define fileno_unlocked fileno
# endif
# ifdef HAVE_FREAD
#  define fread_unlocked fread
# endif
# ifdef HAVE_FWRITE
#  define fwrite_unlocked fwrite
# endif
# ifdef HAVE_GETC
#  define getc_unlocked getc
# endif
# ifdef HAVE_PUTC
#  define putc_unlocked putc
# endif

# define API_SYMBOL
# define ssize_t int
# define PACK_BEGIN PRAGMA(pack(push,1))
# define PACK_END PRAGMA(pack(pop))
# define PACK_ATTRIBUTE

# include <io.h>
# include <fcntl.h>

#else

# define PACK_ATTRIBUTE __attribute__((__packed__))
# define PACK_BEGIN
# define PACK_END
#if ( __GLIBC__ >= 2 ) && ( __GLIBC_MINOR__ >= 1 )
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
# define HAVE_MEMPCPY
#endif

#endif //_MSC_VER

#include <string.h>
#endif
