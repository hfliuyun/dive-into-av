# 第22节：线程模型与队列设计

## 本节能力目标
- **掌握播放器经典线程模型**：理解读包（Demux）、解码（Decode）、渲染（Render）三者分离的必要性。
- **掌握生产者-消费者模式**：学会在音视频工程中应用多线程同步机制。
- **手写线程安全队列**：能够使用 C++ `mutex` 和 `condition_variable` 实现高效的阻塞队列。
- **掌握多线程退出机制**：学会如何在多个并发线程中优雅地处理停止信号和资源回收。

## 真实工程对应场景
- **专业播放器架构**：FFplay、VLC、IJKPlayer 等所有工业级播放器都采用了类似的多线程+队列结构。
- **高吞吐数据处理**：在处理 4K/8K 视频或高并发流媒体时，单线程完全无法满足性能需求。

## 引言
在上一节课中，我们实现了一个简单的视频播放循环。虽然它能工作，但在实际工程中存在巨大隐患：**解码和读取包都在主线程中执行**。

想象一下：如果磁盘读取突然变慢（I/O 抖动），或者解码某一帧特别费劲（如高码率 B 帧），主线程就会卡住，导致画面瞬间停顿。更糟糕的是，此时你无法拖动窗口，甚至无法点击“退出”，因为事件循环也被阻塞了。

为了解决这个问题，本节课我们将引入**生产者-消费者（Producer-Consumer）模型**，将播放器升级为“多线程+缓冲队列”架构。

## 理论讲解

### 1. 播放器的三权分立
一个健壮的播放器通常至少有三个核心线程：
1.  **解封装线程 (read_thread)**：负责从文件或网络读取 `AVPacket`，并放入 `PacketQueue`。它是“生产者”。
2.  **解码线程 (decode_thread)**：从 `PacketQueue` 拿包，解码成 `AVFrame`，放入 `FrameQueue`。它既是“消费者”又是“生产者”。
3.  **渲染线程 (main_thread)**：根据定时器信号，从 `FrameQueue` 拿帧并显示。它是最终的“消费者”。

### 2. 线程安全队列 (SafeQueue)
为了在线程之间传递数据，我们需要一个具备以下特性的队列：
-   **互斥（Mutual Exclusion）**：同一时间只有一个线程能修改队列。
-   **阻塞（Blocking）**：如果队列满了，生产者应该等待；如果队列空了，消费者应该等待。
-   **唤醒（Signaling）**：一旦有新空间或新数据，应立即通知等待中的线程。

### 3. 缓冲控制 (Throttling)
如果解封装线程跑得太快，而解码太慢，`PacketQueue` 就会无限膨胀，撑爆内存。因此，我们需要为队列设置**最大容量限制**。当达到阈值时，生产者线程应被阻塞，直到消费者消耗了部分数据。

## 关键点总结
-   **解耦**：通过多线程，让 I/O、计算、显示互不干扰。
-   **缓冲**：队列充当了“蓄水池”，能够平滑处理瞬间的性能抖动。
-   **同步**：正确使用锁和条件变量是多线程开发的灵魂。

## 环境与素材准备

### 环境要求
-   支持 C++17 的编译器（使用标准库线程 `<thread>`）。
-   FFmpeg 与 SDL3 开发库。

### 测试素材
本节课可沿用上一节生成的 `test.mp4`。

## 核心代码实验

### 实验1：实现多线程视频播放器
**目标：** 构建一个基于 `SafeQueue` 的多线程播放器，实现读、解、显分离。

**核心逻辑解析：**
-   **SafeQueue 模板类**：使用条件变量实现真正的阻塞。
    ```cpp
    bool push(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        // 如果队列满了，就阻塞等待 cond_full_ 信号
        cond_full_.wait(lock, [this] { return abort_ || queue_.size() < max_size_; });
        if (abort_) return false;
        queue_.push(std::move(value));
        cond_empty_.notify_one(); // 通知等待取数据的线程
        return true;
    }
    ```
-   **读线程逻辑**：
    ```cpp
    void read_thread() {
        while (!quit) {
            AVPacket* pkt = av_packet_alloc();
            av_read_frame(fmt_ctx, pkt);
            // push 是阻塞的，不再需要手动 SDL_Delay 轮询
            if (!pkt_queue.push(pkt)) {
                av_packet_free(&pkt);
            }
        }
    }
    ```
-   **主线程渲染**：
    收到 `REFRESH_EVENT` 时，不再现场解码，而是直接从 `frame_queue.pop()` 获取已解码好的帧。由于渲染速度受定时器限制，这里通常能实现平滑的“生产-消费”平衡。

## 预期结果与验收标准

### 预期输出
-   视频播放极其流畅，即使人为在 `read_thread` 中加入随机小延时，画面也不会卡顿。
-   窗口响应灵敏，关闭操作瞬间响应。

### 验收标准
-   成功分离出读线程和解码线程。
-   多线程环境下运行稳定，无死锁。
-   通过 ASAN 检查，确保 AVPacket 和 AVFrame 在跨线程传递时引用计数管理正确。

## 常见错误与排查
-   **程序退出时挂起**：通常是因为某个线程阻塞在 `queue.pop()` 处，而没有收到退出信号。**解决方法**：在退出时通过条件变量广播信号（notify_all），并让队列返回一个“无效值”或错误码。
-   **内存飞速上涨**：检查是否忘记设置队列的最大上限。
-   **花屏**：检查是否在多个线程中共享了同一个 `AVFrame` 实例而没有进行引用计数处理（`av_frame_unref`）。

## 课后小挑战
1.  **实现阻塞推送**：优化 `SafeQueue`，让 `push` 操作在队列满时也进入阻塞状态（使用第二个条件变量），而不是简单的 `SDL_Delay`。
2.  **多流并行**：尝试为音频也开启独立的解码线程和队列。

## 面试延伸题
-   **问**：在播放器中，`PacketQueue` 和 `FrameQueue` 的大小通常设置成多少比较合适？
-   **答**：通常 `PacketQueue` 可以设置大一些（如几百个包，约几秒的数据），因为它占内存少；而 `FrameQueue` 必须严格限制（如 3-5 帧），因为原始像素数据极其庞大，过多的 Frame 缓存会迅速耗尽系统内存。

## 延伸阅读
-   [C++ Concurrency in Action](https://www.manning.com/books/c-concurrency-in-action-second-edition)
-   [FFplay.c Source Analysis](https://github.com/FFmpeg/FFmpeg/blob/master/fftools/ffplay.c)
