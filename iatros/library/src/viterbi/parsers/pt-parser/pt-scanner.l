%{
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <prhlt/constants.h>

#define YY_NO_INPUT
#include <viterbi/parsers/pt-parser/pt-flex.h>
%}


/* Options */
%option noyywrap nounput case-insensitive
%option prefix="pt_"
/* %option debug */

WSCHAR              [ \t\f\v\r]
OCTDIGIT            [0-7]
HEXDIGIT            [0-9a-fA-F]
DIGIT               [0-9]

                /* Strings */
ESCAPE              (\\[ntvbrfa\n\\?'"])
STRIPT              \"([^\\\n"]|{ESCAPE})*\"
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

IDENTCHARS	        [_/a-zA-Z]
IDENT               {IDENTCHARS}({IDENTCHARS}|[0-9])*

NOT_WHITESPACE      [^ \t\f\v\r\n]+

%%


%{
  /*BEGIN(INITIAL);*/
  errno=0;
%}

{WHITESPACE}            { pt_begin_token(yytext); /*return(SPACE);*/ }

"|||"		                { pt_begin_token(yytext); return SEPARATOR; }


{NEWLINE}               {
                           pt_begin_token(yytext);
                           return ENDL;
                        }

{NOT_WHITESPACE}        {
                           pt_begin_token(yytext);
                           pt_lval.sval = pt_copy_string(yytext, false);
                           return TOKEN;
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

