cmake_minimum_required(VERSION 3.20)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../eax-native/include
)

add_executable(test_dsound_compat
    test_dsound_compat.cpp
)

target_link_libraries(test_dsound_compat PRIVATE wine_dsound)

# If MinGW cross-compiler is present, also build the Windows test binary .exe
if(MINGW_CXX)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wine_dsound_test.exe"
        COMMAND ${MINGW_CXX} -O2 -std=c++20
            -I${CMAKE_CURRENT_SOURCE_DIR}/..
            -I${CMAKE_CURRENT_SOURCE_DIR}/../../eax-native/include
            ${CMAKE_CURRENT_SOURCE_DIR}/test_dsound_compat.cpp
            -L$<TARGET_FILE_DIR:wine_dsound> -ldsound
            -o "${CMAKE_CURRENT_BINARY_DIR}/wine_dsound_test.exe"
        DEPENDS test_dsound_compat.cpp wine_dsound
        COMMENT "Cross-compiling Windows PE test binary wine_dsound_test.exe"
    )
    add_custom_target(wine_dsound_test_pe ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/wine_dsound_test.exe")
endif()

add_test(NAME wine_dsound_compat_test COMMAND test_dsound_compat)
