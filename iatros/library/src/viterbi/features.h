/*
 * features.h
 *
 *  Created on: 24-dic-2008
 *      Author: valabau
 */

#ifndef FEATURES_H_
#define FEATURES_H_

#include <stdio.h>

typedef enum { FT_GENERIC, FT_CC, FF_CC_DER_ACC, FF_LDA, FT_EMISSION_PROBABILITIES, FT_POSTERIOR_PROBABILITIES, FT_UNKNOWN } feat_type_t;

typedef struct {
  char *name;
  char *structure;
  feat_type_t type;
  float **vector;
  int n_vectors;
  int n_features;
} features_t;

#ifdef __cplusplus
extern "C" {
#endif


features_t * features_create_from_file(const char *name);
void features_delete(features_t * features);
void features_save(const features_t * features, FILE *out);
void features_resize(features_t *features, int new_size);
void features_filter_energy_threshold(features_t *features, float threshold);

#ifdef __cplusplus
}
#endif


#endif /* FEATURES_H_ */
