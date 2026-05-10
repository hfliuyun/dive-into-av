# 第22节：线程模型与队列设计 示例工程

本工程展示了播放器的核心多线程架构：
1. **Read Thread**: 负责解封装（Demuxing）并将 Packet 存入队列。
2. **Decode Thread**: 负责解码并将 Frame 存入队列。
3. **Main Thread**: 负责 SDL 事件循环与定时渲染。

## 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 运行
```bash
./lesson22_thread_queue ../../../assets/video/test.mp4
```

## 核心设计
- **`SafeQueue.h`**: 这是一个高性能的线程安全阻塞队列模板类。
- **解耦设计**: 通过队列缓冲区，即使读取磁盘出现瞬间延迟，由于 `FrameQueue` 中有预存的帧，播放依然能保持流畅。
- **生命周期管理**: 演示了在多线程环境下，如何通过 `atomic<bool>` 和 `queue.abort()` 实现干净的资源清理和线程退出。
