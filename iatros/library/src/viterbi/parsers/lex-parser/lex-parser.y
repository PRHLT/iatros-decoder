%{
#include <prhlt/constants.h>
#include <prhlt/trace.h>
#include <stdlib.h>
#include <viterbi/parsers/lex-parser/lex-flex.h>
#define YYERROR_VERBOSE 1

//Global
//Hash table to name the states of the model
static vocab_t *state_vocab;
static model_t *model;
%}

%name-prefix="lex_"
%defines
%expect 1
%parse-param { lex_t * lex }


// Symbols.
%union
{
  int          ival;
  double       dval;
  char	      *sval;
}

//Tokens
%token        END 0 "end of file"
%token        ENDL  "end of line"
%token <sval> IDENTIFIER STRING
%token <ival> INT
%token <dval> FLOAT
%token        NAME STATE UNEXPECTED INITIAL_F PROB_F END_F EQUAL

//Type of the no-terminals
%type  <dval> number

%destructor { free($$); } IDENTIFIER STRING

//Axiom of the grammar
%start file

%%

/* the base case is empty line; @$ declares the lloc variable that stores the cursor position */
file: {@$.first_line = 0;} lex END {
        lex_postprocess(lex);
      }

//Lexic
lex:
   | models

models : model
       | models model

//Lines of the file can be the name of the model, a state or a edge
model: NAME STRING {
				 model = lex_model_create($2); 
				 lex_append_model(lex, model);

			 	 //Create hash table to name the states
			 	 state_vocab = vocab_create(541, NULL);

				 free($2);
       }
			 definitions{
				 vocab_delete(state_vocab);
			 }

definitions: definition
           | definitions definition

definition: state
          | edge

//A state can be a initial state or a end state
state: state_initial
     | state_end

//A initial state has i= (INITIAL_F)
state_initial: STATE INT INITIAL_F EQUAL number {
                 char source[256];
                 sprintf(source, "%d", $2);

                 int num_source = vocab_find_symbol(state_vocab, source);
                 if (num_source == VOCAB_NONE) {
                   num_source = vocab_insert_symbol(state_vocab, source, -1);
                   lex_append_state(lex, model, num_source);
                 }
                 model->initial = num_source;
               }


//A end state has f= (END_F)
state_end: STATE INT END_F EQUAL number {
             char destination[256];

             sprintf(destination, "%d", $2);
             int num_destination = vocab_find_symbol(state_vocab, destination);
             if (num_destination == VOCAB_NONE){
               num_destination = vocab_insert_symbol(state_vocab, destination, -1);
               lex_append_state(lex, model, num_destination);
             }
             model->end = num_destination;
           }


//A edge
edge: INT INT STRING PROB_F EQUAL number {
        char source[256];
        char destination[256];

        sprintf(source, "%d", $1);
        sprintf(destination, "%d", $2);

        //Create source state
        int num_source = vocab_find_symbol(state_vocab, source);
        if (num_source == VOCAB_NONE){
          num_source = vocab_insert_symbol(state_vocab, source, -1);
          lex_append_state(lex, model, num_source);
        }

        //Create destination state
        int num_destination = vocab_find_symbol(state_vocab, destination);
        if (num_destination == VOCAB_NONE) {
          num_destination = vocab_insert_symbol(state_vocab, destination, -1);
          lex_append_state(lex, model, num_destination);
        }

        float probability = ($6 == 0)? LOG_ZERO : log($6);

        lex_append_edge(lex, model, $3, num_source, num_destination, probability);

        free($3);
      }

//A end state has f= (END_F)
state_end: STATE number END_F EQUAL number {
             char destination[256];
            
             sprintf(destination, "%f", $2);
             int num_destination = vocab_find_symbol(state_vocab, destination);
             if (num_destination == VOCAB_NONE){
               num_destination = vocab_insert_symbol(state_vocab, destination, -1);
               lex_append_state(lex, model, num_destination);
             }
             lex->models[lex->num_models-1]->end = num_destination;
           }

number: FLOAT { $$ = $1; }
      | INT   { $$ = (double)$1; }

%%

