# Install include files
install_headers(prhlt
  ${CMAKE_CURRENT_SOURCE_DIR}/../version.h.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/../config.h.cmake
  args.h
  args_friend.h
  args.h
  constants.h
  extended_vocab.h
  gzip.h
  hash.h
  trace.h
  utils.h
  vector.h
  vocab.h
  monitor.h
)

# Add the parser to process it
append_parser_sources(args_parser_SRCS args-parser.y args-scanner.l)
list(APPEND prhlt_SRCS args.c args-flex.c trace.c gzip.c vector.c hash.c ${args_parser_SRCS})
list(APPEND prhlt_SRCS extended_vocab.c utils.c vocab.c)
list(APPEND prhlt_SRCS sigsegv.c monitor.c)



# Create the library and export configuration files 
add_static_and_dynamic_library(prhlt ${prhlt_SRCS})
static_and_dynamic_link_libraries(prhlt m)
if(HAVE_BACKTRACE)
    static_and_dynamic_link_libraries(prhlt dl)
endif(HAVE_BACKTRACE)
export_static_and_dynamic_libraries(prhlt-config prhlt)
install(EXPORT prhlt-config DESTINATION lib)

#test args
add_executable(test_args test_args.c)
target_link_libraries(test_args prhlt)

#test sigsegv
add_executable(test_sigsegv segfaulter.c)
target_link_libraries(test_sigsegv prhlt)

#test monitor
add_executable(test_monitor test_monitor.c)
target_link_libraries(test_monitor prhlt)

