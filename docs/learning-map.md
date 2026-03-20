# GPU 软件栈学习路径（Demo 导向）

## 1) OpenGL 入门：`basic_triangle`

学习目标：

- 理解窗口、上下文、渲染循环
- 理解顶点数据、`VAO/VBO`
- 理解着色器编译、链接与绘制调用

关键调用路径（简化）：

- `glfwInit` / `glfwCreateWindow`
- `glCreateShader` / `glCompileShader`
- `glCreateProgram` / `glLinkProgram`
- `glGenVertexArrays` / `glGenBuffers`
- `glDrawArrays`

## 2) 未来可扩展的小 Demo（建议）

### OpenGL

- 纹理采样与 `uniform`
- 摄像机与矩阵变换
- 离屏渲染（FBO）

### OpenGL ES

- 在 ES 3.0 下复刻三角形 demo
- 对比桌面 OpenGL 与 ES 的功能差异

### Vulkan

- 实例/设备/交换链初始化
- 管线与命令缓冲最小闭环

### EGL

- 独立于某些窗口系统的上下文创建
- 与 OpenGL ES 配合进行离屏渲染

## 3) 建议学习顺序

1. OpenGL `basic_triangle`
2. OpenGL 纹理 + 变换
3. EGL 上下文基础
4. OpenGL ES 迁移
5. Vulkan 最小三角形
