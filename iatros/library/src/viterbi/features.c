/*
 * features.c
 *
 *  Created on: 24-dic-2008
 *      Author: valabau
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <prhlt/constants.h>
#include <prhlt/trace.h>
#include <prhlt/gzip.h>
#include <prhlt/utils.h>
#include <viterbi/features.h>

#ifdef MAX_LINE
#undef MAX_LINE
#endif

#define MAX_LINE 1024*100

static const char *feat_type_str[] = {
    "Generic",
    "CepstrumCoef",
    "CepstrumCoefDerAcc",
    "LdaTransformed",
    "EmissionProbabilities",
    "PosteriorProbabilities",
    "Unknown"
};

///Find feature type id
feat_type_t find_feat_type(const char *feat_str) {
  feat_type_t type = FT_UNKNOWN;
  for (int f = 0; f < FT_UNKNOWN; f++) {
    if (strcmp(feat_str, feat_type_str[f]) == 0) {
      type = (feat_type_t)f;
      break;
    }
  }
  return type;
}

///Read file with vector of cepstrals
/**
 @param vector_cc Direction of vector of ceptrals
 @param name Name of file with ceptrals
 @param num Number of features of hmm
 @return Return el number of vectors
 */
features_t * features_create_from_file(const char *filename) {
  int n_lines = 0, n_vectors_readed = 0;
  bool in_data = false;

  features_t *feas = (features_t *)calloc(1, sizeof(features_t));
  MEMTEST(feas);

  //Open file
  FILE *file = smart_fopen(filename, "r");
  CHECK_SYS_ERROR(file != NULL, "Cannot open file '%s'\n", filename);

  //Read file
  char line[MAX_LINE];
  while (fgets(line, MAX_LINE, file) != 0) {
    line[strlen(line) - 1] = '\0';

    // Read number of features
    if (strncmp(line, "Name", 4) == 0 && feas->name == NULL) {
      char *name = line + 5;
      while (*name != '\0' && isspace(*name)) name++;
      if (*name != '\0') {
        feas->name = (char *)malloc((strlen(name)+1)*sizeof(char));
        strcpy(feas->name, name);
      }
    }
    if (strncmp(line, "Structure", 9) == 0 && feas->structure == NULL) {
      char *structure = line + 10;
      while (*structure != '\0' && isspace(*structure)) structure++;
      if (*structure != '\0') {
        feas->structure = (char *)malloc((strlen(structure)+1)*sizeof(char));
        strcpy(feas->structure, structure);
      }
    }
    // Read number of features
    if (strncmp(line, "NumParam", 8) == 0) {
      sscanf(line, "NumParam %d", &(feas->n_features));
    }
    // Read number of features and allocate memory for vector
    if (strncmp(line, "NumVect", 7) == 0) {
      sscanf(line, "NumVect %d", &(feas->n_vectors));
    }
    // Read type of data
    if (strncmp(line, "DataType", 8) == 0) {
      char feat_str[MAX_LINE];
      sscanf(line, "DataType %s", feat_str);
      feas->type = find_feat_type(feat_str);
    }

    // Read feature vectors
    if (strncmp(line, "Data", 4) == 0 && strncmp(line, "DataType", 8) != 0) {
      feas->vector = (float **) malloc(feas->n_vectors * sizeof(float *));
      for (int i = 0; i < feas->n_vectors; i++) {
        feas->vector[i] = (float *) malloc(feas->n_features * sizeof(float));
      }
      in_data = true;
      n_vectors_readed = 0;
    } else if (in_data && n_vectors_readed < feas->n_vectors) {
      int i = 0;
      char delimiters[] = " \n\t";
      //First call
      char *ptr = strtok(line, delimiters);
      feas->vector[n_vectors_readed][i++] = atof(ptr);
      while ((ptr = strtok(NULL, delimiters)) != NULL) {
        feas->vector[n_vectors_readed][i++] = atof(ptr);
      }
      n_vectors_readed++;
    }

    n_lines++;
  }//While

  smart_fclose(file);

  return feas;
}

///Delete matrix with cepstrals
/**
@param vector_cc Matrix with cepstrals
@param num_vectors Number of vectors
*/
void features_delete(features_t * features) {
  for (int t = 0; t < features->n_vectors; t++) {
    free(features->vector[t]);
  }
  free(features->vector);
  free(features->name);
  free(features->structure);
  free(features);
}

void features_save_header(const features_t * features, FILE *out) {
  fprintf(out, "Name            %s\n", ((features->name != NULL)?features->name:"feature file"));
  //fprintf(out, "Date            Thu Mar 15 19:45:51 2007\n");
  //fprintf(out, "CorpusName      unknown\n");
  //fprintf(out, "OrtTrans        micro\n");
  //fprintf(out, "CommandLine     atrosd\n");
  //fprintf(out, "ConfigFile      /home/vtamarit/dihana/AtrosV6.00/models/dihana.cnf\n");
  fprintf(out, "DataType        %s\n", feat_type_str[features->type]);
  fprintf(out, "Structure       ");
  if (features->structure != NULL) fprintf(out, "%s", features->structure);
  else for (int i = 0; i < features->n_features; i++) fprintf(out, "f%d ", i);
  fprintf(out, "\n");
  fprintf(out, "NumParam        %d\n", features->n_features);
  fprintf(out, "NumVect         %d\n", features->n_vectors);
  fprintf(out, "Data\n");
}

void features_save(const features_t * features, FILE *out) {
  features_save_header(features, out);

  for (int t = 0; t < features->n_vectors; t++) {
    for (int f = 0; f < features->n_features; f++) {
      fprintf(out, "%g ", features->vector[t][f]);
    }
    fprintf(out, "\n");
  }
}

void features_resize(features_t *features, int new_size) {
  // shrink
  for (int i = new_size; i < features->n_vectors; i++) {
    free(features->vector[i]);
  }

  features->vector = (float **) realloc(features->vector, new_size * sizeof(float *));

  // extend
  for (int i = features->n_vectors; i < new_size; i++) {
    features->vector[i] = (float *)malloc(features->n_features * sizeof(float));
  }

  features->n_vectors = new_size;
}

void features_filter_energy_threshold(features_t *features, float threshold) {
  for (int t = 0; t < features->n_vectors; t++) {
    if (features->vector[t][0] < threshold) {
      for (int f = 0; f < features->n_features; f++) {
        features->vector[t][f] = 0;
      }
    }
  }
}

