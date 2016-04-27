%{
#include <prhlt/constants.h>
#include <viterbi/parsers/fsm-parser/fsm-flex.h>
#include <prhlt/vocab.h>
#include <viterbi/grammar.h>
#include <prhlt/trace.h>

#define YYERROR_VERBOSE 1

struct options {
 float prob;
};

static state_grammar_t * state = STATE_NONE;
static char word_str[MAX_LINE];

%}

%name-prefix="fsm_"
%expect 10
%defines
%parse-param { grammar_t *fsm }

// Symbols.
%union
{
  int          ival;
  double       dval;
  char	      *sval;
  char         *string_history;
  symbol_t     *word_list;
  symbol_t      word;

}
%{
 struct options opts;
%}

%token        END 0 "end of file"
%token        ENDL  "end of line"
%token <sval> IDENTIFIER STRING
%token <ival> INT
%token <dval> FLOAT
%token <string_history> STRING_HISTORY
%token	      BEGINHMM ENDHMM NAME NUMSTATES NUMEDGES TOTALCHECKSUM STATE CHECKSUM I F EQUAL OUT P
%type  <dval> number

%destructor { free($$); } IDENTIFIER STRING

%start file

%%

/* the base case is empty line; @$ declares the lloc variable that stores the cursor position */
file: {@$.first_line = 0;} lines END {
        grammar_complete_fsm(fsm);
      }

lines: line 
     | lines line

line: config ENDL
  | state ENDL
  | arc ENDL
  | ENDL

config: NAME STRING { free($2); state = STATE_NONE;}
  | NUMSTATES INT { fsm->n=1; }
  | NUMEDGES INT
  | TOTALCHECKSUM number

state: STATE INT {
         state = (state_grammar_t *)(size_t)$2;
         REQUIRE($2 == fsm->num_states,
                 "ERROR: the state %d is not in the language model or the states are not in correct order.\n", fsm->num_states);

         state_grammar_t *new_state = state_grammar_create();
         grammar_append(fsm, new_state);
         
       } options


arc: INT INT word options {
       symbol_t extended_word = extended_vocab_insert_symbol(fsm->vocab, word_str, -1);

			 //XXX: we must check if words are in lexicon
       //if ($$==VOCAB_NONE) {
       // printf("ERROR: %s is not in input lexicon\n", $1);
       //exit(1);
       // }

       REQUIRE($1 < fsm->num_states, "Source state must be declared before the arc.\n");
       // we divide the arc probability by the number of words in the symbol so that 
       // probability is distributed uniformly among them.
       #if defined(DISTRIBUTE_UNIFORMLY_PHRASE_PROBABILITY)         
         float input_length = symlen(extended_vocab_get_extended_symbol(fsm->vocab, extended_word)->input);
         opts.prob /= input_length;
       #endif
       state_grammar_append(fsm->vector[$1], extended_word, opts.prob, (state_grammar_t *)(size_t)$2);
    }

options: option
  | options option
  | /* empty */

option: CHECKSUM EQUAL number
      | I EQUAL number {
          if ($3 != 0.0) list_states_append(fsm->list_initial, state, log($3));
        }
      | F EQUAL number {
          if ($3 != 0.0) list_states_append(fsm->list_end, state, log($3));
        }
      | OUT EQUAL STRING {
          REQUIRE(fsm->vocab->language_delimiter != NULL, "This is a bilingual language model but the language delimiter has not been set.");
  		    strcat(word_str, fsm->vocab->language_delimiter);
  		    strcat(word_str, $3);
          free($3);
        }
      | P EQUAL number {
          if ($3 == 0) {
            opts.prob = LOG_ZERO;
          }
          else{
            opts.prob = log($3);
          }
        }

word: STRING {
        strcpy(word_str, $1);
        free($1);
      }

number:	FLOAT { $$ = $1; }
      | INT   { $$ = (double) $1; }

%%

