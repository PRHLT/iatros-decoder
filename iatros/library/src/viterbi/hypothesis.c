/*
 * hypothesis.c
 *
 *  Created on: 23-sep-2008
 *      Author: valabau
 */

#include "hypothesis.h"
#include <prhlt/utils.h>

///Hash function for hypothesis
/**
 * @param hyp the hypothesis
 * Note: state_in and state_out have been not considered since that grammars are to be deterministic.
 * Thus, state_in and state_out do not add additional information
 */
INLINE unsigned int hyp_hash(const hyp_t *hyp) {
  return hyp->state_hmm + (hyp->state_lexic << 9)
         + (hyp->phoneme << 18) + (hyp->word << 7) + ((size_t)hyp->history << 11)
         + ((size_t)(hyp->word_ptr) << 13);
}

///Compares two hypotheses
/**
 * @param h1 an hypothesis
 * @param h2 an hypothesis
 * @return true if h1 == h2, false otherwise
 * Note: state_in and state_out have been not considered since that grammars are to be deterministic.
 * Thus, state_in and state_out do not add additional information
 */
INLINE bool hyp_cmp(const hyp_t *h1, const hyp_t *h2) {
 return (h1->phoneme==h2->phoneme && h1->word==h2->word && h1->word_ptr==h2->word_ptr
        && h1->state==h2->state && h1->state_hmm==h2->state_hmm
        && h1->state_lexic==h2->state_lexic&& h1->history==h2->history);

}

///Prints an hypothesis
/**
 * @param out output stream
 * @param hyp hypothesis
 */
void hyp_print(FILE *out, const hyp_t *hyp, const extended_vocab_t *vocab) {
  int hist_state_num = (hyp->history != NULL) ? hyp->history->num_state : -1;
  float ac = hyp->probability.acoustic;
  float lm = hyp->probability.lm;

  fprintf(out, "%s(%d)[%s(%d)](%d,%d,%d) %d->%d (a=%g,l=%g,f=%g) from %d",
      vocab_get_string(vocab->in, hyp->word), hyp->word,
      extended_vocab_get_string(vocab, hyp->extended), hyp->extended,
      hyp->state_hmm, hyp->phoneme, hyp->state_lexic, hist_state_num, hyp->state->num_state,
      ac, lm, hyp->probability.final, hyp->index);
}
