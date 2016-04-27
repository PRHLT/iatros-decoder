/*
 * ng.h
 *
 *  Created on: 29-dic-2008
 *      Author: valabau
 */

#ifndef NG_H_
#define NG_H_

#include <viterbi/grammar.h>

/// Node of hash table of grammar.
typedef struct node_grammar {
  state_grammar_t *state; ///< State
  struct node_grammar *n; ///< Next node
} node_grammar_t;

/// Hash table of grammar.
typedef struct table_grammar {
  node_grammar_t **vector; ///< Vector of hash table
  int b; ///< Number of buckets
  int n; ///< Number of elements
} table_grammar_t;

state_grammar_t * ng_add_initial_state(grammar_t *grammar, table_grammar_t *table);
void ng_set_initial_and_final_states(grammar_t *grammar, table_grammar_t *table);
void grammar_complete_ngram(grammar_t *grammar, table_grammar_t *table);
void table_grammar_delete(table_grammar_t *t);
node_grammar_t *table_grammar_find(table_grammar_t *t, const symbol_t *state_name);
node_grammar_t *table_grammar_insert(table_grammar_t *t, state_grammar_t *state);
table_grammar_t *table_grammar_create(int b);

#endif /* NG_H_ */
