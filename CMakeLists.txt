cmake_minimum_required(VERSION 3.5)
project(f710
    VERSION 0.0.1
    DESCRIPTION "Logitech F710 gampad reader"
    LANGUAGES CXX
)
set(CMAKE_BUILD_TYPE Debug)
add_executable(f710
        ../f710_demo/main.cpp
        src/f710_time.h
        src/f710.h
        src/f710.cpp
        src/f710_helpers.cpp
        src/f710_helpers.h
        rbl/logger.cpp
        rbl/logger.h
)
target_include_directories(f710  PUBLIC ./  ./src)
#target_compile_definitions(f710 PUBLIC RBL_LOG_ENABLED RBL_LOG_ALLOW_GLOBAL)