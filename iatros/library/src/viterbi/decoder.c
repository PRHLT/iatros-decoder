/*
 * decoder.c
 *
 *  Created on: 19-abr-2009
 *      Author: valabau
 */

#include <prhlt/trace.h>
#include <prhlt/utils.h>
#include <prhlt/gzip.h>
#include <prhlt/args.h>
#include <prhlt/constants.h>
#include <viterbi/decoder.h>
#include <string.h>
#include <strings.h>
#include <math.h>

/** creates a new decoder
 * @return a new empty decoder
 */
decoder_t * decoder_new() {
  decoder_t *decoder;

  decoder = (decoder_t *)calloc(1, sizeof(decoder_t));
  MEMTEST(decoder);

  return decoder;
}

decoder_t *decoder_dup(const decoder_t *source) {
  decoder_t *decoder = (decoder_t *) malloc(sizeof(decoder_t));
  MEMTEST(decoder);
  *decoder = *source;

  return decoder;
}


/** creates a new decoder
@param file_categories A file with the name of category and the name of file with a fsm with the category
@param decoder Structure of decoder
 */
void load_categories(const char *file_categories, decoder_t *decoder) {
  //Structure of categories
  decoder->categories = (categories_t *) malloc(sizeof(categories_t) * 1);
  //Vector with categories
  decoder->categories->categories = NULL;
  decoder->categories->num_categories = 0;

  //File of fsm with categories
  FILE * grammar_file;

  //Open file of categories
  FILE *file = smart_fopen(file_categories, "r");
  CHECK_SYS_ERROR(file!= NULL, "Couldn't open file of categories '%s'\n", file_categories);

  //Read file
  char line[MAX_LINE];
  while (fgets(line, MAX_LINE, file) != 0) {
    line[strlen(line) - 1] = '\0';

    //Split the line
    char **splits = (char **) malloc(sizeof(char *) * MAX_LINE);
    split(line, splits, " ");

    grammar_file = smart_fopen(splits[1], "r");
    if (grammar_file == NULL) {
      printf("No file fsm with categories:\n%s", splits[1]);
      exit(1);
    }
    //Memory of vector of categories
    decoder->categories->categories = (grammar_t **) realloc(decoder->categories->categories, sizeof(grammar_t *) * (decoder->categories->num_categories + 1));

    //Category
    decoder->categories->categories[decoder->categories->num_categories] = grammar_create(decoder->lex, decoder->vocab);

    //Load fsm with the category
    grammar_load(decoder->categories->categories[decoder->categories->num_categories], grammar_file, FSM_GRAMMAR);

    //Insert the category in vocab->in with the number of category
    //   vocab_insert_symbol(decoder->vocab->in, splits[0], decoder->categories->num_categories);
    extended_vocab_insert_symbol(decoder->vocab, splits[0], decoder->categories->num_categories);
    // printf("%s %d\n", splits[0], decoder->categories->num_categories);
    //Write the number of category in vocab->in
    //   he_t *aux2=(he_t *)hs2(splits[0], strlen(splits[0]), decoder->vocab->in->hash);
    //   aux2->category=decoder->categories->num_categories;

    //Increase num_categories
    decoder->categories->num_categories++;

    //Free
    free(splits);
    smart_fclose(grammar_file);
  }
  smart_fclose(file);
}




/** Creates a new decoder with the info in args
 * @param params info used to initializate the decoder
 * @return a new decoder
 */
decoder_t * decoder_create_from_args(const args_t *args) {

  decoder_t *decoder = decoder_new();
  arg_error_t error;
  char const *value = NULL;

  // copy basic parameters
  decoder->gsf = args_get_float(args, DECODER_MODULE_NAME".grammar-scale-factor", &error);
  decoder->gsf_in = args_get_float(args, DECODER_MODULE_NAME".input-grammar-scale-factor", &error);
  decoder->gsf_out = args_get_float(args, DECODER_MODULE_NAME".output-grammar-scale-factor", &error);
  decoder->wip = args_get_float(args, DECODER_MODULE_NAME".word-insertion-penalty", &error);
  decoder->wip_out = args_get_float(args, DECODER_MODULE_NAME".output-word-insertion-penalty", &error);
  decoder->do_acoustic_early_pruning = args_get_bool(args, DECODER_MODULE_NAME".do-acoustic-early-pruning", &error);
  decoder->histogram_pruning = args_get_float(args, DECODER_MODULE_NAME".histogram-pruning", &error);
  decoder->beam_pruning = args_get_float(args, DECODER_MODULE_NAME".beam", &error);

  grammar_type_t grammar_type = NGRAM_GRAMMAR;
  {
    const char *grammar_type_str = args_get_string(args, "decoder.grammar-type", &error);
    if (error == ARG_OK && grammar_type_str != NULL) {
      grammar_type = get_grammar_type(grammar_type_str);
    } else {
      fprintf(stderr, "Wrong grammar type '%s'", grammar_type_str);
    }
  }


  const char *language_sep = NULL;
  const char *word_sep = NULL;

  value = args_get_string(args, DECODER_MODULE_NAME".language-separator", &error);
  if (error == ARG_OK && value != NULL) {
    language_sep = value;
  }

  value = args_get_string(args, DECODER_MODULE_NAME".word-separator", &error);
  if (error == ARG_OK && value != NULL) {
    word_sep = value;
  }

  bool is_phrase_table = false;
  value = args_get_string(args, DECODER_MODULE_NAME".phrase-table", &error);
  if (error == ARG_OK && value != NULL) is_phrase_table = true;

  if (is_phrase_table && language_sep == NULL) language_sep = "|||";
  if (is_phrase_table && word_sep == NULL) language_sep = "_";


  FILE *file;

//  decoder->create_dummy_acoustic_models = args_get_bool(args, DECODER_MODULE_NAME".create-dummy-acoustic-models", NULL);
//  if (params->create_dummy_acoustic_models) {
//    REQUIRE(params->transcriptions != NULL,
//        "Dummy acoustic models have been enables but transcriptions are not present");
//  }

  TRACE(1, "Loading HMM models...\n");
  //Create hmm
  decoder->hmm = hmm_create();

  //Copy the information of the file_hmm in the structure hmm
  value = args_get_string(args, DECODER_MODULE_NAME".hmm", &error);
  REQUIRE(error == ARG_OK && value != NULL, "The HMM models are missing");
  file = smart_fopen(value, "r");
  CHECK_SYS_ERROR(file != NULL, "Couldn't open hmm file '%s'\n", value);

  // load hmm from file
  hmm_load(decoder->hmm, file);
  smart_fclose(file);

  TRACE(1, "Loading lexicon...\n");
  //Create vocab
  value = args_get_string(args, DECODER_MODULE_NAME".unk", &error);
  vocab_t *input_vocab = vocab_create(541, value);

  extended_vocab_t *vocab = NULL;
  //XXX: should we create a new parameter to enable translation?
  if (grammar_type == FSM_GRAMMAR) {
    vocab = extended_vocab_create(input_vocab, input_vocab->unk_word, " ", NULL);
  }
  else {
    vocab = extended_vocab_create(input_vocab, input_vocab->unk_word, word_sep, language_sep);
  }

  //Create lex
  decoder->lex = lex_create(input_vocab, decoder->hmm);

  //Copy the information of the file_lex in the structure lex
  value = args_get_string(args, DECODER_MODULE_NAME".lexicon", &error);
  REQUIRE(error == ARG_OK && value != NULL, "The lexicon is missing");
  file = smart_fopen(value, "r");
  CHECK_SYS_ERROR(file != NULL, "Couldn't open lexicon file '%s'\n", value);
  // load lex from file


  // assign the silence word
  value = args_get_string(args, DECODER_MODULE_NAME".lexicon-type", &error);
  REQUIRE (error == ARG_OK && value != NULL, "Unknown lexicon type");

  if (strcasecmp(value, "ATROS") == 0) {
    // assign the start word
    value = args_get_string(args, DECODER_MODULE_NAME".start", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      extended_vocab_insert_symbol(vocab, value, CATEGORY_NONE);
    }

    // assign the end word
    value = args_get_string(args, DECODER_MODULE_NAME".end", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      extended_vocab_insert_symbol(vocab, value, CATEGORY_NONE);
    }

    // assign the silence word
    value = args_get_string(args, DECODER_MODULE_NAME".silence", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      extended_vocab_insert_symbol(vocab, value, CATEGORY_NONE);
    }

    // assign the pause word
    value = args_get_string(args, DECODER_MODULE_NAME".pause", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      extended_vocab_insert_symbol(vocab, value, CATEGORY_NONE);
    }

    lex_load(decoder->lex, file);
  }

  else if (strcasecmp(value, "HTK") == 0) {
    dict_load(decoder->lex, file);

    // assign the start word
    value = args_get_string(args, DECODER_MODULE_NAME".start", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      extended_vocab_insert_symbol(vocab, value, CATEGORY_NONE);
    }

    // assign the end word
    value = args_get_string(args, DECODER_MODULE_NAME".end", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      extended_vocab_insert_symbol(vocab, value, CATEGORY_NONE);
    }

    // assign the silence word
    value = args_get_string(args, DECODER_MODULE_NAME".silence", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      extended_vocab_insert_symbol(vocab, value, CATEGORY_NONE);
    }

    // assign the pause word
    value = args_get_string(args, DECODER_MODULE_NAME".pause", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      extended_vocab_insert_symbol(vocab, value, CATEGORY_NONE);
    }


  }
  else {
    REQUIRE(false, "Unknown lexicon type");
  }
  smart_fclose(file);

  TRACE(1, "Loading grammar...\n");
  decoder->grammar = grammar_create(decoder->lex, vocab);

  {
    // assign the silence word
    value = args_get_string(args, DECODER_MODULE_NAME".silence", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      decoder->grammar->silence = extended_vocab_find_symbol(vocab, value);
      REQUIRE(decoder->grammar->silence != VOCAB_NONE, "ERROR: '%s' is not in lexic\n", value);
      decoder->grammar->force_silence = args_get_bool(args, DECODER_MODULE_NAME".force-silence", &error);
    }
    decoder->grammar->silence_score = args_get_float(args, DECODER_MODULE_NAME".silence-score", &error);

    // assign the silence word
    value = args_get_string(args, DECODER_MODULE_NAME".pause", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      decoder->grammar->pause = extended_vocab_find_symbol(vocab, value);
      REQUIRE(decoder->grammar->pause != VOCAB_NONE, "ERROR: '%s' is not in lexic\n", value);
    }

    // assign the start word
    value = args_get_string(args, DECODER_MODULE_NAME".start", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      decoder->grammar->start = extended_vocab_find_symbol(vocab, value);
      REQUIRE(decoder->grammar->start != VOCAB_NONE, "ERROR: '%s' is not in lexic\n", value);
    }

    // assign the end word
    value = args_get_string(args, DECODER_MODULE_NAME".end", &error);
    if (error == ARG_OK && value != NULL && strcmp(value, "") != 0) {
      decoder->grammar->end = extended_vocab_find_symbol(vocab, value);
      REQUIRE(decoder->grammar->end != VOCAB_NONE, "ERROR: '%s' is not in lexic\n", value);
    }
  }

  {
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


  //Load grammar
  value = args_get_string(args, DECODER_MODULE_NAME".grammar", &error);
  if (error != ARG_OK) {
    value = args_get_string(args, DECODER_MODULE_NAME".phrase-table", &error);
    grammar_type = PHRASE_TABLE_GRAMMAR;
  }

  REQUIRE(error == ARG_OK && value != NULL, "The grammar is missing");
  file = smart_fopen(value, "r");
  CHECK_SYS_ERROR(file != NULL, "Couldn't open grammar file '%s'\n", value);
  grammar_load(decoder->grammar, file, grammar_type);

  smart_fclose(file);


  // load phrasetable score and discard grammar
  value = args_get_string(args, DECODER_MODULE_NAME".phrase-table", &error);
  if (error == ARG_OK && value != NULL) {
    file = smart_fopen(value, "r");
    CHECK_SYS_ERROR(file != NULL, "Couldn't open phrasetable file '%s'\n", value);

    grammar_t *dummy_grammar = grammar_create(decoder->lex, vocab);
    grammar_load(dummy_grammar, file, PHRASE_TABLE_GRAMMAR);
    grammar_delete(dummy_grammar);
    smart_fclose(file);
  }

  value = args_get_string(args, DECODER_MODULE_NAME".input-grammar", &error);
  if (error == ARG_OK && value != NULL) {
    decoder->input_grammar = grammar_create_secondary(decoder->grammar, GV_INPUT);
    file = smart_fopen(value, "r");
    CHECK_SYS_ERROR(file != NULL, "Couldn't open input grammar file '%s'\n", value);
    grammar_load(decoder->input_grammar, file, NGRAM_GRAMMAR);
    grammar_build_word_search(decoder->input_grammar);
    smart_fclose(file);
  }

  value = args_get_string(args, DECODER_MODULE_NAME".output-grammar", &error);
  if (error == ARG_OK && value != NULL) {
    decoder->output_grammar = grammar_create_secondary(decoder->grammar, GV_OUTPUT);
    file = smart_fopen(value, "r");
    CHECK_SYS_ERROR(file != NULL, "Couldn't open output grammar file '%s'\n", value);
    grammar_load(decoder->output_grammar, file, NGRAM_GRAMMAR);
    grammar_build_word_search(decoder->output_grammar);
    smart_fclose(file);
  }
  decoder->vocab = decoder->grammar->vocab;

  //Categories
  value = args_get_string(args, DECODER_MODULE_NAME".categories", &error);
  if(error == ARG_OK && value != NULL){
   load_categories(value, decoder);
  }

  //check if lexicon is ok
  REQUIRE(lex_check_vocab(decoder->lex) == 0, "ERROR: There are words in the input vocabulary that are not in the lexicon");

  // insert !NULL symbol for lattices
  extended_vocab_insert_symbol(vocab, "!NULL", CATEGORY_NONE);

  return decoder;
}



/** Creates a new decoder from a config file
 * @param file a pointer file
 * @return a new decoder
 */
decoder_t * decoder_create_from_file(FILE *file) {
  args_t *args = args_create();
  args_update_from_file(args, file);
  decoder_t *decoder = decoder_create_from_args(args);
  args_delete(args);
  return decoder;
}


/// Deletes categories
/**
@param categories Categories
*/
void categories_delete(categories_t * categories){

 for(int i=0; i<categories->num_categories; i++){
  grammar_delete(categories->categories[i]);
 }
 free(categories->categories);
 free(categories);
}

/// Deletes the decoder
/**
@param decoder the decoder
*/
void decoder_delete(decoder_t *decoder) {
  if (decoder->input_grammar != NULL) grammar_delete(decoder->input_grammar);
  if (decoder->output_grammar != NULL) grammar_delete(decoder->output_grammar);
  grammar_delete(decoder->grammar);
  hmm_delete(decoder->hmm);
  extended_vocab_delete(decoder->vocab);
  lex_delete(decoder->lex);
  if(decoder->categories!=NULL){
   categories_delete(decoder->categories);
  }
  free(decoder);
}

