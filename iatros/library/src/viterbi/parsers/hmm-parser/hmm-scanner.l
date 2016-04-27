%{
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define YY_NO_INPUT
#include <viterbi/parsers/hmm-parser/hmm-flex.h>
%}


/* Options */
%option noyywrap nounput case-insensitive
%option prefix="hmm_"
/* %option debug */

%x INCOMMENT

WSCHAR              [ \t\f\v\r]
OCTDIGIT            [0-7]
HEXDIGIT            [0-9a-fA-F]
DIGIT               [0-9]

                /* Strings */
ESCAPE              (\\[ntvbrfa\n\\?'"])
STRING              \"([^\\\n"]|{ESCAPE})*\"
CHARLIT             \'([^\\\n']|{ESCAPE})*\'

                /* Intergers */
OCTAL               (0{OCTDIGIT}+)
HEXADECIMAL         (0[xX]{HEXDIGIT}+)
INTEGER             [+-]?([1-9]{DIGIT}*)|0

                /* Floats */
EXPONENT            [Ee][+-]?{DIGIT}+
FRACTIONAL          ({DIGIT}+\.)|({DIGIT}*\.{DIGIT}+)
FLOAT               [+-]?{FRACTIONAL}{EXPONENT}?|{DIGIT}+{EXPONENT}

                /* Others */
WHITESPACE          {WSCHAR}+|({WSCHAR}*\\)+{WSCHAR}*
NEWLINE             [\n]

IDENTCHARS	    [_/a-zA-Z]
IDENT               {IDENTCHARS}({IDENTCHARS}|[0-9])*


%%


%{
  BEGIN(INITIAL);
  errno=0;
%}

"%".*                   { hmm_begin_token(yytext); /*return(COMMENT);*/ }

{NEWLINE}               {hmm_begin_token(yytext);                          /*return ENDL;*/}

{WHITESPACE}            { hmm_begin_token(yytext); /*return(SPACE);*/ }

"~o"		{ return OPTIONS; }
"<STREAMINFO>"  { return STREAMINFO; }

"<VECSIZE>"     { return VECSIZE; }
"<NULLD>"	{ return NULLD; }
"<MFCC_D_A>"	{ return MFCC_D_A; }
"<MFCC>"	{ return MFCC; }
"<DIAGC>"       { return DIAGC; }
"<HMMSETID>"    { return HMMSETID; }

"~r"		{ return REGRESSION; }
"<REGTREE>"	{ return REGTREE; }
"<NODE>"	{ return NODE; }
"<TNODE>"	{ return TNODE; }
"<RCLASS>"	{ return RCLASS; }

"~h"		{ return HMM; }
"<BEGINHMM>"    { return BEGINHMM; }
"<NUMSTATES>"   { return NUMSTATES; }
"<STATE>"       { return STATE; }
"<NUMMIXES>"	{ return NUMMIXES; }
"<MIXTURE>"	{ return MIXTURE; }
"<MEAN>"	{ return MEAN; }
"<VARIANCE>"	{ return VARIANCE; }
"<GCONST>"	{ return GCONST; } 
"<TRANSP>"	{ return TRANSP; }
"<ENDHMM>"	{ return ENDHMM; }
"~s"		{ return STATE_LINK; }
"~t"		{ return MATRIX_LINK; }
"~m"		{ return MIXTURE_LINK;}
"~v"		{ return VARIANCE_LINK;}
"~u"		{ return MEAN_LINK;}
"<TMIX>"	{ return TMIX;}

{OCTAL}		{
                  hmm_begin_token(yytext);
                  long n = strtol(yytext+1,NULL,8);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          hmm_lval.ival = n;
                  return INT;
		}
{HEXADECIMAL}   {
                  hmm_begin_token(yytext);
                  long n = strtol(yytext+2,NULL,16);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          hmm_lval.ival = n;
	          return INT;
		}

{INTEGER}       {
                  hmm_begin_token(yytext);
                  long n = strtol(yytext,NULL,10);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          hmm_lval.ival = n;
                  return INT;
		}


{FLOAT}         {
                  hmm_begin_token(yytext);
                  float n = strtod(yytext,NULL);
                  if (! (errno != ERANGE)) {
                    fprintf(stderr, "float is out of range");
                  }
	          hmm_lval.dval = n;
                  return FLOAT;
		}


{STRING} |
{CHARLIT}       { // Copy and remove "" and ''
                  hmm_begin_token(yytext);
                  hmm_lval.sval = hmm_copy_string(yytext, true);
                  return STRING;
                }

{IDENT}         {
                  hmm_begin_token(yytext);
                  hmm_lval.sval = hmm_copy_string(yytext, false);
                  return IDENTIFIER;
                }


.               {
                  hmm_begin_token(yytext);
                  fprintf(stderr, "unexpected character\n");
                  return UNEXPECTED;
                }

%%

