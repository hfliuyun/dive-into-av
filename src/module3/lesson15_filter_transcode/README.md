# 第15节：图像滤镜与转码

## 简介

本项目演示使用 FFmpeg libavfilter 对视频进行滤镜处理并转码，支持缩放、文字水印、翻转等操作。

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
sudo apt install libavcodec-dev libavformat-dev libavutil-dev libavfilter-dev cmake g++
```

### 验证安装

```bash
pkg-config --cflags --libs libavcodec libavformat libavutil libavfilter
```

## 编译

```bash
mkdir build && cd build
cmake ..
make

# Debug 模式 (启用 ASAN):
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### macOS 用户在 Debug 模式下运行

```bash
export ASAN_OPTIONS=detect_leaks=1
```

## 准备测试素材

```bash
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=25        -c:v libx264 -y test.mp4
```

## 运行

```bash
# 缩放
./filter_transcode test.mp4 scaled.mp4 scale 1280:720

# 文字水印
./filter_transcode test.mp4 watermarked.mp4 text Hello:32:10:10

# 水平翻转
./filter_transcode test.mp4 hflipped.mp4 hflip

# 垂直翻转
./filter_transcode test.mp4 vflipped.mp4 vflip

# 自由滤镜
./filter_transcode test.mp4 custom.mp4 "scale=640:360,drawtext=text='AV':fontsize=32:x=10:y=10"
```

## 验证输出

```bash
ffplay scaled.mp4
```

## 预期输出

```text
filter pipeline:
  filter: scale=1280:720
  input:  640x360 (yuv420p)
  output: 1280x720 (yuv420p)

transcode done:
  frames: 125
  output: scaled.mp4
  verify: ffplay scaled.mp4
```

## 关键概念

### 滤镜图 (Filter Graph)

```
解码帧 -> [buffer src] -> [滤镜1] -> [滤镜2] -> ... -> [buffersink] -> 编码器
```

滤镜通过字符串描述，用逗号分隔多个滤镜：

```
scale=1280:720                          # 缩放
drawtext=text='AV':fontsize=32:x=10:y=10 # 文字水印
scale=640:360,drawtext=text='AV'         # 先缩小再加水印
```

### 滤镜图构建流程

1. avfilter_graph_alloc() — 分配滤镜图
2. avfilter_graph_create_filter() — 创建 buffer src 和 buffersink
3. avfilter_graph_parse_ptr() — 解析滤镜描述字符串，生成中间节点
4. avfilter_graph_config() — 验证并完成滤镜图配置
5. avfilter_graph_free() — 释放整个滤镜图

## 常见问题

### drawtext 滤镜找不到字体

macOS 默认无字体文件目录，可安装 fontconfig：

```bash
brew install fontconfig
```

或使用简单滤镜如 scale、hflip、vflip。

### 编码器 libx264 未找到

```bash
brew install ffmpeg  # macOS
sudo apt install libx264-dev  # Ubuntu
```
