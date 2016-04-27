/*
 * hypothesis.h
 *
 *  Created on: 23-sep-2008
 *      Author: valabau
 */

#ifndef HYPOTHESIS_H_
#define HYPOTHESIS_H_

#include <iatros/probability.h>
#include <iatros/grammar.h>

/// State viterbi. State in heap in each t
typedef struct {
  int index; ///< Index in the lattice
  state_grammar_t * history; ///< Number of source state of the grammar
  state_grammar_t * state;   ///< Number of target state of the grammar
  state_grammar_t * history_category; ///< Number of source state of the grammar in the aef with the category
  state_grammar_t * state_in; ///< Number of target state of the input grammar
  state_grammar_t * state_out; ///< Number of target state of the output grammar
  symbol_t word;      ///< The current word. It should be *word_ptr or silence
  int category;       ///< Number of vector of categories with the AEF or -1
  const symbol_t *word_ptr; /**< Word pointer for extended symbols.
                           It points to the current position in the VOCAB_NONE-terminated
                           input symbol. In principle *word_ptr should be equal to word
                           except for silence and non-grammar symbols */
  symbol_t extended;  ///< The extended symbol
  unsigned char state_hmm;   ///< State of hmm
  unsigned char state_lexic; ///< State of model lexic
  short phoneme;             ///< Phoneme of the word
  probability_t probability; ///< Probability of the state
} hyp_t;

INLINE unsigned int hyp_hash(const hyp_t *hyp);
INLINE bool hyp_cmp(const hyp_t *s1, const hyp_t *s2);
void hyp_print(FILE *out, const hyp_t *hyp, const extended_vocab_t *vocab);

#endif /* HYPOTHESIS_H_ */
