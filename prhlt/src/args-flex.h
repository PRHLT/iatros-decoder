#ifndef _ARGS_FLEX_H_
#define _ARGS_FLEX_H_

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>
#include "args-parser.h"
#include "args.h"


#define true 1
#define false 0

#define YY_INPUT(buf,result,max_size)  {\
    result = args_get_next_char(buf, max_size); \
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
extern int args_lex(void);
extern int args_parse(args_t *args);
extern int args_lex_destroy(void);
extern void args_error(args_t *args,char*);

/*
 * ccalc.c
 */
extern void args_begin_token(char*);

extern void args_dump_row(void);
extern int  args_get_next_char(char *b, int max_buffer);
// extern void begin_token(char*);
extern void args_print_error(char *s, ...);

/*
 * utilities
 */
char *args_copy_string(const char *txt, int remove_quotes);

#endif /*_ARGS_FLEX_H_*/
