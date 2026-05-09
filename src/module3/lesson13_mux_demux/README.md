# 第13节：解封装与封装 API 详解

## 简介

本项目演示了 FFmpeg 的解封装与封装 API，将 H.264 裸流打包为 MP4 容器。

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

## 准备测试素材

```bash
# 生成测试视频（5秒，640x360）
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=25 \
       -f lavfi -i sine=frequency=440:duration=5 \
       -c:v libx264 -c:a aac -shortest -y test.mp4

# 提取 H.264 裸流
ffmpeg -i test.mp4 -c:v copy -an test.h264
```

## 运行

```bash
# 将 H.264 裸流封装为 MP4
./mux test.h264 output.mp4
```

## 验证输出

```bash
# 用 ffprobe 分析封装后的文件
ffprobe output.mp4

# 用 ffplay 播放封装后的文件
ffplay output.mp4
```

## 预期输出

```text
输入信息:
  格式: h264
  流数量: 1
  流 0: video (h264)

开始封装...
  输入: test.h264
  输出: output.mp4

封装完成: 125 个包
输出文件: output.mp4
验证命令: ffprobe output.mp4
```

## 关键概念

### AVFormatContext

AVFormatContext 是 FFmpeg 的格式上下文，包含容器的所有信息：
- `nb_streams`：流的数量
- `streams`：流数组
- `duration`：总时长
- `bit_rate`：总码率

### 解封装流程

```
打开输入 → 查找流信息 → 读取包 → 关闭输入
```

### 封装流程

```
创建输出 → 添加流 → 写入头 → 写入包 → 写入尾 → 关闭输出
```

### 时间基转换

不同流可能有不同的时间基，需要使用 `av_rescale_q()` 进行转换：

```c
packet->pts = av_rescale_q(packet->pts, in_stream->time_base, out_stream->time_base);
```

## 常见问题

### 打开输出文件失败

检查输出文件路径是否正确，是否有写入权限。

### 写入文件头失败

检查输出格式是否支持，编解码参数是否正确。

### 封装后播放异常

检查时间基转换是否正确，时间戳是否连续。

### 封装后音视频不同步

确保音频和视频都正确转换时间基。
