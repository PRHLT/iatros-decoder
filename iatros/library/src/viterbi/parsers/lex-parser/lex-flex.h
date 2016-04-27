#ifndef ___FLEX_H_
#define ___FLEX_H_

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>
#include "lex-driver.h"
#include "lex-parser.h"

#define true 1
#define false 0

#define YY_INPUT(buf,result,max_size)  {\
    result = get_next_char(buf, max_size); \
    if (  result <= 0  ) \
      result = YY_NULL; \
    }

/*
 * global variable
 */
extern int debug;

/*
 * lex & parse
 */
extern int lex_lex(void);
extern int lex_lex_destroy(void);
extern int lex_parse(lex_t * lex);
extern void lex_error(lex_t * lex,char*);

/*
 * ccalc.c
 */

extern void begin_token(char*);

extern void dump_row(void);
extern int  get_next_char(char *b, int max_buffer);
//extern void begin_token(char*);
extern void print_error(char *s, ...);

/*
 * utilities
 */
char *copy_string(const char *txt, int remove_quotes);

#endif /*___FLEX_H_*/
