#################################################################
#                    Create iAtros library                      # 
#################################################################

# Install include files
install_headers(iatros 
  ${CMAKE_SOURCE_DIR}/version.h.cmake
  ${CMAKE_SOURCE_DIR}/config.h.cmake
  viterbi/cat.h
  viterbi/hmm.h
  viterbi/lex.h
  viterbi/grammar.h
  viterbi/grammar_search.h
  viterbi/lattice.h
  viterbi/heap.h
  viterbi/decoder.h
  viterbi/search.h
  viterbi/viterbi.h
  viterbi/probability.h
  viterbi/cat.h
  viterbi/hypothesis.h
  viterbi/features.h
  viterbi/statistics.h
)

set(iatros_SRCS)

# Add the parser to process it
append_parser_sources(ng_parser_SRCS viterbi/parsers/ng-parser/ng-parser.y viterbi/parsers/ng-parser/ng-scanner.l)
list(APPEND iatros_SRCS viterbi/parsers/ng-parser/ng.c viterbi/parsers/ng-parser/ng-flex.c ${ng_parser_SRCS})

append_parser_sources(pt_parser_SRCS viterbi/parsers/pt-parser/pt-parser.y viterbi/parsers/pt-parser/pt-scanner.l)
list(APPEND iatros_SRCS viterbi/parsers/pt-parser/pt.c viterbi/parsers/pt-parser/pt-flex.c ${pt_parser_SRCS})

append_parser_sources(fsm_parser_SRCS viterbi/parsers/fsm-parser/fsm-parser.y viterbi/parsers/fsm-parser/fsm-scanner.l)
list(APPEND iatros_SRCS viterbi/parsers/fsm-parser/fsm.c viterbi/parsers/fsm-parser/fsm-flex.c ${fsm_parser_SRCS})

#append_parser_sources(lat_parser_SRCS viterbi/parsers/lat-parser/lat-parser.y viterbi/parsers/lat-parser/lat-scanner.l)
#list(APPEND iatros_SRCS viterbi/parsers/lat-parser/lat.c viterbi/parsers/lat-parser/lat-flex.c ${lat_parser_SRCS})

append_parser_sources(lex_parser_SRCS viterbi/parsers/lex-parser/lex-parser.y viterbi/parsers/lex-parser/lex-scanner.l)
list(APPEND iatros_SRCS viterbi/parsers/lex-parser/lex-flex.c ${lex_parser_SRCS})

append_parser_sources(dict_parser_SRCS viterbi/parsers/dict-parser/dict-parser.y viterbi/parsers/dict-parser/dict-scanner.l)
list(APPEND iatros_SRCS viterbi/parsers/dict-parser/dict-flex.c ${dict_parser_SRCS})

append_parser_sources(hmm_parser_SRCS viterbi/parsers/hmm-parser/hmm-parser.y  viterbi/parsers/hmm-parser/hmm-scanner.l)
list(APPEND iatros_SRCS viterbi/parsers/hmm-parser/hmm-flex.c ${hmm_parser_SRCS})

# Add the models
list(APPEND iatros_SRCS viterbi/lex.c viterbi/hmm.c viterbi/dict.c)
list(APPEND iatros_SRCS viterbi/grammar.c viterbi/grammar_search.c viterbi/cat.c)


# Add the search 
list(APPEND iatros_SRCS viterbi/features.c viterbi/hypothesis.c viterbi/heap.c viterbi/lattice.c viterbi/viterbi.c)
list(APPEND iatros_SRCS viterbi/decoder.c viterbi/search.c)

# Add the statistics
if(ENABLE_STATISTICS)
  message(STATUS "Enabling statistics")
  list(APPEND iatros_SRCS viterbi/statistics.c)
else(ENABLE_STATISTICS)
  message(STATUS "Statistics are disabled")
endif(ENABLE_STATISTICS)

# Create the library and link to audio library
add_static_and_dynamic_library(iatros ${iatros_SRCS})
target_link_libraries(iatros m prhlt)
target_link_libraries(iatros_nonshared m prhlt_nonshared)
export_static_and_dynamic_libraries(iatros-config iatros)

install(EXPORT iatros-config DESTINATION lib)

add_executable(dict-test viterbi/parsers/dict-parser/dict-test.c)
target_link_libraries(dict-test iatros_nonshared)

