%{
#include "args.h"
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define YY_NO_INPUT
#include "args-flex.h"
%}


/* Options */
%option noyywrap nounput case-insensitive
%option prefix="args_"
/* %option debug */

%x SECTION ARGUMENT

WSCHAR              [ \t\f\v\r]
OCTDIGIT            [0-7]
HEXDIGIT            [0-9a-fA-F]
DIGIT               [0-9]

                /* Strings */
ESCAPE              (\\[ntvbrfa\n\\?'"])
STRING              \"([^\\\n"]|{ESCAPE})*\"
CHARLIT             \'([^\\\n']|{ESCAPE})*\'

VSTRING             (\\\n|[^\\\n]|{ESCAPE})*
NSTRING             (\\\n|[^\[\]\\\n=]|{ESCAPE})*

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

{NEWLINE}      { args_begin_token(yytext); return ENDL; }

{WHITESPACE}   { args_begin_token(yytext); }

[%#].*         { args_begin_token(yytext); }

<SECTION>{
  "]" { 
        args_begin_token(yytext); 
        BEGIN(INITIAL); 
        return END_SECTION_NAME; 
      }
  [^\]]*  { 
        args_begin_token(yytext);
        args_lval.sval = args_copy_string(yytext, false);
        return SECTION_NAME;
      }
}
            
<ARGUMENT>{
  {VSTRING}  { 
        args_begin_token(yytext);
        args_lval.sval = args_copy_string(yytext, false);
        return VALUE;
      }
  {NEWLINE} { 
        args_begin_token(yytext); 
        BEGIN(INITIAL); 
        return ENDL;
      }
}

<INITIAL>{
  "[" { 
        args_begin_token(yytext); 
        BEGIN(SECTION);
        return BEGIN_SECTION_NAME; 
      }
            
  {NSTRING} { 
        args_begin_token(yytext); 
        args_lval.sval = args_copy_string(yytext, false);
        return NAME;
      }

  "=" { 
        args_begin_token(yytext); 
        BEGIN(ARGUMENT); 
        return EQUAL;
      }
}      
            

<<EOF>>         {
                  args_begin_token(yytext); 
                  // This frees all the memory, solves valgrind 16k memory leak
                  // However, it assumes that the parser is not reentrant
                  // That probably would cause a memory exception
                  // http://sourceforge.net/mailarchive/message.php?msg_id=481592269.1175582017.160223624.5281%40mcgi23.rambler.ru
                  //yy_delete_buffer( YY_CURRENT_BUFFER );
                  //yyfree( yy_buffer_stack );
                  yyterminate();
                }
%%

