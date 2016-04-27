%{
#include <prhlt/constants.h>
#include <prhlt/trace.h>
#include <viterbi/parsers/hmm-parser/hmm-flex.h>

#define YYERROR_VERBOSE 1

//Global
static int i, j, k, l, m, num_state=-1, mixture=0, number_mixtures=0, num=0, num_states=0, num2=0, mixture2=-1;
%}

%name-prefix="hmm_"
/* %expect 13 */
%defines
%parse-param { hmm_t * hmm }


// Symbols.
%union
{
  int          ival;
  double       dval;
  char	      *sval;
  int *list_numberint;
  float *list_numberfloat;
  float *list_prob;
  state_t    *state;
  state_t    **states;
  gaussian_t *gaussian;
  distribution_t *distribution;
  distribution_t **mixture_components;
  phoneme_t *phoneme;
  mean_t *mean;
  variance_t *variance;
  mixture_t *mixture;
  matrix_transitions_t *matrix;
}

//Tokens
%token        END 0 "end of file"
%token        ENDL  "end of line"
%token <sval> IDENTIFIER STRING
%token <ival> INT
%token <dval> FLOAT
%token        OPTIONS HMM STREAMINFO VECSIZE NULLD MFCC_D_A MFCC DIAGC BEGINHMM NUMSTATES STATE NUMMIXES MIXTURE MEAN VARIANCE GCONST TRANSP ENDHMM STATE_LINK UNEXPECTED MATRIX_LINK MIXTURE_LINK TMIX VARIANCE_LINK MEAN_LINK HMMSETID REGRESSION REGTREE NODE TNODE RCLASS

//Type of the no-terminals
%type  <dval> numberfloat gconst
%type  <ival> numberint
%type  <mean> mean shared_mean
%type  <variance> variance shared_variance
%type  <list_numberint> list_numberint
%type  <list_numberfloat> list_numberfloat list_matrix list_prob
%type  <state> shared_state state
%type  <gaussian> gaussian
%type	<distribution> mixture_component shared_distribution  distribution
%type  <mixture_components> mixture_components
%type  <phoneme> phoneme shared_phoneme
%type  <states> states
%type  <mixture> mixture
%type  <matrix> matrix shared_matrix
%destructor { free($$); } IDENTIFIER STRING

//Axiom of the grammar
%start file

%%

/* the base case is empty line; @$ declares the lloc variable that stores the cursor position */
file: {@$.first_line = 0;} hmm END

hmm: 
   | bloques

bloques: bloque
  | bloques bloque

bloque: OPTIONS opciones
 | shared_phoneme
 | shared_state
 | shared_distribution
 | shared_mean
 | shared_variance
 | shared_matrix
 | regression_class
 
opciones: opcion
  | opciones opcion
  | /* empty */

opcion: STREAMINFO list_numberint {
	//Save the number of features
	hmm->num_features = $2[0];
	free($2);
}
  | VECSIZE numberint { hmm->num_features = $2; }
  | NULLD
  | MFCC_D_A
  | MFCC
  | DIAGC
  | HMMSETID IDENTIFIER { free($2); }
 
 
regression_class: REGRESSION STRING REGTREE numberint {
 free($2);
} options

options: option
 | options option
 
option: nodes 
 | tnodes

nodes: node
 | nodes node
 | /* empty */
 
node: NODE numberint numberint numberint

tnodes: tnode
 | tnodes tnode
 
tnode: TNODE numberint numberint
 
shared_phoneme: HMM STRING phoneme {
	//List of all phonemes
	hmm->phonemes=(phoneme_t **)realloc(hmm->phonemes, sizeof(phoneme_t *)*(hmm->num_phonemes+1));
	hmm->phonemes[hmm->num_phonemes]=$3;
	hmm->phonemes[hmm->num_phonemes]->label=(char *)calloc(strlen($2)+1, sizeof(char));
	strcpy(hmm->phonemes[hmm->num_phonemes]->label, $2);
	hmm->num_phonemes++;
	free($2);
}

//A phoneme
phoneme: BEGINHMM opciones NUMSTATES numberint  {
		$<phoneme>$=(phoneme_t *)calloc(1, sizeof(phoneme_t));
		num_states=$4;
		$<phoneme>$->num_states=$4;
	}
	states matrix ENDHMM {
		$$=$<phoneme>5;
		$$->states = $6;
		$$->matrix=$7;

		hmm->locations = (hmm_location_t *) realloc(hmm->locations, (hmm->n_hmm_states + num_states) * sizeof(hmm_location_t));
		MEMTEST(hmm->locations);
		$$->hmm_ids = (hmm_id_t *) malloc(num_states * sizeof(hmm_id_t));
		MEMTEST($$->hmm_ids);
		// the first and last do not emit feature vectors
		for (int s = 0; s < $$->num_states - 2; s++) {
			hmm->locations[hmm->n_hmm_states].phoneme = hmm->num_phonemes;
			hmm->locations[hmm->n_hmm_states].state = s;
			hmm->locations[hmm->n_hmm_states].state_id = $$->states[s]->id;
			$$->hmm_ids[s] = hmm->n_hmm_states++;
		}
	}

//Vector of states
states: state {
	$$=(state_t **)calloc(num_states, sizeof(state_t *));
	$$[0]=$1;
	}
  | states state {
	$$[num_state]=$2;
	}

//List of all matrix
shared_matrix: MATRIX_LINK STRING matrix {
	$3->label=(char *)calloc(strlen($2)+1, sizeof(char));
	strcpy($3->label, $2);
	free($2);
}

//A matrix
matrix: TRANSP numberint {
	//Save the matrix of transitions
	$<matrix>$=(matrix_transitions_t *)calloc(1, sizeof(matrix_transitions_t));
        $<matrix>$->label = NULL;
	$<matrix>$->num_transitions = $2;
	num = $2;
	}
  //List with probabilities of transitions
  list_matrix {
	$$ = $<matrix>3;
	$$->matrix_transitions=(float **)calloc($2,sizeof(float *));
	l = 0;
	for(j=0; j<$2; j++){
		$$->matrix_transitions[j]=(float *)calloc($2,sizeof(float));
	        for(k=0; k<$2; k++) {
			$$->matrix_transitions[j][k]=$4[l];
			l++;
		}
	}

	//Save the matrix with all matrix
	hmm->matrix=(matrix_transitions_t **)realloc(hmm->matrix, sizeof(matrix_transitions_t *)*(hmm->num_matrix+1));
	hmm->matrix[hmm->num_matrix]=$$;
	hmm->num_matrix++;

	free($4);
	}
  //Link matrix
  | MATRIX_LINK STRING {
	for(i=0; i < hmm->num_matrix; i++){
		//Compare my matrix with link matrix
		if (hmm->matrix[i]->label != NULL && strcmp(hmm->matrix[i]->label, $2) == 0){
			$$=hmm->matrix[i];
			break;
		}
	}
	free($2);
	}

//Link state
shared_state: STATE_LINK STRING state {
	$3->label=(char *)calloc(strlen($2)+1, sizeof(char));
	strcpy($3->label, $2);
	free($2);
	}

//A state
state:
    //A state with a mixture
    STATE numberint mixture {
	num_state=$2-2;

	$$ = (state_t *)calloc(1, sizeof(state_t));
	$$->mixture=$3;
	$$->label=NULL;

	hmm->states=(state_t **)realloc(hmm->states, sizeof(state_t *)*(hmm->num_states+1));
	hmm->states[hmm->num_states]=$$;
        hmm->states[hmm->num_states]->id=hmm->num_states;
	hmm->num_states++;
        }

    //A mixture for a link state
    | mixture {
	$$ = (state_t *)calloc(1, sizeof(state_t));
	$$->mixture=$1;
	$$->label=NULL;

	hmm->states=(state_t **)realloc(hmm->states, sizeof(state_t *)*(hmm->num_states+1));
	hmm->states[hmm->num_states]=$$;
        hmm->states[hmm->num_states]->id=hmm->num_states;
	hmm->num_states++;
	}
    //Link state
    | STATE_LINK STRING {
	for(i=0; i < hmm->num_states; i++){
		//Compare my state with link states
		if (hmm->states[i]->label != NULL && strcmp(hmm->states[i]->label, $2) == 0){
			$$=hmm->states[i];
			break;
		}
	}
	free($2);
	num_states++;
	}

    //Link state
    | STATE numberint STATE_LINK STRING {
	num_state=$2-2;
	for(i=0; i < hmm->num_states; i++){
		//Compare my state with link states
		if (hmm->states[i]->label != NULL && strcmp(hmm->states[i]->label, $4) == 0){
			$$=hmm->states[i];
			break;
		}
	}
	num_states++;
	free($4);
	}


//A mixture
mixture:
   //A mixture with 1 component, only a distribution
   distribution {
	$$= (mixture_t *)calloc(1, sizeof(mixture_t));
	$$->num_distributions=1;
	$$->distributions=(distribution_t **)calloc(1, sizeof(distribution_t *));
	$$->distributions[0]=$1;
	$$->prior_probs = NULL;
	}
  //A mixture with several components
  | NUMMIXES numberint {
	$<mixture>$= (mixture_t *)calloc(1, sizeof(mixture_t));
        mixture=$2;
        number_mixtures=$2;
	}
     mixture_components {
	$$ = $<mixture>3;
	$$->distributions = $4;
	$$->prior_probs = NULL;

	$$->num_distributions=mixture2+1;
        number_mixtures=mixture2+1;
        if(mixture!=number_mixtures){
	  fprintf(stderr,"WARNING: The phoneme %d in the state %d has %d gaussians instead of %d gaussians\n", hmm->num_phonemes, num_state+3, number_mixtures, mixture);
	}
	}

  //A mixture with link components
  | NUMMIXES numberint {
	num2=$2;
	$<mixture>$= (mixture_t *)calloc(1, sizeof(mixture_t));
	$<mixture>$->distributions=(distribution_t **)calloc($2, sizeof(distribution_t *));
	$<mixture>$->num_distributions=$2;
	}

	TMIX STRING list_prob {
	$$=$<mixture>3;
	for (j=0; j < $2; j++) {
		for(i=0; i < hmm->num_distributions; i++){
			char *cadena=(char *)calloc(strlen($5)+2, sizeof(char));
			strcpy(cadena, $5);
			sprintf(cadena,"%s%d", cadena, j+1);
			//Compare my component with link distributions
			if (hmm->distributions[i]->label != NULL && strcmp(hmm->distributions[i]->label, cadena) == 0){
				$$->distributions[i]=hmm->distributions[i];
				break;
			}
		}
	}
	$$->prior_probs = (float *)calloc($2, sizeof(float));
	for (i=0; i < $2; i++) {
		$$->prior_probs[i] = $6[i];
	}
	free($5);
	}

//Vector with all componets of a mixture
mixture_components: mixture_component {
	mixture2=0;
	$$=(distribution_t **)calloc(number_mixtures, sizeof(distribution_t *));
	$$[0]=$1;
	}
  | mixture_components mixture_component {
	mixture2++;
	$$[mixture2]=$2;
	}
	
class: 
     | RCLASS numberint

//A component of a mixture
mixture_component: MIXTURE numberint numberfloat class distribution {

	$$ = $5;

	//Probability of the distribution
	$$->prior=log($3);

	}
    | MIXTURE_LINK STRING {
	for(i=0; i < hmm->num_distributions; i++){
		//Compare my component with link distributions
		if (hmm->distributions[i]->label != NULL && strcmp(hmm->distributions[i]->label, $2) == 0){
			$$=hmm->distributions[i];
			break;
		}
	}
	free($2);
	}

//Link distribution
shared_distribution: MIXTURE_LINK STRING distribution {
	$3->label=(char *)calloc(strlen($2)+1, sizeof(char));
	//Save the name of distribution
	strcpy($3->label, $2);
}

//Distribution can be a gaussian
distribution: gaussian {
	$$ = (distribution_t *) calloc(1, sizeof(distribution_t));
	$$->gaussian = $1;
	$$->type = GAUSSIAN;

	//Save distribution in list all distributions
	hmm->distributions=(distribution_t **)realloc(hmm->distributions, sizeof(distribution_t *)*(hmm->num_distributions+1));
	hmm->distributions[hmm->num_distributions]=$$;
	hmm->num_distributions++;
	}

//A gaussian
gaussian: mean variance gconst {
        $$ = (gaussian_t *)calloc(1,sizeof(gaussian_t));
	//Mean
        $$->mean=$1;
	//Variance
        $$->variance=$2;
        //Gconst
        $$->constant=$3;
        }

//Link mean
shared_mean: MEAN_LINK STRING mean {
	$3->label=(char *)calloc(strlen($2)+1, sizeof(char));
	//Save the name of mean
	strcpy(hmm->means[hmm->num_means-1]->label, $2);
        free($2);
}

//Mean
mean: MEAN numberint list_numberfloat {
	//Mean
	$$=(mean_t *)calloc(1, sizeof(mean_t));
        $$->mean=$3;
	$$->label=NULL;

	hmm->means=(mean_t **)realloc(hmm->means, sizeof(mean_t *)*(hmm->num_means+1));
	hmm->means[hmm->num_means]=$$;
	hmm->num_means++;
	}
    | MEAN_LINK STRING {
	//Searh mean in list all means
	for(i=0; i < hmm->num_means; i++){
		//Compare my mean with link mean
		if (hmm->means[i]->label != NULL && strcmp(hmm->means[i]->label, $2) == 0){
			$$=hmm->means[i];
			break;
		}
	}
	free($2);
	}

//Link variance
shared_variance: VARIANCE_LINK STRING variance {
	$3->label=(char *)calloc(strlen($2)+1, sizeof(char));
	//Save the name of variance
 	strcpy($3->label, $2);
        free($2);
}

//Variance
variance: VARIANCE numberint list_numberfloat {
	//Variance
	$$=(variance_t *)calloc(1, sizeof(variance_t));
	$$->variance=$3;
	$$->label=NULL;

	//List of all variances
	hmm->variances=(variance_t **)realloc(hmm->variances, sizeof(variance_t *)*(hmm->num_variances+1));
	hmm->variances[hmm->num_variances]=$$;
	hmm->num_variances++;
	}
    | VARIANCE_LINK STRING {
	//Searh variance in list all variances
	for(i=0; i < hmm->num_variances; i++){
		//Compare my variance with link variance
		if (hmm->variances[i]->label != NULL && strcmp(hmm->variances[i]->label, $2) == 0){
			$$=hmm->variances[i];
			break;
		}
	}
	free($2);
	}

//Gconst is optional
gconst: /* empty */ {
	$$ = LOG_ZERO;
	//Original acoustic models have not gconst
	//In this point it must be calculated gconst
        $$=calculate_gconst(hmm->variances[hmm->num_variances-1], hmm->num_features);
	}
	| GCONST numberfloat { $$ = $2; }


//List of numbers
list_numberint: numberint {
                       $$ = (int *)calloc(1,sizeof(int));
                     }
  | list_numberint numberint { $$[0] = $2; }

list_numberfloat: numberfloat {
	i = 0;
        $$ = (float *)calloc(hmm->num_features,sizeof(float));
	$$[i]=$1;
	}

  | list_numberfloat numberfloat {
	i=i+1;
	$$[i] = $2;
	}

list_matrix : numberfloat {
	m = 0;
        $$ = (float *)calloc(num*num,sizeof(float));
        if($1==0.0){
         $$[m]=LOG_ZERO;
        }
        else{
	 $$[m]=log($1);
        }
	}

  | list_matrix numberfloat {
	m=m+1;
        if($2==0.0){
         $$[m]=LOG_ZERO;
        }
        else{
	 $$[m]=log($2);
        }
	}

list_prob: numberfloat {
	i=0;
        $$ = (float *)calloc(num2,sizeof(float));
	$$[i] = log($1);
	}

  | list_prob numberfloat {
	i=i+1;
	$$[i] = log($2);
	}

numberfloat: FLOAT { $$ = $1; }

numberint: INT { $$ = $1; }

%%

