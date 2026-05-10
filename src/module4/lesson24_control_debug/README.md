# 第24节：播放控制与播放器排障 示例工程

本工程实现了播放器的交互控制功能，并提供了排障参考代码。

## 交互说明
- **空格键 (Space)**：切换 暂停/恢复 状态。
- **左方向键 (Left)**：快退 5 秒。
- **右方向键 (Right)**：快进 5 秒。
- **Q 键 / Esc 键**：安全退出。

## 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 运行
```bash
./lesson24_control_debug ../../../assets/video/sync_test.mp4
```

## 核心排障清单
如果你在开发中遇到问题，请对照以下 checklist：
1. **画面花屏**：是否在 Seek 后调用了 `avcodec_flush_buffers`？
2. **Seek 后卡顿**：是否清空了 `PacketQueue` 和 `FrameQueue` 里的旧数据？
3. **内存上涨**：被 `flush` 掉的 Packet 和 Frame 是否执行了相应的 `free` 操作？
4. **声音消失**：Pause 状态下是否停止了向 `SDL_AudioStream` 推送数据？
5. **程序无法退出**：是否先调用了 `queue.abort()` 再执行 `thread.join()`？
