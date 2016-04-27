/*
 * decoder.h
 *
 *  Created on: 19-abr-2009
 *      Author: valabau
 */

#ifndef DECODER_H_
#define DECODER_H_

#include <iatros/grammar.h>
#include <prhlt/args.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DECODER_MODULE_NAME "decoder"

static const arg_module_t decoder_module = { DECODER_MODULE_NAME, "decoder module",
    {
        {"hmm", ARG_FILE, NULL, ARG_FLAGS_NONE, "Hidden Markov Model in HTK format"},
        {"lexicon", ARG_FILE, NULL, ARG_FLAGS_NONE, "Lexicon file"},
        {"lexicon-type", ARG_STRING, "ATROS", ARG_FLAGS_NONE, "Lexicon format (ATROS, HTK)"},
        {"grammar", ARG_FILE, NULL, ARG_FLAGS_NONE, "Main grammar"},
        {"grammar-type", ARG_STRING, "NGRAM", ARG_FLAGS_NONE, "Type of the main grammar (FSM, NGRAM). NGRAM by default"},
        {"input-grammar", ARG_FILE, NULL, ARG_FLAGS_NONE, "Input grammar"},
        {"output-grammar", ARG_FILE, NULL, ARG_FLAGS_NONE, "Output grammar"},

        {"beam", ARG_FLOAT, "1e+30", ARG_FLAGS_NONE, "Relative beam"},
        {"grammar-scale-factor", ARG_FLOAT, "1", ARG_FLAGS_NONE, "Grammar scale factor"},
        {"input-grammar-scale-factor", ARG_FLOAT, "1", ARG_FLAGS_NONE, "Grammar scale factor for the input grammar"},
        {"output-grammar-scale-factor", ARG_FLOAT, "1", ARG_FLAGS_NONE, "Grammar scale factor for the output grammar"},
        {"word-insertion-penalty", ARG_FLOAT, "0", ARG_FLAGS_NONE, "Word insertion penalty"},
        {"output-word-insertion-penalty", ARG_FLOAT, "0", ARG_FLAGS_NONE, "Word insertion penalty for output words"},

        {"start", ARG_STRING, NULL, ARG_FLAGS_NONE, "Start symbol. None by default"},
        {"end", ARG_STRING, NULL, ARG_FLAGS_NONE, "End symbole. None by default"},
        {"unk", ARG_STRING, NULL, ARG_FLAGS_NONE, "Unknown symbol. None by default"},
        {"silence", ARG_STRING, NULL, ARG_FLAGS_NONE, "Silence symbol. None by default"},
        {"pause", ARG_STRING, NULL, ARG_FLAGS_NONE, "Short pause symbol. None by default"},
        {"silence-score", ARG_FLOAT, "0.0", ARG_FLAGS_NONE, "Score (log probability) for the silence and short pause. '0.0' by default"},
        {"force-silence", ARG_BOOL, false, ARG_FLAGS_NONE, "Force silence at beginning and end of recognition"},
        {"language-separator", ARG_STRING, NULL, ARG_FLAGS_NONE, "String that separates languages in a phrase, i.e. '|||'. Disabled by default."},
        {"word-separator", ARG_STRING, NULL, ARG_FLAGS_NONE, "String that separates words in a language phrase, i.e. '_'. Disabled by default."},

        {"histogram-pruning", ARG_INT, "10000", ARG_FLAGS_NONE, "Maximum number of hypotheses allowed by frame."},

        {"phrase-table", ARG_FILE, NULL, ARG_FLAGS_NONE, "Phrase table in moses format"},
        {"weights", ARG_STRING, NULL, ARG_FLAGS_NONE, "Weights for the log-lineal model in the phrase table separated by commas"},

        {"do-acoustic-early-pruning", ARG_BOOL, "true", ARG_FLAGS_NONE, "Enables acoustic early pruning"},
        {"create-dummy-acoustic-models", ARG_BOOL, NULL, ARG_FLAGS_NONE, "Enables the creation of dummy acoustic models"},

        {"categories", ARG_FILE, NULL, ARG_FLAGS_NONE, "List of the categories with the associated grammars"},
        {NULL, ARG_END_MODULE, NULL, ARG_FLAGS_NONE, NULL}
    }
};


///Categories
typedef struct categories {
 grammar_t **categories;  ///< Vector of aef with categories
 int num_categories;      ///< Number of categories
} categories_t;

typedef struct decoder_t {
  extended_vocab_t *vocab;         ///< vocabulary
  hmm_t *hmm;                      ///< Acoustic model
  lex_t *lex;                      ///< Lexical model
  grammar_t *grammar;              ///< Grammar model
  categories_t *categories;          ///< Structure of categories
  grammar_t *input_grammar;        /**< Input language model. To make the search simpler,
                                      * the input grammar must be deterministic at word level.
                                      * Thus, phrase models and fsm are forbidden. Assuming n-gram
                                      */
  grammar_t *output_grammar;       /**< Output language model. This model can be used with
                                      * conditional probability transducers to perform
                                      * integrated phrase-based speech translation.
                                      * To make the search simpler,
                                      * the input grammar must be deterministic at word level.
                                      * Thus, phrase models and fsm are forbidden. Assuming n-gram
                                      */

  float gsf;             ///< Grammar Scale Factor
  float gsf_in;          ///< Gsf for the input grammar
  float gsf_out;         ///< Gsf for the output grammar
  float wip;             ///< Word Insertion Penalty
  float wip_out;         ///< Word Insertion Penalty for output words
  int histogram_pruning; ///< Maximum number of hypotheses per frame
  float beam_pruning;    ///< Relative pruning w.r.t. the maximum hypothesis
  bool do_acoustic_early_pruning; /**< If the acoustic early pruning is enabled or not.
                                     *  In this mode, before language model expansions,
                                     *  an estimation of the best acoustic score is
                                     *  obtained as the maximum acoustic score computed
                                     *  so far, for the current feature vector. This estimate
                                     *  is used to prune the hypothesis before expanding
                                     *  the word */

} decoder_t;

decoder_t * decoder_create_from_file(FILE *file);
decoder_t * decoder_create_from_args(const args_t *args);
decoder_t *decoder_dup(const decoder_t *source);
void decoder_delete(decoder_t *decoder);

#ifdef __cplusplus
}
#endif

#endif /* DECODER_H_ */
