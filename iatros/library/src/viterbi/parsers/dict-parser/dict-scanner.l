%{
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define YY_NO_INPUT
#include <viterbi/parsers/dict-parser/dict-flex.h>
%}


/* Options */
%option noyywrap nounput case-insensitive
%option prefix="dict_"
/* %option debug */

%x INCOMMENT PROBABILITY EMISSION

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
WHITESPACE          {WSCHAR}+ 
                /* we remove this since we want to match the \ symbol|({WSCHAR}*\\)+{WSCHAR}**/
NEWLINE             [\n]

IDENTCHARS          [^ \t\f\v\r\n]|\\
IDENT               {IDENTCHARS}+
%%


<*>{
  {NEWLINE}    {
                 dict_begin_token(yytext);
                 BEGIN(INITIAL);
                 return ENDL;
               }
  {WHITESPACE} { dict_begin_token(yytext);  /*return(SPACE);*/ }
}


<INITIAL>{
  ^"SENT-START"   { BEGIN(PROBABILITY); return SENTSTART; }
  ^"SENT-END"     { BEGIN(PROBABILITY); return SENTEND; }
  ^{IDENT}        {
                    dict_begin_token(yytext);
                    dict_lval.sval = dict_copy_string(yytext, false);
                    BEGIN(PROBABILITY);
                    return IDENTIFIER;
                  }
}

<EMISSION>"]"     { BEGIN(PROBABILITY); return BRACKET_OUT; }
<INITIAL,EMISSION>{IDENT}      {
                  dict_begin_token(yytext);
                  dict_lval.sval = dict_copy_string(yytext, false);
                  return IDENTIFIER;
                }


<PROBABILITY>{
  "["  { BEGIN(EMISSION); return BRACKET_IN; }
  {INTEGER}       {
                    dict_begin_token(yytext);
                    long n = strtol(yytext,NULL,10);
                    if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
                      fprintf(stderr, "integer is out of range");
                    }
                    dict_lval.ival = n;
                    BEGIN(INITIAL);
                    return INT;
                  }
  
  
  {FLOAT}         {
                    dict_begin_token(yytext);
                    float n = strtod(yytext,NULL);
                    if (! (errno != ERANGE)) {
                      fprintf(stderr, "float is out of range");
                    }
                    dict_lval.dval = n;
                    BEGIN(INITIAL);
                    return FLOAT;
                  }

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

