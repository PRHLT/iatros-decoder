#ifndef _FSM_DRIVER_H
#define _FSM_DRIVER_H

#include <stdio.h>
#include <viterbi/grammar.h>

int grammar_load_fsm(grammar_t *fsm, FILE *file);

#endif // _FSM_DRIVER_H
