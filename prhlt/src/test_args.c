/*! \author Miriam Lujan Mares mlujan@iti.upv.es
 *  \version 1.0
 *  \date    2008
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#include <prhlt/config.h>
#include "utils.h"
#include "trace.h"
#include "gzip.h"
#include "args.h"


static const arg_module_t module = {NULL, "General options",
    {
      {"say-hello", ARG_BOOL, "false", 0, "Say hello"},
      {NULL, ARG_END_MODULE, NULL, 0, NULL}
    }
};

static const arg_shortcut_t shortcuts[] = {
    {"s", "say-hello"},
    {NULL, NULL}
};

int main(int argc, char *argv[]) {

  args_t *args = args_create();
  args_set_summary(args, "Test for args library");
  args_set_doc(args, "For more info see http://prhlt.iti.es");
  args_set_version(args, PRHLT_PROJECT_STRING"\n"PRHLT_BUILD_INFO);
  args_set_bug_report(args, "Report bugs to "PRHLT_PROJECT_BUGREPORT".");
  args_add_module(args, &module);
  args_add_shortcuts(args, shortcuts);

  args_parse_command_line(args, argc, argv);

  args_dump(args, stdout);
  args_delete(args);
  return EXIT_SUCCESS;
}
