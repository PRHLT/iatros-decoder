#ifndef _LEX_DRIVER_H

#define _LEX_DRIVER_H

#include <viterbi/lex.h>
#include <stdio.h>

void lex_append_model(lex_t *lex, model_t *model);
void lex_append_state(lex_t *lex, model_t *model, int state_id);
void lex_append_edge (lex_t *lex, model_t *model, const char *label, int source, int destination, float probability);


#endif // _LEX_DRIVER_H
