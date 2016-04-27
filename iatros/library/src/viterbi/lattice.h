/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#ifndef __LATTICE_H__
#define __LATTICE_H__

#include <prhlt/args.h>
#include <iatros/probability.h>
#include <iatros/hypothesis.h>
#include <iatros/grammar.h>
#include <iatros/decoder.h>


#define LATTICE_MODULE_NAME "lattice"

static const arg_module_t lattice_module = { LATTICE_MODULE_NAME, "lattice module",
    {
        {"nbest", ARG_INT, "1", ARG_FLAGS_NONE, "Number of nbest incoming words to be stored by the final lattice state."
                                   "'1' by default. '0' to indicate infinite nbests"},
        {"nnode", ARG_INT, "-1", ARG_FLAGS_NONE, "Number of nbest incoming words to be stored per each lattice state."
                                    "By default it equals to nbest"},
        {NULL, ARG_END_MODULE, NULL, ARG_FLAGS_NONE, NULL}
    }
};


/// Heap of table of words. Min-heap to delete the minimum element when the heap is full
typedef struct {
  int index; ///< Index in the table of words of the history
  probability_t probability; ///Information about the probability
  symbol_t word; ///< Word
  const symbol_t *word_ptr; ///< Word pointer for extended symbols.
  symbol_t extended; ///< extended word
  state_grammar_t * state_in; ///< Number of target state of the input grammar
  state_grammar_t * state_out; ///< Number of target state of the output grammar
} lat_hyp_t;

/// A state of the lattice
typedef struct {
  lat_hyp_t **words; ///< Egdes with the word that arrived to the state
  lat_hyp_t *max; ///< Maximum element in the heap of table of words
  int num_words; ///< Number of words
  state_grammar_t * state; ///< Number of state
  state_grammar_t * history_category; ///< Number of source state of the grammar in the aef with the category
  const symbol_t *word_ptr; ///< Word pointer for extended symbols.

  int category; ///< Number of aef with the category
  int t; ///< t when the word ends
  int nbest; ///< the maximum number of hypotheses that can be stored
  int index; ///< index of the lat_state in the lattice
} lat_state_t;

/// List with table of words.
typedef struct lattice_t {
  int num_elements; ///< Number of elements in a vector
  lat_state_t **vector; ///< Table of words

  int nnode; ///< Number of edges in a state in word graph
  int nbest; ///< Number of n-best to word graph

  const decoder_t *decoder; ///< grammar

  int n_frames; ///< number of frames
  int initial_index; ///< latest initial index

  hash_t *grammar_state_index; ///< contains the table indexes for the visited states in the grammar
} lattice_t;

#ifdef __cplusplus
extern "C" {
#endif


lattice_t *lattice_create_from_args(const args_t *args, const decoder_t *decoder);
lattice_t *lattice_create(int nbest, int nnode, const decoder_t *decoder);
lat_state_t * lattice_insert(lattice_t *lattice, hyp_t *state);
void lattice_sort(lattice_t *lattice);
float lattice_best_hyp(const lattice_t *lattice, symbol_t **result);
void lattice_write(const lattice_t *lattice, FILE *file, const char *name);
void lattice_delete(lattice_t *lattice);
void lattice_dump(const lattice_t *lattice, FILE *file);
void lattice_add_final_node(lattice_t *lattice);
void lattice_reset_frame(lattice_t *lattice);
void lattice_start_frame(lattice_t *lattice);
void lattice_to_wordlist(const lattice_t *lattice, int *n_elems, lat_hyp_t const *** words);

#ifdef __cplusplus
}
#endif


#endif // __LATTICE_H__
