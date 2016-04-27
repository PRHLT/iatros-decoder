#ifndef _PT_DRIVER_H

#define _PT_DRIVER_H

#include <viterbi/grammar.h>
#include <stdio.h>

int grammar_load_phrase_table(grammar_t *pt, FILE *file);

#endif // _PT_DRIVER_H
