# 第26节：RTMP 推流与拉流 示例工程

本工程演示了如何使用 FFmpeg API 将本地视频文件推送到 RTMP 流媒体服务器，并模拟实时流的推送节奏。

## 环境准备

### 1. 启动 SRS 服务器
推荐使用 Docker：
```bash
docker run --rm -it -p 1935:1935 -p 1985:1985 -p 8080:8080 ossrs/srs:5
```
启动后，SRS 将监听 1935 端口接收 RTMP 推流。

> **提示**：如果你在 macOS/Windows 上使用 Docker Desktop，推流地址可使用 `rtmp://localhost/live/test`。如果你在远程服务器或虚拟机上运行，请将 `localhost` 替换为真实的 IP 地址。

## 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 运行实验

### 1. 启动推流
```bash
./lesson26_rtmp ../../../assets/video/sync_test.mp4 rtmp://localhost/live/test
```
程序会开始读取 `sync_test.mp4` 并以正常播放速度向服务器推送。

### 2. 拉流测试
打开另一个终端，使用 `ffplay` 播放推出来的流：
```bash
ffplay rtmp://localhost/live/test
```
如果一切正常，你应该能看到正在“直播”的视频画面。

## 核心 API
- `avformat_alloc_output_context2`: 初始化输出，格式需设为 `flv`。
- `av_gettime`: 获取当前系统微秒级时间，用于节奏控制。
- `av_usleep`: 进程休眠，实现精准的推流频率控制。
- `av_interleaved_write_frame`: 将 Packet 写入输出上下文（在本例中为网络 Socket）。
