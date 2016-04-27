/** @defgroup viterbi Viterbi Fuctions
 *  The Viterbi Functions
 *  @{
 */

/** @page authorsVit Authors
 *  Miriam Lujan - mlujan@iti.upv.es
 *  February 2008
 */

#include <iatros/config.h>
#include <prhlt/constants.h>
#include <prhlt/utils.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <float.h>
#include <stdlib.h>
#include <libgen.h>
#include <viterbi/hypothesis.h>
#include <viterbi/lex.h>
#include <viterbi/heap.h>
#include <viterbi/grammar.h>
#include <viterbi/lattice.h>
#include <viterbi/viterbi.h>
#include <prhlt/trace.h>


///Calculate probability of emission with a ceptral vector and a state
/**
@param vector_cc Vector of cepstrals
@param phoneme Number of phoneme
@param state_hmm State of hmm
@param t_probability Vector of probabilities
@param hmm Structure with hmm
@return Probability of emission
*/
INLINE float prob_emission(search_t *search, const float *vector_cc, const hyp_t * hyp) {
  const hmm_t* hmm = search->decoder->hmm;
  //If probability is not calculate
  int state = hmm->phonemes[hyp->phoneme]->states[hyp->state_hmm]->id;
  if (is_logzero(search->t_probability[state])) {
    search->t_probability[state] = log_gaussian_mixture(vector_cc, hmm->states[state]->mixture, hmm->num_features);
  }
  return search->t_probability[state];
}

/** Reset statistics, search caches and other search variables necessary for the next frame
 */
void start_frame(search_t *search, const float *feat_vec, lattice_t *lattice) {
  search->n_frames++;

  // clear the already expanded heap and swap heaps
  hh_clear(search->prev_heap);
  SWAP(search->heap, search->prev_heap, hyp_heap_t *);

  if (ENABLE_STATISTICS) {
    search->stats = (stats_t **) realloc(search->stats, search->n_frames * sizeof(stats_t *));
    MEMTEST(search->stats);
    search->stats[search->n_frames-1] = (stats_t *)malloc(sizeof(stats_t));
    MEMTEST(search->stats[search->n_frames-1]);
    reset_frame_stats(search->stats[search->n_frames-1]);


    if (ENABLE_STATISTICS >= SV_SHOW_FRAME) {
      const hmm_t* hmm = search->decoder->hmm;
      stats_t *stats = search->stats[search->n_frames - 1];
      stats->n_hmm_counts  = search->decoder->hmm->n_hmm_states;
      stats->hmm_counts    = (int *) calloc(search->decoder->hmm->n_hmm_states, sizeof(int));
      stats->n_lex_counts  = search->decoder->lex->n_states;
      stats->lex_counts    = (int *) calloc(search->decoder->lex->n_states, sizeof(int));
      stats->n_word_counts = search->decoder->lex->num_models;
      stats->word_counts   = (int*) calloc(search->decoder->lex->num_models, sizeof(int));

      stats->num_hist = search->decoder->grammar->num_states;
      if (!search->is_prefix_search) stats->hist_count = (int*) calloc(search->decoder->grammar->num_states, sizeof(int));

      stats->num_states = hmm->num_states;

      stats->n_hyps = hh_size(search->prev_heap);
      stats->n_word_hyps = 0;
    }
  }

  // prepare lattice for this frame
  lattice_start_frame(lattice);

  if (search->emission_cache != NULL) {
    if ((int) search->emission_cache->n_elems < search->n_frames) {
      float *t_probability = (float *) malloc(search->decoder->hmm->num_states * sizeof(float));
      vector_append(search->emission_cache, t_probability);
      search->t_probability = t_probability;
      search_clear_acoustic_probability_cache(search);
    }
  }
  search->do_acoustic_early_pruning = false;

  if (search->feature_type == FT_EMISSION_PROBABILITIES) {
    memcpy(search->t_probability, feat_vec, search->decoder->hmm->num_states * sizeof(float));
  }
  else if (search->emission_cache == NULL) {
    // initialize emission probabilities
    search_clear_acoustic_probability_cache(search);
  }
  else {
    search->t_probability = (float *)search->emission_cache->data[search->n_frames - 1];
  }
}

/** Finish gathering statistics for a frame
 */
void end_frame(search_t *search) {
  if (ENABLE_STATISTICS >= SV_SHOW_FRAME) {
    stats_t *stats = search->stats[search->n_frames - 1];
    stats->heap_size = hh_size(search->heap);
    stats->heap_capacity = hh_capacity(search->heap);
    stats->max_final = hh_max(search->heap);
    stats->min_final = hh_min(search->heap);
    stats->limit_final = hh_limit(search->heap);
    stats->beam = hh_beam(search->heap);

    for (int i = 0; i < stats->n_hmm_counts; i++) {
      if (stats->hmm_counts[i] > 0) {
        stats->total_events[HMM_BEAM] += stats->hmm_counts[i];
        stats->distinct_events[HMM_BEAM]++;
      }
    }

    for (int i = 0; i < stats->n_lex_counts; i++) {
      if (stats->lex_counts[i] > 0) {
        stats->total_events[LEX_BEAM] += stats->lex_counts[i];
        stats->distinct_events[LEX_BEAM]++;
      }
    }

    for (int i = 0; i < stats->n_word_counts; i++) {
      if (stats->word_counts[i] > 0) {
        stats->total_events[GRAMMAR_BEAM] += stats->word_counts[i];
        stats->distinct_events[GRAMMAR_BEAM]++;
      }
    }

    fprintf(stderr, "frame %8d\n", search->n_frames - 1);
    print_frame_stats(stderr, stats);

    free(stats->hmm_counts);
    free(stats->lex_counts);
    free(stats->word_counts);
    if (!search->is_prefix_search) free(stats->hist_count);
  }
}

/** expands a hmm transition based on the previous hypothesis
 * @param search the search
 * @param feat_vec a feature vector
 * @param prev_hyp the previous hypothesis
 * @param transition the hmm transition index
 */
void expand_hmm_transition(search_t *search, const float *feat_vec, const hyp_t *prev_hyp, int transition) {
  const hmm_t* hmm = search->decoder->hmm;

  if (ENABLE_STATISTICS >= SV_SHOW_FRAME && search->stats[search->n_frames-1]->hmm_counts != NULL) {
    hmm_id_t hmm_id = hmm->phonemes[prev_hyp->phoneme]->hmm_ids[prev_hyp->state_hmm];
    search->stats[search->n_frames-1]->hmm_counts[hmm_id]++;
  }

  hyp_t hyp = *prev_hyp;
  hyp.state_hmm = transition - 1;
  hyp.probability.acoustic = prob_emission(search, feat_vec, &hyp)
      + hmm->phonemes[hyp.phoneme]->matrix->matrix_transitions[prev_hyp->state_hmm + 1][transition];
  hyp.probability.final += hyp.probability.acoustic;
  hyp.probability.acoustic += prev_hyp->probability.acoustic;
  beam_status_t in = hh_insert(search->heap, &hyp);

  if (ENABLE_STATISTICS) {
    search->stats[search->n_frames-1]->beam_stats[HMM_BEAM].status[in]++;
  }
}

/** expands a lex transition based on the previous hypothesis
 * @param search the search
 * @param feat_vec a feature vector
 * @param prev_hyp the previous hypothesis
 */
void expand_lex_transition(search_t *search, const float *feat_vec, const hyp_t *prev_hyp) {
  const decoder_t* decoder = search->decoder;
  const lex_t* lex = decoder->lex;
  const hmm_t* hmm = decoder->hmm;

  if (ENABLE_STATISTICS >= SV_SHOW_FRAME  && search->stats[search->n_frames-1]->lex_counts != NULL) {
    lex_state_id_t lex_id = lex->models[prev_hyp->word]->state_ids[prev_hyp->state_lexic];
    search->stats[search->n_frames-1]->lex_counts[lex_id]++;
  }

  hyp_t hyp = *prev_hyp;
  int word = prev_hyp->word;

  //for every alternative pronounciation
  const state_lex_t *state_lex = lex->models[word]->states[prev_hyp->state_lexic];
  for (int o = 0; o < state_lex->num_edges; o++) {

    hyp.phoneme     = state_lex->edges[o]->phoneme;
    hyp.state_lexic = state_lex->edges[o]->destination;
    float lex_prob  = state_lex->edges[o]->probability;

    //In transition
    const matrix_transitions_t *matrix = hmm->phonemes[hyp.phoneme]->matrix;
    for (int p = 1; p < matrix->num_transitions - 1; p++) {
      //Probability of initial state
      float hmm_prob = matrix->matrix_transitions[0][p];
      if (!is_logzero(hmm_prob)) {
        hyp.state_hmm = p - 1;

        //Calculate probability
        hyp.probability.acoustic = lex_prob + hmm_prob + prob_emission(search, feat_vec, &hyp);

        hyp.probability.final = prev_hyp->probability.final + hyp.probability.acoustic;
        hyp.probability.acoustic += prev_hyp->probability.acoustic;

        //BEAM
        beam_status_t in = hh_insert(search->heap, &hyp);

        if (ENABLE_STATISTICS) {
          search->stats[search->n_frames-1]->beam_stats[LEX_BEAM].status[in]++;
        }

      }//If state initial
    }//State initial

  }
}

/** expands a word transition based on a partially completed hypothesis
 * @param search the search
 * @param feat_vec a feature vector
 * @param hyp the the partially completed hypothesis
 * for expand_word_transtion to function properly, some hypothesis fields
 * must be completed beforehand. These fields are:
 *  - word:  the word to expand
 *  - index: the index of the lattice that generated this hypothesis
 *  - state: the target state in in the grammar
 *  - history: the source state in in the grammar
 *  - probability.lm: the language model probability
 *  - probability.final: the final probability including all except the acoustic probability
 *  for examples, please, see the initial_stage
 */
void expand_word_transition(search_t *search, const float *feat_vec, hyp_t *hyp) {
  const decoder_t* decoder = search->decoder;
  const lex_t* lex = decoder->lex;
  const hmm_t* hmm = decoder->hmm;


//   //If the word is a category
//   if(hyp->category!=CATEGORY_NONE){
//    printf("%d %d\n", hyp->word, category);
//   }

  if (decoder->vocab->in->info[hyp->word].category != CATEGORY_NONE) {
    fprintf(stderr, "WARNING\n");
  }

  if (ENABLE_STATISTICS >= SV_SHOW_FRAME && search->stats[search->n_frames-1]->word_counts != NULL) {
    search->stats[search->n_frames-1]->word_counts[hyp->word]++;
  }

  // store the current final probability since it is going to be restored
  // after adding a new hypothesis
  const float final_minus_acoustic = hyp->probability.final;

  //for every alternative pronounciation
  const state_lex_t *state_lex = lex->models[hyp->word]->states[lex->models[hyp->word]->initial];

  for (int o = 0; o < state_lex->num_edges; o++) {

    hyp->phoneme     = state_lex->edges[o]->phoneme;
    hyp->state_lexic = state_lex->edges[o]->destination;
    float lex_prob   = state_lex->edges[o]->probability;

    // for every transition from the initial state of the phoneme hmm model
    const matrix_transitions_t *matrix = hmm->phonemes[hyp->phoneme]->matrix;
    for (int p = 1; p < matrix->num_transitions - 1; p++) {
      float hmm_prob = matrix->matrix_transitions[0][p];
      // we can reach the state from the initial hmm state
      if (!is_logzero(hmm_prob)) {
        hyp->state_hmm = p - 1;

        // compute acoustic probability
        hyp->probability.acoustic = lex_prob + hmm_prob + prob_emission(search, feat_vec, hyp);
        // add it up to the final probability
        hyp->probability.final = final_minus_acoustic + hyp->probability.acoustic;

        if (ENABLE_STATISTICS && !isfinite(search->stats[search->n_frames-1]->first_expanded.final)) {
          search->stats[search->n_frames-1]->first_expanded = hyp->probability;
        }

        // insert the hypothesis into the heap
        beam_status_t in = hh_insert(search->heap, hyp);

        if (ENABLE_STATISTICS) {
          search->stats[search->n_frames-1]->beam_stats[GRAMMAR_BEAM].status[in]++;
          search->stats[search->n_frames-1]->total_expanded++;
        }

        if (in >= INSERT) {
          if (ENABLE_STATISTICS) {
            search->stats[search->n_frames-1]->total_accepted++;
            search->stats[search->n_frames-1]->last_accepted = search->stats[search->n_frames-1]->total_expanded;
            search->stats[search->n_frames-1]->last_expanded = hyp->probability;
          }
        }

      }// if we can reach state
    }//for hmm transitions
  }// for every alternative pronounciation
}


/** expands a lex transition based on the previous hypothesis
 * @param search the search
 * @param feat_vec a feature vector
 * @param prev_hyp the previous hypothesis
 */
void expand_phrase_transition(search_t *search, const float *feat_vec, const hyp_t *prev_hyp) {
  const decoder_t* decoder = search->decoder;
  const grammar_t* grammar = decoder->grammar;

  // expand next word in the phrase
  if (*(prev_hyp->word_ptr + 1) != VOCAB_NONE) {
    // copy all fields
    hyp_t hyp = *prev_hyp;
    hyp.word_ptr++;
    hyp.word = *hyp.word_ptr;

    #if defined(DISTRIBUTE_UNIFORMLY_PHRASE_PROBABILITY)
      //since we have distributed uniformly the probability among
      //the words in the phrase, now, we must add again the lm, out_lm and wip_out
      if (decoder->output_grammar != NULL) {
        hyp.probability.final += hyp.probability.out_lm * decoder->gsf_out;
        hyp.probability.final -= hyp.probability.wip_out;
      }
    #else
      // we have already added these score when then phrase was first expanded
      // so we set them to 0
      hyp.probability.lm = 0;
      hyp.probability.out_lm = 0;
      hyp.probability.wip_out = 0;
    #endif

    if (decoder->input_grammar != NULL) {
      words_state_t ws = { STATE_NONE, hyp.word, LOG_ZERO };
      state_grammar_t * state_in = hyp.state_in;
      state_grammar_fill_word_state(state_in, &ws);
      hyp.probability.in_lm = ws.prob;
      hyp.state_in = ws.state_next;
      hyp.probability.final += hyp.probability.in_lm * decoder->gsf_in;
    }
    else hyp.probability.in_lm = 0;

    if (hyp.extended != grammar->end) {
      hyp.probability.final -= decoder->wip;
    }
    expand_word_transition(search, feat_vec, &hyp);
  }

  // expand silence if possible
  if (grammar->silence != VOCAB_NONE) {
    hyp_t hyp = *prev_hyp;
    hyp.word = search->decoder->grammar->silence;
    hyp.probability.lm = decoder->grammar->silence_score;
    hyp.probability.out_lm = 0;
    hyp.probability.in_lm = 0;
    hyp.probability.final += (hyp.probability.lm * decoder->gsf) - decoder->wip;
    expand_word_transition(search, feat_vec, &hyp);
  }

  // expand pause if possible
  if (grammar->pause != VOCAB_NONE) {
    hyp_t hyp = *prev_hyp;
    hyp.word = search->decoder->grammar->pause;
    hyp.probability.lm = decoder->grammar->silence_score;
    hyp.probability.out_lm = 0;
    hyp.probability.in_lm = 0;
    hyp.probability.final += (hyp.probability.lm * decoder->gsf) - decoder->wip;
    expand_word_transition(search, feat_vec, &hyp);
  }

}


/** expands word transtions from the grammar states
 * @param search search status
 * @param feat_vec feature vector in frame t
 * @param t current frame index
 * @param lattice lattice from which to expand
 */
void expand_words_from_lat_state(search_t *search, const float *feat_vec, const lat_state_t *lat_state, float initial_prob) {
  const decoder_t* decoder = search->decoder;
  const grammar_t* grammar = decoder->grammar;
  const lex_t* lex = decoder->lex;

  state_grammar_t *state_current = lat_state->state;

  // since we are doing viterbi, we are just interested in expanding
  // the most probable hypothesis from this state
  const lat_hyp_t *best_hyp = lat_state->max;

  //Category
  //if (lattice->vector[index_table]->category != CATEGORY_NONE) {
  //  grammar = decoder->categories->categories[lattice->vector[index_table]->category];
  //}

  // we only expand if we haven't reached the end of the sentence
  bool is_final = false;
  float cat_end_prob = LOG_ZERO;
  if (lat_state->category != CATEGORY_NONE) {
    // we are in the middle of a category
    grammar = decoder->categories->categories[lat_state->category];

    int final_idx = grammar_is_final_state(grammar, state_current);
    if (final_idx != -1) {
      cat_end_prob = grammar->list_end->vector[final_idx].prob;
      // we are at the end of a category
      grammar = decoder->grammar;
      state_current = lat_state->history_category;
      is_final = true;
    }
  }


  if (ENABLE_STATISTICS >= SV_SHOW_WORD_EXPANSION)
    fprintf(stderr, "limit = %f, max_prob = %f, best ac = %f\n", hh_limit(search->heap), best_hyp->probability.final, search->best_achievable_ac);

  // find the first state with words to expand
  // from current_state to unigram state through backoff
  while (state_current != STATE_NONE && state_current->num_words == 0 && state_current->state_bo != NULL) {
    // if num_words == 0, backoff should be 0. Go backoff
    state_current = state_current->state_bo;
  }

  // check if word expansions are possible from this state
  // if the model has secondary grammars, early grammar quitting
  // will be less effective. The problem is that the loglinear combination
  // of the different grammars results is a non-sorted list of output words
  if (state_current != STATE_NONE && state_current->num_words > 0) {
    if (search->do_acoustic_early_pruning) {
      float best_achievable_lm = state_current->words[0].prob * decoder->gsf - decoder->wip;

      if (decoder->input_grammar != NULL) {
        state_grammar_t * state_in = best_hyp->state_in;
        if (state_in && state_in->num_words > 0) {
          best_achievable_lm += state_in->words[0].prob * decoder->gsf_in;
        }
      }
      if (decoder->output_grammar != NULL) {
        state_grammar_t * state_out = best_hyp->state_out;
        if (state_out && state_out->num_words > 0) {
          best_achievable_lm += state_out->words[0].prob * decoder->gsf_out;
        }
      }


      // if the state is bellow the beam reject this and the following states
      // please, note that hyps in lattice are sorted
      if (best_hyp->probability.final + search->best_achievable_ac + best_achievable_lm <= hh_limit(search->heap)) {
        // if the most probable word in the state can't pass
        // the limit then no expansions possible. Ignore the state
        return;
      }
    }
  } else {
    // there are no words to expand. Ignore the state
    return;
  }

  if (ENABLE_STATISTICS >= SV_SHOW_WORD_EXPANSION) reset_word_expand_stats(search->stats[search->n_frames - 1]);
  float backoff = initial_prob;

  // initialize visited words. when the grammar is a ngram,
  // we use it to know if a given word has already been
  // expanded from the actual state or corresponding backoff states,
  // since it is only allowed one expansion per word
  if (grammar->is_ngram) search_clear_visited_words(search);

  // we iterate from the current state through backoff up to the unigram state
  while (state_current != STATE_NONE) {
    if (ENABLE_STATISTICS >= SV_SHOW_FRAME && !search->is_prefix_search) search->stats[search->n_frames-1]->hist_count[state_current->num_state]++;

    // iterate over words in the state
    for (int m = 0; state_current != STATE_NONE && m < state_current->num_words; m++) {
      const extended_symbol_t *word = extended_vocab_get_extended_symbol(grammar->vocab, state_current->words[m].word);
      int category = search->decoder->vocab->in->info[*word->input].category;

      // we enter a category
/*      if (category >= 0 && lat_state->category == CATEGORY_NONE) {*/
      if (category >= 0 && (lat_state->category == CATEGORY_NONE || is_final)) {
        grammar_t *cat_grammar = decoder->categories->categories[category];
        for (int s = 0; s < cat_grammar->list_initial->num_elements; s++) {
          lat_state_t lat_state_dummy;
          lat_state_dummy = *lat_state;

          lat_state_dummy.category = category;
          lat_state_dummy.history_category = state_current->words[m].state_next;
          lat_state_dummy.state = cat_grammar->list_initial->vector[s].state;
          expand_words_from_lat_state(search, feat_vec, &lat_state_dummy, state_current->words[m].prob+cat_grammar->list_initial->vector[s].prob);
        }
      }
      // we do a normal expansion
      else {
        bool expansion_allowed = true;

        // if we are in a ngram, update the visit vector and
        // check if expansion is allowed
        if (grammar->is_ngram) {
          if (search->visit[word->extended] == 0) {
            if (!is_logzero(state_current->bo)) {
              search->visit[word->extended] = 1;
            }
          } else expansion_allowed = false;
        }

        //XXX: now, start and end words do not eat up silences so do not allow expansion
        if (word->extended == grammar->start || word->extended == grammar->end) expansion_allowed = false;

        if (expansion_allowed) {
          hyp_t hyp;

          hyp.extended = word->extended;
          hyp.word_ptr = word->input;
          hyp.word = *hyp.word_ptr;

          if (is_final) {
            hyp.category = CATEGORY_NONE;
            hyp.history_category = STATE_NONE;
          }
          else {
            hyp.category = lat_state->category;
            hyp.history_category = lat_state->history_category;
          }

          hyp.index = lat_state->index;
          hyp.state = state_current->words[m].state_next;
          hyp.history = state_current;

          hyp.probability.lm = state_current->words[m].prob + backoff;
          if (is_final) hyp.probability.lm += cat_end_prob;

          hyp.probability.final = best_hyp->probability.final + (hyp.probability.lm * decoder->gsf);
          hyp.probability.final += word->combined_score;

          if (decoder->input_grammar != NULL) {
            words_state_t ws = { STATE_NONE, hyp.word, LOG_ZERO};
            state_grammar_fill_word_state(best_hyp->state_in, &ws);
            hyp.probability.in_lm = ws.prob;
            hyp.state_in = ws.state_next;
            hyp.probability.final += hyp.probability.in_lm * decoder->gsf_in;
          }
          else hyp.probability.in_lm = 0;

          hyp.probability.out_lm = 0;
          hyp.probability.wip_out = 0;
          if (word->output != NULL) {
            //XXX: we add the whole lm_out probability here
            // this may not be fair and it may harm beam search
            symbol_t const * out_word = word->output;
            hyp.state_out = best_hyp->state_out;
            while (*out_word != VOCAB_NONE) {
              // add lm out probability
              if (decoder->output_grammar != NULL) {
                words_state_t ws = { STATE_NONE, *out_word, LOG_ZERO};
                state_grammar_fill_word_state(hyp.state_out, &ws);
                hyp.probability.out_lm += ws.prob;
                hyp.state_out = ws.state_next;
              }
              // add wip out probability
              if (decoder->output_grammar == NULL || *out_word != decoder->output_grammar->end) {
                hyp.probability.wip_out -= decoder->wip_out;
              }
              out_word++;
            }
            // we distribute uniformly among the input words
            #if defined(DISTRIBUTE_UNIFORMLY_PHRASE_PROBABILITY)
              float input_length = symlen(word->input);
              hyp.probability.out_lm /= input_length;
              hyp.probability.wip_out /= input_length;
            #endif
            hyp.probability.final += hyp.probability.out_lm * decoder->gsf_out;
            hyp.probability.final -= hyp.probability.wip_out;
          }

          if (hyp.extended != grammar->end) {
            hyp.probability.final -= decoder->wip;
          }

          // do early pruning assuming the best_achievable_ac is the best
          // score we can get in the expansion
          if (!search->do_acoustic_early_pruning || hyp.probability.final + search->best_achievable_ac > hh_limit(search->heap)) {
            expand_word_transition(search, feat_vec, &hyp);
          }

          // words in states are sorted by probability so if previous
          // word didn't pass the current limit, following words won't either
          // then exit expanding from this state
          else {
            if (ENABLE_STATISTICS >= SV_SHOW_WORD_EXPANSION)
              fprintf(stderr, "exiting language at %d(%6.2f%%) out of %d\n", search->stats[search->n_frames - 1]->total_expanded, 100.0
                  * search->stats[search->n_frames - 1]->total_expanded / (float) lex->num_models, lex->num_models);
            state_current = STATE_NONE;
            break;
          }
        } // expansion is allowed
      }
    }// iterate over words in the state

    if (state_current != STATE_NONE) {
      backoff += state_current->bo;
      state_current = state_current->state_bo;
    }

  } //while (state_current != STATE_NONE)

  if (ENABLE_STATISTICS >= SV_SHOW_WORD_EXPANSION) {
    if (grammar->is_ngram) {
      // count number of visited words
      search->stats[search->n_frames-1]->num_visited_words = 0;
      search->stats[search->n_frames-1]->total_words = search->decoder->lex->vocab->last;
      for (int i = 0; i < search->decoder->vocab->extended->last; i++ ) {
        if (search->visit[i] > 0) search->stats[search->n_frames-1]->num_visited_words++;
      }
    }
    print_word_expand_stats(stderr, search->stats[search->n_frames - 1]);
  }

  // expand silence if possible
  if (grammar->silence != VOCAB_NONE) {
    const extended_symbol_t *word = extended_vocab_get_extended_symbol(grammar->vocab, grammar->silence);
    hyp_t hyp;
    hyp.extended = word->extended;
    hyp.word_ptr = word->input;
    hyp.word = *hyp.word_ptr;
    hyp.index = lat_state->index;
    hyp.state = lat_state->state;
    //XXX: we should clean this up
    hyp.history = (best_hyp->index == -1)?NULL:lat_state->state;
    hyp.category = lat_state->category;
    hyp.history_category = lat_state->history_category;
    hyp.probability.lm = decoder->grammar->silence_score;
    hyp.probability.out_lm = 0;
    hyp.probability.in_lm = 0;
    hyp.probability.final = best_hyp->probability.final + (hyp.probability.lm * decoder->gsf) - decoder->wip;
    if (!search->do_acoustic_early_pruning || hyp.probability.final + search->best_achievable_ac > hh_limit(search->heap)) {
      expand_word_transition(search, feat_vec, &hyp);
    }
  }

  // expand short pause if possible
  if (grammar->pause != VOCAB_NONE) {
    const extended_symbol_t *word = extended_vocab_get_extended_symbol(grammar->vocab, grammar->pause);
    hyp_t hyp;
    hyp.extended = word->extended;
    hyp.word_ptr = word->input;
    hyp.word = *hyp.word_ptr;
    hyp.index = lat_state->index;
    hyp.state = lat_state->state;
    //XXX: we should clean this up
    hyp.history = (best_hyp->index == -1)?NULL:lat_state->state;
    hyp.category = lat_state->category;
    hyp.history_category = lat_state->history_category;
    hyp.probability.lm = decoder->grammar->silence_score;
    hyp.probability.out_lm = 0;
    hyp.probability.in_lm = 0;
    hyp.probability.final = best_hyp->probability.final + (hyp.probability.lm * decoder->gsf) - decoder->wip;
    if (!search->do_acoustic_early_pruning || hyp.probability.final + search->best_achievable_ac > hh_limit(search->heap)) {
      expand_word_transition(search, feat_vec, &hyp);
    }
  }
}

/** expands word transtions from the states inserted in the lattice in frame t starting in a given lattice index
 * @param search search status
 * @param feat_vec feature vector in frame t
 * @param t current frame index
 * @param lattice lattice from which to expand
 */
void expand_words_from_lattice(search_t *search, const float *feat_vec, lattice_t *lattice) {
  const decoder_t* decoder = search->decoder;
  const grammar_t* grammar = decoder->grammar;

  search_compute_best_achievable_ac(search);

  if (ENABLE_STATISTICS >= SV_SHOW_WORD_EXPANSION) {
    search->stats[search->n_frames-1]->num_seen_states = 0;
  }

  // expand words from states inserted in frame t. each index_table is a different grammar state
  for (int index_table = lattice->initial_index; index_table < lattice->num_elements; index_table++) {
    // since we are doing viterbi, we are just interested in expanding
    // the most probable hypothesis from this state
    const lat_hyp_t *best_hyp = lattice->vector[index_table]->max;

    // the state is not is not in the middle of a phrase so we expand
    // the words from the grammar state
    if (*(best_hyp->word_ptr + 1) == VOCAB_NONE) {
      if (grammar->end == UNK_WORD || best_hyp->extended != grammar->end) {
        expand_words_from_lat_state(search, feat_vec, lattice->vector[index_table], 0.0);
      } // word is not end word
    }

    // transition between words in a phrase
    // we expand the following word in the phrase
    else {
      hyp_t hyp;
      hyp.extended = best_hyp->extended;
      hyp.word_ptr = best_hyp->word_ptr;
      hyp.word = best_hyp->word;
      hyp.index = index_table;
      hyp.state = lattice->vector[index_table]->state;
      hyp.state_in = best_hyp->state_in;
      hyp.state_out = best_hyp->state_out;
      hyp.history = (best_hyp->index == -1)?NULL:lattice->vector[best_hyp->index]->state;
      hyp.category = lattice->vector[index_table]->category;
      hyp.history_category = lattice->vector[index_table]->history_category;
      hyp.probability = best_hyp->probability;

      expand_phrase_transition(search, feat_vec, &hyp);
    }
  } // end iterate over lattice states
}


/// Initializes the search with the first feature vector
/**
@param search the search status info
@param feat_vec the first feature vector
@param lattice output lattice
*/
void initial_stage(search_t *search, const float *feat_vec, lattice_t *lattice) {
  const decoder_t* decoder = search->decoder;
  const grammar_t* grammar = decoder->grammar;

  // as secondary grammars are restricted to deterministic n-grams
  // there only will be a initial state either the unigram state or
  // the <s> state
  state_grammar_t * initial_state_in = STATE_NONE;
  if (decoder->input_grammar != NULL) {
    REQUIRE(decoder->input_grammar->list_initial->num_elements == 1, "ERROR: input grammar should have a unique initial state");
    initial_state_in = decoder->input_grammar->list_initial->vector[0].state;
  }
  state_grammar_t * initial_state_out = STATE_NONE;
  if (decoder->output_grammar != NULL) {
    REQUIRE(decoder->output_grammar->list_initial->num_elements == 1, "ERROR: output grammar should have a unique initial state");
    initial_state_out = decoder->output_grammar->list_initial->vector[0].state;
  }

  start_frame(search, feat_vec, lattice);

  // for each initial state of the grammar we expand words
  for (int l = 0; l < grammar->list_initial->num_elements; l++) {
    // create a fake lattice state from which we are going to expand
    lat_hyp_t best_hyp;
    best_hyp.word = VOCAB_NONE;
    best_hyp.word_ptr = NULL;
    best_hyp.extended = VOCAB_NONE;
    best_hyp.state_in = initial_state_in;
    best_hyp.state_out = initial_state_out;
    best_hyp.probability = one_probability;
    best_hyp.index = -1;

    lat_state_t lat_state;
    lat_state.words = NULL;
    lat_state.max = &best_hyp;
    lat_state.num_words = 0;
    lat_state.state = grammar->list_initial->vector[l].state;
    lat_state.category = CATEGORY_NONE;
    lat_state.history_category = NULL;
    lat_state.word_ptr = NULL;
    lat_state.t = -1;
    lat_state.nbest = 0;
    lat_state.index = -1;

    // do normal word expansion from lattice
    expand_words_from_lat_state(search, feat_vec, &lat_state, grammar->list_initial->vector[l].prob);
  }

  end_frame(search);
}


/// adds to the hypothesis language probability the probability to go to a grammar final state
/// for uncompleted hypothesis when partial decoding is enabled
/**
 * @param search the search status
 * @param hyp a hypothesis that has been considered as not completed
 */
void add_probability_to_final_grammar_state(const search_t *search, hyp_t *hyp) {
  const decoder_t* decoder = search->decoder;
  const grammar_t* grammar = decoder->grammar;

  float bo = 0.0;
  //Current state to expansion
  state_grammar_t * state_current = hyp->state;

  // we iterate from the current state through backoff up to the unigram state
  while (state_current != STATE_NONE) {
    if (!is_logzero(bo)) {
      for (int l = 0; l < state_current->num_words; l++) {
        // we have found the probability to the end word in the grammar
        if (state_current->words[l].word == grammar->end) {
          float lm = state_current->words[l].prob + bo + grammar->list_end->vector[0].prob;
          hyp->probability.lm += lm;

          //XXX: should we add an extra (- decoder->wip)? we don't add
          // a new arc to the lattice so other lattice tools would
          // obtain a different score. However, an extra wip should be added
          // to be fairer w.r.t. complete hypotheses
          hyp->probability.final += ((lm) * decoder->gsf); // - decoder->wip;

          return;
        }
      }
    }

    if (state_current == STATE_NONE || state_current->state_bo == NULL || is_logzero(state_current->bo)) {
      state_current = STATE_NONE;
    } else {
      bo = bo + state_current->bo;
      state_current = state_current->state_bo;
    }
  }
}

/// Finalize the search, we need to add the last language model transition to the lattice
/**
@param search the search status info
@param t the last frame
@param lattice output lattice
*/
void end_stage(search_t *search, lattice_t *lattice) {
  const decoder_t* decoder = search->decoder;
  const grammar_t* grammar = decoder->grammar;
  const lex_t* lex = decoder->lex;
  const hmm_t* hmm = decoder->hmm;
  int final = 0;
  vector_t *states = vector_create();

  lattice_reset_frame(lattice);

//  size_t n_finalizable_hmms = 0;
//  size_t n_final_lexics = 0;
  while (!hh_is_empty(search->heap)) {
    hyp_t *hyp = hh_pop(search->heap);

    bool is_finalizable_hmm_state = !is_logzero(hmm->phonemes[hyp->phoneme]->matrix->matrix_transitions[hyp->state_hmm+1][hmm->phonemes[hyp->phoneme]->num_states-1]);
    bool is_final_lexic_state = lex->models[hyp->word]->end == hyp->state_lexic;

//    if (is_finalizable_hmm_state) n_finalizable_hmms++;
//    if (is_final_lexic_state)     n_final_lexics++;

    // from hyp we can find a transition to the end of the word
    // (i.e. we can go to the final hmm state and we are in the final lexicon state)
    if (is_finalizable_hmm_state && is_final_lexic_state)  {

      // add the probability of the last hmm transition
      float last_hmm_prob = hmm->phonemes[hyp->phoneme]->matrix->matrix_transitions[hyp->state_hmm + 1][hmm->phonemes[hyp->phoneme]->num_states - 1];
      hyp->probability.final += last_hmm_prob;
      hyp->probability.acoustic += last_hmm_prob;

      //Category
      if (hyp->category != CATEGORY_NONE) {
        grammar = decoder->categories->categories[hyp->category];
      } else {
        grammar = decoder->grammar;
      }

      // we only expand if we haven't reached the end of the sentence
      bool is_final;
      int index = 0;
      if (grammar->is_ngram) {
        is_final = grammar_is_end_word(grammar, hyp->extended);
        // if final silence is forced, discount language model probability
        if (grammar->silence != VOCAB_NONE && grammar->force_silence) {
          hyp->probability.final -= hyp->probability.lm * decoder->gsf;
          hyp->probability.lm = 0;
        }
      } else {
        state_grammar_t *state = hyp->state;
        if (hyp->category != CATEGORY_NONE) state = hyp->history_category;
        index = grammar_is_final_state(grammar, state);
        is_final = (index != -1);
      }

      if (hyp->category != CATEGORY_NONE) {
        // we are in the middle of a category
        grammar_t *cat_grammar = decoder->categories->categories[hyp->category];

        int final_idx = grammar_is_final_state(grammar, hyp->state);
        if (final_idx != -1) {
          // we are at the end of a category
          hyp->probability.lm += cat_grammar->list_end->vector[final_idx].prob;
          hyp->probability.final += cat_grammar->list_end->vector[final_idx].prob * decoder->gsf;
        }
        else {
          is_final = false;
        }
      }

      // if are not in a n-gram
      // add the probability to go to the final grammar state
      if (grammar->is_ngram && grammar->end != VOCAB_NONE) {
        add_probability_to_final_grammar_state(search, hyp);
      }

      // if we are in a final fsm state, add the probability to finalize
      if (!grammar->is_ngram && is_final) {
        hyp->probability.lm += grammar->list_end->vector[index].prob;
        hyp->probability.final += grammar->list_end->vector[index].prob * decoder->gsf;
      }

      // add the final probability for the input and output grammars
      if (decoder->input_grammar != NULL && decoder->input_grammar->end != VOCAB_NONE
          && hyp->extended != decoder->input_grammar->end)
      {
        words_state_t ws = { STATE_NONE, decoder->input_grammar->end, LOG_ZERO };
        state_grammar_fill_word_state(hyp->state_in, &ws);
        hyp->probability.in_lm += ws.prob;
        hyp->state_in = ws.state_next;
        hyp->probability.final += ws.prob * decoder->gsf_in;
      }

      //XXX: is the hyp->extended != decoder->output_grammar->end part correct?
      if (decoder->output_grammar != NULL && decoder->output_grammar->end != VOCAB_NONE
          && hyp->extended != decoder->output_grammar->end)
      {
        words_state_t ws = { STATE_NONE, decoder->output_grammar->end, LOG_ZERO };
        state_grammar_fill_word_state(hyp->state_out, &ws);
        hyp->probability.out_lm += ws.prob;
        hyp->state_out = ws.state_next;
        hyp->probability.final += ws.prob * decoder->gsf_out;
      }


      // if we can go to a final grammar state or we do partial
      // decoding then we insert the word into the lattice
      // we just ignore the rest of the transitions
      if (is_final) {
        lattice_insert(lattice, hyp);
        final++;
      } else {
        //We save the states just in case we need a partial decoding
        if (!final) {
          vector_append(states, hyp);
        }
      }
    } // end if word end transition
  } // while heap not empty

//  fprintf(stderr, "Finalizable HMMs %zu. Final lexic states %zu, Finals %d.\n", n_finalizable_hmms, n_final_lexics, final);

  // we haven't found any final states, so we use partial decoding
  if (!final) {
    fprintf(stderr, "WARNING!: A complete decoding is not possible, using partial decoding.\n");
    for (size_t i = 0; i < states->n_elems; i++) {
      lattice_insert(lattice, (hyp_t *) states->data[i]);
    }
  }
  vector_delete(states);

  // we create a fake final node in the lattice
  lattice_add_final_node(lattice);

  //Sort heaps of table of words as a vector
  lattice_sort(lattice);
}


int cmp_lattice_state(const void *a, const void *b) {
  double temp = (*(lat_state_t **) a)->max->probability.final - (*(lat_state_t **) b)->max->probability.final;
  if (temp > .0)
    return -1; // a first
  else if (temp < .0)
    return 1;  // b first
  else
    return 0;
}

/** analizes feature vector t and expands it to the next hypothesis heap
 * @param search a search status
 * @param feat_vec feature vector
 * @param t current frame
 * @param lattice output lattice
 */
void viterbi_frm(search_t *search, const float *feat_vec, lattice_t *lattice) {
  const decoder_t* decoder = search->decoder;
  const lex_t* lex = decoder->lex;
  const hmm_t* hmm = decoder->hmm;

  start_frame(search, feat_vec, lattice);

  while (!hh_is_empty(search->prev_heap)) {
    hyp_t *prev_hyp = hh_pop(search->prev_heap);

    // ignore hyps that do not pass the threshold
    if (hh_limit(search->prev_heap) > prev_hyp->probability.final)
      continue;

    //Matrix transitions
    const matrix_transitions_t *matrix = hmm->phonemes[prev_hyp->phoneme]->matrix;
    for (int transition = 0; transition < matrix->num_transitions; transition++) {
      //If the transition has a probability greater than 0
      if (!is_logzero(matrix->matrix_transitions[prev_hyp->state_hmm+1][transition])) {

        //If state is not a final state then this is a hmm transition
        if (transition != matrix->num_transitions - 1) {
          expand_hmm_transition(search, feat_vec, prev_hyp, transition);
        }
        //Else it must be either a lexic transition or language model transition
        else {
          //add transition probability from current hmm state to hmm final hmm state
          prev_hyp->probability.acoustic += matrix->matrix_transitions[prev_hyp->state_hmm + 1][transition];
          prev_hyp->probability.final    += matrix->matrix_transitions[prev_hyp->state_hmm + 1][transition];

          //If lex state is not the final state, then this is a lex transition
          if (lex->models[prev_hyp->word]->end != prev_hyp->state_lexic) {
            expand_lex_transition(search, feat_vec, prev_hyp);
          }//Lexic transition

          //Else this is language model transition
          else {
            // keep number of elements for statistics
            int prev_num_elements = lattice->num_elements;

            // language model transition
            lattice_insert(lattice, prev_hyp);

            if (ENABLE_STATISTICS >= SV_SHOW_FRAME) {
              if (prev_num_elements != lattice->num_elements)
                search->stats[search->n_frames - 1]->num_table_words++;
              search->stats[search->n_frames - 1]->n_word_hyps++;
            }
          }

        }//Lexic transition

      }//If
    }//Matrix transitions

  }

  expand_words_from_lattice(search, feat_vec, lattice);

  end_frame(search);
}



/// Obtains a lattice resulting from a decoding of list of feature vectors
/**
@param search Search status
@param feat_vec List of feature vectors
@param num_vectors Number of feature vectors
@param lattice Output lattice
*/
void decode(search_t *search, const features_t *features, lattice_t *lattice) {

  if (ENABLE_STATISTICS >= SV_SHOW_SAMPLE) {
    fprintf(stderr, "start sample stats:\n");
  }

  search->feature_type = features->type;
  if (features->type == FT_EMISSION_PROBABILITIES) {
    CHECK_SYS_ERROR(search->decoder->hmm->num_states == features->n_features, "Mismatch in number of features\n");
  }

  //fprintf(stderr, "frame %d:\n", search->n_frames);
  initial_stage(search, features->vector[search->n_frames], lattice);
  //hh_print(stderr, search->heap, search->decoder->grammar);

  while (search->n_frames < features->n_vectors) {
    //fprintf(stderr, "frame %d:\n", search->n_frames);
    viterbi_frm(search, features->vector[search->n_frames], lattice);
    //hh_print(stderr, search->heap, search->decoder->grammar);
  }

  end_stage(search, lattice);

  if (ENABLE_STATISTICS >= SV_SHOW_SAMPLE) {
    fprintf(stderr, "summary of sample stats:\n");
    print_sample_stats(stderr, search->stats, search->n_frames);
    fprintf(stderr, "\n");
  }
}

/** @} */ // end of viterbi
