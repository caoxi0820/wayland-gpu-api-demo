add_executable(opengl_egl_offscreen
    src/main.cpp
)

# EGL：上下文创建与平台抽象
find_package(PkgConfig REQUIRED)
pkg_check_modules(EGL REQUIRED egl)

# OpenGL：渲染 API
find_package(OpenGL REQUIRED)

# GLEW：OpenGL 函数指针加载器（Core Profile 下需要）
find_package(GLEW REQUIRED)

target_include_directories(opengl_egl_offscreen PRIVATE ${EGL_INCLUDE_DIRS})

if(TARGET OpenGL::GL)
    target_link_libraries(opengl_egl_offscreen PRIVATE OpenGL::GL)
else()
    target_link_libraries(opengl_egl_offscreen PRIVATE ${OPENGL_LIBRARIES})
endif()

target_link_libraries(opengl_egl_offscreen PRIVATE GLEW::GLEW)
target_link_libraries(opengl_egl_offscreen PRIVATE ${EGL_LIBRARIES})
