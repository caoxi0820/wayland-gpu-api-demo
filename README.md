# wayland-gpu-api-demo

一个用于学习 GPU 软件栈的实验仓库，目标是按主题拆分多个小 demo。

## 当前已实现

- `OpenGL`：基础三角形渲染（`GLFW` + `OpenGL 3.3 Core`）
- `EGL`：离屏渲染（`EGL` + `OpenGL 3.3 Core`，无窗口，输出 PPM 图片）

## 目录结构

- `demos/opengl/basic_triangle`：OpenGL 入门 demo（GLFW 管理窗口和上下文）
- `demos/opengl/egl_offscreen`：EGL 离屏渲染 demo（手动创建上下文，无窗口）
- `docs/learning-map.md`：学习路径与各 demo 目标

## 快速构建（示例）

> 说明：此仓库偏学习用途。当前 OpenGL demo 已按 Linux 环境配置，安装依赖后可编译运行。

### Linux (Ubuntu/Debian)

先安装依赖：

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libglfw3-dev libglew-dev libgl1-mesa-dev libegl1-mesa-dev
```

构建与运行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# 运行 basic_triangle（窗口渲染）
./build/demos/opengl/basic_triangle/opengl_basic_triangle

# 运行 egl_offscreen（离屏渲染，生成 egl_offscreen_output.ppm）
./build/demos/opengl/egl_offscreen/opengl_egl_offscreen
```

用于自动验证（例如 CI）可执行：

```bash
./build/demos/opengl/basic_triangle/opengl_basic_triangle --max-frames 120
```

仓库已添加 Linux 自动验证工作流：

- `.github/workflows/linux-opengl-demo.yml`（Ubuntu + xvfb，执行构建并实际启动 demo）

### Windows (Visual Studio / Ninja)

```bash
cmake -S . -B build
cmake --build build --config Release
```

可执行文件目标名：`opengl_basic_triangle`、`opengl_egl_offscreen`

## 下一步建议

1. 增加 `OpenGL` 第二个 demo：索引缓冲、纹理、矩阵变换。
2. 增加 `OpenGL ES` demo：尽量复用渲染逻辑，对比 API 差异。
3. 增加 `Vulkan` 最小三角形 demo，建立 pipeline 概念。
