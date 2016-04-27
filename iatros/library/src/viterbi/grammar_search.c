/*
 * grammar_search.c
 *
 *  Created on: 02-ene-2009
 *      Author: valabau
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <viterbi/grammar_search.h>
#include <viterbi/grammar.h>
#include <prhlt/constants.h>
#include <prhlt/trace.h>
#include <prhlt/vector.h>

///compares the symbols of two word states
int word_state_sym_cmp(const void *va, const void *vb) {
  symbol_t sa = (*((words_state_t**) va))->word;
  symbol_t sb = (*((words_state_t**) vb))->word;
  if (sa < sb) return -1;
  else {
    if (sa > sb) return 1;
    else return 0;
  }
}

///creates the necessary structures to search output words in states of secondary grammars
///this is useful for additional input/output ngram expansion
/**
@param grammar Grammar
@param vocab the vocab
*/
void state_grammar_build_word_search_secondary(state_grammar_t *state, const vocab_t *vocab) {
  // first case. the number state outputs is so small that linear search is the most efficient algorithm
  if (state->num_words <= 2) {
    state->search.type = SS_LINEAR_SEARCH_SECONDARY;
  }
  // second case. the number state outputs is more than 90% the vocabulary size. We use direct accessing.
  // we sum two just in case the grammar has <s> and </s>.
  else if (state->num_words + 2 > vocab->last * 0.9) {
    REQUIRE(state->num_words <= vocab->last, "Incorrect number of words in state");
    state->search.data = (words_state_t **)malloc(vocab->last * sizeof(words_state_t *));
    memset(state->search.data, 0, vocab->last * sizeof(words_state_t *));
    for (int w = 0; w < state->num_words; w++) {
      ((words_state_t **)(state->search.data))[state->words[w].word] = &(state->words[w]);
    }
    state->search.type = SS_DIRECT_ACCESS_SECONDARY;
  }
  // third case. So far, the default is to use binary search. More efficient alternatives will be
  // studied in the future
  else {
    state->search.data = (words_state_t **)malloc(state->num_words * sizeof(words_state_t *));
    for (int w = 0; w < state->num_words; w++) ((words_state_t **)(state->search.data))[w] = &(state->words[w]);
    qsort(state->search.data, state->num_words, sizeof(words_state_t*), word_state_sym_cmp);
    state->search.type = SS_BINARY_SEARCH_SECONDARY;
  }
}

///finds a word in the list of output words in a state of a secondary grammar
/**
 * @param the state
 * @param symbol the word we are searching for
 * @return the words_state_t structure for symbol or NULL if not found
 * Note: there is a distinction between primary and secondary grammars regarding this matter
 * because we assume secondary grammmar are deterministic. Therefore, one or zero words will be found.
 * However, primary grammars are can be non-deterministic. Thus, many output words can be found.
 */
words_state_t* state_grammar_find_secondary(state_grammar_t *state, const symbol_t symbol) {
  switch (state->search.type) {
  case SS_DIRECT_ACCESS_SECONDARY:
    return ((words_state_t **)(state->search.data))[symbol];
    break;
  case SS_BINARY_SEARCH_SECONDARY: {
      words_state_t key = {0, symbol, -1};
      words_state_t *key_ptr = &key;
      words_state_t **ret = (words_state_t **)bsearch(&key_ptr, state->search.data, state->num_words, sizeof(words_state_t *), word_state_sym_cmp);
      if (ret == NULL) return NULL;
      else return *ret;
    }
    break;
  case SS_LINEAR_SEARCH_SECONDARY:
  default: {// for secondary grammars linear search and no search are equivalent
      for (words_state_t *word = state->words; word < state->words + state->num_words; word++) {
        if (word->word == symbol) return word;
      }
    }
  }
  return NULL;
}

/// stores the list of words_state_t begining with the input word in_word
typedef struct {
  symbol_t in_word;
  vector_t *words;
} search_list_t;

/// creates a search list
/**
 * @return a new search list
 */
search_list_t *search_list_create() {
  search_list_t *search_list = (search_list_t *) malloc(sizeof(search_list_t));
  MEMTEST(search_list);
  search_list->words = vector_create();
  return search_list;
}

/// deletes a search list
/**
 * @param search_list a search list
 */
void search_list_delete(search_list_t *search_list) {
  vector_delete(search_list->words);
  free(search_list);
}

///compares the symbols of two search lists
int search_list_sym_cmp(const void *va, const void *vb) {
  symbol_t sa = (*((search_list_t**) va))->in_word;
  symbol_t sb = (*((search_list_t**) vb))->in_word;
  if (sa < sb) return -1;
  else {
    if (sa > sb) return 1;
    else return 0;
  }
}

///finds a word in the list of output words in a state of a primary grammar
/**
 * @param the state
 * @param symbol the word we are searching for
 * @return the words_state_t structure for symbol or NULL if not found
 * Note: there is a distinction between primary and secondary grammars regarding this matter
 * because we assume secondary grammmar are deterministic. Therefore, one or zero words will be found.
 * However, primary grammars are can be non-deterministic. Thus, many output words can be found.
 * Note: this interface should be changed to take into account the non-determinism mentioned in the
 * previous note
 */
vector_t*  state_grammar_find_primary(state_grammar_t *state, const symbol_t symbol) {
  vector_t *vector = (vector_t *)(state->search.data);
  search_list_t *search_list = NULL;

  switch (state->search.type) {
  case SS_DIRECT_ACCESS_SECONDARY:
    search_list = (search_list_t *)vector->data[symbol];
    break;
  case SS_BINARY_SEARCH_SECONDARY: {
      search_list_t key = {symbol, NULL};
      search_list = (search_list_t *) vector_bsearch(vector, &key, word_state_sym_cmp);
    }
    break;
  case SS_LINEAR_SEARCH_SECONDARY:
  default: {// for secondary grammars linear search and no search are equivalent
      for (size_t i = 0; i < vector->n_elems; i++) {
        search_list_t *sl = (search_list_t *) vector->data[i];
        if (sl->in_word == symbol) {
          search_list = sl;
          break;
        }
      }
    }
  }
  if (search_list == NULL) return NULL;
  else return search_list->words;
}


///creates the necessary structures to search output words in states of primary grammars
///this is useful for prefix search
/**
@param grammar Grammar
@param vocab the extended vocab
*/
void state_grammar_build_word_search_primary(state_grammar_t *state, const extended_vocab_t *vocab) {

  /// structure to store a temporary list of extended words begining with a given word
  vector_t *word_list = vector_create();
  vector_reserve(word_list, vocab->in->last);

  // create search_list items
  for (int in_word = 0; in_word < vocab->in->last; in_word++) {
    search_list_t *search_list = search_list_create();
    search_list->in_word = in_word;
    vector_append(word_list, search_list);
  }

  // fill the word list with the state output words
  for (int w = 0; w < state->num_words; w++) {
    // from extended_vocab_insert_symbol, extended words with null input are not allowed
    // so the next instruction is safe
    symbol_t in_word = vocab->extended_symbols[state->words[w].word].input[0];
    REQUIRE(in_word < vocab->in->last, "Invalid input word\n");
    search_list_t *search_list = (search_list_t *)word_list->data[in_word];
    vector_append(search_list->words, &(state->words[w]));
  }

  int n_distinct_in_words = 0;
  for (int in_word = 0; in_word < vocab->in->last; in_word++) {
    search_list_t *search_list = (search_list_t *)word_list->data[in_word];
    if (search_list->words->n_elems > 0) n_distinct_in_words++;
  }


  // Now, fill the search field in the state
  // first case. the number state outputs is more than 90% the vocabulary size. We use direct accessing.
  // we sum two just in case the grammar has <s> and </s>.
  if (n_distinct_in_words + 2 > vocab->in->last * 0.9) {
    REQUIRE(n_distinct_in_words <= vocab->in->last, "Incorrect number of words in state");
    state->search.data = word_list;
    state->search.type = SS_DIRECT_ACCESS;
  }
  else {
    vector_t *vector = (vector_t *) vector_create();
    vector_reserve(vector, n_distinct_in_words);
    for (int in_word = 0; in_word < vocab->in->last; in_word++) {
      search_list_t *search_list = (search_list_t *)word_list->data[in_word];
      if (search_list->words->n_elems > 0) {
        vector_append(vector, search_list);
      }
      else {
        search_list_delete(search_list);
      }
    }
    vector_delete(word_list);

    vector_qsort(vector, search_list_sym_cmp);

    // second case. the number state outputs is more than 90% the vocabulary size. We use direct accessing
    if (n_distinct_in_words <= 2) {
      state->search.type = SS_LINEAR_SEARCH;
    }
    // third case. So far, the default is to use binary search. More efficient alternatives will be
    // studied in the future
    else {
      state->search.type = SS_BINARY_SEARCH;
    }
    state->search.data = vector;
  }
}

/// releases the memory allocated by build_word_search
/**
 * @param state a grammar state
 */
void state_grammar_delete_word_search(state_grammar_t *state) {
  switch (state->search.type) {
  case SS_BINARY_SEARCH_SECONDARY:
  case SS_DIRECT_ACCESS_SECONDARY:
    free(state->search.data);
    break;
  case SS_LINEAR_SEARCH:
  case SS_BINARY_SEARCH:
  case SS_DIRECT_ACCESS:
    {
      vector_t *vector = (vector_t *) state->search.data;
      for (size_t i = 0; i < vector->n_elems; i++) {
        search_list_delete((search_list_t *) vector->data[i]);
      }
      vector_delete(vector);
    }
    break;
  default:
    break;
  }
}


/// Completes the prob and next_state word_state fields given word_state->word from a secondary
/// grammar state
/**
 * @param state a state of a secondary grammar
 * @param word_state a word state to be filled. The word field must be set to the word we are looking for
 * @return true if the word has been found. false if no arc from this state have been found (unk word)
 * Given the word in word_state->word, this function computes the probability of the word going through
 * backoff if necessary, and sets the target state. If the word could not be found, it looks for the
 * "<unk>" word. Otherwise sets word_state to {LOG_ZERO, VOCAB_NONE, STATE_NONE }
 */
bool state_grammar_fill_word_state(state_grammar_t *state, words_state_t* word_state) {
  float bo = 0.0;

  // while we can go backoff
  bool not_exit = true;
  while (not_exit) {
    const words_state_t *ws = state_grammar_find_secondary(state, word_state->word);
    if (ws != NULL) {
      // we found it, update fields and exit
      word_state->prob = bo + ws->prob;
      word_state->state_next = ws->state_next;
      return true;
    }
    // we did not found the word, go backoff
    if (state->state_bo != NULL) {
      bo += state->bo;
      state = state->state_bo;
    }
    else { // if no further backoff go find unk word
      not_exit = false;
    }
  }

  { // there is no backoff state so we take it as an unknown word
    const words_state_t *ws = state_grammar_find_secondary(state, UNK_WORD);
    // unknown word probability
    if (ws != NULL) {
      *word_state = *ws;
      word_state->prob += bo;
    }
    // zero probability
    else {
      word_state->prob = LOG_ZERO;
      word_state->state_next = state; // keep it in the unigram
      word_state->word = VOCAB_NONE;
    }
  }
  return false;
}
