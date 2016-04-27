%{
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define YY_NO_INPUT
#include <viterbi/parsers/lex-parser/lex-flex.h>
%}


/* Options */
%option noyywrap nounput case-insensitive
%option prefix="lex_"
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

"%".*                   { begin_token(yytext); /*return(COMMENT);*/ }

{NEWLINE}               {
                          begin_token(yytext);
                          /*return ENDL;*/
                        }

{WHITESPACE}            { begin_token(yytext); /*return(SPACE);*/ }

"Name"	        { return NAME; }
"State"		{ return STATE; }
"i"		{ return INITIAL_F; }
"p"		{ return PROB_F; }
"f"		{ return END_F; }
"="		{ return EQUAL; }

{OCTAL}		{
                  begin_token(yytext);
                  long n = strtol(yytext+1,NULL,8);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          lex_lval.ival = n;
                  return INT;
		}
{HEXADECIMAL}   {
                  begin_token(yytext);
                  long n = strtol(yytext+2,NULL,16);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          lex_lval.ival = n;
	          return INT;
		}

{INTEGER}       {
                  begin_token(yytext);
                  long n = strtol(yytext,NULL,10);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          lex_lval.ival = n;
                  return INT;
		}


{FLOAT}         {
                  begin_token(yytext);
                  float n = strtod(yytext,NULL);
                  if (! (errno != ERANGE)) {
                    fprintf(stderr, "float is out of range");
                  }
	          lex_lval.dval = n;
                  return FLOAT;
		}


{STRING} |
{CHARLIT}       { // Copy and remove "" and ''
                  begin_token(yytext);
                  lex_lval.sval = copy_string(yytext, true);
                  return STRING;
                }

{IDENT}         {
                  begin_token(yytext);
                  lex_lval.sval = copy_string(yytext, false);
                  return IDENTIFIER;
                }


.               {
                  begin_token(yytext);
                  fprintf(stderr, "unexpected character");
                  return UNEXPECTED;
                }

%%

