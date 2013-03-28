cmake_minimum_required(VERSION 2.6)

PROJECT(commotion)
SET(NO_OPTIMIZE 1)
SET(DEBUG 1)
SET(OPENWRT 1)
SET(COMMOTION_VERSION_MAJOR 0)
SET(COMMOTION_VERSION_MINOR 1)
SET(COMMOTION_VERSION_PATCH 0)
SET(COMMOTION_STATEDIR \"/var/run/commotion\")
SET(COMMOTION_PIDFILE \"/var/run/commotiond.pid\")
SET(COMMOTION_MANAGESOCK \"/var/run/commotiond.sock\")
SET(COMMOTION_PLUGINDIR \"/usr/lib/commotion/plugins\")
SET(COMMOTION_PROFILEDIR \"/etc/commotion/profiles.d\")
SET(CMAKE_EXE_LINKER_FLAGS "-ldl")

CONFIGURE_FILE( "${CMAKE_SOURCE_DIR}/src/config.h.in" "${CMAKE_SOURCE_DIR}/src/config.h" )

ADD_DEFINITIONS(-Os -Wall --std=gnu99 -Wmissing-declarations -DNDEBUG)

IF(OPENWRT)
  ADD_DEFINITIONS(-D_OPENWRT)
ENDIF()

IF(DEBUG)
  ADD_DEFINITIONS(-g3)
  REMOVE_DEFINITIONS(-DNDEBUG)
  IF(NO_OPTIMIZE)
    ADD_DEFINITIONS(-O0)
  ENDIF()
ENDIF()

ADD_SUBDIRECTORY(src)

# CMake + Doxygen
# https://majewsky.wordpress.com/2010/08/14/tip-of-the-day-cmake-and-doxygen/
# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc ALL
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)
