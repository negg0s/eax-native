cmake_minimum_required(VERSION 3.20)
project(eaxctl VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(eaxctl
    main.cpp
)

target_include_directories(eaxctl PRIVATE
    ${CMAKE_SOURCE_DIR}/eax-native/include
    ${CMAKE_SOURCE_DIR}/eax-native/src
    ${CMAKE_SOURCE_DIR}/pipewire-integration/include
)

target_link_libraries(eaxctl PRIVATE
    eax-native
    eax-pipewire
)

# Standard C++ filesystem libraries
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(eaxctl PRIVATE stdc++fs)
endif()

# Latency benchmark probe
add_executable(eax_latency_bench
    ../eax_latency_bench.cpp
)

target_include_directories(eax_latency_bench PRIVATE
    ${CMAKE_SOURCE_DIR}/eax-native/include
    ${CMAKE_SOURCE_DIR}/eax-native/src
    ${CMAKE_SOURCE_DIR}/pipewire-integration/include
)

target_link_libraries(eax_latency_bench PRIVATE
    eax-native
    eax-pipewire
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(eax_latency_bench PRIVATE stdc++fs)
endif()

