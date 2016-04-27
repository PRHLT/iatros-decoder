/*
 * ng.c
 *
 *  Created on: 29-dic-2008
 *      Author: valabau
 */

#include "ng.h"
#include <prhlt/trace.h>
#include <prhlt/utils.h>
#include <prhlt/constants.h>

/// adds a initial state for the case when the grammar should start with the start word
/**
@param grammar Grammar
*/
state_grammar_t * ng_add_initial_state(grammar_t *grammar, table_grammar_t *table) {
  state_grammar_t *real_initial_state = STATE_NONE;
  if (grammar->start != VOCAB_NONE && table != NULL) {
    symbol_t s_initial[] = {grammar->start, VOCAB_NONE};
    node_grammar_t *a = table_grammar_find(table, s_initial);

    if (a != NULL) {
      real_initial_state = a->state;
    }
  }
  // second case: we do not have start words
  // so we add the first state of the grammar as initial state.
  // In case the grammar is an ngram the first state is the unigram state
  if (real_initial_state == STATE_NONE) {
    if (grammar->num_states > 0) {
      real_initial_state = grammar->vector[0];
      // this should give the same results than grammar->vector[0]
  //    symbol_t s_unigram[] = { VOCAB_NONE };
  //    node_grammar_t *a = table_grammar_find(table, s_unigram);
  //    REQUIRE(a != NULL, "ERROR: couldn't find unigram state in grammar\n");
  //    list_states_append(grammar->list_initial, a->state, 0.0);
    }
    else {
      REQUIRE(real_initial_state != STATE_NONE, "Couldn't find the unigram state in the ngram");
    }
  }

  if (grammar->silence != VOCAB_NONE && grammar->force_silence && table != NULL) {
    state_grammar_t *initial_state = state_grammar_create();
    symbol_t state_name[] = { VOCAB_NONE };
    state_grammar_set_name(initial_state, state_name);

    grammar_append(grammar, initial_state);
    state_grammar_append(initial_state, grammar->silence, .0, real_initial_state);

    return initial_state;
  }

  return real_initial_state;
}

///fills the initial and final state lists when they are empty (typically the case for ngrams)
/**
@param grammar Grammar
*/
void ng_set_initial_and_final_states(grammar_t *grammar, table_grammar_t *table) {

  // if we don't have initial and final states in the lists, we add them
  if (grammar->list_initial->num_elements == 0) {

    // find the initial state which can only be one in ngrams
    state_grammar_t * initial_state = ng_add_initial_state(grammar, table);
    list_states_append(grammar->list_initial, initial_state, 0.0);

    //XXX: This should'nt be necessary for ngrams
    if (grammar->end != VOCAB_NONE && grammar->list_end->num_elements == 0) {
      symbol_t s_end[] = {grammar->end, VOCAB_NONE};
      node_grammar_t *a = table_grammar_find(table, s_end);
      if (a == NULL) {
        state_grammar_t *state = state_grammar_create();
        grammar_append(grammar, state);
        state_grammar_set_name(state, s_end);
        a = table_grammar_insert(table, state);
      }
      REQUIRE(a != NULL, "ERROR: End symbol '%s' is not in grammar\n", vocab_get_string(grammar->vocab->in, grammar->end));

      list_states_append(grammar->list_end, a->state, 0.0);
    }
  }

}

/// Completes n-gram word arcs and backoff arcs
/**
 * @param grammar
 *
 * The n-gram parser encodes arcs and backoff arcs in a hash table. After the parser has
 * finished, it is necessary to complete the arcs with the information in the hash table.
 */
void ng_grammar_complete_arcs(grammar_t *grammar, table_grammar_t *table) {
  // this sould be only necessary for the following loop to find the end state
  // add end state
  if (grammar->end != VOCAB_NONE) {
    symbol_t name[] = {grammar->end, VOCAB_NONE};
    node_grammar_t *node = table_grammar_find(table, name);

    // we could not find the state. Create a new one
    if (node == NULL) {
      state_grammar_t *state = state_grammar_create();
      grammar_append(grammar, state);
      state_grammar_set_name(state, name);

      table_grammar_insert(table, state);
    }
  }

  for (int i = 0; i < grammar->num_states; i++) {
    symbol_t name[grammar->n + 1];
    symcpy(name, grammar->vector[i]->name);
    size_t len = symlen(name); ///< length of the new n-gram history

    // complete n-gram word arcs
    for (int j = 0; j < grammar->vector[i]->num_words; j++) {
      // append word
      symbol_t state_name[len + 2];
      int state_len = len;
      symcpy(state_name, name);
      state_name[state_len++] = grammar->vector[i]->words[j].word;
      state_name[state_len] = VOCAB_NONE;

      node_grammar_t *a = table_grammar_find(table, state_name);
      while (a == NULL && state_len > 0) {
        for (int l = 0; l < state_len; l++) state_name[l] = state_name[l + 1];
        state_len--;
        a = table_grammar_find(table, state_name);
      }
      REQUIRE(a != NULL, "ERROR: The target state does not exist. There must be a bug in the parser\n");
      grammar->vector[i]->words[j].state_next = a->state;
    }

    // complete backoff arcs if has backoff and not end symbol or "</s>"
    if (len > 0 && !is_logzero(grammar->vector[i]->bo)) {
      for (size_t l = 0; l < len; l++) name[l] = name[l + 1];
      len--;
      node_grammar_t *a = table_grammar_find(table, name);

      // the state does not exist, so create a new state
      // assuming that the backoff is zero
      if (a == NULL) {
        char *str = NULL;
        extended_vocab_symbols_to_string(name, grammar->vocab, &str);
        PRINT("Warning: Unseen n-gram history '%s'. Assuming a backoff weight of 0. "
            "Either there the n-gram format is incorrect or there is a bug in the parser\n", str);
        free(str);

        state_grammar_t *state = state_grammar_create();
        grammar_append(grammar, state);
        state_grammar_set_name(state, name);

        a = table_grammar_insert(table, state);
        a->state->bo = LOG_ONE;
      }


      grammar->vector[i]->state_bo = a->state;
    }
    else {
      grammar->vector[i]->bo = LOG_ZERO;
      grammar->vector[i]->state_bo = STATE_NONE;
    }
  }
}


///Delete hash table of grammar
/**
@param t Hash table
*/
void table_grammar_delete(table_grammar_t *t) {
  if (t == NULL) return;

  for (int i = 0; i < t->b; i++) {
    while (t->vector[i] != NULL) {
      node_grammar_t *a = t->vector[i];
      t->vector[i] = t->vector[i]->n;
      free(a);
      t->n--;
    }
  }
  free(t->vector);
  free(t);
}

///Find element in a hash table of grammar
/**
@param t Hash table
@param name Name to find in hash table
@param length Length of the name
@return the node if it has been found. Else returns NULL
*/
node_grammar_t *table_grammar_find(table_grammar_t *t, const symbol_t *state_name) {
  if (t->n == 0) return NULL;

  int f = (hash_generic(state_name, symlen(state_name) * sizeof(symbol_t)) % t->b);

  node_grammar_t *a = t->vector[f];

  while (a != NULL && symcmp(state_name, a->state->name) != 0) {
    a = a->n;
  }

  return a;
}

///Insert element in a hash table of grammar
/**
@param t the hash table
@param state a grammar state
@return the inserted grammar node holding the grammar state
*/
node_grammar_t *table_grammar_insert(table_grammar_t *t, state_grammar_t *state) {

  node_grammar_t *a = (node_grammar_t *) malloc(sizeof(node_grammar_t));
  MEMTEST(a);

  a->state = state;

  //Number of elements increased
  t->n++;

  int f = (hash_generic(state->name, symlen(state->name) * sizeof(symbol_t)) % t->b);

  REQUIRE(f<t->b, "Incorrect bucket\n");

  //Inserts into the list
  a->n = t->vector[f];
  t->vector[f] = a;

  return a;
}

///Function to create the hash table of grammar
/**
@param b Number of buckets of hash table
@return a new hash table for grammars
The actual number of buckets will be the closest prime number bigger then b
*/
table_grammar_t *table_grammar_create(int b) {
  //Create table
  table_grammar_t *t = (table_grammar_t *) malloc(sizeof(table_grammar_t));
  MEMTEST(t);

  //Number of buckets
  t->b = next_prime(b);

  //Create array
  t->vector = (node_grammar_t **) calloc(t->b, sizeof(node_grammar_t *));
  MEMTEST(t->vector);

  //Initial number of elements
  t->n = 0;

  return t;
}

///Performs the necessary operations to create a valid grammar from the n-gram parser output
/**
 * @param grammar
 * @param table
 */
void grammar_complete_ngram(grammar_t *grammar, table_grammar_t *table) {
  grammar->is_ngram = true;
  ng_grammar_complete_arcs(grammar, table);
  ng_set_initial_and_final_states(grammar, table);
}

