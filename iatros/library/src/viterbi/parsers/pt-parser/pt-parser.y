%{
#include <viterbi/parsers/pt-parser/pt-flex.h>
#include <viterbi/parsers/pt-parser/pt.h>
#include <prhlt/vocab.h>
#include <viterbi/grammar.h>
#include <prhlt/trace.h>
#include <prhlt/constants.h>

#define YYERROR_VERBOSE 1
static char *source;
static char *target;
static int n = 0;
%}

%name-prefix="pt_"
%defines
%expect 3
%parse-param { grammar_t *pt }
// Symbols.
%union
{
  int          ival;
  double       dval;
  char         *sval;
  char         *words;
  char         *word;
  float        score;
  float        *scores;
}

%token        END 0 "end of file"
%token        ENDL  "end of line"
%token <sval> TOKEN
%token	      SEPARATOR
//Type of the no-terminals
%type  <score> score
%type  <scores> scores
%type  <word> word
%type  <words> words

%destructor { free($$); } TOKEN

%start file

%%

/* the base case is empty line; @$ declares the lloc variable that stores the cursor position */
file: {@$.first_line = 0;} pt END {
        grammar_complete_pt(pt);
      }

pt:
  | {
      // insert the only state (unigram like)
      state_grammar_t *state = state_grammar_create();
      grammar_append(pt, state);
    }
    phrases

phrases: phrase
       | phrases phrase

phrase: words {
          source = $<words>1;
          if (source == NULL) {
            source = (char *)malloc(sizeof(char));
            strcpy(source, "");
          }
        }
        SEPARATOR words {
          target = $<words>4;
          if (target == NULL) {
            target = (char *)malloc(sizeof(char));
            strcpy(target, "");
          }
        }
        SEPARATOR alignments SEPARATOR alignments
        SEPARATOR scores {
          REQUIRE(pt->vocab->language_delimiter != NULL, "This is a bilingual language model but the language delimiter has not been set.");
          strcat(source, pt->vocab->language_delimiter);
          strcat(source, target);
          symbol_t extended_word = extended_vocab_insert_symbol(pt->vocab, source, CATEGORY_NONE);

          REQUIRE(n == pt->vocab->n_weights, "Invalid number of features");
          extended_vocab_set_scores(pt->vocab, extended_word, $<scores>11);
          free($<scores>11);

          // we set the score to 0 since the combined log-linear score
          // will be already append by the viterbi step
          state_grammar_append(pt->vector[0], extended_word, .0, 0);
          free(source);
          free(target);
        }
        ENDL


words: /* lambda */ { $$ = NULL; }
     | word {
         $$ = (char *) malloc((MAX_LINE + 1) * sizeof(char));
         strcpy($$,$1);
       }
     | words word {
         $$ = $1;
         REQUIRE(pt->vocab->word_delimiter != NULL, "This is a bilingual language model but the word delimiter has not been set.");
         strncat($$, pt->vocab->word_delimiter, MAX_LINE);
         strncat($$, $2, MAX_LINE);
       }

word: TOKEN {
        $$ = $1;
      }

alignments: alignment
          | alignments alignment

alignment: TOKEN


scores: score {
          $$ = (float *) malloc(pt->vocab->n_weights * sizeof(float));
          n = 0;
          REQUIRE(n < pt->vocab->n_weights, "Invalid number of features (%d < %d)", n, pt->vocab->n_weights);
          $$[n++] = log($1);
        }
      | scores score {
          $$ = $1;
          REQUIRE(n < pt->vocab->n_weights, "Invalid number of features (%d < %d)", n, pt->vocab->n_weights);
          $$[n++] = log($2);
        }

score: TOKEN {
         $$ = strtod($1, NULL);
         CHECK_SYS_ERROR($$ != HUGE_VAL, "Invalid number");
       }

%%

