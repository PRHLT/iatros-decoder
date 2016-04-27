# Copyright (C) 2007 by Vicente Alabau Gonzalvo <valabau at dsic.upv.es>,
# Pattern Recognition and Artificial Intelligence Group,
# Technological Institute of Computer Science,
# Polytechnic  University of Valencia, Valencia (Spain).

# Utils is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# Utils is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with LogProb; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

# - Find srilm
# Find the native SRILM includes and library
#
#  SRILM_INCLUDE_DIR - where to find srilm.h, etc.
#  SRILM_LIBRARIES   - List of libraries when using srilm.
#  SRILM_FOUND       - True if srilm found.


IF (SRILM_INCLUDE_DIR)
  # Already in cache, be silent
  SET(SRILM_FIND_QUIETLY TRUE)
ELSE (SRILM_INCLUDE_DIR)
  IF(NOT SRILM_PATH)
    SET(SRILM_PATH $ENV{SRILM_PATH})
  ENDIF(NOT SRILM_PATH)

  IF(SRILM_PATH)
    SET(SRILM_POSSIBLE_PATHS ${SRILM_PATH})
    SET(SRILM_PATH ${SRILM_PATH} CACHE PATH "This variable points to the SRILM directory in your system")
  ELSE(SRILM_PATH)
    SET(SRILM_POSSIBLE_PATHS "")
  ENDIF(SRILM_PATH)

  LIST(APPEND SRILM_POSSIBLE_PATHS
        $ENV{HOME}/local/src/srilm
        $ENV{HOME}/local/srilm
        $ENV{HOME}/srilm
        /usr/local/srilm
        /usr/srilm
        $ENV{HOME}/local
        /usr/local
        /usr
  )
  # Create a list with possible include paths
  FOREACH(POSSIBLE_PATH ${SRILM_POSSIBLE_PATHS}) 
    LIST(APPEND SRILM_POSSIBLE_INC ${POSSIBLE_PATH}/include)
  ENDFOREACH(POSSIBLE_PATH) 
  # Create a list with possible lib paths
  FOREACH(POSSIBLE_PATH ${SRILM_POSSIBLE_PATHS}) 
    LIST(APPEND SRILM_POSSIBLE_LIB ${POSSIBLE_PATH}/lib)
    FILE(GLOB SRILM_SUBPATHS_AUX ${POSSIBLE_PATH}/lib/*)
    FOREACH(SUBPATH ${SRILM_SUBPATHS_AUX}) 
      LIST(APPEND SRILM_POSSIBLE_LIB ${SUBPATH})
    ENDFOREACH(SUBPATH ${SRILM_POSSIBLE_PATHS}) 
  ENDFOREACH(POSSIBLE_PATH) 
ENDIF (SRILM_INCLUDE_DIR)


FIND_PATH(SRILM_INCLUDE_DIR Vocab.h PATHS ${SRILM_POSSIBLE_INC})

IF(NOT SRILM_PATH)
  STRING(REGEX REPLACE "/include" "" SRILM_PATH "${SRILM_INCLUDE_DIR}")
  SET(SRILM_PATH ${SRILM_PATH} CACHE PATH "This variable points to the SRILM directory in your system")
ENDIF(NOT SRILM_PATH)

FIND_LIBRARY(SRILM_OOLM_LIBRARY oolm PATHS ${SRILM_POSSIBLE_LIB})
FIND_LIBRARY(SRILM_DSTRUCT_LIBRARY dstruct PATHS ${SRILM_POSSIBLE_LIB})
FIND_LIBRARY(SRILM_MISC_LIBRARY misc PATHS ${SRILM_POSSIBLE_LIB})

IF (SRILM_INCLUDE_DIR AND SRILM_OOLM_LIBRARY AND SRILM_DSTRUCT_LIBRARY AND SRILM_MISC_LIBRARY)
   SET(SRILM_FOUND TRUE)
   INCLUDE_DIRECTORIES(${SRILM_INCLUDE_DIR})
   SET(SRILM_LIBRARIES  ${SRILM_OOLM_LIBRARY} ${SRILM_DSTRUCT_LIBRARY} ${SRILM_MISC_LIBRARY})
ELSE (SRILM_INCLUDE_DIR AND SRILM_OOLM_LIBRARY AND SRILM_DSTRUCT_LIBRARY AND SRILM_MISC_LIBRARY)
   SET(SRILM_FOUND FALSE)
   SET(SRILM_LIBRARIES )
ENDIF (SRILM_INCLUDE_DIR AND SRILM_OOLM_LIBRARY AND SRILM_DSTRUCT_LIBRARY AND SRILM_MISC_LIBRARY)

IF (SRILM_FOUND)
   IF (NOT SRILM_FIND_QUIETLY)
      MESSAGE(STATUS "Found SRILM: ${SRILM_PATH}")
      SET(HAVE_SRILM ON CACHE PATH "SRILM enbled")
   ENDIF (NOT SRILM_FIND_QUIETLY)
ELSE (SRILM_FOUND)
   IF (SRILM_FIND_REQUIRED)
      MESSAGE(STATUS "Looking for SRILM libraries at ${SRILM_POSSIBLE_PATHS}.")
      MESSAGE(FATAL_ERROR "Could NOT find srilm library")
   ENDIF (SRILM_FIND_REQUIRED)
ENDIF (SRILM_FOUND)

MARK_AS_ADVANCED(
  SRILM_OOLM_LIBRARY
  SRILM_DSTRUCT_LIBRARY
  SRILM_MISC_LIBRARY
  SRILM_INCLUDE_DIR
  )

