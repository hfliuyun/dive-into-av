# 第21节：视频播放循环

## 本节能力目标
- **掌握视频刷新控制原理**：理解为什么视频渲染需要精确的时间间隔控制。
- **学会使用 SDL3 定时器**：掌握 `SDL_AddTimer` 与自定义事件（Custom Event）的结合使用。
- **构建事件驱动播放模型**：建立“定时器触发 -> 消息队列 -> 解码渲染”的现代播放器模型。
- **实现纯视频播放器**：结合 FFmpeg 解码与 SDL3 渲染，完成一个能够按正确帧率播放的播放器。

## 真实工程对应场景
- **所有主流播放器核心循环**：FFplay、VLC 等播放器底层都会维护一个时钟或定时器，根据视频的 FPS 或同步基准来触发每一帧的渲染。
- **视频监控系统**：多路监控画面需要按照预设的帧率进行规律刷新。

## 引言
在前面的课程中，我们学会了如何渲染一帧 YUV 图片，也学会了如何让声卡播放一段音频。那么，如何让一串连续的视频帧动起来，并变成一段看起来“自然”的视频呢？

初学者最容易犯的错误是在一个死循环里不停地解码和渲染：
```cpp
while (true) {
    read_packet();
    decode_frame();
    render_frame();
}
```
这样做会导致视频以 CPU 所能达到的最快速度播放（由于现在的 CPU 性能极强，几分钟的视频可能几秒钟就播完了），且由于没有给系统留出空隙，会导致 UI 界面完全卡死。

本节课我们将引入 **SDL3 定时器与自定义事件** 机制，构建一个呼吸感十足、性能合理的视频播放循环。

## 理论讲解

### 1. 刷新节奏 (Refresh Rhythm)
视频播放的本质是在单位时间内显示固定数量的图片。例如 25fps 的视频，每两帧之间的时间间隔应为 $1000 / 25 = 40ms$。

### 2. 基于事件驱动的渲染模型
为了不阻塞主线程（UI 线程）并保持稳定的刷新频率，我们采用以下架构：
1. **主线程**：运行 SDL 事件循环 (`SDL_WaitEvent`)，等待各种输入。
2. **定时器线程**：SDL 内部维护的线程，根据设定的时间（如 40ms）定期执行回调函数。
3. **自定义事件**：定时器回调函数不直接进行复杂的渲染，而是向主线程的消息队列推送一个自定义的“刷新信号”（如 `REFRESH_EVENT`）。
4. **触发解码**：主线程收到刷新信号后，才执行一次“读包 -> 解码 -> 渲染”的操作。

### 3. 解码延迟与 B 帧（重要）
在实验代码中，你会发现一个 `while` 循环在处理 `AVERROR(EAGAIN)`。这是因为现代编码标准（如 H.264）引入了 **B 帧**。B 帧的解码依赖于其前后的参考帧，因此当你向解码器发送一个包（Packet）时，它可能无法立即吐出一帧（Frame）。你需要连续发送多个包，解码器缓存足够信息后才会开始输出。这就是为什么我们需要在刷新信号到达时，通过循环不断“喂数据”直到解码器吐出一帧可供显示的画面。

### 4. SDL3 自定义事件
在 SDL3 中，我们可以通过以下方式注册并推送自定义事件：
- **注册**：`uint32_t REFRESH_EVENT = SDL_RegisterEvents(1);`
- **推送**：在回调中构造一个 `SDL_Event` 并调用 `SDL_PushEvent(&event);`。

## 关键点总结
- **千万不要用死循环渲染**。
- **SDL_AddTimer** 是实现稳定帧率的关键。
- **事件驱动模型** 是开发高性能 UI 和多媒体应用的基础。

## 环境与素材准备

### 环境要求
- 已安装 SDL3 与 FFmpeg 开发库。

### 测试素材
使用 FFmpeg 生成一个 10 秒、25fps、无声的测试视频：
```bash
ffmpeg -f lavfi -i testsrc=duration=10:size=640x360:rate=25 -c:v libx264 -pix_fmt yuv420p -an test.mp4
```

## 核心代码实验

### 实验1：实现纯视频播放器
**目标：** 实现一个 C++ 程序，能够按 25fps 的速度播放 `test.mp4` 中的视频内容。

**核心逻辑解析：**
- **计算延时**：
  ```cpp
  double fps = av_q2d(video_stream->avg_frame_rate);
  uint32_t interval = (uint32_t)(1000.0 / fps);
  ```
- **设置定时器**：
  ```cpp
  SDL_AddTimer(interval, [](void* userdata, SDL_TimerID timerID, uint32_t interval) -> uint32_t {
      SDL_Event event;
      SDL_zero(event); // SDL3 推荐清零
      event.type = REFRESH_EVENT;
      SDL_PushEvent(&event);
      return interval; 
  }, nullptr);
  ```
- **事件循环处理**：
  ```cpp
  while (SDL_WaitEvent(&event)) {
      if (event.type == REFRESH_EVENT) {
          // 循环尝试解码，直到吐出一帧
          while(avcodec_receive_frame(dec_ctx, frame) == AVERROR(EAGAIN)) {
              av_read_frame(fmt_ctx, pkt);
              avcodec_send_packet(dec_ctx, pkt);
          }
          // 更新纹理并 RenderTexture
      } else if (event.type == SDL_EVENT_QUIT) {
          break;
      }
  }
  ```

## 预期结果与验收标准

### 预期输出
- 弹出一个播放窗口。
- 窗口中的视频画面平滑滚动（测试素材通常为计时器或彩条）。
- 窗口标题实时显示当前的播放状态。

### 验收标准
- 播放速度与视频 FPS 匹配（肉眼观察不快进、不慢动作）。
- 能够正常关闭窗口并安全退出。
- 无内存泄漏。

## 常见错误与排查
- **视频播一下就停了**：检查 FFmpeg 解码循环中是否正确处理了 `AVERROR(EAGAIN)` 状态。
- **画面闪烁**：确保渲染逻辑（Clear -> Texture -> Present）完整且顺序正确。
- **退出时崩溃**：检查是否在释放 FFmpeg Context 后依然有定时器事件尝试推送。建议退出循环后立即 `SDL_RemoveTimer`。

## 课后小挑战
1. **思考 PTS 的意义**：本节课我们假设视频是恒定帧率（CFR）且没有考虑帧本身的显示时间戳（PTS）。尝试在渲染前打印 `frame->pts`，观察它与定时器触发时间的对应关系。
2. **实现倍速播放**：尝试通过按键修改定时器的间隔，实现 0.5x 或 2.0x 播放。

## 面试延伸题
- **问**：为什么 SDL 定时器回调函数里不建议直接写复杂的渲染代码？
- **答**：因为 SDL 定时器是在独立的辅助线程中运行的。大多数系统的图形 API（如 Windows GDI, macOS Metal）要求所有渲染操作必须在主线程（创建窗口的那个线程）中执行。在回调中渲染会导致不可预知的崩溃或黑屏。
- **问**：在主线程直接调用 `av_read_frame` 有什么隐患？
- **答**：`av_read_frame` 是一个阻塞的 I/O 操作。如果网络流抖动或磁盘读取缓慢，主线程会卡死在读操作上，导致整个 UI（包括窗口拖动、缩放）无响应。在生产级播放器中，读包、解码、渲染通常是在不同的线程中异步进行的（即“生产-消费”模型）。

## 延伸阅读
- [SDL3 Timer API](https://wiki.libsdl.org/SDL3/SDL_AddTimer)
- [FFmpeg Decoding Example](https://ffmpeg.org/doxygen/trunk/decode_video_8c-example.html)
