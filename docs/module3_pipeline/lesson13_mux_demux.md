# 第13节：解封装与封装 API 详解

| 字段 | 内容 |
|------|------|
| lesson_id | 13 |
| module | module3_pipeline |
| type | cpp_ffmpeg |
| prerequisites | 第1-12节 |
| outputs | 理解 FFmpeg 解封装与封装 API，能用 C++ 将裸流打包为 MP4 |
| estimated_time | 4-5 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.h264, test.aac |
| source | src/module3/lesson13_mux_demux/ |

## 本节能力目标

- 理解 AVFormatContext 的作用
- 理解解封装（Demux）和封装（Mux）的流程
- 理解时间基转换的概念
- 能用 C++ 将 H.264 裸流打包为 MP4
- 能用 C++ 将 H.264 + AAC 打包为 MP4

## 真实工程对应场景

- 视频转码：解封装 → 解码 → 编码 → 封装
- 视频剪辑：解封装 → 裁剪 → 封装
- 直播推流：解封装 → 编码 → 封装 → 推流
- 格式转换：解封装 → 封装（转封装）

## 引言

在第10-12节中，我们学习了 FFmpeg 解码器 API、内存模型和编码器 API。但你是否想过：这些压缩后的数据是如何被打包成 MP4 文件的？MP4 文件是如何被读取和分离的？

本节课我们将学习 **FFmpeg 的解封装与封装 API**。你将用 C++ 编写程序，将 H.264 裸流和 AAC 裸流打包为 MP4 文件。这是音视频处理管道的核心步骤。

---

## 理论讲解

### 核心概念1：AVFormatContext 概览

`AVFormatContext` 是 FFmpeg 的格式上下文，包含容器的所有信息：

```c
AVFormatContext *fmt_ctx = NULL;
avformat_open_input(&fmt_ctx, "input.mp4", NULL, NULL);
```

**关键字段：**
- `nb_streams`：流的数量
- `streams`：流数组（视频流、音频流、字幕流）
- `duration`：总时长
- `bit_rate`：总码率
- `iformat`：输入格式
- `oformat`：输出格式

### 核心概念2：解封装（Demux）流程

解封装是从容器中分离出各路码流的过程：

```
打开输入 → 查找流信息 → 读取包 → 关闭输入
```

**详细步骤：**

1. **打开输入文件**
```c
AVFormatContext *fmt_ctx = NULL;
avformat_open_input(&fmt_ctx, "input.mp4", NULL, NULL);
avformat_find_stream_info(fmt_ctx, NULL);
```

2. **查找流**
```c
int video_idx = -1;
int audio_idx = -1;
for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        video_idx = i;
    } else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        audio_idx = i;
    }
}
```

3. **读取包**
```c
AVPacket *packet = av_packet_alloc();
while (av_read_frame(fmt_ctx, packet) >= 0) {
    if (packet->stream_index == video_idx) {
        // 处理视频包
    } else if (packet->stream_index == audio_idx) {
        // 处理音频包
    }
    av_packet_unref(packet);
}
```

4. **关闭输入**
```c
av_packet_free(&packet);
avformat_close_input(&fmt_ctx);
```

### 核心概念3：封装（Mux）流程

封装是将多路码流打包进容器的过程：

```
创建输出 → 添加流 → 写入头 → 写入包 → 写入尾 → 关闭输出
```

**详细步骤：**

1. **创建输出上下文**
```c
AVFormatContext *ofmt_ctx = NULL;
avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, "output.mp4");
```

2. **添加流**
```c
AVStream *out_stream = avformat_new_stream(ofmt_ctx, NULL);
avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
```

3. **写入头**
```c
avio_open(&ofmt_ctx->pb, "output.mp4", AVIO_FLAG_WRITE);
avformat_write_header(ofmt_ctx, NULL);
```

4. **写入包**
```c
packet->stream_index = out_stream->index;
av_interleaved_write_frame(ofmt_ctx, packet);
```

**注意**：`av_interleaved_write_frame()` 会自动对包进行排序，确保输出文件的时间戳顺序正确。如果不需要排序，可以使用 `av_write_frame()`。

5. **写入尾**
```c
av_write_trailer(ofmt_ctx);
```

6. **关闭输出**
```c
avio_closep(&ofmt_ctx->pb);
avformat_free_context(ofmt_ctx);
```

### 核心概念4：时间基转换

不同流可能有不同的时间基，需要进行转换：

```c
// 时间基转换
int64_t out_pts = av_rescale_q(packet->pts, in_stream->time_base, out_stream->time_base);
int64_t out_dts = av_rescale_q(packet->dts, in_stream->time_base, out_stream->time_base);
int64_t out_duration = av_rescale_q(packet->duration, in_stream->time_base, out_stream->time_base);
```

**时间基说明：**
- `time_base`：时间基准，表示每个时间单位的秒数
- `AVRational`：有理数，如 `{1, 90000}` 表示 1/90000 秒
- 不同流的时间基可能不同（如视频 1/90000，音频 1/44100）

### 核心概念5：流索引（Stream Index）

每个流都有一个唯一的索引，用于标识不同的流：

```c
// 查找视频流和音频流
int video_idx = -1;
int audio_idx = -1;
for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        video_idx = i;
    } else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        audio_idx = i;
    }
}

// 读取包时根据 stream_index 判断是哪种流
while (av_read_frame(fmt_ctx, packet) >= 0) {
    if (packet->stream_index == video_idx) {
        // 处理视频包
    } else if (packet->stream_index == audio_idx) {
        // 处理音频包
    }
    av_packet_unref(packet);
}
```

### 核心概念6：mux/demux 对称性

解封装和封装是互逆过程：

| 操作 | 解封装（Demux） | 封装（Mux） |
|------|-----------------|-------------|
| 打开 | `avformat_open_input()` | `avformat_alloc_output_context2()` |
| 获取信息 | `avformat_find_stream_info()` | `avformat_new_stream()` |
| 读写 | `av_read_frame()` | `av_interleaved_write_frame()` |
| 关闭 | `avformat_close_input()` | `av_write_trailer()` + `avio_closep()` |

**对称性说明**：
- 解封装读取包，封装写入包
- 解封装关闭输入，封装关闭输出
- 两个流程的数据流向相反

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **AVFormatContext 是格式上下文** | 包含容器的所有信息 |
| **解封装是分离过程** | 从容器中分离出各路码流 |
| **封装是打包过程** | 将多路码流打包进容器 |
| **流索引标识不同的流** | 通过 `stream_index` 区分视频和音频 |
| **mux/demux 是互逆过程** | 解封装读取包，封装写入包 |
| **时间基是时间单位** | 不同流可能有不同的时间基 |
| **av_rescale_q 转换时间基** | 将时间戳从一个时间基转换为另一个 |

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

### 测试素材

```bash
# 生成测试视频（5秒，640x360）
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=25 \
       -f lavfi -i sine=frequency=440:duration=5 \
       -c:v libx264 -c:a aac -shortest -y test.mp4

# 提取裸流
ffmpeg -i test.mp4 -c:v copy -an test.h264
ffmpeg -i test.mp4 -c:a copy -vn test.aac
```

---

## 核心代码实验

### 实验1：将 H.264 裸流打包为 MP4

**目标**：读取 H.264 裸流，封装为 MP4 容器

**代码实现：**
```cpp
// src/module3/lesson13_mux_demux/mux.cpp
#include <iostream>
#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <input.h264> <output.mp4>" << std::endl;
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    int ret = 0;

    // 1. 打开输入（H.264 裸流）
    AVFormatContext *ifmt_ctx = NULL;
    ret = avformat_open_input(&ifmt_ctx, input_file, NULL, NULL);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "打开输入文件失败: " << err_buf << std::endl;
        return 1;
    }

    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0) {
        std::cerr << "查找流信息失败" << std::endl;
        avformat_close_input(&ifmt_ctx);
        return 1;
    }

    // 2. 创建输出上下文
    AVFormatContext *ofmt_ctx = NULL;
    ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output_file);
    if (ret < 0) {
        std::cerr << "创建输出上下文失败" << std::endl;
        avformat_close_input(&ifmt_ctx);
        return 1;
    }

    // 3. 添加输出流
    AVStream *out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream) {
        std::cerr << "添加输出流失败" << std::endl;
        avformat_close_input(&ifmt_ctx);
        avformat_free_context(ofmt_ctx);
        return 1;
    }

    // 复制编解码参数
    ret = avcodec_parameters_copy(out_stream->codecpar, ifmt_ctx->streams[0]->codecpar);
    if (ret < 0) {
        std::cerr << "复制编解码参数失败" << std::endl;
        avformat_close_input(&ifmt_ctx);
        avformat_free_context(ofmt_ctx);
        return 1;
    }
    out_stream->codecpar->codec_tag = 0;

    // 4. 打开输出文件
    ret = avio_open(&ofmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "打开输出文件失败: " << err_buf << std::endl;
        avformat_close_input(&ifmt_ctx);
        avformat_free_context(ofmt_ctx);
        return 1;
    }

    // 5. 写入文件头
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "写入文件头失败: " << err_buf << std::endl;
        avio_closep(&ofmt_ctx->pb);
        avformat_close_input(&ifmt_ctx);
        avformat_free_context(ofmt_ctx);
        return 1;
    }

    // 6. 读取包并写入输出
    AVPacket *packet = av_packet_alloc();
    int packet_count = 0;

    std::cout << "开始封装..." << std::endl;
    std::cout << "  输入: " << input_file << std::endl;
    std::cout << "  输出: " << output_file << std::endl;
    std::cout << std::endl;

    while (av_read_frame(ifmt_ctx, packet) >= 0) {
        // 转换时间基
        packet->pts = av_rescale_q(packet->pts, ifmt_ctx->streams[0]->time_base, out_stream->time_base);
        packet->dts = av_rescale_q(packet->dts, ifmt_ctx->streams[0]->time_base, out_stream->time_base);
        packet->duration = av_rescale_q(packet->duration, ifmt_ctx->streams[0]->time_base, out_stream->time_base);
        packet->stream_index = 0;

        // 写入包
        ret = av_write_frame(ofmt_ctx, packet);
        if (ret < 0) {
            std::cerr << "写入包失败" << std::endl;
            break;
        }

        packet_count++;
        av_packet_unref(packet);
    }

    // 7. 写入文件尾
    av_write_trailer(ofmt_ctx);

    std::cout << "封装完成: " << packet_count << " 个包" << std::endl;

    // 8. 释放资源
    av_packet_free(&packet);
    avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    avformat_close_input(&ifmt_ctx);

    return 0;
}
```

**代码解析：**
- `avformat_open_input()`：打开输入文件
- `avformat_alloc_output_context2()`：创建输出上下文
- `avformat_new_stream()`：添加输出流
- `avcodec_parameters_copy()`：复制编解码参数
- `avformat_write_header()`：写入文件头
- `av_write_frame()`：写入包
- `av_write_trailer()`：写入文件尾

---

## 预期结果与验收标准

### 预期输出

```text
开始封装...
  输入: test.h264
  输出: output.mp4

封装完成: 125 个包
```

### 验证编码输出

```bash
# 用 ffplay 播放封装后的 MP4 文件
ffplay output.mp4

# 用 ffprobe 分析封装后的文件
ffprobe output.mp4
```

### 验收标准

- [ ] 能理解 AVFormatContext 的作用
- [ ] 能理解解封装和封装的流程
- [ ] 能理解时间基转换的概念
- [ ] 能用 C++ 将 H.264 裸流打包为 MP4
- [ ] 能用 ffplay 验证封装输出

---

## 常见错误与排查

### 错误1：打开输出文件失败

**原因**：输出文件路径错误或权限不足。

**排查**：
- 检查输出文件路径是否正确
- 检查是否有写入权限

### 错误2：写入文件头失败

**原因**：输出格式不支持或参数错误。

**排查**：
- 检查输出格式是否支持
- 检查编解码参数是否正确

### 错误3：封装后播放异常

**原因**：时间基转换错误或时间戳不连续。

**排查**：
- 检查时间基转换是否正确
- 检查时间戳是否连续

### 错误4：封装后音视频不同步

**原因**：音频和视频的时间基转换不一致。

**排查**：
- 确保音频和视频都正确转换时间基
- 检查时间戳是否正确

---

## 课后小挑战

### 基础题：将 H.264 + AAC 打包为 MP4

修改代码，同时封装视频流和音频流。

### 进阶题：实现转封装

编写程序，将 MKV 容器转换为 MP4 容器（不解码）。

### 思考题1：为什么需要时间基转换？

提示：考虑不同流的时间基可能不同。

### 思考题2：mux 和 demux 的对称性是什么？

提示：考虑解封装和封装的流程是互逆的。

---

## 面试延伸题

### Q1：什么是 AVFormatContext？

**答题要点**：
1. AVFormatContext 是 FFmpeg 的格式上下文
2. 包含容器的所有信息（流、时长、码率等）
3. 用于解封装和封装操作

### Q2：解封装和封装的流程是什么？

**答题要点**：
1. 解封装：打开输入 → 查找流信息 → 读取包 → 关闭输入
2. 封装：创建输出 → 添加流 → 写入头 → 写入包 → 写入尾 → 关闭输出

### Q3：什么是时间基？为什么需要时间基转换？

**答题要点**：
1. 时间基是时间单位，表示每个时间单位的秒数
2. 不同流的时间基可能不同（如视频 1/90000，音频 1/44100）
3. 封装时需要将时间戳从输入时间基转换为输出时间基

---

## 延伸阅读

1. [FFmpeg 官方文档 - Muxing](https://ffmpeg.org/ffmpeg-formats.html#Muxers)
2. [FFmpeg 官方文档 - Demuxing](https://ffmpeg.org/ffmpeg-formats.html#Demuxers)
3. [FFmpeg Mux/Demux 教程](https://ffmpeg.org/doxygen/trunk/group__libavf.html)

---

### 下节课预告

第14节：解码管道构建
- 学习解封装 → 解码 → 后处理的数据流转
- 将 MP4 解码为原始 YUV/PCM 文件
- 构建完整的音视频处理管道
