cmake_minimum_required(VERSION 3.6)
project(MiniLog)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    MiniLog/MiniLog.cpp
    MiniLog/MiniLog.hpp)

add_library(MiniLog ${SOURCE_FILES})

add_executable(MiniLogRun sample/main.cpp)

TARGET_LINK_LIBRARIES(MiniLogRun MiniLog pthread)