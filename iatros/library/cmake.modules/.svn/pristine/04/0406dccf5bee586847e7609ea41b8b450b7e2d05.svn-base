# Options to enable PHP bindings
if(DEFAULT_NO_BINDINGS)
  option(ENABLE_PHP_BINDINGS "Enable PHP bindings" OFF)
else(DEFAULT_NO_BINDINGS)
  option(ENABLE_PHP_BINDINGS "Enable PHP bindings" ON)
endif(DEFAULT_NO_BINDINGS)

#if(ENABLE_PHP_BINDINGS AND NOT BUILD_SHARED_LIBS)
#  message(STATUS "WARNING: "
#    "PHP requires shared libraries. Disabling php bindings")
#  set(ENABLE_PHP_BINDINGS OFF CACHE BOOL "Enable PHP bindings" FORCE)
#endif(ENABLE_PHP_BINDINGS AND NOT BUILD_SHARED_LIBS)

if(ENABLE_PHP_BINDINGS AND NOT SWIG_FOUND)
  message(STATUS "WARNING: "
    "swig not found. Disabling php bindings")
  set(ENABLE_PHP_BINDINGS OFF CACHE BOOL "Enable PHP bindings" FORCE)
endif(ENABLE_PHP_BINDINGS AND NOT SWIG_FOUND)

if(ENABLE_PHP_BINDINGS)
  # Check for PHP libraries which defines
  #
  #  PHP_INCLUDE_PATH       = path to where php.h can be found
  #  PHP_EXECUTABLE         = full path to the php binary

  find_package(PHP)
  if(NOT PHP_INCLUDE_PATH)
    message(STATUS "WARNING: "
    "php headers not found. Disabling php bindings")
    set(ENABLE_PHP_BINDINGS OFF CACHE BOOL "Enable PHP bindings" FORCE)
  endif(NOT PHP_INCLUDE_PATH)
endif(ENABLE_PHP_BINDINGS)

if(ENABLE_PHP_BINDINGS)
  message(STATUS "Found PHP: ${PHP_INCLUDE_PATH}")
  include_directories(${PHP_INCLUDE_PATH})
else(ENABLE_PHP_BINDINGS)
    message(STATUS "PHP not found")
endif(ENABLE_PHP_BINDINGS)

