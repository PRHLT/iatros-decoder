/*
 * fsm.c
 *
 *  Created on: 29-dic-2008
 *      Author: valabau
 */

#include "fsm.h"
#include <prhlt/trace.h>

///Complete the fsm grammar with a initial and end state if necessary
/**
@param grammar Grammar
*/
void fsm_grammar_add_initial_and_end_states(grammar_t *grammar) {
  //Create a initial state
  state_grammar_t *initial_state = state_grammar_create();
  grammar_append(grammar, initial_state);

  //Edges of new initial states to a old initial states
  for (int i = 0; i < grammar->list_initial->num_elements; i++) {
    state_grammar_append(initial_state, grammar->start, grammar->list_initial->vector[i].prob, grammar->list_initial->vector[i].state);
  }

  //List of initial states
  list_states_clear(grammar->list_initial);
  list_states_append(grammar->list_initial, initial_state, 0.0);

  //Create an end state
  state_grammar_t *final_state = state_grammar_create();
  grammar_append(grammar, final_state);

  //Edges of old end states to new end states
  for (int i = 0; i < grammar->list_end->num_elements; i++) {
    state_grammar_append(grammar->list_end->vector[i].state, grammar->end, grammar->list_end->vector[i].prob, final_state);
  }

  //List of end states
  list_states_clear(grammar->list_end);
  list_states_append(grammar->list_end, final_state, 0.0);
}


///Performs the necessary operations to create a valid grammar from the n-gram parser output
/**
 * @param grammar
 * @param table
 */
void grammar_complete_fsm(grammar_t *grammar) {
  grammar->is_ngram = false;
  grammar_convert_indexes_to_pointers(grammar);
  if (grammar->start != VOCAB_NONE && grammar->end != VOCAB_NONE) {
    fsm_grammar_add_initial_and_end_states(grammar);
  }
}
