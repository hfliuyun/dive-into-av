# 第27节：HLS 切片与基础直播链路 示例工程

本工程演示了如何使用 FFmpeg API 将本地视频文件切片为 HLS (M3U8 + TS)，并提供了一个简单的 H5 播放页面进行验证。

## 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 运行实验

### 1. 启动切片器
首先在项目目录下创建 `output` 文件夹：
```bash
mkdir -p output
```

然后运行切片程序：
```bash
./lesson27_hls ../../../assets/video/sync_test.mp4 output/playlist.m3u8
```
成功后，你会在 `output/` 目录下看到 `playlist.m3u8` 和一系列以 `seg_xxx.ts` 命名的视频片段。

### 2. 浏览器播放验证
由于 HLS 涉及到多个小文件的按序加载，通常需要通过 HTTP 协议分发。你可以使用 Python 快速启动一个静态 Web 服务器：
```bash
# 在 src/module5/lesson27_hls 目录下运行
python3 -m http.server 8000
```
然后在浏览器中打开：[http://localhost:8000/player.html](http://localhost:8000/player.html)

你应该能看到视频正在正常播放，并且可以自由拖动进度条。

## 核心设计
- **`hls` Muxer**: FFmpeg 内置的切片器封装，自动处理 M3U8 文件的更新和 TS 文件的切分。
- **GOP 约束**: HLS 切片必须在关键帧（IDR 帧）处开启新的分片。如果视频的 GOP 较大，切片的实际时长将受限于关键帧间隔。
- **hls.js**: 一个优秀的 JavaScript 库，让不支持 HLS 原生播放的浏览器（如 Chrome, Firefox）也能通过 Media Source Extensions (MSE) 播放 HLS。
