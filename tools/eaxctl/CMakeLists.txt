cmake_minimum_required(VERSION 3.20)
project(eaxctl VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(eaxctl
    main.cpp
)

# Standard C++ filesystem libraries
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(eaxctl PRIVATE stdc++fs)
endif()
