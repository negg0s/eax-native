cmake_minimum_required(VERSION 3.20)
project(eax-native VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Warning options applied per-target to prevent poisoning third_party subprojects
set(EAX_COMPILE_OPTIONS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(EAX_COMPILE_OPTIONS -Wall -Wextra -Werror -pedantic)
elseif(MSVC)
    set(EAX_COMPILE_OPTIONS /W4 /WX /permissive-)
endif()

# Build options
option(EAX_BUILD_TESTS "Build eax-native test suite" ON)
option(EAX_BUILD_DEMO "Build eax-native 3D fly-by demo" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Check and incorporate OpenAL Soft subproject if present
set(OPENAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openal-soft")
if(EXISTS "${OPENAL_DIR}/CMakeLists.txt")
    set(ALSOFT_UTILS OFF CACHE BOOL "Build alsoft utils" FORCE)
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "Build alsoft examples" FORCE)
    set(ALSOFT_TESTS OFF CACHE BOOL "Build alsoft tests" FORCE)
    set(ALSOFT_INSTALL OFF CACHE BOOL "Install alsoft" FORCE)
    add_subdirectory("${OPENAL_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/third_party/openal-soft" EXCLUDE_FROM_ALL)
    set(OPENAL_INCLUDE_DIRS "${OPENAL_DIR}/include" "${OPENAL_DIR}/include/AL")
    set(OPENAL_LIBRARY OpenAL)
else()
    find_package(OpenAL REQUIRED)
    set(OPENAL_INCLUDE_DIRS ${OPENAL_INCLUDE_DIR})
    set(OPENAL_LIBRARY ${OPENAL_LIBRARY})
endif()

# Core Library: eax-native
add_library(eax-native
    src/openal_bridge.cpp
    src/arena.cpp
    src/voice_pool.cpp
    src/eax_reverb.cpp
    src/context.cpp
    src/abi_dispatch.cpp
)

target_include_directories(eax-native
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OPENAL_INCLUDE_DIRS}
)

if(TARGET OpenAL)
    target_link_libraries(eax-native PRIVATE OpenAL)
elseif(OPENAL_LIBRARY)
    target_link_libraries(eax-native PRIVATE ${OPENAL_LIBRARY})
endif()

target_compile_definitions(eax-native
    PUBLIC
        EAX_NATIVE_BUILD_SHARED
)

if(EAX_COMPILE_OPTIONS)
    target_compile_options(eax-native PRIVATE ${EAX_COMPILE_OPTIONS})
endif()

# Demo executable
if(EAX_BUILD_DEMO)
    add_executable(eax_demo
        src/demo.cpp
    )
    target_link_libraries(eax_demo PRIVATE eax-native)
    target_include_directories(eax_demo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OPENAL_INCLUDE_DIRS}
    )
    if(EAX_COMPILE_OPTIONS)
        target_compile_options(eax_demo PRIVATE ${EAX_COMPILE_OPTIONS})
    endif()
endif()

# Test suite with Catch2
if(EAX_BUILD_TESTS)
    enable_testing()

    add_executable(eax_tests
        tests/main_tests.cpp
        tests/catch_amalgamated.cpp
        tests/test_abi_layout.cpp
        tests/test_coordinates.cpp
        tests/test_spatial_pan_rms.cpp
        tests/test_eax_reverb.cpp
        tests/test_properties.cpp
    )

    target_include_directories(eax_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
            ${OPENAL_INCLUDE_DIRS}
    )

    target_link_libraries(eax_tests PRIVATE eax-native)

    if(EAX_COMPILE_OPTIONS)
        target_compile_options(eax_tests PRIVATE ${EAX_COMPILE_OPTIONS})
    endif()

    # Disable pedantic errors on external test framework code if necessary
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set_source_files_properties(tests/catch_amalgamated.cpp PROPERTIES COMPILE_FLAGS "-Wno-error")
    endif()

    add_test(NAME eax_unit_tests COMMAND eax_tests)
endif()
