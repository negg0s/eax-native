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
)
# The PipeWire DSP library is optional (Linux-only by default). Link it and
# enable the pw backend only when the subproject actually produced the target.
if(TARGET eax-pipewire)
    target_link_libraries(eaxctl PRIVATE eax-pipewire)
    target_compile_definitions(eaxctl PRIVATE EAX_WITH_PIPEWIRE=1)
endif()

# eaxctl links the shared eax-native library (which imports OpenAL32.dll)
# - deploy both runtimes next to the exe
add_dependencies(eaxctl eax-native)
add_custom_command(TARGET eaxctl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:eax-native> $<TARGET_FILE_DIR:eaxctl>
    COMMENT "Deploying eax-native runtime next to eaxctl"
)
if(TARGET OpenAL)
    add_custom_command(TARGET eaxctl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:OpenAL> $<TARGET_FILE_DIR:eaxctl>
        COMMENT "Deploying vendored OpenAL runtime next to eaxctl"
    )
endif()

# Standard C++ filesystem libraries
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(eaxctl PRIVATE stdc++fs)
endif()

# Latency benchmark probe — PipeWire-only by design (it measures the PW
# DSP filter round-trip), so build it only when the PW target exists.
if(TARGET eax-pipewire)
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
    )
    target_link_libraries(eax_latency_bench PRIVATE eax-pipewire)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(eax_latency_bench PRIVATE stdc++fs)
    endif()
endif()

