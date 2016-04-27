# Set build info variables

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_RELEASE 00)

# Version
message(STATUS "Retrieving project info from '${PROJECT_SOURCE_DIR}'")
if(EXISTS "${PROJECT_SOURCE_DIR}/VERSION")
    file(READ "${PROJECT_SOURCE_DIR}/VERSION" VERSION_IN_FILE)
    message(STATUS "  * VERSION file: ${VERSION_IN_FILE}")
    string(REGEX REPLACE "^ *([0-9]+).*$" "\\1" VERSION_MAJOR "${VERSION_IN_FILE}")
    string(REGEX REPLACE "^ *([0-9]+)\\.([0-9]+).*$" "\\2" VERSION_MINOR "${VERSION_IN_FILE}")
endif(EXISTS "${PROJECT_SOURCE_DIR}/VERSION")

# Check git info
find_package(Git)
if(Git_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  message(STATUS "  * git information: ")
  git_wt_info(${PROJECT_SOURCE_DIR} PROJECT)
  message(STATUS "      branch:${PROJECT_WT_BRANCH};state:${PROJECT_WT_STATE};idx:${PROJECT_WT_COMMIT}")
  set(GIT_INFO "\\n\\tgit info:\\n\\t\\tpath:\\t${PROJECT_WT_URL}\\n\\t\\tbranch:\\t${PROJECT_WT_BRANCH}\\n\\t\\t"
               "\\bstate:\\t${PROJECT_WT_STATE}\\n\\t\\tcommit:\\t${PROJECT_WT_COMMIT}\\n")
  set(GIT_COMMIT_NUMBER ${PROJECT_COMMIT_NUMBER})
endif(Git_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")

#Check svn info
find_package(Subversion)
if(GIT_SVN_FOUND)
    set(HAVE_SVN_INFO TRUE)
elseif(Subversion_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.svn")
    set(HAVE_SVN_INFO TRUE)
    Subversion_WC_INFO(${PROJECT_SOURCE_DIR} PROJECT)
endif(GIT_SVN_FOUND)

if(HAVE_SVN_INFO)
  message(STATUS "  * svn information: ")
  message(STATUS "      branch:${PROJECT_WC_URL};rev:${PROJECT_WC_REVISION}")
  #set(SVN_INFO "${PROJECT_WC_INFO}")
  set(SVN_INFO "\\n\\tsvn info:\\n\\t\\turl:\\t${PROJECT_WC_URL}\\n\\t\\t"
               "\\brev:\\t${PROJECT_WC_REVISION}\\n")
  set(VERSION_RELEASE "${PROJECT_WC_REVISION}")
else(HAVE_SVN_INFO)
  set(VERSION_RELEASE "local")
endif(HAVE_SVN_INFO)


# Set information about system compilation
if(HAVE_FORCE32)
  set(FORCED32_INFO " (compiled for 32bits)")
endif(HAVE_FORCE32)

set(BUILD_INFO "\\n\\tbuild system:\\n\\t\\tsystem name:\\t${CMAKE_SYSTEM_NAME}\\n\\t\\tprocessor:\\t${CMAKE_SYSTEM_PROCESSOR}${FORCED32_INFO}\\n${GIT_INFO}${SVN_INFO}")
string(REGEX REPLACE "\\\"" "\\\\\"" BUILD_INFO_TEX "${BUILD_INFO}")
string(REGEX REPLACE "\n" "\\\\n" BUILD_INFO_TEX "${BUILD_INFO_TEX}")
#string(REGEX REPLACE "\\\\n" "\\\\\\\\\n" BUILD_INFO_TEX "${BUILD_INFO}")
#string(REGEX REPLACE "\\\\t" "\t" BUILD_INFO_TEX "${BUILD_INFO_TEX}")
#string(REGEX REPLACE ";\\\\b" ""  BUILD_INFO_TEX "${BUILD_INFO_TEX}")
set(BUILD_INFO "${BUILD_INFO_TEX}")

set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}-svn${VERSION_RELEASE}" )
if(CMAKE_BUILD_TYPE)
  set(VERSION_STRING "${VERSION} (${CMAKE_BUILD_TYPE})")
else(CMAKE_BUILD_TYPE)
  set(VERSION_STRING "${VERSION}")
endif(CMAKE_BUILD_TYPE)

string(REGEX REPLACE "-" "_" PROJECT_PREFIX "${PROJECT_NAME}")
string(REGEX REPLACE " " "_" PROJECT_PREFIX "${PROJECT_PREFIX}")
string(TOUPPER "${PROJECT_PREFIX}" PROJECT_PREFIX)

message(STATUS "  * version string: ${VERSION_STRING}")
