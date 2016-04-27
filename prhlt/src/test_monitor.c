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
#include "monitor.h"


static const arg_module_t module = {NULL, "General options",
    {
      {"exit-value", ARG_INT, "0", 0, "Set exit value"},
      {"segfault", ARG_BOOL, "false", 0, "Provoke segmentation fault"},
      {"double-free", ARG_BOOL, "false", 0, "Provoke double-free segmentation fault"},
      {"verbosity", ARG_INT, "0", 0, "Set verbosity level"},
      {NULL, ARG_END_MODULE, NULL, 0, NULL}
    }
};

static const arg_shortcut_t shortcuts[] = {
    {"e", "exit-value"},
    {"s", "segfault"},
    {"d", "double-free"},
    {"v", "verbosity"},
    {NULL, NULL}
};

int main(int argc, char *argv[]) {
  args_t *args = args_create();
  args_set_summary(args, "Test for monitor library");
  args_set_doc(args, "For more info see http://prhlt.iti.es");
  args_set_version(args, PRHLT_PROJECT_STRING"\n"PRHLT_BUILD_INFO);
  args_set_bug_report(args, "Report bugs to "PRHLT_PROJECT_BUGREPORT".");
  args_add_module(args, &module);
  args_add_shortcuts(args, shortcuts);

  if (args_parse_command_line(args, argc, argv) > 0) {
    return EXIT_FAILURE;
  }

  arg_error_t error = ARG_OK;
  INIT_TRACE(args_get_int(args, "verbosity", &error));

  start_monitoring();


  args_dump(args, stdout);

  int exit_value = args_get_int(args, "exit-value", &error);
  if (error != ARG_OK) {
    exit_value = EXIT_SUCCESS;
  }

  bool provoke_segfault = args_get_bool(args, "segfault", &error);
  if (error == ARG_OK && provoke_segfault) {
    int *ptr = NULL;
     *ptr = 1;
     PRINT("Should have segfaulted\n");
  }

  bool provoke_double_free = args_get_bool(args, "double-free", &error);
  if (error == ARG_OK && provoke_double_free) {
    int *ptr = (int *)malloc(sizeof(int));
    free(ptr);
    free(ptr);
    PRINT("Should have segfaulted\n");
  }

  args_delete(args);
  return exit_value;
}
