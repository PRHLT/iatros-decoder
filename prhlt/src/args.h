/*
 * args.h
 *
 *  Created on: 29-dic-2008
 *      Author: valabau
 */

#ifndef ARGS_H_
#define ARGS_H_

#ifndef __cplusplus
// C99 bool type
#include <stdbool.h>
// for non-C99 compilant use this:
// define a new bool type that looks like a c++ bool
//typedef enum {false = 0, true = 1} bool;
#endif

#include <stdio.h>

typedef enum {
  ARG_OK, ARG_WRONG_TYPE, ARG_NOT_FOUND
} arg_error_t;

typedef enum {
  ARG_FLAGS_NONE = 0,
  ARG_FLAG_REQUIRED = 0x1 << 0,
  ARG_MULTIPLE = 0x1 << 1
} arg_flags_t;

typedef enum {
  ARG_BOOL, ARG_INT, ARG_FLOAT, ARG_STRING, ARG_FILE, ARG_DIR, ARG_END_MODULE
} arg_type_t;


typedef struct {
  char const *shortcut;
  char const *fullname;
} arg_shortcut_t;

typedef struct {
  char const *name;
  arg_type_t type;
  char const *default_value;
  arg_flags_t flags;
  char const *doc;
} arg_option_t;

typedef struct {
  char const *name;
  char const *doc;
  arg_option_t options[256];
} arg_module_t;



typedef struct args_t args_t;

#ifdef __cplusplus
extern "C" {
#endif

args_t *args_create();
void args_delete(args_t *args);

void args_set_summary(args_t *args, const char *summary);
void args_set_doc(args_t *args, const char *doc);
void args_set_version(args_t *args, const char *version);
void args_set_bug_report(args_t *args, const char *bug_report);


void args_usage(args_t *args, FILE *out);
void args_help(args_t *args, const char *module, FILE *out);

void args_add_module(args_t *args, const arg_module_t *module);
void args_add_shortcuts(args_t *args, const arg_shortcut_t *shortcuts);

size_t args_parse_command_line(args_t *args, int argc, char **argv);

void args_update(args_t *args, const char *name, const char *value);
int args_update_from_file(args_t *args, FILE *file);

bool args_get_bool(const args_t *args, const char *name, arg_error_t *error);
int args_get_int(const args_t *args, const char *name,  arg_error_t *error);
float args_get_float(const args_t *args, const char *name, arg_error_t *error);
const char *args_get_string(const args_t *args, const char *name, arg_error_t *error);
void args_dump(const args_t *args, FILE *out);
void args_dump_format(const args_t *args, const char *val_sep, const char *reg_sep, FILE *out);
void args_write_default_config_file(args_t *args, FILE *out);
void args_write_config_file(args_t *args, FILE *out);

#ifdef __cplusplus
}
#endif


#endif /* ARGS_H_ */
