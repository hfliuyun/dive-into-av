# 第19节：SDL3 渲染基础

## 本节能力目标
- **掌握 SDL3 环境搭建**：学会如何在开发环境中集成最新的 SDL3 库。
- **理解 SDL3 渲染管线**：掌握 Window、Renderer 和 Texture 之间的层级关系。
- **显式色彩空间管理**：理解 SDL3 相比 SDL2 的重大改进，学会显式设置纹理色彩空间。
- **YUV 画面渲染**：能够将原始 YUV420P 数据更新到 SDL 纹理并正确显示。

## 真实工程对应场景
- **播放器渲染引擎**：所有主流播放器（如 IJKPlayer, FFplay, VLC）的渲染层本质上都是将解码后的 YUV 数据映射到 GPU 纹理进行显示。
- **调试监控工具**：在开发编解码算法时，需要一个轻量级的工具来实时观察输出的原始像素。

## 引言
欢迎来到模块四！从本章开始，我们将从“离线处理”转入“实时显示”。

为了紧跟技术前沿，本课程决定采用 **SDL3**（Simple DirectMedia Layer 3）。SDL3 是 SDL 库的最新大版本，它对多媒体开发者最友好的改进就是引入了**显式的色彩空间（Colorspace）管理**。在 SDL2 时代，YUV 到 RGB 的转换通常依赖全局设置，容易导致高清（BT.709）和标清（BT.601）视频色彩还原不准确。而在 SDL3 中，你可以为每一个纹理精准指定色彩空间，从而获得完美的视觉还原。

## 理论讲解

### 1. SDL3 核心对象模型
在 SDL3 中渲染画面的基本流程如下：
- **SDL_Window**: 操作系统层面的窗口实例。
- **SDL_Renderer**: 绑定在窗口上的渲染上下文，负责执行具体的绘图命令。
- **SDL_Texture**: 存储在 GPU 显存中的图像数据。

关系：`Window` 拥有 `Renderer`，`Renderer` 负责将 `Texture` 拷贝到 `Window` 的后台缓冲区。

### 2. YUV 纹理更新
对于视频播放，我们通常创建 `SDL_PIXELFORMAT_IYUV` (即 YUV420P) 格式的纹理。
SDL3 提供了 `SDL_UpdateYUVTexture` 函数，它接收三个平面（Y, U, V）的指针和步长（Pitch），直接将数据推送到显存。

### 3. 色彩空间显式绑定（SDL3 特性）
这是 SDL3 的核心优势。通过 `SDL_SetTextureColorspace`，我们可以明确告诉渲染器：
- “这段数据是 BT.709（高清标准）的有限量程数据。”
- 或者 “这段数据是 BT.601（标清标准）的全量程数据。”
这样，GPU 在进行 YUV -> RGB 矩阵运算时，会自动选用正确的公式。
- **提示**：如果渲染出的画面看起来发灰（Limited vs Full 量程不匹配）或颜色偏绿/偏红（BT.601 vs BT.709 不匹配），通常只需切换该设置即可。

## 关键点总结
- **SDL3 优于 SDL2**：更好的色彩空间支持、更现代的 API 设计（如 `bool` 返回值）。
- **渲染三部曲**：Clear（清空） -> Copy（拷贝纹理） -> Present（呈现）。
- **Event Loop**：必须处理事件循环，否则窗口会因未响应系统请求而卡死。

## 环境与素材准备

### 环境要求
- **macOS**: `brew install sdl3`
- **Ubuntu**: 建议从 [SDL GitHub](https://github.com/libsdl-org/SDL) 下载源码并编译安装。
- **Windows**: 推荐使用 vcpkg: `vcpkg install sdl3`。

### 测试素材
使用 FFmpeg 提取一帧 YUV：
```bash
ffmpeg -i input.mp4 -vframes 1 -f rawvideo -pix_fmt yuv420p -s 640x360 test.yuv
```

## 核心代码实验

### 实验1：显示一张静态 YUV 图像
**目标：** 实现一个简单的 C++ 程序，读取 `test.yuv` 并弹窗显示 5 秒。

**核心逻辑解析：**
- **初始化**：`SDL_Init(SDL_INIT_VIDEO)`。
- **创建**：`SDL_CreateWindowAndRenderer` 一步到位创建窗口和渲染器。
- **设置色彩空间**：
  ```cpp
  SDL_SetTextureColorspace(texture, SDL_COLORSPACE_BT709_LIMITED);
  ```
- **事件处理**：
  ```cpp
  SDL_Event event;
  while (running) {
      while (SDL_PollEvent(&event)) {
          if (event.type == SDL_EVENT_QUIT) running = false;
      }
      // ... render logic
  }
  ```

## 预期结果与验收标准

### 预期输出
- 弹出一个标题为 "SDL3 YUV Renderer" 的窗口。
- 窗口中正确显示图像，色彩自然（无发白或偏绿现象）。

### 验收标准
- 程序能成功加载 SDL3 动态库并运行。
- 能够通过窗口关闭按钮正常退出，无崩溃。
- 使用 ASAN 检查无内存泄漏。

## 常见错误与排查
- **窗口秒闪就消失**：检查是否实现了事件循环（Event Loop）。
- **色彩偏红或偏蓝**：检查 `U` 和 `V` 分量的指针是否传反了。
- **图像发白**：检查色彩空间设置是否匹配（如 BT.709 vs BT.601）。

## 课后小挑战
1. **支持窗口缩放**：修改创建纹理的参数，尝试在窗口大小改变时保持图像比例。
2. **多帧预览**：修改代码，支持读取包含多帧的 YUV 文件，并按固定频率切换显示。

## 面试延伸题
- **问**：SDL3 为什么要取消全局的 `SDL_SetYUVConversionMode`？
- **答**：因为在复杂的现代应用中，可能同时渲染多个视频流（如画中画）。全局设置无法处理不同流采用不同标准（如一个 BT.601，一个 BT.709）的情况。将色彩空间绑定在纹理上（Per-texture basis）是更符合 GPU 渲染管线的正确设计。

## 延伸阅读
- [SDL3 Wiki](https://wiki.libsdl.org/SDL3/FrontPage)
- [SDL3 YUV Rendering Guide](https://wiki.libsdl.org/SDL3/Tutorials-YUV)
