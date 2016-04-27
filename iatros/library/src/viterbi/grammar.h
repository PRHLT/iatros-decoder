/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#ifndef _GRAMMAR_H
#define _GRAMMAR_H
#include <stdio.h>
#include <stdlib.h>
#include <iatros/grammar_search.h>
#include <prhlt/utils.h>
#include <prhlt/extended_vocab.h>
#include <iatros/lex.h>

#ifdef __cplusplus
extern "C" {
#endif

/// type of grammar vocabulary
typedef enum { GV_BILINGUAL, GV_INPUT, GV_OUTPUT, MAX_GV} grammar_vocab_type_t;
/// type of grammar
typedef enum { FSM_GRAMMAR = 0, NGRAM_GRAMMAR, PHRASE_TABLE_GRAMMAR, LAT_GRAMMAR, MAX_GRAMMAR_TYPE } grammar_type_t;

/// Initial and end states
typedef struct prob_state {
  float prob; ///< Probability of state
  state_grammar_t *state; ///< Number of state
} prob_state_t;

/// List of initial and end states
typedef struct list_states {
  prob_state_t *vector; ///< Vector with the initial or end states
  int num_elements; ///< Number of states in vector
} list_states_t;

/// Information of next state.
struct words_state_t {
  state_grammar_t * state_next; ///< Number of the state in vector of word
  symbol_t word; ///< Name of word
  float prob; ///< Probability of word
};

/// Node of state in vector of states.
struct state_grammar_t {
  words_state_t *words; ///< List of words in edges that out of state
  int num_words; ///< Num words in the list of word
  symbol_t *name; ///< Name of state
  float bo; ///< Probability of backoff
  state_grammar_t *state_bo; ///< State of backoff
  int num_state; ///< Number of the state in vector
  state_search_t search;
};
#define STATE_NONE NULL

/// Grammar.
typedef struct grammar_t {
  list_states_t *list_initial; ///< List of initial states
  list_states_t *list_end; ///< List of end states
  bool is_ngram; ///< If grammar is a ngram or not
  state_grammar_t **vector; ///< Vector with the states in grammar
  int num_states; ///< Number of states in grammar
  int n; ///< Number of n in n-gram

  lex_t *lex; ///< Lexicon
  extended_vocab_t *vocab; ///< Vocabulary
  grammar_vocab_type_t vocab_type; ///< Vocabulaty type

  symbol_t silence; ///< Number of word silence
  float silence_score;   ///< Silence score
  bool force_silence;    ///< Force silence at beginning and end of decoding
  symbol_t pause; ///< Number of word short pause
  symbol_t start; ///< Number of word intial
  symbol_t end; ///< Number of word end
} grammar_t;

grammar_type_t get_grammar_type(const char *grammar_type_str);
grammar_t * grammar_create(lex_t *lex, extended_vocab_t *vocab);
grammar_t * grammar_create_secondary(grammar_t *base_grammar, grammar_vocab_type_t vocab_type);
void grammar_delete(grammar_t *grammar);
void grammar_load(grammar_t *grammar, FILE *aux, grammar_type_t type);
bool grammar_is_end_word(const grammar_t *grammar, symbol_t word);
int  grammar_is_final_state(const grammar_t *grammar, const state_grammar_t * state);
int  grammar_is_initial_state(const grammar_t *grammar, state_grammar_t * state);
void grammar_append(grammar_t *grammar, state_grammar_t * state_grammar);
void grammar_convert_indexes_to_pointers(grammar_t *grammar);
void grammar_sort_by_prob(grammar_t *grammar);
void grammar_build_word_search(grammar_t *grammar);
void grammar_write_dot(const grammar_t *grammar, FILE* file);
void grammar_write_slf(const grammar_t *grammar, FILE* file);


state_grammar_t *state_grammar_create();
int state_grammar_append(state_grammar_t * state, symbol_t word, float prob, state_grammar_t * state_next);
void state_grammar_set_name(state_grammar_t * state, symbol_t *syms);
void state_grammar_delete(state_grammar_t *state);

void list_states_clear(list_states_t *list);
void list_states_append(list_states_t *list, state_grammar_t *state_id, float prob);

#ifdef __cplusplus
}
#endif

#endif // _GRAMMAR_h


