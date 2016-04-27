/*
 * cat.c
 *
 *  Created on: 17-feb-2009
 *      Author: valabau
 */

#include <prhlt/gzip.h>
#include <viterbi/cat.h>
#include <string.h>
#include <prhlt/trace.h>



typedef struct {
  vector_t *fifo;
  hash_t *hash;
} search_prefix_t;

typedef struct {
  struct prefix_fifo_t *prev_pf;
  int word_idx;
} incoming_t;

typedef struct prefix_fifo_t {
  const state_grammar_t *base_state;
  state_grammar_t *state;
  const symbol_t *input;
  const symbol_t *output;
  vector_t *incoming;
  float initial_prob;
  bool is_accesible;
} prefix_fifo_t;

typedef struct {
  const state_grammar_t *base_state;
  const symbol_t *input;
  const symbol_t *output;
} prefix_fifo_key_t;


/** Hash function for hypothesis
 * @param hyp the hypothesis
 */
INLINE unsigned int pf_hash(const prefix_fifo_key_t *pf) {
  return ((size_t)pf->base_state) + ((size_t)pf->input << 3) + ((size_t)pf->output << 6);
}

/** Compares two hypotheses
 * @param h1 an hypothesis
 * @param h2 an hypothesis
 * @return true if h1 == h2, false otherwise
 */
INLINE int pf_cmp(const void *_pf1, const void *_pf2) {
  const prefix_fifo_t *pf1 = (const prefix_fifo_t *) _pf1;
  const prefix_fifo_t *pf2 = (const prefix_fifo_t *) _pf2;
  return pf1->base_state == pf2->base_state && pf1->input == pf2->input && pf1->output == pf2->output;
}

void expand_word_state(search_prefix_t *search, prefix_fifo_t * prefix_info,
                       const struct words_state_t *word_state,
                       const symbol_t *ip, const symbol_t *op, float bo)
{
  prefix_fifo_key_t key = { word_state->state_next, ip, op };
  prefix_fifo_t *pf = (prefix_fifo_t *)hash_search((void *)&key, sizeof(prefix_fifo_key_t), search->hash);
  if (pf == NULL) {
    pf = (prefix_fifo_t *) malloc(sizeof(prefix_fifo_t));
    pf->base_state = word_state->state_next;
    pf->input = ip;
    pf->output = op;

    pf->state = state_grammar_create();
    pf->state->name = (symbol_t*) realloc(pf->state->name, sizeof(symbol_t) * (symlen(pf->base_state->name) + 1));
    symcpy(pf->state->name, pf->base_state->name);
    pf->initial_prob = LOG_ZERO;
    pf->is_accesible = false;
    pf->incoming = vector_create();

    vector_append(search->fifo, pf);
    hash_insert((void *)&key, sizeof(prefix_fifo_key_t), pf, search->hash);
  }

  incoming_t *incoming = (incoming_t *) malloc(sizeof(incoming_t));
  MEMTEST(incoming);
  incoming->prev_pf = prefix_info;
  incoming->word_idx = state_grammar_append(prefix_info->state, word_state->word, word_state->prob + bo, pf->state);
  vector_append(pf->incoming, incoming);
}

///Creates a nre grammar state
/**
 * @return a new grammar state
 */
void expand_from_prefix(search_prefix_t *search, prefix_fifo_t * prefix_info,
                             const extended_vocab_t *vocab,
                             const symbol_t *input_subwords,
                             const symbol_t *output_subwords)
{
  if ((prefix_info->input  == NULL || *prefix_info->input == VOCAB_NONE) &&
      (prefix_info->output == NULL || *prefix_info->output == VOCAB_NONE))
  {
    prefix_info->is_accesible = true;
  }
  else {
    bool *visited = (bool *) malloc(vocab->extended->last * sizeof(bool));
    memset(visited, false, vocab->extended->last * sizeof(bool));
    const state_grammar_t * curr_state = prefix_info->base_state;
    bool has_backoff = true;
    float bo = 0.0;
    size_t n_expansions = 0;
    float smallest_word_score = .0;
    while (has_backoff) {
      for (int w = 0; w < curr_state->num_words; w++) {
        const symbol_t word = curr_state->words[w].word;
        smallest_word_score = curr_state->words[w].prob;
        if (!visited[word]) {
          visited[word] = true;
          const symbol_t *ip = prefix_info->input;
          const symbol_t *is = input_subwords;
          const symbol_t *op = prefix_info->output;
          const symbol_t *os = output_subwords;

          if (extended_vocab_symbol_is_compatible(vocab, word, &ip, &is, &op, &os)) {
            n_expansions++;
            expand_word_state(search, prefix_info, &(curr_state->words[w]), ip, op, bo);
          }
        }
      }
      if (curr_state->state_bo != STATE_NONE && !is_logzero(curr_state->bo)) {
        bo += curr_state->bo;
        curr_state = curr_state->state_bo;
      }
      else {
        has_backoff = false;
      }
    }
    free(visited);

    if (n_expansions == 0 && vocab->extended->unk_word != NULL) {
      const symbol_t *ip = prefix_info->input;
      const symbol_t *op = prefix_info->output;
      if (ip != NULL && *ip != VOCAB_NONE) ip++;
      if (op != NULL && *op != VOCAB_NONE) op++;
      smallest_word_score = .0; bo = .0;
      struct words_state_t word_state = {(state_grammar_t *) curr_state, UNK_WORD, smallest_word_score};
      expand_word_state(search, prefix_info, &word_state, ip, op, bo);
    }
  }
}

/** Creates a prefix grammar from a base grammar and a input/ouput prefix
 * @param base_grammar the grammar that will be used as base to construct the prefix grammar
 * @param input_prefix
 * @param output_prefix
 *
 * Note: There is a problem when there are arcs in the grammar with lambda output.
 * If that happens, the algorithm enters in an infinite loop, since no output symbol
 * is consumed when using that symbol
 */
grammar_t * grammar_create_from_prefix(const grammar_t *base_grammar, const symbol_t *input_prefix, const symbol_t *output_prefix) {
  REQUIRE(base_grammar->vocab_type == GV_BILINGUAL, "Error: cannot create a prefix from secondary grammars");

  if (base_grammar->start != VOCAB_NONE) {
    if (input_prefix != NULL && *input_prefix == base_grammar->start)   { input_prefix++; }
    if (output_prefix != NULL && *output_prefix == base_grammar->start) { output_prefix++; }
  }


  grammar_t * grammar = (grammar_t *) malloc(sizeof(grammar_t));
  MEMTEST(grammar);

  // copy all base grammar fields
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
  for (int l = 0; l < base_grammar->list_end->num_elements; l++) {
    const prob_state_t *ps = &base_grammar->list_end->vector[l];
    list_states_append(grammar->list_end, ps->state, ps->prob);
  }

  if ((input_prefix  == NULL || *input_prefix == VOCAB_NONE) &&
      (output_prefix == NULL || *output_prefix == VOCAB_NONE))
  {
    for (int l = 0; l < base_grammar->list_initial->num_elements; l++) {
      const prob_state_t *ps = &base_grammar->list_initial->vector[l];
      list_states_append(grammar->list_initial, ps->state, ps->prob);
    }
  }
  else {
    // initialise fifo with that states
    search_prefix_t search;
    search.fifo = vector_create();
    search.hash = hash_create(271, NULL);
    for (int l = 0; l < base_grammar->list_initial->num_elements; l++) {
      prefix_fifo_key_t key = { base_grammar->list_initial->vector[l].state, input_prefix, output_prefix };
      prefix_fifo_t *pf = (prefix_fifo_t *)hash_search((void *) &key, sizeof(prefix_fifo_key_t), search.hash);
      if (pf == NULL) {
        pf = (prefix_fifo_t *) malloc(sizeof(prefix_fifo_t));
        pf->base_state = base_grammar->list_initial->vector[l].state;
        pf->input = input_prefix;
        pf->output = output_prefix;

        pf->state = state_grammar_create();
        pf->state->name = (symbol_t*) realloc(pf->state->name, sizeof(symbol_t) * (symlen(pf->base_state->name) + 1));
        symcpy(pf->state->name, pf->base_state->name);
        pf->incoming = vector_create();
        pf->initial_prob = base_grammar->list_initial->vector[l].prob;
        pf->is_accesible = false;
        vector_append(search.fifo, pf);
        hash_insert((void *) &key, sizeof(prefix_fifo_key_t), pf, search.hash);
      }
    }

    size_t fifo_idx = 0;
    // build grammar from preffix
    while (fifo_idx < search.fifo->n_elems) {
      prefix_fifo_t *prev_pf = (prefix_fifo_t *) search.fifo->data[fifo_idx++];
      expand_from_prefix(&search, prev_pf, base_grammar->vocab, NULL, NULL);
    }

    for (int i = search.fifo->n_elems - 1; i >= 0; i--) {
      prefix_fifo_t *pf = (prefix_fifo_t *) search.fifo->data[i];
      // XXX: this while loop should be made in reverse topological order
      // until it is done, we just ignore that some paths should be pruned
      // this is valid for complete prefixes
      pf->is_accesible = true;
      if (pf->is_accesible) {
        int l = grammar_is_final_state(base_grammar, pf->base_state);
        bool is_final = false;
        if (l != -1) {
          list_states_append(grammar->list_end, pf->state, base_grammar->list_end->vector[l].prob);
          is_final = true;
        }

        // it is an initial state
        if (!is_logzero(pf->initial_prob)) {
          list_states_append(grammar->list_initial, pf->state, pf->initial_prob);
        }
        else if (pf->incoming == NULL || pf->incoming->n_elems == 0) {
          pf->is_accesible = false;
        }

        // it is a regular state
        for (size_t n = 0; n < pf->incoming->n_elems; n++) {
          incoming_t *incoming = (incoming_t *)pf->incoming->data[n];
          incoming->prev_pf->is_accesible = true;
        }
        // link the arc with the base grammar if we are at the end of the prefix
        if ((pf->input  == NULL || *pf->input == VOCAB_NONE) &&
            (pf->output == NULL || *pf->output == VOCAB_NONE) && !is_final)
        {
          for (size_t n = 0; n < pf->incoming->n_elems; n++) {
            incoming_t *incoming = (incoming_t *)pf->incoming->data[n];
            incoming->prev_pf->state->words[incoming->word_idx].state_next = (state_grammar_t *)pf->base_state;
          }
          pf->is_accesible = false;
        }
      }
      else {
        for (size_t n = 0; n < pf->incoming->n_elems; n++) {
          incoming_t *incoming = (incoming_t *)pf->incoming->data[n];
          incoming->prev_pf->state->words[incoming->word_idx].prob = LOG_ZERO;
        }
      }
    }

    for (size_t i = 0; i < search.fifo->n_elems; i++) {
      prefix_fifo_t *pf = (prefix_fifo_t *) search.fifo->data[i];
      if (pf->is_accesible) {
        grammar_append(grammar, pf->state);
      }
      else {
        state_grammar_delete(pf->state);
      }
      if (pf->incoming != NULL) {
        for (size_t n = 0; n < pf->incoming->n_elems; n++) {
          free(pf->incoming->data[n]);
        }
        vector_delete(pf->incoming);
      }
      free(pf);
    }
    vector_delete(search.fifo);
    hash_delete(search.hash, false);

    grammar_sort_by_prob(grammar);
    grammar_build_word_search(grammar);
  }

  //size_t fifo_idx = 0;
  return grammar;
}


/** Creates a prefix grammar from a base grammar and a input prefix (but prefix not included in grammar)
 * @param base_grammar the grammar that will be used as base to construct the prefix grammar
 * @param input_prefix
 * @param output_prefix
 *
 * Note: There is a problem when there are arcs in the grammar with lambda output.
 * If that happens, the algorithm enters in an infinite loop, since no output symbol
 * is consumed when using that symbol
 *
 * XXX: Currently, it only works for only input single word n-grams which are the most common case
 */
grammar_t * grammar_create_conditioned_to_prefix(const grammar_t *base_grammar, const symbol_t *input_prefix) {
  REQUIRE(base_grammar->vocab_type == GV_BILINGUAL, "Error: cannot create a wordlist grammar from secondary grammars");
  REQUIRE(base_grammar->list_initial->num_elements == 1, "Error: cannot create a wordlist grammar from non n-gram grammars");
  grammar_t * grammar = (grammar_t *) malloc(sizeof(grammar_t));
  MEMTEST(grammar);

  // copy all base grammar fields
  *grammar = *base_grammar;

  grammar->vector = (state_grammar_t **) malloc(sizeof(state_grammar_t *));
  MEMTEST(grammar->vector);
  grammar->num_states = 0;

  // find n-gram state that matches the prefix
  state_grammar_t *initial_state = base_grammar->list_initial->vector[0].state;
  symbol_t const * word = input_prefix;
  while (*word != VOCAB_NONE) {
    words_state_t ws = { STATE_NONE, *word, LOG_ZERO };
    state_grammar_fill_word_state(initial_state, &ws);
    if (verbose > 1) {
      char *name = NULL, *name2 = NULL;
      extended_vocab_symbols_to_string(initial_state->name, grammar->vocab, &name);
      if (ws.state_next != STATE_NONE) {
        extended_vocab_symbols_to_string(ws.state_next->name, grammar->vocab, &name2);
      }
      PRINT("state: %s - %s -> %s, %s, p = %g\n", name, extended_vocab_get_string(grammar->vocab, *word), name2, extended_vocab_get_string(grammar->vocab, ws.word), ws.prob);
      free(name);
      free(name2);
    }
    initial_state = ws.state_next;
    word++;
  }

  if (initial_state == NULL) {
    ERROR("Unknown initial state");
  }

  //Create list of initial states
  grammar->list_initial = (list_states_t *) malloc(sizeof(list_states_t));
  MEMTEST(grammar->list_initial);
  grammar->list_initial->vector = NULL;
  grammar->list_initial->num_elements = 0;
  list_states_append(grammar->list_initial, initial_state, LOG_ONE);

  //Create list of end states
  grammar->list_end = (list_states_t *) malloc(sizeof(list_states_t));
  MEMTEST(grammar->list_end);
  grammar->list_end->vector = NULL;
  grammar->list_end->num_elements = 0;
  for (int l = 0; l < base_grammar->list_end->num_elements; l++) {
    const prob_state_t *ps = &base_grammar->list_end->vector[l];
    list_states_append(grammar->list_end, ps->state, ps->prob);
  }

  grammar_sort_by_prob(grammar);
  grammar_build_word_search(grammar);

  //size_t fifo_idx = 0;
  return grammar;
}


/** Creates a wordlist grammar from a base grammar and a input/ouput prefix
 * @param base_grammar the grammar that will be used as base to construct the prefix grammar
 * @param input_prefix
 * @param output_prefix
 *
 * A wordlist grammar is a grammar that can only recognize one word.
 * Note: There is a problem when there are arcs in the grammar with lambda output.
 * If that happens, the algorithm enters in an infinite loop, since no output symbol
 * is consumed when using that symbol
 *
 * XXX: Currently, it only works for only input single word n-grams which are the most common case
 */
grammar_t * grammar_create_wordlist_from_prefix(const grammar_t *base_grammar, const symbol_t *input_prefix) {
  REQUIRE(base_grammar->vocab_type == GV_BILINGUAL, "Error: cannot create a wordlist grammar from secondary grammars");
  REQUIRE(base_grammar->list_initial->num_elements == 1, "Error: cannot create a wordlist grammar from non n-gram grammars");
  grammar_t * grammar = (grammar_t *) malloc(sizeof(grammar_t));
  MEMTEST(grammar);

  // copy all base grammar fields
  *grammar = *base_grammar;

  grammar->vector = (state_grammar_t **) malloc(sizeof(state_grammar_t *));
  MEMTEST(grammar->vector);

  grammar->num_states = 0;
  grammar->is_ngram = false;
  grammar->n = 1;

  //Create list of initial states
  grammar->list_initial = (list_states_t *) malloc(sizeof(list_states_t));
  MEMTEST(grammar->list_initial);
  grammar->list_initial->vector = NULL;
  grammar->list_initial->num_elements = 0;
  state_grammar_t *initial_state = state_grammar_create();
  grammar_append(grammar, initial_state);
  list_states_append(grammar->list_initial, initial_state, LOG_ONE);

  //Create list of end states
  grammar->list_end = (list_states_t *) malloc(sizeof(list_states_t));
  MEMTEST(grammar->list_end);
  grammar->list_end->vector = NULL;
  grammar->list_end->num_elements = 0;
  state_grammar_t *end_state = state_grammar_create();
  grammar_append(grammar, end_state);
  list_states_append(grammar->list_end, end_state, LOG_ONE);


  // find n-gram state that matches the prefix
  state_grammar_t *current_state = base_grammar->list_initial->vector[0].state;
  symbol_t const * word = input_prefix;
  while (*word != VOCAB_NONE) {
    words_state_t ws = { STATE_NONE, *word, LOG_ZERO };
    state_grammar_fill_word_state(current_state, &ws);
    current_state = ws.state_next;
    word++;
  }

  //Create vector of visits for language model expansion
  bool *visit = (bool *) malloc(grammar->vocab->extended->last * sizeof(bool));
  MEMTEST(visit);
  memset(visit, false, (grammar->vocab->extended->last) * sizeof(bool));

  float backoff = 0;
  while (current_state != NULL) {
    for (int w = 0; w < current_state->num_words; w++) {
      if (!visit[current_state->words[w].word]) {
        state_grammar_append(initial_state, current_state->words[w].word, current_state->words[w].prob + backoff, end_state);
        visit[current_state->words[w].word] = true;
      }
    }
    if (current_state->state_bo != NULL && isfinite(current_state->bo)) {
      backoff += current_state->bo;
      current_state = current_state->state_bo;
    }
    else {
      current_state = NULL;
    }
  }
  free(visit);

  grammar_sort_by_prob(grammar);
  grammar_build_word_search(grammar);

  //size_t fifo_idx = 0;
  return grammar;
}


search_t *search_create_from_prefix(const search_t *search, symbol_t *in_prefix, symbol_t *out_prefix) {
  search_t *prefix_search = search_create(search->decoder);

  prefix_search->decoder->grammar = grammar_create_from_prefix(search->decoder->grammar, in_prefix, out_prefix);
  if (search->emission_cache) {
    prefix_search->emission_cache = search->emission_cache;
    free(prefix_search->t_probability);
  }
  prefix_search->is_prefix_search = true;
  return prefix_search;
}


search_t *search_create_wordlist_from_prefix(const search_t *search, symbol_t *in_prefix) {
  search_t *prefix_search = search_create(search->decoder);

  prefix_search->decoder->grammar = grammar_create_wordlist_from_prefix(search->decoder->grammar, in_prefix);
  if (search->emission_cache) {
    prefix_search->emission_cache = search->emission_cache;
    free(prefix_search->t_probability);
  }
  prefix_search->is_prefix_search = true;
  return prefix_search;
}

search_t *search_create_conditioned_to_prefix(const search_t *search, symbol_t *in_prefix) {
  search_t *prefix_search = search_create(search->decoder);

  prefix_search->decoder->grammar = grammar_create_conditioned_to_prefix(search->decoder->grammar, in_prefix);
  if (search->emission_cache) {
    prefix_search->emission_cache = search->emission_cache;
    free(prefix_search->t_probability);
  }
  prefix_search->is_prefix_search = true;
  return prefix_search;
}



int cat_decode(search_t *search, const features_t *features, lattice_t *lattice,
               symbol_t *ref, reference_t ref_type)
{
  int iter = 0;
  bool is_different = true;
  const int ref_len = symlen(ref);

  search_create_emission_cache(search);

  symbol_t *prefix = NULL;


  if (ref_type == REF_SOURCE || ref_type == REF_TARGET) {
    do {
      search_t *prefix_search = NULL;
      if (ref_type == REF_SOURCE) {
        prefix_search = search_create_from_prefix(search, prefix, NULL);
      }
      else if (ref_type == REF_TARGET) {
        prefix_search = search_create_from_prefix(search, NULL, prefix);
      }
      else {
        REQUIRE(ref_type > REF_NONE && ref_type < REF_MAX, "Invalid reference type\n");
      }
      CHECK(prefix_search->decoder->grammar->list_initial->num_elements > 0, "Empty prefix grammar. Possible lack of coverture\n");
      fprintf(stderr, "n initials = %d, n_states = %d\n", prefix_search->decoder->grammar->list_initial->num_elements, prefix_search->decoder->grammar->num_states);
      //grammar_write_dot(prefix_search->decoder->grammar, stderr);
      lattice_t *prefix_lattice = lattice_create(lattice->nbest, lattice->nnode, prefix_search->decoder);

      clock_t tim = clock();
      decode(prefix_search, features, prefix_lattice);
      clock_t tim2 = clock();
      TRACE(1, "iter %d tim %fs\n", iter, ((float) ((tim2 - tim) / CLOCKS_PER_SEC) / prefix_search->n_frames) / 0.01);

      //Calculate best hypothesis
      {
        symbol_t *best_ext_hyp = NULL;
        lattice_best_hyp(prefix_lattice, &best_ext_hyp);

        if (best_ext_hyp != NULL) {
          // free previous prefix and compute new hypothesis
          free(prefix);
          prefix = NULL;

          symbol_t *best_in_hyp = NULL, *best_out_hyp = NULL;
          extended_vocab_separate_languages(prefix_lattice->decoder->vocab, best_ext_hyp, &best_in_hyp, &best_out_hyp);

          if (ref_type == REF_SOURCE) {
            prefix = symdup(best_in_hyp);
          }
          else if (ref_type == REF_TARGET) {
            prefix = symdup(best_out_hyp);
          }
          free(best_out_hyp);
          free(best_in_hyp);

          int prefix_len = sym_longest_prefix_match(ref, prefix);

          if (prefix_len == ref_len) {
            is_different = false;
          }
          else  {
            // add one word to the prefix
            prefix[prefix_len] = ref[prefix_len];
            prefix[prefix_len + 1] = VOCAB_NONE;
          }

          {
            char *prefix_str = NULL;
            vocab_symbols_to_string(prefix, (ref_type == REF_SOURCE)?prefix_lattice->decoder->vocab->in:prefix_lattice->decoder->vocab->out, &prefix_str);
            TRACE(1, "next prefix: %s\n", prefix_str);
            free(prefix_str);
          }
          char *sentence_str = NULL;
          extended_vocab_symbols_to_string(best_ext_hyp, prefix_lattice->decoder->vocab, &sentence_str);
          TRACE(1, "%s\n", sentence_str);
          free(sentence_str);
        } else {
          TRACE(1, "next prefix: NULL\n");
          TRACE(1, "Sentence not recognized. Increasing beam search\n");
          prefix_search->decoder->beam_pruning *= 2;
          iter--;
        }

        free(best_ext_hyp);
      }


      lattice_delete(prefix_lattice);
      search_delete(prefix_search);

      iter++;

      fflush(stdout);

    } while (is_different);
  }

  {
    search_t *prefix_search = NULL;
    if (ref_type == REF_SOURCE) {
      prefix_search = search_create_from_prefix(search, prefix, NULL);
      TRACE(1, "forced decoding...\n");
    }
    else if (ref_type == REF_TARGET) {
      prefix_search = search_create_from_prefix(search, NULL, prefix);
      TRACE(1, "forced decoding...\n");
    }
    else if (ref_type == REF_PREFIX) {
      prefix_search = search_create_conditioned_to_prefix(search, ref);
      TRACE(1, "suffix decoding...\n");
    }
    else {
      REQUIRE(ref_type > REF_NONE && ref_type < REF_MAX, "Invalid reference type\n");
    }
    CHECK(prefix_search->decoder->grammar->list_initial->num_elements > 0, "Empty prefix grammar. Possible lack of coverture\n");
    //grammar_write_dot(prefix_search->decoder->grammar, stderr);
    //lattice_t *prefix_lattice = lattice_create(lattice->nnode, lattice->nbest, prefix_search->decoder);

    clock_t tim = clock();
    decode(prefix_search, features, lattice);
    clock_t tim2 = clock();
    TRACE(1, "iter %d tim %f\n", iter, ((float) ((tim2 - tim) / CLOCKS_PER_SEC) / prefix_search->n_frames) / 0.01);
    search_delete(prefix_search);
  }

  free(prefix);

  return iter;
}

int cat_decode_word(search_t *search, const features_t *features, lattice_t *lattice,
               symbol_t *ref, reference_t ref_type, const char *lattice_tmpl)
{
  TRACE(1, "Decoding CAT word...\n");
  int prefix_len = 0;
  const int ref_len = symlen(ref);

  float beam = search->decoder->beam_pruning;

  search_create_emission_cache(search);

  symbol_t *prefix = (symbol_t *)malloc((ref_len+1)*sizeof(symbol_t));
  if (search->decoder->grammar->start != VOCAB_NONE) {
    prefix[prefix_len++] = search->decoder->grammar->start;
  }
  prefix[prefix_len] = VOCAB_NONE;

  if (ref_type == REF_SOURCE || ref_type == REF_TARGET) {
    for (; prefix_len < ref_len; prefix_len++) {
      {
        char *prefix_str = NULL;
        vocab_symbols_to_string(prefix, (ref_type == REF_SOURCE)?search->decoder->vocab->in:search->decoder->vocab->out, &prefix_str);
        TRACE(1, "next prefix: %s\n", prefix_str);
        free(prefix_str);
      }

      search_t *prefix_search = NULL;
      if (ref_type == REF_SOURCE) {
        prefix_search = search_create_from_prefix(search, prefix, NULL);
      }
      else if (ref_type == REF_TARGET) {
        prefix_search = search_create_from_prefix(search, NULL, prefix);
      }
      else {
        REQUIRE(ref_type > REF_NONE && ref_type < REF_MAX, "Invalid reference type\n");
      }
      search->decoder->beam_pruning = beam;

      CHECK(prefix_search->decoder->grammar->list_initial->num_elements > 0, "Empty prefix grammar. Possible lack of coverture\n");
      fprintf(stderr, "n initials = %d, n_states = %d\n", prefix_search->decoder->grammar->list_initial->num_elements, prefix_search->decoder->grammar->num_states);
      grammar_write_dot(prefix_search->decoder->grammar, stderr);
      lattice_t *prefix_lattice = lattice_create(lattice->nbest, lattice->nnode, prefix_search->decoder);
      //lattice_t *prefix_lattice = lattice_create(lattice->nbest, lattice->nnode, search->decoder);

      clock_t tim = clock();
      decode(prefix_search, features, prefix_lattice);
      //decode(search, features, prefix_lattice);
      clock_t tim2 = clock();
      TRACE(1, "iter %d tim %f\n", prefix_len, ((float) ((tim2 - tim) / CLOCKS_PER_SEC) / prefix_search->n_frames) / 0.01);

      //Calculate best hypothesis
      {
        symbol_t *best_ext_hyp = NULL;
        lattice_best_hyp(prefix_lattice, &best_ext_hyp);

        if (best_ext_hyp != NULL) {
          // write lattice
          {
            char path[MAX_LINE];      
            sprintf(path, lattice_tmpl, prefix_len);
            FILE *lattice_file = smart_fopen(path, "w");
            CHECK_SYS_ERROR(lattice_file != NULL, "Couldn't create word graph file '%s'\n", path);
            lattice_write(prefix_lattice, lattice_file, path);
            smart_fclose(lattice_file);
          }

          // add one word to the prefix
          prefix[prefix_len] = ref[prefix_len];
          prefix[prefix_len + 1] = VOCAB_NONE;

          {
            char *sentence_str = NULL;
            extended_vocab_symbols_to_string(best_ext_hyp, prefix_lattice->decoder->vocab, &sentence_str);
            TRACE(1, "%s\n", sentence_str);
            free(sentence_str);
          }
          free(best_ext_hyp);
        } else {
          TRACE(1, "Sentence not recognized. Increasing beam search\n");
          // add one word to the prefix
          //prefix[prefix_len] = ref[prefix_len];
          //prefix[prefix_len + 1] = VOCAB_NONE;

          //abort();
          prefix_len--;
          beam *= 2;
        }

      }


      lattice_delete(prefix_lattice);
      search_delete(prefix_search);

      fflush(stdout);

    }
  }

  free(prefix);

  return prefix_len;
}
