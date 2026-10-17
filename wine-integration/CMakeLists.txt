cmake_minimum_required(VERSION 3.20)
project(wine-dsound-integration VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine whether MinGW cross-compiler is available
find_program(MINGW_CXX x86_64-w64-mingw32-g++)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../eax-native/include
)

# 1. Native shared library target (can be built on any Linux or MinGW host)
add_library(wine_dsound SHARED
    dsound_shim.cpp
)

target_link_libraries(wine_dsound PRIVATE eax-native)

set_target_properties(wine_dsound PROPERTIES
    OUTPUT_NAME "dsound"
    PREFIX ""
)

# If MinGW cross-compiler is present, also declare the PE cross-compilation rule
if(MINGW_CXX)
    message(STATUS "MinGW cross-compiler found: ${MINGW_CXX}")
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dsound.dll"
        COMMAND ${MINGW_CXX} -shared -O2 -std=c++20
            -I${CMAKE_CURRENT_SOURCE_DIR}
            -I${CMAKE_CURRENT_SOURCE_DIR}/../eax-native/include
            ${CMAKE_CURRENT_SOURCE_DIR}/dsound_shim.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/dsound.def
            -L$<TARGET_FILE_DIR:eax-native> -leax-native
            -o "${CMAKE_CURRENT_BINARY_DIR}/dsound.dll"
        DEPENDS dsound_shim.cpp dsound.def eax-native
        COMMENT "Cross-compiling Windows PE dsound.dll using ${MINGW_CXX}"
    )
    add_custom_target(dsound_pe ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/dsound.dll")
endif()

# Compatibility test harness
add_subdirectory(tests)
