#ifndef _FSM_FLEX_H_
#define _FSM_FLEX_H_

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>
#include "fsm-driver.h"
#include "fsm-parser.h"
#include "fsm.h"


#define true 1
#define false 0

#define YY_INPUT(buf,result,max_size)  {\
    result = fsm_get_next_char(buf, max_size); \
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
extern int fsm_lex(void);
extern int fsm_parse(grammar_t *fsm);
extern int fsm_lex_destroy(void);
extern void fsm_error(grammar_t *fsm,char*);

/*
 * ccalc.c
 */
extern void fsm_begin_token(char*);

extern void fsm_dump_row(void);
extern int  fsm_get_next_char(char *b, int max_buffer);
// extern void begin_token(char*);
extern void fsm_print_error(char *s, ...);

/*
 * utilities
 */
char *fsm_copy_string(const char *txt, int remove_quotes);

#endif /*_FSM_FLEX_H_*/
