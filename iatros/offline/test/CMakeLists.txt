# Enables a test target
# To add test executables use:
# add_test(testname Exename arg1 arg2 ...)

add_test(text ${CMAKE_CURRENT_SOURCE_DIR}/test.sh text)
add_test(eutrans ${CMAKE_CURRENT_SOURCE_DIR}/test.sh eutrans)
add_test(wsj ${CMAKE_CURRENT_SOURCE_DIR}/test.sh wsj)
