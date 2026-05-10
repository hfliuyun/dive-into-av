# 第20节：音频播放与 AudioStream 机制

## 本节能力目标
- **理解 SDL3 音频架构重构**：掌握 SDL3 弃用回调函数（Callback）转向 `SDL_AudioStream`（Push/Pull）的原因。
- **掌握音频流配置**：学习如何定义 `SDL_AudioSpec` 并绑定物理设备。
- **实现 PCM 原始数据播放**：能够通过 C++ 将外部 PCM 文件平滑地推送到声卡输出。
- **理解音频缓冲控制**：掌握如何通过监控可用缓冲区大小来控制数据推送节奏，防止爆音或溢出。

## 真实工程对应场景
- **播放器音频输出层**：FFplay 或主流播放器在解码出 PCM 帧后，需要将其送入声卡，这个过程在 SDL3 中通过 AudioStream 完成。
- **实时语音/音效系统**：游戏中的背景音乐或实时语音通话，通常也采用这种 Push 模式进行动态音频合成。

## 引言
在多媒体开发的历史长河中，“音频回调”曾是跨平台音频库的标准模式：由系统时钟驱动，定期“呼叫”你的函数来索要数据。然而，回调函数对多线程安全、实时性以及开发者心智负担有着极高的要求。

**SDL3 彻底改变了这一切**。它引入了基于 FIFO（先进先出）逻辑的 **`SDL_AudioStream`**。现在，你可以像写文件一样简单地向声卡“推送”数据，或者像读文件一样从中“拉取”数据。这种改变极大地简化了音频同步和格式转换的逻辑。

## 理论讲解

### 1. 从回调到 AudioStream 的进化
- **SDL2 (旧模式)**：设置回调 -> 开启设备 -> 等待系统索要 -> 在回调函数中拼命填满 Buffer。
- **SDL3 (新模式)**：创建流 -> 开启设备 -> 在你方便的时候调用 `SDL_PutAudioStreamData`。

虽然 SDL3 依然支持底层回调（对于需要极低延迟的专业场景），但对于大多数应用，**AudioStream 是官方首选和推荐的模式**。你掌控主动权，不再需要在中断上下文中操作，逻辑更加清晰。

### 2. 音频规格 (AudioSpec)
在 SDL3 中，音频规格非常直观：
- `format`: 采样格式（如 `SDL_AUDIO_S16LE` 表示 16 位有符号小端）。
- `channels`: 声道数（1 为单声道，2 为立体声）。
- `freq`: 采样率（如 44100Hz）。

### 3. 数据推送与缓冲控制
由于声卡播放的速度是恒定的，而 CPU 处理速度极快。如果你无节制地推送数据，会导致内存占用飙升。
- 使用 `SDL_GetAudioStreamAvailable(stream)`：查询当前已经在缓冲区中排队的数据大小。
- **节奏控制**：当缓冲区已满（例如缓冲了 500ms 的数据）时，让推送线程休眠一小会儿。
- **防止提前退出**：当文件读取完毕时，不能立即关闭程序。必须循环检查 `SDL_GetAudioStreamAvailable(stream) > 0`，直到所有缓冲数据都被送往声卡。

## 关键点总结
- **SDL3 推荐模式**：`SDL_OpenAudioDeviceStream` + `SDL_PutAudioStreamData`。
- **状态管理**：新创建的流默认是暂停的，必须调用 `SDL_ResumeAudioStreamDevice`。
- **播放收尾**：务必等待缓冲区排空后再退出程序。

## 环境与素材准备

### 环境要求
- 已安装 SDL3 库。

### 测试素材
使用 FFmpeg 生成一段标准的 PCM (S16LE, 44100Hz, 2ch) 音频：
```bash
ffmpeg -i input.mp4 -f s16le -acodec pcm_s16le -ar 44100 -ac 2 test.pcm
```

## 核心代码实验

### 实验1：实现 PCM 播放器
**目标：** 编写一个 C++ 程序，读取 `test.pcm` 并通过 SDL3 播放。

**核心逻辑解析：**
- **初始化**：`SDL_Init(SDL_INIT_AUDIO)`。
- **开启设备流**：
  ```cpp
  SDL_AudioSpec spec = { SDL_AUDIO_S16LE, 2, 44100 };
  SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, nullptr);
  ```
- **播放循环**：
  ```cpp
  while (read_data(buffer)) {
      // 检查缓冲，避免堆积（保持 500ms 左右的缓冲）
      while (SDL_GetAudioStreamAvailable(stream) > max_buffer_size) {
          SDL_Delay(10);
      }
      SDL_PutAudioStreamData(stream, buffer, size);
  }
  // 收尾：等待所有缓冲数据播完
  while (SDL_GetAudioStreamAvailable(stream) > 0) {
      SDL_Delay(10);
  }
  ```

## 预期结果与验收标准

### 预期输出
- 系统扬声器传出清晰的音频。
- 控制台实时打印出数据推送的进度。

### 验收标准
- 声音无卡顿、无爆音。
- 程序能正确处理文件结束并自动关闭设备。
- 无内存泄漏。

## 常见错误与排查
- **只有噪音**：检查读取 PCM 的格式是否与 `SDL_AudioSpec` 匹配（大端/小端，位深度）。
- **没有声音**：检查是否调用了 `SDL_ResumeAudioStreamDevice`。
- **声音断断续续**：检查推送逻辑中的 `SDL_Delay` 是否过长。

## 课后小挑战
1. **支持 WAV 格式**：修改代码，解析 WAV 文件头以获取音频规格。
2. **动态音量控制**：研究 `SDL_SetAudioStreamGain` (如果 SDL3 已实现) 或手动修改 PCM 数据调整振幅。

## 面试延伸题
- **问**：SDL3 的 AudioStream 相比传统的双缓冲切换有什么优势？
- **答**：AudioStream 提供了一个统一的 FIFO 抽象，且内置了高质量的重采样器。它能自动处理输入输出频率不一致的情况，极大降低了跨平台音频开发的门槛。

## 延伸阅读
- [SDL3 AudioStream API Reference](https://wiki.libsdl.org/SDL3/SDL_AudioStream)
