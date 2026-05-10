# 第20节：音频播放与 AudioStream 机制 示例工程

本工程演示了如何使用最新的 SDL3 `SDL_AudioStream` API 来播放原始 PCM 数据。

## 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 准备测试素材
使用 FFmpeg 生成一段 5 秒钟的 440Hz 正弦波 PCM 音频：
```bash
ffmpeg -f lavfi -i "sine=frequency=440:duration=5" -f s16le -ar 44100 -ac 2 test.pcm
```

## 运行
确保 `test.pcm` 位于当前运行目录下。
```bash
./lesson20_audio_stream
```

## 核心 API 总结
- `SDL_OpenAudioDeviceStream`: 一步到位开启默认播放设备并获取流句柄。
- `SDL_ResumeAudioStreamDevice`: 启动流设备，否则处于静音/暂停状态。
- `SDL_PutAudioStreamData`: 向流推送 PCM 数据。
- `SDL_GetAudioStreamAvailable`: 获取当前待播放的缓冲数据量，用于节奏控制。
