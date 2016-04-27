/*
 * search.c
 *
 *  Created on: 19-abr-2009
 *      Author: valabau
 */

#include <viterbi/search.h>
#include <viterbi/lattice.h>
#include <prhlt/trace.h>
#include <string.h>

/** Creates a new search info and associates it to a decoder
 * @param decoder decoder to which the search is associated
 * @return a new search info
 */
search_t * search_create(const decoder_t *decoder) {
  search_t *search = (search_t *)malloc(sizeof(*search));
  MEMTEST(search);

  search->decoder = decoder_dup(decoder);

  //Create vector of visits for language model expansion
  search->visit = (int *) malloc(decoder->vocab->extended->last * sizeof(int));
  MEMTEST(search->visit);

  // Vectors to probabilities
  search->t_probability =  (float *)malloc(decoder->hmm->num_states * sizeof(float));
  search_clear_acoustic_probability_cache(search);

  // create couple of hypothesis heaps to alternate during the decoding process
  search->heap      = hh_create(decoder->histogram_pruning, decoder->beam_pruning);
  search->prev_heap = hh_create(decoder->histogram_pruning, decoder->beam_pruning);

  search->stats = NULL;
  search->n_frames = 0;
  search->is_prefix_search = false;
  search->emission_cache = NULL;

  return search;
}

/** Deletes the search
 * @param search the search
 */
void search_delete(search_t *search) {
  if (ENABLE_STATISTICS) {
    for (int f = 0; f < search->n_frames; f++) {
      free(search->stats[f]);
    }
    free(search->stats);
  }

  free(search->visit);

  if (search->emission_cache == NULL) {
    free(search->t_probability);
  }

  hh_delete(search->heap);
  hh_delete(search->prev_heap);
  // we do not delete the full decoder but we make a shallow delete
  // so that grammar, vocab, etc. are not delete
  if (search->is_prefix_search) {
    grammar_delete(search->decoder->grammar);
  }
  else {
    if (search->emission_cache != NULL) {
      for (size_t i = 0; i < search->emission_cache->n_elems; i++) {
        free(search->emission_cache->data[i]);
      }
      vector_delete(search->emission_cache);
    }
  }
  free(search->decoder);

  free(search);
}

/** clears the search info so that a new recognition process can be run on it
 * @param search the search
 */
void search_clear(search_t *search) {
  const decoder_t *decoder = search->decoder;

  // Create vector of visits for language model expansion
  search->visit = (int *)realloc(search->visit, decoder->vocab->extended->last * sizeof(int));
  MEMTEST(search->visit);

  // Vectors to probabilities
  if (search->emission_cache == NULL) {
    search->t_probability =  (float *)realloc(search->t_probability, decoder->hmm->num_states * sizeof(float));
  }
  search_clear_acoustic_probability_cache(search);


  //XXX: histogram and beam are not updated. Should we update them?
  hh_clear(search->heap);
  hh_clear(search->prev_heap);

  // reset the statistics
  if (ENABLE_STATISTICS) {
    for (int f = 0; f < search->n_frames; f++) {
      free(search->stats[f]);
    }
    free(search->stats);
    search->stats = NULL;
  }
  search->n_frames = 0;

}

/** initializes the emission cache
 * @param search the search
 */
void search_create_emission_cache(search_t *search) {
  free(search->t_probability);
  search->emission_cache = vector_create();
}

/// initialises the acoustic probability cache
/**
 * @param search the search with the acoustic cache to initialise
 */
INLINE void search_clear_acoustic_probability_cache(search_t *search) {
  for (int i = 0; i < search->decoder->hmm->num_states; i++) {
    search->t_probability[i] = LOG_ZERO;
  }
  search->best_achievable_ac = LOG_ZERO;
}


/// initialises the vector of visited words for backoff word expansion
/**
 * @param search the search
 */
INLINE void search_clear_visited_words(search_t *search) {
  memset(search->visit, 0, (search->decoder->vocab->extended->last) * sizeof(int));
}

/** Computes best achievable ac
 * @param search the search
 * we store the best acoustic seen so far
 * so that we can use it in the future to do early pruning
 */
void search_compute_best_achievable_ac(search_t *search) {
  search->best_achievable_ac = LOG_ZERO;
  for (int s = 0; s < search->decoder->hmm->num_states; s++) {
    if (search->t_probability[s] > search->best_achievable_ac) search->best_achievable_ac = search->t_probability[s];
  }

  if (ENABLE_STATISTICS >= SV_SHOW_WORD_EXPANSION) {
    search->stats[search->n_frames-1]->num_seen_states = 0;
    for (int s = 0; s < search->decoder->hmm->num_states; s++) {
      if (!is_logzero(search->t_probability[s])) search->stats[search->n_frames - 1]->num_seen_states++;
    }
    fprintf(stderr, "Seen %d(%6.2f%%) states out of %d\n", search->stats[search->n_frames-1]->num_seen_states, 100.0 * search->stats[search->n_frames-1]->num_seen_states
        / (float) search->stats[search->n_frames-1]->num_states, search->stats[search->n_frames-1]->num_states);
  }

  // set if early pruning is enabled or disabled
  search->do_acoustic_early_pruning = search->decoder->do_acoustic_early_pruning;
  if (is_logzero(search->best_achievable_ac)) {
    search->do_acoustic_early_pruning = false;
  }


}

