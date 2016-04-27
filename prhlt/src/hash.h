/***************************************************************************
                          hash.h  -  description
                             -------------------
    begin                : dom nov 21 2004
    copyright            : (C) 2004 by Jorge Civera Saiz
    email                : jorcisai@iti.upv.es
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MEMHASH_INCLUDED
#define MEMHASH_INCLUDED

typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;   /* unsigned 1-byte quantities */

typedef struct he_type {
  ub1 *key;
  int key_length;
  void *data;
  struct he_type *p;
} hash_element_t;

struct h_type {
  hash_element_t **htable;
  int hsize;
  int ubits;
  void *null;
};


struct h_type;
typedef  struct h_type hash_t;

/* Create hash table of hsize size and return pointer to it */
hash_t *hash_create(int hsize, const void *null);

/* Search element with key and key_length and return pointer to data */
void *hash_search(const void *key, int key_length, const hash_t *ht);

/* Insert element in ht pointed by data with key and key_length */
void *hash_insert(const void *key, int key_length, void *data, hash_t *ht);

/* Remove element with key and key_length and return pointer to data */
void *hash_remove(const void *key, int key_length, hash_t *ht);

/* Remove all elements from the hash tables and free contained elements if is_pointer */
void hash_clear(hash_t *ht, int is_pointer);

/* Destroy hash tables and free contained elements if is_pointer*/
void hash_delete(hash_t *ht, int is_pointer);

ub4 hash_generic(const void *k, int length);
#endif
