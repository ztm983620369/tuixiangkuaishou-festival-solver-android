cmake_minimum_required(VERSION 3.22.1)
project(festivalcore LANGUAGES CXX)

file(GLOB FESTIVAL_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/festival/*.cpp")

set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/festival/sokoban_solver.cpp"
        PROPERTIES COMPILE_DEFINITIONS "main=festival_cli_main")

add_library(festivalcore SHARED
        festival_jni.cpp
        ${FESTIVAL_SOURCES})

target_include_directories(festivalcore PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/festival")

target_compile_definitions(festivalcore PRIVATE
        THREADS
        __ANDROID_FESTIVAL__)

target_compile_options(festivalcore PRIVATE
        -Os
        -fsigned-char
        -fpermissive
        -Wno-format
        -Wno-unused-result)

target_link_libraries(festivalcore
        android
        log)
