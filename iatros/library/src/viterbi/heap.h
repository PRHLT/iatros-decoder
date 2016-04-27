/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#ifndef __HEAP_H__
#define __HEAP_H__

#include <iatros/probability.h>
#include <iatros/hypothesis.h>
#include <iatros/statistics.h>
#include <prhlt/utils.h>


struct hyp_heap_t;
typedef struct hyp_heap_t hyp_heap_t;

hyp_heap_t *hh_create(int max_elems, float beam);
void hh_delete(hyp_heap_t *hh);
void hh_clear(hyp_heap_t *hh);
INLINE beam_status_t hh_insert(hyp_heap_t *hh, hyp_t *hyp);
INLINE hyp_t *hh_pop(hyp_heap_t *hh);
INLINE int hh_size(const hyp_heap_t *hh);
INLINE int hh_capacity(const hyp_heap_t *hh);
INLINE bool hh_is_empty(const hyp_heap_t *hh);
INLINE float hh_limit(const hyp_heap_t *hh);
INLINE float hh_max(const hyp_heap_t *hh);
INLINE float hh_min(const hyp_heap_t *hh);
INLINE float hh_beam(const hyp_heap_t *hh);
INLINE int hh_sort(hyp_heap_t *hh);
void hh_print(FILE *file, const hyp_heap_t *hh, const grammar_t *grammar);
void hh_print_stats(FILE *file, const hyp_heap_t *heap, const grammar_t *grammar);

#endif // __HEAP_H__
