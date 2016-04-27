#
#  Utilities to find custom packages
#

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

macro(find_custom_package _PKG_NAME)

  if(EXISTS ${${_PKG_NAME}_BUILD_TREE}/exports.cmake)
    message(STATUS "Found ${${_PKG_NAME}_BUILD_TREE}/exports.cmake")
    include(${${_PKG_NAME}_BUILD_TREE}/exports.cmake)
    include_directories("${${_PKG_NAME}_BUILD_TREE}")
    set(HAVE_${_PKG_NAME} ON)
  else(EXISTS ${${_PKG_NAME}_BUILD_TREE}/exports.cmake)
    find_package(${_PKG_NAME} REQUIRED NO_MODULE PATHS ${${_PKG_NAME}_INSTALL_PATH}/lib ${CMAKE_INSTALL_PREFIX}/lib /usr/lib /usr/local/lib)
    if(${_PKG_NAME}_FOUND)
      set(HAVE_${_PKG_NAME} ON)
      message(STATUS "Custom package ${_PKG_NAME} found at '${${_PKG_NAME}_DIR}'")
      if(EXISTS "${${_PKG_NAME}_DIR}/../include")
        include_directories("${${_PKG_NAME}_DIR}/../include")
        mark_as_advanced("${${_PKG_NAME}_DIR/../include}")
      endif(EXISTS "${${_PKG_NAME}_DIR}/../include")
    endif(${_PKG_NAME}_FOUND)
  endif(EXISTS ${${_PKG_NAME}_BUILD_TREE}/exports.cmake)

endmacro(find_custom_package PACKAGE_NAME)

macro(find_optional_custom_package _PKG_NAME)

  if(EXISTS ${${_PKG_NAME}_BUILD_TREE}/exports.cmake)
    message(STATUS "Found ${${_PKG_NAME}_BUILD_TREE}/exports.cmake")
    include(${${_PKG_NAME}_BUILD_TREE}/exports.cmake)
    include_directories("${${_PKG_NAME}_BUILD_TREE}")
    set(HAVE_${_PKG_NAME} ON)
  else(EXISTS ${${_PKG_NAME}_BUILD_TREE}/exports.cmake)
    find_package(${_PKG_NAME} QUIET NO_MODULE PATHS ${${_PKG_NAME}_INSTALL_PATH}/lib ${CMAKE_INSTALL_PREFIX}/lib /usr/lib /usr/local/lib)
    if(${_PKG_NAME}_FOUND)
      set(HAVE_${_PKG_NAME} ON)
      message(STATUS "Custom package ${_PKG_NAME} found at '${${_PKG_NAME}_DIR}'")
      if(EXISTS "${${_PKG_NAME}_DIR}/../include")
        include_directories("${${_PKG_NAME}_DIR}/../include")
        mark_as_advanced("${${_PKG_NAME}_DIR/../include}")
      endif(EXISTS "${${_PKG_NAME}_DIR}/../include")
    endif(${_PKG_NAME}_FOUND)
  endif(EXISTS ${${_PKG_NAME}_BUILD_TREE}/exports.cmake)

endmacro(find_optional_custom_package PACKAGE_NAME)
