#
#  Utilities for libraries 
#

file(REMOVE ${CMAKE_BINARY_DIR}/exports.cmake)

# Create static and dynamic version of the library. 
# Static version target name is made by appending _nonshared to the name of the library
# Avoid recompilation of static and dynamic libraries if possible
# This should only be applied if the compilation options are the same
# http://www.vtk.org/Wiki/CMake_FAQ#Does_that_mean_I_have_to_build_all_my_library_objects_twice.2C_once_for_shared_and_once_for_static.3F.21__I_don.27t_like_that.21
macro(add_static_and_dynamic_library _LIB_NAME)
  #if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_COMPILER_IS_GNUCC)
  add_library(${_LIB_NAME}           SHARED ${ARGN})
  add_library(${_LIB_NAME}_nonshared STATIC ${ARGN})

  #if(CMAKE_COMPILER_IS_GNUCC)
  #  target_link_libraries(${_LIB_NAME}_nonshared -fPIC)
  #  target_link_libraries(${_LIB_NAME} -Wl,-whole-archive ${_LIB_NAME}_nonshared -Wl,-no-whole-archive)
  #else(CMAKE_COMPILER_IS_GNUCC)
  #endif(CMAKE_COMPILER_IS_GNUCC)
endmacro(add_static_and_dynamic_library _LIB_NAME)

macro(static_and_dynamic_link_libraries _LIB_NAME)
  target_link_libraries(${_LIB_NAME}           ${ARGN})
  target_link_libraries(${_LIB_NAME}_nonshared ${ARGN})
endmacro(static_and_dynamic_link_libraries _LIB_NAME)

macro(export_static_and_dynamic_libraries _EXPORT_NAME)
  foreach(_LIB ${ARGN})
    install(TARGETS ${_LIB}           EXPORT ${_EXPORT_NAME} DESTINATION lib)
    install(TARGETS ${_LIB}_nonshared EXPORT ${_EXPORT_NAME} DESTINATION lib)
    export(TARGETS ${_LIB} ${_LIB}_nonshared APPEND FILE ${CMAKE_BINARY_DIR}/exports.cmake)
    #if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    #  set_target_properties(${_LIB}_nonshared PROPERTIES COMPILE_FLAGS "-fPIC")
    #endif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  endforeach(_LIB)
endmacro(export_static_and_dynamic_libraries _EXPORT_NAME)

macro(install_headers _DIR_NAME)
  include_directories(${CMAKE_BINARY_DIR})
  foreach(_HEADER ${ARGN})
    get_filename_component(_CMAKE_HEADER_NAME "${_HEADER}" NAME)
    string(REGEX REPLACE ".cmake$" "" _HEADER_NAME ${_CMAKE_HEADER_NAME})
    configure_file(${_HEADER} ${CMAKE_BINARY_DIR}/${_DIR_NAME}/${_HEADER_NAME})
  endforeach(_HEADER)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/${_DIR_NAME} DESTINATION include)
endmacro(install_headers _DIR_NAME)

