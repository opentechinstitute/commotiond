cmake_minimum_required(VERSION 2.6)

set(PROJECT_NAME_STR commotion)
PROJECT(${PROJECT_NAME_STR})
SET(NO_OPTIMIZE ON CACHE BOOL "Disable optimization flags.")
SET(DEBUG ON CACHE BOOL "Turn on debugging.")
SET(OPENWRT ON CACHE BOOL "Disable subsystems that are unnecessary for OpenWRT platform.")
SET(TEST ON CACHE BOOL "Build unit tests.")
SET(PLUGINS ON CACHE BOOL "Build plugins.")
SET(COMMOTION_VERSION_MAJOR 0)
SET(COMMOTION_VERSION_MINOR 1)
SET(COMMOTION_VERSION_PATCH 0)
SET(COMMOTION_CONFIGFILE \"/etc/commotion/commotiond.conf\")
SET(COMMOTION_STATEDIR \"/var/run/commotion\")
SET(COMMOTION_PIDFILE \"/var/run/commotiond.pid\")
SET(COMMOTION_MANAGESOCK \"/var/run/commotiond.sock\")
SET(COMMOTION_PLUGINDIR \"/usr/lib/commotion/plugins\")
SET(COMMOTION_PROFILEDIR \"/etc/commotion/profiles.d\")

SET(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/src)

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

IF(TEST)
  ENABLE_TESTING()
  ADD_SUBDIRECTORY(tests)
ENDIF()

IF(PLUGINS)
  ADD_SUBDIRECTORY(plugins)
ENDIF()

if(NOT OPENWRT)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc ALL
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  endif(DOXYGEN_FOUND)
endif()

INSTALL(FILES files/commotiond.conf DESTINATION etc/commotion)
INSTALL(DIRECTORY files/profiles.d DESTINATION etc/commotion)
