#ifndef _STATISTICS_H
#define _STATISTICS_H

#include <stdio.h>
#include <iatros/probability.h>

#cmakedefine ENABLE_STATISTICS
#if !defined(SET_STATISTICS_VERBOSITY)
  #if defined(ENABLE_STATISTICS)
    #undef  ENABLE_STATISTICS
    #define ENABLE_STATISTICS stats_verbosity
    #define SET_STATISTICS_VERBOSITY(x) (stats_verbosity = x)
  #else 
    #define ENABLE_STATISTICS (0)
    #define SET_STATISTICS_VERBOSITY(x) 
  #endif
#endif


typedef enum { HMM_BEAM = 0, LEX_BEAM, GRAMMAR_BEAM, BEAM_LEVEL_MAX } beam_level_t;
typedef enum { REJECT_BEAM = 0, REJECT_NO_REPLACE, REJECT_FULL, INSERT, REPLACE, BEAM_STATUS_MAX } beam_status_t;

typedef enum { SV_SHOW_NONE = 0, SV_SHOW_CORPUS, SV_SHOW_SAMPLE, SV_SHOW_FRAME, SV_SHOW_WORD_EXPANSION , SV_SHOW_MAX } stats_verbosity_t;
extern stats_verbosity_t stats_verbosity;

/// Beam statistics
typedef struct {
  int status[BEAM_STATUS_MAX];
} beam_stats_t;

/// General statistics
typedef struct {
  beam_stats_t beam_stats[BEAM_LEVEL_MAX]; //< beam statistics for hmm, lex and grammar

  int * hist_count; //< count of incoming hypotheses to history
  int num_hist;   //< number of histories
  int num_table_words;

  int num_visited_words;
  int total_words;
  int num_states;
  int num_seen_states;
  int num_initial_states;
  int num_seen_initial_states;

  int last_accepted;
  int total_expanded;
  int total_accepted;
  probability_t first_expanded;
  probability_t last_expanded;

  int heap_capacity; ///< maximum number of heap elements
  int heap_size;     ///< number of heap elements

  float beam;           ///< beam in current frame
  float max_final;      ///< maximum probability of the frame
  float min_final;      ///< minimum probability of the frame
  float limit_final;    ///< minimum probability that can pass the beam

  int n_hmm_counts;
  int *hmm_counts;///< counts of each hmm state expansion trials
  int n_lex_counts;
  int *lex_counts;///< counts of each lex expansion trials
  int n_word_counts;
  int *word_counts;///< counts of each word expansion trials
  int total_events[BEAM_LEVEL_MAX];///< number of total expansions
  int distinct_events[BEAM_LEVEL_MAX];///< number of distinct expansions

  int n_hyps;      ///< n of hyps to be expanded
  int n_word_hyps; ///< n of word hyps to be expanded
} stats_t;


void reset_frame_stats(stats_t * stats);
void reset_word_expand_stats(stats_t * stats);
void print_frame_stats(FILE * out, const stats_t * stats);
void print_word_expand_stats(FILE * out, const stats_t * stats);
void print_sample_stats(FILE * out, stats_t ** const stats, int n_stats);

#endif // _STATISTICS_H
