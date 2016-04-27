/*
 * search.h
 *
 *  Created on: 19-abr-2009
 *      Author: valabau
 */

#ifndef SEARCH_H_
#define SEARCH_H_

#include <iatros/decoder.h>
#include <iatros/statistics.h>
#include <iatros/heap.h>

typedef struct {
  decoder_t *decoder;         ///< decoder used to perform the search

  stats_t **stats;            ///< frame statistics
  int n_frames;               ///< number of decoded frames

  hyp_heap_t *heap;           ///< Heap where new hypothesis are added
  hyp_heap_t *prev_heap;      ///< Heap with hypothesis that must be expanded

  //Vectors to stores probability information from states.
  float *t_probability; ///< Vectors to write the calculated probabilities

  feat_type_t feature_type; ///< type of features. Some types have special behaviours

  //Vector of visits in Language Model Expansion
  int *visit;
  bool is_prefix_search; ///< indicates if it is a prefix search
  vector_t *emission_cache; ///< if != NULL, it stores temporary emission probabilities for latter usage
  float best_achievable_ac; ///< cache for the best achievable ac score
  bool do_acoustic_early_pruning; /**< If the acoustic early pruning is enabled or not.
                                       Note that this can be different from the one in decoder
                                       since when we do not have best achievable ac, we disable
                                       temporarily acoustic early pruning */
} search_t;

#ifdef __cplusplus
extern "C" {
#endif

search_t * search_create(const decoder_t *decoder);
void search_delete(search_t *search);
void search_clear(search_t *search);
void search_create_emission_cache(search_t *search);
INLINE void search_clear_acoustic_probability_cache(search_t *search);
INLINE void search_clear_visited_words(search_t *search);
INLINE void search_compute_best_achievable_ac(search_t *search);

#ifdef __cplusplus
}
#endif


#endif /* SEARCH_H_ */
