/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#ifndef __VOCAB_H
#define __VOCAB_H

#include <prhlt/hash.h>
#include <prhlt/utils.h>
#include <stdio.h>

/// a numerical representation of a textual symbol
typedef enum {
  VOCAB_NONE = -1, UNK_WORD = 0, FIRST_SYMBOL = 0
} symbol_t;
extern const char unk_word[];

/// category identifier
typedef enum { CATEGORY_NONE = -1 } category_id_t;

/// Structure to save information of the vocabulary
typedef struct vocab_info {
  int category; ///<
  char * strings; ///< String with the name of word
} vocab_info_t;

/// Structure to save the vocabulary
typedef struct {
  symbol_t last; ///< Number of elements
  hash_t * hash; ///< Hash table to save the words as numbers
  vocab_info_t * info; ///< Vector with the information of the vocabulary
  char *unk_word; ///< the unknown word
  bool has_unk; ///< if the dictionary has explicitly the word unk
  bool closed; ///< if closed == true, the insertion of new words is forbidden
} vocab_t;

#ifdef __cplusplus
extern "C" {
#endif


INLINE size_t symlen(const symbol_t *symbols);
symbol_t *symcat(symbol_t *dest, const symbol_t *src);
INLINE int symcmp(const symbol_t *s1, const symbol_t *s2);
INLINE symbol_t *symcpy(symbol_t *dest, const symbol_t *src);
INLINE symbol_t *symdup(const symbol_t *src);
INLINE int sym_longest_prefix_match(const symbol_t *reference, const symbol_t *candidate);
INLINE int search_symcmp(const void *s1, const void *s2);

vocab_t * vocab_create(int hsize, const char *unk);
symbol_t vocab_find_symbol(const vocab_t * vocab, const char * string);
symbol_t vocab_insert_symbol(vocab_t * vocab, const char * string, int category);
void vocab_delete(vocab_t * vocab);
const char *vocab_get_string(const vocab_t * vocab, symbol_t symbol);
void vocab_symbols_to_string(const symbol_t *s3, const vocab_t *vocab, char **s);
symbol_t *vocab_string_to_symbols(vocab_t *vocab, char *string, const char *delimiter, int category);
void vocab_set_closed(vocab_t *vocab, bool closed);
void vocab_write(const vocab_t *vocab, FILE *file);

#ifdef __cplusplus
}
#endif


#endif //__VOCAB_H
