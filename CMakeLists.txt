cmake_minimum_required(VERSION 3.16)
project(wayland_gpu_api_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_OPENGL_DEMOS "Build OpenGL learning demos" ON)

if(BUILD_OPENGL_DEMOS)
    add_subdirectory(demos/opengl/basic_triangle)
    add_subdirectory(demos/opengl/egl_offscreen)
endif()
