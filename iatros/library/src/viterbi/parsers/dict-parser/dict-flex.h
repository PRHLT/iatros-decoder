#ifndef _DICT_FLEX_H_
#define _DICT_FLEX_H_

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>
#include "dict-driver.h"
#include "dict-parser.h"

#define true 1
#define false 0

#define YY_INPUT(buf,result,max_size)  {\
    result = dict_get_next_char(buf, max_size); \
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
extern int dict_lex(void);
extern int dict_lex_destroy(void);
extern int dict_parse(lex_t * dict);
extern void dict_error(lex_t * dict, char*);

/*
 * ccalc.c
 */
extern void dict_begin_token(char*);









extern void dict_dump_row(void);
extern int  dict_get_next_char(char *b, int max_buffer);
//extern void begin_token(char*);
extern void dict_print_error(char *s, ...);

/*
 * utilities
 */
char *dict_copy_string(const char *txt, int remove_quotes);









#endif /*_DICT_FLEX_H_*/
