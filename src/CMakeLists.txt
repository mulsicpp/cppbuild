cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(cppbuild)

add_executable(cppbuild main.cpp CppBuilder.cpp native_commands.cpp ProjectInfo.cpp compiler_commands.cpp)