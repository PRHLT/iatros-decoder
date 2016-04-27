/*
 * grammar2dot.c
 *
 *  Created on: 12-feb-2009
 *      Author: valabau
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

#include <config.h>
#include <iatros/version.h>
#include <iatros/viterbi.h>
#include <prhlt/trace.h>
#include <prhlt/gzip.h>
#include <prhlt/vocab.h>
#include <iatros/features.h>
#include <iatros/lattice.h>
#include <iatros/cat.h>
#include <prhlt/constants.h>


static const arg_module_t grammar_tool_module = {NULL, "General options",
    {
      {"wordlist", ARG_BOOL, "false", 0, "Get wordlist instead of subgrammar"},
      {"prefix-conditioned", ARG_BOOL, "false", 0, "Get prefix conditioned n-gram instead of subgrammar"},
      {"prefix", ARG_STRING, NULL, 0, "Get subgrammar that matches prefix"},
      {"output-prefix", ARG_STRING, NULL, 0, "Get subgrammar that matches output prefix"},
      {"output", ARG_STRING, NULL, 0, "Output file. If not set, standard output"},
      {"verbosity", ARG_INT, "0", 0, "Set verbosity level"},
      {"format", ARG_STRING, "slf", 0, "Format in wich the grammar will be printed (slf*, dot)"},
      {"save-vocab", ARG_STRING, NULL, 0, "Save vocabulary to file"},
      {"save-slf-grammar", ARG_STRING, NULL, 0, "Save grammar to slf file"},
      {NULL, ARG_END_MODULE, NULL, 0, NULL}
    }
};

static const arg_shortcut_t shortcuts[] = {
    {"c", "config"},
    {"v", "verbosity"},
    {"w", "wordlist"},
    {"p", "prefix"},
    {"P", "output-prefix"},
    {"o", "output"},
    {"f", "format"},
    {"g", "decoder.grammar"},
    {"t", "decoder.grammar-type"},
    {"l", "decoder.weights"},
    {"s", "decoder.start"},
    {"e", "decoder.end"},
    {NULL, NULL}
};



int main(int argc, char *argv[]) {

  arg_error_t error = ARG_OK;

  args_t *args = args_create();
  args_set_summary(args, "Toolkit to process grammars");
  args_set_doc(args, "For more info see http://prhlt.iti.es");
  args_set_version(args, IATROS_OFFLINE_PROJECT_STRING"\n"IATROS_OFFLINE_BUILD_INFO"\n\n"
                         IATROS_PROJECT_STRING"\n"IATROS_BUILD_INFO);
  args_set_bug_report(args, "Report bugs to "IATROS_OFFLINE_PROJECT_BUGREPORT".");
  args_add_module(args, &grammar_tool_module);
  args_add_module(args, &decoder_module);
  args_add_shortcuts(args, shortcuts);
  args_parse_command_line(args, argc, argv);


  const char *grammar_fn = args_get_string(args, "decoder.grammar", &error);
  if (error != ARG_OK || grammar_fn == NULL) {
    grammar_fn = args_get_string(args, "decoder.phrase-table", &error);
    if (error != ARG_OK || grammar_fn == NULL) {
      fprintf(stderr, "ERROR: grammar missing\n");
      args_usage(args, stderr);
      args_delete(args);
      return EXIT_FAILURE;
    }
  }

  FILE *file = stdout;
  const char *output_fn = args_get_string(args, "output", &error);
  if (output_fn != NULL) {
    file = smart_fopen(output_fn, "w");
    CHECK_SYS_ERROR(file != NULL, "Couldn't create file '%s'\n", output_fn);
  }

  INIT_TRACE(args_get_int(args, "verbosity", &error));

  const bool is_wordlist = args_get_bool(args, "wordlist", &error);
  REQUIRE(error == ARG_OK, "Wrong wordlist argument");

  const bool is_prefix_conditioned = args_get_bool(args, "prefix-conditioned", &error);
  REQUIRE(error == ARG_OK, "Wrong prefix-conditioned argument");

  vocab_t *vocab_in = vocab_create(4321, (is_wordlist)?NULL:unk_word);
  extended_vocab_t *vocab = extended_vocab_create(vocab_in, (is_wordlist)?NULL:unk_word, (is_wordlist)?" ":"_", (is_wordlist)?NULL:"|||");
  grammar_t *grammar = grammar_create(NULL, vocab);
  const char *start = args_get_string(args, "decoder.start", &error);
  if (error == ARG_OK && start != NULL && strlen(start) > 0) {
    grammar->start = extended_vocab_insert_symbol(vocab, start, CATEGORY_NONE);
  }
  else {
    fprintf(stderr, "Start word not set\n");
    grammar->start = VOCAB_NONE;
  }
  const char *end = args_get_string(args, "decoder.end", &error);
  if (error == ARG_OK && end != NULL && strlen(end) > 0) {
    grammar->end   = extended_vocab_insert_symbol(vocab, end, CATEGORY_NONE);
  }
  else {
    fprintf(stderr, "End word not set\n");
    grammar->end   = VOCAB_NONE;
  }

  grammar_type_t type = MAX_GRAMMAR_TYPE;
  {
    const char *grammar_type_str = args_get_string(args, "decoder.grammar-type", &error);
    if (error == ARG_OK && grammar_type_str != NULL) {
      type = get_grammar_type(grammar_type_str);
    } else {
      fprintf(stderr, "Wrong grammar type '%s'", grammar_type_str);
      args_usage(args, stderr);
    }
  }

  if (type == PHRASE_TABLE_GRAMMAR) {
    const char *weights_arg = args_get_string(args, DECODER_MODULE_NAME".weights", &error);
    float *weights = NULL;
    int n_weights = 0;
    if (error == ARG_OK && weights_arg != NULL) {
      char *weights_str = strdup(weights_arg);
      char *save_str = NULL;
      char *score_str = tokenize(weights_str, ",", &save_str);
      while (score_str != NULL) {
        weights = (float *) realloc(weights, (n_weights + 1) * sizeof(float));
        weights[n_weights] = strtod(score_str, NULL);
        CHECK_SYS_ERROR(weights[n_weights] != HUGE_VAL, "Invalid number");
        n_weights++;
        score_str = tokenize(NULL, ",", &save_str);
      }
      extended_vocab_set_weights(vocab, n_weights, weights);
      free(weights_str);
      free(weights);
    }
  }

  {
    FILE *grammar_file = smart_fopen(grammar_fn, "r");
    CHECK_SYS_ERROR(grammar_file != NULL, "Couldn't create file '%s'\n", output_fn);
    grammar_load(grammar, grammar_file, type);
    smart_fclose(grammar_file);
  }

  extended_vocab_set_closed(vocab, true);

  {
    //Open file of cepstrals
    const char *vocab_fn = args_get_string(args, "save-vocab", &error);
    if (error == ARG_OK && vocab_fn != NULL) {
      FILE *vocab_file = smart_fopen(vocab_fn, "w");
      CHECK_SYS_ERROR(vocab_file != NULL, "Couldn't open vocab file '%s'\n", vocab_fn);
      extended_vocab_write(vocab, vocab_file);
      smart_fclose(vocab_file);
    }
  }

  {
    const char *slf_grammar_fn = args_get_string(args, "save-slf-grammar", &error);
    if (error == ARG_OK && slf_grammar_fn != NULL) {
      FILE *slf_grammar_file = smart_fopen(slf_grammar_fn, "w");
      CHECK_SYS_ERROR(slf_grammar_file != NULL, "Couldn't open slf grammar file '%s' for writing\n", slf_grammar_fn);
      grammar_write_slf(grammar, slf_grammar_file);
      smart_fclose(slf_grammar_file);
    }
  }

  const char *in_prefix_str = args_get_string(args, "prefix", &error);
  const char *out_prefix_str = args_get_string(args, "output-prefix", &error);
  if (is_wordlist) {
    symbol_t *in_prefix = NULL;
    if (in_prefix_str != NULL) {
      char prefix[1024] = "";
      if (grammar->start && grammar->start != VOCAB_NONE) {
        sprintf(prefix, "%s ", extended_vocab_get_string(vocab, grammar->start));
      }
      strcat(prefix, in_prefix_str);
      fprintf(stderr, "wordlist prefix = %s\n", prefix);
      in_prefix  = vocab_string_to_symbols(vocab->extended, prefix,  " ", CATEGORY_NONE);
    }
    grammar_t *prefix_grammar = grammar_create_wordlist_from_prefix(grammar, in_prefix);
    if (strcmp(args_get_string(args, "format", &error), "dot") == 0) {
      grammar_write_dot(prefix_grammar, file);
    }
    else {
      grammar_write_slf(prefix_grammar, file);
    }
    grammar_delete(prefix_grammar);
  }
  else if (in_prefix_str != NULL || out_prefix_str != NULL) {
    symbol_t *in_prefix = NULL;
    symbol_t *out_prefix = NULL;

    if (in_prefix_str != NULL) {
      char prefix[1024] = "";
//      if (grammar->start && grammar->start != VOCAB_NONE) {
//        if (extended_vocab_get_extended_symbol(vocab, grammar->start)->input != NULL) {
//          sprintf(prefix, "%s ", vocab_get_string(vocab->in,
//                                   extended_vocab_get_extended_symbol(vocab, grammar->start)->input[0]));
//        }
//      }
      strcat(prefix, in_prefix_str);
//       if (grammar->end && grammar->end != VOCAB_NONE) {
//         char buffer[1024] = "";
//         if (extended_vocab_get_extended_symbol(vocab, grammar->end)->input != NULL) {
//           sprintf(buffer, " %s", vocab_get_string(vocab->in,
//                                    extended_vocab_get_extended_symbol(vocab, grammar->end)->input[0]));
//         }
//         strcat(prefix, buffer);
//       }
      fprintf(stderr, "full prefix = %s\n", prefix);
      in_prefix  = vocab_string_to_symbols(vocab->in, prefix,  " ", CATEGORY_NONE);
    }

    if (out_prefix_str != NULL) {
      char prefix[1024] = "";
      if (grammar->start && grammar->start != VOCAB_NONE) {
        if (extended_vocab_get_extended_symbol(vocab, grammar->start)->output != NULL) {
          sprintf(prefix, "%s ", vocab_get_string(vocab->out,
                                 extended_vocab_get_extended_symbol(vocab, grammar->start)->output[0]));
        }
      }
      strcat(prefix, out_prefix_str);
      // if (grammar->end && grammar->end != VOCAB_NONE) {
      //   char buffer[1024];
      //   if (extended_vocab_get_extended_symbol(vocab, grammar->end)->output != NULL) {
      //     sprintf(buffer, " %s", vocab_get_string(vocab->out,
      //                              extended_vocab_get_extended_symbol(vocab, grammar->end)->output[0]));
      //   }
      //   strcat(prefix, buffer);
      // }
      out_prefix = vocab_string_to_symbols(vocab->out, prefix, " ", CATEGORY_NONE);
    }

    if (in_prefix) {
      char *prefix_str = NULL;
      if (is_prefix_conditioned) {
        vocab_symbols_to_string(in_prefix, grammar->vocab->extended, &prefix_str);
      }
      else {
        vocab_symbols_to_string(in_prefix, grammar->vocab->in, &prefix_str);
      }
      fprintf(stderr, "Search in prefix: %s\n", prefix_str);
      free(prefix_str);
    }

    if (out_prefix) {
      char *prefix_str = NULL;
      vocab_symbols_to_string(out_prefix, grammar->vocab->out, &prefix_str);
      fprintf(stderr, "Search out prefix: %s\n", prefix_str);
      free(prefix_str);
    }

    grammar_t *prefix_grammar;
    if (is_prefix_conditioned) {
       prefix_grammar = grammar_create_conditioned_to_prefix(grammar, in_prefix);
    }
    else {
      prefix_grammar = grammar_create_from_prefix(grammar, in_prefix, out_prefix);
    } 
    free(in_prefix);
    free(out_prefix);

    if (strcmp(args_get_string(args, "format", &error), "dot") == 0) {
      grammar_write_dot(prefix_grammar, file);
    }
    else {
      grammar_write_slf(prefix_grammar, file);
    }

    grammar_delete(prefix_grammar);
  }
  else {
    if (strcmp(args_get_string(args, "format", &error), "dot") == 0) {
      grammar_write_dot(grammar, file);
    }
    else {
      grammar_write_slf(grammar, file);
    }
  }

  smart_fclose(file);
  args_delete(args);
  extended_vocab_delete(vocab);
  grammar_delete(grammar);

  return EXIT_SUCCESS;
}
