#include <prhlt/trace.h>
#include <prhlt/gzip.h>
#include <viterbi/lex.h>
#include <viterbi/parsers/dict-parser/dict-flex.h>

extern int dict_debug;
extern int dict__flex_debug;
int main (int UNUSED(argc), char *argv[]) {
  vocab_t *vocab = vocab_create(1024, NULL);
  hmm_t *hmm = hmm_create();
  lex_t *lex = lex_create(vocab, hmm);
  dict_debug = 1;
  dict__flex_debug = 1;
  debug = 1;
  for (++argv; *argv; ++argv) {
    FILE *file = smart_fopen(*argv, "r");
    dict_load(lex, file);
    smart_fclose(file);
  }
  lex_delete(lex);
  hmm_delete(hmm);
  vocab_delete(vocab);
  return 0;
}
