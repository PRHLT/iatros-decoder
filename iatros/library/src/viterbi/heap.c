/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#include <prhlt/trace.h>
#include <prhlt/utils.h>
#include <prhlt/constants.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "heap.h"

#define parent(i) (i/2)
#define left(i)   (2*i)
#define right(i)  (2*i+1)

/// State viterbi. State in heap in each t
typedef struct hyp_node_t {
  hyp_t hyp;  ///< Hypothesis of the search process
  int index_heap; ///< Index of the heap where is the state
  struct hyp_node_t *next; ///< Next node in hash table
} hyp_node_t;

/// Heap to viterbi
typedef struct {
  hyp_node_t **vector; ///< Vector with states
  int num_elements; ///< Number of elements in vector
  int max; ///< Maximum number of the heap
  bool is_heap; ///< Flag to indicate if is a heap o a vector
} heap_t;

///Hash table
typedef struct {
  hyp_node_t **vector; ///< Array of pointers to states (array of lists of states)
  int num_buckets; ///< Number of buckets
  int num_elements; ///< Number of elements
} thash_v_t;

/// Pool of states
typedef struct {
  hyp_node_t *vector; ///< Vector with elements of the pool
  int num_elements; ///< Number of elements of the pool
} pool_t;

struct hyp_heap_t {
  heap_t *heap;  ///< a heap of hypotheses
  thash_v_t *hash; ///< a hash to find hypotheses in the heap
  pool_t *pool; ///< a memory pool to avoid continuous and costly memory allocations

  float beam;  ///< the beam w.r.t. the maximum probability
  float max_elements; ///< max number of elements (a.k.a. histogram_pruning)
  float max_probability; ///< Maximum state probability in the heap
  float current_limit;   ///< Scores worse that current limit are automatically discarded
};


///creates a new heap
/**
@param num_max Maxim number of elements of heap
@return a new heap
*/
heap_t * heap_create(int num_max) {
  heap_t *heap;

  heap = (heap_t *) malloc(sizeof(heap_t));
  MEMTEST(heap);

  heap->vector = (hyp_node_t **) malloc(num_max * sizeof(hyp_node_t *));
  MEMTEST(heap->vector);
  heap->max = num_max;
  heap->num_elements = 0;
  heap->is_heap = 0;

  return heap;
}

///Erases all heap elements
/**
@param heap a heap
*/
void heap_clear(heap_t *heap) {
    heap->num_elements = 0;
    heap->is_heap = 0;
}

///Heapify
/**
@param heap Heap
@param position Position of the father to heapify
*/
INLINE void heap_heapify(heap_t *heap, int position) {
  int son;
  hyp_node_t *node = heap->vector[position];

  if (heap->num_elements == 0) return;

  while (left(position)<=heap->num_elements) {
    son = left(position);
    if (son < heap->num_elements && (heap->vector[son + 1]->hyp.probability.final < heap->vector[son]->hyp.probability.final)) {
      son++;
    }
    if (heap->vector[son]->hyp.probability.final < node->hyp.probability.final) {
      heap->vector[position] = heap->vector[son];
    } else {
      break;
    }
    position = son;
  }
  heap->vector[position] = node;
}

///Build a heap from a vector
/**
@param heap the heap
*/
void hh_build_heap(hyp_heap_t *heap) {
 for(int i=parent(heap->heap->num_elements); i>0; i--){
  heap_heapify(heap->heap, i);
 }
}

///fixes the heap upwards
/**
@param heap Heap
@param position a position in the heap that is expected to have broken the heap conditions
It is assumed that the sub-heap starting from position downwards is a valid heap.
From position upwards, the heap conditions may be broken. This function bubbles up
the element in position to fix the heap conditions
*/
INLINE void heap_bubble_up(heap_t *heap, int position) {
  while (position > 1 && (heap->vector[parent(position)]->hyp.probability.final) > (heap->vector[position]->hyp.probability.final)) {
    SWAP(heap->vector[position], heap->vector[parent(position)], hyp_node_t*);
    heap->vector[position]->index_heap = position;
    heap->vector[parent(position)]->index_heap = parent(position);
    position = parent(position);
  }
}

///Inserts an element in a heap
/**
@param heap Heap
@param element Element to insert in the heap
*/
INLINE void heap_insert(heap_t *heap, hyp_node_t *element) {
  //increase the number of elements of the heap
  heap->num_elements = heap->num_elements + 1;
  //put element in the last position of the heap
  heap->vector[heap->num_elements] = element;
  element->index_heap = heap->num_elements;

  if (heap->is_heap) {
   //fix the heap upwards
   heap_bubble_up(heap, heap->num_elements);
  }
}

///If heap is a vector then we create a heap
/**
@param hh the heap
*/
void hh_check_heap(hyp_heap_t *hh){
 if(!hh->heap->is_heap){
  //Build a heap from a vector
  hh_build_heap(hh);
  hh->heap->is_heap=1;
 }
}

///Sorts heap as a vector
/**
@param heap Heap
@return the number of elements
*/
// INLINE int heap_sort(heap_t *heap) {
//   int num_elements = heap->num_elements;
//
//   while (heap->num_elements > 1) {
//     hyp_node_t *element = heap->vector[heap->num_elements];
//     heap->vector[heap->num_elements] = heap->vector[1];
//     heap->vector[heap->num_elements]->index_heap = heap->num_elements;
//     heap->vector[1] = element;
//     heap->vector[1]->index_heap = 1;
//     heap->num_elements--;
//     if (heap->num_elements > 0) {
//       heap_heapify(heap, 1);
//     }
//   }
//   return num_elements;
// }

///Deletes an element from hash table and returns it
/**
 @param thash a hash table
 @param hyp_node an hypothesis node
 @return the deleted state
 */
INLINE hyp_node_t *thash_delete_element(thash_v_t *thash, hyp_node_t *hyp_node) {
  //Calculate the number of bucket
  unsigned int f = (hyp_hash(&hyp_node->hyp) % thash->num_buckets);

  hyp_node_t *current = thash->vector[f];
  hyp_node_t *previous = NULL;

  // search the hyp node in the bucket list
  while (current != NULL && hyp_cmp(&hyp_node->hyp, &current->hyp) == 0) {
    previous = current;
    current = current->next;
  }

  //XXX: this shouldn't happen. if we are trying to delete it is because we inserted it
  // however it is necessary to put it. why? is there something wrong in the code?
  if (current == NULL)
    return NULL;

  // it is the first element in the list
  if (previous == NULL)
    thash->vector[f] = current->next;
  // it is next to another element
  else
    previous->next = current->next;

  thash->num_elements--;

  return current;

}

///Erases all hash table elements
/**
 @param thash hash table
 */
INLINE void thash_clear(thash_v_t * thash) {
  memset(thash->vector, 0, thash->num_buckets * sizeof(hyp_node_t*));
  thash->num_elements = 0;
}

///Creates a new hash table
/**
 @param b Number of buckets
 @return Hash table
 The actual number of buckets is the closest prime number higher than the double of b
 */
INLINE thash_v_t *thash_create(int b) {
  thash_v_t *thash = (thash_v_t*) malloc(sizeof(thash_v_t));
  MEMTEST(thash);

  thash->num_buckets = next_prime(b);

  thash->vector = (hyp_node_t**) malloc(thash->num_buckets * sizeof(hyp_node_t*));
  MEMTEST(thash->vector);
  thash_clear(thash);

  return thash;
}

///Delete hash table
/**
 @param thash Hash table
 */
INLINE void thash_delete(thash_v_t *thash) {
  free(thash->vector);
  free(thash);
}

INLINE void thash_insert(thash_v_t *thash, hyp_node_t *node);
    //If we have the same number of elements of half of the num_buckets we duplicate the size of thash

///Duplicate the size of hash table
/**
 @param thash Hash table
 */
INLINE void thash_expand(thash_v_t *thash){

 hyp_node_t *aux;

 //We duplicate the size of thash
 thash_v_t *thash_aux=thash_create(thash->num_buckets*2);

 //We calculate the new position of all elements of hash table
 for(int i=0; i<thash->num_buckets; i++){
  if(thash->vector[i]!=NULL){
   aux=thash->vector[i];
   while(aux!=NULL){
    thash->vector[i]=aux->next;
    thash_insert(thash_aux, aux);
    aux=thash->vector[i];
   }
  }
 }

 // we swap the hash tables and delete the old one
 SWAP(*thash, *thash_aux, thash_v_t);
 thash_delete(thash_aux);

}

///Inserts a node in a hash table
/**
 @param thash Hash table
 @param node node
 */
INLINE void thash_insert(thash_v_t *thash, hyp_node_t *node) {
  //XXX: why do we need this? is b the max number of elements?
  //     it shoudn't, since we have set it as closest double prime thing
  if (thash->num_buckets != thash->num_elements) {
    thash->num_elements++;

    // find the bucket
    int f = (hyp_hash(&node->hyp) % thash->num_buckets);

    REQUIRE(f<thash->num_buckets, "Incorrect bucket (%d) in hash (max: %d) \n", f, thash->num_buckets);

    // insert the element in the front of the list
    node->next = thash->vector[f];
    thash->vector[f] = node;

    //If we have the same number of elements of half of the num_buckets we duplicate the size of thash
    if(thash->num_elements > (thash->num_buckets/2)){
     thash_expand(thash);
    }

  }
}

///finds a node in the hash table
/**
 @param thash Hash table
 @param node node
 @return the node if it has been found, NULL otherwise
 */
INLINE hyp_node_t *thash_find(const thash_v_t *thash, const hyp_t *hyp) {
  // the hash in empty, so the node doesn't exist
  if (thash->num_elements == 0)
    return NULL;

  // find the bucket
  unsigned int f = (hyp_hash(hyp) % thash->num_buckets);

  hyp_node_t *current = thash->vector[f];
  hyp_node_t *previous = NULL;

  // find the hyp in the list
  while (current != NULL && hyp_cmp(hyp, &(current->hyp)) == 0) {
    previous = current;
    current = current->next;
  }

   //XXX: why do we do this? isn't that wrong? we should change the table hash content
   //The element in the first position of the list
   if (previous != NULL && current != NULL) {
     previous->next = current->next;
     current->next = thash->vector[f];
     thash->vector[f] = current;
   }

  return current;
}

///creates a new hypothesis heap
/**
 * @param max_elems maximum number of elements (a.k.a. histogram pruning)
 * @param beam beam w.r.t. the maximum probability
 */
hyp_heap_t *hh_create(int max_elems, float beam) {
  hyp_heap_t *hh;

  hh = (hyp_heap_t *)malloc(sizeof(hyp_heap_t));
  MEMTEST(hh);

  hh->beam = beam;
  hh->max_probability = LOG_ZERO;
  hh->max_elements = max_elems;
  hh->current_limit = LOG_ZERO;

  //Create hash table
/*  hh->hash = thash_create(max_elems);*/
  hh->hash = thash_create(10000);

  hh->heap = heap_create(max_elems);

  //Pools of memory
  hh->pool=(pool_t *)malloc(sizeof(pool_t));
  MEMTEST(hh->pool);
  hh->pool->vector=(hyp_node_t *)malloc((max_elems+1)*sizeof(hyp_node_t));
  MEMTEST(hh->pool->vector);
  hh->pool->num_elements=0;

  return hh;
}

///Deletes a node from the hypothesis heap
/**
@param hh heap
@param node the node that is to be deleted from the heap
@return the deleted node. As we are using a pool, the memory of this pointer is
available to be used as memory for a new hypothesis, thus avoiding the need of allocating
new memory
*/
INLINE hyp_node_t *hh_delete_node(hyp_heap_t *hh, hyp_node_t *node) {
  //Delete state from hash table
  hyp_node_t *deleted_node = thash_delete_element(hh->hash, node);

  //XXX: what is this for?
  //*deleted_node = hh->pool->vector[hh->pool->num_elements];
  hh->pool->num_elements--;

  hh->heap->vector[1] = hh->heap->vector[hh->heap->num_elements];
  hh->heap->vector[1]->index_heap = 1;
  hh->heap->num_elements--;
  heap_heapify(hh->heap, 1);

  return deleted_node;
}

/// deletes a hypothesis heap
/**
 * @param hh the heap
 */
void hh_delete(hyp_heap_t *hh) {
  free(hh->heap->vector);
  free(hh->heap);
  thash_delete(hh->hash);
  free(hh->pool->vector);
  free(hh->pool);
  free(hh);
}


/// clears a heap. Erases all the elements
/**
 * @param hh the heap
 */
void hh_clear(hyp_heap_t *hh) {
  hh->max_probability = LOG_ZERO;
  hh->current_limit = LOG_ZERO;

  hh->pool->num_elements = 0;

  hh->heap->is_heap=0;
  thash_clear(hh->hash);
  heap_clear(hh->heap);
}

///Calculate the new state
/**
@param hh heap
@param hyp hypothesis to insert
@return New node or null if the pool is full
*/
INLINE hyp_node_t *hh_new_node(hyp_heap_t *hh, hyp_t *hyp) {

  //New state
  hyp_node_t *new_node = NULL;
  // if the heap is full, either we make room or reject the hyp

  //If the heap is full
  if (hh->heap->num_elements + 1 < hh->heap->max) {
    new_node = &(hh->pool->vector[hh->pool->num_elements++]);
    new_node->hyp = *hyp;
  }
  else {
    hh_check_heap(hh);

    if(hh->heap->max==0){
     PRINT("WARNING: The number of histogram-pruning have to be greater than 0.\n");
     exit(1);
    }

    ///if the new hypothesis is better than the worst one in the heap
    if ((hh->heap->vector[1]->hyp.probability.final) < (hyp->probability.final)) {
      // delete the worst hypothesis so that there is room for the new better one
      // and reuse the memory of the old node
      new_node = hh_delete_node(hh, hh->heap->vector[1]);
      new_node->hyp = *hyp;
    }
  }
  return new_node;
}


///Inserts a hypothesis into the heap
/**
@param hh Heap where to introduce the new state
@param hyp the new hypothesis
@return a code defined by beam_status_t which indicates if the hyp has been inserted
or the reason why it has been rejected
*/
INLINE beam_status_t hh_insert(hyp_heap_t *hh, hyp_t *hyp) {
  // the hyp passes the beam
  if (hh->current_limit <= hyp->probability.final) {

    // if it is maximum probability update variables
    if (hyp->probability.final > hh->max_probability) {
      hh->max_probability = hyp->probability.final;
      hh->current_limit = hh->max_probability - hh->beam;
    }

    // remember we are doing dynamic programming
    // so the new hypotheses reaching the same state must compete.
    // therefore, we find if the node has already been inserted
    hyp_node_t *existing_node = thash_find(hh->hash, hyp);

    // we didn't find it
    if (existing_node == NULL) {
      //Create the new state. If it is necessary, delete the lowest probability
      //node to make room for the new node
      hyp_node_t *node = hh_new_node(hh, hyp);

      // if hyp has lower prob than the min in the heap, then reject the hyp
      if (node == NULL) return REJECT_FULL;

      // insert the new node in the heap and the table
      heap_insert(hh->heap, node);
      thash_insert(hh->hash, node);
      return INSERT;
    }
    // we found it, so we make it compete with the existing hypotheses
    else {
      //If the new probability is better then replace the hypothesis
      if (hyp->probability.final > existing_node->hyp.probability.final) {
        //XXX: what about copying the remaining fields? I supose they are the same
        existing_node->hyp = *hyp;
        if (hh->heap->is_heap) {
         heap_heapify(hh->heap, existing_node->index_heap);
        }
        return REPLACE;
      } else {
        return REJECT_NO_REPLACE;
      }
    }
  }
  else { // the hyp doesn't pass the beam
    return REJECT_BEAM;
  }
}

/// returns the heap top element and erases it
/**
@param hh the heap
@return the top element of the heap
Actually, this function does not return the hyp with highest
probability, but it returns a hyp that is likely to have high probability.
What it does, is to iterate over the min-heap from back to front.
*/
INLINE hyp_t *hh_pop(hyp_heap_t *hh) {
  return &(hh->heap->vector[hh->heap->num_elements--]->hyp);
}

///returns the number of elements of the heap
/**
@param hh the heap
@return the number of elements
*/
INLINE int hh_size(const hyp_heap_t *hh) {
  return hh->heap->num_elements;
}

///returns the number of elements of the heap
/**
@param hh the heap
@return the number of elements
*/
INLINE int hh_capacity(const hyp_heap_t *hh) {
  return hh->max_elements;
}

///returns if the heap is empty or not
/**
@param hh the heap
@return true if the heap is empty, false if it is not
*/
INLINE bool hh_is_empty(const hyp_heap_t *hh) {
  return hh->heap->num_elements == 0;
}

///returns the current limit of the heap.
/// (i.e. the minimum probability needed to be inserted)
/**
@param hh the heap
@return the current limit
*/
float hh_limit(const hyp_heap_t *hh) {
  return hh->current_limit;
}

///returns the maximum probability of the heap.
/**
@param hh the heap
@return the maximum probability of the heap
*/
INLINE float hh_max(const hyp_heap_t *hh) {
  return hh->max_probability;
}

///returns the minimum probability of the heap.
/**
@param hh the heap
@return the maximum probability of the heap
*/
INLINE float hh_min(const hyp_heap_t *hh) {
  return hh->heap->vector[1]->hyp.probability.final;
}

///returns the current of the heap.
/**
@param hh the heap
@return the current beam of the heap
*/
INLINE float hh_beam(const hyp_heap_t *hh) {
  return hh->beam;
}

///Sorts the heap elements so that they can be iterated from higher probability to lower
/**
@param heap Heap
*/
// INLINE int hh_sort(hyp_heap_t *heap) {
//   heap->heap->num_elements = heap_sort(heap->heap);
//   return heap->heap->num_elements;
// }

///returns if the heap is a heap (1) o a vector (0).
/**
@param hh the heap
@return 1 if is a heap, 0 if is a vector
*/
INLINE bool hh_is_heap(const hyp_heap_t *hh) {
  return hh->heap->is_heap;
}

///prints a representation of the heap to file
/**
 * @param file
 * @param hh
 * @param grammar
 */

void hh_print(FILE *file, const hyp_heap_t *hh, const grammar_t *grammar) {
  fprintf(file, "-------------\n");
  for (int i = 1; i <= hh->heap->num_elements; i++) {
    hyp_print(file, &(hh->heap->vector[i]->hyp), grammar->vocab);
    fprintf(file, "\n");
//    const hyp_t *hyp = &(hh->heap->vector[i]->hyp);
//    char *hn = NULL;
//    if (hyp->history != NULL && hyp->history->name) {
//      vocab_symbols_to_string(hyp->history->name, grammar->vocab->in, &hn);
//    }
//    char *sn = NULL;
//    if (hyp->state != NULL && hyp->state->name != NULL) {
//      vocab_symbols_to_string(hyp->state->name, grammar->vocab->in, &sn);
//    }
//    fprintf(file, "%s, %s(%d,%d,%d) %s(%d)->%s(%d) (a=%g,l=%g,lin=%g, f=%g)",
//        extended_vocab_get_string(grammar->vocab, hyp->extended),
//        vocab_get_string(grammar->vocab->in, hyp->word),
//        hyp->state_hmm, hyp->phoneme, hyp->state_lexic,
//        hn, ((hyp->history != NULL)?hyp->history->num_state:-1), sn, hyp->state->num_state,
//        hyp->probability.acoustic, hyp->probability.lm,
//        hyp->probability.in_lm, hyp->probability.final);
//    fprintf(stderr, "\n");
//    free(hn);
//    free(sn);
  }
  fprintf(file, "-------------\n");
}

void hh_print_stats(FILE *file, const hyp_heap_t *hh, const grammar_t *grammar) {
  int *word_counts = (int *) malloc(grammar->vocab->extended->last * sizeof(int));

  for (int i = 1; i <= hh->heap->num_elements; i++) {
    if (hh->heap->vector[i]) {
      const hyp_t *hyp = &(hh->heap->vector[i]->hyp);
      word_counts[hyp->extended]++;
    }
  }

  size_t n_distinct_words = 0;
  for (int i = 0; i < grammar->vocab->extended->last; i++) {
    if (word_counts[i] > 0) n_distinct_words++;
  }
  fprintf(file, "HEAP INFO: %zu distinct words\n", n_distinct_words);

  free(word_counts);
}


