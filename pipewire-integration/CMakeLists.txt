cmake_minimum_required(VERSION 3.20)
project(pipewire-integration VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3 libspa-0.2)

# Warning options applied per-target (allowing GNU extensions used by PipeWire C headers)
set(PW_COMPILE_OPTIONS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(PW_COMPILE_OPTIONS -Wall -Wextra -Werror -Wno-error=pedantic -Wno-pedantic)
endif()

# Core PipeWire DSP Filter Library
add_library(eax-pipewire SHARED
    src/eax_pipewire_filter.cpp
    src/spa_eax_node.cpp
)

target_include_directories(eax-pipewire
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${PIPEWIRE_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/eax-native/include
        ${CMAKE_SOURCE_DIR}/eax-native/src
)

target_link_libraries(eax-pipewire
    PUBLIC
        eax-native
        ${PIPEWIRE_LIBRARIES}
)

target_compile_options(eax-pipewire PRIVATE ${PIPEWIRE_CFLAGS_OTHER})
if(PW_COMPILE_OPTIONS)
    target_compile_options(eax-pipewire PRIVATE ${PW_COMPILE_OPTIONS})
endif()

# SPA Plugin (Simple Plugin Architecture Module)
add_library(spa-eax-filter MODULE
    src/spa_eax_node.cpp
)

target_include_directories(spa-eax-filter
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/eax-native/include
        ${PIPEWIRE_INCLUDE_DIRS}
)

target_link_libraries(spa-eax-filter
    PRIVATE
        eax-native
        ${PIPEWIRE_LIBRARIES}
)

target_compile_options(spa-eax-filter PRIVATE ${PIPEWIRE_CFLAGS_OTHER})
if(PW_COMPILE_OPTIONS)
    target_compile_options(spa-eax-filter PRIVATE ${PW_COMPILE_OPTIONS})
endif()

# Integration Test
add_executable(test_pipewire_filter
    tests/test_pipewire_filter.cpp
)

target_include_directories(test_pipewire_filter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/eax-native/include
        ${CMAKE_SOURCE_DIR}/eax-native/src
        ${PIPEWIRE_INCLUDE_DIRS}
)

target_link_libraries(test_pipewire_filter
    PRIVATE
        eax-pipewire
        eax-native
        ${PIPEWIRE_LIBRARIES}
)

if(PW_COMPILE_OPTIONS)
    target_compile_options(test_pipewire_filter PRIVATE ${PW_COMPILE_OPTIONS})
endif()

add_test(NAME pipewire_filter_test COMMAND test_pipewire_filter)
