#ifndef _HMM_FLEX_H_
#define _HMM_FLEX_H_

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>
#include "hmm-driver.h"
#include "hmm-parser.h"

#define true 1
#define false 0

#define YY_INPUT(buf,result,max_size)  {\
    result = hmm_get_next_char(buf, max_size); \
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
extern int hmm_lex(void);
extern int hmm_lex_destroy(void);
extern int hmm_parse(hmm_t * hmm);
extern void hmm_error(hmm_t * hmm, char*);

/*
 * ccalc.c
 */
extern void hmm_begin_token(char*);









extern void hmm_dump_row(void);
extern int  hmm_get_next_char(char *b, int max_buffer);
//extern void begin_token(char*);
extern void hmm_print_error(char *s, ...);

/*
 * utilities
 */
char *hmm_copy_string(const char *txt, int remove_quotes);









#endif /*_HMM_FLEX_H_*/
