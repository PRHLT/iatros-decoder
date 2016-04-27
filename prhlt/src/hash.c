/***************************************************************************
                          hash.c  -  description
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"


/***************************************************************************/
/*************************CONSTANTS*****************************************/
/***************************************************************************/

#define INITVAL 23312819
#define HASHSIZE(n) ((ub4)1<<(n))
#define HASHMASK(n) (HASHSIZE(n)-1)



/***************************************************************************/
/*******************************FUNCTIONS***********************************/
/***************************************************************************/

/*
--------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.
For every delta with one or two bits set, and the deltas of all three
  high bits or all three low bits, whether the original value of a,b,c
  is almost all zero or is uniformly distributed,
* If mix() is run forward or backward, at least 32 bits in a,b,c
  have at least 1/4 probability of changing.
y* If mix() is run forward, every bit of c will change between 1/3 and
  2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
mix() was built out of 36 single-cycle latency instructions in a
  structure that could supported 2x parallelism, like so:
      a -= b;
      a -= c; x = (c>>13);
      b -= c; a ^= x;
      b -= a; x = (a<<8);
      c -= a; b ^= x;
      c -= b; x = (b>>13);
      ...
  Unfortunately, superscalar Pentiums and Sparcs can't take advantage
  of that parallelism.  They've also turned some of those single-cycle
  latency instructions into multi-cycle latency instructions.  Still,
  this is the fastest good hash I could find.  There were about 2^^68
  to choose from.  I only looked at a billion or so.
--------------------------------------------------------------------
*/
#define mix(a,b,c){ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

/*
--------------------------------------------------------------------
hash() -- hash a variable-length key into a 32-bit value
  k       : the key (the unaligned variable-length array of bytes)
  len     : the length of the key, counting by bytes
  initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Every 1-bit and 2-bit delta achieves avalanche.
About 6*len+35 instructions.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have HASHSIZE(10) elements.

If you are hashing n strings (ub1 **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);

By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

See http://burtleburtle.net/bob/hash/evahash.html
Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
--------------------------------------------------------------------
*/



ub1 *keydup(const ub1 *key, size_t len) {
  void *new_key = malloc(len);
  if (new_key == NULL) return NULL;
  return (ub1 *) memcpy(new_key, key, len);
}

inline int keys_are_equal(const void *k1, int l1, const void *k2, int l2) {
  return l1 == l2 && memcmp(k1, k2, l1) == 0;
}

/***************************************************************************/
/***************************************************************************/

/* Check if memory was allocated to pointer ptr */
void mt(void *ptr){
  if (!ptr) {
    fprintf(stderr,"%s: out of memory\n",__FILE__);
    exit(EXIT_FAILURE);}
}

/***************************************************************************/
/***************************************************************************/

/* the key */ /* key length */ /* the previous hash, or an arbitrary value */
inline ub4 hf( k, length, initval)
register const ub1 *k;        /* the key */
register ub4  length;   /* the length of the key */
register ub4  initval;  /* the previous hash, or an arbitrary value */
{
   register ub4 a,b,c,len;

   /* Set up the internal state */
   len = length;
   a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
   c = initval;         /* the previous hash value */

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      a += (k[0] +((ub4)k[1]<<8) +((ub4)k[2]<<16) +((ub4)k[3]<<24));
      b += (k[4] +((ub4)k[5]<<8) +((ub4)k[6]<<16) +((ub4)k[7]<<24));
      c += (k[8] +((ub4)k[9]<<8) +((ub4)k[10]<<16)+((ub4)k[11]<<24));
      mix(a,b,c);
      k += 12; len -= 12;
   }

   /*------------------------------------- handle the last 11 bytes */
   c += length;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((ub4)k[10]<<24);
   case 10: c+=((ub4)k[9]<<16);
   case 9 : c+=((ub4)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((ub4)k[7]<<24);
   case 7 : b+=((ub4)k[6]<<16);
   case 6 : b+=((ub4)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((ub4)k[3]<<24);
   case 3 : a+=((ub4)k[2]<<16);
   case 2 : a+=((ub4)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   mix(a,b,c);
   /*-------------------------------------------- report the result */
   return c;
}

inline ub4 hash_generic(const void *k, int length) {
  return hf((const ub1 *) k, length, 23312819);
}

/***************************************************************************/

hash_t *hash_create(int hsize, const void *null){
  int i;
  hash_t *ht;

  mt(ht=(hash_t *)malloc(sizeof(hash_t)));
  ht->hsize=hsize;
  mt(ht->htable=(hash_element_t **)malloc(hsize*sizeof(hash_element_t *)));
  for(i=0;i<hsize;i++) ht->htable[i]=NULL;
  for(i=0;hsize!=0;i++) hsize=hsize>>1;
  ht->ubits=i-1;
  ht->null=(void *)null;
  return ht;
}

/***************************************************************************/
/***************************************************************************/

inline ub4 h(const ub1 *key, int key_length, int ubits){
  return hf(key, key_length, INITVAL) & HASHMASK(ubits);
}

/***************************************************************************/
/***************************************************************************/

void *hash_search(const void *_key, int key_length, const hash_t *ht){
  hash_element_t *p;
  const ub1 *key = (const ub1 *) _key;

  for (p=ht->htable[h(key,key_length,ht->ubits)];p!=NULL;p=p->p)
    if (keys_are_equal(key, key_length, p->key, p->key_length)) return p->data;
  return ht->null;
}

/***************************************************************************/
/***************************************************************************/

void *hash_insert(const void *_key, int key_length, void *data, hash_t *ht){
  hash_element_t *p;
  ub4 hkey;
  const ub1 *key = (const ub1 *) _key;

  hkey=h(key,key_length,ht->ubits);
  for (p=ht->htable[hkey];p!=NULL;p=p->p)
    /* We have already inserted this element */
    if (keys_are_equal(key, key_length, p->key, p->key_length)) return p->data;

  /* New element in hash table */
  mt(p=(hash_element_t *)malloc(sizeof(hash_element_t)));
  p->key = keydup(key, key_length); p->key_length = key_length; p->data=data;
  /* New entry in hash table */
  p->p=ht->htable[hkey]; ht->htable[hkey]=p;
  return data;
}

/***************************************************************************/
/***************************************************************************/

void *hash_remove(const void *_key, int key_length, hash_t *ht){
  hash_element_t *p,*pp;
  ub4 hkey;
  const ub1 *key = (const ub1 *) _key;
  void *data;

  hkey=h(key,key_length,ht->ubits);
  for (pp=p=ht->htable[hkey];p!=NULL;pp=p,p=p->p)
    if (keys_are_equal(key, key_length, p->key, p->key_length)){
      data=p->data; pp->p=p->p;
      if(pp==p) ht->htable[hkey]=p->p;/*one-element list*/
      free(p->key); free(p); return data;}
   return ht->null;
}

/***************************************************************************/
/***************************************************************************/

void hash_clear(hash_t *ht, int is_pointer) {
  int i;
  hash_element_t *p, *pp;

  for (i = 0; i < ht->hsize; i++) {
    for (p = ht->htable[i]; p != NULL; pp = p, p = p->p, free(pp)) {
      free(p->key);
      if (is_pointer) free(p->data);
    }
    ht->htable[i] = NULL;
  }
}


void hash_delete(hash_t *ht, int is_pointer){

  int i;
  hash_element_t *p,*pp;

  for(i=0;i<ht->hsize;i++)
    for (p=ht->htable[i];p!=NULL;pp=p,p=p->p,free(pp))
      {free(p->key);if(is_pointer) free(p->data);}
  free(ht->htable);
  free(ht);
}

