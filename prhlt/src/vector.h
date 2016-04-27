/*
 * vector.h
 *
 *  Created on: 02-ene-2009
 *      Author: valabau
 */

#ifndef VECTOR_H_
#define VECTOR_H_

#include <stdlib.h>
#include <prhlt/utils.h>

typedef struct {
  size_t n_elems;
  size_t capacity;
  void **data;
} vector_t;

vector_t *vector_create();
void vector_delete(vector_t *vector);
void vector_append(vector_t *vector, void *elem);
void vector_clear(vector_t *vector);
void vector_reserve(vector_t *vector, size_t size);
void vector_resize(vector_t *vector, size_t size);
void vector_shrink(vector_t *vector);
void vector_qsort(vector_t *vector, cmp_fn cmp);
void *vector_bsearch(vector_t *vector, void *key, cmp_fn cmp);

#endif /* VECTOR_H_ */
