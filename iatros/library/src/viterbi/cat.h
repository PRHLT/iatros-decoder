/*
 * cat.h
 *
 *  Created on: 17-feb-2009
 *      Author: valabau
 */

#ifndef CAT_H_
#define CAT_H_

#include <iatros/viterbi.h>

typedef enum { REF_NONE, REF_SOURCE, REF_TARGET, REF_PREFIX, REF_MAX } reference_t;

#ifdef __cplusplus
extern "C" {
#endif


grammar_t * grammar_create_from_prefix(const grammar_t *grammar, const symbol_t *input_prefix, const symbol_t *output_prefix);
grammar_t * grammar_create_conditioned_to_prefix(const grammar_t *grammar, const symbol_t *input_prefix);
grammar_t * grammar_create_wordlist_from_prefix(const grammar_t *grammar, const symbol_t *input_prefix);

search_t *search_create_from_prefix(const search_t *search, symbol_t *in_prefix, symbol_t *out_prefix);
/* in_prefix is a prefix of extended symbols */
search_t *search_create_conditioned_to_prefix(const search_t *search, symbol_t *in_prefix);
search_t *search_create_wordlist_from_prefix(const search_t *search, symbol_t *in_prefix);

int cat_decode(search_t *search, const features_t *features, lattice_t *lattice,
               symbol_t *ref, reference_t ref_type);

int cat_decode_word(search_t *search, const features_t *features, lattice_t *lattice,
               symbol_t *ref, reference_t ref_type, const char *lattice_tmpl);

#ifdef __cplusplus
}
#endif


#endif /* CAT_H_ */
