
/*! @file
 *  \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#include <prhlt/constants.h>
#include <prhlt/utils.h>
#include "hmm.h"
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <float.h>


/** Computes the constant part of the logarithm of the gaussian density function
 * @param variance the variance of a gaussian distribution \f${\bf \Sigma}\f$
 * @param num_features the number of features \f$n\f$
 * @return \f$gconst\f$
 *
 * gconst is the logarithm of the constant part of the gaussian density function as established
 * by HTK. It can be precomputed so that the search is performed more efficiently.
 * \f$ gconst = n*\log(2\pi) + \log(|{\bf \Sigma}|) \f$
 */
double calculate_gconst(variance_t *variance, int num_features) {
  double gconst = ((double) num_features) * log(2 * M_PI);

  double determinant = .0;
  for (int i = 0; i < num_features; i++) {
    determinant += log(variance->variance[i]);
  }

  return gconst + determinant;
}


/** Computes the logarithm of the gaussian density function
 * @param data a feature vector \f${\bf o}\f$
 * @param gaussian a gaussian distribution \f$\mathcal{N}({\bf \mu},{\bf \Sigma})\f$
 * @param num_features the number of features \f$n\f$
 * @return the logarithm of the gaussian density function
 *         \f$log\left(\mathcal{N}({\bf o};{\bf \mu},{\bf \Sigma})\right)\f$
 *
 * As in the HTK toolkit, this function can be decomposed into a constant (\f$gconst\f$)
 * part and a part that depends on the feature vector \f${\bf o}\f$,
 * resulting in this formula:
 *
 * \f$ log\left(\mathcal{N}({\bf o};{\bf \mu},{\bf \Sigma})\right) =
 * \frac{1}{-2} \left(gconst + ({\bf o}-{\bf \mu})'{\bf \Sigma}^{-1}({\bf o}-{\bf \mu}) \right) \f$
 *
 * \f$gconst\f$ is usually precomputed by the function calculate_gconst()
 */
INLINE float log_gaussian(const float * data, const gaussian_t *gaussian, int num_features) {
  register double num = 0.0;

  for (int ll = 0; ll < num_features; ll++) {
    register float diff = data[ll] - gaussian->mean->mean[ll];
    num += (diff * diff) / gaussian->variance->variance[ll];
  }

  return (gaussian->constant + num) / -2.0;
}

/** Computes the logarithm of the gaussian mixture density function
 * @param data a feature vector \f${\bf o}\f$
 * @param gaussian a gaussian distribution \f$\mathcal{N}(\vec{{\bf \Theta}})\f$
 * @param num_features the number of features \f$n\f$
 * @return the logarithm of the gaussian mixture density function
 *         \f$log\left(\mathcal{N}({\bf o};\vec{{\bf \Theta}})\right)\f$
 *
 * Following the HTK book notation, the logarithm of the gaussian mixture density function
 * can be computed as:
 *
 * \f$log\left(\mathcal{N}({\bf o};\vec{{\bf \Theta}})\right) =
 * \log \left( \sum_{m=1}^{M} c_{m} \mathcal{N}({\bf o};{\bf \mu}_{m},{\bf \Sigma}_{m}) \right)\f$
*/
INLINE float log_gaussian_mixture(const float * data, const mixture_t *mixture, int num_features) {
  float sum = LOG_ZERO;
  for (int d = 0; d < mixture->num_distributions; d++) {
    sum = add_log(sum, log_gaussian(data, mixture->distributions[d]->gaussian, num_features)
                       + mixture->distributions[d]->prior);
  }
  return sum;
}


void compute_all_emissions(float *vector_cc, float *t_probability, hmm_t *hmm) {
  for (int s = 0; s < hmm->num_states; s++) {
    //If probability is not calculate
    if (is_logzero(t_probability[s])) {
      t_probability[s] = log_gaussian_mixture(vector_cc, hmm->states[s]->mixture, hmm->num_features);
    }
  }
}

///Print matrix
/**
@param matrix Matrix of transitions
@param file File result
*/
void print_matrix(matrix_transitions_t *matrix, FILE *file){
 int i, j;
 fprintf(file, "<TRANSP> %d\n", matrix->num_transitions);
 for(i=0; i<matrix->num_transitions; i++){
  for(j=0; j<matrix->num_transitions; j++){
   fprintf(file, "%e ", exp(matrix->matrix_transitions[i][j]));
  }
  fprintf(file, "\n");
 }
}
///Print mean
/**
@param mean Vector of mean
@param file File result
@param num Num of features (Dimension of vector)
*/
void print_mean(mean_t *mean, FILE *file, int num){
 int i;
 fprintf(file, "<MEAN> %d\n", num);
   for(i=0; i<num; i++){
    fprintf(file, "%e ", mean->mean[i]);
   }
   fprintf(file, "\n");
}
///Print variance
/**
@param variance Vector of variance
@param file File result
@param num Num of features (Dimension of vector)
*/
void print_variance(variance_t *variance, FILE *file, int num){
 int i;
fprintf(file, "<VARIANCE> %d\n", num);
   for(i=0; i<num; i++){
    fprintf(file, "%e ", variance->variance[i]);
   }
   fprintf(file, "\n");
}

///Print gconst
/**
@param constant Gconst
@param file File result
*/
void print_gconst(float constant, FILE *file){
 if(isnan(constant)==0){
  fprintf(file, "<GCONST> %e\n", constant);
 }
}

///Print gaussian
/**
@param gaussian gaussian
@param file File result
@param num Num of features (Dimension of vector)
*/
void print_gaussian(gaussian_t *gaussian, FILE *file, int num){
 //Link mean
 if(gaussian->mean->label!=NULL && strcmp(gaussian->mean->label, "")!=0){
  fprintf(file, "~u \"%s\"\n", gaussian->mean->label);
 }
 else{
  //Mean
  print_mean(gaussian->mean, file, num);
 }
 //Link variance
 if(gaussian->variance->label!=NULL && strcmp(gaussian->variance->label, "")!=0){
  fprintf(file, "~v \"%s\"\n", gaussian->variance->label);
 }
 else{
  //Variance
  print_variance(gaussian->variance, file, num);
 }
 //Constant
 print_gconst(gaussian->constant, file);
}


///Print mixture
/**
@param mixture Mixture
@param file File result
@param num Num of features
*/
void print_mixture(mixture_t *mixture, FILE *file, int num) {
 int gaussian;

 if (mixture->prior_probs == NULL) {
  //If there are more of 2 gaussians
  if(mixture->num_distributions != 1){
   //Number of gaussians
   fprintf(file, "<NUMMIXES> %d\n", mixture->num_distributions);
   for(gaussian=0; gaussian<mixture->num_distributions; gaussian++){
    //Probability
    fprintf(file, "<MIXTURE> %d %e\n", gaussian+1, exp(mixture->distributions[gaussian]->prior));
    //Link distribution
    if (mixture->distributions[gaussian]->label!=NULL && strcmp(mixture->distributions[gaussian]->label, "")!=0) {
     fprintf(file, "~m \"%s\"\n", mixture->distributions[gaussian]->label);
    }
    //Print gaussian
    else {
     print_gaussian(mixture->distributions[gaussian]->gaussian, file, num);
    }
   }
  }
  //Only a gaussian
  else{
   //Print gaussian
   print_gaussian(mixture->distributions[0]->gaussian, file, num);
  }
 }
 else {
  int i;
  char string[strlen(mixture->distributions[0]->label)+1];
  strcpy(string,mixture->distributions[0]->label);
  for (i=strlen(mixture->distributions[0]->label)-1; i > 0; i--) {
    if (!isdigit(string[i])) break;
  }
  string[i+1] = '\0';
  //Link distribution
  fprintf(file, "<NUMMIXES> %d\n", mixture->num_distributions);
  fprintf(file, "<TMIX> \"%s\" ", string);
  for(i=0; i<mixture->num_distributions; i++){
   fprintf(file, "%f ", exp(mixture->prior_probs[i]));
  }
  fprintf(file, "\n");
 }
}

///Print phoneme
/**
@param phoneme Phoneme of hmm
@param file File result
@param num Num of features
*/
void print_phoneme(phoneme_t *phoneme, FILE *file, int num){
 int state;
 //Begin of hmm
  fprintf(file,"<BEGINHMM>\n");
  //Number of states
  fprintf(file,"<NUMSTATES> %d\n", phoneme->num_states);
  //States
  for(state=0; state<(phoneme->num_states-2); state++){
   //state
   fprintf(file, "<STATE> %d\n", state+2);

   //State link
   if(phoneme->states[state]->label!=NULL && strcmp(phoneme->states[state]->label, "")!=0){
    fprintf(file, "~s \"%s\"\n", phoneme->states[state]->label);
   }
   //Print mixture
   else{
    print_mixture(phoneme->states[state]->mixture, file, num);
   }
  }//for states
  //Link matrix
  if(phoneme->matrix->label !=NULL && strcmp(phoneme->matrix->label, "")!=0){
   fprintf(file, "~t \"%s\"\n", phoneme->matrix->label);
  }
  else{
   //Print matrix of transitions
   print_matrix(phoneme->matrix, file);
  }

  fprintf(file, "<ENDHMM>\n");
}

///Save hmm
/**
@param hmm Hmm
@param file File result
*/
void hmm_save(hmm_t *hmm, FILE *file) {
 int phoneme=0, state_link=0, matrix=0, distribution_link=0;

 //Print the options
 fprintf(file, "~o\n<STREAMINFO> 1 %d\n<VECSIZE> %d<NULLD><MFCC_D_A><DIAGC>\n", hmm->num_features, hmm->num_features);

 //Link mean
 for(matrix=0; matrix<hmm->num_means; matrix++){
  if(hmm->means[matrix]->label !=NULL && strcmp(hmm->means[matrix]->label, "")!=0){
   fprintf(file, "~u \"%s\"\n", hmm->means[matrix]->label);
   print_mean(hmm->means[matrix], file, hmm->num_features);
  }
 }

 //Link variance
 for(matrix=0; matrix<hmm->num_variances; matrix++){
  if(hmm->variances[matrix]->label !=NULL && strcmp(hmm->variances[matrix]->label, "")!=0){
   fprintf(file, "~v \"%s\"\n", hmm->variances[matrix]->label);
   print_variance(hmm->variances[matrix], file, hmm->num_features);
  }
 }

 //Link matrix
 for(matrix=0; matrix<hmm->num_matrix; matrix++){
  if(hmm->matrix[matrix]->label !=NULL && strcmp(hmm->matrix[matrix]->label, "")!=0){
   fprintf(file, "~t \"%s\"\n", hmm->matrix[matrix]->label);
   print_matrix(hmm->matrix[matrix], file);
  }
 }

 //Link states
 for(state_link=0; state_link<hmm->num_states; state_link++){
  if(hmm->states[state_link]->label !=NULL && strcmp(hmm->states[state_link]->label, "")!=0){
   fprintf(file, "~s \"%s\"\n", hmm->states[state_link]->label);
   print_mixture(hmm->states[state_link]->mixture, file, hmm->num_features);
  }
 }

 //Link distribution
 for(distribution_link=0; distribution_link<hmm->num_distributions; distribution_link++){
  if(hmm->distributions[distribution_link]->label !=NULL && strcmp(hmm->distributions[distribution_link]->label, "")!=0){
   fprintf(file, "~m \"%s\"\n", hmm->distributions[distribution_link]->label);
   print_mean(hmm->distributions[distribution_link]->gaussian->mean, file, hmm->num_features);
   print_variance(hmm->distributions[distribution_link]->gaussian->variance, file, hmm->num_features);
  }
 }

 //Phonemes
 for(phoneme=0; phoneme<hmm->num_phonemes; phoneme++){
  //Name of the phoneme
  fprintf(file, "~h \"%s\"\n", hmm->phonemes[phoneme]->label);
  print_phoneme(hmm->phonemes[phoneme], file, hmm->num_features);
 }

}

///Function to create the hmm
/**
@return hmm
*/
hmm_t * hmm_create() {
 hmm_t * hmm;

 hmm = (hmm_t *)malloc(sizeof(hmm_t));
 //Phonemes
 hmm->num_phonemes = 0;
 hmm->phonemes = NULL;

 //States
 hmm->num_states=0;
 hmm->states = NULL;

 //Matrix
 hmm->num_matrix=0;
 hmm->matrix = NULL;

 //Distributions
 hmm->num_distributions=0;
 hmm->distributions = NULL;

 //Means
 hmm->num_means=0;
 hmm->means=NULL;

 //Variances
 hmm->num_variances=0;
 hmm->variances=NULL;

 //Features
 hmm->num_features=0;

 hmm->n_hmm_states = 0;
 hmm->locations = NULL;

 return hmm;
}

///Delete hmm
/**
@param hmm Hmm
*/
void hmm_delete(hmm_t *hmm){
 int i, j;

 //Variances
 for(i=0; i<hmm->num_variances; i++){
  free(hmm->variances[i]->label);
  free(hmm->variances[i]->variance);
  free(hmm->variances[i]);
 }
 free(hmm->variances);

 //Means
 for(i=0; i<hmm->num_means; i++){
  free(hmm->means[i]->label);
  free(hmm->means[i]->mean);
  free(hmm->means[i]);
 }
 free(hmm->means);

 //Distributions
 for(i=0; i<hmm->num_distributions; i++){
  free(hmm->distributions[i]->label);
  free(hmm->distributions[i]->gaussian);
  free(hmm->distributions[i]);
 }
 free(hmm->distributions);

 //Matrix
 for(i=0; i<hmm->num_matrix; i++){
  for(j=0; j<hmm->matrix[i]->num_transitions; j++){
   free(hmm->matrix[i]->matrix_transitions[j]);
  }
  free(hmm->matrix[i]->matrix_transitions);
  free(hmm->matrix[i]->label);
  free(hmm->matrix[i]);
 }
 free(hmm->matrix);

 //States
 for(i=0; i<hmm->num_states; i++){
  free(hmm->states[i]->label);
  free(hmm->states[i]->mixture->distributions);
  free(hmm->states[i]->mixture);
  free(hmm->states[i]);
 }
 free(hmm->states);

 //Phonemes
 for(i=0; i<hmm->num_phonemes; i++){
  free(hmm->phonemes[i]->label);
  free(hmm->phonemes[i]->states);
  free(hmm->phonemes[i]->hmm_ids);
  free(hmm->phonemes[i]);
 }
 free(hmm->phonemes);

 free(hmm->locations);
 free(hmm);
}

void hmm_compute_emission_probabilities(const hmm_t *hmm, features_t *features) {
  // for each class
  for (int t = 0; t < features->n_vectors; t++) {
    float *probs = (float *) malloc(hmm->num_states * sizeof(float));
    for (int s = 0; s < hmm->num_states; s++) {
      probs[s] = log_gaussian_mixture(features->vector[t], hmm->states[s]->mixture, hmm->num_features);
    }
    SWAP(features->vector[t], probs, float *);
    free(probs);
  }

  // fill structure field
  free(features->structure);
  features->structure = NULL;
  int len = 0;
  for (int i = 0; i < hmm->num_states; i++) {
    char label[MAX_LINE];
    if (hmm->states[i]->label != NULL) strcpy(label, hmm->states[i]->label);
    else snprintf(label, MAX_LINE, "state%d", i);
    len += strlen(label) + 2;
    features->structure = (char *) realloc(features->structure, len);
    if (i > 0) strcat(features->structure, " ");
    else strcpy(features->structure, "");
    strcat(features->structure, label);
  }

  features->n_features = hmm->num_states;
  features->type = FT_EMISSION_PROBABILITIES;
}
