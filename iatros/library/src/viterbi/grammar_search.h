/*
 * grammar_search.h
 *
 *  Created on: 02-ene-2009
 *      Author: valabau
 */

#ifndef GRAMMAR_SEARCH_H_
#define GRAMMAR_SEARCH_H_

#include <prhlt/extended_vocab.h>
#include <prhlt/vector.h>

/// type of state search
typedef enum {  SS_NO_SEARCH,
                SS_START_PRIMARY,
                SS_LINEAR_SEARCH,
                SS_BINARY_SEARCH,
                SS_DIRECT_ACCESS,
                SS_END_PRIMARY,
                SS_START_SECONDARY,
                SS_LINEAR_SEARCH_SECONDARY,
                SS_BINARY_SEARCH_SECONDARY,
                SS_DIRECT_ACCESS_SECONDARY,
                SS_END_SECONDARY,
                MAX_SS
} search_type_t;

/// information necessary to find a word in a state_grammar
typedef struct state_search_t {
  void *data;
  search_type_t type;
} state_search_t;

//forward declaration
typedef struct state_grammar_t state_grammar_t;
typedef struct words_state_t words_state_t;

void state_grammar_build_word_search_secondary(state_grammar_t *state, const vocab_t *vocab);
words_state_t* state_grammar_find_secondary(state_grammar_t *state, const symbol_t symbol);
vector_t* state_grammar_find_primary(state_grammar_t *state, const symbol_t symbol);
void state_grammar_build_word_search_primary(state_grammar_t *state, const extended_vocab_t *vocab);
void state_grammar_delete_word_search(state_grammar_t *state);

bool state_grammar_fill_word_state(state_grammar_t *state, words_state_t* word_state);

#endif /* GRAMMAR_SEARCH_H_ */
