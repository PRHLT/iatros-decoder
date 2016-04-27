# Based on FindSubversion
# - Extract information from a git working tree 
# The module defines the following variables:
#  Git_GIT_EXECUTABLE - path to git command line client
#  Git_VERSION_GIT - version of git command line client
#  Git_FOUND - true if the command line client was found
# If the command line client executable is found the macro
#  Git_WT_INFO(<dir> <var-prefix>)
# is defined to extract information of a git working copy at
# a given location. The macro defines the following variables:
#  <var-prefix>_WT_URL - url of the repository (at <dir>)
#  <var-prefix>_WT_ROOT - root url of the repository
#  <var-prefix>_WT_REVISION - current revision
#  <var-prefix>_WT_LAST_CHANGED_AUTHOR - author of last commit
#  <var-prefix>_WT_LAST_CHANGED_DATE - date of last commit
#  <var-prefix>_WT_LAST_CHANGED_REV - revision of last commit
#  <var-prefix>_WT_LAST_CHANGED_LOG - last log of base revision
#  <var-prefix>_WT_INFO - output of command `git info <dir>'
# Example usage:
#  FIND_PACKAGE(Git)
#  IF(Git_FOUND)
#    Git_WT_INFO(${PROJECT_SOURCE_DIR} Project)
#    MESSAGE(STATUS "Current revision is ${Project_WT_REVISION}")
#  ENDIF(Git_FOUND)

# Copyright (c) 2006, Tristan Carel
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the University of California, Berkeley nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

SET(Git_FOUND FALSE)
SET(Git_GIT_FOUND FALSE)

FIND_PROGRAM(Git_GIT_EXECUTABLE git
  DOC "git command line client")
MARK_AS_ADVANCED(Git_GIT_EXECUTABLE)

IF(Git_GIT_EXECUTABLE)
  SET(Git_GIT_FOUND TRUE)
  SET(Git_FOUND TRUE)

  MACRO(Git_WT_INFO dir prefix)
    EXECUTE_PROCESS(COMMAND ${Git_GIT_EXECUTABLE} --version
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      OUTPUT_VARIABLE Git_VERSION_GIT
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Find git show info
    EXECUTE_PROCESS(COMMAND ${Git_GIT_EXECUTABLE} log -n 1 --pretty --parents  HEAD 
      WORKING_DIRECTORY ${dir}
      OUTPUT_VARIABLE ${prefix}_WT_INFO
      ERROR_VARIABLE Git_git_log_error
      RESULT_VARIABLE Git_git_log_result
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    # It is not a working tree
    IF(NOT ${Git_git_log_result} EQUAL 0)
      #MESSAGE(SEND_ERROR "Command \"${Git_GIT_EXECUTABLE} -n 1 --pretty --parents  HEAD\" failed with output:\n${Git_git_log_error}")
      SET(${prefix}_WT_COMMIT "unknown")
      SET(${prefix}_WT_PARENT "unknown")
      SET(${prefix}_WT_AUTHOR "unknown")
      SET(${prefix}_WT_DATE   "unknown")
      SET(${prefix}_WT_BRANCH "unknown")
      SET(${prefix}_WT_STATE  "unknown")

    # It is a working tree
    ELSE(NOT ${Git_git_log_result} EQUAL 0)

      STRING(REGEX REPLACE "^commit +([^ \n]+)[ \n]+.*"
        "\\1" ${prefix}_WT_COMMIT "${${prefix}_WT_INFO}")
      STRING(REGEX REPLACE "^commit +([^ \n]+) +([^ \n]+)[ \n].*"
        "\\2" ${prefix}_WT_PARENT "${${prefix}_WT_INFO}")
      STRING(REGEX REPLACE "^(.*\n)?Author: ([^\n]+).*"
        "\\2" ${prefix}_WT_AUTHOR "${${prefix}_WT_INFO}")
      STRING(REGEX REPLACE "^(.*\n)?Date: +([^\n]+).*"
        "\\2" ${prefix}_WT_DATE "${${prefix}_WT_INFO}")

      # Find git commit number 
      EXECUTE_PROCESS(COMMAND ${Git_GIT_EXECUTABLE} log --pretty=oneline COMMAND awk "END {print NR-1}" 
        WORKING_DIRECTORY ${dir}
        OUTPUT_VARIABLE ${prefix}_COMMIT_NUMBER
        ERROR_VARIABLE Git_git_log_error
        RESULT_VARIABLE Git_git_log_result
        OUTPUT_STRIP_TRAILING_WHITESPACE)

      # Find git branch info
      EXECUTE_PROCESS(COMMAND ${Git_GIT_EXECUTABLE} branch 
        WORKING_DIRECTORY ${dir}
        OUTPUT_VARIABLE ${prefix}_BRANCH_INFO
        ERROR_VARIABLE Git_git_log_error
        RESULT_VARIABLE Git_git_log_result
        OUTPUT_STRIP_TRAILING_WHITESPACE)

      IF(NOT ${Git_git_log_result} EQUAL 0)
        #MESSAGE(SEND_ERROR "Command \"${Git_GIT_EXECUTABLE} branch\" failed with output:\n${Git_git_log_error}")
      ELSE(NOT ${Git_git_log_result} EQUAL 0)

        STRING(REGEX REPLACE "(.*\n)?\\* +([^\n]+).*"
          "\\2" ${prefix}_WT_BRANCH "${${prefix}_BRANCH_INFO}")

      ENDIF(NOT ${Git_git_log_result} EQUAL 0)

      # Find git state 
      EXECUTE_PROCESS(COMMAND ${Git_GIT_EXECUTABLE} diff COMMAND wc -l 
        WORKING_DIRECTORY ${dir}
        OUTPUT_VARIABLE ${prefix}_DIFF_INFO
        ERROR_VARIABLE Git_git_log_error
        RESULT_VARIABLE Git_git_log_result
        OUTPUT_STRIP_TRAILING_WHITESPACE)

      
      IF(${${prefix}_DIFF_INFO} EQUAL "0")
        SET(${prefix}_WT_STATE "clean")
      ELSE(${${prefix}_DIFF_INFO} EQUAL "0")
        SET(${prefix}_WT_STATE "dirty")
      ENDIF(${${prefix}_DIFF_INFO} EQUAL "0")

    ENDIF(NOT ${Git_git_log_result} EQUAL 0)

    SET(${prefix}_WT_URL "${dir}")

    #Check svn info
    find_package(Subversion)
    if(Subversion_FOUND)
        GIT_SVN_WC_INFO("${dir}" "${prefix}")
    endif(Subversion_FOUND)    
    
  ENDMACRO(Git_WT_INFO)


  MACRO(GIT_SVN_WC_INFO dir prefix)
    # the subversion commands should be executed with the C locale, otherwise
    # the message (which are parsed) may be translated, Alex
    SET(_SAVED_LC_ALL "$ENV{LC_ALL}")
    SET(ENV{LC_ALL} C)

    EXECUTE_PROCESS(COMMAND ${Git_GIT_EXECUTABLE} svn --version
      WORKING_DIRECTORY ${dir}
      OUTPUT_VARIABLE GIT_SVN_VERSION_SVN
      OUTPUT_STRIP_TRAILING_WHITESPACE)
      
    EXECUTE_PROCESS(COMMAND ${Git_GIT_EXECUTABLE} svn info 
      WORKING_DIRECTORY ${dir}
      OUTPUT_VARIABLE ${prefix}_WC_INFO
      ERROR_VARIABLE GIT_SVN_info_error
      RESULT_VARIABLE GIT_SVN_info_result
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    IF(NOT ${GIT_SVN_info_result} EQUAL 0)
      MESSAGE(SEND_ERROR "Command \"${Git_GIT_EXECUTABLE} svn info\" failed with output:\n${GIT_SVN_info_error}")
    ELSE(NOT ${GIT_SVN_info_result} EQUAL 0)
      SET(GIT_SVN_FOUND TRUE)

      STRING(REGEX REPLACE "^(.*\n)?svn, version ([.0-9]+).*"
        "\\2" GIT_SVN_VERSION_SVN "${GIT_SVN_VERSION_SVN}")
      STRING(REGEX REPLACE "^(.*\n)?URL: ([^\n]+).*"
        "\\2" ${prefix}_WC_URL "${${prefix}_WC_INFO}")
      STRING(REGEX REPLACE "^(.*\n)?Revision: ([^\n]+).*"
        "\\2" ${prefix}_WC_REVISION "${${prefix}_WC_INFO}")
      STRING(REGEX REPLACE "^(.*\n)?Last Changed Author: ([^\n]+).*"
        "\\2" ${prefix}_WC_LAST_CHANGED_AUTHOR "${${prefix}_WC_INFO}")
      STRING(REGEX REPLACE "^(.*\n)?Last Changed Rev: ([^\n]+).*"
        "\\2" ${prefix}_WC_LAST_CHANGED_REV "${${prefix}_WC_INFO}")
      STRING(REGEX REPLACE "^(.*\n)?Last Changed Date: ([^\n]+).*"
        "\\2" ${prefix}_WC_LAST_CHANGED_DATE "${${prefix}_WC_INFO}")

    ENDIF(NOT ${GIT_SVN_info_result} EQUAL 0)

    # restore the previous LC_ALL
    SET(ENV{LC_ALL} ${_SAVED_LC_ALL})

  ENDMACRO(GIT_SVN_WC_INFO dir prefix)
  
ENDIF(Git_GIT_EXECUTABLE)

IF(NOT Git_FOUND)
  IF(NOT Git_FIND_QUIETLY)
    MESSAGE(STATUS "Git was not found.")
  ELSE(NOT Git_FIND_QUIETLY)
    IF(Git_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Git was not found.")
    ENDIF(Git_FIND_REQUIRED)
  ENDIF(NOT Git_FIND_QUIETLY)
ENDIF(NOT Git_FOUND)

# FindGit.cmake ends here.
