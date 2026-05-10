# 第19节：SDL3 渲染基础 示例工程

本工程演示了如何使用最新的 SDL3 库来渲染一张静态的 YUV420P 图像，并展示了 SDL3 的显式色彩空间管理特性。

## 环境准备 (SDL3 安装)

### macOS
```bash
brew install sdl3
```

### Ubuntu (源码安装)
```bash
git clone https://github.com/libsdl-org/SDL.git
cd SDL
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

## 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 准备测试素材
使用 FFmpeg 提取一帧 640x360 的 YUV420P 像素数据：
```bash
ffmpeg -i your_video.mp4 -vframes 1 -f rawvideo -pix_fmt yuv420p -s 640x360 test.yuv
```

## 运行
确保 `test.yuv` 位于当前运行目录下。
```bash
./lesson19_sdl_render
```

## 核心 API
- `SDL_Init(SDL_INIT_VIDEO)`: 初始化视频子系统。
- `SDL_CreateWindowAndRenderer`: 一步创建窗口和渲染器。
- `SDL_CreateTexture`: 创建纹理，使用 `SDL_PIXELFORMAT_IYUV` 格式。
- `SDL_SetTextureColorspace`: **(SDL3 特有)** 设置纹理的色彩空间（如 BT.709）。
- `SDL_UpdateYUVTexture`: 更新 YUV 平面数据到纹理。
- `SDL_RenderTexture`: **(SDL3 特有)** 将纹理拷贝到渲染目标（原 `SDL_RenderCopy`）。
