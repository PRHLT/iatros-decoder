#ifndef ___FLEX_H_
#define ___FLEX_H_

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>
#include "pt-driver.h"
#include "pt-parser.h"
//#include <viterbi/pt.h>

#define true 1
#define false 0

#define YY_INPUT(buf,result,max_size)  {\
    result = pt_get_next_char(buf, max_size); \
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
extern int pt_lex(void);
extern int pt_lex_destroy(void);
extern int pt_parse(grammar_t *ng);
extern void pt_error(grammar_t *ng, char*);

/*
 * ccalc.c
 */

extern void pt_begin_token(char*);


extern void pt_dump_row(void);
extern int  pt_get_next_char(char *b, int max_buffer);
//extern void begin_token(char*);
extern void pt_print_error(char *s, ...);

/*
 * utilities
 */
char *pt_copy_string(const char *txt, int remove_quotes);

#endif /*___FLEX_H_*/
