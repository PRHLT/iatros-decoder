/*
 * extended_vocab.h
 *
 *  Created on: 19-oct-2008
 *      Author: valabau
 */

#ifndef EXTENDED_VOCAB_H_
#define EXTENDED_VOCAB_H_

#include <prhlt/vocab.h>
#include <stdio.h>

/// a symbol of an extended vocabulary.
/// an extended symbol is formed by a sequence of symbols of an input language
/// and a sequence of symbols of the transtalion in an output language
typedef struct {
  const symbol_t *input;   ///< a VOCAB_NONE-terminated sequence of input symbols
  const symbol_t *output;  ///< a VOCAB_NONE-terminated sequence of output symbols
  symbol_t extended; ///< a symbol for the extended symbol
  float *scores;   ///< a set of scores of the log-linear model for this phrase
  float combined_score; ///< the combined score for features scores
} extended_symbol_t;

/// Extended vocabulary
typedef struct {
  vocab_t *in; ///< vocabulary for the input language
  vocab_t *out; ///< vocabulary for the output language
  vocab_t *extended; ///< vocabulary fo the bilingual phrases
  const char * word_delimiter; ///< separator between words of the same language in the phrase
  const char * language_delimiter; ///< separator between segments of different languages in the phrase
  extended_symbol_t *extended_symbols; ///< properties of the extended symbols
  float *weights; ///< weights of the log-linear model
  int n_weights;  ///< number of weights of the log-linear model
} extended_vocab_t;


#ifdef __cplusplus
extern "C" {
#endif


extended_vocab_t * extended_vocab_create(vocab_t *input_vocab, const char *unk, const char *word_delimiter, const char *language_delimiter);
symbol_t extended_vocab_find_symbol(const extended_vocab_t * vocab, const char * string);
symbol_t extended_vocab_insert_symbol(extended_vocab_t * vocab, const char * string, int category);
void extended_vocab_delete(extended_vocab_t * vocab);
const char *extended_vocab_get_string(const extended_vocab_t * vocab, symbol_t symbol);
const extended_symbol_t *extended_vocab_get_extended_symbol(const extended_vocab_t * vocab, symbol_t symbol);
void extended_vocab_symbols_to_string(const symbol_t *symbols, const extended_vocab_t *vocab, char **string);
void extended_vocab_separate_languages(const extended_vocab_t * vocab,
                                       symbol_t *symbols,
                                       symbol_t **input,
                                       symbol_t **output);
bool extended_vocab_symbol_is_compatible(const extended_vocab_t * vocab, symbol_t symbol,
                                         const symbol_t **input, const symbol_t **input_subwords,
                                         const symbol_t **output, const symbol_t **output_subwords);
void extended_vocab_set_closed(extended_vocab_t *vocab, bool closed);
void extended_vocab_dump(const extended_vocab_t *vocab, FILE *out);
void extended_vocab_set_weights(extended_vocab_t *vocab, int n_weights, const float *weights);
void extended_vocab_set_scores(extended_vocab_t *vocab, symbol_t symbol, const float *scores);
void extended_vocab_write(const extended_vocab_t *vocab, FILE *file);
symbol_t *extended_vocab_string_to_symbols(extended_vocab_t *vocab, char *string, const char *delimiter, int category);

#ifdef __cplusplus
}
#endif



#endif /* EXTENDED_VOCAB_H_ */
