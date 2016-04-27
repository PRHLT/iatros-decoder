
/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#include <prhlt/trace.h>
#include <viterbi/lex.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

///Function to put in order the vector of edges of lexic with manys pronunciations
int edge_prob_cmp(const void *va, const void *vb) {
  const edge_t *ia = *((edge_t**) va);
  const edge_t *ib = *((edge_t**) vb);
  if (ia->probability < ib->probability) return 1;
  else {
    if (ia->probability > ib->probability) return -1;
    else return 0;
  }
}


model_t *lex_model_create(const char *label) {
  model_t *model = (model_t *) calloc(1, sizeof(model_t));
  MEMTEST(model);

  model->label = (char *) calloc(strlen(label) + 1, sizeof(char));
  MEMTEST(model->label);
  strcpy(model->label, label);

  model->states = NULL;
  model->num_states = 0;

  return model;
}

///Deletes a model
/**
 * @param model
 */
void lex_model_delete(model_t *model) {
  for (int j = 0; j < model->num_states; j++) {
    for (int k = 0; k < model->states[j]->num_edges; k++) {
      free(model->states[j]->edges[k]->label);
      free(model->states[j]->edges[k]);
    }
    free(model->states[j]->edges);
    free(model->states[j]);
  }
  free(model->states);
  free(model->state_ids);
  free(model->label);
  free(model);
}

model_t * lex_append_model(lex_t *lex, model_t *model) {
  symbol_t word = vocab_find_symbol(lex->vocab, model->label);

  if (word == VOCAB_NONE) {
    word = vocab_insert_symbol(lex->vocab, model->label, -1);
  }

  if (word >= lex->num_models) {
    lex->models = (model_t **) realloc(lex->models, (word + 1) * sizeof(model_t *));
    MEMTEST(lex->models);
    while (lex->num_models <= word) lex->models[lex->num_models++] = NULL;
  }

  model->vocab = word;

  CHECK(lex->models[word] == NULL, "WARNING: duplicated word in lexicon: '%s'. Substituting old model", model->label);

  if (lex->models[word] != NULL) {
    lex_model_delete(lex->models[word]);
  }

  lex->models[word] = model;

  if (lex->vocab->unk_word != NULL &&
      strcmp(model->label, lex->vocab->unk_word) == 0) lex->unk_word_idx = word;

  return model;
}

void lex_append_state(lex_t *lex, model_t *model, int state_id) {
  model->states = (state_lex_t **) realloc(model->states, (model->num_states + 1) * sizeof(state_lex_t *));
  MEMTEST(model->states);

  model->states[state_id] = (state_lex_t *) calloc(1, sizeof(state_lex_t));
  MEMTEST(model->states[state_id]);

  model->states[state_id]->label = state_id;
  model->states[state_id]->num_edges = 0;
  model->states[state_id]->edges = NULL;

  model->state_ids = (lex_state_id_t *) realloc(model->state_ids, (model->num_states + 1) * sizeof(lex_state_id_t));
  MEMTEST(model->state_ids);

  model->state_ids[state_id] = lex->n_states;

  lex->state_locations = (lex_state_location_t *) realloc(lex->state_locations, (lex->n_states + 1) * sizeof(lex_state_location_t));
  MEMTEST(lex->state_locations);

  lex->state_locations[lex->n_states].model = lex->num_models - 1;
  lex->state_locations[lex->n_states].state = state_id;

  lex->n_states++;
  model->num_states++;
}


void lex_append_edge(lex_t *lex, model_t *model, const char *label, int source, int destination, float probability) {
  state_lex_t *state = model->states[source];

  //Save the out edges of the same state
  state->edges = (edge_t **) realloc(state->edges, (state->num_edges + 1) * sizeof(edge_t *));
  MEMTEST(state->edges);

  state->edges[state->num_edges] = (edge_t *) calloc(1, sizeof(edge_t));
  MEMTEST(state->edges[state->num_edges]);

  edge_t *edge = state->edges[state->num_edges];

  edge->destination = destination;

  edge->label = (char *) malloc((strlen(label) + 1) * sizeof(char));
  MEMTEST(edge->label);
  strcpy(edge->label, label);

  // set the phoneme
  edge->phoneme = -1;
  for (int k = 0; k < lex->hmm->num_phonemes; k++) {
    if (strcmp(lex->hmm->phonemes[k]->label, edge->label) == 0) {
      edge->phoneme = k;
      break;
    }
  }
  REQUIRE(edge->phoneme != -1, "ERROR: The phoneme '%s' from word '%s' could not be found in the hmm\n",
                               edge->label, lex->models[lex->num_models-1]->label);

  edge->probability = probability;

  // save location
  edge->id = lex->n_edges++;
  lex->edge_locations = (lex_edge_location_t *) realloc(lex->edge_locations, lex->n_edges * sizeof(lex_edge_location_t));
  MEMTEST(lex->edge_locations);
  lex->edge_locations[edge->id].model = lex->num_models - 1;
  lex->edge_locations[edge->id].state = source;
  lex->edge_locations[edge->id].edge  = state->num_edges;

  state->num_edges++;
}

/// sorts edges by prob
/**
 * @param lex a lexicon
 */
void lex_postprocess(lex_t *lex) {
  for (int j = 0; j < lex->num_models; j++) {
    if (lex->models[j] != NULL) {
      for (int i = 0; i < lex->models[j]->num_states; i++) {
        if (lex->models[j]->states[i]->num_edges > 1) {
          //sort the vector of edges of lexic with manys pronunciations
          qsort(lex->models[j]->states[i]->edges, lex->models[j]->states[i]->num_edges, sizeof(edge_t*), edge_prob_cmp);
        }
      }
    }
  }
}

///Function to create the lexic
/**
 * @return structure with lexic
 */
lex_t * lex_create(vocab_t *vocab, hmm_t *hmm) {
  lex_t *lex;

  lex = (lex_t *) calloc(1, sizeof(lex_t));

  lex->num_models = 0;
  lex->models = NULL;
  //Create vocabulary
  lex->vocab = vocab;
  lex->hmm = hmm;

  lex->n_edges = 0;
  lex->edge_locations = NULL;
  lex->n_states = 0;
  lex->state_locations = NULL;

  lex->unk_word_idx = -1;

  return lex;
}

///Delete lexical models
/**
 * @param lex Models lexic
 */
void lex_delete(lex_t *lex) {
  for (int i = 0; i < lex->num_models; i++) {
    if (lex->models[i] != NULL) lex_model_delete(lex->models[i]);
  }
  free(lex->models);
  free(lex->edge_locations);
  free(lex->state_locations);
  free(lex);
}

///Checks if there is any word in the input vocabulary that is not in the lexicon
/**
 * @param a lexicon
 * @return the number of words in the vocab not included in the lexicon
 */
int lex_check_vocab(const lex_t *lex) {
  const vocab_t *vocab = lex->vocab;
  vocab_t *lex_vocab = vocab_create(vocab->last, NULL);

  for (int i = 0; i < lex->num_models; i++) {
    if (lex->models[i] != NULL) {
//      printf("The word '%s' (index '%d') is in the lexicon\n", lex->models[i]->label, i);
      vocab_insert_symbol(lex_vocab, lex->models[i]->label, CATEGORY_NONE);
    }
    else {
      PRINT("WARNING: Empty lexicon for index '%d' in the lexicon\n", i);
    }
  }

  int n_not_in_lex = 0;
  for (int i = 0; i < vocab->last; i++) {
    symbol_t word = vocab_find_symbol(lex_vocab, vocab->info[i].strings);
    //if the vocab explicitly has the word unk, then we must ensure that unk is in the lexicon
    if (vocab->info[i].category == CATEGORY_NONE && word == VOCAB_NONE && ((vocab->has_unk && i == UNK_WORD) || i!= UNK_WORD)) {
      PRINT("WARNING: The word '%s' (symbol '%d') was not found in the lexicon\n", vocab->info[i].strings, i);
      n_not_in_lex++;
    }
  }
  CHECK(n_not_in_lex == 0, "WARNING: %d words were not found in the lexicon\n", n_not_in_lex);
  vocab_delete(lex_vocab);
  return n_not_in_lex;
}
