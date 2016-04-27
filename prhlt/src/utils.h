/*
 * utils.h
 *
 *  Created on: 25-sep-2008
 *      Author: valabau
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <prhlt/config.h>

#ifdef HAVE_VALUES_H
  #include <values.h>
#endif

#ifndef INFINITY
  #ifdef __GNUC__
    #define INFINITY (__builtin_inff())
  #else
    #define INFINITY FLT_MAX
  #endif
#endif


#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

// Portable inline functions. See http://www.greenend.org.uk/rjk/2003/03/inline.html
#ifndef INLINE
# ifdef __GNUC__
#  if defined __GNUC_STDC_INLINE__
#   define INLINE inline
#  elif __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 2)
#   define INLINE inline __attribute__ ((__gnu_inline__))
#  else
#   define INLINE inline
#  endif
# else
   /* With other compilers, assume the ISO C99 meaning of 'inline', if
      the compiler supports 'inline' at all.  */
#  define INLINE inline
# endif
#endif
#undef INLINE
#define INLINE
#define SWAP(a, b, T) do { T temporary = a; a = b; b = temporary; } while(0)

#ifdef __cplusplus
extern "C" {
#endif



INLINE int next_prime(int n);
INLINE float add_log(float a, float b);

#ifndef __cplusplus
// C99 bool type
#include <stdbool.h>
// for non-C99 compilant use this:
// define a new bool type that looks like a c++ bool
//typedef enum {false = 0, true = 1} bool;
#endif

typedef int (*cmp_fn)(const void *, const void *);
char *tokenize(char *str, const char *delimiter, char **saveptr);
char *strip(char *str);
int split(char *sentence,char **result, char *character);

#ifndef HAVE_ERRNO_H
#define errno __prhlt_errno
#define perror __prhlt_perror
#endif

#ifndef HAVE_ISATTY
#define isatty __prhlt_isatty
#endif

#include <time.h>
#if !defined(HAVE_CLOCK)
extern clock_t clock(void);
#define clock __prhlt_clock
#endif

#if !defined(HAVE_STRDUP) && !defined(__cplusplus)
#define strdup __prhlt_strdup
extern char *strdup(const char *str);
#endif

#if !defined(HAVE_STRTOK_R)
#define strtok_r(_s, _sep, _lasts) (*(_lasts) = strtok((_s), (_sep)))
#endif

#ifdef __cplusplus
}
#endif


#endif /* UTILS_H_ */
