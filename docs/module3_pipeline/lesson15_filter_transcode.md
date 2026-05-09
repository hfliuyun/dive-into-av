# 第15节：图像滤镜与转码

| 字段 | 内容 |
|------|------|
| lesson_id | 15 |
| module | module3_pipeline |
| type | cpp_ffmpeg |
| prerequisites | 第1-10节、第12-14节 |
| outputs | 理解 libavfilter 滤镜图，能用 C++ 实现缩放、水印等滤镜并转码 |
| estimated_time | 4-5 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.mp4 |
| source | src/module3/lesson15_filter_transcode/ |

## 本节能力目标

- 理解 libavfilter 滤镜图的架构和构建流程
- 理解 buffer src / buffersink 作为滤镜图端点的作用
- 能用 C++ 实现视频缩放滤镜
- 能用 C++ 实现文字水印滤镜
- 能组合多个滤镜并完成视频转码

## 真实工程对应场景

- 视频转码服务：解码后的视频在编码前经过滤镜链处理
- 视频编辑工具：添加水印、调色、裁剪等功能
- 视频预处理：统一分辨率、像素格式、帧率
- 直播推流：实时缩放、水印叠加

## 引言

在第14节中，我们构建了完整的解码管道，将 MP4 解码为 YUV/PCM。但实际工程中，解码后的原始帧通常需要进一步处理才能编码输出——比如调整分辨率、添加水印、翻转画面。这些操作由 FFmpeg 的 **libavfilter** 库完成。

本节课我们将学习 **libavfilter 滤镜图**。你将用 C++ 构建滤镜管道，对视频进行缩放、加水印、翻转等处理，再编码输出为新的 MP4 文件。

---

## 理论讲解

### 核心概念1：滤镜图 (Filter Graph)

滤镜图是一个有向无环图（DAG），定义了音视频帧的处理流程：

```
解码帧 → [buffer src] → [滤镜1] → [滤镜2] → ... → [buffersink] → 编码器
```

- **buffer src**：滤镜图的输入端，接收解码后的 AVFrame
- **中间滤镜**：如 scale（缩放）、drawtext（文字水印）、hflip/vflip（翻转）
- **buffersink**：滤镜图的输出端，输出处理后的 AVFrame

滤镜通过字符串描述，用逗号连接：

```
scale=1280:720                              # 缩放
drawtext=text='Hello':fontsize=32:x=10:y=10 # 文字水印
scale=640:360,drawtext=text='AV':x=w-tw-10:y=10  # 先缩小再加右上角水印
```

### 核心概念2：滤镜图构建流程

构建滤镜图的标准流程（5步）：

```c
// 1. 分配滤镜图
AVFilterGraph *graph = avfilter_graph_alloc();

// 2. 创建 buffer src（输入端点）
AVFilterContext *src_ctx;
avfilter_graph_create_filter(&src_ctx, avfilter_get_by_name("buffer"),
    "in", "video_size=640x360:pix_fmt=0:time_base=1/25", NULL, graph);

// 3. 创建 buffersink（输出端点）
AVFilterContext *sink_ctx;
avfilter_graph_create_filter(&sink_ctx, avfilter_get_by_name("buffersink"),
    "out", NULL, NULL, graph);

// 4. 解析滤镜描述字符串，连接节点
AVFilterInOut *inputs = avfilter_inout_alloc();
AVFilterInOut *outputs = avfilter_inout_alloc();
outputs->name = av_strdup("in");  outputs->filter_ctx = src_ctx;
inputs->name  = av_strdup("out"); inputs->filter_ctx  = sink_ctx;
avfilter_graph_parse_ptr(graph, "scale=1280:720", &inputs, &outputs, NULL);

// 5. 配置并验证滤镜图
avfilter_graph_config(graph, NULL);
```

### 核心概念3：滤镜图中的数据流转

帧在滤镜图中的流向：

```c
// 将解码帧送入滤镜图
av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);

// 从滤镜图获 取处理后的帧
while (av_buffersink_get_frame(buffersink_ctx, filtered_frame) >= 0) {
    // filtered_frame 已应用所有滤镜
    av_frame_unref(filtered_frame);
}
```

### 核心概念4：转码流水线

完整转码流程 = 解码 → 滤镜 → 编码 → 封装：

```
输入文件 → 解封装 → AVPacket → 解码 → AVFrame → 滤镜图 → AVFrame → 编码 → AVPacket → 封装 → 输出文件
```

本节课的代码实现了这条完整流水线。

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **滤镜图是 DAG** | buffer src → 中间滤镜 → buffersink |
| **buffer src** | 输入端点，接收解码后的 AVFrame |
| **buffersink** | 输出端点，输出处理后的 AVFrame |
| **滤镜描述字符串** | 用逗号连接多个滤镜，如 "scale=1280:720,drawtext=..." |
| **转码流水线** | 解封装→解码→滤镜→编码→封装 |

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
sudo apt install libavcodec-dev libavformat-dev libavutil-dev libavfilter-dev cmake g++
```

### 测试素材

```bash
# 生成测试视频（5秒，640x360，25fps）
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=25 \
       -c:v libx264 -y test.mp4
```

---

## 核心代码实验

完整的代码实现请参见 `src/module3/lesson15_filter_transcode/main.cpp`。

### 实验1：视频缩放

**目标**：将 640x360 的视频放大到 1280x720，重新编码

```bash
./filter_transcode test.mp4 scaled.mp4 scale 1280:720
```

### 实验2：文字水印

**目标**：在视频上添加自定义文字水印

```bash
./filter_transcode test.mp4 watermarked.mp4 text Hello:32:10:10
```

### 实验3：水平/垂直翻转

```bash
./filter_transcode test.mp4 hflipped.mp4 hflip
./filter_transcode test.mp4 vflipped.mp4 vflip
```

### 实验4：组合滤镜

```bash
# 先缩小到 640x360，再在右上角添加 "AV" 水印
./filter_transcode test.mp4 custom.mp4 "scale=640:360,drawtext=text='AV':fontsize=32:x=w-tw-10:y=10:fontcolor=white:box=1:boxcolor=black@0.5"
```

以下为关键代码片段解析：

**滤镜图初始化（init_filter_graph 函数）：**

```cpp
// buffer src 参数描述输入帧的格式
char args[512];
snprintf(args, sizeof(args),
         "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
         dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
         dec_ctx->time_base.num, dec_ctx->time_base.den,
         dec_ctx->sample_aspect_ratio.num,
         dec_ctx->sample_aspect_ratio.den > 0
             ? dec_ctx->sample_aspect_ratio.den : 1);

// 创建输入/输出端点
avfilter_graph_create_filter(&src_ctx, avfilter_get_by_name("buffer"),
    "in", args, nullptr, graph);
avfilter_graph_create_filter(&sink_ctx, avfilter_get_by_name("buffersink"),
    "out", nullptr, nullptr, graph);

// 使用 AVFilterInOut 连接端点
AVFilterInOut *outputs = avfilter_inout_alloc();
AVFilterInOut *inputs  = avfilter_inout_alloc();
outputs->filter_ctx = src_ctx;
inputs->filter_ctx  = sink_ctx;

avfilter_graph_parse_ptr(graph, filter_desc, &inputs, &outputs, nullptr);
avfilter_graph_config(graph, nullptr);
```

**编码器参数从滤镜输出获取：**

```cpp
// 编码器分辨率必须与滤镜输出一致！
enc_ctx->width   = av_buffersink_get_w(buffersink_ctx);
enc_ctx->height  = av_buffersink_get_h(buffersink_ctx);
enc_ctx->pix_fmt = (AVPixelFormat)av_buffersink_get_format(buffersink_ctx);
```

**转码循环核心逻辑：**

```cpp
while (av_read_frame(fmt_ctx, packet) >= 0) {
    avcodec_send_packet(dec_ctx, packet);
    while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
        // 送入滤镜图
        av_buffersrc_add_frame_flags(buffersrc_ctx, frame,
                                     AV_BUFFERSRC_FLAG_KEEP_REF);

        // 从滤镜图取出并编码
        while (av_buffersink_get_frame(buffersink_ctx, filtered_frame) >= 0) {
            avcodec_send_frame(enc_ctx, filtered_frame);
            while (avcodec_receive_packet(enc_ctx, enc_pkt) >= 0) {
                av_packet_rescale_ts(enc_pkt, enc_ctx->time_base,
                                     out_stream->time_base);
                av_interleaved_write_frame(ofmt_ctx, enc_pkt);
                av_packet_unref(enc_pkt);
            }
            av_frame_unref(filtered_frame);
        }
        av_frame_unref(frame);
    }
    av_packet_unref(packet);
}
```

**关键 API 解析：**

- `avfilter_graph_alloc()` — 分配滤镜图
- `avfilter_get_by_name()` — 按名称获取滤镜定义（"buffer", "buffersink", "scale", "drawtext" 等）
- `avfilter_graph_create_filter()` — 创建滤镜实例并加入图中
- `avfilter_inout_alloc()` — 分配滤镜图 I/O 端点描述符
- `avfilter_graph_parse_ptr()` — 解析滤镜描述字符串，创建中间节点并连接
- `avfilter_graph_config()` — 验证并完成滤镜图配置（必须调用）
- `av_buffersrc_add_frame_flags()` — 将 AVFrame 送入滤镜图
- `av_buffersink_get_frame()` — 从滤镜图取出处理后的 AVFrame
- `avfilter_graph_free()` — 释放整个滤镜图（包括所有节点）

---

## 预期结果与验收标准

### 预期输出

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

> **注意**：输出帧数取决于输入视频时长和帧率。使用 testsrc 5秒 25fps 视频，预期约 125 帧。实际的帧率和输出分辨率由滤镜图决定。

### 验收标准

- [ ] 能解释滤镜图的架构（buffer src → 滤镜 → buffersink）
- [ ] 能独立构建包含 scale 滤镜的转码管道
- [ ] 能独立构建包含 drawtext 滤镜的转码管道
- [ ] 能组合多个滤镜（如 scale + drawtext）
- [ ] 能用 ffplay 验证转码输出

---

## 常见错误与排查

### 错误1：drawtext 滤镜初始化失败（字体未找到）

**原因**：系统未安装字体文件或字体路径不正确。

**排查**：
- macOS: `brew install fontconfig`
- 先使用不需要字体的滤镜（scale、hflip、vflip）
- 检查可用字体：`fc-list`

### 错误2：编码输出帧数不符合预期

**原因**：flush 流程不完整。

**排查**：
- 确认主循环结束后，先 flush 解码器（发送 NULL 包），再 flush 编码器
- 确认 `av_write_trailer()` 在 flush 编码器之后调用

### 错误3：输出画面花屏或颜色异常

**原因**：编码器像素格式与滤镜输出不匹配。

**排查**：
- 编码器 pix_fmt 必须使用 `av_buffersink_get_format()` 获取
- 打印滤镜前后的像素格式，确认转换是否正确

---

## 课后小挑战

### 基础题：尝试不同的缩放分辨率

将视频缩小到 320x180，观察输出文件大小变化。思考：为什么缩小的视频文件更小？

### 进阶题：图片水印

修改代码，添加 `movie` 滤镜 + `overlay` 滤镜支持图片水印（PNG logo）。提示：`movie=logo.png[wm];[in][wm]overlay=W-w-10:10[out]`。

### 挑战题：添加动态时间码

使用 drawtext 滤镜显示当前播放时间。提示：FFmpeg drawtext 支持 `%{pts:hms}` 变量。

### 思考题1：为什么需要用 AV_BUFFERSRC_FLAG_KEEP_REF？

提示：考虑 AVFrame 的引用计数机制。如果不保留引用，解码器可能在下一次 `avcodec_receive_frame()` 时复用同一内存区域。

### 思考题2：滤镜图如何处理音频帧？

提示：libavfilter 同时支持音频滤镜（如 aresample、volume），但需要使用 abuffer/abuffersink 端点，参数格式为 "sample_rate=...:sample_fmt=...:channel_layout=..."。

---

## 面试延伸题

### Q1：libavfilter 滤镜图的构建流程是什么？

**答题要点**：
1. `avfilter_graph_alloc()` 分配图
2. 创建 buffer src 和 buffersink 端点
3. `avfilter_graph_parse_ptr()` 解析滤镜描述字符串
4. `avfilter_graph_config()` 配置验证
5. 使用时 `av_buffersrc_add_frame_flags()` 输入帧，`av_buffersink_get_frame()` 输出帧

**答题要点**：
1. 从解码器上下文获取：`dec_ctx->width`、`dec_ctx->height`、`dec_ctx->pix_fmt`、`dec_ctx->time_base`
2. buffer src 必须知道输入帧的格式才能正确分配内部缓冲区
3. 参数不匹配会导致 `av_buffersrc_add_frame_flags()` 返回错误

### Q3：如何同时处理音频和视频滤镜？

**答题要点**：
1. 分别创建视频滤镜图（buffer/buffersink）和音频滤镜图（abuffer/abuffersink）
2. 循环中按 stream_index 区分，视频帧走视频滤镜图，音频帧走音频滤镜图
3. 各自编码后交错写入输出文件（`av_interleaved_write_frame`）

---

## 延伸阅读

1. [FFmpeg 官方文档 - libavfilter](https://ffmpeg.org/libavfilter.html)
2. [FFmpeg Filters Documentation](https://ffmpeg.org/ffmpeg-filters.html)
3. [FFmpeg Filtering Guide](https://trac.ffmpeg.org/wiki/FilteringGuide)

---

### 下节课预告

第16节：音频重采样与格式转换
- 学习 libswresample、采样率转换、声道映射、采样格式
- 将 48kHz 立体声转换为 44.1kHz 单声道
