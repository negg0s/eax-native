cmake_minimum_required(VERSION 3.20)
project(linux-gaming-stack VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

# 1. Native EAX & OpenAL Soft translation engine
add_subdirectory(eax-native)

# 2. Wine DirectSound PE shim & integration
add_subdirectory(wine-integration)

# 3. eaxctl management CLI tool
add_subdirectory(tools/eaxctl)
