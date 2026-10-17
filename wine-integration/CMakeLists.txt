cmake_minimum_required(VERSION 3.20)
project(wine-dsound-integration VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../eax-native/include
)

set(SHIM_SOURCES dsound_shim.cpp)
if(WIN32)
    list(APPEND SHIM_SOURCES dsound.def)
endif()

add_library(wine_dsound SHARED
    ${SHIM_SOURCES}
)

target_link_libraries(wine_dsound PRIVATE eax-native)

set_target_properties(wine_dsound PROPERTIES
    OUTPUT_NAME "dsound"
    PREFIX ""
)

# On Windows/MinGW, wine_dsound directly produces dsound.dll.
# Provide dsound_pe target for cross-compilation target compatibility.
if(WIN32)
    add_custom_target(dsound_pe ALL DEPENDS wine_dsound)
else()
    add_custom_target(dsound_pe
        COMMAND ${CMAKE_COMMAND} -E echo "Error: To build the Windows PE dsound.dll, cross-compile the stack using:"
        COMMAND ${CMAKE_COMMAND} -E echo "  cmake -B build-pe64 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64.cmake"
        COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build build-pe64 --target wine_dsound"
        COMMAND ${CMAKE_COMMAND} -E false
        COMMENT "dsound_pe requires cross-compilation with MinGW toolchain"
    )
endif()

# Compatibility test harness
add_subdirectory(tests)
