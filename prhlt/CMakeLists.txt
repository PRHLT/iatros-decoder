cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Name the project
project(prhlt)
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_FULLNAME "PRHLT")

# Add customized cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake.modules/")

# Version
set(PACKAGE_VERSION_MAJOR 0)
set(PACKAGE_VERSION_MINOR 0)
set(PACKAGE_VERSION_RELEASE 00)
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_RELEASE}" )
set(PACKAGE_VERSION_STRING "${PACKAGE_VERSION} (whatever)")
set(PACKAGE_VENDOR "equipo_iatros@iti.upv.es")

# Use doxygen to generate documentation
include(Documentation)
if(BUILD_DOCUMENTATION AND DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.cmake ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxyfile)
  add_custom_target(doc ${DOXYGEN} ${PROJECT_SOURCE_DIR}/doc/doxyfile)
endif(BUILD_DOCUMENTATION AND DOXYGEN_FOUND)

# Check library dependences
#find_package(ZLIB)
#find_package(BOOST)
#find_package(SRILM REQUIRED)

# Add build info variables
include(BuildInfo)

#include SWIG for multilanguage bindings
if(BUILD_BINDINGS)
  include(SWIG)
endif(BUILD_BINDINGS)

include(LibTools)

# Include package generation 
include(Package)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


include(CommonCompileOptions)

#http://www.vtk.org/Wiki/CMake_HowToDoPlatformChecks
set(CMAKE_REQUIRED_INCLUDES $ENV{HOME}/local/include)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)

check_include_files(errno.h  HAVE_ERRNO_H)
if(NOT HAVE_ERRNO_H)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/errno.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/errno.h)
endif(NOT HAVE_ERRNO_H)
check_include_files(values.h HAVE_VALUES_H)

check_function_exists(popen HAVE_POPEN)
if(NOT HAVE_POPEN OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(HAVE_POPEN OFF)
  message(STATUS "popen function couldn't be found. Disabling gz support")
endif(NOT HAVE_POPEN OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

check_function_exists(isatty "unistd.h" HAVE_ISATTY)
check_symbol_exists(clock "time.h" HAVE_CLOCK)
check_symbol_exists(strdup "string.h" HAVE_STRDUP)
check_symbol_exists(strtok_r "string.h" HAVE_STRTOK_R)
check_symbol_exists(strcasecmp "string.h" HAVE_STRCASECMP)
check_symbol_exists(fileno "strdio.h" HAVE_FILENO)
check_include_files(valgrind/callgrind.h HAVE_CALLGRIND_H)

check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)
find_program(HAVE_ADDR2LINE addr2line)

# Build type options
#test build
#include(Test)
#enable_testing()

# Find packages
find_package(FLEX)
find_package(BISON)
#find_package(cat_api_config)

# Process the subdirectories
add_subdirectory(src)
