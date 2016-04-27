/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#ifndef _HMM_H
#define _HMM_H
#include <stdio.h>
#include <stdlib.h>
#include <prhlt/utils.h>
#include <iatros/features.h>


///Structs for hmm
typedef enum { GAUSSIAN } distribution_type_t;

/// Mean. ~u
typedef struct mean {
  char *label; ///< Name of mean
  float *mean; ///< vector of means
} mean_t;

/// Variance. ~v
typedef struct variance {
  char *label; ///< Name of variance
  float *variance; ///< Vector of variances
} variance_t;

/// Gaussian mixture component. ~m
typedef struct gaussian {
  mean_t *mean; ///< Mean
  variance_t *variance; ///< Variance
  float constant; ///< Gconst of gaussian
} gaussian_t;

/// Distribution type.
typedef struct distribution {
  distribution_type_t type; ///< Type of distribution
  gaussian_t *gaussian; ///< Distribution
  float prior; ///< Prior probability of distribution
  char *label; ///< Name
} distribution_t;

/// Mixture of distributions.
typedef struct mixture {
  int num_distributions; ///< Number of distributions in the mixture
  float *prior_probs; ///< Vector of prior probabilities
  distribution_t **distributions; ///< Distributions
} mixture_t;

/// hmm state identifier
typedef enum { HMM_NONE = -1 } hmm_id_t;
/// hmm id location in the hmm models
typedef struct {
  int phoneme; ///< phoneme
  int state;   ///< state in the phoneme
  int state_id;///< state id
} hmm_location_t;

/// State. ~s
typedef struct state {
  mixture_t *mixture; ///< Mixture of the state
  char *label; ///< Name of state
  int id; ///< state id
} state_t;

/// Transition probabilities. ~t
typedef struct matrix_transitions {
  char *label; ///< Name of matrix
  float **matrix_transitions; ///< Matrix in log-probabilities
  int num_transitions; ///< Number of transitions
} matrix_transitions_t;

/// Phoneme. ~h
typedef struct phoneme {
  char *label; ///< Name of phoneme
  state_t **states; ///< States of the phoneme
  hmm_id_t *hmm_ids;///< hmm ids of the states in phoneme
  matrix_transitions_t *matrix; ///< Matrix of the phoneme
  int num_states; ///< Number of states in phoneme
} phoneme_t;

/// Hidden Markov Model
typedef struct hmm {
  int num_phonemes; ///< Number of phonemes in the HMM
  phoneme_t **phonemes; ///< List of phonemes

  int num_states; ///< Number of states in the HMM
  state_t **states; ///< List of states

  int n_hmm_states; ///< Number of hmm states counting repetitions
  hmm_location_t *locations; ///< List of hmm locations

  int num_matrix; ///< Number of matrix in the HMM
  matrix_transitions_t **matrix; ///< Vector of transition matrixs

  int num_distributions; ///< Number of distribution in the HMM
  distribution_t **distributions; ///< Vector of distributions

  int num_means; ///< Number of means in the HMM
  mean_t **means; ///< Vector of means

  int num_variances; ///< Number of variances in the HMM
  variance_t **variances; ///< Vector of variances

  int num_features; ///< Number of features
} hmm_t;

//Functions for hmm
int hmm_load(hmm_t * hmm, FILE *file);
//Save in the structure hmm the informacion of the file
void hmm_save(hmm_t *hmm, FILE *file);
//Create and initiate the hmm
hmm_t * hmm_create();
//Delete hmm
void hmm_delete(hmm_t *hmm);

INLINE float log_gaussian(const float * data, const gaussian_t *gaussian, int num_features);
INLINE float log_gaussian_mixture(const float * data, const mixture_t *mixture, int num_features);
void hmm_compute_emission_probabilities(const hmm_t *hmm, features_t *features);
#endif // _HMM_H

