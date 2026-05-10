# 阶段项目二：mini_player

## 本节能力目标
- **工程级架构能力**：学会通过多线程分离（解封装、视频解码、音频解码、渲染）来构建健壮的播放器引擎。
- **状态管理思维**：掌握如何通过一个统一的 `PlayerContext` 维护播放器的全局状态，包括时钟、队列、标志位及资源句柄。
- **复杂逻辑整合**：将之前学到的音画同步算法、Seek 精准跳转以及资源安全回收逻辑整合进一个完整的项目中。
- **面向对象/模块化编程**：初步体验如何通过模块化拆分（多文件协作）来降低音视频工程的复杂度。

## 项目背景
恭喜你完成了模块四“音视频播放与同步”的学习。在前面的 6 节课中，我们分别攻克了 SDL3 渲染、音频流播放、多线程架构、同步算法以及交互控制。

但是，真实工程中的播放器并不是简单的代码堆砌。为了保持代码的可维护性，我们需要一个更专业的组织方式。本节课我们将实现 `mini_player`——这是一个功能闭环的本地视频播放器，它将作为你简历中第一个沉甸甸的音视频项目。

## 理论讲解：播放器架构设计

### 1. 核心上下文：PlayerContext
在之前的实验代码中，我们使用了大量的全局变量（如 `g_quit`, `g_pkt_queue` 等）。在正式项目中，这会导致代码难以扩展且容易产生命名冲突。
我们将引入 `PlayerContext` 结构体，它像一个“中央控制器”，持有：
- **资源句柄**：`AVFormatContext`, `AVCodecContext`, `SDL_Window` 等。
- **同步时钟**：`audio_clock`。
- **缓冲区**：音视频 Packet 队列和 Frame 队列。
- **控制标志**：`quit`, `paused`, `seek_request` 等。

### 2. 线程分工（三权分立）
本项目将线程逻辑拆分到不同的模块中：
- **Demux 模块**：运行 `read_thread`，负责读取原始包并分发。
- **Video 模块**：运行 `video_thread`，负责视频解码并控制同步。
- **Audio 模块**：运行 `audio_thread`，负责音频解码、重采样及输出。
- **Main 模块**：负责 SDL 交互与视频画面最终呈现。

### 3. Seek 与 Flush 机制回顾
跳转（Seek）是考验播放器稳定性的试金石。在 `mini_player` 中，我们将严谨地执行以下逻辑：
1. 主线程接收按键请求，更新 `PlayerContext` 中的 `seek_pos`。
2. 读线程检测到请求，执行 `avformat_seek_file`。
3. 关键点：读线程通知所有队列进行 `abort` 并 `flush`。
4. 解码线程在收到新的包序列时，自动执行 `avcodec_flush_buffers`。

## 关键点总结
- **解耦**：模块之间通过 `SafeQueue` 通信。
- **同步**：视频始终盯着音频时钟。
- **生命周期**：所有的 `alloc` 必须对应 `free`，所有的线程必须被 `join`。

## 核心代码实验：mini_player

本项目同样提供了两套代码：
1. **reference (参考实现)**：包含了完整的、模块化的播放器源码。
2. **student (学员练习)**：提供了多线程框架和上下文定义，但关键的同步逻辑和解码循环被挖空，需要你根据 `TODO` 指示补全。

### 实验步骤
1. 进入 `src/projects/mini_player/student` 目录。
2. 先阅读 `player_context.h` 了解数据结构。
3. 按照 `demux_thread.cpp` -> `audio_thread.cpp` -> `video_thread.cpp` -> `main.cpp` 的顺序补全逻辑。
4. 编译并运行，测试播放、暂停、进度跳转。

## 预期结果与验收标准

### 预期输出
- 能够正常打开各种主流格式（MP4, MKV, MOV）的本地视频。
- 声画同步表现良好，Seek 后画面切换迅速不卡顿。

### 验收标准
- 成功整合了音视频双路解码。
- 采用多线程模型，UI 交互灵敏（暂停、Seek 响应即时）。
- 使用 ASAN 验证，播放过程中及退出时无内存泄漏。

## 常见错误与排查
- **音频有杂音**：检查重采样输出的格式是否与 SDL3 AudioSpec 严格匹配。
- **Seek 后画面不动**：检查是否忘记在解码线程中调用 `avcodec_flush_buffers`。
- **退出时崩溃**：检查 `PlayerContext` 中 SDL 资源的销毁顺序，确保先停线程再销毁渲染器。

## 课后小挑战
1. **实现静音切换**：添加按键（如 M 键）来快速切换静音状态（提示：调用 `SDL_PauseAudioStreamDevice` 或清空推送数据）。
2. **UI 实时进度显示**：利用 SDL3 渲染一些简单的矩形作为进度条。

## 面试延伸题
- **问**：在 `mini_player` 的架构中，如果有网络抖动导致 `read_thread` 读不到包，渲染线程会发生什么？
- **答**：渲染线程会尝试从 `FrameQueue` 弹出数据。如果 `FrameQueue` 也空了，渲染线程会因为阻塞在 `pop` 操作上而“定格”在最后一帧，直到新数据到达。这本质上就是播放器的“卡顿”保护机制。

## 延伸阅读
- [Design of ffplay](https://ffmpeg.org/ffplay.html)
- [Multi-threaded Architecture in Media Players](https://www.apriorit.com/dev-blog/544-how-to-create-video-player-ffmpeg-sdl)
