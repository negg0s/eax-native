cmake_minimum_required(VERSION 3.20)
project(linux-gaming-stack VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

# Optional components: PE shim needs a MinGW cross toolchain (or native MinGW),
# PipeWire needs pipewire dev headers (Linux only)
# The Wine PE shim is MinGW/wine-target code; MSVC cannot compile it
# (conflicting cdecl/extern decls for DirectSound*), so default it off there.
if(MSVC)
    option(EAX_BUILD_WINE_SHIM "Build Wine dsound PE shim" OFF)
else()
    option(EAX_BUILD_WINE_SHIM "Build Wine dsound PE shim" ON)
endif()
set(EAX_BUILD_PIPEWIRE_DEFAULT ON)
if(WIN32)
    set(EAX_BUILD_PIPEWIRE_DEFAULT OFF)
endif()
option(EAX_BUILD_PIPEWIRE "Build PipeWire DSP filter" ${EAX_BUILD_PIPEWIRE_DEFAULT})

# 1. Native EAX & OpenAL Soft translation engine
add_subdirectory(eax-native)

# 2. Wine DirectSound PE shim & integration (cross toolchain or native MinGW)
if(EAX_BUILD_WINE_SHIM)
    add_subdirectory(wine-integration)
endif()

# 3. PipeWire DSP filter & SPA node integration
if(EAX_BUILD_PIPEWIRE)
    add_subdirectory(pipewire-integration)
endif()

# 4. eaxctl management CLI tool
add_subdirectory(tools/eaxctl)
