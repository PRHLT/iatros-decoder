/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#ifndef _VITERBI_H
#define _VITERBI_H
#include <stdio.h>
#include <iatros/hmm.h>
#include <iatros/lex.h>
#include <iatros/grammar.h>
#include <iatros/heap.h>
#include <iatros/lattice.h>
#include <iatros/search.h>

#ifdef __cplusplus
extern "C" {
#endif


void decode(search_t *search, const features_t *features, lattice_t *lattice);

void initial_stage(search_t *search, const float *feat_vec, lattice_t *lattice);
void viterbi_frm(search_t *search, const float *feat_vec, lattice_t *lattice);
void end_stage(search_t *search, lattice_t *lattice);

#ifdef __cplusplus
}
#endif


#endif // _VITERBI_DRIVER_H
