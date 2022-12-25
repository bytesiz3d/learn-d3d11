cmake_minimum_required(VERSION 3.20)

add_executable(hello-triangle hello.cpp)
set_target_properties(hello-triangle PROPERTIES WIN32_EXECUTABLE ON)
target_compile_definitions(hello-triangle PRIVATE SHADER_PATH=L"${CMAKE_CURRENT_SOURCE_DIR}/hello.hlsl")