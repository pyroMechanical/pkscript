cmake_minimum_required(VERSION 3.10)

project(pkscript VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE sources RELATIVE ${PROJECT_SOURCE_DIR} "*.cpp" "*.h")

add_executable(pkscript ${sources})