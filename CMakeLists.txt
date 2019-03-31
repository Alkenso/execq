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

set(LIB_SOURCES
    include/execq/IExecutionStream.h
    include/execq/IExecutionQueue.h
    include/execq/ExecutionPool.h
    include/execq/execq.h

    include/execq/internal/TaskProviderList.h
    include/execq/internal/ITaskProvider.h
    include/execq/internal/ExecutionStream.h
    include/execq/internal/ExecutionQueue.h
    include/execq/internal/ThreadWorker.h
    include/execq/internal/CancelTokenProvider.h

    src/ThreadWorker.cpp
    src/TaskProviderList.cpp
    src/ExecutionStream.cpp
    src/ExecutionPool.cpp
    src/CancelTokenProvider.cpp
    src/CancelTokenProviderTest.cpp
)

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
    set(TEST_SOURCES
        tests/TaskProviderListTest.cpp
        tests/ExecutionStreamTest.cpp
        tests/ExecutionQueueTest.cpp
    )
    add_executable(execq_tests ${TEST_SOURCES})

    # setup 3rdParty
    add_subdirectory(3rdParty/googletest)
    set_target_properties(gmock PROPERTIES FOLDER 3rdParty)
    set_target_properties(gmock_main PROPERTIES FOLDER 3rdParty)
    set_target_properties(gtest PROPERTIES FOLDER 3rdParty)
    set_target_properties(gtest_main PROPERTIES FOLDER 3rdParty)

    target_link_libraries(execq_tests execq gtest gmock gmock_main)
endif()
