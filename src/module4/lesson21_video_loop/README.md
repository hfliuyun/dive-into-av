# 第21节：视频播放循环 示例工程

本工程演示了如何结合 FFmpeg 解码与 SDL3 渲染，并使用 SDL 定时器机制控制视频的刷新频率，实现一个无声的视频播放器。

## 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 准备测试素材
生成一个 10 秒、25fps、无声的测试视频：
```bash
ffmpeg -f lavfi -i testsrc=duration=10:size=640x360:rate=25 -c:v libx264 -pix_fmt yuv420p -an test.mp4
```

## 运行
```bash
./lesson21_video_loop test.mp4
```

## 核心设计模式
- **生产者-消费者思路**：虽然目前是单线程模拟，但通过 `REFRESH_EVENT` 将“渲染时间点”和“渲染执行”了解耦。
- **SDL3 定时器**：使用 `SDL_AddTimer` 实现高精度的周期性回调。
- **自定义事件**：通过 `SDL_RegisterEvents` 和 `SDL_PushEvent` 实现跨线程通信（定时器线程到主线程）。
