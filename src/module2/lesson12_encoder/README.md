# 第12节：FFmpeg 编码器 API 入门

## 简介

本项目实现了一个最简视频编码器，使用 FFmpeg API 将 YUV 序列编码为 H.264 文件。

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
# 检查 H.264 编码器是否可用
ffmpeg -encoders | grep h264

# 检查 FFmpeg 开发库
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

## 准备测试素材

```bash
# 生成测试视频（5秒，640x360）
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=25 \
       -c:v libx264 -y test.mp4

# 提取 YUV 数据
ffmpeg -i test.mp4 -pix_fmt yuv420p -s 640x360 test.yuv
```

## 运行

```bash
# 编码 YUV 为 H.264
./encoder test.yuv test.h264
```

## 验证输出

```bash
# 用 ffplay 播放编码后的 H.264 文件
ffplay test.h264

# 用 ffprobe 分析编码后的文件
ffprobe test.h264

# 用 ffmpeg 转换为 MP4
ffmpeg -i test.h264 -c copy output.mp4
```

## 预期输出

```text
编码参数:
  分辨率: 640x360
  帧率: 25/1
  码率: 500000 bps
  GOP: 10

--- flush 编码器 ---

编码完成: 125 帧
输出文件: test.h264
验证命令: ffplay test.h264
```

## 编码参数说明

### CRF（恒定质量因子）

| CRF | 质量 | 应用场景 |
|-----|------|----------|
| 0 | 无损 | 专业视频制作 |
| 18-23 | 高质量 | 网络视频 |
| 23-28 | 中等质量 | 移动设备 |
| 28-51 | 低质量 | 低带宽 |

### Preset（编码速度预设）

| Preset | 速度 | 压缩率 | 应用场景 |
|--------|------|--------|----------|
| ultrafast | 最快 | 最低 | 实时编码 |
| fast | 快 | 低 | 快速转码 |
| medium | 中等 | 中等 | 默认 |
| slow | 慢 | 高 | 高质量转码 |
| veryslow | 最慢 | 最高 | 专业视频制作 |

### Profile（编码配置）

| Profile | 特点 | 应用场景 |
|---------|------|----------|
| Baseline | 无 B 帧，无 CABAC | 移动设备 |
| Main | 有 B 帧，有 CABAC | 网络视频 |
| High | 最高压缩率 | 高清视频 |

## 代码说明

- `avcodec_find_encoder()`：查找编码器
- `avcodec_alloc_context3()`：创建编码器上下文
- `av_opt_set()`：设置编码器私有参数
- `avcodec_open2()`：打开编码器
- `avcodec_send_frame()`：发送帧到编码器
- `avcodec_receive_packet()`：从编码器接收包

## 常见问题

### 找不到 H.264 编码器

```bash
# 检查 FFmpeg 是否支持 H.264 编码
ffmpeg -encoders | grep h264

# 安装 libx264 开发库
# macOS
brew install x264

# Fedora
sudo dnf install x264-devel

# Ubuntu
sudo apt install libx264-dev
```

### 编码失败 "Invalid argument"

检查编码参数：
- 像素格式是否支持
- 分辨率是否为偶数
- 时间基准是否正确

### 输出文件为空

确保正确 flush 编码器：
```cpp
avcodec_send_frame(codec_ctx, NULL);
while (avcodec_receive_packet(codec_ctx, packet) >= 0) {
    fwrite(packet->data, 1, packet->size, h264_file);
    av_packet_unref(packet);
}
```
