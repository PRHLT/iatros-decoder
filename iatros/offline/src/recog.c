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
    if (lattice_file != NULL){
        lattice_write(lattice, lattice_file, path);
        smart_fclose(lattice_file);
    }
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
      {"transcriptions", ARG_STRING, NULL, 0, "List of transcriptions for the samples"},
      {"translations", ARG_STRING, NULL, 0, "List of translations for the samples"},
      {"save-vocab", ARG_STRING, NULL, 0, "Save vocabulary to file"},
      {"do-forced-recognition", ARG_BOOL, NULL, 0, "Enables forced recognition"},
      {"print-time", ARG_BOOL, "false", 0, "Print realtime factor"},
      {"print-score", ARG_BOOL, "false", 0, "Print hypothesis score"},
      {"part", ARG_STRING, NULL, 0, "Split the corpus into parts and run just one. Ex. --part '1:4' runs the 1st part out of 4"},
      {"verbosity", ARG_INT, "0", 0, "Set verbosity level"},
      {"statistics-verbosity", ARG_INT, "0", 0, "Set statistics verbosity level"},
      {"print-default", ARG_BOOL, "false", 0, "Print default config file"},
      {"print-config", ARG_BOOL, "false", 0, "Print final config file"},
      {"energy-threshold", ARG_FLOAT, NULL, 0, "Set to zero all feature vectors with energy (as in CC) less than the threshold"},
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
    {"w", "decoder.word-insertion-penalty"},
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

  reference_t ref_type = REF_NONE;
  arg_error_t error = ARG_OK;

  args_t *args = args_create();
  args_set_summary(args, "Speech/Handwritten text recognition/translation toolkit");
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
  bool do_forced_recognition = args_get_bool(args, "do-forced-recognition", &error);


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

  if (args_get_string(args, "transcriptions", NULL) != NULL || args_get_string(args, "translations", NULL) != NULL) {
    grammar_build_word_search(decoder->grammar);
  }


  float energy_threshold = args_get_float(args, "energy-threshold", &error);
  bool has_energy_threshold = false;
  if (error == ARG_OK) {
    TRACE(0, "Set energy filtering with threshold %g\n", energy_threshold);
    has_energy_threshold = true;
  }


  //Open file of cepstrals

  const char *samples_fn = args_get_string(args, "samples", &error);
  FILE *samples_file = smart_fopen(samples_fn, "r");
  CHECK_SYS_ERROR(samples_file != NULL, "Couldn't open file of feature vectors '%s'\n", samples_fn);

  const char *transcriptions = args_get_string(args, "transcriptions", NULL);
  const char *translations = args_get_string(args, "translations", NULL);
  const char *prefixes = args_get_string(args, "prefixes", NULL);
  REQUIRE(transcriptions == NULL || translations == NULL || prefixes == NULL,
          "Transcriptions and translations and prefixes are not allowed to be given at the same time");

  FILE *refs_f = NULL;
  if (transcriptions != NULL) {
    refs_f = smart_fopen(transcriptions, "r");
    CHECK_SYS_ERROR(refs_f != NULL, "Couldn't open file of transcriptions '%s'\n", transcriptions);
    ref_type = REF_SOURCE;
  }
  else if (translations != NULL) {
    refs_f = smart_fopen(translations, "r");
    CHECK_SYS_ERROR(refs_f != NULL, "Couldn't open file of translations '%s'\n", translations);
    ref_type = REF_TARGET;
  }
  else if (prefixes != NULL) {
    refs_f = smart_fopen(prefixes, "r");
    CHECK_SYS_ERROR(refs_f != NULL, "Couldn't open file of prefixes'%s'\n", translations);
    ref_type = REF_PREFIX;
  }

  if (ref_type != REF_NONE && refs_f != NULL) {
    if (args_get_bool(args, "do-forced-recognition", &error)) {
      TRACE(0, "Forced recognition...\n");
    }
    else if (ref_type == REF_PREFIX) {
      TRACE(0, "Prefix recognition...\n");
    }
    else {
      TRACE(0, "Iterative recognition...\n");
    }
  }
  else {
    TRACE(0, "Start decoding...\n");
  }
  fflush(stdout);

  //for each and every sample
  char line[MAX_LINE];
  int line_number = 1;
  while (fgets(line, MAX_LINE, samples_file) != NULL) {
    // do only this part
    if (line_number >= start_line && line_number <= end_line) {
      strip(line);

      //Read cepstrals
      TRACE(0, "Feature file: '%s'\n", line);
      features_t *feas = features_create_from_file(line);
      if (has_energy_threshold) features_filter_energy_threshold(feas, energy_threshold);

      CHECK(decoder->hmm->num_features == feas->n_features
          || (feas->type == FT_EMISSION_PROBABILITIES && decoder->hmm->num_states == feas->n_features),
          "Invalid number of features in file '%s'\n", line);

      search_t *search = search_create(decoder);
      lattice_t *lattice = lattice_create_from_args(args, decoder);

      // if there are references, perform a cat decoding
      if (ref_type != REF_NONE && refs_f != NULL) {
        char readed_ref[MAX_LINE];
        CHECK_SYS_ERROR(fgets(readed_ref, MAX_LINE, refs_f) != NULL, "Error reading from reference file");
        strip(readed_ref);

        char ref_str[MAX_LINE] = "";
//        if (decoder->grammar->start != VOCAB_NONE) {
//          const extended_symbol_t *start = extended_vocab_get_extended_symbol(decoder->vocab, decoder->grammar->start);
//          if ((ref_type == REF_SOURCE || ref_type == REF_PREFIX) && start->input != NULL) {
//            sprintf(ref_str, "%s ", vocab_get_string(decoder->vocab->in, start->input[0]));
//          }
//          else if (ref_type == REF_TARGET && start->output != NULL) {
//            sprintf(ref_str, "%s ", vocab_get_string(decoder->vocab->out, start->output[0]));
//          }
//        }
        strcat(ref_str, readed_ref);
        strcat(ref_str, " ");
        if (decoder->grammar->end != VOCAB_NONE) {
          const extended_symbol_t *end = extended_vocab_get_extended_symbol(decoder->vocab, decoder->grammar->end);
          if ((ref_type == REF_SOURCE) && end->input != NULL) {
          //if ((ref_type == REF_SOURCE || ref_type == REF_PREFIX) && end->input != NULL) {
            strcat(ref_str, vocab_get_string(decoder->vocab->in, end->input[0]));
          }
          else if (ref_type == REF_TARGET && end->output != NULL) {
            strcat(ref_str, vocab_get_string(decoder->vocab->out, end->output[0]));
          }
        }
        strip(ref_str);
        fprintf(stderr, "ref: %s\n", ref_str);

        symbol_t *ref = NULL;
        if (ref_type == REF_SOURCE) {
          ref = vocab_string_to_symbols(decoder->vocab->in, ref_str,  " ", CATEGORY_NONE);
        }
        else if (ref_type == REF_TARGET) {
          ref = vocab_string_to_symbols(decoder->vocab->out, ref_str,  " ", CATEGORY_NONE);
        }
        else if (ref_type == REF_PREFIX) {
          ref = vocab_string_to_symbols(decoder->vocab->extended, ref_str,  " ", CATEGORY_NONE);
        }

        if (do_forced_recognition) {
          search_create_emission_cache(search);
          search_t *prefix_search = NULL;
          if (ref_type == REF_SOURCE || ref_type == REF_PREFIX) {
            prefix_search = search_create_from_prefix(search, ref, NULL);
          }
          else if (ref_type == REF_TARGET) {
            prefix_search = search_create_from_prefix(search, NULL, ref);
          }
          else {
            REQUIRE(ref_type > REF_NONE && ref_type < REF_MAX, "Invalid reference type\n");
          }
          CHECK(prefix_search->decoder->grammar->list_initial->num_elements > 0, "Empty prefix grammar. Possible lack of coverture\n");
          //fprintf(stderr, "n initials = %d, n_states = %d\n", prefix_search->decoder->grammar->list_initial->num_elements, prefix_search->decoder->grammar->num_states);
          //grammar_write_dot(prefix_search->decoder->grammar, stderr);
          lattice_t *prefix_lattice = lattice_create(lattice->nnode, lattice->nbest, prefix_search->decoder);

          clock_t tim = clock();
          CALLGRIND_START_INSTRUMENTATION;
          decode(prefix_search, feas, prefix_lattice);
          CALLGRIND_STOP_INSTRUMENTATION;
          clock_t tim2 = clock();
          if (print_time) {
            printf("xRT %f\n", ((float) ((tim2 - tim) / CLOCKS_PER_SEC) / prefix_search->n_frames) / 0.01);
          }

          //Calculate best hypothesis and word_graph
          outputs(line, args, prefix_lattice);
          lattice_delete(prefix_lattice);
          search_delete(prefix_search);
        }
        else {
          int ref_len = symlen(ref) - ((decoder->grammar->start != VOCAB_NONE)?1:0)
                                    - ((decoder->grammar->end != VOCAB_NONE)?1:0);


          CALLGRIND_START_INSTRUMENTATION;
          bool decode_word = false;
          int n_iter;
          if (decode_word) {
            char *lattice_tmpl = NULL;
  
            if (args_get_bool(args, "save-lattices", NULL)) {
              lattice_tmpl = (char *) malloc(MAX_LINE * sizeof(char));
  
              //Name of file with word_graph
              char const *lattices_dn = args_get_string(args, "lattice-directory", NULL);
              if (lattices_dn == NULL) lattices_dn = ".";
              sprintf(lattice_tmpl, "%s/%s_%%d.lat.gz", lattices_dn, basename(line));
              fprintf(stderr, "template: %s/%s_%%d.lat.gz\n", lattices_dn, basename(line));
            }

            n_iter = cat_decode_word(search, feas, lattice, ref, ref_type, lattice_tmpl);
            if (lattice_tmpl) free(lattice_tmpl);
          }
          else {
            n_iter = cat_decode(search, feas, lattice, ref, ref_type);
          }
          CALLGRIND_STOP_INSTRUMENTATION;

          if (ref_type == REF_SOURCE || ref_type == REF_TARGET) {
            printf("WSR = %d/%d = %g\n\n", n_iter, ref_len, (float)n_iter/(float)ref_len);
          }
          else {
            //Calculate best hypothesis and word_graph
            outputs(line, args, lattice);
          }
        }
        free(ref);
      }
      // if there are no references, perform a normal decoding
      else {
        clock_t tim = clock();
        CALLGRIND_START_INSTRUMENTATION;
        decode(search, feas, lattice);
        CALLGRIND_STOP_INSTRUMENTATION;
        clock_t tim2 = clock();
        if (print_time) {
          printf("xRT %f\n", ((float) ((tim2 - tim) / CLOCKS_PER_SEC) / search->n_frames) / 0.01);
        }

        //Calculate best hypothesis and word_graph
        outputs(line, args, lattice);
      }
      fflush(stdout);
      lattice_delete(lattice);
      search_delete(search);
      features_delete(feas);
    }
    line_number++;
  }//while

  decoder_delete(decoder);
  args_delete(args);

  smart_fclose(samples_file);
  smart_fclose(refs_f);

  return EXIT_SUCCESS;
}
