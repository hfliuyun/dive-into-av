# 第14节：解码管道构建

| 字段 | 内容 |
|------|------|
| lesson_id | 14 |
| module | module3_pipeline |
| type | cpp_ffmpeg |
| prerequisites | 第1-10节、第13节 |
| outputs | 理解解码管道的数据流转，能用 C++ 将 MP4 解码为 YUV/PCM |
| estimated_time | 4-5 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.mp4 |
| source | src/module3/lesson14_decode_pipeline/ |

## 本节能力目标

- 理解解码管道的数据流转：解封装 → 解码 → 后处理
- 理解 YUV420P 的平面内存布局以及像素格式检查的重要性
- 理解音视频交错读取（Interleaved Demuxing）的概念
- 能用 C++ 将 MP4 解码为 YUV 文件
- 能用 C++ 将 MP4 解码为 PCM 文件（正确处理 planar 和交错格式）
- 能用 ffplay 验证解码输出

## 真实工程对应场景

- 视频播放器：解码是播放器的核心步骤
- 视频转码：解码是转码的第一步
- 视频分析：解码后才能分析视频内容
- 视频编辑：解码后才能编辑视频帧

## 引言

在第10-13节中，我们学习了 FFmpeg 解码器 API、内存模型、编码器 API 和解封装/封装 API。但你是否想过：这些 API 是如何组合在一起的？一个完整的解码管道是如何构建的？

本节课我们将学习 **解码管道的构建**。你将用 C++ 编写一个完整的解码管道，将 MP4 文件解码为 YUV 和 PCM 文件。这是音视频处理的核心技能。

---

## 理论讲解

### 核心概念1：解码管道数据流转

解码管道的数据流转：

```
MP4 → 解封装 → AVPacket → 解码 → AVFrame → 后处理 → YUV/PCM
```

**详细步骤：**

1. **解封装**：从 MP4 文件中读取压缩数据包（`AVPacket`）
2. **解码**：将压缩数据包解码为原始帧（`AVFrame`）
3. **后处理**：将原始帧保存为 YUV/PCM 文件

**代码流程（简化版）：**
```c
// 1. 打开输入文件
avformat_open_input(&fmt_ctx, "input.mp4", NULL, NULL);
avformat_find_stream_info(fmt_ctx, NULL);

// 2. 为每个流创建解码器
const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
avcodec_parameters_to_context(codec_ctx, codecpar);
avcodec_open2(codec_ctx, codec, NULL);

// 3. 解码循环（交错读取音视频包）
while (av_read_frame(fmt_ctx, packet) >= 0) {
    avcodec_send_packet(codec_ctx, packet);
    while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
        // 保存 YUV/PCM 数据
    }
    av_packet_unref(packet);
}

// 4. flush 解码器（输出缓冲区中剩余的帧）
ret = avcodec_send_packet(codec_ctx, NULL);
if (ret < 0 && ret != AVERROR_EOF) {
    // 输出错误信息
}
while ((ret = avcodec_receive_frame(codec_ctx, frame)) >= 0) {
    // 保存 YUV/PCM 数据
}
```

### 核心概念2：YUV420P 平面内存布局

H.264 解码器默认输出 **YUV420P** 格式。YUV420P 使用 planar（平面）存储方式，三个分量分别存放在独立的连续内存区域中：

```
data[0] → Y 平面：  所有像素的亮度分量，大小 = width × height
data[1] → U 平面：  色度 U 分量，大小 = (width/2) × (height/2) = width × height / 4
data[2] → V 平面：  色度 V 分量，大小 = (width/2) × (height/2) = width × height / 4
```

写入 YUV 文件时，按 Y → U → V 的顺序连续写入三个平面的数据即可：

```c
int y_size = frame->width * frame->height;
fwrite(frame->data[0], 1, y_size, yuv_fp);        // Y 平面
fwrite(frame->data[1], 1, y_size / 4, yuv_fp);     // U 平面
fwrite(frame->data[2], 1, y_size / 4, yuv_fp);     // V 平面
```

> ⚠️ **重要**：不同编码器的解码输出像素格式可能不同（如 VP9 可输出 YUV444P、AV1 可输出 YUV420P10）。代码中通过 `pix_fmt != AV_PIX_FMT_YUV420P` 检查并打印警告，如果格式不匹配，输出的 YUV 文件可能无法正确播放。

### 核心概念3：音视频交错读取

当 MP4 文件包含音频和视频流时，`av_read_frame()` 按文件中包的实际排列顺序读取（而不是先读完所有视频再读音频），这就是**交错读取（Interleaved Demuxing）**。

需要通过 `packet->stream_index` 判断当前包属于哪个流，然后发送到对应的解码器：

```c
// 查找视频流和音频流的索引
int video_idx = -1;
int audio_idx = -1;
for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        video_idx = i;
    } else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        audio_idx = i;
    }
}

// 交错读取
while (av_read_frame(fmt_ctx, packet) >= 0) {
    if (packet->stream_index == video_idx) {
        // 解码视频包
    } else if (packet->stream_index == audio_idx) {
        // 解码音频包
    }
    av_packet_unref(packet);
}
```

### 核心概念4：音频 PCM 的 Planar 与交错格式

FFmpeg 解码器输出的音频 `AVFrame` 有两种内存布局：

- **Planar（平面）**：每个声道的数据存放在独立的 `data[ch]` 中，如 `AV_SAMPLE_FMT_S16P`（planar 16-bit signed integer）
- **Packed/Interleaved（交错）**：所有声道的数据交织存放在 `data[0]` 中，如 `AV_SAMPLE_FMT_S16`（交错 16-bit signed integer）

写入 PCM 文件时必须正确处理两种格式：

```c
int data_size = av_get_bytes_per_sample(audio_ctx->sample_fmt);
if (av_sample_fmt_is_planar(audio_ctx->sample_fmt)) {
    // 平面格式：逐样本交错写入
    for (int i = 0; i < frame->nb_samples; i++)
        for (int ch = 0; ch < audio_ctx->ch_layout.nb_channels; ch++)
            fwrite(frame->data[ch] + data_size * i, 1, data_size, pcm_fp);
} else {
    // 交错格式：一次性写入整个 data[0]
    int total = frame->nb_samples * data_size * audio_ctx->ch_layout.nb_channels;
    fwrite(frame->data[0], 1, total, pcm_fp);
}
```

> 💡 **效率提示**：以上逐样本 `fwrite` 是教学展示用途，每次写入 2-4 字节。在真实工程中应使用批量写入或 `memcpy` 到中间缓冲区后一次性写入，减少系统调用开销。

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **解码管道是数据流转** | 解封装 → 解码 → 后处理 |
| **解封装读取压缩包** | 从 MP4 文件中读取 AVPacket |
| **解码产生原始帧** | 将 AVPacket 解码为 AVFrame |
| **YUV420P 是平面布局** | Y/U/V 三个分量独立连续存储，写入 YUV 文件时按序写入 |
| **音视频交错读取** | 按 stream_index 区分音频和视频 |
| **音频有 planar/交错之分** | 使用 `av_sample_fmt_is_planar()` 区分，不同格式写入逻辑不同 |
| **flush 清空解码器缓冲区** | 解码完成后必须发送 NULL 包��发 flush，输出 B 帧等延迟帧 |

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
# 生成测试视频（5秒，640x360，带音频）
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=25 \
       -f lavfi -i sine=frequency=440:duration=5 \
       -c:v libx264 -c:a aac -shortest -y test.mp4
```

---

## 核心代码实验

### 实验1：将 MP4 解码为 YUV 和 PCM 文件

**目标**：读取 MP4 文件，解码视频和音频流，分别保存为 YUV 和 PCM 文件

完整的代码实现请参见 `src/module3/lesson14_decode_pipeline/main.cpp`。以下为关键代码片段解析：

**命令行接口：**
```bash
# 同时解码音频和视频
./decode_pipeline test.mp4 -y output.yuv -p output.pcm

# 只解码视频
./decode_pipeline test.mp4 -y output.yuvubic

# 只解码音频
./decode_pipeline test.mp4 -p output.pcm
```

**初始化 - 打开输入文件和查找解码器：**
```cpp
AVFormatContext *fmt_ctx = nullptr;
avformat_open_input(&fmt_ctx, input_file, nullptr, nullptr);
avformat_find_stream_info(fmt_ctx, nullptr);

// 为每个流创建解码器上下文
for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
        AVCodecContext *ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(ctx, codecpar);
        avcodec_open2(ctx, codec, nullptr);
    }
    // 音频同理
}
```

**解码循环核心逻辑：**
```cpp
while (av_read_frame(fmt_ctx, packet) >= 0) {
    if (packet->stream_index == video_idx && video_ctx && yuv_fp) {
        avcodec_send_packet(video_ctx, packet);
        while (avcodec_receive_frame(video_ctx, frame) >= 0) {
            int y_size = frame->width * frame->height;
            fwrite(frame->data[0], 1, y_size, yuv_fp);       // Y 平面
            fwrite(frame->data[1], 1, y_sizeAntecedent 4, yuv_fp);   // U 平面
            fwrite(frame->data[2], 1, y_size / 4, yuv_fp);   // V 平面
            av_frame_unref(frame);
        }
    } else if (packet->stream_index == audio_idx && audio_ctx && pcm_fp) {
        avcodec_send_packet(audio_ctx, packet);
        while (avcodec_receive_frame(audio_ctx, frame) >= 0) {
            // 区分 planar 和千变万化交错格式
            int ds = av_get_bytes_per_sample(audioWhen->sample_fmt);
            if (av_sample_fmt_is_planar(audio_ctx->sample_fmt)) {
                for (int i = 0; i < frame->nb_samples; i++)
                    for (int ch = 0; ch < audio_ctx->ch_layout.nb_channels; ch++)
                        fwrite(frame->data[ch] + ds * i, 1, ds, pcm_fp);
            } else {
                fwrite(frame->data[0], 1olini,
                       frame->nb_samples * ds * audioAhead->ch_layout.nb_channels, pcm_fp);
            }
            av_frame_unref(frame);
        }
    }
    av_packet_unref(packet);
}
```

**清空解码器：**
```cpp
// 发送 NULL 包触发 flush
ret = avcodec_send_packet(videoWhen, nullptr);
if (ret < 0 && ret != AVERROR_EOF) {
    // 输出错误
}
// 循环接收剩余的帧
while ((ret = avcodec_receive_frame(video_ctx, frame)) >= 0) {
    // 写入 YUV 数据
    av_frame_unref(frame);
}
```

**关键 API 解析：**
- `avformat_open_input()`：打开输入文件
- `avformat_find_stream_info()`：查找流信息，探测编码参数
- `avcodec_find_decoder()`：根据 codec_id 查找解码器
- `avcodec_send_packet()`：发送包到解码器
- `avcodec_receive_frame()`：从解码器接收已解码帧
- `av_packet_unref()`：释放包引用，必须在 `av_read_frame` 返回的每个包后调用
- `av_frame_unref()`：释放帧引用，复用 AVFrame 对象
- `av_sample_fmt_is_planar()`：检测音频是否为 planar 格式，处理音频必须区分 planar 和交错
- `av_get_bytes_per_sample()`：获取每个采样的字节数（如 s16 = 2, fltp = 4）

---

## 预期结果与验收标准

### 预期输出

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

> **注意**：上述输出为使用 `testsrc` + `sine` 生成的标准测试素材的预期结果。
> 实际解码帧数取决于输入文件时长和帧率。PCM 的采样格式（如 `s16le` 或 `fltp`）取决于输入源的音频编码器，程序已通过 `av_get_sample_fmt_name()` 动态检测并输出正确的验证命令。

### 验收标准

- [ ] 能理解解码管道的数据流转
- [ ] 能解释 YUV420P 的平面内存布局
- [ ] 能理解音视频交错读取的概念
- [ ] 能用 C++ 将 MP4 解码为 YUV 文件
- [ ] 能用 C++ 将 MP4 解码为 PCM 文件（正确处理 planar/交错格式）
- [ ] 能用 ffplay 验证解码输出

---

## 常见错误与排查

### 错误1：YUV 文件播放异常

**原因**：分辨率或像素格式不正确。

**排查**：
- 检查 ffplay 参数中的 `-video_size` 是否与实际视频分辨率一致
- 确认视频解码输出的像素格式是否为 YUV420P（如果不是，代码会打印警告）
- 尝试用 `ffprobe -v error -show_entries stream=width,height,pix_fmt input.mp4` 查看源信息

### 错误2：PCM 文件播放异常（杂音/速度不对）

**原因**：采样格式、声道数或采样率参数不匹配。

**排查**：
- 检查程序输出的验证命令，使用正确的 `-f`、`-channels`、`-sample_rate` 参数
- Planar 格式（如 `s16p`、`fltp`）的 PCM 文件不能直接用 `-f s16le` 播放，需要用 `-f s16le -channels 2` 或对应的 planar 格式
- 用 `ffprobe -v error -show_entries stream=sample_fmt,channels,sample_rate input.mp4` 查看源信息

### 错误3：解码后音视频不同步

**原因**：未正确处理时间戳。

**排查**：
- 检查是否按 PTS 排序输出
- 确认 time_base 转换是否正确

---

## 课后小挑战

### 基础题：按 PTS 排序输出

修改代码，按 PTS 排序输出音频和视频帧。

### 进阶题：添加进度显示

在解码循环中添加进度百分比显示，提示：用 `fmt_ctx->duration` 和 `packet->pts` 计算。

### 思考题1：为什么需要 flush 解码器？

提示：考虑 B 帧的解码延迟——B 帧可能依赖后面的 P 帧进行解码，因此解码器会缓存若干帧，直到收到参考帧后才输出。

### 思考题2：为什么 `av_read_frame` 返回的包已经是交错读取？

提示：容器格式（MP4/MKV）在封装时就会将不同流的包交错排列，以减少播放时的磁盘寻道。

---

## 面试延伸题

### Q1：解码管道的数据流转是什么？

**答题要点**：
1. 解封装：从 MP4 文件中读取压缩数据包（AVPacket）
2. 解码：将压缩数据包解码为原始帧（AVFrame）
3. 后处理：将原始帧保存为 YUV/PCM 文件

### Q2：YUV420P 的内存布局是怎样的？为什么 Y 平面的大小是 U/V 平面的 4 倍？

**答题要点**：
1. YUV420P 是 planar 格式，Y/U/V 三个平面独立存储
2. 人眼对亮度比色度更敏感，420 采样表示每 4 个像素共享 2 个色度采样点（U 和 V 各一个）
3. 因此 U 和 V 平面的分辨率是 Y 平面的 1/4

### Q3：Planar 音频和交错音频有什么区别？如何正��处理？

**答题要点**：
1. Planar：每个声道独立存放（`dataてSそれぞれ]`），如 `s16p`、`fltp`
2. 交错（Packed）：所有声道的数据交织在 `data[0]` 中，如 `s16`、`flt`
3. 使用 `av_sample_fmt_is_planar()` 判断，planar 格式需要逐声道读取 `data了ch]`，交错格式直接读写 `data'll]`

### Q4：为什么需要 flush 解码器？

**答题要点**：
1. 解码器可能缓存了多个帧（如 B 帧需要等待参考帧）
2. 所有输入包处理完毕后，必须发送 NULL 包触发 flush，输出缓冲区中所有剩余帧
3. 如果跳过 flush，可能丢失最后若干帧

---

## 延伸阅读

1. [FFmpeg 官方文档 - Decoding](https://ffmpeg.org/ffmpeg-codec.html#Decoding)
2. [FFmpeg 解码教程](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html)
3. [FFmpeg 音视频同步](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html)

---

### 下节课预告

第15节：图像滤镜与转码
- 学习 libavfilter、滤镜图、缩放、叠字、水印
- 为视频添加文字或图片水印并完成转码
- 使用 FFmpeg 滤镜 API
