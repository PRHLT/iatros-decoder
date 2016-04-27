/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#include <config.h>
#include <iatros/features.h>
#include <iatros/hmm.h>
#include <iatros/grammar.h>
#include <prhlt/trace.h>
#include <prhlt/gzip.h>
#include <prhlt/constants.h>
#include <prhlt/utils.h>

void help(int UNUSED(argc), char *argv[]){
  printf("Usage: %s\n", argv[0]);
  printf("This software computes GMM probabilities\n"
         "-h\t This help\n"
         "-g <file>\t GMMs in HTK format\n"
         "-l <file>\t List of feature vector files\n"
         "-m <file>\t Save posterior probabilities for each mixture\n"
         "-c <file>\t Save posterior probabilities for each class\n"
         "-s <file>\t Save a string made of mixture components identifiers\n"
         "-a <file>\t Save probability of one against the rest\n"
         "-v       \t Use viterbi approximation\n"
         "-n       \t Do not multiply mixture priors\n"
         );

}

void arg_max(float *vector, int num_elems, float *max_value, int *arg) {
  *max_value = -FLT_MAX;
  for (int i = 0; i < num_elems; i++) {
    if (vector[i] > *max_value) {
      *max_value = vector[i];
      *arg = i;
    }
  }
}

float log_normalize(float *vector, int num_elems) {
  float norm = LOG_ZERO;
  for (int c = 0; c < num_elems; c++) {
    norm = add_log(norm, vector[c]);
  }
  for (int c = 0; c < num_elems; c++) {
    vector[c] -= norm;
    vector[c] = exp(vector[c]);
  }
  return norm;
}

int main (int argc, char *argv[]) {

  //Structure hmm
  hmm_t * hmm;

  //File of cepstrals
  FILE *file_cepstrals;

  int option;
  int use_priors = 1, use_viterbi = 0;
  char *line=(char *)malloc(MAX_LINE*sizeof(char));
  char *hmm_fn = NULL, *list_fn = NULL;
  char *mix_fn = NULL, *class_fn = NULL, *string_fn = NULL, *against_fn = NULL;
  FILE *class_fd = NULL, *mix_fd = NULL, *against_fd = NULL;
  clock_t tim=0, tim2=0;


   while ((option=getopt(argc,argv,"l:g:m:c:s:a:vnh"))!=-1){
     switch (option){
     case 'g':
       hmm_fn = optarg;
       break;
     case 'l':
       list_fn = optarg;
       break;
     case 'm':
       mix_fn = optarg;
       break;
     case 'c':
       class_fn = optarg;
       break;
     case 's':
       string_fn = optarg;
       break;
     case 'v':
       use_viterbi = 1;
       break;
     case 'n':
       use_priors = 0;
       break;
     case 'a':
       against_fn = optarg;
       break;
     case 'h':
       help(argc, argv);
       break;
     default:
       help(argc, argv);
       exit(1);
       break;
     }
   }//End while




  INIT_TRACE(0);

  REQUIRE(hmm_fn != NULL && list_fn != NULL,"Gaussian file and list of feature vector files needed\n");

  //Load all models
  {
    //Aux
    FILE *aux;

    //Create hmm
    hmm = hmm_create();

    //Copy the information of the file_hmm in the structure hmm
    aux = smart_fopen(hmm_fn,"r");
    CHECK_SYS_ERROR(aux != NULL, "No file hmm\n");
    hmm_load(hmm,aux);
    smart_fclose(aux);

    for (int p = 0; p < hmm->num_phonemes; p++) {
      REQUIRE(hmm->phonemes[p]->num_states == 3, "Wrong number of states in phoneme %d\n", p);
    }
  }

  //Open file of cepstrals
  file_cepstrals = smart_fopen(list_fn, "r");
  CHECK_SYS_ERROR(file_cepstrals != NULL, "No file of ceptrals\n");

  //Open output files
  if (mix_fn     != NULL) mix_fd     = smart_fopen(mix_fn, "w");
  if (against_fn != NULL) {
    against_fd = smart_fopen(against_fn, "w");
    if (against_fd != NULL) {
      fprintf(against_fd,"#");
      for (int c = 0; c < hmm->num_phonemes; c++) {
        fprintf(against_fd, "%s ", hmm->phonemes[c]->label);
        fprintf(against_fd, "not_%s ", hmm->phonemes[c]->label);
      }
      fprintf(against_fd,"\n");
    }
  }
  if (class_fn != NULL) {
    class_fd = smart_fopen(class_fn,  "w");
    if (class_fd != NULL) {
      fprintf(class_fd,"#");
      for (int c = 0; c < hmm->num_phonemes; c++) {
        fprintf(class_fd, "%s ", hmm->phonemes[c]->label);
      }
      fprintf(class_fd,"\n");
    }
  }

  {
    FILE *string_fd[hmm->num_phonemes];
    float prior[hmm->num_phonemes];
    float posterior[hmm->num_phonemes];
    char  string_i_fn[MAX_LINE];

    printf("#class\tprob\t tim\n");
    for (int c = 0; c < hmm->num_phonemes; c++) {
      if (string_fn != NULL) {
        sprintf(string_i_fn, "%s.%s.gz", string_fn, hmm->phonemes[c]->label);
        string_fd[c] = smart_fopen(string_i_fn,    "w");
        if (string_fd[c] == NULL) {
         printf("Error opening file '%s' for writing\n", string_i_fn);
         exit(1);
        }
      }
      else {
        string_fd[c] = NULL;
      }
    }

    // assume equiprobable classes
    for (int c = 0; c < hmm->num_phonemes; c++) {
      prior[c] = log(1.0/(float)hmm->num_phonemes);
    }

    //Ceptral to ceptral
    while(fgets(line, MAX_LINE, file_cepstrals) != 0) {
      line[strlen(line)-1]='\0';

      features_t * feas = features_create_from_file(line);

      //Read cepstrals
      tim=clock();

      // for each class
      for (int c = 0; c < hmm->num_phonemes; c++) {
        int state = 0;
        posterior[c] = prior[c];
        // accumulate log gaussian mixture density
        for(int t = 0; t < feas->n_vectors; t++) {
          float sum = .0;
          float mix_posterior[hmm->phonemes[c]->states[state]->mixture->num_distributions];
          float max_lprob = LOG_ZERO;
          int max_i = 0;

          // compute gaussian density for each component and sum
          for(int p = 0; p < hmm->phonemes[c]->states[state]->mixture->num_distributions; p++) {
            mix_posterior[p] = log_gaussian(feas->vector[t], hmm->phonemes[c]->states[state]->mixture->distributions[p]->gaussian, hmm->num_features);
            if (use_priors) {
              mix_posterior[p] += hmm->phonemes[c]->states[state]->mixture->distributions[p]->prior;
            }
          }

          // get argmax_i and max log prob
          arg_max(mix_posterior, hmm->phonemes[c]->states[state]->mixture->num_distributions, &max_lprob, &max_i);
          // normalize mix posteriors
          sum = log_normalize(mix_posterior, hmm->phonemes[c]->states[state]->mixture->num_distributions);

          if (use_viterbi) {
            // add sum
            posterior[c] += max_lprob;
          }
          else {
            // add sum
            posterior[c] += sum;
          }

          // print mix posteriors
          if (mix_fd != NULL) {
            for (int i = 0; i < hmm->phonemes[c]->states[state]->mixture->num_distributions; i++) {
              fprintf(mix_fd, "%g ", mix_posterior[i]);
            }
            fprintf(mix_fd, "\n");
          }

          // print string made of mixture identifiers
          if (string_fd[c] != NULL) {
            fprintf(string_fd[c], "%d ", max_i);
          }

        }
        if (string_fd[c] != NULL) fprintf(string_fd[c], "\n");
      }


      { // do maximum-a-posteriori
        float max_posterior = .0;
        int   max_c = 0;
        float error_prob = LOG_ZERO;

        if (against_fd != NULL) {
          float post[2];
          for (int ac = 0; ac < hmm->num_phonemes; ac++) {
            for (int c = 0; c < hmm->num_phonemes; c++) {
              if (c != max_c) {
                error_prob = add_log(error_prob, posterior[c]);
              }
            }
            post[0] = posterior[max_c];
            post[1] = error_prob;
            log_normalize(post, 2);
            fprintf(against_fd, "%g %g ", post[0], post[1]);
          }
          fprintf(against_fd, "\n");

        }

        log_normalize(posterior, hmm->num_phonemes);
        arg_max(posterior, hmm->num_phonemes, &max_posterior, &max_c);

        if (class_fd != NULL) {
          for (int c = 0; c < hmm->num_phonemes; c++) {
            fprintf(class_fd, "%g ", posterior[c]);
          }
          fprintf(class_fd, "\n");
        }

        tim2=clock();
        printf("%d\t%g\t%f\n", max_c, max_posterior, ((float)((tim2-tim)/CLOCKS_PER_SEC)/feas->n_vectors)/0.01);
        fflush(stdout);
      }

      features_delete(feas);
    }//while
    for (int c = 0; c < hmm->num_phonemes; c++) if (string_fd[c] != NULL) smart_fclose(string_fd[c]);
  }

  hmm_delete(hmm);
  free(line);

  if (mix_fd) smart_fclose(mix_fd);
  if (class_fd)  smart_fclose(class_fd);
  smart_fclose(file_cepstrals);

  return EXIT_SUCCESS;
}
