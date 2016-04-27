/*
 * args.c
 *
 *  Created on: 29-dic-2008
 *      Author: valabau
 */

#include "utils.h"
#include "args.h"
#include "args_friend.h"
#include "trace.h"
#include "gzip.h"

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


static const void *ARG_UNDEFINED = (void *)-1;


static const arg_module_t default_module = {NULL, "Default options",
    {
        {"help", ARG_STRING, NULL, 0, "Show help and exit. If parameter is given print module help"},
        {"config", ARG_FILE, NULL, 0, "Configuration file"},
        {"usage", ARG_BOOL, "false", 0, "Show usage and exit"},
        {"version", ARG_BOOL, "false", 0, "Show version"},
        {NULL, ARG_END_MODULE, NULL, 0, NULL}
    }
};

static const arg_shortcut_t default_shortcuts[] = {
    {"c", "config"},
    {"h", "help"},
    {"V", "version"},
    {NULL, NULL}

};

args_t *args_create() {
  args_t *args = (args_t *) calloc(1, sizeof(args_t));
  MEMTEST(args);
  args->values = hash_create(271, ARG_UNDEFINED);
  args->shortcuts = hash_create(271, NULL);
  args->options = hash_create(271, NULL);
  args->option_shortcuts = hash_create(271, NULL);
  args->modules = vector_create();
  args->allow_free_parameters = false;
  args->enable_default_module = true;
  args->n_remaining = 0;
  args->remaining = NULL;
  args->summary = NULL;
  args->doc = NULL;
  args->version = NULL;
  args->bug_report = NULL;

  args_add_shortcuts(args, default_shortcuts);
  return args;
}

void args_delete(args_t *args) {
  hash_delete(args->values, true);
  hash_delete(args->shortcuts, false);
  hash_delete(args->options, false);
  for (int s = 0; s < args->option_shortcuts->hsize; s++) {
    hash_element_t *elem = args->option_shortcuts->htable[s];
    while (elem != NULL) {
      vector_delete(((vector_t *)elem->data));
      elem = elem->p;
    }
  }
  hash_delete(args->option_shortcuts, false);
  vector_delete(args->modules);
  free(args->remaining);
  free(args->summary);
  free(args->doc);
  free(args->version);
  free(args->bug_report);
  free(args);
}

void args_set_summary(args_t *args, const char *summary) {
  free(args->summary);
  if (summary != NULL) {
    args->summary = strdup(summary);
  }
  else {
    args->summary = NULL;
  }
}

void args_set_doc(args_t *args, const char *doc) {
  free(args->doc);
  if (doc != NULL) {
    args->doc = strdup(doc);
  }
  else {
    args->doc = NULL;
  }
}
void args_set_version(args_t *args, const char *version) {
  free(args->version);
  if (version != NULL) {
    args->version = strdup(version);
  }
  else {
    args->version = NULL;
  }
}
void args_set_bug_report(args_t *args, const char *bug_report) {
  free(args->bug_report);
  if (bug_report) {
    args->bug_report = strdup(bug_report);
  }
  else {
    args->bug_report = NULL;
  }
}

void args_usage(args_t *args, FILE *out) {
  fprintf(out, "Usage: %s", args->program_name);
  for (int s = 0; s < args->shortcuts->hsize; s++) {
    hash_element_t *elem = args->shortcuts->htable[s];
    while (elem != NULL) {
      fprintf(out, " [ -%s ]", elem->key);
      elem = elem->p;
    }
  }
  fprintf(out, "\n\n");
  fprintf(out, "Try `%s --help` for more help.\n\n", args->program_name);
}

static const char *arg_param_name[] = {
    "BOOL", "INT", "FLOAT", "STRING", "FILE", "DIR", "END_OF_MODULE"
};


static void print_module(const args_t *args, const arg_module_t *module, FILE *out) {
  if (module->doc != NULL) {
    fprintf(out, "\n %s:\n", module->doc);
  }
  const arg_option_t *option = module->options;
  while (option->type != ARG_END_MODULE) {
    char fullname[1024] = "";
    if (module->name != NULL) {
      strcat(fullname, module->name);
      strcat(fullname, ".");
    }
    strcat(fullname, option->name);

    fprintf(out, " ");
    vector_t *shortcuts = (vector_t *) hash_search(fullname, strlen(fullname)+1, args->option_shortcuts);
    if (shortcuts != NULL) {
      for (size_t i = 0; i < shortcuts->n_elems; i++) {
        const char *sc = (const char *) shortcuts->data[i];
        fprintf(out, " %s%s,", (strlen(sc) == 1)?"-":"--", sc);
      }
    }
    else {
      fprintf(out, "    ");
    }

    fprintf(out, " --%-24s %-6s\t\t%s\n", fullname,
        arg_param_name[option->type],
        //(option->default_value != NULL)?option->default_value:"STRING",
        option->doc);
    option++;
  }
}

void args_help(args_t *args, const char *module_name, FILE *out) {
  arg_module_t const * module = NULL;
  if (module_name != NULL) {
    for (size_t m = 0; m < args->modules->n_elems; m++) {
      arg_module_t const * current_module = (arg_module_t *)args->modules->data[m];
      if (current_module->name != NULL && strcmp(current_module->name, module_name) == 0) {
        module = current_module;
        break;
      }
    }
  }

  REQUIRE(module_name == NULL || module != NULL, "Unknown module '%s'", module_name);


  fprintf(out, "Usage: %s [OPTIONS...]\n\n", args->program_name);
  if (args->summary != NULL) fprintf(out, "%s\n\n", args->summary);

  if (module == NULL) {
    int n_modules = 0;
    { // print general modules
      for (size_t m = 0; m < args->modules->n_elems; m++) {
        module = (arg_module_t *)args->modules->data[m];
        if (module->name == NULL) {
          print_module(args, (arg_module_t *)args->modules->data[m], out);
        }
        else {
          n_modules++;
        }
      }
    }

    if (args->enable_default_module) print_module(args, &default_module, out);

    if (n_modules > 0) fprintf(out, "\n Modules:\n");
    for (size_t m = 0; m < args->modules->n_elems; m++) {
      module = (arg_module_t *)args->modules->data[m];
      if (module->name != NULL) {
        fprintf(out, "   %s\t\t\t\t\t%s\n", module->name, (module->doc != NULL)?module->doc:"Undocumented");
      }
    }

    if (args->doc != NULL) fprintf(out, "\n\n%s\n", args->doc);
  }
  else {
    print_module(args, module, out);
  }
  if (args->bug_report != NULL) fprintf(out, "\n\n%s\n", args->bug_report);
}

void args_version(args_t *args, FILE *out) {
  fprintf(out, "%s\n", args->version);
}

void args_write_default_config_file(args_t *args, FILE *out) {
  for (size_t m = 0; m < args->modules->n_elems; m++) {
    arg_module_t *module = (arg_module_t *)args->modules->data[m];
    if (module->doc != NULL) {
      fprintf(out, "# %s\n", module->doc);
    }
    if (module->name != NULL) {
      fprintf(out, "[%s]\n", module->name);
    }
    fprintf(out, "\n\n");
    arg_option_t *option = module->options;
    while (option->type != ARG_END_MODULE) {
      fprintf(out, "# %s\n", option->doc);
      if (option->default_value != NULL) {
        fprintf(out, "%s = %s\n", option->name, option->default_value);
      }
      else {
        fprintf(out, "# %s = %s\n", option->name, arg_param_name[option->type]);
      }
      fprintf(out, "\n");
      option++;
    }
    fprintf(out, "\n");
  }
}

void args_write_config_file(args_t *args, FILE *out) {
  for (size_t m = 0; m < args->modules->n_elems; m++) {
    arg_module_t *module = (arg_module_t *)args->modules->data[m];
    if (module->doc != NULL) {
      fprintf(out, "# %s\n", module->doc);
    }
    if (module->name != NULL) {
      fprintf(out, "[%s]\n", module->name);
    }
    fprintf(out, "\n\n");
    arg_option_t *option = module->options;
    while (option->type != ARG_END_MODULE) {
      fprintf(out, "# %s\n", option->doc);
      arg_error_t error = ARG_OK;
      const char *value = args_get_string(args, option->name, &error);
      if (error == ARG_OK) fprintf(out, "%s = %s\n", option->name, value);
      else fprintf(out, "# %s = %s\n", option->name, arg_param_name[option->type]);
      fprintf(out, "\n");
      option++;
    }
    fprintf(out, "\n");
  }
}

static void args_add_option(args_t *args, const arg_module_t *module, const arg_option_t *option) {
  char fullname[1024] = "";
  if (module->name != NULL) {
    strcat(fullname, module->name);
    strcat(fullname, ".");
  }
  strcat(fullname, option->name);
  args_update(args, fullname, option->default_value);
  hash_insert(fullname, strlen(fullname) + 1, (void *)option, args->options);
}

void args_add_module(args_t *args, const arg_module_t *module) {
  bool already_inserted = false;
  for (size_t m = 0; m < args->modules->n_elems; m++) {
    arg_module_t *existing_module = (arg_module_t *)args->modules->data[m];
    if (module->name == existing_module->name ||
        (module->name != NULL && existing_module->name != NULL &&
            strcmp(module->name, existing_module->name) == 0))
    {
      ERROR("WARNING: adding module '%s' which has already been inserted", module->name);
      already_inserted = true;
    }
  }
  if (!already_inserted) {
    const arg_option_t * option = module->options;
    while (option->type != ARG_END_MODULE) {
      if (option->name != NULL) {
        args_add_option(args, module, option);
      }
      option++;
    }
    vector_append(args->modules, (void *)module);
  }
}

bool fullname_is_default_option(args_t *args, const char *fullname) {
  if (!args->enable_default_module || fullname == NULL) return false;
  arg_option_t const *option = default_module.options;
  while (option->type != ARG_END_MODULE) {
    if (strcmp(fullname, option->name) == 0) return true;
    option++;
  }
  return false;
}

void args_add_shortcuts(args_t *args, const arg_shortcut_t *shortcuts) {
  while (shortcuts->fullname != NULL) {
    bool is_default_option = fullname_is_default_option(args, shortcuts->fullname);
    arg_option_t *option = (arg_option_t *)hash_search(shortcuts->fullname, strlen(shortcuts->fullname) + 1, args->options);
    CHECK(is_default_option || option != NULL, "WARNING: Trying to add shortcut '%s%s' to unknown argument '--%s'",
        (strlen(shortcuts->shortcut) == 1)?"-":"--", shortcuts->shortcut, shortcuts->fullname);

    if (is_default_option || option != NULL) {
      hash_insert(shortcuts->shortcut, strlen(shortcuts->shortcut) + 1, (char *)shortcuts->fullname, args->shortcuts);
      vector_t *vector = (vector_t *)hash_search(shortcuts->fullname, strlen(shortcuts->fullname) + 1, args->option_shortcuts);
      if (vector == NULL) {
        vector = vector_create();
        hash_insert(shortcuts->fullname, strlen(shortcuts->fullname) + 1, vector, args->option_shortcuts);
      }
      vector_append(vector, (char *)shortcuts->shortcut);
    }
    shortcuts++;
  }
}

inline char *args_search(const args_t *args, const char *name) {
  return (char *) hash_search((char *) name, strlen(name) + 1, args->values);
}

static inline bool value_is_true(const char *value) {
  return strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcasecmp(value, "on") == 0;
}

static inline bool value_is_false(const char *value) {
  return strcasecmp(value, "false") == 0 || strcmp(value, "0") == 0 || strcasecmp(value, "false") == 0;
}

static bool args_process_default_option(args_t *args, const char *name, const char *value) {
  bool has_parameter = false;
  if (strcmp(name, "help") == 0) {
    if (value != NULL && value[0] != '-') {
      args_help(args, value, stderr);
      has_parameter = true;
    }
    else {
      args_help(args, NULL, stderr);
    }
    args_delete(args);
    exit(EXIT_SUCCESS);
  }
  else if (strcmp(name, "usage") == 0) {
    args_usage(args, stderr);
    args_delete(args);
    exit(EXIT_SUCCESS);
  }

  else if (strcmp(name, "version") == 0) {
    args_version(args, stderr);
    args_delete(args);
    exit(EXIT_SUCCESS);
  }

  else if (strcmp(name, "config") == 0) {
    if (value != NULL) {
      FILE *file = smart_fopen(value, "r");
      CHECK_SYS_ERROR(file != NULL, "Couldn't open config file '%s'\n", value);
      args_update_from_file(args, file);
      smart_fclose(file);
      has_parameter = true;
    }
  }
  return has_parameter;
}

static bool args_update_from_command_line(args_t *args, const arg_option_t *option,
                                          const char *name, const char *value)
{
  bool has_parameter = true;

  if (option != NULL && option->type == ARG_BOOL) {
    if (value == NULL || (!value_is_true(value) && !value_is_false(value))) {
      has_parameter = false;
    }
  }
  if (has_parameter) {
    args_update(args, name, value);
  }
  else {
    if (option != NULL) {
      args_update(args, name, (value_is_true(option->default_value))?"false":"true");
    }
    else {
      args_update(args, name, "true");
    }
  }
  return has_parameter;
}

size_t args_parse_command_line(args_t *args, int argc, char **argv) {
  size_t n_errors = 0;
  strncpy(args->program_name, argv[0], 1024);
  for (int a = 1; a < argc && argv[a] != NULL; a++) {
    char *name_copy = strdup(argv[a]);
    char *name = name_copy;
    char const *value = argv[a+1];

    {
      char *pos = strchr(name, '=');
      if (pos != NULL) {
        *pos = '\0';
        value = pos + 1;
      }
    }

    // it is a long option
    arg_option_t const *option = NULL;
    if (strncmp(name, "--", 2) == 0) {
      name += 2; // eat up '--'
      option = (arg_option_t *)hash_search(name, strlen(name) + 1, args->options);
      if (option == NULL) {
        char *sc_name = (char *)hash_search(name, strlen(name) + 1, args->shortcuts);
        if (sc_name != NULL) {
          name = sc_name;
          option = (arg_option_t *)hash_search(name, strlen(name) + 1, args->options);
        }
      }
    }
    // it is a shortcut
    else if (strncmp(name, "-", 1) == 0) {
      name++; // eat up '-'
      char *sc_name = (char *)hash_search(name, strlen(name) + 1, args->shortcuts);
      if (sc_name != NULL) {
        name = sc_name;
        option = (arg_option_t *)hash_search(name, strlen(name) + 1, args->options);
      }
    }

    // is a default option
    if (args->enable_default_module && fullname_is_default_option(args, name)) {
      bool has_parameter = args_process_default_option(args, name, value);
      if (value == argv[a+1] && has_parameter) a++;
    }
    else if (name != argv[a]) { // we have eaten up '-' symbols so it is an option
      if (option == NULL && !args->allow_free_parameters) {
        FAIL("Invalid option '%s'", name);
        fprintf(stderr, "Try `%s --help' or `%s --usage' for more information.", args->program_name, args->program_name);
        args_usage(args, stderr);
        n_errors++;
      }
      else {
        bool has_parameter = args_update_from_command_line(args, option, name, value);
        if (value == argv[a+1] && has_parameter) a++;
      }
    }
    else { // it is not an option but a remaining arguments
      args->remaining = (char **) realloc(args->remaining, (args->n_remaining + 1) * sizeof(char const *));
      args->remaining[args->n_remaining++] = (char *)argv[a];
    }
    free(name_copy);
  }
  return n_errors;
}

void args_update(args_t *args, const char *name, const char *value) {
  char *old_value = (char *)hash_remove(name, strlen(name) + 1, args->values);
  if (old_value != ARG_UNDEFINED) free(old_value);
  hash_insert(name, strlen(name) + 1, (value == NULL)?NULL:strdup(value), args->values);
}

bool args_get_bool(const args_t *args, const char *name, arg_error_t *error) {
  if (error != NULL) *error = ARG_OK;
  char *value = args_search(args, (char *)name);
  if (value == ARG_UNDEFINED || value == NULL) {
    if (error != NULL) *error = ARG_NOT_FOUND;
    return false;
  }
  if (value_is_true(value)) {
    return true;
  }
  else if (value_is_false(value)) {
    return false;
  }
  else {
    ERROR("Argument '%s' = '%s' cannot be converted to boolean", name, value);
    *error = ARG_WRONG_TYPE;
    return false;
  }
}

int args_get_int(const args_t *args, const char *name, arg_error_t *error) {
  if (error != NULL) *error = ARG_OK;
  char *value = args_search(args, (char *) name);
  if (value == ARG_UNDEFINED || value == NULL) {
    if (error != NULL) *error = ARG_NOT_FOUND;
    return 0;
  }

  if (strcasecmp(value, "inf") == 0) {
    return INT_MAX;
  }
  else if (strcasecmp(value, "-inf") == 0) {
    return -INT_MAX;
  }
  else {
    char *endptr;
    int val = strtol(value, &endptr, 10);
    CHECK_SYS_ERROR(endptr != value,
        "WARNING: wrong integer argument: '%s' = '%s'\n",
        name, value);
    if (endptr != value) {
      return val;
    }
    else {
      if (error != NULL) *error = ARG_WRONG_TYPE;
      return 0;
    }
  }
}

float args_get_float(const args_t *args, const char *name, arg_error_t *error) {
  if (error != NULL) *error = ARG_OK;
  char *value = args_search(args, (char *)name);
  if (value == ARG_UNDEFINED || value == NULL) {
    if (error != NULL) *error = ARG_NOT_FOUND;
    return 0;
  }

  if (strcasecmp(value, "inf") == 0) {
    return INFINITY;
  }
  else if (strcasecmp(value, "-inf") == 0) {
    return -INFINITY;
  }
  else {
    char *endptr;
    float val = strtod(value, &endptr);
    CHECK_SYS_ERROR(endptr != value,
        "WARNING: wrong float argument: '%s' = '%s'\n",
        name, value);
    if (endptr != value) {
      return val;
    }
    else {
      if (error != NULL) *error = ARG_WRONG_TYPE;
      return 0;

    }
  }
}

const char *args_get_string(const args_t *args, const char *name, arg_error_t *error) {
  if (error != NULL) *error = ARG_OK;
  char *value = args_search(args, name);
  if (value == ARG_UNDEFINED) {
    if (error != NULL) *error = ARG_NOT_FOUND;
    return NULL;
  }
  return value;
}

void args_dump_format(const args_t *args, const char *val_sep, const char *reg_sep, FILE *out) {
  for (int i = 0; i < args->values->hsize; i++) {
    hash_element_t *he = args->values->htable[i];
    while (he != NULL) {
      if (he->data != NULL) {
        fprintf(out, "%s%s%s%s", (char *) he->key, val_sep, (char *) he->data, reg_sep);
      }
      he = he->p;
    }
  }
}

void args_dump(const args_t *args, FILE *out) {
  fprintf(out, "########## arguments ##########\n");
  args_dump_format(args, " = ", "\n", out);
  fprintf(out, "###############################\n\n");
}

