# 第16节：音频重采样与格式转换

| 字段 | 内容 |
|------|------|
| lesson_id | 16 |
| module | module3_pipeline |
| type | cpp_ffmpeg |
| prerequisites | 第1-3节、第9-15节 |
| outputs | 理解 libswresample，能用 C++ 实现采样率转换、声道转换、格式转换 |
| estimated_time | 4-5 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test_audio.mp4 |
| source | src/module3/lesson16_audio_resample/ |

## 本节能力目标

- 理解 libswresample 重采样上下文 (SwrContext) 的作用
- 能用 C++ 实现采样率转换 (48kHz → 44.1kHz)
- 能用 C++ 实现声道转换 (立体声 → 单声道)
- 能用 C++ 实现采样格式转换 (S16 → FLT → S16P)
- 理解 planar 和 interleaved 采样格式的区别

## 真实工程对应场景

- 音频转码服务：调整采样率以适配不同设备
- 语音通话：降噪后转换为单声道以节省带宽
- 音频编辑：统一采样格式以便后续处理
- 视频播放器：重采样以适配音频输出设备

## 引言

在第15节中，我们学习了使用 libavfilter 对视频进行滤镜处理。但实际应用中，音频同样需要处理——比如将 48kHz 的高清音频转换为 CD 质量的 44.1kHz，或将立体声转换为单声道以节省存储空间。

本节课我们将学习 **libswresample** 音频重采样库。你将用 C++ 构建音频重采样管道，实现采样率转换、声道转换和采样格式转换。

---

## 理论讲解

### 核心概念1：SwrContext 重采样上下文

`SwrContext` 是 FFmpeg 音频重采样的核心结构体，包含了输入/输出的所有参数：

```c
SwrContext *swr = swr_alloc();
av_opt_set_int(swr, "in_sample_rate", 48000, 0);
av_opt_set_int(swr, "out_sample_rate", 44100, 0);
av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
swr_init(swr);
```

### 核心概念2：重采样流程（5步）

```c
// 1. 分配重采样上下文
SwrContext *swr = swr_alloc();

// 2. 设置输入参数
av_opt_set_int(swr, "in_sample_rate", 48000, 0);
av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
av_channel_layout_from_mask(&in_layout, AV_CH_LAYOUT_STEREO);
av_opt_set_channel_layout(swr, "in_channel_layout", &in_layout, 0);

// 3. 设置输出参数
av_opt_set_int(swr, "out_sample_rate", 44100, 0);
av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16P, 0);
av_channel_layout_from_mask(&out_layout, AV_CH_LAYOUT_MONO);
av_opt_set_channel_layout(swr, "out_channel_layout", &out_layout, 0);

// 4. 初始化
swr_init(swr);

// 5. 执行转换并释放
int out_samples = swr_convert(swr, out_data, out_max_samples, in_data, in_samples);
swr_free(&swr);
```

### 核心概念3：Planar vs Interleaved 采样格式

FFmpeg 音频帧有两种存储方式：

- **Interleaved（交错）**：所有声道的数据交织在 `data[0]` 中
  - 例：`LLLLRRRR` (L=左声道，R=右声道)
  - 格式：`S16`、`FLT`、`S32`

- **Planar（平面）**：每个声道独立存放在 `data[ch]` 中
  - 例：`LLLL` 存 `data[0]`，`RRRR` 存 `data[1]`
  - 格式：`S16P`、`FLTP`、`S32P`

```c
// 检测格式
int is_planar = av_sample_fmt_is_planar(fmt);

// 平面格式写入
for (int i = 0; i < nb_samples; i++)
    for (int ch = 0; ch < channels; ch++)
        fwrite(data[ch] + i * bytes_per_sample, bytes_per_sample, 1, fp);

// 交错格式写入
fwrite(data[0], nb_samples * channels * bytes_per_sample, 1, fp);
```

### 核心概念4：声道布局 (Channel Layout)

声道布局定义了各声道的位置和含义：

```c
AVChannelLayout mono = AV_CH_LAYOUT_MONO;       // 1声道 (C)
AVChannelLayout stereo = AV_CH_LAYOUT_STEREO;   // 2声道 (L R)
AVChannelLayout layout5_1 = AV_CH_LAYOUT_5_1;   // 6声道 (C L R Ls Rs LFE)

// 从 mask 创建
AVChannelLayout layout;
av_channel_layout_from_mask(&layout, AV_CH_LAYOUT_STEREO);
```

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **SwrContext** | 重采样上下文，包含所有输入/输出参数 |
| **5步流程** | alloc → set in → set out → init → convert |
| **采样率转换** | 改变数字音频的时间密度（48k→44.1k） |
| **声道转换** | 改变空间维度（立体声→单声道） |
| **格式转换** | 改变精度和数据排列（S16→FLTP） |
| **Planar vs Interleaved** | 平面格式独立存储每个声道，交错格式交织存储 |

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
sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswresample-dev cmake g++
```

### 测试素材

```bash
# 生成测试音频（5秒，440Hz正弦波）
ffmpeg -f lavfi -i "sine=frequency=440:duration=5" -ar 48000 -ac 2 -c:a aac -b:a 128k -y test_audio.mp4
```

---

## 核心代码实验

完整的代码实现请参见 `src/module3/lesson16_audio_resample/main.cpp`。

### 实验1：采样率转换

**目标**：将 48kHz 音频转换为 44.1kHz

```bash
./audio_resample test_audio.mp4 output.mp4 -ar 44100
```

### 实验2：声道转换

**目标**：将立体声转换为单声道

```bash
./audio_resample test_audio.mp4 output.mp4 -ac 1
```

### 实验3：采样格式转换

**目标**：转换为浮点 planar 格式

```bash
./audio_resample test_audio.mp4 output.mp4 -af fltp
```

### 实验4：组合转换

**目标**：48kHz立体声 → 44.1kHz单声道S16

```bash
./audio_resample test_audio.mp4 output.mp4 -ar 44100 -ac 1 -af s16
```

以下为关键代码片段解析：

**重采样上下文初始化：**

```cpp
SwrContext *swr = swr_alloc();

// 设置输入参数
av_channel_layout_from_mask(&in_layout, 
    dec_ctx->ch_layout.nb_channels > 0 ? dec_ctx->ch_layout.u.mask : AV_CH_LAYOUT_STEREO);
av_opt_set_channel_layout(swr, "in_channel_layout", &in_layout, 0);
av_opt_set_int(swr, "in_sample_rate", dec_ctx->sample_rate, 0);
av_opt_set_sample_fmt(swr, "in_sample_fmt", dec_ctx->sample_fmt, 0);

// 设置输出参数
av_channel_layout_from_mask(&out_layout, 
    out_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO);
av_opt_set_channel_layout(swr, "out_channel_layout", &out_layout, 0);
av_opt_set_int(swr, "out_sample_rate", out_sample_rate, 0);
av_opt_set_sample_fmt(swr, "out_sample_fmt", out_sample_fmt, 0);

// 初始化
swr_init(swr);
```

**执行重采样：**

```cpp
int out_samples = swr_convert(swr,
    resampled_frame->data,     // 输出数据指针数组
    max_out_samples,           // 输出缓冲区最大样本数
    (const uint8_t **)frame->data,  // 输入数据
    frame->nb_samples);        // 输入样本数

if (out_samples < 0) {
    // 处理错误
}
resampled_frame->nb_samples = out_samples;
```

**关键 API 解析：**

- `swr_alloc()` — 分配重采样上下文
- `av_channel_layout_from_mask()` — 从掩码创建声道布局
- `av_opt_set_int()` / `av_opt_set_sample_fmt()` — 设置参数
- `av_opt_set_channel_layout()` — 设置声道布局
- `swr_init()` — 初始化重采样器
- `swr_convert()` — 执行重采样转换
- `swr_get_out_samples()` — 获取剩余输出样本数（用于 flush）
- `swr_close()` — 关闭重采样器
- `swr_free()` — 释放重采样上下文

---

## 预期结果与验收标准

### 预期输出

```
Audio Resampling Pipeline:
  Input:  48000Hz, 2ch, s16
  Output: 44100Hz, 1ch, s16
  Encoder: aac

Resampling done:
  Output frames: 216
  Output file: output.mp4
  Verify: ffplay output.mp4
```

### 验收标准

- [ ] 能解释 SwrContext 的作用和 5 步构建流程
- [ ] 能实现采样率转换（48kHz → 44.1kHz）
- [ ] 能实现声道转换（立体声 → 单声道）
- [ ] 能实现采样格式转换（S16 → FLTP）
- [ ] 能区分 planar 和 interleaved 格式并正确处理

---

## 常见错误与排查

### 错误1：声道布局设置失败

**原因**：`ch_layout.u.mask` 未正确初始化。

**排查**：
- 使用 `av_channel_layout_from_mask()` 显式创建布局
- 或在解码器未提供时使用默认值 `AV_CH_LAYOUT_STEREO`

### 错误2：重采样输出样本数为0

**原因**：输出缓冲区可能不足或输入样本格式不匹配。

**排查**：
- 检查 `max_out_samples` 是否足够大（建议 1024 或更大）
- 确认输入/输出采样格式设置正确

### 错误3：内存泄漏

**原因**：未调用 `swr_free()` 释放上下文。

**排查**：
- 确保所有退出路径都调用 `swr_free(&swr)`
- 使用 ASAN 检测泄漏

---

## 课后小挑战

### 基础题：可变采样率

修改代码，支持任意输入采样率转换为指定输出采样率。

### 进阶题：批量重采样

实现批量读取音频帧、重采样、编码的流水线，观察性能。

### 思考题1：为什么重采样会改变音频时长？

提示：采样率改变意味着每秒样本数改变，总时长 = 总样本数 / 采样率。

### 思考题2：如何实现音频时间拉伸（不改变音调）？

提示：可使用 FFmpeg 的 `atempo` 音频滤镜，或使用专门的库如 `librubberband`、`SoundTouch`。`libavresample` 在 FFmpeg 5.0+ 已废弃。

---

## 面试延伸题

### Q1：SwrContext 的构建流程是什么？

**答题要点**：
1. `swr_alloc()` 分配上下文
2. 设置输入参数（采样率、格式、声道布局）
3. 设置输出参数
4. `swr_init()` 初始化
5. `swr_convert()` 执行转换

### Q2：Planar 和 Interleaved 格式有什么区别？

**答题要点**：
- Planar：每个声道独立存储在 `data[ch]` 中，如 `S16P`、`FLTP`
- Interleaved：所有声道交织在 `data[0]` 中，如 `S16`、`FLT`
- 使用 `av_sample_fmt_is_planar()` 检测

### Q3：重采样时如何处理声道布局转换？

**答题要点**：
- 使用 `av_channel_layout_from_mask()` 创建目标布局
- 通过 `av_opt_set_channel_layout()` 设置 in/out 参数
- 支持 downmix (5.1→stereo) 和 upmix (stereo→5.1)

---

## 延伸阅读

1. [FFmpeg 官方文档 - libswresample](https://ffmpeg.org/libswresample.html)
2. [FFmpeg Resampling Documentation](https://ffmpeg.org/ffmpeg-resampler.html)
3. [Audio Channel Masks](https://ffmpeg.org/ffmpeg-resampler.html#Audio-Channel-Layout)

---

### 下节课预告

第17节：转封装、流拷贝与精确剪辑
- 学习 stream copy、seek、关键帧约束
- 对比"秒开剪辑"和"必须重编码"的场景差异
