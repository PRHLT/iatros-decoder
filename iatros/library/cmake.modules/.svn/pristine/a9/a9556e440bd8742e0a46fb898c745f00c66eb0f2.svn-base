# cmake/modules/java.cmake
#
# Java binding configuration
#
# Copyright (C) 2006  Andrew Ross
#
# This file is part of PLplot.
#
# PLplot is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library General Public License as published
# by the Free Software Foundation; version 2 of the License.
#
# PLplot is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with the file PLplot; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

# Module for determining Java bindings configuration options

# Options to enable Java bindings
# option(ENABLE_JAVA_BINDINGS "Enable Java bindings" ON)
if(DEFAULT_NO_BINDINGS)
  option(ENABLE_JAVA_BINDINGS "Enable Java bindings" OFF)
else(DEFAULT_NO_BINDINGS)
  option(ENABLE_JAVA_BINDINGS "Enable Java bindings" ON)
endif(DEFAULT_NO_BINDINGS)

#if(ENABLE_JAVA_BINDINGS AND NOT BUILD_SHARED_LIBS)
#  message(STATUS "WARNING: "
#    "Java requires shared libraries. Disabling java bindings")
#  set(ENABLE_JAVA_BINDINGS OFF CACHE BOOL "Enable Java bindings" FORCE)
#endif(ENABLE_JAVA_BINDINGS AND NOT BUILD_SHARED_LIBS)

if(ENABLE_JAVA_BINDINGS AND NOT SWIG_FOUND)
  message(STATUS "WARNING: "
    "swig not found. Disabling java bindings")
  set(ENABLE_JAVA_BINDINGS OFF CACHE BOOL "Enable Java bindings" FORCE)
endif(ENABLE_JAVA_BINDINGS AND NOT SWIG_FOUND)

if(ENABLE_JAVA_BINDINGS)
  # Check for java compiler
  include(CMakeDetermineJavaCompiler)
  if(NOT CMAKE_Java_COMPILER)
    message(STATUS "WARNING: "
      "java compiler not found. Disabling java bindings")
    set(ENABLE_JAVA_BINDINGS OFF CACHE BOOL "Enable Java bindings" FORCE)
  endif(NOT CMAKE_Java_COMPILER)
endif(ENABLE_JAVA_BINDINGS)

if(ENABLE_JAVA_BINDINGS)
  # Check for java environment
  enable_language(Java)
  find_package(Java)
  find_package(JNI)
  # If CMake doesn't find jni.h you need set CMAKE_INCLUDE_PATH
  if(NOT JAVA_INCLUDE_PATH)
    message(STATUS
      "WARNING: jni.h header not found. Disabling Java bindings.\n" 
    "   Please install that header and/or set the environment variable\n"
    "   CMAKE_INCLUDE_PATH appropriately."
    )
    set(ENABLE_JAVA_BINDINGS OFF CACHE BOOL "Enable Java bindings" FORCE)
  endif(NOT JAVA_INCLUDE_PATH)
endif(ENABLE_JAVA_BINDINGS)

if(ENABLE_JAVA_BINDINGS)
  message(STATUS "Found Java: ${JAVA_INCLUDE_PATH}")
  include_directories(${JAVA_INCLUDE_PATH})
  file(GLOB JNI_SUBPATHS_AUX ${JAVA_INCLUDE_PATH}/*)
  foreach(SUBPATH ${JNI_SUBPATHS_AUX}) 
    if(IS_DIRECTORY ${SUBPATH})
      include_directories(${SUBPATH})
    endif(IS_DIRECTORY ${SUBPATH})
  endforeach(SUBPATH ${JNI_SUBPATHS_AUX})
  # Set up installation locations for java specific files.
  # Java .jar files.
  set(JAR_DIR ${CMAKE_INSTALL_DATADIR}/java)
  get_filename_component(JAVADATA_HARDDIR ${JAR_DIR} ABSOLUTE)
  # JNI .so files.
  set(JAVAWRAPPER_DIR ${LIB_DIR}/jni)
  get_filename_component(JAVAWRAPPER_HARDDIR ${JAVAWRAPPER_DIR} ABSOLUTE)
  # Library name suffix is system dependent.  Tests have shown
  # that ${CMAKE_SHARED_LIBRARY_SUFFIX} gives the correct suffix on both
  # Linux and windows, and from CMAKEPREFIX/share/cmake-2.4/Modules/Platform
  # it appears this CMake variable is defined on Mac OS X and other platforms.
  #set(PLPLOTJAVAC_WRAP_DLL plplotjavac_wrap${CMAKE_SHARED_LIBRARY_SUFFIX})

else(ENABLE_JAVA_BINDINGS)
    message(STATUS "Java not found")
endif(ENABLE_JAVA_BINDINGS)

macro(compile_java_source SOURCE_FILE BUILD_DIR)
  if(ENABLE_JAVA_BINDINGS)
    get_filename_component(SOURCE_NAME ${SOURCE_FILE} NAME_WE)
    set(TARGET_FILE ${BUILD_DIR}/${SOURCE_NAME}.class)
    set(TARGET_NAME ${SOURCE_NAME}.class)
    add_custom_command(
      OUTPUT ${TARGET_FILE} 
      COMMAND ${CMAKE_Java_COMPILER} 
              -classpath ${BUILD_DIR} 
              -d ${BUILD_DIR}
              ${SOURCE_FILE} 
      DEPENDS ${SOURCE_FILE} ${${TARGET_FILE}_DEPENDS}
    )
    add_custom_target(${TARGET_NAME} ALL DEPENDS ${SOURCE_FILE})
  endif(ENABLE_JAVA_BINDINGS)
endmacro(compile_java_source SOURCE_FILE BUILD_DIR)

macro(create_manifest LIBRARY MAIN_CLASS)
  #file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}.MF" "Manifest-Version: 1.0\n")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}.MF" "Class-Path: ${ARGN}\n")
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}.MF" "Main-Class: ${MAIN_CLASS}\n")
  add_custom_target(${LIBRARY}_jar_main_class ALL COMMAND ${JAVA_ARCHIVE} -umf ${LIBRARY}.MF ${LIBRARY}.jar DEPENDS ${LIBRARY})
endmacro(create_manifest LIBRARY MAIN_CLASS)
