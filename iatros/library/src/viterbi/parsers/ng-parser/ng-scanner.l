%{
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <prhlt/constants.h>

#define YY_NO_INPUT
#include <viterbi/parsers/ng-parser/ng-flex.h>
//Global
int num_words=0, order=0;
%}


/* Options */
%option noyywrap nounput case-insensitive
%option prefix="ng_"
/* %option debug */

%x INCOMMENT NGRAM_CTX

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
NUMBER              {FLOAT}|{INTEGER}
                /* Others */
WHITESPACE          {WSCHAR}+|({WSCHAR}*)+{WSCHAR}*
NEWLINE             [\n]

IDENTCHARS	    [_/a-zA-Z]
IDENT               {IDENTCHARS}({IDENTCHARS}|[0-9])*

NOT_WHITESPACE      [^ \t\f\v\r\n]+

INITIAL		    ("*"|"#")

%%


%{
  /*BEGIN(INITIAL);*/
  errno=0;
%}

^{INITIAL}		{ ng_begin_token(yytext); BEGIN(INCOMMENT); }

<*>{WHITESPACE}            { ng_begin_token(yytext); /*return(SPACE);*/ }

<*>^{WHITESPACE}*"%".*      { ng_begin_token(yytext); /*return(COMMENT);*/ }

<*>^"\\data\\"		{ ng_begin_token(yytext);
				BEGIN(INITIAL);
				return DATA; 
			}
<INCOMMENT>. { ng_begin_token(yytext); }
<INCOMMENT>{NEWLINE} { ng_begin_token(yytext); }


"ngram"		{ ng_begin_token(yytext); return NGRAM; }

"="		{ ng_begin_token(yytext);return EQUAL; }


^{WHITESPACE}"\\"	{ ng_begin_token(yytext);  return ESCAPES; }	

"-grams:"{NEWLINE}		{ ng_begin_token(yytext);   return GRAMS;}


"\\end\\"		{ ng_begin_token(yytext);return ENDNG; }

{NEWLINE}               {
                          ng_begin_token(yytext);
                          /*return ENDL;*/
                        }

^{NUMBER}        {
		  num_words=0;
                  ng_begin_token(yytext);

		  if(strcmp(yytext, "-99")==0){
                    ng_lval.dval = -INFINITY;
 		  }
		  else{
		    float n = strtod(yytext,NULL);
                    if (! (errno != ERANGE)) {
                      fprintf(stderr, "float is out of range");
                    }
	            ng_lval.dval = n;
		  }

		  BEGIN(NGRAM_CTX);	
                  return FLOAT;
		 

		}


<NGRAM_CTX>{

 {NOT_WHITESPACE} {
		  num_words++;
		  ng_begin_token(yytext);
      if (num_words>order) {
		    if (strcmp(yytext, "-99") == 0) {
          ng_lval.dval = -INFINITY;
 		    }
  		  else{
          float n = strtod(yytext,NULL);
          if (!(errno != ERANGE)) {
            fprintf(stderr, "float is out of range");
          }
	        ng_lval.dval = n;
		    }
        return FLOAT;
		  }
		  else {
        ng_lval.sval = yytext;
        return STRING_HISTORY;
		  }
  } 


{NEWLINE}   {
                          ng_begin_token(yytext);
			  BEGIN(INITIAL);
			  return ENDL;
                        }
}

{INTEGER}     {
                  ng_begin_token(yytext);
                  long n = strtol(yytext,NULL,10);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          ng_lval.ival = n;
		  order=n;
                  return INT;
		}
<<EOF>>         {
                  // This frees all the memory, solves valgrind 16k memory leak
                  // However, it assumes that the parser is not reentrant
                  // That probably would cause a exception memory
                  // http://sourceforge.net/mailarchive/message.php?msg_id=481592269.1175582017.160223624.5281%40mcgi23.rambler.ru
                  //yy_delete_buffer( YY_CURRENT_BUFFER );
                  //yyfree( yy_buffer_stack );
                  yyterminate();
                }

%%

