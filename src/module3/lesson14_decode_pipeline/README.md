# 第14节：解码管道构建

## 简介

本项目演示了 FFmpeg 解码管道的构建，将 MP4 文件解码为 YUV 和 PCM 文件。

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

### macOS 用户在 Debug 模式下运行

由于 macOS 系统的 ASAN 默认不启用内存泄漏检测，需要先设置环境变量：

```bash
export ASAN_OPTIONS=detect_leaks=1
```

### Linux 用户

ASAN 通常在 Debug 模式下自动生效，无需额外配置。若没有检测到泄漏，可以设置：

```bash
export ASAN_OPTIONS=detect_leaks=1
```

## 准备测试素材

```bash
# 生成测试视频（5秒，640x360）
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=25 \
       -f lavfi -i sine=frequency=440:duration=5 \
       -c:v libx264 -c:a aac -shortest -y test.mp4
```

## 运行

```bash
# 解码为 YUV 和 PCM
./decode_pipeline test.mp4 -y output.yuv -p output.pcm

# 只解码视频
./decode_pipeline test.mp4 -y output.yuv

# 只解码音频
./decode_pipeline test.mp4 -p output.pcm
```

## 验证输出

```bash
# 播放 YUV 文件
ffplay -video_size 640x360 -pixel_format yuv420p output.yuv

# 播放 PCM 文件
ffplay -f s16le -channels 2 -sample_rate 44100 output.pcm
```

## 预期输出

```text
解码管道信息:
  视频: 640x360 (h264)
  音频: 44100Hz, 2ch (aac)

--- flush 解码器 ---

解码完成:
  视频帧: 125
  音频帧: 216
  YUV 文件: output.yuv
  验证命令: ffplay -video_size 640x360 -pixel_format yuv420p output.yuv
  PCM 文件: output.pcm
  验证命令: ffplay -f s16le -channels 2 -sample_rate 44100 output.pcm
```

## 关键概念

### 解码管道数据流转

```
MP4 → 解封装 → AVPacket → 解码 → AVFrame → 后处理 → YUV/PCM
```

### 音视频同步读取

交错读取音频和视频包，按 stream_index 区分：

```c
while (av_read_frame(fmt_ctx, packet) >= 0) {
    if (packet->stream_index == video_idx) {
        // 解码视频包
    } else if (packet->stream_index == audio_idx) {
        // 解码音频包
    }
    av_packet_unref(packet);
}
```

## 常见问题

### YUV 文件播放异常

检查 ffplay 参数是否正确，确认视频分辨率和像素格式。

### PCM 文件播放异常

检查 ffplay 参数是否正确，确认音频采样格式、声道数和采样率。

### 解码后音视频不同步

检查是否按 PTS 排序输出，确认时间基转换是否正确。
