# 第10节：FFmpeg 解码器 API 入门

| 字段 | 内容 |
|------|------|
| lesson_id | 10 |
| module | module2_codec |
| type | cpp_ffmpeg |
| prerequisites | 第1-9节 |
| outputs | 理解 FFmpeg 解码器 API，能用 C++ 编写最简视频解码器 |
| estimated_time | 4-5 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.mp4 |
| source | src/module2/lesson10_decoder/ |

## 本节能力目标

- 理解 FFmpeg 解码器 API 的核心概念：`AVCodecContext`、`AVPacket`、`AVFrame`
- 理解 send/receive 解码模型
- 能用 C++ 编写最简视频解码器，输出 YUV 帧
- 能正确管理 FFmpeg 内存资源
- 能编译和运行 C++ 程序

## 真实工程对应场景

- 视频播放器：解码是播放器的核心步骤
- 视频编辑：解码后才能处理视频帧
- 视频转码：解码是转码的第一步
- 视频分析：解码后才能分析视频内容

## 引言

在前几节课中，我们学习了视频编码原理（DCT、量化、H.264 码流结构）和音频编码原理（MDCT、心理声学）。但你是否想过：这些压缩后的数据是如何被解码回原始帧的？

本节课我们将进入 **C++ 工程实战阶段**，学习 FFmpeg 解码器 API。你将用 C++ 编写一个最简视频解码器，将 MP4 文件解码为 YUV 帧。这是成为音视频工程师的关键一步。

---

## 理论讲解

### 核心概念1：FFmpeg 解码器 API 概览

FFmpeg 解码器 API 的核心结构体：

#### 1.1 AVCodecContext

`AVCodecContext` 是编解码器上下文，包含编解码器的所有参数：

```c
AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
```

**关键字段：**
- `width`、`height`：视频分辨率
- `pix_fmt`：像素格式（如 YUV420P）
- `time_base`：时间基准
- `codec_type`：编码类型（视频/音频）

#### 1.2 AVPacket

`AVPacket` 是压缩数据包，包含一帧压缩后的数据：

```c
AVPacket *packet = av_packet_alloc();
```

**关键字段：**
- `data`：压缩数据指针
- `size`：数据大小
- `pts`、`dts`：时间戳
- `stream_index`：流索引

#### 1.3 AVFrame

`AVFrame` 是解码后的帧数据，包含一帧原始数据：

```c
AVFrame *frame = av_frame_alloc();
```

**关键字段：**
- `data[0]`、`data[1]`、`data[2]`：YUV 数据指针
- `linesize[0]`、`linesize[1]`、`linesize[2]`：每行字节数
- `width`、`height`：帧分辨率
- `pts`：显示时间戳

---

### 核心概念2：解码流程

FFmpeg 解码流程：

```
打开输入 → 查找解码器 → 打开解码器 → 读取包 → 发送包 → 接收帧 → 处理帧 → 关闭
```

**详细步骤：**

1. **打开输入文件**
```c
AVFormatContext *fmt_ctx = NULL;
avformat_open_input(&fmt_ctx, "input.mp4", NULL, NULL);
avformat_find_stream_info(fmt_ctx, NULL);
```

2. **查找视频流**
```c
int video_stream_idx = -1;
for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        video_stream_idx = i;
        break;
    }
}
```

3. **查找解码器并创建上下文**
```c
AVCodecParameters *codecpar = fmt_ctx->streams[video_stream_idx]->codecpar;
const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
avcodec_parameters_to_context(codec_ctx, codecpar);
avcodec_open2(codec_ctx, codec, NULL);
```

4. **解码循环**
```c
AVPacket *packet = av_packet_alloc();
AVFrame *frame = av_frame_alloc();

while (av_read_frame(fmt_ctx, packet) >= 0) {
    if (packet->stream_index == video_stream_idx) {
        avcodec_send_packet(codec_ctx, packet);
        while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
            // 处理 frame
        }
    }
    av_packet_unref(packet);
}

// flush
avcodec_send_packet(codec_ctx, NULL);
while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
    // 处理 frame
}
```

5. **释放资源**
```c
av_frame_free(&frame);
av_packet_free(&packet);
avcodec_free_context(&codec_ctx);
avformat_close_input(&fmt_ctx);
```

---

### 核心概念3：内存管理

FFmpeg 内存管理必须严格遵循以下规则：

#### 3.1 资源释放必须严格配对

| 分配函数 | 释放函数 |
|----------|----------|
| `avformat_open_input()` | `avformat_close_input()` |
| `avcodec_alloc_context3()` | `avcodec_free_context()` |
| `av_packet_alloc()` | `av_packet_free()` |
| `av_frame_alloc()` | `av_frame_free()` |

#### 3.2 循环复用对象

在循环中读取包和帧时，使用 `av_packet_unref()` 和 `av_frame_unref()` 清空对象，而不是释放再分配：

```c
while (av_read_frame(fmt_ctx, packet) >= 0) {
    // 使用 packet
    av_packet_unref(packet);  // 清空，不是 av_packet_free()
}
```

#### 3.3 返回值检查

所有 FFmpeg API 的返回值必须检查，失败时输出错误信息：

```c
int ret = avcodec_send_packet(codec_ctx, packet);
if (ret < 0) {
    char err_buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, err_buf, sizeof(err_buf));
    fprintf(stderr, "发送包失败: %s\n", err_buf);
    return ret;
}
```

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **AVCodecContext 是上下文** | 包含编解码器的所有参数 |
| **AVPacket 是压缩数据** | 包含一帧压缩后的数据 |
| **AVFrame 是原始数据** | 包含一帧解码后的 YUV 数据 |
| **send/receive 模型** | 发送包，接收帧，是 FFmpeg 解码的核心 |
| **内存管理必须严格** | alloc/free 必须配对，返回值必须检查 |

---

## 环境与素材准备

### 环境配置

**macOS：**
```bash
brew install ffmpeg cmake
```

**Fedora：**
```bash
sudo dnf install ffmpeg-devel cmake gcc-c++
```

**Ubuntu/Debian：**
```bash
sudo apt install libavcodec-dev libavformat-dev libavutil-dev cmake g++
```

**Docker：**
```bash
docker pull jrottenberg/ffmpeg:4.4-ubuntu20.04
```

### 验证安装

```bash
# 检查 FFmpeg 开发库
pkg-config --cflags --libs libavcodec libavformat libavutil

# 检查 CMake
cmake --version

# 检查编译器
g++ --version
```

### 测试素材

```bash
# 生成测试视频（5秒，360p）
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=25 \
       -c:v libx264 -y test.mp4

# 或复制已有素材
cp notebooks/module1/test.mp4 src/module2/lesson10_decoder/
```

---

## 核心代码实验

### 实验1：最简视频解码器

**目标**：解码 MP4 文件，输出 YUV 帧信息

**代码实现：**
```cpp
// src/module2/lesson10_decoder/main.cpp
#include <iostream>
#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <input.mp4>" << std::endl;
        return 1;
    }

    const char *input_file = argv[1];
    int ret = 0;

    // 1. 打开输入文件
    AVFormatContext *fmt_ctx = nullptr;
    ret = avformat_open_input(&fmt_ctx, input_file, nullptr, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "打开输入文件失败: " << err_buf << std::endl;
        return 1;
    }

    // 2. 查找流信息
    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        std::cerr << "查找流信息失败" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 3. 查找视频流
    int video_stream_idx = -1;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    if (video_stream_idx == -1) {
        std::cerr << "未找到视频流" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 4. 查找解码器
    AVCodecParameters *codecpar = fmt_ctx->streams[video_stream_idx]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "未找到解码器" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 5. 创建解码器上下文
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "创建解码器上下文失败" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 6. 复制参数到上下文
    ret = avcodec_parameters_to_context(codec_ctx, codecpar);
    if (ret < 0) {
        std::cerr << "复制参数失败" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 7. 打开解码器
    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        std::cerr << "打开解码器失败" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 8. 解码循环
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    std::cout << "视频信息:" << std::endl;
    std::cout << "  分辨率: " << codec_ctx->width << "x" << codec_ctx->height << std::endl;
    std::cout << "  像素格式: " << av_get_pix_fmt_name(codec_ctx->pix_fmt) << std::endl;
    std::cout << "  解码器: " << codec->name << std::endl;
    std::cout << std::endl;

    int frame_count = 0;
    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_idx) {
            ret = avcodec_send_packet(codec_ctx, packet);
            if (ret < 0) {
                std::cerr << "发送包失败" << std::endl;
                av_packet_unref(packet);
                continue;
            }

            while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                frame_count++;
                std::cout << "帧 " << frame_count
                          << ": PTS=" << frame->pts
                          ", 尺寸=" << frame->width << "x" << frame->height
                          << ", YUV大小=" << frame->linesize[0] * frame->height * 3 / 2
                          << " 字节" << std::endl;
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    // 9. flush 解码器
    avcodec_send_packet(codec_ctx, nullptr);
    while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
        frame_count++;
        av_frame_unref(frame);
    }

    std::cout << "\n总共解码 " << frame_count << " 帧" << std::endl;

    // 10. 释放资源
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);

    return 0;
}
```

**代码解析：**
- `avformat_open_input()`：打开输入文件
- `avformat_find_stream_info()`：查找流信息
- `avcodec_find_decoder()`：查找解码器
- `avcodec_alloc_context3()`：创建解码器上下文
- `avcodec_parameters_to_context()`：复制参数到上下文
- `avcodec_open2()`：打开解码器
- `avcodec_send_packet()`：发送包到解码器
- `avcodec_receive_frame()`：从解码器接收帧

---

### 实验2：YUV 帧保存

**目标**：将解码的 YUV 帧保存为文件

**代码实现：**
```cpp
// 保存 YUV 帧到文件
void save_yuv_frame(AVFrame *frame, FILE *output_file) {
    // 保存 Y 分量
    for (int i = 0; i < frame->height; i++) {
        fwrite(frame->data[0] + i * frame->linesize[0], 1, frame->width, output_file);
    }

    // 保存 U 分量
    for (int i = 0; i < frame->height / 2; i++) {
        fwrite(frame->data[1] + i * frame->linesize[1], 1, frame->width / 2, output_file);
    }

    // 保存 V 分量
    for (int i = 0; i < frame->height / 2; i++) {
        fwrite(frame->data[2] + i * frame->linesize[2], 1, frame->width / 2, output_file);
    }
}
```

**代码解析：**
- YUV420P 格式：Y 分量是全尺寸，U 和 V 分量是半尺寸
- `linesize` 是每行字节数，可能大于宽度（有填充）
- 必须按行写入，不能直接写入整个 buffer

---

## 预期结果与验收标准

### 预期输出

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

### 验收标准

- [ ] 能理解 AVCodecContext、AVPacket、AVFrame 的作用
- [ ] 能理解 send/receive 解码模型
- [ ] 能用 C++ 编写最简视频解码器
- [ ] 能正确管理 FFmpeg 内存资源
- [ ] 能编译和运行 C++ 程序
- [ ] 能用 ffplay 验证输出的 YUV 文件

---

## 常见错误与排查

### 错误1：找不到 FFmpeg 开发库

**原因**：未安装 FFmpeg 开发库或 pkg-config 配置错误。

**排查**：
- 检查是否安装了开发库：`pkg-config --cflags --libs libavcodec`
- macOS：`brew install ffmpeg`
- Fedora：`sudo dnf install ffmpeg-devel`
- Ubuntu：`sudo apt install libavcodec-dev`

### 错误2：编译错误 "undefined reference"

**原因**：链接库顺序错误或缺少链接库。

**排查**：
- 检查 CMakeLists.txt 中的链接库
- 确保链接顺序正确：`libavformat libavcodec libavutil`

### 错误3：运行时错误 "段错误"

**原因**：内存访问越界或未初始化指针。

**排查**：
- 启用 ASAN：`cmake -DCMAKE_BUILD_TYPE=Debug ..`
- 检查所有指针是否初始化
- 检查所有返回值是否检查

### 错误4：解码失败 "Invalid data found"

**原因**：输入文件损坏或格式不支持。

**排查**：
- 用 `ffprobe` 检查输入文件
- 确保文件格式支持

---

## 课后小挑战

### 基础题：解码音频流

修改代码，解码音频流并输出 PCM 帧信息。

### 进阶题：保存前 10 帧为 YUV 文件

修改代码，将前 10 帧 YUV 数据保存为文件，用 `ffplay` 验证。

```bash
ffplay -video_size 1280x720 -pixel_format yuv420p output.yuv
```

### 思考题1：为什么需要 flush 解码器？

提示：考虑 B 帧的解码延迟。

### 思考题2：linesize 和 width 有什么区别？

提示：考虑内存对齐和 SIMD 优化。

---

## 面试延伸题

### Q1：FFmpeg 的 send/receive 模型是什么？

**答题要点**：
1. `avcodec_send_packet()` 发送压缩数据包到解码器
2. `avcodec_receive_frame()` 从解码器接收解码后的帧
3. 一个包可能产生多个帧（如 B 帧），多个包可能产生一个帧
4. 解码完成后需要 flush（发送 NULL 包）

### Q2：FFmpeg 内存管理的最佳实践是什么？

**答题要点**：
1. `av_packet_alloc()` / `av_packet_free()` 必须配对
2. `av_frame_alloc()` / `av_frame_free()` 必须配对
3. 循环中使用 `av_packet_unref()` / `av_frame_unref()` 清空对象
4. 所有返回值必须检查，失败时输出错误信息

### Q3：如何验证解码输出的 YUV 文件？

**答题要点**：
1. 使用 `ffplay` 播放：`ffplay -video_size 1280x720 -pixel_format yuv420p output.yuv`
2. 需要指定分辨率和像素格式
3. 可以用 `ffmpeg` 转换为 MP4 验证

---

## 延伸阅读

1. [FFmpeg 官方文档 - Decoding](https://ffmpeg.org/ffmpeg-codec.html#Decoding)
2. [FFmpeg 解码教程](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html)
3. [FFmpeg 内存管理](https://ffmpeg.org/doxygen/trunk/group__lavu__buffer.html)

---

### 下节课预告

第11节：FFmpeg 内存模型与引用计数
- 学习 AVBufferRef、引用计数的概念
- 编写泄漏检测程序，用 ASAN 验证资源释放
- 深入理解 FFmpeg 的内存管理机制
