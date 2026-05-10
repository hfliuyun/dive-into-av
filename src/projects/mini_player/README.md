# 阶段项目二：mini_player

本项目是模块四（音视频播放与同步）的结课项目。你将整合多线程架构、音画同步算法、Seek 跳转等核心技术，打造一个功能完备的本地视频播放器。

## 目录结构
- **`student/` (学员练习)**：核心逻辑已被挖空，请根据 `// TODO:` 指示补全代码。
- **`reference/` (参考实现)**：完整的模块化参考代码，供学习和排障。

## 功能特性
1.  **多线程架构**：Demux, Video Decode, Audio Decode 线程全部分离。
2.  **音画同步**：以音频时钟为基准的动态视频追赶算法。
3.  **交互控制**：
    -   `Space`: 暂停/恢复。
    -   `Left/Right`: 快退/快进 5 秒。
    -   `Q / Esc`: 退出播放器。
4.  **健壮性**：支持 Seek 时的线程安全 Flush 和 SDL 音频缓冲区清理。

## 编译与测试

### 1. 编译
进入对应目录（建议先从 `student` 开始）：
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### 2. 测试
```bash
./mini_player ../../../assets/video/sync_test.mp4
```

## 核心挑战说明
-   **Audio Clock 补偿**：由于音频推送到声卡有缓冲区延迟，计算时钟时必须使用 `pts - buffered_duration`。
-   **线程安全 Flush**：不要在其他线程强行重置解码器，应通过 `serial` 序列号由解码线程自主完成。
