# Avoid inline warnings. See DEVEL.NOTES
include(CheckCCompilerFlag)

check_c_compiler_flag("-fgnu89-inline" HAVE_GNU89_INLINE)
if(HAVE_GNU89_INLINE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fgnu89-inline")
endif(HAVE_GNU89_INLINE)

check_c_compiler_flag("-Werror-implicit-function-declaration" HAVE_ERROR_IMPLICIT_FUNCTION)
if(HAVE_ERROR_IMPLICIT_FUNCTION)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror-implicit-function-declaration")
endif(HAVE_ERROR_IMPLICIT_FUNCTION)

if(FORCE32)
  set(OLD_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  check_c_compiler_flag("-m32" HAVE_FORCE32)
  if(NOT HAVE_FORCE32)
    set(CMAKE_C_FLAGS "${OLD_CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")
    message(STATUS "Compiler does not support 32bit cross-compiling")
  endif(NOT HAVE_FORCE32)
endif(FORCE32)

if(GPROF)
  check_c_compiler_flag("-pg" HAVE_GPROF)
  if(HAVE_GPROF)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
  else(HAVE_GPROF)
    message(STATUS "Compiler does not support gprof")
  endif(HAVE_GPROF)
endif(GPROF)


#for capturing backtarce
#check_c_compiler_flag("-rdynamic" HAVE_RDYNAMIC)
#if(HAVE_RDYNAMIC)
#  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -rdynamic")
#endif(HAVE_RDYNAMIC)


# C general options
check_c_compiler_flag("-std=c99" HAVE_STD_C99)
if(HAVE_STD_C99)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
else(HAVE_STD_C99)
  check_c_compiler_flag("-std=c99" HAVE_STD_C99)
  if(HAVE_STD_GNU99)
    # migw32 does not support C99. Using GNU99
    message(STATUS "Compiler does not support the C99 standard, enabling GNU99 support")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
  else(HAVE_STD_GNU99)
    message(STATUS "Compiler does not support either the C99 standard or the GNU99")
  endif(HAVE_STD_GNU99)
endif(HAVE_STD_C99)

check_c_compiler_flag("-Wc++-compat" HAVE_CXX_COMPAT)
if(HAVE_CXX_COMPAT)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wc++-compat")
endif(HAVE_CXX_COMPAT)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_POSIX_C_SOURCE=200809L")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Winline -Wextra -W -Wall -Wshadow")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused-variable -Wunused-parameter -Wunused-function -Wunused")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunreachable-code")
#set(CMAKE_C_FLAGS ${CFLAGS})
#debug build
# -Wabi should be enabled to ensure compiler independence
#set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG -Wall -fno-implicit-templates -Wabi")
set(CMAKE_C_FLAGS_DEBUG " -O0 -g3 -DDEBUG ")
#release with debug information build
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g3 -DDEBUG -O3")
#release build
set(CMAKE_C_FLAGS_RELEASE " -O3")
#-ffast-math is not compatible with -O


# CXX general options
# add no-deprecared to avoid compiling problems with g++ 4.3
# this should be fixed properly
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winline -Wextra -W -Wall -Wshadow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-variable -Wunused-parameter -Wunused-function -Wunused")
#debug build
# -Wabi should be enabled to ensure compiler independence
#set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG -Wall -fno-implicit-templates -Wabi")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG -Wall")
#optimize build
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -ffast-math -g3 -DDEBUG -Wall")
#release build
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ffast-math")


if(WIN32_TARGET)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U__STRICT_ANSI__")
endif(WIN32_TARGET)

set(CMAKE_REQUIRED_FLAGS "-Werror-implicit-function-declaration")

#add extra include directories for generated source files
include_directories(BEFORE ${CMAKE_BINARY_DIR})

if(CMAKE_INSTALL_PREFIX)
  include_directories(${CMAKE_INSTALL_PREFIX}/include)
  list(APPEND CMAKE_REQUIRED_INCLUDES ${CMAKE_INSTALL_PREFIX}/include)
  link_directories(${CMAKE_INSTALL_PREFIX}/lib)
endif(CMAKE_INSTALL_PREFIX)

foreach(prefix ${CMAKE_EXTRA_PREFIXES})
  message(STATUS "Adding extra prefix: ${prefix}")
  include_directories(${prefix}/include)
  list(APPEND CMAKE_REQUIRED_INCLUDES ${prefix}/include)
  link_directories(${prefix}/lib)
endforeach(prefix ${CMAKE_EXTRA_PREFIXES})

get_directory_property(_INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES)
foreach(inc ${_INCLUDE_DIRECTORIES})
  list(APPEND CMAKE_REQUIRED_INCLUDES ${inc})
endforeach(inc ${_INCLUDE_DIRECTORIES})

#get_directory_property(_LINK_DIRECTORIES LINK_DIRECTORIES)
#foreach(link ${_LINK_DIRECTORIES})
#endforeach(link ${_LINK_DIRECTORIES})

# add support for hyades and aldebaran compilation
# libraries must be instaled in $HOME/local
if(EXISTS $ENV{HOME}/local/include)
  include_directories($ENV{HOME}/local/include)
endif(EXISTS $ENV{HOME}/local/include)
if(EXISTS $ENV{HOME}/local/lib)
  link_directories($ENV{HOME}/local/lib)
endif(EXISTS $ENV{HOME}/local/lib)
