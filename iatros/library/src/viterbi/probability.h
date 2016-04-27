/*
 * probability.h
 *
 *  Created on: 23-sep-2008
 *      Author: valabau
 */

#ifndef PROBABILITY_H_
#define PROBABILITY_H_

#include <prhlt/constants.h>
#include <math.h>

/// Probability of state.
typedef struct {
  float acoustic; ///< Acoustic probability
  float lm;       ///< Probability of language model
  float in_lm;    ///< Probability of input language model
  float out_lm;   ///< Probability of output language model
  float wip_out;  ///< Fraction of the out_wip correconding to each word of the input phrase
  float final;    ///< Final probability is final=final+acoustic+lm*gsf-wip+(gsf_in*lm_in + gsf_out*lm_out + combined_score)
} probability_t;

static const probability_t one_probability = { .0, .0, .0, .0, .0, .0 };
static const probability_t zero_probability = { LOG_ZERO, LOG_ZERO, LOG_ZERO, LOG_ZERO, LOG_ZERO, LOG_ZERO };
static const probability_t inf_probability = { INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY };
#endif /* PROBABILITY_H_ */
