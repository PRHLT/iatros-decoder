/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#include <iatros/config.h>
#include <prhlt/constants.h>
#include <prhlt/utils.h>
#include <viterbi/parsers/fsm-parser/fsm-driver.h>
#include <viterbi/parsers/ng-parser/ng-driver.h>
#include <viterbi/parsers/pt-parser/pt-driver.h>
//#include <viterbi/parsers/lat-parser/lat-driver.h>
#include <viterbi/grammar.h>
#include <prhlt/trace.h>
#include <prhlt/gzip.h>
#include <prhlt/vector.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <search.h>


grammar_type_t get_grammar_type(const char *grammar_type_str) {
  grammar_type_t type = MAX_GRAMMAR_TYPE;
  if (grammar_type_str == NULL)                    type = MAX_GRAMMAR_TYPE;
  else if (strcmp(grammar_type_str, "NGRAM") == 0) type = NGRAM_GRAMMAR;
  else if (strcmp(grammar_type_str, "FSM") == 0)   type = FSM_GRAMMAR;
  else if (strcmp(grammar_type_str, "PHRASE_TABLE") == 0) type = PHRASE_TABLE_GRAMMAR;
  else if (strcmp(grammar_type_str, "LAT") == 0)   type = LAT_GRAMMAR;
  return type;
}


///Returns the index in the list_end list of -1 if not found
/**
@param grammar Grammar
@param state State
*/
int grammar_is_final_state(const grammar_t *grammar, const state_grammar_t * state) {
  //Look the list of end states
  for (int l = 0; l < grammar->list_end->num_elements; l++) {
    if (state == grammar->list_end->vector[l].state) {
      return l;
    }
  }
  return -1;
}

///Returns the index in the list_initial list of -1 if not found
/**
@param grammar Grammar
@param state State
*/
int grammar_is_initial_state(const grammar_t *grammar, state_grammar_t * state) {
  //Look the list of end states
  for (int l = 0; l < grammar->list_initial->num_elements; l++) {
    if (state == grammar->list_initial->vector[l].state) {
      return l;
    }
  }
  return -1;
}

///Returns if the word is a final word or not
/**
@param grammar Grammar
@param word Word
*/
bool grammar_is_end_word(const grammar_t *grammar, symbol_t word) {
  if (!grammar->force_silence || word == grammar->silence) return true;
  else return false;
}

///Creates a nre grammar state
/**
 * @return a new grammar state
 */
state_grammar_t *state_grammar_create() {
  state_grammar_t * state_grammar = (state_grammar_t *) malloc(sizeof(state_grammar_t));
  MEMTEST(state_grammar);

  state_grammar->words = NULL;
  state_grammar->num_words = 0;

  state_grammar->name = (symbol_t *) malloc(sizeof(symbol_t));
  state_grammar->name[0] = VOCAB_NONE;

  state_grammar->num_state = 0;

  state_grammar->bo = LOG_ZERO;
  state_grammar->state_bo = STATE_NONE;

  state_grammar->search.type = SS_NO_SEARCH;
  state_grammar->search.data = NULL;

  return state_grammar;
}

///Appends a new outgoing arc at the end of the list
/**
 * @param state
 * @param word
 * @param prob
 * @param state_next the next state folowing the arc
 */
int state_grammar_append(state_grammar_t * state, symbol_t word, float prob, state_grammar_t * state_next) {
  state->words = (words_state_t *) realloc(state->words, (state->num_words + 1) * sizeof(words_state_t));
  MEMTEST(state->words);

  state->words[state->num_words].word = word;
  state->words[state->num_words].prob = prob;
  state->words[state->num_words].state_next = state_next;

  state->num_words++;
  return state->num_words - 1;
}

/// sets the grammar state name
/**
 * @param state a grammar state
 * @param syms string of syms
 * @param len the length of the string
 */
void state_grammar_set_name(state_grammar_t * state, symbol_t *syms) {
  free(state->name);
  state->name = symdup(syms);
}

///Appends a new grammar state at the end of the list to the grammar and links the grammar id
/**
 * @param grammar
 * @param state_grammar
 */
void grammar_append(grammar_t *grammar, state_grammar_t * state_grammar) {
  grammar->vector = (state_grammar_t **) realloc(grammar->vector, sizeof(state_grammar_t *) * (grammar->num_states + 1));
  MEMTEST(grammar->vector);
  grammar->vector[grammar->num_states] = state_grammar;
  state_grammar->num_state = grammar->num_states;
  grammar->num_states++;
}



///compares the probability of two word states
int word_state_prob_cmp(const void *va, const void *vb) {
  const float pa = (*((words_state_t**) va))->prob;
  const float pb = (*((words_state_t**) vb))->prob;
  if (pa < pb) return 1;
  else {
    if (pa > pb) return -1;
    else return 0;
  }
}

///sorts a vector of words_state_t
/**
@param state a grammar state
@param cmp a function to compare states
*/
void state_grammar_sort(state_grammar_t *state, cmp_fn cmp) {
  words_state_t *word_copy = (words_state_t *) malloc(state->num_words * sizeof(words_state_t));
  words_state_t **word_ptrs = (words_state_t **) malloc(state->num_words * sizeof(words_state_t *));
  for (int w = 0; w < state->num_words; w++) {
    word_copy[w] = state->words[w];
    word_ptrs[w] = &word_copy[w];
  }
  qsort(word_ptrs, state->num_words, sizeof(words_state_t*), cmp);
  int first_unreachable = -1;
  for (int w = 0; w < state->num_words; w++) {
    if (!is_logzero(word_ptrs[w]->prob)) {
      state->words[w] = *word_ptrs[w];
    }
    else if (first_unreachable == -1) {
      first_unreachable = w;
    }
  };
  if (first_unreachable != -1) {
    state->num_words = first_unreachable;
    state->words = (words_state_t *) realloc(state->words, state->num_words * sizeof(words_state_t));
  }
  free(word_copy);
  free(word_ptrs);
}



///< Clears list of states
/**
 * @param list
 */
void list_states_clear(list_states_t *list) {
  list->num_elements = 0;
  free(list->vector);
  list->vector = NULL;
}

///< Appends a state to the list of states
/**
 * @param list
 * @param state_id
 * @param prob
 */
void list_states_append(list_states_t *list, state_grammar_t *state, float prob) {
  list->vector = (prob_state_t*) realloc(list->vector, (list->num_elements + 1)*sizeof(prob_state_t));
  list->vector[list->num_elements].state = state;
  list->vector[list->num_elements].prob = prob;
  list->num_elements++;
}




///sorts the output words by their probability in the grammar states so that word expansions can be
///performed more efficiently in the decoding process
/**
@param grammar Grammar
*/
void grammar_sort_by_prob(grammar_t *grammar) {
  for (int i = 0; i < grammar->num_states; i++) {
    state_grammar_sort(grammar->vector[i], word_state_prob_cmp);
  }
}

///creates the necessary structures to search output words in grammar states
///this is useful for additional input/output ngram expansion and for prefix search
/**
@param grammar Grammar
*/
void grammar_build_word_search(grammar_t *grammar) {
  for (int i = 0; i < grammar->num_states; i++) {
    switch (grammar->vocab_type) {
    case GV_INPUT:
      state_grammar_build_word_search_secondary(grammar->vector[i], grammar->vocab->in);
      break;
    case GV_OUTPUT:
      state_grammar_build_word_search_secondary(grammar->vector[i], grammar->vocab->out);
      break;
    case GV_BILINGUAL:
      state_grammar_build_word_search_primary(grammar->vector[i], grammar->vocab);
      break;
    default:
      ERROR("Invalid grammar vocab type\n");
    }
  }
}


///Converts indexes to pointers in grammar
/**
 * @param grammar grammar
 */
void grammar_convert_indexes_to_pointers(grammar_t *grammar) {
  for (int i = 0; i < grammar->num_states; i++) {
    for (int w = 0; w < grammar->vector[i]->num_words; w++) {
      size_t state_idx = (size_t) grammar->vector[i]->words[w].state_next;
      if (state_idx >= (size_t) grammar->num_states) {
        REQUIRE(state_idx < (size_t) grammar->num_states, "Incorrect state number in grammar");
      }
      else {
        grammar->vector[i]->words[w].state_next = grammar->vector[state_idx];
      }
    }
    //XXX: What should we do with backoff states. In principle, when converting
    // indexes is needed, backoff arcs are not, so we comment it out for the moment
    //grammar->vector[i]->state_bo = grammar->vector[grammar->vector[i]->state_bo];
  }
  for (int l = 0; l < grammar->list_initial->num_elements; l++) {
    size_t state_idx = (size_t) grammar->list_initial->vector[l].state;
    if (state_idx >= (size_t) grammar->num_states) {
      REQUIRE(state_idx < (size_t) grammar->num_states, "Incorrect state number in grammar");
    }
    else {
      grammar->list_initial->vector[l].state = grammar->vector[state_idx];
    }
  }
  for (int l = 0; l < grammar->list_end->num_elements; l++) {
    size_t state_idx = (size_t) grammar->list_end->vector[l].state;
    if (state_idx >= (size_t) grammar->num_states) {
      REQUIRE(state_idx < (size_t) grammar->num_states, "Incorrect state number in grammar");
    }
    else {
      grammar->list_end->vector[l].state = grammar->vector[state_idx];
    }
  }
}

///Load grammar
/**
@param grammar Grammar
@param aux File with grammar
@param type type of grammar
*/
void grammar_load(grammar_t *grammar, FILE *file, grammar_type_t type) {

  //Load grammar
  if (type == NGRAM_GRAMMAR) {
    grammar_load_ngram(grammar, file);
  } else if (type == FSM_GRAMMAR) {
    grammar_load_fsm(grammar, file);
  } else if (type == PHRASE_TABLE_GRAMMAR) {
    grammar_load_phrase_table(grammar, file);
//  } else if (type == LAT_GRAMMAR) {
//    grammar_load_lat(grammar, file);
  } else {
    FAIL("Unknown type of grammar\n");
  }

  grammar_sort_by_prob(grammar);
  //XXX: the build word search process is quite consuming for large
  //     corpus (WSJ 64K). For this reason, we move the function out.
  //     Just call it in case it is needed.
  //grammar_build_word_search(grammar);
}

///create a grammar
/**
@param lex lexicon that has the pronounciations of the words in grammar
*/
grammar_t * grammar_create(lex_t *lex, extended_vocab_t *vocab) {

  grammar_t * grammar = (grammar_t *) malloc(sizeof(grammar_t));
  MEMTEST(grammar);

  grammar->vector = (state_grammar_t **) malloc(sizeof(state_grammar_t *));
  MEMTEST(grammar->vector);


  grammar->vocab = vocab;
  grammar->lex   = lex;
  grammar->silence = VOCAB_NONE;
  grammar->force_silence = false;
  grammar->pause   = VOCAB_NONE;
  grammar->start   = VOCAB_NONE;
  grammar->end     = VOCAB_NONE;

  grammar->num_states = 0;

  //Create list of initial states
  grammar->list_initial = (list_states_t *) malloc(sizeof(list_states_t));
  MEMTEST(grammar->list_initial);
  grammar->list_initial->vector = NULL;
  grammar->list_initial->num_elements = 0;

  //Create list of end states
  grammar->list_end = (list_states_t *) malloc(sizeof(list_states_t));
  MEMTEST(grammar->list_end);
  grammar->list_end->vector = NULL;
  grammar->list_end->num_elements = 0;

  grammar->vocab_type = GV_BILINGUAL;
  return grammar;
}

///create an input grammar
/**
@param lex lexicon that has the pronounciations of the words in grammar
*/
grammar_t * grammar_create_secondary(grammar_t *base_grammar, grammar_vocab_type_t vocab_type) {

  grammar_t * grammar = (grammar_t *) malloc(sizeof(grammar_t));
  MEMTEST(grammar);

  *grammar = *base_grammar;

  grammar->vector = (state_grammar_t **) malloc(sizeof(state_grammar_t *));
  MEMTEST(grammar->vector);

  grammar->num_states = 0;

  //Create list of initial states
  grammar->list_initial = (list_states_t *) malloc(sizeof(list_states_t));
  MEMTEST(grammar->list_initial);
  grammar->list_initial->vector = NULL;
  grammar->list_initial->num_elements = 0;

  //Create list of end states
  grammar->list_end = (list_states_t *) malloc(sizeof(list_states_t));
  MEMTEST(grammar->list_end);
  grammar->list_end->vector = NULL;
  grammar->list_end->num_elements = 0;

  grammar->vocab_type = vocab_type;
  grammar->lex = NULL;

  return grammar;
}


///deletes the grammar state
/**
@param state Grammar state
*/
void state_grammar_delete(state_grammar_t *state) {
  state_grammar_delete_word_search(state);
  free(state->words);
  free(state->name);
  free(state);
}

///deletes the grammar
/**
@param grammar Grammar
*/
void grammar_delete(grammar_t *grammar){
  for (int i = 0; i < grammar->num_states; i++) {
    state_grammar_delete(grammar->vector[i]);
  }
  free(grammar->vector);

  free(grammar->list_initial->vector);
  free(grammar->list_initial);
  free(grammar->list_end->vector);
  free(grammar->list_end);

  free(grammar);
}


/** writes the grammar in dot format to a file
 *
 */
void grammar_write_dot(const grammar_t *grammar, FILE* file) {
  fprintf(file, "# Word graph in SLF format generated by " IATROS_PROJECT_STRING "\n");
  fprintf(file, "digraph slf {\n");
  fprintf(file, "rankdir=LR;\n");
  fprintf(file, "size=\"8,5\"\n");

  for (int l = 0; l < grammar->list_initial->num_elements; l++) {
    const state_grammar_t *state = grammar->list_initial->vector[l].state;
    fprintf(file, "node [shape = circle, style = filled]; S%d\n", state->num_state);
  }

  for (int l = 0; l < grammar->list_end->num_elements; l++) {
    const state_grammar_t *state = grammar->list_end->vector[l].state;
    fprintf(file, "node [shape = doublecircle, fillcolor = transparent]; S%d\n", state->num_state);
  }

  fprintf(file, "# Node definitions\n");
  fprintf(file, "node [shape = circle, fillcolor = transparent];\n");
  for (int i = 0; i < grammar->num_states; i++) {
    const state_grammar_t *state = grammar->vector[i];

    if (state->name != NULL) {
      int len = symlen(state->name);
      if (len > 0 && state->name[len - 1] == grammar->end) {
        fprintf(file, "node [shape = doublecircle]; S%d\n", state->num_state);
      }
    }

    char *name = NULL;
    extended_vocab_symbols_to_string(state->name, grammar->vocab, &name);
    fprintf(file, "S%d [ label = \"S%d\\n%s\"]\n", state->num_state, state->num_state, name);
    free(name);
  }

  fprintf(file, "# Link definitions\n");
  for (int i = 0; i < grammar->num_states; i++) {
    const state_grammar_t *state = grammar->vector[i];
    for (int w = 0; w < state->num_words; w++) {
      const words_state_t *ws = &state->words[w];
      fprintf(file, "S%d -> S%d [ label = \"%s: %g\"]\n", state->num_state,
                    ws->state_next->num_state,
                    extended_vocab_get_string(grammar->vocab, ws->word),
                    ws->prob);
    }
    if (!is_logzero(state->bo) && state->state_bo != STATE_NONE) {
      fprintf(file, "S%d -> S%d [ label = \"<BACKOFF>: %g\"]\n", state->num_state,
                    state->state_bo->num_state,
                    state->bo);
    }
  }
  fprintf(file, "}\n");

}

/** write the grammar in HTK lattice format to a file
 *
 */
void grammar_write_slf(const grammar_t *grammar, FILE* file) {
  fprintf(file, "# Word graph in SLF format generated by " IATROS_PROJECT_STRING "\n");
  fprintf(file, "VERSION=1.0\n");
  fprintf(file, "UTTERANCE=grammar\n");
  fprintf(file, "lmscale=1\n");
  fprintf(file, "wdpenalty=0\n");
  size_t n_links = 0;
  for (int i = 0; i < grammar->num_states; i++) {
    const state_grammar_t *state = grammar->vector[i];
    n_links += state->num_words;
    if (!is_logzero(state->bo) && state->state_bo != STATE_NONE) n_links++;
  }
  fprintf(file, "N=%d L=%zu\n", grammar->num_states, n_links);
  fprintf(file, "# Node definitions\n");

  for (int i = 0; i < grammar->num_states; i++) {
    const state_grammar_t *state = grammar->vector[i];
    fprintf(file, "I=%d", state->num_state);
//    if (state->name != NULL && state->name[0] != VOCAB_NONE) {
//      fprintf(file, "    # name =");
//      int s = 0;
//      while (state->name[s] != VOCAB_NONE) {
//        fprintf(file, " %s", extended_vocab_get_string(grammar->vocab, state->name[s]));
//        s++;
//      }
//    }
    fprintf(file, "\n");
  }
  fprintf(file, "# Link definitions\n");
  n_links = 0;
  for (int i = 0; i < grammar->num_states; i++) {
    const state_grammar_t *state = grammar->vector[i];
    for (int w = 0; w < state->num_words; w++) {
      const words_state_t *ws = &state->words[w];
      const extended_symbol_t *word = extended_vocab_get_extended_symbol(grammar->vocab, ws->word);

      char *input = NULL;
      vocab_symbols_to_string(word->input, grammar->vocab->in, &input);
      {  // convert spaces to underscores so that output is SRILM compatible
        char *ptr = input;
        while (*ptr != '\0') {if (*ptr == ' ') *ptr = '_'; ptr++;}
      }
      fprintf(file, "J=%zu S=%d E=%d W=%s",
          n_links++,
          state->num_state,
          ws->state_next->num_state,
          input);
      free(input);

      if (word->output != NULL && word->output[0] != VOCAB_NONE) {
        char *output = NULL;
        vocab_symbols_to_string(word->output, grammar->vocab->out, &output);
        {  // convert spaces to underscores so that output is SRILM compatible
          char *ptr = output;
          while (*ptr != '\0') {if (*ptr == ' ') *ptr = '_'; ptr++;}
        }
        fprintf(file, " O=%s", output);
        free(output);
      }
      fprintf(file, " l=%f\n", ws->prob);
    }
    if (!is_logzero(state->bo) && state->state_bo != STATE_NONE) {
      fprintf(file, "J=%zu S=%d E=%d W=!NULL l=%f\n", n_links++,
                    state->num_state,
                    state->state_bo->num_state,
                    state->bo);
    }
  }

}
