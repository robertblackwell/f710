cmake_minimum_required(VERSION 3.5)
project(f710
    VERSION 0.0.1
    DESCRIPTION "Logitech F710 gampad reader"
    LANGUAGES CXX
)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
if(ON)
add_executable(f710
        src/main.cpp
        src/f710_time.h
        src/reader.h
        src/model.h
        src/timeout_context.h
        src/model.cpp
#        src/reader.cpp
        src/f710_helpers.cpp
        src/f710_helpers.h
        rbl/logger.cpp
        rbl/logger.h
)
target_include_directories(f710  PUBLIC ./  ./src)
target_compile_definitions(f710 PUBLIC F710_READLOOP) # RBL_LOG_ENABLED RBL_LOG_ALLOW_GLOBAL)
endif()
if(ON)
add_executable(f710_asio
        src/main.cpp
        src/f710_time.h
        src/asio_reader.h
        src/model.h
        src/model.cpp
#        src/asio_reader.cpp
        src/f710_helpers.cpp
        src/f710_helpers.h
        rbl/logger.cpp
        rbl/logger.h
)
target_include_directories(f710_asio  PUBLIC ./  ./src)
target_compile_definitions(f710_asio PUBLIC ASIO_READER)
#target_compile_definitions(f710_asio PUBLIC RBL_LOG_ENABLED RBL_LOG_ALLOW_GLOBAL)
endif()
add_subdirectory("tests/template_ex")