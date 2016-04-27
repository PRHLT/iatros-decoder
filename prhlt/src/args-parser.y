%{
#include "args.h"
#include "args-flex.h"
#include "trace.h"
#include "utils.h"

#define YYERROR_VERBOSE 1
#define MAX_LINE 1024

%}

%name-prefix="args_"
%expect 0
%defines
%parse-param { args_t *args }

// Symbols.
%union
{
  int          ival;
  double       dval;
  char	      *sval;

}
%{
  char section[MAX_LINE] = "";
  char name[MAX_LINE] = "";
%}

%token        END 0 "end of file"
%token        ENDL  "end of line"
%token <sval> SECTION_NAME NAME VALUE
%token        EQUAL BEGIN_SECTION_NAME END_SECTION_NAME

%destructor { free($$); } SECTION_NAME NAME VALUE

%start file

%%

/* the base case is empty line; @$ declares the lloc variable that stores the cursor position */
file: {@$.first_line = 0;} lines END

lines: line 
     | lines line

line: 
    BEGIN_SECTION_NAME SECTION_NAME END_SECTION_NAME ENDL {
      strcpy(section, strip($2));
      free($2);
    } 
  | NAME EQUAL VALUE ENDL {
      strcpy(name, "");
      if (strcmp(section, "") != 0) {
        strcat(name, section);
        strcat(name, ".");
      }
      strcat(name, strip($1));
      args_update(args, name, strip($3)); 
      free($1);
      free($3);
    }
  | ENDL

%%

