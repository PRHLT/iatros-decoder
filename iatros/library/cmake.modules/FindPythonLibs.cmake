# - Find python libraries
# This module finds if Python is installed and determines where the
# include files and libraries are. It also determines what the name of
# the library is. This code sets the following variables:
#
#  PYTHONLIBS_FOUND     = have the Python libs been found
#  PYTHON_LIBRARIES     = path to the python library
#  PYTHON_INCLUDE_PATH  = path to where Python.h is found
#  PYTHON_DEBUG_LIBRARIES = path to the debug library
#

find_package(PythonInterp)

if(PYTHONINTERP_FOUND)
    include("${CMAKE_ROOT}/Modules/FindPythonLibs.cmake")
    
    if(PYTHONLIBS_FOUND)
      set(HAVE_PYTHON TRUE)
      set(PYTHON_LIB ${PYTHON_LIBRARIES})
      include_directories(${PYTHON_INCLUDE_PATH})
      
      find_program(PYTHON_CONFIG_EXECUTABLE
        NAMES python3.0-config python2.6-config python2.5-config python2.4-config python2.3-config python2.2-config python2.1-config python2.0-config python1.6-config python1.5-config python-config
        PATHS
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\3.0\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.6\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.5\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.4\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.3\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.2\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.1\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.0\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.6\\InstallPath]
        [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.5\\InstallPath]
        )

      if(PYTHON_CONFIG_EXECUTABLE)
        execute_process(COMMAND ${PYTHON_CONFIG_EXECUTABLE} "--libs"
          OUTPUT_VARIABLE PYTHON_LDFLAGS_OUTPUT_VARIABLE
          ERROR_VARIABLE PYTHON_LDFLAGS_ERROR_VARIABLE
          RESULT_VARIABLE PYTHON_LDFLAGS_RETURN_VALUE
          )
      else(PYTHON_CONFIG_EXECUTABLE)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import distutils.sysconfig\nprint distutils.sysconfig.get_config_var('LINKFORSHARED')"
          OUTPUT_VARIABLE PYTHON_LDFLAGS_OUTPUT_VARIABLE
          ERROR_VARIABLE PYTHON_LDFLAGS_ERROR_VARIABLE
          RESULT_VARIABLE PYTHON_LDFLAGS_RETURN_VALUE
          )
      endif(PYTHON_CONFIG_EXECUTABLE)
      if(NOT PYTHON_LDFLAGS_RETURN_VALUE)
        string(REGEX REPLACE "\\\n" "" PYTHON_EXTRA_LINK_FLAGS "${PYTHON_LDFLAGS_OUTPUT_VARIABLE}")
        set(PYTHON_EXTRA_LINK_FLAGS ${PYTHON_EXTRA_LINK_FLAGS})
      endif(NOT PYTHON_LDFLAGS_RETURN_VALUE)
    
    endif(PYTHONLIBS_FOUND)
    
endif(PYTHONINTERP_FOUND)
