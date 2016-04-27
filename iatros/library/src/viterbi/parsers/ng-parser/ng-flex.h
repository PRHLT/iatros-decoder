#ifndef ___FLEX_H_
#define ___FLEX_H_

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>
#include "ng-driver.h"
#include "ng-parser.h"
#include "ng.h"

#define true 1
#define false 0

#define YY_INPUT(buf,result,max_size)  {\
    result = ng_get_next_char(buf, max_size); \
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
extern int ng_lex(void);
extern int ng_lex_destroy(void);
extern int ng_parse(grammar_t *ng);
extern void ng_error(grammar_t *ng, char*);

/*
 * ccalc.c
 */

extern void ng_begin_token(char*);


extern void ng_dump_row(void);
extern int  ng_get_next_char(char *b, int max_buffer);
//extern void begin_token(char*);
extern void ng_print_error(char *s, ...);

/*
 * utilities
 */
char *ng_copy_string(const char *txt, int remove_quotes);

#endif /*___FLEX_H_*/
