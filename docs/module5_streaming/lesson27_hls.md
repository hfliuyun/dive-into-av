# 第27节：HLS 切片与基础直播链路

## 本节能力目标
- **掌握 HLS 协议原理**：理解 M3U8 索引文件与 TS 切片之间的关系。
- **掌握 FFmpeg HLS Muxer**：学会通过 API 配置切片时长、播放列表长度等关键参数。
- **实现本地 VOD 切片器**：编写 C++ 程序将 MP4 文件转换为标准的 HLS 流。
- **构建全栈播放演示**：结合 H5 播放器（hls.js）完成从服务端切片到前端播放的完整链路验证。

## 真实工程对应场景
- **长视频网站（点播）**：如优酷、腾讯视频，为了适配网络波动和实现秒开，会将视频切片并通过 CDN 分发。
- **现代直播分发**：虽然推流用 RTMP，但在手机端（尤其是 iOS/微信）观看直播时，分发协议通常是 HLS。

## 引言
在上一节课中，我们学习了 RTMP 推流。RTMP 基于 TCP 长连接，实时性好，但它有一个致命弱点：对防火墙不友好，且在移动端网页（HTML5）中兼容性较差。

为了解决这些问题，Apple 公司推出了 **HLS (HTTP Live Streaming)**。HLS 的核心思想非常简单：把视频切成一小段一小段的 `.ts` 文件，然后用一个 `.m3u8` 文本文件记录这些片段的地址。播放器只需要通过标准的 HTTP 协议下载这些文件即可。这种“伪流媒体”方式让 HLS 具有极强的跨平台能力和 CDN 兼容性。

## 理论讲解

### 1. HLS 协议架构
-   **Server (切片器)**：接收媒体流，将其封装为 TS 格式的片段，并更新 M3U8 索引。
-   **Distribution (分发)**：普通的 HTTP Web 服务器（如 Nginx, Apache, Node.js）。
-   **Client (播放器)**：先下载 M3U8，解析出片段列表，然后按顺序下载播放。

### 2. M3U8 文件结构
一个典型的点播 M3U8 如下：
```text
#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:6
#EXT-X-PLAYLIST-TYPE:VOD
#EXTINF:5.040000,
segment_000.ts
#EXTINF:5.040000,
segment_001.ts
#EXT-X-ENDLIST
```
-   `#EXT-X-TARGETDURATION`: 定义最大切片时长。
-   `#EXTINF`: 当前切片的实际时长。
-   `#EXT-X-ENDLIST`: 表示该列表已结束，播放器播完即止。如果没有这一行，播放器会不断请求 M3U8 以获取新片段（即直播模式）。

### 3. FFmpeg HLS 封装器参数
-   `hls_time`: 设定的切片时长（单位：秒）。
-   `hls_list_size`: 索引文件中保留的切片数量。设置为 0 表示保留全部。
-   `hls_segment_filename`: 指定切片文件的命名规则（如 `data_%03d.ts`）。

## 关键点总结
-   **HLS = HTTP + M3U8 + TS**。
-   **高延迟**：由于需要切片缓存，HLS 的延迟通常在 10s-30s。
-   **兼容性王**：几乎所有现代浏览器和移动设备都原生或通过插件支持 HLS。

## 环境与素材准备

### 1. 安装 hls.js (前端环境)
本节课我们将使用 [hls.js](https://github.com/video-dev/hls.js) 在浏览器中测试。无需安装，直接在 HTML 中引入 CDN 即可。

### 2. 测试素材
准备上一节课使用的 `test.mp4`。

## 核心代码实验

### 实验1：实现 C++ HLS 切片器
**目标：** 编写程序将 `input.mp4` 切片输出到 `output/` 目录。

**核心逻辑解析：**
-   **指定格式**：
    ```cpp
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, "hls", out_url);
    ```
-   **传递字典参数**：
    ```cpp
    AVDictionary* options = nullptr;
    av_dict_set(&options, "hls_time", "5", 0); // 5秒一切片
    av_dict_set(&options, "hls_list_size", "0", 0); // 保留全部
    av_dict_set(&options, "hls_segment_filename", "output/seg_%03d.ts", 0);
    avformat_write_header(ofmt_ctx, &options);
    ```

## 预期结果与验收标准

### 预期输出
-   `output/` 目录下出现 `playlist.m3u8`。
-   目录下出现多个 `.ts` 文件。
-   使用 `python3 -m http.server` 启动后，在浏览器访问 `player.html` 能看到视频播放。

### 验收标准
-   M3U8 文件格式正确。
-   切片时长与设定基本相符。
-   浏览器播放无卡顿，Seek（进度条拖动）响应正常。

## 常见错误与排查
-   **切片时长不准**：HLS 切片只能在 **I 帧（关键帧）** 处切开。如果你的视频 GOP 太大（如 10s 一个 I 帧），即便设置 `hls_time` 为 2s，实际切片也会是 10s。
-   **跨域问题**：浏览器播放时如果报错 `CORS`，请确保 HTTP 服务器开启了跨域支持。
-   **404 错误**：检查 M3U8 中记录的 TS 路径与实际文件路径是否一致。

## 课后小挑战
1.  **实现直播模式**：修改参数，使 M3U8 仅保留最近 3 片，并观察播放器的行为（进度条消失，变为直播态）。
2.  **多码率自适应 (ABR)**：研究如何生成“主播放列表 (Master Playlist)”，关联不同分辨率的子流。

## 面试延伸题
-   **问**：为什么 HLS 的延迟比 RTMP 高这么多？
-   **答**：RTMP 基于长连接，包到了就能发。HLS 必须等待一个完整的切片生成并写盘后，才会将其写入 M3U8 索引。播放器通常还要下载 3 个切片后才开始渲染，这就产生了 `hls_time * 3` 左右的固有延迟。

## 延伸阅读
-   [Apple HLS Authoring Specification](https://developer.apple.com/documentation/http-live-streaming/hls-authoring-specification-for-apple-devices)
-   [FFmpeg HLS Muxer Documentation](https://ffmpeg.org/ffmpeg-formats.html#hls-2)
