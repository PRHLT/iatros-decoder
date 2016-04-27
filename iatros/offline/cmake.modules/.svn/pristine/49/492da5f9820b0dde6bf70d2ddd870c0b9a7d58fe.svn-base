# - Find PHP
# This module finds if PHP is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
#  PHP_INCLUDE_PATH       = path to where php.h can be found
#  PHP_EXECUTABLE         = full path to the php binary
#  PHP_FLAGS              = compilation flags for php
#

SET(PHP_POSSIBLE_INCLUDE_PATHS
  /usr/include/php4
  /usr/local/include/php4
  /usr/include/php5
  /usr/local/include/php5
  /usr/include/php
  /usr/local/include/php
  /usr/local/apache/php
  )

SET(PHP_POSSIBLE_LIB_PATHS
  /usr/lib
  )

FIND_PATH(PHP_FOUND_INCLUDE_PATH main/php.h
  ${PHP_POSSIBLE_INCLUDE_PATHS})

IF(PHP_FOUND_INCLUDE_PATH)
  SET(php_paths "${PHP_POSSIBLE_INCLUDE_PATHS}")
  FOREACH(php_path Zend main TSRM)
    SET(php_paths ${php_paths} "${PHP_FOUND_INCLUDE_PATH}/${php_path}")
  ENDFOREACH(php_path Zend main TSRM)
  SET(PHP_INCLUDE_PATH "${php_paths}" CACHE INTERNAL "PHP include paths")
ENDIF(PHP_FOUND_INCLUDE_PATH)

FIND_PROGRAM(PHP_EXECUTABLE NAMES php php4 php5)

EXECUTE_PROCESS(COMMAND php-config --includes OUTPUT_VARIABLE php_config)
STRING(REGEX REPLACE "\n" " " PHP_FLAGS "${php_config} -fpic")
SET(PHP_FLAGS "${PHP_FLAGS}" CACHE INTERNAL "PHP compilation flags")
message(STATUS "php config = ${PHP_FLAGS}")

MARK_AS_ADVANCED(
  PHP_EXECUTABLE
  PHP_FOUND_INCLUDE_PATH
  )

IF(APPLE)
# this is a hack for now
  SET(CMAKE_SHARED_MODULE_CREATE_C_FLAGS 
   "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -Wl,-flat_namespace")
  FOREACH(symbol
    __efree
    __emalloc
    __estrdup
    __object_init_ex
    __zend_get_parameters_array_ex
    __zend_list_find
    __zval_copy_ctor
    _add_property_zval_ex
    _alloc_globals
    _compiler_globals
    _convert_to_double
    _convert_to_long
    _zend_error
    _zend_hash_find
    _zend_register_internal_class_ex
    _zend_register_list_destructors_ex
    _zend_register_resource
    _zend_rsrc_list_get_rsrc_type
    _zend_wrong_param_count
    _zval_used_for_init
    )
    SET(CMAKE_SHARED_MODULE_CREATE_C_FLAGS 
      "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS},-U,${symbol}")
  ENDFOREACH(symbol)
ENDIF(APPLE)

