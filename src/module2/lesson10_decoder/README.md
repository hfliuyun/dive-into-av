# 第10节：FFmpeg 解码器 API 入门

## 简介

本项目实现了一个最简视频解码器，使用 FFmpeg API 将 MP4 文件解码为 YUV 帧。

## 环境配置

### macOS

```bash
brew install ffmpeg cmake
```

### Fedora

```bash
sudo dnf install ffmpeg-devel cmake gcc-c++
```

### Ubuntu/Debian

```bash
sudo apt install libavcodec-dev libavformat-dev libavutil-dev cmake g++
```

### 验证安装

```bash
pkg-config --cflags --libs libavcodec libavformat libavutil
```

## 编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置（Release 模式）
cmake ..

# 或配置（Debug 模式，启用 ASAN）
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 编译
make
```

## 运行

```bash
# 基本用法：解码并输出帧信息
./decoder input.mp4

# 保存 YUV 数据到文件
./decoder input.mp4 output.yuv
```

## 验证输出

```bash
# 用 ffplay 播放 YUV 文件
ffplay -video_size 1280x720 -pixel_format yuv420p output.yuv

# 用 ffmpeg 转换为 MP4
ffmpeg -video_size 1280x720 -pixel_format yuv420p -i output.yuv -c:v libx264 output.mp4
```

## 预期输出

```text
视频信息:
  分辨率: 1280x720
  像素格式: yuv420p
  解码器: h264

帧 1: PTS=0, 尺寸=1280x720, YUV大小=1382400 字节
帧 2: PTS=1000, 尺寸=1280x720, YUV大小=1382400 字节
帧 3: PTS=2000, 尺寸=1280x720, YUV大小=1382400 字节
...

总共解码 150 帧
```

## 代码说明

- `avformat_open_input()`：打开输入文件
- `avformat_find_stream_info()`：查找流信息
- `avcodec_find_decoder()`：查找解码器
- `avcodec_alloc_context3()`：创建解码器上下文
- `avcodec_parameters_to_context()`：复制参数到上下文
- `avcodec_open2()`：打开解码器
- `avcodec_send_packet()`：发送包到解码器
- `avcodec_receive_frame()`：从解码器接收帧

## 内存管理

FFmpeg 内存管理必须严格遵循以下规则：

- `av_packet_alloc()` / `av_packet_free()` 必须配对
- `av_frame_alloc()` / `av_frame_free()` 必须配对
- 循环中使用 `av_packet_unref()` / `av_frame_unref()` 清空对象
- 所有返回值必须检查，失败时输出错误信息

## 常见问题

### 找不到 FFmpeg 开发库

```bash
# 检查是否安装
pkg-config --cflags --libs libavcodec

# macOS
brew install ffmpeg

# Fedora
sudo dnf install ffmpeg-devel

# Ubuntu
sudo apt install libavcodec-dev
```

### 编译错误 "undefined reference"

检查 CMakeLists.txt 中的链接库顺序。

### 运行时错误 "段错误"

启用 ASAN 进行调试：

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
./decoder input.mp4
```
