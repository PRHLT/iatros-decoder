/*
 * vector.c
 *
 *  Created on: 02-ene-2009
 *      Author: valabau
 */

#include <prhlt/vector.h>
#include <prhlt/trace.h>

/// Creates an empty vector
/**
 * @return a vector
 */
vector_t *vector_create() {
  vector_t *vector = (vector_t *)calloc(1, sizeof(vector_t));
  MEMTEST(vector);
  return vector;
}

/// Deletes a vector
/**
 * @param vector the vector to be deleted
 */
void vector_delete(vector_t *vector) {
  free(vector->data);
  free(vector);
}

/// Inserts a new element at the end
/**
 * @param vector a vector
 * @param elem the element to be appended at the end of the vector
 */
INLINE void vector_append(vector_t *vector, void *elem) {
  if (vector->n_elems >= vector->capacity) {
    vector_reserve(vector, vector->n_elems + 1);
  }
  vector->data[vector->n_elems++] = elem;
}

/// Erases all of the elements
/**
 * @param vector the vector to be cleared
 */
void vector_clear(vector_t *vector) {
  vector->n_elems = 0;
}


/// Allocates memory
/**
 * @param vector the vector
 * @param size the new capacity of the vector
 *
 * Note: If size is less than or equal to capacity, this call has no effect. Otherwise,
 *       it is a request for allocation of additional memory. If the request is successful,
 *       then capacity is greater than or equal to size; otherwise, capacity is unchanged.
 *       In either case, n_elems is unchanged. (from http://www.sgi.com/tech/stl/Vector.html)
 */

INLINE void vector_reserve(vector_t *vector, size_t size) {
  if (size > vector->capacity) {
    vector->capacity = size;
    vector->data = (void **)realloc(vector->data, (vector->capacity) *sizeof(void *));
    MEMTEST(vector->data);
  }
}

/// Inserts or erases elements at the end such that the size becomes n
/**
 * @param vector a vector
 * @param size the new size of the vector
 *
 * Note: if size > n_elems, the new elements will be set to NULL. If size < elements
 *       the elements above size will be lost and thre will be a memory leak.
 *       If size == n_elements the vector will shrink its capacity to the minimum
 */
void vector_resize(vector_t *vector, size_t size) {
  vector_reserve(vector, size);
  if (vector->n_elems >= size) {
    vector->n_elems = size;
  }
  else {
    for (size_t i = vector->n_elems; i < size; i++) {
      vector->data[i] = NULL;
    }
    vector->n_elems = size;
  }
}

/// Reduces the vector capacity to fit the number of elements
/**
 * @param vector a vector
 */
void vector_shrink(vector_t *vector) {
  vector_reserve(vector, vector->n_elems);
}

/// Sorts the vector using the compare function cmp
/**
 * @param vector a vector
 * @param cmp a compare function
 */
void vector_qsort(vector_t *vector, cmp_fn cmp) {
  qsort(vector->data, vector->n_elems, sizeof(void *), cmp);
}

/// Find an element in a sorted vector
/**
 * @param vector a vector sorted using cmp
 * @param cmp a compare function
 */
void *vector_bsearch(vector_t *vector, void *key, cmp_fn cmp) {
  void **ret = (void **)bsearch(&key, vector->data, vector->n_elems, sizeof(void *), cmp);
  if (ret == NULL) return NULL;
  else return *ret;
}
