cmake_minimum_required(VERSION 3.4.3)
project(rascal C CXX)

set(CMAKE_CXX_FLAGS "-g -O0 -Wall -Werror -Wno-error=unused-result -Wno-error=unused-variable -fsanitize=address,undefined -static-libasan ")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -O0 -fsanitize=address,undefined -static-libasan -Wno-error=unused-variable")
add_executable(rascal main.cpp)
set_target_properties(rascal PROPERTIES CXX_STANDARD 17)
