#include <stdlib.h>
#include <string.h>
#include <utils/constants.h>
#include <viterbi/statistics.h>

stats_verbosity_t stats_verbosity = 0;

void reset_word_expand_stats(stats_t * stats) {
  stats->last_accepted = 0;
  stats->total_expanded = 0;
  stats->total_accepted = 0;
  stats->num_visited_words = 0;
  stats->first_expanded = inf_probability;
  stats->last_expanded = inf_probability;
}
void reset_frame_stats(stats_t * stats) {
  memset(stats, 0, sizeof(stats_t));
  for (int l = 0; l < BEAM_LEVEL_MAX; l++) {
    memset(stats->beam_stats[l].status, 0, BEAM_STATUS_MAX * sizeof(beam_status_t));
  }
  reset_word_expand_stats(stats);
}

void print_word_expand_stats(FILE * out, const stats_t * stats) {
  fprintf(out, "total expanded %8d, total accepted %8d(%6.2f%%), last accepted %8d(%6.2f%%)\n", stats->total_expanded, stats->total_accepted, 100.0
      * (float) stats->total_accepted / (float) stats->total_expanded, stats->last_accepted, 100.0 * (float) stats->last_accepted / (float) stats->total_expanded);
  if (stats->total_words > 0) {
    fprintf(out, "\t\tnum visited words = %d (%f%%)\n", stats->num_visited_words, (100.0 * stats->num_visited_words)/(float)stats->total_words);
  }
  fprintf(out, "\ttotal first = %f, last = %f, diff = %f\n", stats->first_expanded.final, stats->last_expanded.final, stats->first_expanded.final - stats->last_expanded.final);
  fprintf(out, "\tlm    first = %f, last = %f, diff = %f\n", stats->first_expanded.lm, stats->last_expanded.lm, stats->first_expanded.lm - stats->last_expanded.lm);
  fprintf(out, "\tac    first = %f, last = %f, diff = %f\n", stats->first_expanded.acoustic, stats->last_expanded.acoustic, stats->first_expanded.acoustic
      - stats->last_expanded.acoustic);
}

void print_frame_stats(FILE * out, const stats_t * stats) {
   int insertions = 0;
   int rejected   = 0;
   int totalsep[BEAM_LEVEL_MAX];
   memset(totalsep, 0, BEAM_STATUS_MAX * sizeof(beam_status_t));

   for (int l = 0; l < BEAM_LEVEL_MAX; l++) {
     int total = 0;
     for (int s = 0; s < BEAM_STATUS_MAX; s++) total += stats->beam_stats[l].status[s];

     switch (l) {
       case HMM_BEAM:     fprintf(out, "hmm(%8d):", total);break;
       case LEX_BEAM:     fprintf(out, "lex(%8d):", total);break;
       case GRAMMAR_BEAM: fprintf(out, "grm(%8d):", total);break;
     }
     for (int s = 0; s < BEAM_STATUS_MAX; s++) {
       totalsep[s] += stats->beam_stats[l].status[s];
       insertions += stats->beam_stats[l].status[s];
       if (s < INSERT) rejected += stats->beam_stats[l].status[s];

       float ratio = (total > .0)?(100.0*stats->beam_stats[l].status[s])/(float)total:.0;
       switch (s) {
         case REJECT_BEAM:       fprintf(out, "beam");break;
         case REJECT_NO_REPLACE: fprintf(out, "no_r");break;
         case REJECT_FULL:       fprintf(out, "full");break;
         case INSERT:            fprintf(out, "insr");break;
         case REPLACE:           fprintf(out, "repl");break;
       }
       fprintf(out, "  = %8d (%6.2f%%),", stats->beam_stats[l].status[s], ratio);
     }
     fprintf(out, "\n");
   }

   fprintf(out, "tot(%8d):", insertions);
   for (int s = 0; s < BEAM_STATUS_MAX; s++) {
     float ratio = (insertions > .0)?(100.0*totalsep[s])/(float)insertions:.0;
     switch (s) {
       case REJECT_BEAM:       fprintf(out, "beam");break;
       case REJECT_NO_REPLACE: fprintf(out, "no_r");break;
       case REJECT_FULL:       fprintf(out, "full");break;
       case INSERT:            fprintf(out, "insr");break;
       case REPLACE:           fprintf(out, "repl");break;
     }
     fprintf(out, "  = %8d (%6.2f%%),", totalsep[s], ratio);
   }
   fprintf(out, "\n");
   fprintf(out, "total beam:   num insertions = %8d, num rejected = %8d, ratio = %6.2f%%\n",
       insertions, rejected, (100.0*rejected)/(float)insertions);
   fprintf(out, "heap: size = %8d, capacity = %8d, ratio = %6.2f%%\n",
       stats->heap_size, stats->heap_capacity, (100.0*stats->heap_size)/(float)stats->heap_capacity);

   if (stats->hist_count != NULL) {
     int hist_max = 0, hist_total = 0;
     for (int i = 0; i < stats->num_hist; i++) {
       if (stats->hist_count[i] > 0) {
         hist_max++;
         hist_total += stats->hist_count[i];
       }
     }
     fprintf(out, "hist total = %d, max = %8d, ", hist_total, hist_max);
     if (hist_total == 0) {
       fprintf(out, "no grammar transitions\n");
     }
     else {
       fprintf(out, "ratio = %3.2f%%\n", (100.0*hist_max)/((float)hist_total));
     }
   }
   fprintf(out, "max = %f, min = %f, limit = %f, beam = %f\n",
       stats->max_final, stats->min_final, stats->limit_final, stats->beam);
   fprintf(out, "hyps to expand: total = %d, word = %d, ratio = %g\n",
       stats->n_hyps, stats->n_word_hyps, (100.0*(float)stats->n_word_hyps/(float)stats->n_hyps));


   if (stats->total_events[HMM_BEAM] > 0) {
     fprintf(stderr, "hmm events: distinct = %d, total = %d, ratio = %3.2f%%\n",
         stats->distinct_events[HMM_BEAM], stats->total_events[HMM_BEAM],
         (100.0 * (float)stats->distinct_events[HMM_BEAM])/(float)stats->total_events[HMM_BEAM]);
   }

   if (stats->total_events[LEX_BEAM] > 0) {
     fprintf(stderr, "lex events: distinct = %d, total = %d, ratio = %3.2f%%\n",
         stats->distinct_events[LEX_BEAM], stats->total_events[LEX_BEAM],
         (100.0 * (float)stats->distinct_events[LEX_BEAM])/(float)stats->total_events[LEX_BEAM]);
   }

   if (stats->total_events[GRAMMAR_BEAM] > 0) {
     fprintf(stderr, "wrd events: distinct = %d, total = %d, ratio = %3.2f%%\n",
         stats->distinct_events[GRAMMAR_BEAM], stats->total_events[GRAMMAR_BEAM],
         (100.0 * (float)stats->distinct_events[GRAMMAR_BEAM])/(float)stats->total_events[GRAMMAR_BEAM]);
   }

   //   fprintf(out, "num table words = %8d\n", stats->num_table_words);
   fprintf(out, "\n");
}

void print_sample_stats(FILE * out, stats_t ** const stats, int n_stats) {
  if (n_stats == 0) return;

  stats_t total;
  reset_frame_stats(&total);

  for (int i = 0; i < n_stats; i++) {
    if (stats[i] != NULL) {
      for (int l = 0; l < BEAM_LEVEL_MAX; l++) {
        for (int s = 0; s < BEAM_STATUS_MAX; s++) {
          total.beam_stats[l].status[s] += stats[i]->beam_stats[l].status[s];
        }
        total.distinct_events[l] += stats[i]->distinct_events[l];
        total.total_events[l]    += stats[i]->total_events[l];
      }
      total.heap_capacity += stats[i]->heap_capacity;
      total.heap_size += stats[i]->heap_size;
    }
  }
  print_frame_stats(out, &total);
  free(total.hist_count);
}
