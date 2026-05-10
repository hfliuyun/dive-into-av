# 第24节：播放控制与播放器排障

## 本节能力目标
- **掌握播放器状态机设计**：学会如何管理播放、暂停、跳转（Seek）等交互状态。
- **掌握精准跳转（Seek）的完整流程**：深刻理解为什么 Seek 后必须进行“双重 Flush”（队列 + 解码器）。
- **建立系统的排障思路**：能够根据现象（如花屏、杂音、卡顿）快速定位音视频工程中的典型 Bug。
- **提升多线程交互能力**：学会在多线程环境下安全地分发和处理用户输入事件。

## 真实工程对应场景
- **用户交互层实现**：所有商业播放器（腾讯视频、B站、TikTok）的播放/暂停、进度条拖动逻辑均基于此。
- **线上问题排障**：作为音视频工程师，80% 的工作是在排查为什么某个特定的视频在某些设备上播放异常。

## 引言
恭喜你！我们已经攻克了多线程架构和音视频同步这两座大山。现在的播放器已经能够平稳地“跑”起来了。但作为一个真正的软件产品，它还需要能听懂用户的指令：用户想停的时候得停，想跳着看的时候得跳。

本节课是模块四的收官之战。我们将赋予播放器“灵魂”——交互控制，并总结一套价值千金的播放器排障指南。

## 理论讲解

### 1. 播放器状态机 (State Machine)
为了防止逻辑混乱（例如在没打开文件时点击暂停，或者在退出时点击跳转），我们需要定义清晰的状态：
- **IDLE**: 初始状态。
- **PLAYING**: 正常解码渲染。
- **PAUSED**: 解码和渲染暂停，但时钟保持。
- **SEEKING**: 正在执行跳转，此时应临时屏蔽刷新信号。

### 2. 精准跳转 (Seek) 的三步走
Seek 是音视频开发中最容易出 Bug 的地方。正确的 Seek 必须遵循以下流程：
1.  **文件定位与序列化**：调用 `avformat_seek_file`。同时，增加一个全局的 `g_seek_serial` 计数器。所有后续读取的 Packet 和生成的 Frame 都带上这个序列号。
2.  **清空队列与缓冲区**：
    -   丢弃队列里旧序列号的所有数据。
    -   **清理 SDL 音频缓冲区**：调用 `SDL_ClearAudioStream` 抹除声卡中残余的旧声音，防止“幻听”现象。
3.  **重置解码器 (Codec Flush)**：**必须线程安全**。不要在读线程里强行 Flush。应由解码线程在发现序列号改变时，在自己的上下文中调用 `avcodec_flush_buffers`。

### 3. 播放器排障指南 (Troubleshooting)
| 现象 | 可能原因 | 排查方向 |
| :--- | :--- | :--- |
| **花屏/绿屏** | 丢失关键帧、解码器未 Flush、色彩空间转换错误 | 检查 Seek 序列号处理、检查 YUV 步长 |
| **交互卡顿** | 频繁 Seek 导致队列阻塞、主线程执行阻塞操作 | 使用 `atomic` 标志位、异步处理 Seek 请求 |
| **音画不同步** | 时钟基准选错、PTS 换算溢出、系统调度延迟 | 监控 A-V Diff 值、检查 TimeBase 换算 |
| **退出时卡死** | 线程死锁、条件变量未唤醒 | 检查 `queue.abort()` 是否在 `thread.join()` 之前 |
| **内存持续上涨** | 引用计数未释放 | 检查 `av_packet_unref` 是否在所有退出路径执行 |

## 关键点总结
- **交互不仅仅是 UI，更是对底层流水线的调度**。
- **Flush 是 Seek 的灵魂**。
- **多看日志，多用 ASAN**。

## 环境与素材准备
沿用上一节的 `sync_test.mp4`。

## 核心代码实验

### 实验1：实现带交互的同步播放器
**目标：** 在第 23 节代码基础上，支持空格键暂停，左右方向键跳转 $\pm 5s$。

**核心逻辑解析：**
- **处理按键事件**：
    ```cpp
    case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_SPACE) toggle_pause();
        if (event.key.key == SDLK_LEFT)  request_seek(-5.0);
        if (event.key.key == SDLK_RIGHT) request_seek(5.0);
    ```
- **Seek 执行逻辑 (读线程)**：
    ```cpp
    void read_thread() {
        if (g_seek_pos >= 0) {
            avformat_seek_file(fmt_ctx, -1, ..., AVSEEK_FLAG_BACKWARD);
            g_seek_serial++; // 增加序列号
            pkt_queue.flush(); // 清空旧数据
            SDL_ClearAudioStream(a_stream);
        }
    }
    ```
- **解码线程 (安全 Flush)**：
    ```cpp
    void decode_thread() {
        if (pkt.serial != last_serial) {
            avcodec_flush_buffers(dec_ctx); // 解码线程内部安全刷新
            last_serial = pkt.serial;
        }
    }
    ```

## 预期结果与验收标准

### 预期输出
- 按下空格，画面瞬间定格，再次按下恢复播放。
- 按下左右键，进度条（如果实现了）或控制台输出的 PTS 发生跳转，画面在短暂闪烁后恢复正常同步。

### 验收标准
- 交互响应时间 $< 200ms$。
- Seek 后 2 帧内画面必须恢复正常（不花屏）。
- 连续疯狂 Seek 不会导致程序崩溃。

## 课后小挑战
1.  **实现逐帧播放**：在暂停状态下，每按一次特定键，仅渲染下一帧并保持暂停。
2.  **音量调节**：通过上下方向键实时调整 SDL3 AudioStream 的增益（Gain）。

## 面试延伸题
- **问**：在执行 Seek 时，为什么要优先跳转到 I 帧（关键帧）？
- **答**：因为视频压缩是有损且存在帧间参考的。如果跳转到 P/B 帧，由于缺少前面的参考帧，解码器无法还原出完整的图像，导致花屏。`avformat_seek_file` 默认通常会寻找目标时间点附近的关键帧。

## 延伸阅读
- [FFmpeg API: avcodec_flush_buffers](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga5830a842f619942b960b1715a300d832)
- [How to debug multi-threaded programs](https://sourceware.org/gdb/onlinedocs/gdb/Threads.html)
