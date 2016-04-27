#ifndef _DICT_DRIVER_H

#define _DICT_DRIVER_H

#include <viterbi/lex.h>
#include <stdio.h>

void dict_append_model(lex_t *lex, model_t *model, symbol_t word);
int dict_append_state(lex_t *lex, model_t *model, int state_id);
void dict_append_edge (lex_t *lex, model_t *model, const char *label, int source, int destination, float probability);
void dict_create_edge(state_lex_t *state, lex_t *lex, const char *label, int source, int destination, float probability);
void dict_postprocess(lex_t *lex);
int dict_edge_prob_cmp(const void *va, const void *vb);
#endif // _DICT_DRIVER_H
