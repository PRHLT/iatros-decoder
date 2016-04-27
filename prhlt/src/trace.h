/***************************************************************************
 *   Copyright (C) 2005 by Vicente Alabau Gonzalvo                         *
 *   vialgon@vialgonP4                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _TRACE_H
#define _TRACE_H

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#else
#include <stdio.h>
#include <stdlib.h>
#endif

extern int verbose;
extern void __attribute__((constructor)) setup_sigsegv();
extern void dump_backtrace();

#define _write(...) fprintf(stderr, ##__VA_ARGS__)

#define _write_msg(file, line, func_name, exp, ...) \
do { \
  _write("%s (%d): ",file, line);	      \
  _write(" in function '%s': ", func_name);   \
  if ( exp ) { \
    _write(" expression '%s': ", exp); \
  } \
  _write(""__VA_ARGS__); \
  fprintf(stderr, "\n"); \
} while(0)


#define _write_critical_msg(file, line, func_name, exp, ...) \
do { \
  _write_msg(file, line, func_name, exp, ##__VA_ARGS__); \
  if (errno != 0) { \
    perror(""); \
  } \
  dump_backtrace();\
  abort(); \
} while (0)

#define _WRITE(exp, expstr, ...) \
  do { if (exp) { \
         _write_msg(__FILE__ , __LINE__, __FUNCTION__, (expstr), ##__VA_ARGS__); \
       } \
  } while(0)

#define _WRITE_CRITICAL(exp, expstr, ...) \
  do { if (exp) { \
           _write_critical_msg(__FILE__ , __LINE__, __FUNCTION__, (expstr), ##__VA_ARGS__); \
       } \
  } while(0)

#define TRACE(level, ...) \
do { if (level <= verbose) { \
  _write(__VA_ARGS__); \
} } while(0)

#define PRINT(...) _write(__VA_ARGS__)

#define CHECK(exp, ...) _WRITE(!(exp), #exp, ##__VA_ARGS__)

#define CHECK_SYS_ERROR(exp, ...) _WRITE_CRITICAL(!(exp), #exp, ##__VA_ARGS__)

#define REQUIRE(exp, ...) _WRITE_CRITICAL(!(exp), #exp, ##__VA_ARGS__)

#define ERROR(...) _WRITE(1, "", ##__VA_ARGS__)

#define FAIL(...) _WRITE_CRITICAL(1, "", ##__VA_ARGS__)

#define MEMTEST(exp) CHECK_SYS_ERROR((exp) != NULL, "Memory test")

/* Init lib */
#define INIT_TRACE(level) \
  setup_sigsegv();\
  do { \
    verbose = level; \
  } while(0)

#ifdef __cplusplus
}
#endif

#endif // _TRACE_H
