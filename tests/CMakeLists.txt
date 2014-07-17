CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)
REMOVE_DEFINITIONS("--std=gnu99")
PROJECT(gtest_builder C CXX)
INCLUDE(ExternalProject)

ExternalProject_Add(googletest
    SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk
    CMAKE_ARGS -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
               -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
               -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
               -Dgtest_force_shared_crt=ON
     PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
# Disable install step
    INSTALL_COMMAND ""
)

# Specify include dir
ExternalProject_Get_Property(googletest source_dir)
SET(GTEST_INCLUDE_DIR ${source_dir}/include)

# Specify MainTest's link libraries
ExternalProject_Get_Property(googletest binary_dir)
SET(GTEST_LIBRARY ${binary_dir}/libgtest.a)
SET(GTEST_MAIN_LIBRARY ${binary_dir}/libgtest_main.a)

#-------------------
# Test
#-------------------
ENABLE_TESTING()
INCLUDE(FindGTest QUIETLY)
SET(PROJECT_TEST_NAME ${PROJECT_NAME_STR}_test)
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIR} ${COMMON_INCLUDES})

FILE(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
ADD_EXECUTABLE(${PROJECT_TEST_NAME} ${TEST_SRC_FILES})
ADD_DEPENDENCIES(${PROJECT_TEST_NAME} googletest)

TARGET_LINK_LIBRARIES(${PROJECT_TEST_NAME} commotion)
TARGET_LINK_LIBRARIES(${PROJECT_TEST_NAME}
	${GTEST_LIBRARY}
	${GTEST_MAIN_LIBRARY})
TARGET_LINK_LIBRARIES(${PROJECT_TEST_NAME} pthread)

GTEST_ADD_TESTS(${PROJECT_TEST_NAME} "" ${TEST_SRC_FILES})

ADD_CUSTOM_COMMAND(TARGET ${PROJECT_TEST_NAME} POST_BUILD COMMAND ${PROJECT_TEST_NAME})

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
