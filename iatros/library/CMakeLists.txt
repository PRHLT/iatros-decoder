# set minimum cmake version required and cmake policy
cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

enable_language(C)

# Name the project
project(iatros)
set(PROJECT_FULLNAME "iatros")

# Add customized cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake.modules/")


# Use doxygen to generate documentation
include(Documentation)
if(BUILD_DOCUMENTATION AND DOXYGEN_FOUND)
  configure_file(${CMAKE_SOURCE_DIR}/doxyfile.cmake ${CMAKE_SOURCE_DIR}/doc/doxyfile)
  add_custom_target(doc ALL ${DOXYGEN} ${PROJECT_SOURCE_DIR}/doc/doxyfile)
  set_source_files_properties(doc/html PROPERTIES GENERATED true OBJECT_DEPENDS doc)
  install(DIRECTORY doc/html  DESTINATION share/doc/iatros/library)
endif(BUILD_DOCUMENTATION AND DOXYGEN_FOUND)


# Include package generation
include(Package)

include(FindCustomPackage)
find_custom_package(PRHLT)

include(CommonCompileOptions)

#http://www.vtk.org/Wiki/CMake_HowToDoPlatformChecks
set(CMAKE_REQUIRED_INCLUDES $ENV{HOME}/local/include)

#add extra include directories for generated source files
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

# add support for hyades and aldebaran compilation
# libraries must be instaled in $HOME/local
include_directories($ENV{HOME}/local/include)
link_directories($ENV{HOME}/local/lib)

# Find packages
find_package(FLEX)
find_package(BISON)

# add package creation
include(CPack)

include(LibTools)

# Add build info variables and version
include(BuildInfo)

# Process the subdirectories
add_subdirectory(src)
