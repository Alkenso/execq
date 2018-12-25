cmake_minimum_required(VERSION 3.0.2) 

project(execq DESCRIPTION "Threadpool-like implementation that supports task execution in queue-like and stream-like ways.")

if (NOT WIN32)
	add_compile_options(-std=c++11 -stdlib=libc++)
endif()

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
endif()

OPTION(EXECQ_ENABLE_TESTING "Build execq's unit-tests." OFF)

### execq library ###

aux_source_directory(src LIB_SOURCES)

add_library(execq STATIC ${LIB_SOURCES})

target_include_directories(execq PUBLIC
    "include"
)

include_directories(execq PRIVATE
    "include/execq" 
    "include/execq/internal"
)


### execq unit-tests ###

if (EXECQ_ENABLE_TESTING)
    aux_source_directory(tests TEST_SOURCES)
    add_executable(execq_tests ${TEST_SOURCES})

    # setup 3rdParty
    add_subdirectory(3rdParty/googletest)
    set_target_properties(gmock PROPERTIES FOLDER 3rdParty)
    set_target_properties(gmock_main PROPERTIES FOLDER 3rdParty)
    set_target_properties(gtest PROPERTIES FOLDER 3rdParty)
    set_target_properties(gtest_main PROPERTIES FOLDER 3rdParty)

    target_link_libraries(execq_tests execq gtest gmock gmock_main)
endif()
