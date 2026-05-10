# 阶段项目一：mini_ffmpeg

本项目是模块三（音视频解封装、封装与处理管道）的结课项目。你将整合之前学到的所有知识，实现一个功能完备的命令行媒体处理工具。

## 目录结构
- **`student/` (学员练习)**: 这是你工作的地方。我们已经为你搭好了工程骨架，并处理了繁琐的命令行参数解析和基础框架。你只需要根据 `// TODO:` 注释，补全核心的 FFmpeg API 调用逻辑即可。
- **`reference/` (参考实现)**: 这是一个完整的、可编译运行的参考实现。如果你在编写过程中卡住了，或者想查看标准做法，可以参考这里。

## 子命令功能
1. **probe**: 探测媒体流信息（宽、高、码率、采样率等）。
2. **remux**: 高速转封装（例如从 MKV 转换为 MP4，不涉及解码编码）。
3. **transcode**: 视频转码，支持使用 `--scale` 参数进行缩放。
4. **resample**: 音频重采样，支持使用 `--ar` (采样率) 和 `--ac` (声道数) 参数。

## 编译与测试

### 1. 编译
进入对应目录（建议先从 `student` 开始）：
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### 2. 测试建议
准备一个名为 `input.mp4` 的素材，依次运行以下命令：
```bash
# 1. 探测信息
./mini_ffmpeg probe input.mp4

# 2. 转封装为 MKV
./mini_ffmpeg remux input.mp4 output.mkv

# 3. 视频缩放并转码 (H.264)
./mini_ffmpeg transcode input.mp4 output_720p.mp4 --scale 1280x720

# 4. 音频重采样
./mini_ffmpeg resample input.mp4 output_audio.aac --ar 44100 --ac 1
```

## 学习建议
- **不要急于看参考代码**：尝试根据 `TODO` 中的提示和前几节课的讲义独立思考。
- **关注内存管理**：使用 `goto end;` 模式来确保在任何错误发生时都能正确释放资源。
- **善用工具**：编译时已开启 ASAN，运行程序后如果报错，请仔细阅读堆栈信息定位泄漏或非法访问。
