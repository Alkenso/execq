cmake_minimum_required(VERSION 2.8) 

project(execq, DESCRIPTION "Threadpool-like implementation that supports task execution in queue-like and stream-like ways.")

if (NOT WIN32)
	add_compile_options(-std=c++11 -stdlib=libc++)
endif()

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.8")
endif()


AUX_SOURCE_DIRECTORY(src LIB_SOURCES)
set(LIB_INCLUDES "include" "include/execq" "include/execq/internal")

include_directories(${LIB_INCLUDES})
add_library(execq STATIC ${LIB_SOURCES})
