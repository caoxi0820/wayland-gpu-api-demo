# wayland-gpu-api-demo

一个用于学习 GPU 软件栈的实验仓库，目标是按主题拆分多个小 demo。

## 当前已实现

- `OpenGL`：基础三角形渲染（`GLFW` + `OpenGL 3.3 Core`）
- `EGL`：离屏渲染（`EGL` + `OpenGL 3.3 Core`，无窗口，输出 PPM 图片）

## 目录结构

- `demos/opengl/basic_triangle`：OpenGL 入门 demo（GLFW 管理窗口和上下文）
- `demos/opengl/egl_offscreen`：EGL 离屏渲染 demo（手动创建上下文，无窗口）
- `docs/learning-map.md`：学习路径与各 demo 目标

## 构建说明

### 前置条件

- CMake >= 3.16
- C++17 编译器（GCC 7+、Clang 5+、MSVC 2017+）
- pkg-config（Linux，用于查找 EGL）

### Linux (Ubuntu/Debian)

#### 1. 安装系统依赖

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libglfw3-dev \
    libglew-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev
```

各依赖包的作用：

| 包名 | 提供什么 | 哪个 demo 用到 |
|------|---------|---------------|
| `build-essential` | GCC 编译器、make 等基础构建工具 | 全部 |
| `cmake` | CMake 构建系统 | 全部 |
| `pkg-config` | 库查找工具，CMake 的 `pkg_check_modules` 依赖它 | egl_offscreen |
| `libglfw3-dev` | GLFW 窗口/上下文管理库的头文件和链接库 | basic_triangle |
| `libglew-dev` | GLEW OpenGL 函数加载器的头文件和链接库 | 全部 |
| `libgl1-mesa-dev` | Mesa 的 `libGL.so`（OpenGL 实现）和头文件 | 全部 |
| `libegl1-mesa-dev` | Mesa 的 `libEGL.so`（EGL 实现）和头文件 | egl_offscreen |

#### 2. CMake 配置（Configure）

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

这一步做了什么：
- `-S .` 指定源码目录为当前目录（读取顶层 `CMakeLists.txt`）
- `-B build` 指定构建输出目录为 `build/`（out-of-source 构建，不污染源码）
- `-DCMAKE_BUILD_TYPE=Release` 开启编译优化（`-O2`），去掉调试符号
- CMake 会递归处理 `add_subdirectory`，依次配置 `basic_triangle` 和 `egl_offscreen`
- 每个子目录的 `CMakeLists.txt` 会通过 `find_package` / `pkg_check_modules` 查找系统上的 OpenGL、GLEW、GLFW、EGL 库
- 如果某个库找不到，CMake 会报错并提示缺少哪个包

可选的 CMake 选项：

```bash
# 只构建 OpenGL 相关 demo（默认 ON）
cmake -S . -B build -DBUILD_OPENGL_DEMOS=ON

# 跳过 OpenGL demo
cmake -S . -B build -DBUILD_OPENGL_DEMOS=OFF
```

#### 3. 编译（Build）

```bash
cmake --build build -j
```

这一步做了什么：
- `cmake --build build` 在 `build/` 目录下执行实际编译（底层调用 `make` 或 `ninja`）
- `-j` 启用并行编译（自动使用所有 CPU 核心）
- 编译器会编译每个 demo 的 `src/main.cpp`，然后链接对应的库
- 生成的可执行文件位于：
  - `build/demos/opengl/basic_triangle/opengl_basic_triangle`
  - `build/demos/opengl/egl_offscreen/opengl_egl_offscreen`

#### 4. 运行

```bash
# basic_triangle：打开一个窗口，显示彩色三角形
# 按窗口关闭按钮或 Ctrl+C 退出
./build/demos/opengl/basic_triangle/opengl_basic_triangle

# egl_offscreen：无窗口，离屏渲染一帧并保存为 PPM 图片
# 运行后在当前目录生成 egl_offscreen_output.ppm
./build/demos/opengl/egl_offscreen/opengl_egl_offscreen
```

#### 5. CI / 无头环境运行

在没有显示器的环境（CI、远程服务器）下，basic_triangle 需要虚拟帧缓冲：

```bash
# 安装 xvfb
sudo apt install -y xvfb

# 通过 xvfb 运行，--max-frames 120 表示渲染 120 帧后自动退出
xvfb-run -s "-screen 0 1280x720x24" \
    ./build/demos/opengl/basic_triangle/opengl_basic_triangle --max-frames 120
```

egl_offscreen 本身不需要窗口系统，在无头环境下可以直接运行（Mesa 会自动回退到 llvmpipe 软件渲染）。

仓库已配置 GitHub Actions 工作流：
- `.github/workflows/linux-opengl-demo.yml`（Ubuntu + xvfb，自动构建并运行 demo）

### Windows (Visual Studio / Ninja)

```bash
cmake -S . -B build
cmake --build build --config Release
```

可执行文件目标名：`opengl_basic_triangle`、`opengl_egl_offscreen`

> 注意：`egl_offscreen` 在 Windows 上需要支持 EGL 的驱动（如 ANGLE 或 Mesa for Windows），原生 Windows 环境下可能无法直接编译。

### 常见问题

**Q: CMake 报 `Could NOT find GLEW`**
A: 安装 `libglew-dev`：`sudo apt install libglew-dev`

**Q: CMake 报 `No package 'egl' found`**
A: 安装 `libegl1-mesa-dev`：`sudo apt install libegl1-mesa-dev`，并确保 `pkg-config` 已安装。

**Q: 运行 basic_triangle 报 `Failed to create GLFW window`**
A: 可能没有可用的显示环境。在无头环境下用 `xvfb-run` 运行，或者 SSH 时确保 X11 转发已开启。

**Q: egl_offscreen 运行后 `eglGetDisplay failed`**
A: 检查是否安装了 `libegl1-mesa0`（运行时库）。在某些最小化系统上可能需要额外安装。

## 下一步建议

1. 增加 `OpenGL` 第二个 demo：索引缓冲、纹理、矩阵变换。
2. 增加 `OpenGL ES` demo：尽量复用渲染逻辑，对比 API 差异。
3. 增加 `Vulkan` 最小三角形 demo，建立 pipeline 概念。
