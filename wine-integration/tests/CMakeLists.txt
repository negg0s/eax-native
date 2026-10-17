cmake_minimum_required(VERSION 3.20)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../eax-native/include
)

add_executable(test_dsound_compat
    test_dsound_compat.cpp
)

target_link_libraries(test_dsound_compat PRIVATE wine_dsound)

if(WIN32)
    set_target_properties(test_dsound_compat PROPERTIES
        OUTPUT_NAME "wine_dsound_test"
    )
    add_custom_target(wine_dsound_test_pe ALL DEPENDS test_dsound_compat)
endif()

add_test(NAME wine_dsound_compat_test COMMAND test_dsound_compat)
