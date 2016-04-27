SET( CMAKE_CXX_FLAGS_TEST "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage" CACHE STRING
    "Flags used by the C++ compiler during test builds."
    FORCE )
SET( CMAKE_C_FLAGS_TEST "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage" CACHE STRING
    "Flags used by the C compiler during test builds."
    FORCE )
SET( CMAKE_EXE_LINKER_FLAGS_TEST
    "-fprofile-arcs -ftest-coverage" CACHE STRING
    "Flags used for linking binaries during test builds."
    FORCE )
SET( CMAKE_SHARED_LINKER_FLAGS_TEST
    "-fprofile-arcs -ftest-coverage" CACHE STRING
    "Flags used by the shared libraries linker during test builds."
    FORCE )
MARK_AS_ADVANCED(
    CMAKE_CXX_FLAGS_TEST
    CMAKE_C_FLAGS_TEST
    CMAKE_EXE_LINKER_FLAGS_TEST
    CMAKE_SHARED_LINKER_FLAGS_TEST )
# Update the documentation string of CMAKE_BUILD_TYPE for GUIs
# It erases current build type. Workaround needed.
#SET( CMAKE_BUILD_TYPE "" CACHE STRING
#    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Test."
#    FORCE )
