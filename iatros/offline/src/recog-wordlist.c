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
#include <sys/stat.h>

#include <prhlt/utils.h>
#include <config.h>
#include <iatros/version.h>
#include <iatros/statistics.h>
#include <iatros/viterbi.h>
#include <prhlt/trace.h>
#include <prhlt/gzip.h>
#include <prhlt/vocab.h>
#include <iatros/features.h>
#include <iatros/lattice.h>
#include <iatros/cat.h>
#include <prhlt/constants.h>

void outputs(char *line, const args_t *args, lattice_t *lattice) {
  char *path = (char *) malloc(MAX_LINE * sizeof(char));

  //Word_graph
  if (args_get_bool(args, "save-lattices", NULL)) {
    //Name of file with word_graph
    char const *lattices_dn = args_get_string(args, "lattice-directory", NULL);
    if (lattices_dn == NULL) lattices_dn = ".";
    sprintf(path, "%s/%s.lat.gz", lattices_dn, basename(line));
    //File of graph
    FILE *lattice_file = smart_fopen(path, "w");
    CHECK_SYS_ERROR(lattice_file != NULL, "Couldn't create word graph file '%s'\n", path);
    lattice_write(lattice, lattice_file, path);
    smart_fclose(lattice_file);
  }

  //Best hypothesis
  symbol_t *sentence;
  float prob = lattice_best_hyp(lattice, &sentence);
  if (sentence != NULL) {
    char *sentence_str = NULL;
    extended_vocab_symbols_to_string(sentence, lattice->decoder->vocab, &sentence_str);
    if (args_get_bool(args, "print-score", NULL)) printf("%g ", prob);
    printf("%s\n", sentence_str);
    free(sentence);
    free(sentence_str);
  } else {
    printf("Sentence not recognized\n");
    fflush(stdout);
  }

  free(path);
}


static const arg_module_t recog_module = {NULL, "General options",
    {
      {"save-lattices", ARG_BOOL, "false", 0, "Enables saving lattices"},
      {"lattice-directory", ARG_DIR, NULL, 0, "Directory where lattices will be saved"},
      {"samples", ARG_FILE, NULL, 0, "List of files to process"},
      {"prefixes", ARG_STRING, NULL, 0, "List of prefixes for the samples"},
      {"save-vocab", ARG_STRING, NULL, 0, "Save vocabulary to file"},
      {"print-time", ARG_BOOL, "false", 0, "Print realtime factor"},
      {"print-score", ARG_BOOL, "false", 0, "Print hypothesis score"},
      {"part", ARG_STRING, NULL, 0, "Split the corpus into parts and run just one. Ex. --part '1:4' runs the 1st part out of 4"},
      {"verbosity", ARG_INT, "0", 0, "Set verbosity level"},
      {"statistics-verbosity", ARG_INT, "0", 0, "Set statistics verbosity level"},
      {"print-default", ARG_BOOL, "false", 0, "Print default config file"},
      {"print-config", ARG_BOOL, "false", 0, "Print final config file"},
      {NULL, ARG_END_MODULE, NULL, 0, NULL}
    }
};

static const arg_shortcut_t shortcuts[] = {
    {"v", "verbosity"},
    {"s", "statistics-verbosity"},
    {"p", "print-score"},
    {"t", "print-time"},
    {"d", "print-default"},
    {"l", "decoder.grammar-scale-factor"},
    {"P", "prefixes"},
    {NULL, NULL}
};

bool directory_exists(const char *dir) {
  if (dir == NULL) return false;
  else {
    struct stat sbuf;
    int serr = stat(dir, &sbuf);
    if (!serr && S_ISDIR(sbuf.st_mode)) return true;
    else return false;
  }
}

int main(int argc, char *argv[]) {
  CALLGRIND_INIT;

  arg_error_t error = ARG_OK;

  args_t *args = args_create();
  args_set_summary(args, "Speech/Handwritten text recognition/translation toolkit - wordlist recognition tool");
  args_set_doc(args, "For more info see http://prhlt.iti.es");
  args_set_version(args, IATROS_OFFLINE_PROJECT_STRING"\n"IATROS_OFFLINE_BUILD_INFO"\n\n"
                         IATROS_PROJECT_STRING"\n"IATROS_BUILD_INFO);
  args_set_bug_report(args, "Report bugs to "IATROS_OFFLINE_PROJECT_BUGREPORT".");
  args_add_module(args, &recog_module);
  args_add_module(args, &decoder_module);
  args_add_module(args, &lattice_module);
  args_add_shortcuts(args, shortcuts);

  args_parse_command_line(args, argc, argv);


  if (args_get_bool(args, "print-default", &error)) {
    args_write_default_config_file(args, stdout);
    args_delete(args);
    return EXIT_SUCCESS;
  }

  if (args_get_bool(args, "print-config", &error)) {
    args_dump(args, stderr);
  }

  //Word_graph
  if (args_get_bool(args, "save-lattices", NULL)) {
    char const *lattices_dn = args_get_string(args, "lattice-directory", NULL);
    REQUIRE(lattices_dn == NULL || directory_exists(lattices_dn), "lattice directory does not exist");
  }

  int start_line = 1;
  int end_line = 0;
  {
    //Open file of cepstrals
    const char *samples_fn = args_get_string(args, "samples", &error);

    REQUIRE(samples_fn != NULL, "Missing samples");

    FILE *samples_file = smart_fopen(samples_fn, "r");
    CHECK_SYS_ERROR(samples_file != NULL, "Couldn't open file of feature vectors '%s'\n", samples_fn);
    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, samples_file) != NULL) {
      if (strlen(strip(line)) > 0) end_line++;
    }
    smart_fclose(samples_file);

    const char *arg_part = args_get_string(args, "part", &error);
    if (error == ARG_OK && arg_part != NULL) {
      char *part = strdup(arg_part);
      char *saveprt = NULL, *endptr = NULL;
      int part_number = strtol(tokenize(part, ":", &saveprt), &endptr, 10);
      int n_parts = strtol(tokenize(NULL, ":", &saveprt), &endptr, 10);

      int part_size = end_line / n_parts;
      int part_mod = end_line % n_parts;
      int s = (part_number - 1) * part_size + 1 + ((part_number-1<part_mod)?part_number-1:part_mod);
      int e = (part_number) * part_size + ((part_number<part_mod)?part_number:part_mod);
      e = (e <= end_line) ? e : end_line;
      start_line = s;
      end_line = e;
      free(part);
    }
  }

  bool print_time = args_get_bool(args, "print-time", &error);


  SET_STATISTICS_VERBOSITY(args_get_int(args, "statistics-verbosity", &error));

  INIT_TRACE(args_get_int(args, "verbosity", &error));

  //Load all models
  decoder_t *decoder = decoder_create_from_args(args);

  {
    //Open file of cepstrals
    const char *vocab_fn = args_get_string(args, "save-vocab", &error);
    if (error == ARG_OK && vocab_fn != NULL) {
      FILE *vocab_file = smart_fopen(vocab_fn, "w");
      CHECK_SYS_ERROR(vocab_file != NULL, "Couldn't open vocab file '%s'\n", vocab_fn);
      extended_vocab_write(decoder->vocab, vocab_file);
      smart_fclose(vocab_file);
    }
  }


  if (!args_get_bool(args, "save-lattices", NULL)) {
    args_update(args, "lattice.nbest", "1");
    args_update(args, "lattice.nnode", "1");
  }

  if (args_get_string(args, "prefixes", NULL) != NULL) {
    grammar_build_word_search(decoder->grammar);
  }



  //Open file of cepstrals

  const char *samples_fn = args_get_string(args, "samples", &error);
  FILE *samples_file = smart_fopen(samples_fn, "r");
  CHECK_SYS_ERROR(samples_file != NULL, "Couldn't open file of feature vectors '%s'\n", samples_fn);

  const char *prefixes_fn = args_get_string(args, "prefixes", NULL);
  REQUIRE(prefixes_fn != NULL, "A file with the prefix for each sample is required");

  FILE *prefixes_f = NULL;
  prefixes_f = smart_fopen(prefixes_fn, "r");
  CHECK_SYS_ERROR(prefixes_f != NULL, "Couldn't open file of transcriptions '%s'\n", prefixes_fn);

  TRACE(0, "Start decoding...\n");
  fflush(stdout);

  //for each and every sample
  char line[MAX_LINE];
  int line_number = 1;
  while (fgets(line, MAX_LINE, samples_file) != NULL) {
    // do only this part
    if (line_number >= start_line && line_number <= end_line) {
      strip(line);

      char in_prefix_str[MAX_LINE];
      CHECK_SYS_ERROR(fgets(in_prefix_str, MAX_LINE, prefixes_f) != NULL, "Error reading from reference file");
      strip(in_prefix_str);

      symbol_t *in_prefix = NULL;
      if (in_prefix_str != NULL) {
        char prefix[1024] = "";
        if (decoder->grammar->start) {
          if (decoder->grammar->start != VOCAB_NONE) {
            sprintf(prefix, "%s ", extended_vocab_get_string(decoder->grammar->vocab, decoder->grammar->start));
          }
        }
        strcat(prefix, in_prefix_str);
        fprintf(stderr, "wordlist prefix = %s\n", prefix);
        in_prefix  = vocab_string_to_symbols(decoder->grammar->vocab->extended, prefix,  " ", CATEGORY_NONE);
      }

      //Read cepstrals
      features_t *feas = features_create_from_file(line);
      CHECK(decoder->hmm->num_features == feas->n_features
          || (feas->type == FT_EMISSION_PROBABILITIES && decoder->hmm->num_states == feas->n_features),
          "Invalid number of features in file '%s'\n", line);


      search_t *search = search_create(decoder);
      search_t *wordlist_search = search_create_wordlist_from_prefix(search, in_prefix);
      lattice_t *lattice = lattice_create_from_args(args, decoder);

      clock_t tim = clock();
      CALLGRIND_START_INSTRUMENTATION;
      decode(wordlist_search, feas, lattice);
      CALLGRIND_STOP_INSTRUMENTATION;
      clock_t tim2 = clock();
      if (print_time) {
        printf("xRT %f\n", ((float) ((tim2 - tim) / CLOCKS_PER_SEC) / search->n_frames) / 0.01);
      }
      fflush(stdout);

      //Calculate best hypothesis and word_graph
      outputs(line, args, lattice);

      lattice_delete(lattice);
      search_delete(wordlist_search);
      search_delete(search);
      features_delete(feas);
    }
    line_number++;
  }//while

  decoder_delete(decoder);
  args_delete(args);

  smart_fclose(samples_file);
  smart_fclose(prefixes_f);

  return EXIT_SUCCESS;
}
