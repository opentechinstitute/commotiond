cmake_minimum_required(VERSION 2.8.8)
project(gtest_builder C CXX)
include(ExternalProject)

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
set(GTEST_INCLUDE_DIRS ${source_dir}/include)

# Specify MainTest's link libraries
ExternalProject_Get_Property(googletest binary_dir)
set(GTEST_LIBS_DIR ${binary_dir})

#-------------------
# Test
#-------------------
enable_testing()
set(PROJECT_TEST_NAME ${PROJECT_NAME_STR}_test)
include_directories(${GTEST_INCLUDE_DIRS} ${COMMON_INCLUDES})

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES})
add_dependencies(${PROJECT_TEST_NAME} googletest)


target_link_libraries(${PROJECT_TEST_NAME} commotion)
target_link_libraries(${PROJECT_TEST_NAME} pthread)
target_link_libraries(${PROJECT_TEST_NAME}
	${GTEST_LIBS_DIR}/libgtest.a
	${GTEST_LIBS_DIR}/libgtest_main.a)

add_test(test1 ${PROJECT_TEST_NAME})

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
