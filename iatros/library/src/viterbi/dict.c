
/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#include <prhlt/trace.h>
#include <viterbi/lex.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <prhlt/constants.h>

void join_end_states(model_t **model){
 int i, j, elimino=0;
 float final=-1;

 //Busco el primer estado q es final
 for(i=0; i<(*model)->num_states; i++){
  if((*model)->states[i]->num_edges==0){
   if((*model)->states[i]->label!=(*model)->end){
    final=(*model)->states[i]->label;
   }
  }
 }
 //si encuentro dos finales me quedo con el primer estado final
 if(final!=-1){
  for(i=0; i<(*model)->num_states; i++){
   for(j=0; j<(*model)->states[i]->num_edges; j++){
    if((*model)->states[i]->edges[j]->destination==(*model)->end){
     elimino=(*model)->states[i]->edges[j]->destination;
     (*model)->states[i]->edges[j]->destination=final;
    }
   }
  }
  (*model)->end=final;
  free((*model)->states[elimino]);
  (*model)->num_states--;
 }

}


model_t * dict_append_model(lex_t *lex, model_t *model, symbol_t word) {

  if (word == VOCAB_NONE) {
    word = vocab_insert_symbol(lex->vocab, model->label, -1);
  }

  if (word >= lex->num_models) {
    lex->models = (model_t **) realloc(lex->models, (word + 1) * sizeof(model_t *));
    MEMTEST(lex->models);
    while (lex->num_models <= word) lex->models[lex->num_models++] = NULL;
  }

  model->vocab = word;

  lex->models[word] = model;

  if (lex->vocab->unk_word != NULL && strcmp(model->label, lex->vocab->unk_word) == 0) lex->unk_word_idx = word;

  return model;
}

int dict_append_state(lex_t *lex, model_t *model, int state_id) {
//   if(state_id<model->num_states){
//    return -1;
//   }
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
  return 1;
}

void dict_create_edge(state_lex_t *state, lex_t *lex, const char *label, int source, int destination, float probability) {

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

  edge->probability = probability+edge->probability;

  // save location
  edge->id = lex->n_edges++;
  lex->edge_locations = (lex_edge_location_t *) realloc(lex->edge_locations, lex->n_edges * sizeof(lex_edge_location_t));
  MEMTEST(lex->edge_locations);
  lex->edge_locations[edge->id].model = lex->num_models - 1;
  lex->edge_locations[edge->id].state = source;
  lex->edge_locations[edge->id].edge  = state->num_edges;

  state->num_edges++;

}

///Function to put in order the vector of edges of lexic with manys pronunciations
int dict_edge_prob_cmp(const void *va, const void *vb) {
  const edge_t *ia = *((edge_t**) va);
  const edge_t *ib = *((edge_t**) vb);
  if (ia->probability < ib->probability) return 1;
  else {
    if (ia->probability > ib->probability) return -1;
    else return 0;
  }
}

/// sorts edges by prob
/**
 * @param lex a lexicon
 */
void dict_postprocess(lex_t *lex) {
  for (int j = 0; j < lex->num_models; j++) {
    if (lex->models[j] != NULL) {
      for (int i = 0; i < lex->models[j]->num_states; i++) {
        for(int k=0; k<lex->models[j]->states[i]->num_edges; k++){
         lex->models[j]->states[i]->edges[k]->probability=(lex->models[j]->states[i]->edges[k]->probability == 0)? LOG_ZERO : log(lex->models[j]->states[i]->edges[k]->probability);
        }
        if (lex->models[j]->states[i]->num_edges > 1) {
          //sort the vector of edges of lexic with manys pronunciations
          qsort(lex->models[j]->states[i]->edges, lex->models[j]->states[i]->num_edges, sizeof(edge_t*), dict_edge_prob_cmp);
        }
      }
    }
  }
}
