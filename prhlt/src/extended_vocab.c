  /*
 * extended_vocab.c
 *
 *  Created on: 19-oct-2008
 *      Author: valabau
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <prhlt/utils.h>
#include <prhlt/constants.h>
#include <prhlt/trace.h>
#include <prhlt/extended_vocab.h>
#include <prhlt/vocab.h>

static const char default_word_delimiter[] = "_";
static const char default_language_delimiter[] = "|||";
static const extended_symbol_t EXTENDED_VOCAB_NONE = { NULL, NULL, VOCAB_NONE, NULL, .0 };

///Search symbol in vocab
/**
 @param vocab Vocabulary
 @param string String to search
 @return Number of string
 */
symbol_t extended_vocab_find_symbol(const extended_vocab_t * vocab, const char * string) {
  symbol_t sym = vocab_find_symbol(vocab->extended, string);
  if (vocab->extended->closed && sym == VOCAB_NONE) {
    return UNK_WORD;
  }
  else return sym;
}

///Insert symbol in vocab
/**
 @param vocab Vocabulary
 @param string String to insert
 @return Number of string
 */
symbol_t extended_vocab_insert_symbol(extended_vocab_t * vocab, const char * string, int category) {
  symbol_t symbol = vocab_find_symbol(vocab->extended, string);
  CHECK(symbol != UNK_WORD || !vocab->extended->closed,
            "Warning: trying to insert a symbol in a closed vocabulary\n");

  if (symbol == VOCAB_NONE) {
    symbol_t *input = NULL, *output = NULL;
    // tokenize extended symbol
    char copy[strlen(string + 1)];
    strcpy(copy, string);

    char *save_lang, *token_lang;

    // tokenize input string
    if (vocab->language_delimiter != NULL) {
      token_lang = tokenize(copy, vocab->language_delimiter, &save_lang);
    }
    else {
      token_lang = copy;
    }
    input = vocab_string_to_symbols(vocab->in, token_lang, vocab->word_delimiter, category);

    if (input == NULL || input[0] == VOCAB_NONE) {
      CHECK(input != NULL && input[0] != VOCAB_NONE, "ERROR: words with empty input string are not allowed '%s'\n", string);
      free(input);
      return VOCAB_NONE;
    }

    if (vocab->language_delimiter != NULL) {
      // tokenize output string
      token_lang = tokenize(NULL, vocab->language_delimiter, &save_lang);
      output = vocab_string_to_symbols(vocab->out, token_lang, vocab->word_delimiter, category);
    }
    else {
      output = vocab_string_to_symbols(vocab->out, "", vocab->word_delimiter, category);
    }

    symbol = vocab_insert_symbol(vocab->extended, string, category);
    if (symbol != VOCAB_NONE) {
      vocab->extended_symbols = (extended_symbol_t *) realloc(vocab->extended_symbols, (vocab->extended->last) * sizeof(extended_symbol_t));
      MEMTEST(vocab->extended_symbols);
      vocab->extended_symbols[symbol].extended = symbol;
      vocab->extended_symbols[symbol].input = (const symbol_t *) input;
      vocab->extended_symbols[symbol].output = (const symbol_t *) output;
      vocab->extended_symbols[symbol].scores = NULL;
      vocab->extended_symbols[symbol].combined_score = .0;

    }
  } else if (category != -1) {
    vocab->extended->info[symbol].category = category;
    symbol_t symbol_in = vocab_find_symbol(vocab->in, string);
    vocab->in->info[symbol_in].category = category;
  }
  return symbol;
}

///Get string associated with symbol
/**
 @param vocab Vocabulary
 @param symbol Symbol to convert in string
 @return String
 */
const extended_symbol_t *extended_vocab_get_extended_symbol(const extended_vocab_t * vocab, symbol_t symbol) {
  if (symbol >= FIRST_SYMBOL && symbol < vocab->extended->last) {
    return &(vocab->extended_symbols[symbol]);
  } else
    return &EXTENDED_VOCAB_NONE;
}

///Get string associated with symbol
/**
 @param vocab Vocabulary
 @param symbol Symbol to convert in string
 @return String
 */
const char *extended_vocab_get_string(const extended_vocab_t * vocab, symbol_t symbol) {
  return vocab_get_string(vocab->extended, symbol);
}

///Convert an array of symbols to a string and append it to an existing string.
/// If the string is NULL, then creates a new string.
/**
 @param symbols an array on VOCAB_NONE terminated symbols
 @param vocab a vocabulary of symbols
 @param s a string to which append the converted string. It must be a valid string or NULL
 */
void extended_vocab_symbols_to_string(const symbol_t *symbols, const extended_vocab_t *vocab, char **string) {
  symbol_t input[MAX_LINE] = { VOCAB_NONE };
  symbol_t output[MAX_LINE] = { VOCAB_NONE };

  if (symbols == NULL) return;

  int i = 0;
  while (symbols[i] != VOCAB_NONE) {
    const extended_symbol_t *extended_symbol = extended_vocab_get_extended_symbol(vocab, symbols[i]);
    symcat(input, extended_symbol->input);
    symcat(output, extended_symbol->output);
    i++;
  }

  vocab_symbols_to_string(input, vocab->in, string);
  if (vocab->language_delimiter != NULL) {
    *string = (char *) realloc(*string, sizeof(char) * (strlen(*string) + strlen(vocab->language_delimiter) + 3));
    MEMTEST(*string);
    if (symlen(output) > 0) {
      strcat(*string, " ");
      strcat(*string, vocab->language_delimiter);
      strcat(*string, " ");
      vocab_symbols_to_string(output, vocab->out, string);
    }
  }
}

symbol_t *extended_vocab_string_to_symbols(extended_vocab_t *vocab, char *string, const char *delimiter, int category) {
  int index = 0;
  symbol_t *result = NULL;

  char *saveptr = (char *) string;
  char *token = tokenize((char *) string, delimiter, &saveptr);

  while (token != NULL) {
    result = (symbol_t *) realloc(result, (index + 1) * sizeof(symbol_t));
    MEMTEST(result);
    if (vocab->extended->closed) {
      result[index++] = extended_vocab_find_symbol(vocab, token);
    }
    else {
      result[index++] = extended_vocab_insert_symbol(vocab, token, category);
    }
    token = tokenize(NULL, delimiter, &saveptr);
  }
  result = (symbol_t *) realloc(result, (index + 1) * sizeof(symbol_t));
  result[index] = VOCAB_NONE;

  return result;
}



///Create vocabulary of hsize size and return pointer to it
/**
 @param hsize Size of vocabulary
 @return Vocabulary
 */
extended_vocab_t * extended_vocab_create(vocab_t *input_vocab, const char *unk, const char *word_delimiter, const char *language_delimiter) {
  extended_vocab_t * vocab;

  vocab = (extended_vocab_t *) malloc(sizeof(extended_vocab_t));

//  if (word_delimiter == NULL)
//    word_delimiter = default_word_delimiter;
//  if (language_delimiter == NULL)
//    language_delimiter = default_language_delimiter;

  if (word_delimiter != NULL) {
    vocab->word_delimiter = (const char *) malloc(strlen(word_delimiter) + 1);
    strcpy((char *) vocab->word_delimiter, word_delimiter);
  }
  else {
    vocab->word_delimiter = NULL;
  }

  if (language_delimiter != NULL) {
    vocab->language_delimiter = (const char *) malloc(strlen(language_delimiter) + 1);
    strcpy((char *) vocab->language_delimiter, language_delimiter);
  }
  else {
    vocab->language_delimiter = NULL;
  }

  vocab->extended_symbols = NULL;

  vocab->in = input_vocab;
  bool input_has_unk = input_vocab->has_unk;
  vocab->out = vocab_create(vocab->in->hash->hsize, unk);
  vocab->extended = vocab_create(vocab->in->hash->hsize, NULL);

  vocab->n_weights = 0;
  vocab->weights = NULL;

  if (unk != NULL) {
    char extended_unk[MAX_LINE];
    strcpy(extended_unk, unk);
    if (language_delimiter != NULL) {
      strcat(extended_unk, language_delimiter);
      strcat(extended_unk, unk);
    }

    extended_vocab_insert_symbol(vocab, extended_unk, CATEGORY_NONE);
    vocab->extended->unk_word = (char *) malloc((strlen(extended_unk) + 1) * sizeof(char));
    strcpy(vocab->extended->unk_word, extended_unk);
    vocab->out->has_unk = true;
  }
  else {
    vocab->out->has_unk = false;
  }
  input_vocab->has_unk = input_has_unk;

  return vocab;
}

///Free all members of the structure
/**
 @param vocab Vocabulary
 */
void extended_vocab_delete(extended_vocab_t * vocab) {
  for (int i = 0; i < vocab->extended->last; i++) {
    free((void *) vocab->extended_symbols[i].input);
    free((void *) vocab->extended_symbols[i].output);
    free((void *) vocab->extended_symbols[i].scores);
  }
  free(vocab->extended_symbols);

  vocab_delete(vocab->in);
  vocab_delete(vocab->out);
  vocab_delete(vocab->extended);
  free(vocab->weights);

  free((char *) vocab->word_delimiter);
  free((char *) vocab->language_delimiter);

  free(vocab);
}

void extended_vocab_separate_languages(const extended_vocab_t * vocab,
                                       symbol_t *symbols,
                                       symbol_t **input,
                                       symbol_t **output)
{
  int input_len = 0, output_len = 0;
  symbol_t *ptr = symbols;
  while (*ptr != VOCAB_NONE) {
    input_len += symlen(vocab->extended_symbols[*ptr].input);
    output_len += symlen(vocab->extended_symbols[*ptr].output);
    ptr++;
  }
  *input  = (symbol_t *) malloc((input_len  +1)  * sizeof(symbol_t));
  (*input)[0] = VOCAB_NONE;
  *output = (symbol_t *) malloc((output_len +1 ) * sizeof(symbol_t));
  (*output)[0] = VOCAB_NONE;

  ptr = symbols;
  while (*ptr != VOCAB_NONE) {
    symcat(*input, vocab->extended_symbols[*ptr].input);
    symcat(*output, vocab->extended_symbols[*ptr].output);
    ptr++;
  }
}


bool extended_vocab_symbol_is_compatible(const extended_vocab_t * vocab, symbol_t symbol,
                                         const symbol_t **input, const symbol_t **input_subwords,
                                         const symbol_t **output, const symbol_t **output_subwords)
{
  {
    const symbol_t *s_input = vocab->extended_symbols[symbol].input;
    const symbol_t *input_ptr = *input;
    if (input != NULL && *input != NULL) {
      while (*s_input != VOCAB_NONE && *input_ptr != VOCAB_NONE) {
        if (*s_input != *input_ptr) return false;
        s_input++;
        input_ptr++;
      }
    }

    if (input_subwords != NULL && *input_subwords != NULL && *s_input != VOCAB_NONE) {
      size_t n_subwords = symlen(*input_subwords);
      symbol_t *ret = (symbol_t *) bsearch(s_input, *input_subwords, n_subwords, sizeof(symbol_t), search_symcmp);
      if (ret == NULL)
        return false;
      else
        *input_subwords = ret;
    }
    *input = input_ptr;
  }

  {
    const symbol_t *s_output = vocab->extended_symbols[symbol].output;
    const symbol_t *output_ptr = *output;
    if (output != NULL && *output != NULL) {
      while (*s_output != VOCAB_NONE && *output_ptr != VOCAB_NONE) {
        if (*s_output != *output_ptr) return false;
        s_output++;
        output_ptr++;
      }
    }

    if (output_subwords != NULL && *output_subwords != NULL && *s_output != VOCAB_NONE) {
      size_t n_subwords = symlen(*output_subwords);
      symbol_t *ret = (symbol_t *) bsearch(s_output, *output_subwords, n_subwords, sizeof(symbol_t), search_symcmp);
      if (ret == NULL)
        return false;
      else
        *output_subwords = ret;
    }
    *output = output_ptr;
  }
  return true;
}

void extended_vocab_set_closed(extended_vocab_t *vocab, bool closed) {
  vocab_set_closed(vocab->in, closed);
  vocab_set_closed(vocab->out, closed);
  vocab_set_closed(vocab->extended, closed);
}

void extended_vocab_dump(const extended_vocab_t *vocab, FILE *out) {
  for (int w = 0; w < vocab->extended->last; w++) {
    fprintf(out, "%s(%d) ||| ", extended_vocab_get_string(vocab, w), w);
    symbol_t *ptr = (symbol_t *)vocab->extended_symbols[w].input;
    while (ptr != NULL && *ptr != VOCAB_NONE) {
      fprintf(out, "%s(%d) ", vocab_get_string(vocab->in, *ptr), *ptr);
      ptr++;
    }
    fprintf(out, "||| ");
    ptr = (symbol_t *)vocab->extended_symbols[w].output;
    while (ptr != NULL && *ptr != VOCAB_NONE) {
      fprintf(out, "%s(%d) ", vocab_get_string(vocab->out, *ptr), *ptr);
      ptr++;
    }
    fprintf(out, "\n");
  }
}

/** sets the weights for the log-linear model
 * @param vocab a extended vocabulary
 * @param
 * Note: Once the weights have been set, they cannot be changed again.
 */
void extended_vocab_set_weights(extended_vocab_t *vocab, int n_weights, const float *weights) {
  if (vocab->n_weights > 0) {
    PRINT("Warning: Attempting to reset weights");
  }
  else {
    vocab->n_weights = n_weights;
    if (n_weights > 0) {
      vocab->weights = (float *) malloc(n_weights * sizeof(float));
      memcpy(vocab->weights, weights, n_weights * sizeof(float));
    }
    else {
      vocab->weights = NULL;
    }
  }
}

/** sets the feature scores for the log-linear model
 * @param vocab a extended vocabulary
 * @param
 * Note: Once the weights have been set, they cannot be changed again.
 */
void extended_vocab_set_scores(extended_vocab_t *vocab, symbol_t symbol, const float *scores) {
  if (vocab->n_weights > 0 && scores != NULL) {
    extended_symbol_t *es = &(vocab->extended_symbols[symbol]);
    es->scores = (float *) realloc(es->scores, vocab->n_weights * sizeof(float));
    memcpy(es->scores, scores, vocab->n_weights * sizeof(float));
    es->combined_score = .0;
    for (int w = 0; w < vocab->n_weights; w++) {
      es->combined_score += vocab->weights[w] * es->scores[w];
    }
    if (! (es->combined_score > LOG_ZERO) ) {
      fprintf(stderr, "WARNING: non-infinite score in phrasetable\n");
    }
  }
}

void extended_vocab_write(const extended_vocab_t *vocab, FILE *file) {
  fprintf(file, "Input vocab\n");
  fprintf(file, "-----------\n");
  vocab_write(vocab->in, file);
  fprintf(file, "Output vocab\n");
  fprintf(file, "-----------\n");
  vocab_write(vocab->out, file);
  fprintf(file, "Extended vocab\n");
  fprintf(file, "-----------\n");
  vocab_write(vocab->extended, file);
}
