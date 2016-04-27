/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#ifndef _LEX_H
#define _LEX_H
#include <stdio.h>
#include <stdlib.h>
#include <prhlt/vocab.h>
#include <iatros/hmm.h>

/// type of lexicon
typedef enum { ATROS_LEX = 0, HTK_LEX } lex_type_t;

/// List phonemes.
typedef struct list_phonemes {
  int max; ///< Number maximum of the phonemes
  int num_elements; ///< Number the phonemes in vector
  int *vector; ///< Vector with the phonemes
} list_phonemes_t;

/// Edge
typedef enum { LEX_EDGE_NONE = -1 } lex_edge_id_t;
/// Location of a lex_id_t in the lexicon
typedef struct {
  int model; ///< model
  int state; ///< state in the model
  int edge;  ///< edge in the state
} lex_edge_location_t;

typedef struct edge {
  int destination; ///< State of destination of the edge
  char *label; ///< Name of egde
  int phoneme; ///< Number of the phoneme of the edge
  float probability; ///< Probability of the edge
  lex_edge_id_t id;   ///< identifies edge among any other in the whole lexicon
} edge_t;

/// State
typedef enum { LEX_STATE_NONE = -1 } lex_state_id_t;
/// Location of a lex_id_t in the lexicon
typedef struct {
  int model; ///< model
  int state; ///< state in the model
} lex_state_location_t;

typedef struct state_lex {
  int label; ///< Name of state
  edge_t **edges; ///< Vector with all edges
  int num_edges; ///< Number of edges of the lexic model
} state_lex_t;

/// Lexic model
typedef struct model {
  char *label; ///< Word of the lexic model
  symbol_t vocab; ///< Number of word
  state_lex_t **states; ///< Vector with states that we saved
  lex_state_id_t *state_ids; ///< Vector with state ids
  int num_states; ///< Number of states that we saved
  int initial; ///< Number of initial state
  int end; ///< Number of end state
} model_t;

/// Lexic model
typedef struct lex {
  int num_models; ///< Number of lexic models
  model_t **models; ///< Lexic models
  vocab_t *vocab; ///< Hash to convert words in numbers
  hmm_t *hmm; ///< HMMs which are linked to morphemes in lexicon edges
  int n_edges; ///< number of total edges and next esge lexicon id
  lex_edge_location_t *edge_locations; ///< edge lexicon id locations
  int n_states; ///< number of total state and next state lexicon id
  lex_state_location_t *state_locations; ///< state lexicon id locations
  int unk_word_idx; ///< index of the unknown word in the list of models. -1 if not in there
} lex_t;

//Create and initiate the lexic
lex_t * lex_create(vocab_t *vocab, hmm_t *hmm);
//Delete models lexic
void lex_delete(lex_t *lex);
int lex_load(lex_t * lex, FILE *file);
int dict_load(lex_t * lex, FILE *file);
void lex_postprocess(lex_t *lex);
int lex_check_vocab(const lex_t *lex);
model_t *lex_model_create(const char *label);
void lex_model_delete(model_t *model);
void join_end_states(model_t **model);

#endif // _LEX_DRIVER_H
