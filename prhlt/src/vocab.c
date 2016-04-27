/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <prhlt/constants.h>
#include <prhlt/trace.h>
#include <prhlt/vocab.h>
#include <prhlt/utils.h>

const char unk_word[] = "<unk>";

///Search symbol in vocab
/**
 @param vocab Vocabulary
 @param string String to search
 @return Number of string
 */
symbol_t vocab_find_symbol(const vocab_t * vocab, const char * string) {
  symbol_t symbol = (symbol_t) hash_search(string, strlen(string) * sizeof(char), vocab->hash);
  if (vocab->closed && symbol == VOCAB_NONE) {
    return UNK_WORD;
  }
  else return symbol;
}

///Insert symbol in vocab
/**
 @param vocab Vocabulary
 @param string String to insert
 @return Number of string
 */
symbol_t vocab_insert_symbol(vocab_t * vocab, const char * string, int category) {

//  REQUIRE(string != NULL && strcmp(string, "") != 0, "Invalid word\n");
  symbol_t symbol = UNK_WORD;
  if (vocab->unk_word != NULL && strcmp(string, vocab->unk_word) == 0) {
    vocab->has_unk = true;
  }
  else {
    symbol = vocab_find_symbol(vocab, string);

    CHECK(symbol != UNK_WORD || !vocab->closed, "Warning: trying to insert a symbol in a closed vocabulary\n");

    if (category != CATEGORY_NONE && symbol != VOCAB_NONE) {
      vocab->info[symbol].category = category;
    }

    if (symbol == VOCAB_NONE) {
      symbol = (symbol_t) hash_insert(string, strlen(string) * sizeof(char), (void *) vocab->last, vocab->hash);
      if (symbol != VOCAB_NONE) {
        vocab->info = (vocab_info_t *) realloc(vocab->info, sizeof(vocab_info_t) * (vocab->last + 1));
        vocab->info[vocab->last].strings = (char *) malloc(strlen(string) + 1);
        strcpy((char *) vocab->info[vocab->last].strings, string);
        vocab->info[vocab->last].category = category;
        vocab->last++;
      }
    }
  }
  return symbol;
}

///Get string associated with symbol
/**
 @param vocab Vocabulary
 @param symbol Symbol to convert in string
 @return String
 */
const char *vocab_get_string(const vocab_t * vocab, symbol_t symbol) {
  if (symbol >= FIRST_SYMBOL && symbol < vocab->last) {
    return vocab->info[symbol].strings;
  }
  return NULL;
}

symbol_t *vocab_string_to_symbols(vocab_t *vocab, char *string, const char *delimiter, int category) {
  int index = 0;
  symbol_t *result = NULL;

  char *saveptr = (char *) string;
  char *token = tokenize((char *) string, delimiter, &saveptr);

  while (token != NULL) {
    result = (symbol_t *) realloc(result, (index + 1) * sizeof(symbol_t));
    MEMTEST(result);
    if (vocab->closed) {
      result[index++] = vocab_find_symbol(vocab, token);
    }
    else {
      result[index++] = vocab_insert_symbol(vocab, token, category);
    }
    token = tokenize(NULL, delimiter, &saveptr);
  }
  result = (symbol_t *) realloc(result, (index + 1) * sizeof(symbol_t));
  result[index] = VOCAB_NONE;

  return result;
}

///Convert an array of symbols to a string and append it to an existing string.
/// If the string is NULL, then creates a new string.
/**
 @param symbols an array on VOCAB_NONE terminated symbols
 @param vocab a vocabulary of symbols
 @param s a string to which append the converted string. It must be a valid string or NULL
 */
void vocab_symbols_to_string(const symbol_t *symbols, const vocab_t *vocab, char **s) {
  char aux[MAX_LINE] = "";

  int i = 0;
  if (symbols[i] != VOCAB_NONE) {
    strcpy(aux, vocab_get_string(vocab, symbols[i]));

    i++;
    while (symbols[i] != VOCAB_NONE) {
      strcat(aux, " ");
      strcat(aux, vocab_get_string(vocab, symbols[i]));
      i++;
    }
  }

  if (*s != NULL) {
    *s = (char *) realloc(*s, sizeof(char) * (strlen(*s) + strlen(aux) + 1));
    MEMTEST(*s);
    strcat((*s), aux);
  } else {
    *s = (char *) malloc(sizeof(char) * (strlen(aux) + 1));
    MEMTEST(*s);
    strcpy((*s), aux);
  }
}

INLINE size_t symlen(const symbol_t *symbols) {
  size_t len = 0;
  while (*symbols != VOCAB_NONE) {
    symbols++;
    len++;
  }
  return len;
}

symbol_t *symcat(symbol_t *dest, const symbol_t *src) {
  symbol_t *result = dest;
  while (*result != VOCAB_NONE)
    result++;
  while (*src != VOCAB_NONE) {
    *result = *src;
    result++;
    src++;
  }
  *result = VOCAB_NONE;
  return dest;
}

///Compare the history of two states
/**
 @param h1 History
 @param h2 History
 @return If the same history then return 0
 */
INLINE int symcmp(const symbol_t *s1, const symbol_t *s2) {
  int i = 0;
  while ((s1[i] == s2[i]) && (s1[i] != VOCAB_NONE))
    i++;
  return (s1[i] - s2[i]);
}

///Find the longest prefix match between a reference and a candidate
/**
 @param reference the reference to which we are comparing
 @param candidate the candidate
 @return the length of the prefix
 */
INLINE int sym_longest_prefix_match(const symbol_t *reference, const symbol_t *candidate) {
  int i = 0;
  while ((reference[i] == candidate[i]) && (reference[i] != VOCAB_NONE))
    i++;
  return i;
}

INLINE symbol_t *symcpy(symbol_t* dest, const symbol_t* src) {
  symbol_t *result = dest;
  while (*src != VOCAB_NONE) {
    *result = *src;
    result++;
    src++;
  }
  *result = VOCAB_NONE;
  return dest;
}

INLINE symbol_t *symdup(const symbol_t *src) {
  symbol_t *dest = (symbol_t *) malloc((symlen(src) + 1) * sizeof(symbol_t));
  MEMTEST(dest);
  return symcpy(dest, src);
}

///Compare to symbols for unix search and sort functions
/**
 @param s1 a symbol pointer
 @param s2 a symbol pointer
 @return If the same history then return 0
 */
INLINE int search_symcmp(const void *s1, const void *s2) {
  return (*(symbol_t *) s1 - *(symbol_t *) s2);
}

///Create vocabulary of hsize size and return pointer to it
/**
 * @param hsize Size of vocabulary
 * @param unk the unknown word or NULL
 * @return Vocabulary
 * In order to have the unk word assigned to the UNK_WORD constant (specially useful for grammars)
 * the unk word must be passed as argument in the construction. If the unk word is not necessary,
 * NULL must be passed
 */
vocab_t * vocab_create(int hsize, const char *unk) {
  vocab_t * vocab;

  vocab = (vocab_t *) malloc(sizeof(vocab_t));
  vocab->hash = hash_create(hsize, (void *) VOCAB_NONE);
  /*  vocab->strings = NULL;*/
  vocab->info = NULL;
  vocab->last = FIRST_SYMBOL;
  vocab->unk_word = NULL;
  vocab_set_closed(vocab, false);

  if (unk != NULL) {
    vocab_insert_symbol(vocab, unk, CATEGORY_NONE);
    vocab->unk_word = (char *) malloc((strlen(unk) + 1) * sizeof(char));
    strcpy(vocab->unk_word, unk);
    vocab->has_unk = true;
  }
  else {
    vocab->has_unk = false;
  }
  return vocab;
}

///Free all members of the structure
/**
 @param vocab Vocabulary
 */
void vocab_delete(vocab_t * vocab) {
  hash_delete(vocab->hash, 0);
  free(vocab->unk_word);
  for (int i = 0; i < vocab->last; i++)
    free((char *) vocab->info[i].strings);
  free(vocab->info);
  //   for (int i = 0; i < vocab->last; i++) free((char *)vocab->strings[i]);
  //   free(vocab->strings);
  free(vocab);
}

void vocab_set_closed(vocab_t *vocab, bool closed) {
  vocab->closed = closed;
}

void vocab_write(const vocab_t *vocab, FILE *file) {
  for (int i = 0; i < vocab->last; i++) {
    fprintf(file, "%s = %d", vocab->info[i].strings, i);
    if (vocab->info[i].category != CATEGORY_NONE) {
      fprintf(file, ", cat = %d", vocab->info[i].category);
    }
    fprintf(file, "\n");
  }
}
