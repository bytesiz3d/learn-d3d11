cmake_minimum_required(VERSION 3.20)

add_executable(hello-triangle hello.cpp)
set_target_properties(hello-triangle PROPERTIES WIN32_EXECUTABLE ON)
