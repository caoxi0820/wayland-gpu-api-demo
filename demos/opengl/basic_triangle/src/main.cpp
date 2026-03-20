#include <iostream>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// 编译单个着色器（顶点着色器 / 片段着色器）。
// 失败时打印日志，便于学习定位 GLSL 语法或版本问题。
static GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return shader;
}

// 将已编译的顶点与片段着色器链接成可执行的 GPU Program。
static GLuint CreateProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '\0');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        std::cerr << "Program link error:\n" << log << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

int main(int argc, char** argv) {
    // 可选参数：--max-frames N
    // 用于 CI 或自动化验证，渲染 N 帧后自动退出。
    int maxFrames = -1;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--max-frames" && i + 1 < argc) {
            try {
                maxFrames = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Invalid value for --max-frames." << std::endl;
                return 1;
            }
        }
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return 1;
    }

    // 申请 OpenGL 3.3 Core 上下文。
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(960, 540, "OpenGL Basic Triangle", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return 1;
    }

    // 当前线程绑定窗口上下文，后续 OpenGL 调用才有意义。
    glfwMakeContextCurrent(window);

    // 初始化 OpenGL 函数加载器（GLEW）。
    // 在 Core Profile 下，大量函数地址需要运行时查询。
    glewExperimental = GL_TRUE;
    const GLenum glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: "
                  << reinterpret_cast<const char*>(glewGetErrorString(glewInitResult))
                  << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // 某些平台 glewInit 可能产生一个无害错误，这里清空错误状态。
    while (glGetError() != GL_NO_ERROR) {}

    std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << '\n'
              << "OpenGL version : " << glGetString(GL_VERSION) << std::endl;

    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 inPosition;
        layout(location = 1) in vec3 inColor;

        out vec3 fragColor;

        void main() {
            gl_Position = vec4(inPosition, 1.0);
            fragColor = inColor;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 fragColor;
        out vec4 outColor;

        void main() {
            outColor = vec4(fragColor, 1.0);
        }
    )";

    const std::vector<float> vertices = {
         0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f
    };

    // 创建并绑定 VAO/VBO：
    // - VBO 保存顶点数据
    // - VAO 记录“如何解释这些顶点数据”
    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // 顶点布局：location 0 = vec3 位置，location 1 = vec3 颜色。
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint program = CreateProgram(vertexShaderSource, fragmentShaderSource);

    // =========================
    // 主渲染循环（每一帧）
    // =========================
    int frameCount = 0;
    while (!glfwWindowShouldClose(window)) {
        // 1) 处理输入与窗口事件
        glfwPollEvents();

        // 2) 同步窗口尺寸到 OpenGL 视口
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        // 3) 清屏
        glClearColor(0.08f, 0.1f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 4) 绑定 Program + VAO 发起绘制
        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 5) 前后缓冲交换，把这一帧显示出来
        glfwSwapBuffers(window);

        ++frameCount;
        if (maxFrames > 0 && frameCount >= maxFrames) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    // 释放 GPU 和窗口资源。
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
