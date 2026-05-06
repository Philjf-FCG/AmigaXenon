cmake_minimum_required(VERSION 3.20)
project(LibretroHarness LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(libretro_harness
    main.cpp
)

target_include_directories(libretro_harness PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Plugins/UnrealLibretro/Source/UnrealLibretro/Public
)

if (WIN32)
    target_compile_definitions(libretro_harness PRIVATE UNICODE _UNICODE)
endif()
