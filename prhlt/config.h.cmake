/* config.h.  Generated from config.h.cmake by cmake.  */

#ifndef _${PROJECT_PREFIX}_CONFIG_H
#define _${PROJECT_PREFIX}_CONFIG_H

#include <prhlt/version.h>

#cmakedefine HAVE_POPEN

#cmakedefine HAVE_VALUES_H
#cmakedefine HAVE_ERRNO_H
#cmakedefine HAVE_ISATTY
#cmakedefine HAVE_CLOCK
#cmakedefine HAVE_STRDUP
#cmakedefine HAVE_STRTOK_R
#cmakedefine HAVE_STRCASECMP
#cmakedefine HAVE_FILENO
#cmakedefine HAVE_BACKTRACE
#cmakedefine HAVE_ADDR2LINE


#cmakedefine HAVE_CALLGRIND_H
#ifdef HAVE_CALLGRIND_H
  #include <valgrind/callgrind.h>
  #define CALLGRIND_INIT { CALLGRIND_STOP_INSTRUMENTATION;  CALLGRIND_ZERO_STATS; }
#else
  #define CALLGRIND_INIT
  #define CALLGRIND_STOP_INSTRUMENTATION
  #define CALLGRIND_START_INSTRUMENTATION
#endif




#endif //_${PROJECT_PREFIX}_CONFIG_H
