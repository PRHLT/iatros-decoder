%{
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define YY_NO_INPUT
#include <viterbi/parsers/fsm-parser/fsm-flex.h>
%}


/* Options */
%option noyywrap nounput case-insensitive
%option prefix="fsm_"
/* %option debug */

%x LINE_STRING

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
IDENT               {IDENTCHARS}({IDENTCHARS}|[0-9]|\.)*


%%


%{
  errno = 0;
%}

{NEWLINE}      { fsm_begin_token(yytext); return ENDL; }

{WHITESPACE}   { fsm_begin_token(yytext); }

"%".*          { fsm_begin_token(yytext); }

<LINE_STRING>{  
  {NEWLINE} { fsm_begin_token(yytext); BEGIN(INITIAL); return ENDL; }
  .*        { 
              fsm_begin_token(yytext);
              fsm_lval.sval = fsm_copy_string(yytext, false);
              return STRING;
            }
}

"Name"      { 
              fsm_begin_token(yytext); 
              BEGIN(LINE_STRING); 
              return NAME; 
            }
"NumStates"	{ fsm_begin_token(yytext); return NUMSTATES; }
"NumEdges"	{ fsm_begin_token(yytext); return NUMEDGES; }
"TotalChecksum"	{ fsm_begin_token(yytext); return TOTALCHECKSUM; }
"State"		{ fsm_begin_token(yytext); return STATE; }
"c"		{ fsm_begin_token(yytext); return CHECKSUM; }
"i"		{ fsm_begin_token(yytext); return I; }
"f"		{ fsm_begin_token(yytext); return F; }
"="		{ fsm_begin_token(yytext); return EQUAL; }
"o"		{ fsm_begin_token(yytext); return OUT; }
"p"		{ fsm_begin_token(yytext); return P; }

{OCTAL}		{
                  fsm_begin_token(yytext);
                  long n = strtol(yytext+1,NULL,8);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          fsm_lval.ival = n;
                  return INT;
		}
{HEXADECIMAL}   {
                  fsm_begin_token(yytext);
                  long n = strtol(yytext+2,NULL,16);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          fsm_lval.ival = n;
	          return INT;
		}

{INTEGER}       {
                  fsm_begin_token(yytext);
                  long n = strtol(yytext,NULL,10);
                  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                    fprintf(stderr, "integer is out of range");
                  }
	          fsm_lval.ival = n;
                  return INT;
		}


{FLOAT}         {
                  fsm_begin_token(yytext);
                  float n = strtod(yytext,NULL);
                  if (! (errno != ERANGE)) {
                    fprintf(stderr, "float is out of range");
                  }
	          fsm_lval.dval = n;
                  return FLOAT;
		}


{STRING} |
{CHARLIT}       { // Copy and remove "" and ''
                  fsm_begin_token(yytext);
                  fsm_lval.sval = fsm_copy_string(yytext, true);
                  return STRING;
                }

{IDENT}         {
                  fsm_begin_token(yytext);
                  fsm_lval.sval = fsm_copy_string(yytext, false);
                  return IDENTIFIER;
                }


.               {
                  fsm_begin_token(yytext);
                  fprintf(stderr, "unexpected character\n");
                }

<<EOF>>         {
                  fsm_begin_token(yytext); 
                  // This frees all the memory, solves valgrind 16k memory leak
                  // However, it assumes that the parser is not reentrant
                  // That probably would cause a memory exception
                  // http://sourceforge.net/mailarchive/message.php?msg_id=481592269.1175582017.160223624.5281%40mcgi23.rambler.ru
                  //yy_delete_buffer( YY_CURRENT_BUFFER );
                  //yyfree( yy_buffer_stack );
                  yyterminate();
                }
%%

