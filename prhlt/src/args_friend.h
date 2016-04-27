/*
 * args_friend.h
 *
 *  Created on: 03-jun-2009
 *      Author: valabau
 */

#ifndef ARGS_FRIEND_H_
#define ARGS_FRIEND_H_

#include "vector.h"
#include "hash.h"

struct args_t {
  char program_name[1024];
  vector_t *modules;
  hash_t *values;
  hash_t *options;
  hash_t *option_shortcuts;
  hash_t *shortcuts;
  bool allow_free_parameters;
  bool enable_default_module;
  int n_remaining;
  char **remaining;
  char *summary;
  char *doc;
  char *version;
  char *bug_report;
};

#endif /* ARGS_FRIEND_H_ */
