// =============================================================================
// EGL 离屏渲染 Demo
// =============================================================================
//
// 这个 demo 展示了如何：
//   1. 用 EGL（而非 GLFW/GLX）手动创建 OpenGL 上下文
//   2. 不创建任何窗口，完全离屏（offscreen）渲染
//   3. 用 FBO + Renderbuffer 作为渲染目标
//   4. 用 glReadPixels 把 GPU 渲染结果读回 CPU
//   5. 保存为 PPM 图片文件
//
// 对比 basic_triangle demo：
//   - basic_triangle 用 GLFW 管理窗口和上下文 → 这里用 EGL 手动管理
//   - basic_triangle 渲染到屏幕（窗口） → 这里渲染到内存（FBO）
//   - basic_triangle 需要显示器/窗口系统 → 这里可以在无头服务器上跑
//
// 调用链对比：
//   basic_triangle:  你的代码 → GLFW → GLX/EGL(隐式) → Mesa → GPU
//   本 demo:         你的代码 → EGL(显式) → Mesa → GPU
//
// =============================================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>

// EGL 头文件 —— 上下文创建与平台抽象层
// 这是本 demo 的核心：直接控制 OpenGL 上下文的生命周期
#include <EGL/egl.h>

// GLEW —— OpenGL 函数指针加载器
// Core Profile 下，glCreateShader 等函数地址需要运行时查询
#include <GL/glew.h>

// =============================================================================
// 辅助函数：检查 EGL 错误
// =============================================================================
static bool CheckEGL(const char* step) {
    EGLint err = eglGetError();
    if (err != EGL_SUCCESS) {
        std::cerr << "EGL error at [" << step << "]: 0x"
                  << std::hex << err << std::dec << std::endl;
        return false;
    }
    return true;
}

// =============================================================================
// 辅助函数：编译着色器（和 basic_triangle 里一样）
// =============================================================================
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

// =============================================================================
// 辅助函数：链接着色器程序
// =============================================================================
static GLuint CreateProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '\0');
        glGetProgramInfoLog(prog, length, nullptr, log.data());
        std::cerr << "Program link error:\n" << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// =============================================================================
// 辅助函数：保存像素数据为 PPM 图片
// PPM 是最简单的图片格式，纯文本头 + 原始 RGB 数据，方便验证渲染结果
// =============================================================================
static bool SavePPM(const char* filename, int width, int height,
                    const std::vector<unsigned char>& pixels) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing." << std::endl;
        return false;
    }
    // PPM 文件头：P6 表示二进制 RGB，然后是宽、高、最大色值
    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
    file.close();
    std::cout << "Saved rendered image to: " << filename << std::endl;
    return true;
}

// =============================================================================
// main
// =============================================================================
int main() {
    const int WIDTH  = 640;
    const int HEIGHT = 480;

    // =========================================================================
    // 第一步：获取 EGL Display
    // =========================================================================
    // EGL Display 是 EGL 的入口点，类似于 X11 的 Display* 或 Wayland 的 wl_display。
    // EGL_DEFAULT_DISPLAY 表示使用系统默认的显示设备。
    // 在有 GPU 的机器上，这会连接到 GPU 驱动。
    // 在无头服务器上，Mesa 会回退到软件渲染（llvmpipe）。
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        std::cerr << "eglGetDisplay failed — no display available." << std::endl;
        return 1;
    }

    // =========================================================================
    // 第二步：初始化 EGL
    // =========================================================================
    // 类似于 glfwInit()，但这里是 EGL 层面的初始化。
    // major/minor 返回 EGL 版本号。
    EGLint major = 0, minor = 0;
    if (!eglInitialize(display, &major, &minor)) {
        CheckEGL("eglInitialize");
        return 1;
    }
    std::cout << "EGL version: " << major << "." << minor << std::endl;

    // 打印 EGL 实现信息 —— 可以看到底层用的是什么驱动
    std::cout << "EGL vendor : " << eglQueryString(display, EGL_VENDOR) << std::endl;
    std::cout << "EGL APIs   : " << eglQueryString(display, EGL_CLIENT_APIS) << std::endl;

    // =========================================================================
    // 第三步：选择 EGL 配置（EGLConfig）
    // =========================================================================
    // EGLConfig 描述了帧缓冲的格式：颜色位数、深度位数、是否支持某种 API 等。
    // 这一步相当于 GLFW 内部帮你做的 glXChooseFBConfig / eglChooseConfig。
    //
    // 关键属性：
    //   EGL_SURFACE_TYPE = EGL_PBUFFER_BIT  → 我们要离屏渲染，不需要窗口
    //   EGL_RENDERABLE_TYPE = EGL_OPENGL_BIT → 我们要用桌面 OpenGL（不是 ES）
    //   EGL_RED/GREEN/BLUE_SIZE = 8         → 每通道 8 位，标准 RGB
    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,    // 离屏渲染（PBuffer surface）
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,     // 桌面 OpenGL（不是 GLES）
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      24,                 // 24 位深度缓冲
        EGL_NONE                                 // 属性列表结束标记
    };

    EGLConfig config;
    EGLint numConfigs = 0;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) || numConfigs == 0) {
        CheckEGL("eglChooseConfig");
        eglTerminate(display);
        return 1;
    }
    std::cout << "EGL configs found: " << numConfigs << std::endl;

    // =========================================================================
    // 第四步：创建 PBuffer Surface
    // =========================================================================
    // Surface 是 EGL 中的渲染目标。有三种类型：
    //   - Window Surface  → 渲染到窗口（需要窗口系统）
    //   - PBuffer Surface → 渲染到离屏缓冲（不需要窗口）← 我们用这个
    //   - Pixmap Surface  → 渲染到像素图（较少使用）
    //
    // 对比 GLFW：glfwCreateWindow 内部创建的是 Window Surface。
    const EGLint pbufferAttribs[] = {
        EGL_WIDTH,  WIDTH,
        EGL_HEIGHT, HEIGHT,
        EGL_NONE
    };

    EGLSurface surface = eglCreatePbufferSurface(display, config, pbufferAttribs);
    if (surface == EGL_NO_SURFACE) {
        CheckEGL("eglCreatePbufferSurface");
        eglTerminate(display);
        return 1;
    }
    std::cout << "PBuffer surface created: " << WIDTH << "x" << HEIGHT << std::endl;

    // =========================================================================
    // 第五步：绑定 OpenGL API
    // =========================================================================
    // EGL 可以管理多种渲染 API（OpenGL、OpenGL ES、OpenVG）。
    // 这里告诉 EGL：接下来创建的上下文是桌面 OpenGL 的。
    // 如果你要用 OpenGL ES，就改成 EGL_OPENGL_ES_API。
    if (!eglBindAPI(EGL_OPENGL_API)) {
        CheckEGL("eglBindAPI");
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return 1;
    }

    // =========================================================================
    // 第六步：创建 OpenGL 上下文
    // =========================================================================
    // 这是最关键的一步 —— 创建 GPU 可以执行命令的上下文。
    // 对比：
    //   GLFW:  glfwCreateWindow 内部调用 glXCreateContextAttribsARB 或 eglCreateContext
    //   这里:  我们直接调用 eglCreateContext
    //
    // EGL_CONTEXT_MAJOR_VERSION / MINOR_VERSION 指定 OpenGL 版本。
    // EGL_CONTEXT_OPENGL_PROFILE_MASK 指定 Core Profile（和 basic_triangle 一致）。
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        CheckEGL("eglCreateContext");
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return 1;
    }

    // =========================================================================
    // 第七步：绑定上下文到当前线程
    // =========================================================================
    // 对比 GLFW 的 glfwMakeContextCurrent(window)。
    // 一个线程同一时刻只能绑定一个上下文。
    // 绑定后，当前线程的所有 OpenGL 调用都会走这个上下文。
    if (!eglMakeCurrent(display, surface, surface, context)) {
        CheckEGL("eglMakeCurrent");
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return 1;
    }
    std::cout << "EGL context bound to current thread." << std::endl;

    // =========================================================================
    // 第八步：初始化 GLEW（和 basic_triangle 一样）
    // =========================================================================
    // 上下文绑定后，才能加载 OpenGL 函数指针。
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "GLEW init failed: "
                  << reinterpret_cast<const char*>(glewGetErrorString(glewErr))
                  << std::endl;
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return 1;
    }
    // 清除 glewInit 可能产生的无害错误
    while (glGetError() != GL_NO_ERROR) {}

    std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL version : " << glGetString(GL_VERSION) << std::endl;

    // =========================================================================
    // 第九步：创建 FBO + Renderbuffer（离屏渲染目标）
    // =========================================================================
    // 虽然 PBuffer 本身就能渲染，但用 FBO 更灵活、更现代。
    // FBO（Framebuffer Object）是 OpenGL 的离屏渲染机制：
    //   - 默认帧缓冲 → 渲染到屏幕/PBuffer
    //   - FBO → 渲染到你指定的纹理或 Renderbuffer
    //
    // Renderbuffer 是一块 GPU 上的图像存储，不能直接采样（和纹理的区别），
    // 但作为渲染目标效率更高。
    GLuint fbo = 0, colorRbo = 0, depthRbo = 0;

    // 创建颜色 Renderbuffer（RGBA8）
    glGenRenderbuffers(1, &colorRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, WIDTH, HEIGHT);

    // 创建深度 Renderbuffer（24 位）
    glGenRenderbuffers(1, &depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, WIDTH, HEIGHT);

    // 创建 FBO 并挂载 Renderbuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);

    // 检查 FBO 是否完整（所有附件格式兼容、尺寸一致）
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "FBO incomplete: 0x" << std::hex << fboStatus << std::dec << std::endl;
        return 1;
    }
    std::cout << "FBO created and complete." << std::endl;

    // =========================================================================
    // 第十步：设置着色器和顶点数据（和 basic_triangle 基本一样）
    // =========================================================================
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 inPosition;
        layout(location = 1) in vec3 inColor;
        out vec3 fragColor;
        void main() {
            gl_Position = vec4(inPosition, 1.0);
            fragColor = inColor;
        }
    )";

    const char* fragSrc = R"(
        #version 330 core
        in vec3 fragColor;
        out vec4 outColor;
        void main() {
            outColor = vec4(fragColor, 1.0);
        }
    )";

    GLuint program = CreateProgram(vertSrc, fragSrc);

    // 三角形顶点数据：位置(xyz) + 颜色(rgb)
    const float vertices[] = {
         0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,  // 顶部 - 红色
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,  // 右下 - 绿色
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,  // 左下 - 蓝色
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // location 0 = 位置 (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // location 1 = 颜色 (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // =========================================================================
    // 第十一步：渲染（和 basic_triangle 的渲染循环类似，但只画一帧）
    // =========================================================================
    // 因为是离屏渲染，没有窗口，不需要循环，画一帧就够了。
    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.08f, 0.1f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // 确保所有 GPU 命令执行完毕
    // glFinish 会阻塞直到 GPU 完成所有排队的命令。
    // 在有窗口的情况下，glfwSwapBuffers 内部会做类似的同步。
    glFinish();
    std::cout << "Rendering complete." << std::endl;

    // =========================================================================
    // 第十二步：读回像素数据（GPU → CPU）
    // =========================================================================
    // glReadPixels 从当前绑定的帧缓冲（我们的 FBO）读取像素。
    // 这是 GPU → CPU 的数据传输，在实际应用中可能是性能瓶颈。
    // 但对于离屏渲染/截图/验证来说，这是标准做法。
    std::vector<unsigned char> pixels(WIDTH * HEIGHT * 3);
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL 的像素坐标原点在左下角，而图片文件通常原点在左上角，
    // 所以需要垂直翻转。
    std::vector<unsigned char> flipped(WIDTH * HEIGHT * 3);
    const int rowBytes = WIDTH * 3;
    for (int y = 0; y < HEIGHT; ++y) {
        const int srcRow = (HEIGHT - 1 - y) * rowBytes;
        const int dstRow = y * rowBytes;
        std::copy(pixels.begin() + srcRow, pixels.begin() + srcRow + rowBytes,
                  flipped.begin() + dstRow);
    }

    SavePPM("egl_offscreen_output.ppm", WIDTH, HEIGHT, flipped);

    // =========================================================================
    // 第十三步：清理资源
    // =========================================================================
    // 清理顺序：OpenGL 资源 → EGL 上下文解绑 → 销毁 EGL 对象
    //
    // 对比 basic_triangle 的清理：
    //   basic_triangle: glDelete* → glfwDestroyWindow → glfwTerminate
    //   本 demo:        glDelete* → eglMakeCurrent(解绑) → eglDestroy* → eglTerminate

    // 清理 OpenGL 资源
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &colorRbo);
    glDeleteRenderbuffers(1, &depthRbo);

    // 解绑当前上下文（让当前线程不再关联任何 GL 上下文）
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    // 销毁 EGL 对象（顺序：上下文 → Surface → Display）
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    std::cout << "All resources cleaned up. Done." << std::endl;
    return 0;
}
