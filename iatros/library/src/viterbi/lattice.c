/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#include <iatros/config.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "lattice.h"
#include <viterbi/viterbi.h>
#include <string.h>
#include <prhlt/gzip.h>
#include <prhlt/constants.h>
#include <prhlt/trace.h>


#define parent(i) (i/2)
#define left(i)   (2*i)
#define right(i)  (2*i+1)

/** creates a new lattice state with capacity for nbest hypothesis
 * @param nbest Maxim number of elements in the state
 * @return a new lattice state
 */
lat_state_t *lat_state_create(int nbest) {
  lat_state_t *lat_state = (lat_state_t *)calloc(1, sizeof(lat_state_t));
  MEMTEST(lat_state);

  lat_state->nbest = nbest;
  int capacity = nbest + 1;


  // nbest == 0 means no nbest limit, do dynamic reallocation
  if (lat_state->nbest == 0) capacity = 1;
  lat_state->words = (lat_hyp_t **)malloc(capacity * sizeof(lat_hyp_t *));
  MEMTEST(lat_state->words);
  for (int i = 1; i < capacity; i++) {
    lat_state->words[i] = (lat_hyp_t *)malloc(sizeof(lat_hyp_t));
    MEMTEST(lat_state->words[i]);
  }

  lat_state->num_words = 0;
  //We do not have max so far
  lat_state->max = NULL;

  lat_state->category = CATEGORY_NONE;

  return lat_state;
}

void lat_state_delete(lat_state_t *lat_state) {
  int capacity = (lat_state->nbest == 0)?lat_state->num_words:lat_state->nbest;
  for (int i = 1; i <= capacity; i++) {
    free(lat_state->words[i]);
  }
  free(lat_state->words);
  free(lat_state);
}

///fixes the heap of the lat_state upwards
/**
@param lat_state a lattice state
@param position a position in the heap that is expected to have broken the heap conditions
It is assumed that the sub-heap starting from position downwards is a valid heap.
From position upwards, the heap conditions may be broken. This function bubbles up
the element in position to fix the heap conditions
*/

void lat_state_bubble_up(lat_state_t *lat_state, int position) {
  if (lat_state->num_words <= 1) return;
  lat_hyp_t **heap = lat_state->words;
  while (position > 1 && (heap[parent(position)]->probability.final > heap[position]->probability.final)) {
    SWAP(heap[position], heap[parent(position)], lat_hyp_t *);
    position = parent(position);
  }
}

///fixes a heap from position downwards
/**
@param lat_state a lattice state
@param position position of the element that may broke heap conditions
*/
void lat_state_heapify(lat_state_t *lat_state, int position) {
  if (lat_state->num_words <= 1) return;

  lat_hyp_t **heap = lat_state->words;
  lat_hyp_t *hyp = heap[position];

  while (left(position) <= lat_state->num_words) {
    int son = left(position);
    if (son < lat_state->num_words && (heap[son + 1]->probability.final < heap[son]->probability.final)) {
      son++;
    }
    if (heap[son]->probability.final < hyp->probability.final) {
      heap[position] = heap[son];
    } else {
      break;
    }
    position = son;
  }
  heap[position] = hyp;
}

///sorts the hypotheses of the lat_state (heapsort)
/**
@param lat_state a lattice state
*/
void lat_state_sort(lat_state_t *lat_state) {
 int actual_num_words = lat_state->num_words;

  while (lat_state->num_words > 1) {
    SWAP(lat_state->words[1], lat_state->words[lat_state->num_words], lat_hyp_t *);
    lat_state->num_words--;
    lat_state_heapify(lat_state, 1);
  }

  lat_state->num_words = actual_num_words;
}


///Inserts a hypothesis in a lattice state
/**
@param lat_state a lattice state
@param hyp a hypothesis
@return true is the hyp has been inserted, else false
*/
bool lat_state_insert(lat_state_t *lat_state, hyp_t *hyp) {
  lat_hyp_t **heap = lat_state->words;

  int replace_idx = -1;
  for (int i = 1; i <= lat_state->num_words; i++) {
    // we have found a hypothesis that should be merged
    if (heap[i]->extended == hyp->extended && heap[i]->index == hyp->index) {
      replace_idx = i;
      break;
    }
  }
  // the heap is full so replace the worst hypothesis
  if (replace_idx == -1 && lat_state->nbest > 0 && lat_state->num_words >= lat_state->nbest) {
    replace_idx = 1;
  }

  if (replace_idx != -1) {
    if (hyp->probability.final > heap[replace_idx]->probability.final) {
      heap[replace_idx]->index = hyp->index;
      heap[replace_idx]->probability = hyp->probability;
      heap[replace_idx]->word = hyp->word;
      heap[replace_idx]->word_ptr = hyp->word_ptr;
      heap[replace_idx]->extended = hyp->extended;
      heap[replace_idx]->state_in = hyp->state_in ;
      heap[replace_idx]->state_out = hyp->state_out;

      if (lat_state->max == NULL || heap[replace_idx]->probability.final > lat_state->max->probability.final) {
        lat_state->max = heap[replace_idx];
      }

      // we have inserted the state at the middle so we fix the heap downwards
      lat_state_heapify(lat_state, replace_idx);
    }
    // reject the hypothesis
    else return false;
  }
  // the heap is not full and hyp does not replace any other element so insert it
  else {
    lat_state->num_words++;

    // nbest == 0 means no nbest limit, allocate space for one more hyp
    if (lat_state->nbest == 0) {
      heap = lat_state->words = (lat_hyp_t **)realloc(lat_state->words, (lat_state->num_words + 1) * sizeof(lat_hyp_t *));
      MEMTEST(lat_state->words);
      heap[lat_state->num_words] = (lat_hyp_t *)malloc(sizeof(lat_hyp_t));
      MEMTEST(lat_state->words[lat_state->num_words]);
    }

    heap[lat_state->num_words]->index = hyp->index;
    heap[lat_state->num_words]->probability = hyp->probability;
    heap[lat_state->num_words]->word = hyp->word;
    heap[lat_state->num_words]->word_ptr = hyp->word_ptr;
    heap[lat_state->num_words]->extended = hyp->extended;
    heap[lat_state->num_words]->state_in = hyp->state_in ;
    heap[lat_state->num_words]->state_out = hyp->state_out;

    if (lat_state->max == NULL || heap[lat_state->num_words]->probability.final > lat_state->max->probability.final) {
      lat_state->max = heap[lat_state->num_words];
    }
    // we have inserted the state at the bottom and bubble it up to fix the heap
    lat_state_bubble_up(lat_state, lat_state->num_words);
  }

  //for (int i = 1; i <= lat_state->num_words; i++) {
  //  if (heap[i]->probability.final > lat_state->max->probability.final) {
  //    fprintf(stderr, "Warning: Illformed lattice heap\n");
  //  }
  //}

  return true;
}


///Create the table of words
/**
@return Table of words
*/
lattice_t *lattice_create_from_args(const args_t *args, const decoder_t *decoder) {
  arg_error_t error;
  int nbest = args_get_int(args, LATTICE_MODULE_NAME".nbest", &error);
  int nnode = args_get_int(args, LATTICE_MODULE_NAME".nnode", &error);

  return lattice_create(nbest, nnode, decoder);
}

lattice_t *lattice_create(int nbest, int nnode, const decoder_t *decoder) {
  lattice_t *lattice;

  //Table of words
  lattice = (lattice_t *) malloc(sizeof(lattice_t));
  lattice->num_elements = 0;
  lattice->vector = NULL;

  lattice->nbest = nbest;
  lattice->nnode = nnode;
  if (lattice->nnode == -1) lattice->nnode = lattice->nbest;

  //Create vector of visits in table of words
  lattice->decoder = decoder;
  lattice->grammar_state_index = hash_create(541, NULL);

  lattice->n_frames = 0;
  lattice->initial_index = 0;

  return lattice;
}



///adds a fake final node to the lattice at frame t and
/// for every real state, creates a link to it with
/// the !NULL symbol and probability 1
/**
@param lattice a lattice
@param t frame number
*/
void lattice_add_final_node(lattice_t *lattice) {
  //Create un new element in table of words
  lattice->vector = (lat_state_t **) realloc(lattice->vector, sizeof(lat_state_t *) * (lattice->num_elements + 1));
  lattice->vector[lattice->num_elements] = lat_state_create(lattice->nbest);
  lattice->vector[lattice->num_elements]->t = lattice->n_frames - 1;
  lattice->num_elements++;

  symbol_t sym = extended_vocab_find_symbol(lattice->decoder->vocab, "!NULL");
  const extended_symbol_t *symbol_null = extended_vocab_get_extended_symbol(lattice->decoder->vocab, sym);

  // for every real final node we add a !NULL
  // transition to the fake final state
  for (int element = lattice->initial_index; element < lattice->num_elements - 1; element++) {
    for (int i = 1; i <= lattice->vector[element]->num_words; i++) {
      hyp_t hyp;
      memset(&hyp, 0, sizeof(hyp_t));
      hyp.probability.final = lattice->vector[element]->words[i]->probability.final;
      hyp.extended = sym;
      hyp.word = symbol_null->input[0];
      hyp.index = element;
      hyp.category = CATEGORY_NONE;
      lat_state_insert(lattice->vector[lattice->num_elements - 1], &hyp);
    }
  }
}

/** key that identifies lattice states
 */
struct lat_state_key {
  const state_grammar_t *grammar_state; /**< the grammar state*/
  const symbol_t *phrase_state; /**< the word being processed in the phrase */
};

/** inserts a hypothesis at frame t in the lattice
@param lattice a lattice
@param hyp a hypothesis
@param t t
@return the index in the lattice of the hypothesis lattice state
*/
lat_state_t * lattice_insert(lattice_t *lattice, hyp_t *hyp) {
  struct lat_state_key key = { hyp->state, hyp->word_ptr };
  lat_state_t * lat_state = (lat_state_t *) hash_search(&key, sizeof(struct lat_state_key), lattice->grammar_state_index);
  //lat_state_t * lat_state = (lat_state_t *) hash_search(&(hyp->state), sizeof(lat_state_t *), lattice->grammar_state_index);

  // if we haven't visited the state yet
  if (lat_state == NULL) {
    //Create un new element in table of words
    lattice->vector = (lat_state_t **) realloc(lattice->vector, sizeof(lat_state_t *) * (lattice->num_elements + 1));
    MEMTEST(lattice->vector);
    lat_state = lat_state_create(lattice->nnode);
    lattice->vector[lattice->num_elements] = lat_state;

    lat_state->state = hyp->state;
    lat_state->word_ptr = hyp->word_ptr;
    lat_state->history_category = hyp->history_category;

    lat_state->category = hyp->category;

    lat_state->t = lattice->n_frames - 1;
    lat_state->index = lattice->num_elements;

    hash_insert(&key, sizeof(struct lat_state_key), lat_state, lattice->grammar_state_index);
    //hash_insert(&(hyp->state), sizeof(lat_state_t *), lat_state, lattice->grammar_state_index);
    lattice->num_elements++;
  }

  //Insert element in heap
  lat_state_insert(lat_state, hyp);
  return lat_state;
}

///deletes the lattice
/**
@param lattice the lattice to be deleted
*/
void lattice_delete(lattice_t *lattice){
  for(int t=0; t<lattice->num_elements; t++){
    lat_state_delete(lattice->vector[t]);
  }
  free(lattice->vector);

  hash_delete(lattice->grammar_state_index, false);
  free(lattice);
}

///sorts all the states in the lattice
/**
@param lattice a lattice
*/
void lattice_sort(lattice_t *lattice) {
  for(int i=0; i < lattice->num_elements; i++) {
    if(lattice->vector[i]->num_words>1) {
      lat_state_sort(lattice->vector[i]);
    }
  }
}

///Writes a lattice to a file
/**
@param lattice Table of words to retrieve the path
@param file File to print the lattice
@param name s string for tha name field in the lattice
*/
void lattice_write(const lattice_t *lattice, FILE *file, const char *name) {
  if (lattice->num_elements == 0) {
    CHECK(lattice->num_elements > 0, "WARNING: The lattice is empty!!!");
    return;
  }
  // stores the state id and tells if the state can be visited
  // from the final state backwards
  int *visited_states = (int *) calloc(lattice->num_elements, sizeof(int));

  int num_edges = 0, num_states = 0;
  //Mark states that can be reached from the final state backwards
  //as valid states
  //Mark the final state as visited
  visited_states[lattice->num_elements-1] = 1;
  //iterate backwards to visit the states
  for (int i = lattice->num_elements - 1; i >= 0; i--) {
    if (visited_states[i] == 1) {
      num_states++; //Count the number of states
      //Iterate edges to mark source states
      for (int j = 1; j < lattice->vector[i]->num_words + 1; j++) {
        num_edges++; //Count the number of edges
        if (lattice->vector[i]->words[j]->index != -1) {
          visited_states[lattice->vector[i]->words[j]->index] = 1;
        }
      }
    }
  }

  // print header
  fprintf(file, "# Word graph in SLF format generated by iAtros\n");
  fprintf(file, "UTTERANCE=%s\n", name);
  fprintf(file, "lmscale=%.2f\n", lattice->decoder->gsf);
  // the wip is negated to fit the standard formulation
  fprintf(file, "wdpenalty=%.2f\n", -lattice->decoder->wip);
  if (lattice->decoder->grammar->silence != VOCAB_NONE || lattice->decoder->grammar->pause != VOCAB_NONE) {
    fprintf(file, "x1scale=%.2f   # silence score\n", lattice->decoder->grammar->silence_score);
  }
  if (lattice->decoder->input_grammar != NULL) {
    fprintf(file, "x2scale=%.2f   # lminscale\n", lattice->decoder->gsf_in);
  }
  if (lattice->decoder->output_grammar != NULL) {
    fprintf(file, "x3scale=%.2f   # lmoutscale\n", lattice->decoder->gsf_out);
  }
  if (lattice->decoder->wip_out != 0) { 
    fprintf(file, "x4scale=%.2f   # wdpenalty_output\n", -lattice->decoder->wip_out);
  }
  //fprintf(file, "base=%g\n", M_E);
  if (lattice->decoder->vocab->n_weights > 0) {
    fprintf(file, "x5scale=%f   # weights[0]\n", lattice->decoder->vocab->weights[0]);
    for (int s = 1; s < lattice->decoder->vocab->n_weights; s++) {
      fprintf(file, "x%dscale=%f   # weights[%d]\n", s+5, lattice->decoder->vocab->weights[s], s);
    }
    fprintf(file, "\n");
  }
  fprintf(file, "# Size line\n");
  fprintf(file, "N=%d L=%d\n", num_states + 1, num_edges);
  fprintf(file, "# Node definitions\n");

  //Print states
  int state_id = 0;
  fprintf(file, "I=%d t=%d\n", state_id, 0);
  state_id++;
  for (int i = 0; i < lattice->num_elements; i++) {
    if (visited_states[i] == 1) {
      fprintf(file, "I=%d t=%d\n", state_id, lattice->vector[i]->t);
      //Rename the states to put the states in order
      visited_states[i] = state_id;
      state_id++;
    }
  }

  //Print edges, probabilities are in log10 to fit the format definition
  int edge_id = 0;
  for (int i = 0; i < lattice->num_elements; i++) {
    for (int j = 1; j < lattice->vector[i]->num_words + 1; j++) {
      if (visited_states[i] > 0) {
        //If history is NULL the previous state is 0
        if (lattice->vector[i]->words[j]->index == -1) {
          fprintf(file, "J=%d S=%d E=%d ", edge_id, 0, visited_states[i]);
        } else {
          fprintf(file, "J=%d S=%d E=%d ", edge_id, visited_states[lattice->vector[i]->words[j]->index], visited_states[i]);
        }
        fprintf(file, "W=%s a=%f",
            vocab_get_string(lattice->decoder->vocab->in, lattice->vector[i]->words[j]->word),
            lattice->vector[i]->words[j]->probability.acoustic);

        if (lattice->vector[i]->words[j]->word == lattice->decoder->grammar->silence
            || lattice->vector[i]->words[j]->word == lattice->decoder->grammar->pause)
        {
          float scr = lattice->vector[i]->words[j]->probability.lm;
          if (lattice->decoder->grammar->silence_score != 0) scr /= lattice->decoder->grammar->silence_score;
          else scr = 1;
          fprintf(file, " x1=%f", scr);
        }
        else {
          fprintf(file, " l=%f", lattice->vector[i]->words[j]->probability.lm);
        }

        if (lattice->vector[i]->words[j]->word_ptr != NULL) {
          const extended_symbol_t *sym = extended_vocab_get_extended_symbol(lattice->decoder->vocab, lattice->vector[i]->words[j]->extended);
          // if it is the first word of the phrase, print out the translation
          if (lattice->vector[i]->words[j]->word_ptr == sym->input) {
            if (lattice->decoder->vocab->out->last > 0) {
              char *output = NULL;
              vocab_symbols_to_string(sym->output, lattice->decoder->vocab->out, &output);
              {  // convert spaces to underscores so that output is SRILM compatible
                char *ptr = output;
                while (*ptr != '\0') {if (*ptr == ' ') *ptr = '_'; ptr++;}
              }
              fprintf(file, " O=%s", output);
              free(output);
            }
            if (lattice->decoder->vocab->n_weights > 0 && sym->scores != NULL) {
              fprintf(file, " x5=%f", sym->scores[0]);
              for (int s = 1; s < lattice->decoder->vocab->n_weights; s++) {
                fprintf(file, " x%d=%f", s+5, sym->scores[s]);
              }
            }
          }
          else {
            //fprintf(file, " O=");
          }
          //fprintf(file, " X=%s", extended_vocab_get_string(lattice->decoder->vocab, lattice->vector[i]->words[j]->extended));
        }
        else {
          if (lattice->decoder->vocab->out->last > 0) {
            fprintf(file, " O=!NULL");
          }
        }
        if (lattice->vector[i]->category != CATEGORY_NONE) {
          fprintf(file, " div=c:%d", lattice->vector[i]->category);
        }
        if (lattice->decoder->input_grammar != NULL) {
          fprintf(file, " x2=%f", lattice->vector[i]->words[j]->probability.in_lm);
        }
        if (lattice->decoder->output_grammar != NULL) {
          fprintf(file, " x3=%f x4=1", lattice->vector[i]->words[j]->probability.out_lm);
        }
        //fprintf(file, " f=%f idx=%d", lattice->vector[i]->words[j]->probability.final, i);
        fprintf(file, "\n");

        edge_id++;
      }
    }
  }

  free(visited_states);
}


///obtains the path (sequence of symbols) that has the maximum probability in the lattice
/**
@param[in] lattice a lattice from which to retrieve the path
@param[out] result an array of VOCAB_NONE terminated symbols
@return the path probability
The value of *result will be overwritten.
It is the caller's responsibility to free the memory pointed by *result.
*/
float lattice_best_hyp(const lattice_t *lattice, symbol_t **result) {
  int index = lattice->num_elements - 1;
  double prob = LOG_ZERO;

  if (index >= 0 && lattice->vector[index]->num_words > 0) {
    prob = lattice->vector[index]->max->probability.final;
    symbol_t *best = (symbol_t *) malloc(MAX_LINE * sizeof(symbol_t));
    symbol_t *frames = (int *) malloc(MAX_LINE * sizeof(int));
    int length = 0;

    // iterate from the final state to the initial
    while (index > -1) {
      // store in reverse order the words of the best path
      const lat_hyp_t const * best_hyp = lattice->vector[index]->max;
      //const extended_symbol_t const * word = extended_vocab_get_extended_symbol(
      //                                         lattice->decoder->grammar->vocab,
      //                                         best_hyp->extended);
      // if it is the start of a phrase then add it
      //if (best_hyp->word_ptr == word->input) {
        best[length] = lattice->vector[index]->max->extended;
        frames[length++] = lattice->vector[index]->t;
      //}
      // go to the previous state
      index = best_hyp->index;
    }

    (*result) = (symbol_t *)malloc(length * sizeof(symbol_t));

    if (ENABLE_STATISTICS >= SV_SHOW_SAMPLE) {
      fprintf(stderr, "Recognized:");
    }
    // reverse order
    int j, t = length - 1;
    for (j = 0; j < length - 1; j++, t--) {
      (*result)[j] = best[t];
      if (ENABLE_STATISTICS >= SV_SHOW_SAMPLE) {
        fprintf(stderr, " %s %d", extended_vocab_get_string(lattice->decoder->vocab, best[t]), frames[t]);
      }

    }
    if (ENABLE_STATISTICS >= SV_SHOW_SAMPLE) {
      fprintf(stderr, "\n");
    }
    (*result)[j] = VOCAB_NONE;

    free(best);
    free(frames);

  } else {
    (*result) = NULL;
    prob = LOG_ZERO;
  }

  return prob;
}

// void lattice_dump(const lattice_t *lattice, FILE *file) {
//    //This code is to print the table of words
//    printf("Table of words\n");
//    //Print all table of words
//    for(int k=0; k<lattice->num_elements; k++){
// //      k2=1;
// //       printf("i--->%d t->%d word->%s prob->ac%e lm %e final %e index->%d state->%d\n", k, lattice->vector[k]->t, get_string(vocab, lattice->vector[k]->words[k2]->word), lattice->vector[k]->words[k2]->probability.acoustic, lattice->vector[k]->words[k2]->probability.lm,lattice->vector[k]->words[k2]->probability.final, lattice->vector[k]->words[k2]->index, lattice->vector[k]->state);
//
//      printf("i--->%d t->%d state->%d\n", k, lattice->vector[k]->t, lattice->vector[k]->state);
//      for(int k2=1; k2<=lattice->vector[k]->num_words; k2++){
//        printf("                           word->%s prob->ac%g lm %g final %g index->%d\n", vocab_get_string(lattice->vocab->in, lattice->vector[k]->words[k2]->word), lattice->vector[k]->words[k2]->probability.acoustic, lattice->vector[k]->words[k2]->probability.lm,lattice->vector[k]->words[k2]->probability.final, lattice->vector[k]->words[k2]->index);
//      }
//    }

//   for (int i = 0; i < lattice->num_elements; i++) {
//     fprintf(file, "lattice element %d, state %d\n", i, lattice->vector[i]->state);
//     fprintf(file, "\t%d words:\n",lattice->vector[i]->num_words);
//     for (int w = 0; w < lattice->vector[i]->num_words; w++) {
//       const lat_hyp_t *hyp = lattice->vector[i]->words[w];
//       if (hyp != NULL) {
//         fprintf(file, "\t\text %d %s, word (%s), index = %d, final = %g, ac = %g, lm = %g\n",
//             w, extended_vocab_get_string(lattice->vocab, hyp->extended),
//             vocab_get_string(lattice->vocab->in, hyp->word),
//             hyp->index, hyp->probability.final, hyp->probability.acoustic, hyp->probability.lm);
//       }
//     }
//   }

// }

void lattice_reset_frame(lattice_t *lattice) {
  // initialize visited words
  hash_clear(lattice->grammar_state_index, false);
  lattice->initial_index = lattice->num_elements;
}

void lattice_start_frame(lattice_t *lattice) {
  // initialize visited words
  lattice_reset_frame(lattice);
  lattice->n_frames++;
}

void lattice_to_wordlist(const lattice_t *lattice, int *n_elems, lat_hyp_t const *** words) {
  if (lattice->num_elements == 0) {
    CHECK(lattice->num_elements > 0, "WARNING: The lattice is empty!!!");
    *n_elems = 0;
    *words = NULL;
    return;
  }
  // stores the state id and tells if the state can be visited
  // from the final state backwards
  int *visited_states = (int *) calloc(lattice->num_elements, sizeof(int));

  //Mark states that can be reached from the final state backwards
  //as valid states
  //Mark the final state as visited
  visited_states[lattice->num_elements-1] = 1;
  //iterate backwards to visit the states
  for (int i = lattice->num_elements - 1; i >= 0; i--) {
    if (visited_states[i] == 1) {
      //Iterate edges to mark source states
      for (int j = 1; j < lattice->vector[i]->num_words + 1; j++) {
        if (lattice->vector[i]->words[j]->index != -1) {
          visited_states[lattice->vector[i]->words[j]->index] = 1;
        }
        else { // it is an initial word and should be added to the word list
          (*n_elems)++;
        }
      }
    }
  }

  (*words) = (lat_hyp_t const **) malloc((*n_elems) * sizeof(lat_hyp_t *));
  MEMTEST(words);

  //Print edges, probabilities are in log10 to fit the format definition
  int pos = 0;
  for (int i = 0; i < lattice->num_elements; i++) {
    for (int j = 1; j < lattice->vector[i]->num_words + 1; j++) {
      if (visited_states[i] > 0 && lattice->vector[i]->words[j]->index == -1) {
        REQUIRE(pos < *n_elems, "This should not occur. There must be a bug");
        (*words)[pos++] = lattice->vector[i]->words[j];
      }
    }
  }

  free(visited_states);
}
