#ifndef _HMM_DRIVER_H

#define _HMM_DRIVER_H

#include <viterbi/hmm.h>
#include <stdio.h>

// definitions from hmm that should'nt be
// exported to other files
double calculate_gconst(variance_t *variance, int num_features);

#endif // _HMM_DRIVER_H
