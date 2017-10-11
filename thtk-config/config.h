#ifndef THTK_CONFIG_H_
#define THTK_CONFIG_H_

#ifdef _MSC_VER
# define PRAGMA(x) __pragma(x)
#else
# define PRAGMA(x) _Pragma(#x)
#endif

#define PACK_PRAGMA
#ifdef PACK_PRAGMA
# define PACK_BEGIN PRAGMA(pack(push,1))
# define PACK_END PRAGMA(pack(pop))
#else
# define PACK_BEGIN
# define PACK_END
#endif

#define PACK_ATTRIBUTE __attribute__((__packed__))

#define HAVE_OFF_T
#define HAVE_SIZE_T
#define HAVE_SIZE_T_BASETSD
#if !defined(HAVE_SIZE_T) && defined(HAVE_SIZE_T_BASETSD)
# include <BaseTsd.h>
  typedef SIZE_T size_t;
#endif
#define HAVE_SSIZE_T
#define HAVE_SSIZE_T_BASETSD
#if !defined(HAVE_SSIZE_T) && defined(HAVE_SSIZE_T_BASETSD)
# include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif
#define HAVE_SYS_TYPES_H
#define HAVE_STDINT_H
#define HAVE_STDDEF_H
#define HAVE_LIBGEN_H
#define HAVE_SYS_STAT_H
#define HAVE_SYS_MMAN_H
#define HAVE_UNISTD_H
#ifndef HAVE_UNISTD_H
# define YY_NO_UNISTD_H
#endif
#define HAVE__SPLITPATH
#define HAVE_MEMPCPY
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

# define API_SYMBOL __declspec(dllexport)

# include <io.h>
# include <fcntl.h>
#endif
#include <string.h>
#endif
