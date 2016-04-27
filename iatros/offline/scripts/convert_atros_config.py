#! /usr/bin/env python



general_opts = [
  ( "UtterancesFile", "samples" ),
  ( "SourceReferences", "transcriptions" ),
  ( "TargetReferences", "translations" ),
  ( "ForcedRecognition", "do-forced-recognition" ),
  ( "WordGraph", "save-lattices" ),
  ( "FileWordGraph", "lattice-directory" )
    ]
decoder_opts = [
  ( "HmmFile", "hmm" ),
  ( "LexiconFile", "lexicon" ),
  ( "TypeLex", "lexicon-type" ),
  ( "GrammarFile", "grammar" ),
  ( "TypeGrammar", "grammar-type" ),
  ( "InputGrammarFile", "input-grammar" ),
  ( "OutputGrammarFile", "output-grammar" ),
  ( "GrammarBeam", "beam" ),
  ( "GrammarScaleFactor", "grammar-scale-factor" ),
  ( "InputGrammarScaleFactor", "input-grammar-scale-factor" ),
  ( "OutputGrammarScaleFactor", "output-grammar-scale-factor" ),
  ( "WordInsertionPenalty", "word-insertion-penalty" ),
  ( "OutputWordInsertionPenalty", "output-word-insertion-penalty" ),
  ( "Unknown", "unk" ),
  ( "Silence", "silence" ),
  ( "Sil_prob", "silence-score" ),
  ( "InitialSentence", "start" ),
  ( "EndSentence", "end" ),
  ( "TamStructures", "histogram-pruning" ),
  ( "Weights", "weights" ),
  ( "AcousticEarlyPruning", "do-acoustic-early-pruning" ),
  ( "Categories", "categories" )
    ]
lattice_opts = [
  ( "N-best", "nbest" ),
  ( "N-node", "nnode" )
    ]


modules = [ (None, general_opts), ("decoder", decoder_opts), ("lattice", lattice_opts) ]

from sys import argv

conf = {}
for line in open(argv[1]):
  line = line.strip().split()
  if len(line) == 0 or line[0][0] == "#" : continue
  name = line[0]
  value = " ".join(line[1:])
  conf[name] = value


for (module_name, args) in modules:
  if module_name: print "[%s]" % module_name
  for (name, arg) in args:
    if name in conf:
      print "%s = %s" % (arg, conf[name])
  print

