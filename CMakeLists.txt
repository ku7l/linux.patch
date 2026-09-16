cmake_minimum_required(VERSION 3.24)
project(EditionFakerExePatcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(EditionFakerExePatcher main.cpp)

target_compile_definitions(EditionFakerExePatcher PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

if (MSVC)
    target_compile_options(EditionFakerExePatcher PRIVATE /W4 /EHsc)
endif()
