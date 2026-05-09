# 第12节：FFmpeg 编码器 API 入门

| 字段 | 内容 |
|------|------|
| lesson_id | 12 |
| module | module2_codec |
| type | cpp_ffmpeg |
| prerequisites | 第1-11节 |
| outputs | 理解 FFmpeg 编码器 API，能用 C++ 将 YUV 编码为 H.264 |
| estimated_time | 4-5 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.yuv |
| source | src/module2/lesson12_encoder/ |

## 本节能力目标

- 理解 FFmpeg 编码器 API 的核心概念
- 理解编码参数（CRF、preset、profile、level）的作用
- 能用 C++ 将 YUV 序列编码为 H.264 文件
- 能正确 flush 编码器
- 能用 ffplay 验证编码输出

## 真实工程对应场景

- 视频转码：将视频从一种格式转换为另一种格式
- 视频压缩：调整视频码率和质量
- 视频编辑：将编辑后的帧编码为视频
- 直播推流：实时编码视频流

## 引言

在第10-11节中，我们学习了 FFmpeg 解码器 API 和内存模型。但你是否想过：视频是如何被编码的？为什么有些视频文件很小但画质很好？

本节课我们将学习 **FFmpeg 编码器 API**。你将用 C++ 编写一个最简视频编码器，将 YUV 序列编码为 H.264 文件。这是成为音视频工程师的关键一步。

---

## 理论讲解

### 核心概念1：FFmpeg 编码器 API 概览

FFmpeg 编码器 API 的核心结构体：

#### 1.1 AVCodecContext

`AVCodecContext` 是编解码器上下文，包含编码器的所有参数：

```c
AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
```

**关键字段：**
- `width`、`height`：视频分辨率
- `pix_fmt`：像素格式（如 YUV420P）
- `time_base`：时间基准
- `bit_rate`：目标码率
- `crf`：恒定质量因子
- `preset`：编码速度预设
- `profile`：编码配置
- `level`：编码级别

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

`AVFrame` 是原始帧数据，包含一帧原始数据：

```c
AVFrame *frame = av_frame_alloc();
```

**关键字段：**
- `data[0]`、`data[1]`、`data[2]`：YUV 数据指针
- `linesize[0]`、`linesize[1]`、`linesize[2]`：每行字节数
- `width`、`height`：帧分辨率
- `pts`：显示时间戳

---

### 核心概念2：编码参数设置

#### 2.1 分辨率和像素格式

```c
codec_ctx->width = 640;
codec_ctx->height = 360;
codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
```

#### 2.2 帧率和时间基准

```c
codec_ctx->time_base = (AVRational){1, 25};  // 25fps
codec_ctx->framerate = (AVRational){25, 1};
```

#### 2.3 码率和 CRF

```c
// 恒定码率模式
codec_ctx->bit_rate = 500000;  // 500kbps

// 恒定质量模式（CRF）
codec_ctx->crf = 23;  // 默认值，范围 0-51，越小质量越好
```

**CRF 说明：**
| CRF | 质量 | 应用场景 |
|-----|------|----------|
| 0 | 无损 | 专业视频制作 |
| 18-23 | 高质量 | 网络视频 |
| 23-28 | 中等质量 | 移动设备 |
| 28-51 | 低质量 | 低带宽 |

#### 2.4 Preset（编码速度预设）

```c
if (codec->id == AV_CODEC_ID_H264) {
    av_opt_set(codec_ctx->priv_data, "preset", "medium", 0);
}
```

**Preset 说明：**
| Preset | 速度 | 压缩率 | 应用场景 |
|--------|------|--------|----------|
| ultrafast | 最快 | 最低 | 实时编码 |
| fast | 快 | 低 | 快速转码 |
| medium | 中等 | 中等 | 默认 |
| slow | 慢 | 高 | 高质量转码 |
| veryslow | 最慢 | 最高 | 专业视频制作 |

#### 2.5 Profile 和 Level

```c
codec_ctx->profile = FF_PROFILE_H264_MAIN;
codec_ctx->level = 31;  // Level 3.1
```

**Profile 说明：**
| Profile | 特点 | 应用场景 |
|---------|------|----------|
| Baseline | 无 B 帧，无 CABAC | 移动设备 |
| Main | 有 B 帧，有 CABAC | 网络视频 |
| High | 最高压缩率 | 高清视频 |

**Level 说明：**
Level 定义了视频的复杂度限制，如最大码率、最大帧大小等。

---

### 核心概念3：编码流程

FFmpeg 编码流程：

```
创建上下文 → 设置参数 → 打开编码器 → 发送帧 → 接收包 → flush → 关闭
```

**详细步骤：**

1. **查找编码器**
```c
const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
```

2. **创建编码器上下文**
```c
AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
```

3. **设置编码参数**
```c
codec_ctx->width = 640;
codec_ctx->height = 360;
codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
codec_ctx->time_base = (AVRational){1, 25};
codec_ctx->framerate = (AVRational){25, 1};
codec_ctx->bit_rate = 500000;
```

4. **打开编码器**
```c
avcodec_open2(codec_ctx, codec, NULL);
```

5. **编码循环**
```c
AVFrame *frame = av_frame_alloc();
AVPacket *packet = av_packet_alloc();

while (read_yuv_frame(frame)) {
    frame->pts = frame_count++;
    
    avcodec_send_frame(codec_ctx, frame);
    while (avcodec_receive_packet(codec_ctx, packet) >= 0) {
        // 写入文件
        fwrite(packet->data, 1, packet->size, output_file);
        av_packet_unref(packet);
    }
    av_frame_unref(frame);
}
```

6. **flush 编码器**
```c
avcodec_send_frame(codec_ctx, NULL);
while (avcodec_receive_packet(codec_ctx, packet) >= 0) {
    fwrite(packet->data, 1, packet->size, output_file);
    av_packet_unref(packet);
}
```

7. **释放资源**
```c
av_frame_free(&frame);
av_packet_free(&packet);
avcodec_free_context(&codec_ctx);
```

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **AVCodecContext 是上下文** | 包含编码器的所有参数 |
| **AVPacket 是压缩数据** | 包含一帧压缩后的数据 |
| **AVFrame 是原始数据** | 包含一帧原始 YUV 数据 |
| **send/receive 模型** | 发送帧，接收包，是 FFmpeg 编码的核心 |
| **CRF 控制质量** | CRF 越小，质量越好，文件越大 |
| **Preset 控制速度** | Preset 越慢，压缩率越高 |
| **flush 编码器** | 编码完成后必须 flush，输出所有缓冲的包 |

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
       -c:v libx264 -y test.mp4

# 提取 YUV 数据
ffmpeg -i test.mp4 -pix_fmt yuv420p -s 640x360 test.yuv
```

---

## 核心代码实验

### 实验1：将 YUV 序列编码为 H.264 文件

**目标**：读取 YUV 文件，编码为 H.264 裸流

**代码实现：**
```cpp
// src/module2/lesson12_encoder/main.cpp
#include <iostream>
#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <input.yuv> <output.h264>" << std::endl;
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    int ret = 0;

    // 1. 查找编码器
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "未找到 H.264 编码器" << std::endl;
        return 1;
    }

    // 2. 创建编码器上下文
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "创建编码器上下文失败" << std::endl;
        return 1;
    }

    // 3. 设置编码参数
    codec_ctx->width = 640;
    codec_ctx->height = 360;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->time_base = (AVRational){1, 25};
    codec_ctx->framerate = (AVRational){25, 1};
    codec_ctx->bit_rate = 500000;  // 码率模式（也可以使用 CRF 模式：codec_ctx->crf = 23）
    codec_ctx->gop_size = 10;
    codec_ctx->max_b_frames = 2;

    // 设置 preset（仅对 H.264 有效）
    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(codec_ctx->priv_data, "preset", "medium", 0);
    }

    // 4. 打开编码器
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "打开编码器失败: " << err_buf << std::endl;
        avcodec_free_context(&codec_ctx);
        return 1;
    }

    // 5. 打开输入文件
    FILE *yuv_file = fopen(input_file, "rb");
    if (!yuv_file) {
        std::cerr << "打开输入文件失败: " << input_file << std::endl;
        avcodec_free_context(&codec_ctx);
        return 1;
    }

    // 6. 打开输出文件
    FILE *h264_file = fopen(output_file, "wb");
    if (!h264_file) {
        std::cerr << "打开输出文件失败: " << output_file << std::endl;
        fclose(yuv_file);
        avcodec_free_context(&codec_ctx);
        return 1;
    }

    // 7. 分配 frame 和 packet
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    if (!frame || !packet) {
        std::cerr << "分配 frame/packet 失败" << std::endl;
        av_frame_free(&frame);
        av_packet_free(&packet);
        fclose(yuv_file);
        fclose(h264_file);
        avcodec_free_context(&codec_ctx);
        return 1;
    }

    // 设置 frame 参数
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    frame->format = codec_ctx->pix_fmt;

    // 8. 编码循环
    int frame_count = 0;
    int y_size = codec_ctx->width * codec_ctx->height;
    int uv_size = y_size / 4;

    std::cout << "编码参数:" << std::endl;
    std::cout << "  分辨率: " << codec_ctx->width << "x" << codec_ctx->height << std::endl;
    std::cout << "  帧率: " << codec_ctx->framerate.num << "/" << codec_ctx->framerate.den << std::endl;
    std::cout << "  码率: " << codec_ctx->bit_rate << " bps" << std::endl;
    std::cout << "  GOP: " << codec_ctx->gop_size << std::endl;
    std::cout << std::endl;

    while (true) {
        // 读取 YUV 数据
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            std::cerr << "分配 frame 缓冲区失败" << std::endl;
            break;
        }

        // 读取 Y 分量
        if (fread(frame->data[0], 1, y_size, yuv_file) != y_size) break;
        // 读取 U 分量
        if (fread(frame->data[1], 1, uv_size, yuv_file) != uv_size) break;
        // 读取 V 分量
        if (fread(frame->data[2], 1, uv_size, yuv_file) != uv_size) break;

        frame->pts = frame_count++;

        // 发送帧到编码器
        ret = avcodec_send_frame(codec_ctx, frame);
        if (ret < 0) {
            std::cerr << "发送帧失败" << std::endl;
            break;
        }

        // 接收编码后的包
        while (avcodec_receive_packet(codec_ctx, packet) >= 0) {
            fwrite(packet->data, 1, packet->size, h264_file);
            av_packet_unref(packet);
        }

        av_frame_unref(frame);
    }

    // 9. flush 编码器
    avcodec_send_frame(codec_ctx, NULL);
    while (avcodec_receive_packet(codec_ctx, packet) >= 0) {
        fwrite(packet->data, 1, packet->size, h264_file);
        av_packet_unref(packet);
    }

    std::cout << "编码完成: " << frame_count << " 帧" << std::endl;

    // 10. 释放资源
    av_frame_free(&frame);
    av_packet_free(&packet);
    fclose(yuv_file);
    fclose(h264_file);
    avcodec_free_context(&codec_ctx);

    return 0;
}
```

**代码解析：**
- `avcodec_find_encoder()`：查找编码器
- `avcodec_alloc_context3()`：创建编码器上下文
- `av_opt_set()`：设置编码器私有参数
- `avcodec_open2()`：打开编码器
- `avcodec_send_frame()`：发送帧到编码器
- `avcodec_receive_packet()`：从编码器接收包

---

## 预期结果与验收标准

### 预期输出

```text
编码参数:
  分辨率: 640x360
  帧率: 25/1
  码率: 500000 bps
  GOP: 10

编码完成: 125 帧
```

### 验证编码输出

```bash
# 用 ffplay 播放编码后的 H.264 文件
ffplay test.h264

# 用 ffprobe 分析编码后的文件
ffprobe test.h264

# 用 ffmpeg 转换为 MP4
ffmpeg -i test.h264 -c copy output.mp4
```

### 验收标准

- [ ] 能理解 FFmpeg 编码器 API 的核心概念
- [ ] 能理解编码参数（CRF、preset、profile、level）的作用
- [ ] 能用 C++ 将 YUV 序列编码为 H.264 文件
- [ ] 能正确 flush 编码器
- [ ] 能用 ffplay 验证编码输出

---

## 常见错误与排查

### 错误1：找不到 H.264 编码器

**原因**：FFmpeg 编译时未启用 libx264。

**排查**：
- 检查 FFmpeg 是否支持 H.264 编码：`ffmpeg -encoders | grep h264`
- 安装 libx264 开发库

### 错误2：编码失败 "Invalid argument"

**原因**：编码参数设置错误。

**排查**：
- 检查像素格式是否支持
- 检查分辨率是否为偶数
- 检查时间基准是否正确

### 错误3：输出文件为空

**原因**：未正确 flush 编码器。

**排查**：
- 确保在编码循环结束后调用 `avcodec_send_frame(codec_ctx, NULL)`
- 确保在 flush 后调用 `avcodec_receive_packet()`

### 错误4：编码质量差

**原因**：码率设置过低或 CRF 设置过高。

**排查**：
- 增加码率：`codec_ctx->bit_rate = 1000000`
- 降低 CRF：`codec_ctx->crf = 18`

---

## 课后小挑战

### 基础题：对比不同 CRF 的编码效果

修改代码，使用不同的 CRF 值（18、23、28）编码同一 YUV 文件，对比输出文件大小和画质。

### 进阶题：实现视频转码

结合第10节的解码器，实现完整的视频转码流程：MP4 → YUV → H.264。

### 思考题1：为什么编码后需要 flush？

提示：考虑 B 帧的编码延迟。

### 思考题2：CRF 和码率模式有什么区别？

提示：考虑恒定质量 vs 恒定码率的应用场景。

---

## 面试延伸题

### Q1：FFmpeg 的 send/receive 编码模型是什么？

**答题要点**：
1. `avcodec_send_frame()` 发送原始帧到编码器
2. `avcodec_receive_packet()` 从编码器接收压缩包
3. 一个帧可能产生多个包，多个帧可能产生一个包
4. 编码完成后需要 flush（发送 NULL 帧）

### Q2：如何选择合适的编码参数？

**答题要点**：
1. 分辨率和帧率：根据应用场景选择
2. 码率和 CRF：根据质量需求选择
3. Preset：根据编码速度需求选择
4. Profile：根据兼容性需求选择

### Q3：什么是 flush？为什么需要 flush？

**答题要点**：
1. Flush 是清空编码器缓冲区的过程
2. 编码器可能缓存了多个帧（如 B 帧）
3. 编码完成后必须 flush，输出所有缓冲的包
4. 调用 `avcodec_send_frame(codec_ctx, NULL)` 触发 flush

---

## 延伸阅读

1. [FFmpeg 官方文档 - Encoding](https://ffmpeg.org/ffmpeg-codec.html#Encoding)
2. [FFmpeg 编码教程](https://ffmpeg.org/doxygen/trunk/group__lavc__encoding.html)
3. [x264 编码参数详解](https://trac.ffmpeg.org/wiki/Encode/H.264)

---

### 下节课预告

第13节：解封装与封装 API 详解
- 学习 AVFormatContext、流索引、时间基转换
- 将 H.264 裸流和 AAC 裸流打包为 MP4
- 进入模块三：音视频解封装、封装与处理管道
