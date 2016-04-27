set(GTK2_DIR ${CMAKE_SOURCE_DIR}/mingw32/gtk2)

set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${GTK2_DIR})

include_directories(
  ${GTK2_DIR}/include/cairo
  ${GTK2_DIR}/include/glib-2.0
  ${GTK2_DIR}/lib/glib-2.0/include
  ${GTK2_DIR}/include/gtk-2.0
  ${GTK2_DIR}/lib/gtk-2.0/include
  ${GTK2_DIR}/include/atk-1.0
  ${GTK2_DIR}/include/gail-1.0
  ${GTK2_DIR}/include/pango-1.0
  ${GTK2_DIR}/include/libpng12
)

set(GTK2_LIB ${GTK2_LIB})

file(GLOB dll_list ${GTK2_DIR}/bin/*.dll)
foreach(dll_file ${dll_list})
  get_filename_component(file_name ${dll_file} NAME)
  string(REGEX REPLACE ".dll$" "" lib_name ${file_name})
  find_library(${lib_name}_LIB ${lib_name})
  if(NOT ${lib_name}_LIB)
    message(FATAL_ERROR "${lib_name} library not found")
  #else(NOT ${${lib_name}_LIB})
  #  message(STATUS "Gtk2Win32 ${lib_name} library found")
  endif(NOT ${lib_name}_LIB)
  list(APPEND GTK2_LIB ${${lib_name}_LIB})
endforeach(dll_file)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mms-bitfields")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mms-bitfields")


