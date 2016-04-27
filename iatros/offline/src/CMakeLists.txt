########### test #########


configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)
configure_file(${CMAKE_SOURCE_DIR}/version.h.cmake ${CMAKE_BINARY_DIR}/version.h)

if(STATIC)
  set(LIBIATROS iatros_nonshared)
else(STATIC)
  set(LIBIATROS iatros)
endif(STATIC)

add_executable(iatros-offline recog.c)
target_link_libraries(iatros-offline ${LIBIATROS})

install(TARGETS iatros-offline RUNTIME DESTINATION bin)
install(TARGETS iatros-offline DESTINATION bin)

#add_executable(iatros-gmm gmm.c)
#target_link_libraries(iatros-gmm ${LIBIATROS})

#install(TARGETS iatros-gmm RUNTIME DESTINATION bin)
#install(TARGETS iatros-gmm DESTINATION bin)

#add_executable(iatros-grammar-tool grammar-tool.c)
#target_link_libraries(iatros-grammar-tool ${LIBIATROS})

#install(TARGETS iatros-grammar-tool RUNTIME DESTINATION bin)
#install(TARGETS iatros-grammar-tool DESTINATION bin)

add_executable(iatros-wordlist-offline recog-wordlist.c)
target_link_libraries(iatros-wordlist-offline ${LIBIATROS})

install(TARGETS iatros-wordlist-offline RUNTIME DESTINATION bin)
install(TARGETS iatros-wordlist-offline DESTINATION bin)

