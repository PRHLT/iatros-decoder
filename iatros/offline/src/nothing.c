/*! \mainpage


 \section intro Introduction

  This is the documentation of iAtros offline frontend, the new Improved Automatic Trainable RecOgnition System

  \section config Configuration File

\verbatim
# General options


# Enables saving lattices
save-lattices = false

# Directory where lattices will be saved
# lattice-directory = DIR

# List of files to process
# samples = FILE

# List of transcriptions for the samples
# transcriptions = STRING

# List of translations for the samples
# translations = STRING

# Save vocabulary to file
# save-vocab = STRING

# Enables forced recognition
# do-forced-recognition = BOOL

# Print realtime factor
print-time = false

# Print hypothesis score
print-score = false

# Split the corpus into parts and run just one. Ex. --part '1:4' runs the 1st part out of 4
# part = STRING

# Set verbosity level
verbosity = 0

# Set statistics verbosity level
statistics-verbosity = 0

# Print default config file
print-default = false

# Print final config file
print-config = false


# decoder module
[decoder]


# Hidden Markov Model in HTK format
# hmm = FILE

# Lexicon file
# lexicon = FILE

# Lexicon format (ATROS, HTK)
lexicon-type = ATROS

# Main grammar
# grammar = FILE

# Type of the main grammar (FSM, NGRAM). NGRAM by default
grammar-type = NGRAM

# Input grammar
# input-grammar = FILE

# Output grammar
# output-grammar = FILE

# Relative beam
beam = inf

# Grammar scale factor
grammar-scale-factor = 1

# Grammar scale factor for the input grammar
input-grammar-scale-factor = 1

# Grammar scale factor for the output grammar
output-grammar-scale-factor = 1

# Word insertion penalty
word-insertion-penalty = 0

# Word insertion penalty for output words
output-word-insertion-penalty = 0

# Start symbol. None by default. Typically <s>
# start = STRING

# End symbole. None by default. Typically </s>
# end = STRING

# Unknown symbol. None by default. Typically <unk>
unk = <unk>

# Silence symbol. None by default
# silence = STRING

# Score for the silence word. '1.0' by default
silence-score = 1.0

# Maximum number of hypotheses allowed by frame. 0 means no histogram pruning
histogram-pruning = 0

# Phrase table in moses format
# phrase-table = FILE

# Weights for the log-lineal model in the phrase table separated by commas
# weights = STRING

# Enables acoustic early pruning
do-acoustic-early-pruning = true

# Enables the creation of dummy acoustic models
# create-dummy-acoustic-models = BOOL

# List of the categories with the associated grammars
# categories = FILE


# lattice module
[lattice]


# Number of nbest incoming words to be stored by the final lattice state.'1' by default. '0' to indicate infinite nbests
nbest = 1

# Number of nbest incoming words to be stored per each lattice state.By default it equals to nbest
nnode = -1

\endverbatim

*/
