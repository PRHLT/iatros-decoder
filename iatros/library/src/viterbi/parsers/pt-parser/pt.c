/*
 * pt.c
 *
 *  Created on: 25-ene-2009
 *      Author: valabau
 */

#include <viterbi/grammar.h>
#include <prhlt/constants.h>

///Complete the pt grammar with a initial and end state if necessary
/**
@param grammar Grammar
*/
void pt_grammar_add_initial_and_end_states(grammar_t *grammar) {
  //Create a initial state
  state_grammar_t *initial_state = grammar->vector[0];
  if (grammar->start != VOCAB_NONE && grammar->end != VOCAB_NONE) {
    initial_state = state_grammar_create();
    grammar_append(grammar, initial_state);
    state_grammar_append(initial_state, grammar->start, LOG_ZERO, 0);
  }

  //List of initial states
  list_states_clear(grammar->list_initial);
  list_states_append(grammar->list_initial, initial_state, 0.0);

  //Create an end state
  state_grammar_t *final_state = grammar->vector[0];
  if (grammar->start != VOCAB_NONE && grammar->end != VOCAB_NONE) {
    final_state = state_grammar_create();
    grammar_append(grammar, final_state);

    //Edges of old end states to new end states
    for (int i = 0; i < grammar->list_end->num_elements; i++) {
      state_grammar_append(grammar->list_end->vector[i].state, grammar->end, grammar->list_end->vector[i].prob, final_state);
    }
  }

  //List of end states
  list_states_clear(grammar->list_end);
  list_states_append(grammar->list_end, final_state, 0.0);
}

///Performs the necessary operations to create a valid grammar from the phrase table parser output
/**
 * @param grammar
 * @param table
 */
void grammar_complete_pt(grammar_t *grammar) {
  grammar->is_ngram = false;
  pt_grammar_add_initial_and_end_states(grammar);
}
