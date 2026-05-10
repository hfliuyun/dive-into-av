# 第17节：转封装、流拷贝与精确剪辑 示例工程

本工程演示了如何使用 FFmpeg API 进行两种模式的视频剪辑。

## 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 准备测试素材
生成一个 GOP=125 (约 5 秒一个关键帧) 的测试视频：
```bash
ffmpeg -f lavfi -i testsrc=duration=20:size=640x360:rate=25 -c:v libx264 -g 125 -y input.mp4
```

## 运行实验

### 1. Fast 模式 (流拷贝)
从 3500ms (3.5秒) 处开始剪辑。由于 3.5s 之前最近的关键帧在 0s，输出视频将从 0s 开始。
```bash
./lesson17_remux_seek input.mp4 fast_out.mp4 3500 fast
```
使用播放器查看 `fast_out.mp4`，你会发现它并没有从 3.5s 开始，而是从更早的地方开始。

### 2. Precise 模式 (精准定位)
从 3500ms 处提取精准的一帧。
```bash
./lesson17_remux_seek input.mp4 precise_frame.yuv 3500 precise
```
程序会输出解码过程，并保存 3500ms 处的那一帧为 YUV420P 格式。
你可以使用 ffplay 验证保存的 YUV：
```bash
ffplay -f rawvideo -pixel_format yuv420p -video_size 640x360 precise_frame.yuv
```

## 核心 API 总结
- `avformat_seek_file`: 支持设定范围的 Seek。
- `AVSEEK_FLAG_BACKWARD`: 确保定位到目标点之前的关键帧，这是解码的基础。
- `av_interleaved_write_frame`: 用于流拷贝模式下直接写入 Packet。
