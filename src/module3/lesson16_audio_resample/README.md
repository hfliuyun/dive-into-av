# 第16节：音频重采样与格式转换

## 简介

本项目演示使用 FFmpeg libswresample 对音频进行重采样、声道转换和格式转换。

## 环境配置

### macOS

```bash
brew install ffmpeg cmake
```

### Fedora

```bash
sudo dnf install ffmpeg-devel cmake gcc-c++
```

### Ubuntu/Debian

```bash
sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswresample-dev cmake g++
```

## 编译

```bash
mkdir build && cd build
cmake ..
make

# Debug mode with ASAN:
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 准备测试素材

```bash
ffmpeg -f lavfi -i "sine=frequency=440:duration=5" -c:a aac -b:a 128k -y test_audio.mp4
```

## 运行

```bash
# 48kHz stereo -> 44.1kHz mono (S16)
./audio_resample test_audio.mp4 output.mp4 -ar 44100 -ac 1 -af s16

# 48kHz stereo -> 48kHz mono (FLT)
./audio_resample test_audio.mp4 output.mp4 -ar 48000 -ac 1 -af flt

# 48kHz stereo -> 44.1kHz stereo (S16P)
./audio_resample test_audio.mp4 output.mp4 -ar 44100 -ac 2 -af s16p
```

## 参数说明

- `-ar <rate>`: 输出采样率 (default: 44100)
- `-ac <ch>`: 输出声道数 (default: 1)
- `-af <fmt>`: 输出采样格式 (default: s16)

支持的采样格式:
- s16  - Signed 16-bit
- s32  - Signed 32-bit
- flt  - Float
- dbl  - Double
- s16p - Signed 16-bit planar
- fltp - Float planar

## 验证输出

```bash
ffplay output.mp4
```

## 预期输出

```
Audio Resampling Pipeline:
  Input:  48000Hz, 2ch, s16
  Output: 44100Hz, 1ch, s16
  Encoder: aac

Resampling done:
  Output frames: 216
  Output file: output.mp4
  Verify: ffplay output.mp4
```

## 关键概念

### SwrContext

重采样上下文，包含输入/输出格式、采样率、声道布局等参数。

### 重采样流程

1. swr_alloc() - 分配重采样上下文
2. 设置输入/输出参数 (in_channel_layout, in_sample_rate, in_sample_fmt 等)
3. swr_init() - 初始化重采样器
4. swr_convert() - 执行重采样
5. swr_close() - 关闭重采样器

### 声道布局

- AV_CH_LAYOUT_MONO - 单声道
- AV_CH_LAYOUT_STEREO - 立体声

## 常见问题

### 声道布局设置失败

检查 ch_layout 是否正确初始化，使用 av_channel_layout_from_mask()。

### 内存泄漏

确保调用 swr_free() 释放重采样上下文。
