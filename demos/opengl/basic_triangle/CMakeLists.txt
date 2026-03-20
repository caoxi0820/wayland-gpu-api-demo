add_executable(opengl_basic_triangle
    src/main.cpp
)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

if(TARGET OpenGL::GL)
    target_link_libraries(opengl_basic_triangle PRIVATE OpenGL::GL)
else()
    target_link_libraries(opengl_basic_triangle PRIVATE ${OPENGL_LIBRARIES})
endif()

target_link_libraries(opengl_basic_triangle PRIVATE glfw)
target_link_libraries(opengl_basic_triangle PRIVATE GLEW::GLEW)

if(WIN32)
    target_compile_definitions(opengl_basic_triangle PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
