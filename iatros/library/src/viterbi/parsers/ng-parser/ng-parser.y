%{
#include <viterbi/parsers/ng-parser/ng-flex.h>
#include <prhlt/vocab.h>
#include <viterbi/grammar.h>
#include <prhlt/trace.h>
#include <prhlt/constants.h>

#define YYERROR_VERBOSE 1
//Global
static int n = 0, words_num = 0, approx_num_states = 0, ng_num_states = 0;
static bool has_bo_weight;
static table_grammar_t *table = NULL; ///< Hash table necessary to build the structure with the grammar
static size_t n_end = 0;
%}

%name-prefix="ng_"
%defines
%parse-param { grammar_t *ng }

// Symbols.
%union
{
  int          ival;
  double       dval;
  char         *sval;
  char         *string_history;
  symbol_t     *word_list;
  symbol_t      word;
}

%token        END 0 "end of file"
%token        ENDL  "end of line"
%token <sval> IDENTIFIER STRING
%token <ival> INT 
%token <dval> FLOAT
%token <string_history> STRING_HISTORY
%token	        BEGINHMM ENDHMM DATA UNEXPECTED NGRAM COMMENT EQUAL ESCAPES GRAMS ENDNG
//Type of the no-terminals
%type  <dval> backoff
%type  <word> word
%type  <word_list> words

%destructor { free($$); } IDENTIFIER STRING

%start file

%%

/* the base case is empty line; @$ declares the lloc variable that stores the cursor position */
file: { @$.first_line = 0;} ng END {
        grammar_complete_ngram(ng, table);
        table_grammar_delete(table);
        if (n_end > 0) {
          PRINT("Warning: Skipping %d n-grams for unset end word '</s>'\n", (unsigned int)n_end);
        }
      }

ng:  
  | DATA datas {
      // substract highest ngram order events
      approx_num_states -= ng_num_states;
      table = table_grammar_create(approx_num_states);
      
      // add unigram so that unigram state_num == 0
      {
        symbol_t unigram[] = {VOCAB_NONE};
        state_grammar_t *state = state_grammar_create();
        grammar_append(ng, state);
        state_grammar_set_name(state, unigram);        
        table_grammar_insert(table, state);
      }

    }
    ngrams ENDNG

datas: data
     | datas data

data: NGRAM INT EQUAL INT {
        ng_num_states = $4;
        approx_num_states += $4;
        ng->n = $2;
      }

ngrams: ngram
      | ngrams ngram

ngram: ESCAPES INT{
           n=$2;
       }
       GRAMS lines

lines: line
     | lines line 

line: FLOAT words backoff ENDL{

//        char *str = NULL;
//        extended_vocab_symbols_to_string($2, ng->vocab, &str);        
//        PRINT("ngram: %s, prob: %g, has_backoff: %d, backoff: %g\n", str, $1, has_bo_weight, $3/M_LOG10E);
//        free(str);

        // if there is no end word, then remove '</s>' from the ngram list
        if (ng->end != VOCAB_NONE ||
            ( strcmp(extended_vocab_get_string(ng->vocab, $2[n-1]), "</s>") != 0
            && strcmp(extended_vocab_get_string(ng->vocab, $2[n-1]), "</s>|||</s>") != 0)) 
          {
          // if has bo weight create a new state
          if (has_bo_weight) {
            node_grammar_t *node = table_grammar_find(table, $2);
           
            // we could not find the state. Create a new one
            if (node == NULL) {
              state_grammar_t *state = state_grammar_create();
              grammar_append(ng, state);
              state_grammar_set_name(state, $2);
          
              node = table_grammar_insert(table, state);
            }
            
            node->state->bo = $3/M_LOG10E;
          }
          
          
          // find the state in the grammar 
          symbol_t state_name[n];
          for(int i=0; i < n - 1; i++) state_name[i] = $2[i];
          state_name[n - 1] = VOCAB_NONE;
          node_grammar_t *node = table_grammar_find(table, state_name);
           
          // we could not find the state. Create a new one
          if (node == NULL) {
            state_grammar_t *state = state_grammar_create();
            grammar_append(ng, state);
            state_grammar_set_name(state, state_name);
          
            node = table_grammar_insert(table, state);
          }
          
          // we divide the arc probability by the number of words in the symbol so that 
          // probability is distributed uniformly among them.
          float log_prob = $1/M_LOG10E;
          #if defined(DISTRIBUTE_UNIFORMLY_PHRASE_PROBABILITY)
            float input_length = symlen(extended_vocab_get_extended_symbol(ng->vocab, $2[n-1])->input);
            log_prob /= input_length;
          #endif
          state_grammar_append(node->state, $2[n-1], log_prob, 0);
        }
        else {
          n_end++;
        }
        
        free($2);
      }

backoff:  FLOAT { 
            has_bo_weight = true;
            $$=$1;
          }
       |  {
            has_bo_weight = false; 
            $$=0.0; 
          }

words: word { 
           $$ = (symbol_t *) malloc((n+1)*sizeof(symbol_t));
           words_num = 0;
           $$[words_num++] = $1; 
           $$[words_num]   = VOCAB_NONE; 
       }
       | words word { 
           $$[words_num++] = $2;
           $$[words_num]   = VOCAB_NONE; 
        }

word: STRING_HISTORY {
        switch (ng->vocab_type) {
        case GV_BILINGUAL:
          $$=extended_vocab_insert_symbol(ng->vocab, $1, -1);
          break;
        case GV_INPUT:
          $$=vocab_insert_symbol(ng->vocab->in, $1, -1);
          break;
        case GV_OUTPUT:
          $$=vocab_insert_symbol(ng->vocab->out, $1, -1);
          break;
        default:
          break;
        }
      }

%%

