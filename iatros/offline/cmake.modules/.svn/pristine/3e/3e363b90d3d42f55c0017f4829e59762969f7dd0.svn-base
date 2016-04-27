#
# - Find tools needed for generating language bindings
#   for other languages. Basically it finds SWIG and 
#   add utilities for making it easier to generate them
#

find_package(SWIG)
include(${SWIG_USE_FILE})

include(java)
include(python)
find_package(PythonLibs)
include(php)

# create bindings for a given interface
# SWIG_INTERFACE_FILE is the definition of the interface (ej. example.i)
# SWIG_LINK_LIB       is the library with the classes to link
# the rest of the parameters are the language bindings to be generated
macro(swig_add_bindings SWIG_INTERFACE_FILE SWIG_LINK_LIB)
  set_source_files_properties(${SWIG_INTERFACE_FILE} PROPERTIES CPLUSPLUS ON)
  set_source_files_properties(${SWIG_INTERFACE_FILE} PROPERTIES SWIG_FLAGS -Wall )
  #set_source_files_properties(${SWIG_INTERFACE_FILE} PROPERTIES SWIG_FLAGS -includeall)


  foreach(SWIG_BINDING ${ARGN})
    if(SWIG_BINDING STREQUAL "python")
      swig_add_python_bindings(${SWIG_INTERFACE_FILE} ${SWIG_LINK_LIB})
    elseif(SWIG_BINDING STREQUAL "php")
      swig_add_php_bindings(${SWIG_INTERFACE_FILE} ${SWIG_LINK_LIB})
    elseif(SWIG_BINDING STREQUAL "java")
      swig_add_java_bindings(${SWIG_INTERFACE_FILE} ${SWIG_LINK_LIB})
    else(SWIG_BINDING STREQUAL "python")
      message(FATAL_ERROR "Binding language ${SWIG_BINDING} not supported!")
    endif(SWIG_BINDING STREQUAL "python")
  endforeach(SWIG_BINDING ARGN)

endmacro(swig_add_bindings SWIG_INTERFACE_FILE SWIG_LINK_LIB)


# create python bindings for a given interface
# SWIG_INTERFACE_FILE is the definition of the interface (ej. example.i)
# SWIG_LINK_LIB       is the library with the classes to link
macro(swig_add_python_bindings SWIG_INTERFACE_FILE SWIG_LINK_LIB)
  if(ENABLE_PYTHON_BINDINGS)
    set(SWIG_PYTHON_MODULE_NAME ${SWIG_LINK_LIB})
    get_filename_component(SWIG_INTERFACE_NAME ${SWIG_INTERFACE_FILE} NAME_WE)
    swig_add_module(${SWIG_PYTHON_MODULE_NAME} python ${SWIG_INTERFACE_FILE})
    set_source_files_properties(${swig_generated_sources} PROPERTIES COMPILE_FLAGS "-U_POSIX_C_SOURCE -Wno-shadow -Wno-unused-parameter -Wno-missing-field-initializers")
    swig_link_libraries(${SWIG_PYTHON_MODULE_NAME} ${PYTHON_LIBRARIES} ${SWIG_LINK_LIB} "${PYTHON_EXTRA_LINK_FLAGS}")
  endif(ENABLE_PYTHON_BINDINGS)
endmacro(swig_add_python_bindings SWIG_INTERFACE_FILE SWIG_LINK_LIB)


# create php bindings for a given interface
# SWIG_INTERFACE_FILE is the definition of the interface (ej. example.i)
# SWIG_LINK_LIB       is the library with the classes to link
macro(swig_add_php_bindings SWIG_INTERFACE_FILE SWIG_LINK_LIB)
  if(ENABLE_PHP_BINDINGS)
    set(SWIG_PHP_MODULE_NAME lib${SWIG_LINK_LIB}_php)
    get_filename_component(SWIG_INTERFACE_NAME ${SWIG_INTERFACE_FILE} NAME_WE)
    swig_add_module(${SWIG_PHP_MODULE_NAME} php5 ${SWIG_INTERFACE_FILE})
    swig_link_libraries(${SWIG_PHP_MODULE_NAME} ${PHP_LIBRARIES} ${SWIG_LINK_LIB})
    set_source_files_properties(${swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS 
                "${PHP_FLAGS} -D_GNU_SOURCE -Wno-unused -Wno-missing-field-initializers")
  endif(ENABLE_PHP_BINDINGS)
endmacro(swig_add_php_bindings SWIG_INTERFACE_FILE SWIG_LINK_LIB)


# create java bindings for a given interface
# SWIG_INTERFACE_FILE is the definition of the interface (ej. example.i)
# SWIG_LINK_LIB       is the library with the classes to link
macro(swig_add_java_bindings SWIG_INTERFACE_FILE SWIG_LINK_LIB)
  if(ENABLE_JAVA_BINDINGS)

    get_filename_component(SWIG_INTERFACE_NAME ${SWIG_INTERFACE_FILE} NAME_WE)

    set(SWIG_JAVA_MODULE_NAME lib${SWIG_LINK_LIB}_java)
    set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/${SWIG_JAVA_MODULE_NAME}.dir)
   
    set(SWIG_MODULE_${SWIG_JAVA_MODULE_NAME}_wrap_EXTRA_DEPS ${SWIG_INTERFACE_FILE})
    
    set_source_files_properties(${SWIG_INTERFACE_FILE} PROPERTIES SWIG_MODULE_NAME ${SWIG_LINK_LIB})

    file(MAKE_DIRECTORY ${CMAKE_SWIG_OUTDIR} ${CMAKE_SWIG_OUTDIR}/binary)
    include_directories(${CMAKE_SWIG_OUTDIR}/binary)
    swig_add_module(${SWIG_JAVA_MODULE_NAME} java ${SWIG_INTERFACE_FILE})
    set_source_files_properties(${swig_generated_sources} PROPERTIES COMPILE_FLAGS "-fpic -fno-strict-aliasing")
    swig_link_libraries(${SWIG_JAVA_MODULE_NAME} ${SWIG_LINK_LIB})
    
    #set(
    #  ${CMAKE_CURRENT_BINARY_DIR}/lib${SWIG_LINK_LIB}JNI.java_DEPENDS
    #  ${SWIG_INTERFACE_FILE}
    #)
    
    set(JAVA_FILES_FULL 
        ${CMAKE_SWIG_OUTDIR}/${SWIG_LINK_LIB}.java 
        ${CMAKE_SWIG_OUTDIR}/${SWIG_LINK_LIB}JNI.java 
    ) 

    # This may be needed in future when we use CMake java support to build the
    # class files, but it obviously is not correct now when we use custom commands
    # to build the class files using file dependencies to keep things straight.
    set_source_files_properties(${JAVA_FILES_FULL} PROPERTIES GENERATED ON)
    
    set(JAVA_CLASSES)
    foreach(srcfile ${JAVA_FILES_FULL})
      #message(STATUS "src file: ${srcfile}")
      get_filename_component(fileroot ${srcfile} NAME_WE)
      set(output_file ${CMAKE_SWIG_OUTDIR}/${fileroot}.class)
      list(APPEND JAVA_CLASSES ${output_file})
    
      add_custom_command(
        OUTPUT ${output_file}
        COMMAND ${CMAKE_Java_COMPILER} 
          -classpath ${CMAKE_SWIG_OUTDIR} ${srcfile} -d ${CMAKE_SWIG_OUTDIR}/binary
        DEPENDS ${srcfile} ${${output_file}_DEPENDS} ${SWIG_JAVA_MODULE_NAME}
        WORKING_DIRECTORY ${CMAKE_SWIG_OUTDIR}
      )
    endforeach(srcfile ${JAVA_FILES_FULL})

    add_custom_target(${SWIG_INTERFACE_NAME}.jar ALL COMMAND ${JAVA_ARCHIVE} -cf ${CMAKE_CURRENT_BINARY_DIR}/${SWIG_INTERFACE_NAME}.jar -C ${CMAKE_SWIG_OUTDIR}/binary . 
                                                 DEPENDS ${JAVA_CLASSES}  WORKING_DIRECTORY ${CMAKE_SWIG_OUTDIR})

    set(CMAKE_SWIG_OUTDIR)
  endif(ENABLE_JAVA_BINDINGS)
endmacro(swig_add_java_bindings SWIG_INTERFACE_FILE SWIG_LINK_LIB)

## Swig generated java files.  Order no longer matters since class 
## dependencies are explicitly handled now.
#
#set(
#SWIG_JAVA_FILES
#plplotjavacJNI.java 
#PLGraphicsIn.java 
#plplotjavacConstants.java
#plplotjavac.java
#)
#
## Full list of generated java files
#set(
#JAVA_GEN_FILES
#config.java
#${SWIG_JAVA_FILES}
#)
#
## List of generated java files with full path names
## Need this otherwise cmake will look in the source directory for the
## .java files.
#string(
#REGEX REPLACE "([a-zA-z]*)\\.java"
#"${CMAKE_CURRENT_BINARY_DIR}/\\1.java" 
#JAVA_GEN_FILES_FULL 
#"${JAVA_GEN_FILES}"
#)
#
## Full pathnames for all java files.
#set(
#JAVA_FILES_FULL
#${JAVA_GEN_FILES_FULL}
#${CMAKE_CURRENT_SOURCE_DIR}/PLStream.java
#)
#
## Explicit full-path class dependencies for the foreach loop below which
## should build classes in the correct order regardless of whether it
## is a parallel build or not and regardless of the order of JAVA_FILES_FULL.
## These are hand-crafted dependencies based on scanning the appropriate
## Java sources.  Apparently CMake has a java dependency scanner which
## might be good enough to do this task as well, but I have avoided it because
## I (AWI) am under the impression that CMake support of java is still in its
## infancy.
#
#set(class_root ${CMAKE_CURRENT_BINARY_DIR}/plplot/core)
#
#set(
#${class_root}/PLGraphicsIn.class_DEPENDS
#${class_root}/plplotjavacJNI.class
#)
#
#set(
#${class_root}/plplotjavacConstants.class_DEPENDS
#${class_root}/plplotjavacJNI.class
#)
#
#set(
#${class_root}/plplotjavac.class_DEPENDS
#${class_root}/plplotjavacConstants.class
#${class_root}/PLGraphicsIn.class
#${class_root}/plplotjavacJNI.class
#)
#
#set(
#${class_root}/PLStream.class_DEPENDS
#${class_root}/plplotjavacConstants.class
#${class_root}/config.class
#${class_root}/plplotjavac.class
#)
#
## This is currently the include list for swig, the C wrapper and the
## the java classpath. Not particular pretty...
#set(java_interface_INCLUDE_PATHS
#${CMAKE_SOURCE_DIR}
#${CMAKE_SOURCE_DIR}/include
#${CMAKE_CURRENT_BINARY_DIR}
#${CMAKE_BINARY_DIR}
#${CMAKE_BINARY_DIR}/include
#${JAVA_INCLUDE_PATH}
#${CMAKE_SOURCE_DIR}/bindings/swig-support
#)
## On some systems JAVA_INCLUDE_PATH2 returns JAVA_INCLUDE_PATH2-NOTFOUND
#if(JAVA_INCLUDE_PATH2)
#  set(java_interface_INCLUDE_PATHS
#  ${java_interface_INCLUDE_PATHS}
#  ${JAVA_INCLUDE_PATH2}
#  )
#endif(JAVA_INCLUDE_PATH2)
#include_directories(${java_interface_INCLUDE_PATHS})
#
## Can't use source file properties as we have to quote the flags in that 
## case and it breaks swig. Doh! I would call this a cmake bug.
#if(SWIG_JAVA_NOPGCPP)
#  #This version of swig supports the -nopgcpp option
#  set(CMAKE_SWIG_FLAGS -DPL_DOUBLE_INTERFACE -DSWIG_JAVA -package plplot.core -nopgcpp)
#else(SWIG_JAVA_NOPGCPP)
#  set(CMAKE_SWIG_FLAGS -DPL_DOUBLE_INTERFACE -DSWIG_JAVA -package plplot.core)
#endif(SWIG_JAVA_NOPGCPP)
#
#set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})
#
## This may be needed in future when we use CMake java support to build the
## class files, but it obviously is not correct now when we use custom commands
## to build the class files using file dependencies to keep things straight.
### set_source_files_properties(${JAVA_GEN_FILES_FULL} PROPERTIES GENERATED ON)
#
#set(SWIG_MODULE_plplotjavac_wrap_EXTRA_DEPS
#${CMAKE_SOURCE_DIR}/bindings/swig-support/plplotcapi.i)
#
#set_source_files_properties(
#plplotjavac.i 
#PROPERTIES SWIG_MODULE_NAME plplotjavac
#)
#
## Set up swig + c wrapper
#swig_add_module(plplotjavac_wrap java plplotjavac.i)
#swig_link_libraries(plplotjavac_wrap plplot${LIB_TAG})
#
## Create config.java.  Other generated java files created by swig.
#configure_file(
#${CMAKE_CURRENT_SOURCE_DIR}/config.java.in
#${CMAKE_CURRENT_BINARY_DIR}/config.java
#)
#
#if(USE_RPATH)
#  get_target_property(LIB_INSTALL_RPATH plplot${LIB_TAG} INSTALL_RPATH)
#  set_target_properties(
#  plplotjavac_wrap
#  PROPERTIES
#  INSTALL_RPATH "${LIB_INSTALL_RPATH}"
#  INSTALL_NAME_DIR "${JAVAWRAPPER_HARDDIR}"
#  )
#else(USE_RPATH)
#  set_target_properties(
#  plplotjavac_wrap
#  PROPERTIES
#  INSTALL_NAME_DIR "${JAVAWRAPPER_HARDDIR}"
#  )
#endif(USE_RPATH)
#
## Ensure we get the correct suffix for OS-X 
#if(APPLE)
#  set_target_properties(
#  plplotjavac_wrap
#  PROPERTIES
#  SUFFIX ".dylib"
#  )
#endif(APPLE)
#
#install(TARGETS plplotjavac_wrap LIBRARY DESTINATION ${JAVAWRAPPER_HARDDIR})
#
#set(JAVA_CLASSES)
#foreach( srcfile ${JAVA_FILES_FULL} )
#  get_filename_component(fileroot ${srcfile} NAME_WE)
#  set(output_file ${CMAKE_CURRENT_BINARY_DIR}/plplot/core/${fileroot}.class)
#  list(APPEND JAVA_CLASSES ${output_file})
#  add_custom_command(
#  OUTPUT ${output_file}
#  COMMAND ${CMAKE_Java_COMPILER} 
#  -classpath ${CMAKE_CURRENT_BINARY_DIR} ${srcfile} -d ${CMAKE_CURRENT_BINARY_DIR}
#  DEPENDS ${srcfile} ${${output_file}_DEPENDS}
#  )
#endforeach( srcfile ${JAVA_FILES_FULL} )
#add_custom_target(plplot_core ALL DEPENDS ${JAVA_CLASSES})
#
## Ensure that swig is executed before we try to compile the java
## classes.
#add_dependencies(plplot_core plplotjavac_wrap)
#
## Java compiler doesn't support -D option.
#remove_definitions("-DHAVE_CONFIG_H")
#
## Installed as part of the example/java directory
##install(FILES ${CMAKE_BINARY_DIR}/plplot.jar DESTINATION ${JAR_DIR})


