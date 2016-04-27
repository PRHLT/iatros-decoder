include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_FULLNAME}")
set(CPACK_PACKAGE_VENDOR "${PACKAGE_VENDOR}")
set(CPACK_PACKAGE_VERSION_MAJOR ${PACKAGE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PACKAGE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PACKAGE_VERSION_RELEASE})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME} ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}.${CMake_VERSION_PATCH}")
#if(WIN32 and not UNIX)
#  # There is a bug in NSI that does not handle full unix paths properly. Make
#  # sure there is at least one set of four (4) backlasshes.
#  set(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
#  set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
#  set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} My Famous Project")
#  set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.my-project-home-page.org")
#  set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.my-personal-home-page.com")
#  set(CPACK_NSIS_CONTACT "me@my-personal-home-page.com")
#  set(CPACK_NSIS_MODIFY_PATH ON)
#else(WIN32 and not UNIX)
#  set(CPACK_STRIP_FILES "bin/MyExecutable")
#  set(CPACK_SOURCE_STRIP_FILES "")
#endif(WIN32 and not UNIX)
#set(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable")

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(CPACK_GENERATOR "ZIP;NSIS")
else("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
set(CPACK_GENERATOR "STGZ;TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
endif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")


if(EXISTS "${PROJECT_SOURCE_DIR}/CPack.cmake")
  message(STATUS "Including CPack options from ${PROJECT_SOURCE_DIR}/CPack.cmake")
  include("${PROJECT_SOURCE_DIR}/CPack.cmake")
endif(EXISTS "${PROJECT_SOURCE_DIR}/CPack.cmake")
include(CPack)


# must be included before RPM Tools to allow
# the use of DEBIAN_FOUND for setting specific
# directories in rpm generation
include(UseDebian)
if(DEBIAN_FOUND)
  add_debian_targets(${PROJECT_NAME})
endif(DEBIAN_FOUND)

include(UseRPMTools)
if(RPMTools_FOUND)
   RPMTools_ADD_RPM_TARGETS(${PROJECT_NAME})
endif(RPMTools_FOUND)
