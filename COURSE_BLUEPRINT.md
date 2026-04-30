# 《动手学音视频开发》课程建设蓝图

本文档定义《动手学音视频开发》的课程定位、能力地图、主线大纲、阶段项目、工程规范与验收标准。

> **职责边界**：本文档是课程结构与规范的最高优先级蓝图；实际开发进度以 `PROGRESS.md` 为准，`README.md` 只展示面向学习者的入口与简化状态。

---

## 1. 课程定位与岗位目标

### 1.1 课程定位

本课程模仿《动手学深度学习》的组织方式，采用“概念讲解 + 可运行实验 + 工程项目”三层结构，面向希望系统入门音视频开发的学习者。

课程默认方向为 `通用媒体工程`，主线覆盖：

- 本地媒体文件分析与排障
- FFmpeg CLI 与 FFmpeg API 基础使用
- 解封装、解码、编码、转封装、转码
- 像素格式转换、音频重采样、滤镜处理
- 本地播放器、音画同步、基础流媒体链路

### 1.2 目标岗位

课程主要对齐以下入门、校招和初级岗位能力：

- 音视频开发工程师（媒体处理方向）
- 播放器开发工程师（基础播放器与同步方向）
- 流媒体开发工程师（推拉流与基础链路方向）
- 多媒体客户端工程师（具备基础音视频模块开发能力）

### 1.3 结课后应具备的能力

学完后，学员应能独立完成以下工作：

- 使用 `ffprobe`、`ffmpeg`、`ffplay` 分析媒体文件与码流特征
- 理解并处理像素格式、采样格式、采样率、时间戳、码率、帧率等基础概念
- 编写简化版解复用、解码、编码、转码、重采样程序
- 实现一个支持音画同步和基础控制功能的本地播放器
- 搭建基础推流、拉流、切片实验，理解直播链路关键环节
- 排查常见问题：时间戳错误、音画不同步、资源泄漏、格式不兼容、队列堆积、颜色异常

### 1.4 非目标

以下内容可作为扩展专题，但不作为主线毕业要求：

- 高性能 RTC 内核开发
- 浏览器音视频内核开发
- 大规模分布式流媒体服务端架构
- 深度优化级硬件编解码适配
- 复杂商业播放器完整产品化能力

---

## 2. 学员画像与先修要求

### 2.1 目标学员

- 具备基础编程能力，希望转向音视频开发的学习者
- 有 C/C++ 或 Python 基础，但缺乏系统音视频知识的工程师
- 希望通过课程完成项目作品、提升面试和上岗能力的学习者

### 2.2 先修要求

建议学习者具备以下基础：

- 能阅读基础 Python 代码
- 能阅读基础 C/C++ 代码
- 了解编译、链接、命令行、文件 I/O 等基本概念
- 熟悉 Linux、macOS、Windows/WSL 中至少一种开发环境

### 2.3 默认学习路径

课程按“先建立直觉，再进入 API，再做项目”的顺序推进：

1. 用 Python 建立音视频数据直觉
2. 用 FFmpeg CLI 建立媒体分析与排障能力
3. 用 C++ + FFmpeg API 完成处理管道
4. 用 SDL2 完成播放器主项目
5. 用 RTMP/HLS 理解流媒体基本链路

---

## 3. 能力地图

### 3.1 基础认知能力

- 读懂图像、音频、视频的底层表示
- 区分容器、码流、编解码器、像素格式、采样格式
- 理解 PTS、DTS、time_base、fps、bitrate 的关系
- 理解 packed/planar、stride/linesize、interleaved/planar audio 等底层布局

### 3.2 工具使用能力

- 使用 `ffmpeg`、`ffprobe`、`ffplay` 进行格式转换、探测与验证
- 使用 `gdb/lldb`、ASAN 进行定位与排障
- 通过日志、返回值和 `av_strerror` 快速定位 FFmpeg API 调用问题
- 使用 `ffprobe -show_streams -show_packets -show_frames` 分析媒体结构

### 3.3 工程实现能力

- 编写解复用、解码、编码、封装、滤镜、重采样程序
- 设计基础线程模型、队列模型和状态机
- 实现本地播放器的音频时钟与音画同步
- 为命令行工具、播放器提供可观测日志和最小帮助信息

### 3.4 岗位排障能力

- 判断某个任务能否使用 `stream copy`
- 识别时间戳错乱、关键帧不足、像素格式不匹配、采样率不兼容
- 分析播放器卡顿、不同步、内存泄漏和 CPU 异常占用
- 判断封装格式与码流格式是否匹配，例如 Annex B、AVCC、extradata 问题

---

## 4. 技术栈选型与环境依赖

### 4.1 基础实验篇（Python + 可视化）

**语言与工具：**

- Python 3.8+
- Jupyter Notebook 或 JupyterLab

**核心库：**

- PyAV
- OpenCV
- NumPy
- Matplotlib
- PyAudio 或 SoundFile
- Pillow

**建议安装方式：**

```bash
conda create -n av-learn python=3.9
conda activate av-learn
conda install -c conda-forge av opencv numpy matplotlib pyaudio
pip install soundfile pillow jupyter
```

### 4.2 工程实战篇（C/C++ + FFmpeg API）

**语言与工具：**

- C++17
- CMake 3.15+
- GDB/LLDB
- AddressSanitizer
- `pkg-config`

**核心库：**

- FFmpeg 开发库：`libavformat`、`libavcodec`、`libavutil`、`libswscale`、`libswresample`、`libavfilter`
- SDL2

**Debug 模式启用 ASAN：**

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()
```

**环境配置：**

```bash
# Ubuntu
sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev \
    libswscale-dev libswresample-dev libavfilter-dev libsdl2-dev \
    cmake build-essential pkg-config

# macOS
brew install ffmpeg sdl2 cmake pkg-config
```

### 4.3 推荐工具链

- `ffprobe`：查看流信息、码率、时间戳、像素格式、关键帧
- `ffmpeg`：转码、切片、抽帧、抽音频、生成测试素材
- `ffplay`：快速验证解码、同步、seek、滤镜效果
- `gdb/lldb`：崩溃定位
- `ASAN`：内存错误与泄漏检测
- `jq`：分析 `ffprobe` JSON 输出，可选

---

## 5. 课程结构设计

### 5.1 主线模块

| 模块 | 主题 | 目标 | 阶段产物 |
|------|------|------|----------|
| 模块一 | 音视频基础概念与数据可视化 | 建立数据直觉和媒体分析基础 | 媒体文件体检报告 |
| 模块二 | 音视频编解码核心原理 | 建立码流、解码、编码与 API 认知 | 最小解码/编码实验 |
| 模块三 | 解封装、封装与处理管道 | 完成文件处理与转码核心链路 | `mini_ffmpeg` |
| 模块四 | 播放与同步 | 完成播放器核心项目 | `mini_player` |
| 模块五 | 流媒体基础与结课项目 | 理解推拉流与切片基本链路 | 本地直播链路或播放器展示 |

### 5.2 扩展模块

扩展模块不作为主线毕业门槛：

- 硬件编解码：NVENC、VAAPI、VideoToolbox
- WebRTC：信令、ICE、STUN/TURN、基础采集与协商
- 高级渲染：OpenGL、自定义渲染链路
- 实时采集：摄像头、麦克风、低延迟编码与推流

### 5.3 课程产出物

课程开发最终沉淀：

- 每节课对应讲义文档
- 每节课对应 Notebook 或 C++ 示例工程
- 统一测试素材、素材生成命令与预期输出
- 阶段 checkpoint 与结课项目说明
- 至少两个简历可写的课程项目：`mini_ffmpeg`、`mini_player`

---

## 6. 课程知识主线

### 6.1 时间戳递进路线

时间戳不是单节课知识点，而是贯穿模块二、三、四的主线能力。

| 阶段 | 学习目标 |
|------|----------|
| 第6节 | 认识 PTS、DTS、time_base、fps、bitrate |
| 第8节 | 结合 B 帧理解解码顺序和显示顺序 |
| 第13节 | 掌握 demux/mux 中的时间基转换 |
| 第17节 | 理解 seek、关键帧、精确剪辑和时间戳重写 |
| 第23节 | 使用音频时钟驱动音画同步 |
| 第24节 | 排查 seek 不准、音画漂移、播放卡顿 |

C++ 阶段必须覆盖：`av_q2d`、`av_rescale_q`、`av_packet_rescale_ts`、stream `time_base`、codec `time_base`、B-frame reorder、flush。

### 6.2 码流与封装递进路线

| 阶段 | 学习目标 |
|------|----------|
| 第4节 | 区分容器、码流、编解码器 |
| 第8节 | 理解 NALU、SPS/PPS/VPS、IDR、Annex B、AVCC |
| 第13节 | 理解 extradata、global header、mux/demux 对称性 |
| 第17节 | 理解 bitstream filter 与 stream copy 边界 |
| 第25-26节 | 理解 RTMP、HLS 对 GOP、关键帧和码流格式的要求 |

### 6.3 色彩与采样格式递进路线

| 阶段 | 学习目标 |
|------|----------|
| 第1-2节 | RGB/BGR/YUV420P、平面结构、可视化 |
| 第10节 | 解码输出帧的 pixel format 和 linesize |
| 第15节 | 使用 `libswscale`、`libavfilter` 完成格式转换与滤镜 |
| 第19节 | SDL2 正确显示 YUV 数据 |

### 6.4 音频递进路线

| 阶段 | 学习目标 |
|------|----------|
| 第3节 | PCM、采样率、位深度、声道、波形、频谱 |
| 第9节 | AAC/MP3 感知编码原理 |
| 第14节 | 解码音频输出 PCM |
| 第16节 | 使用 `libswresample` 完成重采样和声道转换 |
| 第20节 | SDL2 音频播放与 callback |
| 第23节 | 音频时钟与音画同步 |

---

## 7. 完整课程大纲（主线 + 扩展）

### 模块一：音视频基础概念与数据可视化

**目标：** 建立对音视频数据、工具链和媒体分析方法的直观认识。

- **第1节：数字图像基础**
  - [核心概念] 像素、分辨率、位深度、RGB 色彩模型
  - [核心实验] 用 Python 读取图片、打印像素值、修改局部区域后保存

- **第2节：视频的色彩空间**
  - [核心概念] RGB、BGR、YUV420P、采样压缩、人眼视觉特性
  - [核心实验] 剥离视频帧的 YUV 数据，单独修改 U/V 分量观察效果

- **第3节：数字音频基础**
  - [核心概念] 采样率、位深度、声道数、PCM、波形与频谱
  - [核心实验] 读取一段音频，绘制波形图与频谱图

- **第4节：音视频容器格式与编解码**
  - [核心概念] 容器、码流、编解码器、封装、解封装、裸流
  - [核心实验] 用 `ffprobe` 分析 MP4、MKV、AAC、H.264 文件

- **第5节：FFmpeg CLI 与媒体排查基础**
  - [核心概念] `ffmpeg`、`ffprobe`、`ffplay` 的职责划分
  - [核心实验] 抽帧、抽音频、查看关键帧、查看像素格式、验证 seek 行为

- **第6节：帧率、码率与时间戳**
  - [核心概念] FPS、GOP、PTS、DTS、CBR、VBR、time_base
  - [核心实验] 提取帧序列并分析时间戳与显示顺序

- **模块 checkpoint：媒体文件体检报告**
  - 输入一个媒体文件，输出包含容器、编码、分辨率、像素格式、采样率、声道、时长、码率、关键帧、PTS/DTS、stream copy 可行性的分析报告

### 模块二：音视频编解码核心原理

**目标：** 建立从编码原理到 FFmpeg 解码/编码 API 的完整心智模型。

- **第7节：视频编码原理（DCT + 量化）**
  - [核心概念] 变换、量化、熵编码、压缩失真
  - [核心实验] 手写简化版 DCT，观察压缩前后的图像变化

- **第8节：H.264/H.265 关键概念**
  - [核心概念] I/P/B 帧、GOP、NALU、SPS、PPS、VPS、IDR、Annex B、AVCC、extradata
  - [核心实验] 提取关键帧与 NALU，观察不同帧类型对体积和可恢复性的影响

- **第9节：音频编码原理（AAC/MP3）**
  - [核心概念] MDCT、心理声学、码率和感知质量
  - [核心实验] 比较不同 AAC 码率的波形、频谱和听感差异

- **第10节：FFmpeg 解码器 API 入门**
  - [核心概念] `AVCodecContext`、`AVPacket`、`AVFrame`、send/receive 模型、flush
  - [核心实验] 用 C++ 编写最简视频解码器，输出 YUV 帧
  - [前置附录] FFmpeg + CMake + ASAN 最小工程模板

- **第11节：FFmpeg 内存模型与引用计数**
  - [核心概念] `AVBufferRef`、引用计数、`av_frame_unref`、`av_packet_unref`
  - [核心实验] 编写泄漏检测程序，用 ASAN 验证资源释放

- **第12节：FFmpeg 编码器 API 入门**
  - [核心概念] 编码参数、CRF、preset、profile、level、码流输出、flush 流程
  - [核心实验] 将 YUV 序列编码为 H.264 文件

### 模块三：音视频解封装、封装与处理管道

**目标：** 构建音视频文件处理、转码和基础工具链能力。

- **第13节：解封装与封装 API 详解**
  - [核心概念] `AVFormatContext`、流索引、时间基转换、extradata、global header、mux/demux 对称性
  - [核心实验] 将 H.264 裸流和 AAC 裸流打包为 MP4

- **第14节：解码管道构建**
  - [核心概念] 解封装 -> 音视频解码 -> 后处理的数据流转
  - [核心实验] 将 MP4 解码为原始 YUV/PCM 文件

- **第15节：图像滤镜与转码**
  - [核心概念] `libswscale`、`libavfilter`、滤镜图、缩放、叠字、水印
  - [核心实验] 为视频添加文字或图片水印并完成转码

- **第16节：音频重采样与格式转换**
  - [核心概念] `libswresample`、采样率转换、声道映射、采样格式、interleaved/planar
  - [核心实验] 将 48kHz 立体声转换为 44.1kHz 单声道

- **第17节：转封装、流拷贝与精确剪辑**
  - [核心概念] `stream copy`、bitstream filter、seek、关键帧约束、精确剪辑、时间戳重写
  - [核心实验] 对比“秒开剪辑”和“必须重编码”的场景差异

- **第18节：阶段项目一：mini_ffmpeg**
  - [核心概念] 参数解析、处理链路组织、错误处理、日志输出、返回码规范
  - [核心实验] 实现支持探测、转封装、转码、缩放、重采样、剪辑的命令行工具

### 模块四：音视频播放与同步

**目标：** 实现一个具备岗位代表性的基础播放器项目。

- **第19节：SDL2 渲染基础**
  - [核心概念] SDL 窗口、纹理、渲染循环、YUV 显示
  - [核心实验] 用 SDL 显示一张静态 YUV 图像

- **第20节：音频播放与回调机制**
  - [核心概念] SDL 音频设备、callback、缓冲区、欠载与过载、音频延迟
  - [核心实验] 播放 PCM 音频文件

- **第21节：视频播放循环**
  - [核心概念] 刷新节奏、帧率控制、解码与渲染分离
  - [核心实验] 实现无音频的视频播放器

- **第22节：线程模型与队列设计**
  - [核心概念] 读包线程、解码线程、包队列、帧队列、时钟设计、状态机
  - [核心实验] 为播放器引入基础线程和缓冲队列

- **第23节：音视频同步原理**
  - [核心概念] 音频时钟、视频追赶、同步阈值、丢帧与等待策略
  - [核心实验] 实现基于音频时钟的音画同步播放器

- **第24节：播放控制与播放器排障**
  - [核心概念] 暂停、继续、seek、状态切换、卡顿定位、队列观测
  - [核心实验] 为播放器添加控制功能并定位一类同步 bug

### 模块五：流媒体基础与结课项目

**目标：** 建立直播与分发链路基本认知，并完成课程闭环项目。

- **第25节：RTMP 推流与拉流**
  - [核心概念] RTMP 握手、chunk、推流链路、GOP、延迟来源
  - [核心实验] 使用本地 SRS 或 nginx-rtmp，将本地文件推流到 RTMP 服务并拉流验证

- **第26节：HLS 切片与基础直播链路**
  - [核心概念] M3U8、TS/fMP4、切片、播放列表刷新、滑动窗口、ABR 基础
  - [核心实验] 将视频切片为 HLS 流，通过本地 HTTP 服务验证播放

- **扩展专题 A：WebRTC 基础架构**
  - [核心概念] SDP、ICE、STUN/TURN、端到端延迟
  - [核心实验] 搭建一个最小信令流程

- **扩展专题 B：实时采集与直播实验**
  - [核心概念] 摄像头、麦克风采集、实时编码、推流参数
  - [核心实验] 采集本地音视频并推流

- **结课项目：基础媒体播放器或简易直播系统**
  - [核心概念] 模块整合、问题排查、工程组织
  - [核心实验] 在主线项目基础上完成可展示、可录屏、可写入简历的作品

---

## 8. 阶段项目与结课项目

### 8.1 模块一 checkpoint：媒体文件体检报告

目标是让学员完成一份可复现的媒体分析报告。输入一个媒体文件，至少输出：

- 容器格式、时长、总码率
- 视频编码、分辨率、像素格式、帧率、GOP/关键帧信息
- 音频编码、采样率、采样格式、声道数、码率
- PTS/DTS 是否单调、是否存在 B 帧重排
- 是否适合 stream copy、是否适合直接切 HLS 或推 RTMP
- 常见风险提示：seek 不准、首帧黑屏、颜色异常、音频参数不兼容

### 8.2 阶段项目一：mini_ffmpeg

目标是完成一个命令行媒体处理工具，至少支持：

| 命令 | 说明 |
|------|------|
| `probe` | 打印媒体基本信息 |
| `remux` | 转封装 |
| `transcode` | 基础转码 |
| `extract-audio` | 抽取音频 |
| `extract-frame` | 抽帧 |
| `scale` | 视频缩放 |
| `resample` | 音频重采样 |
| `cut` | 基础剪辑 |

建议命令形式：

```bash
mini_ffmpeg probe input.mp4
mini_ffmpeg remux input.mkv output.mp4
mini_ffmpeg transcode input.mp4 output.mp4 --scale 1280x720
mini_ffmpeg resample input.wav output.wav --ar 44100 --ac 1
mini_ffmpeg cut input.mp4 output.mp4 --start 3 --duration 5
```

建议返回码：

| 返回码 | 含义 |
|--------|------|
| 0 | 成功 |
| 2 | 参数错误 |
| 3 | 输入文件错误 |
| 4 | 解码错误 |
| 5 | 编码错误 |
| 6 | 封装错误 |
| 7 | 滤镜或重采样错误 |

### 8.3 阶段项目二：mini_player

目标是完成一个基础本地播放器，至少支持：

- 打开本地媒体文件
- 视频渲染
- 音频播放
- 音画同步
- 暂停、继续
- seek
- 基本状态显示或日志输出

建议架构：

- `DemuxThread`
- `VideoDecodeThread`
- `AudioDecodeThread`
- `PacketQueue`
- `VideoFrameQueue`
- `AudioFrameQueue`
- `AudioClock`
- `VideoClock`
- `PlayerState`

建议状态机：

- `Idle`
- `Opening`
- `Playing`
- `Paused`
- `Seeking`
- `Buffering`
- `Stopped`
- `Error`

同步策略：

- 默认以音频时钟为主时钟
- 视频帧早到则等待
- 视频帧晚到超过阈值则丢帧
- seek 后清空队列并重置 clock
- 队列长度、pts、delay、drop frame 计数必须可观测

### 8.4 结课项目要求

结课项目建议二选一：

- 方案 A：完善 `mini_player`
- 方案 B：实现简易直播链路实验

结课项目必须具备：

- 可运行演示
- 可复现实验步骤
- 清晰 README
- 基础故障排查说明
- 可录屏展示的最终效果

---

## 9. 评估与验收标准

### 9.1 每节课验收

每节课至少满足：

- 能解释本节核心概念
- 能独立跑通核心实验
- 能根据预期输出判断结果是否正确
- 能定位至少一种常见错误
- 能说明本节与后续项目的关系

### 9.2 阶段 checkpoint 与项目验收

关键学习阶段应设置 checkpoint 或阶段项目验收。当前规划中，模块一使用 checkpoint 汇总媒体分析能力，模块三和模块四分别通过 `mini_ffmpeg` 与 `mini_player` 承担阶段项目验收。

阶段检查要求学员：

- 独立完成一个小任务或阶段项目
- 将本阶段知识串成一个完整链路
- 能用自己的话解释关键设计取舍
- 产出可以保存到作品集的结果

### 9.3 工程验收

课程主线代码必须满足：

- 示例工程可以独立构建和运行
- FFmpeg API 调用必须检查返回值
- 关键资源释放必须经过人工检查
- Debug 模式建议通过 ASAN 检查
- 命令行工具必须提供最小可用帮助信息
- 输出媒体文件可被 `ffprobe` 正常解析

### 9.4 自动验证建议

Python 阶段建议验证：

- Notebook 能从头执行到尾
- 输出文件存在且大小合理
- 图像 shape、音频采样率、声道数符合预期
- 涉及显示时显式处理 BGR/RGB 转换

C++ 阶段建议验证：

- CMake 配置成功
- Debug 构建成功
- 示例程序返回码为 0
- ASAN 无明显泄漏
- 输出 YUV/PCM/MP4 可被工具验证
- 关键日志包含输入、输出、时间戳、队列或错误信息

### 9.5 结课验收

满足以下条件可视为达到课程目标：

- 能独立分析至少 3 类媒体输入
- 能完成一个可运行的基础媒体处理工具
- 能完成一个支持音画同步的基础播放器，或一个可验证的推拉流链路
- 能解释时间戳、关键帧、转封装、重编码的区别与应用场景
- 能定位至少 3 类常见问题：资源泄漏、不同步、格式不兼容、seek 异常、码流信息误判

---

## 10. 单节课教学模板

每节课讲义必须遵循以下结构。讲义开头建议包含元数据表，便于自动生成课程索引和进度。

```markdown
# 第X节：[课程标题]

| 字段 | 内容 |
|------|------|
| lesson_id | XX |
| module | moduleX_xxx |
| type | python_visualization / cpp_ffmpeg / project |
| prerequisites | 前置课程 |
| outputs | 本节产出 |
| estimated_time | 预计学习时间 |
| runtime_limit | 实验运行时间目标 |
| assets | 使用素材 |
| notebook/source | 对应代码入口 |

## 本节能力目标
- 学完后应该会什么
- 能独立完成什么操作
- 能解决什么类型的问题

## 真实工程对应场景
- 本节内容在播放器、转码、推流、排障中的应用位置

## 引言
[简短介绍本节课要解决的问题和学习目标，1-2段话]

## 理论讲解

### 核心概念1：[概念名称]
[详细讲解概念原理，配合公式或图示]

### 核心概念2：[概念名称]
[详细讲解概念原理，配合公式或图示]

## 关键点总结
- 要点1
- 要点2
- 要点3

## 环境与素材准备

### 环境要求
```bash
# 列出本节课需要的依赖
```

### 测试素材
```bash
# 准备测试文件或生成命令
```

## 核心代码实验

### 实验1：[实验标题]
**目标：** [说明实验目的]

**代码实现：**
[Notebook 单元或源码文件路径]

**代码解析：**
- [关键点1解释]
- [关键点2解释]

## 预期结果与验收标准

### 预期输出
```text
[展示终端输出或可视化结果]
```

### 验收标准
- 程序成功运行
- 输出结果符合预期
- 学员能解释关键现象

## 常见错误与排查
- 错误1：原因与定位方法
- 错误2：原因与定位方法
- 错误3：原因与定位方法

## 课后小挑战
1. [基础题] 修改代码参数，观察变化
2. [进阶题] 实现额外功能
3. [思考题] 探索边界情况

## 面试延伸题
- 面试官可能会怎么问
- 应如何解释本节关键概念

## 延伸阅读
- [推荐资料1]
- [推荐资料2]
```

---

## 11. 轻量化测试素材规范

### 11.1 素材规格要求

**视频素材：**

- 时长：5-10 秒
- 分辨率：360p 或 720p
- 编码：H.264 为主，必要时补充 H.265
- 容器：MP4 为主，必要时补充 MKV、FLV、TS
- 码率：500-1500 kbps

**音频素材：**

- 时长：5-10 秒
- 采样率：44.1kHz 或 48kHz
- 声道：单声道或立体声
- 格式：WAV、AAC

**图片素材：**

- 分辨率：720p 左右
- 格式：JPG 或 PNG

### 11.2 素材设计原则

- 优先使用可在 3 秒内处理完的轻量素材
- 同一实验尽量只引入一个新变量
- 主线课程使用统一测试素材，减少学习者环境差异
- 对需要对比的实验，明确给出“输入 A / 输入 B”
- 素材必须记录来源、生成命令或许可证信息

### 11.3 素材清单建议

建议维护 `assets/MANIFEST.md`，记录：

- 文件名
- 类型
- 时长
- 分辨率
- 编码格式
- 容器格式
- 采样率/声道数
- 来源或生成命令
- 使用章节
- 许可证

---

## 12. 实现注意事项与最佳实践

### 12.1 内存管理（C++ 篇）

- 所有 `av_frame_alloc()` 分配的帧必须用 `av_frame_free()` 释放
- 所有 `av_packet_alloc()` 分配的包必须用 `av_packet_free()` 释放
- 使用 `av_frame_unref()` 和 `av_packet_unref()` 清空复用对象
- Debug 模式默认启用 ASAN
- 不允许忽略 FFmpeg API 的负数返回值

```cpp
AVFrame *frame = av_frame_alloc();
// 使用 frame
av_frame_free(&frame);

AVPacket *packet = av_packet_alloc();
// 使用 packet
av_packet_unref(packet);
av_packet_free(&packet);
```

### 12.2 颜色空间与像素格式

- OpenCV 默认使用 BGR
- Matplotlib 默认使用 RGB
- YUV420P、NV12、RGB24、BGR24 等格式要明确区分
- 涉及图像显示时，要明确步长、对齐、平面结构
- Python 阶段涉及 OpenCV + Matplotlib 时必须显式 BGR ↔ RGB 转换

### 12.3 时间戳处理

- 明确区分 PTS 与 DTS
- 所有时间戳计算都必须带 `time_base`
- seek、重封装、同步逻辑都依赖正确的时间基换算
- C++ 阶段优先使用 `av_rescale_q` 和 `av_packet_rescale_ts`

```cpp
int64_t out_pts = av_rescale_q(packet->pts, in_stream->time_base, out_stream->time_base);
```

### 12.4 错误处理

- FFmpeg API 返回值必须检查
- 失败时使用 `av_strerror` 输出错误信息
- 文档中应记录本节常见错误和复现方式

```cpp
int ret = avcodec_send_packet(codec_ctx, packet);
if (ret < 0) {
    char err_buf[128];
    av_strerror(ret, err_buf, sizeof(err_buf));
    fprintf(stderr, "Error sending packet: %s\n", err_buf);
    return -1;
}
```

### 12.5 性能与工程意识

- 尽量避免无意义的数据拷贝
- 避免循环中频繁申请和释放大块内存
- 对队列长度、缓冲区占用和同步阈值保持可观测性
- 区分“功能正确”和“工程可用”

---

## 13. 常见问题排查能力要求

每个模块至少覆盖以下一类排障问题：

- 文件能播但 seek 不准
- 封装成功但播放器打不开
- 有视频无声音，或有声音无视频
- 音画同步逐渐漂移
- 重采样后音频速度异常
- 解码成功但颜色错误
- 转封装后首帧黑屏或无法拖动
- 程序退出时泄漏或崩溃
- HLS 延迟过高或首屏过慢
- H.264 码流在不同容器间转换失败

每节课应明确：

- 如何复现问题
- 如何观察现象
- 如何用工具定位
- 如何修复

---

## 14. 项目目录结构规划

当前仓库根目录为 `dive-into-av/`。文档中统一使用相对路径描述目录结构。

```text
./
├── README.md
├── COURSE_BLUEPRINT.md
├── PROGRESS.md
├── requirements.txt
├── docs/
│   ├── module1_basics/
│   ├── module2_codec/
│   ├── module3_pipeline/
│   ├── module4_player/
│   └── module5_streaming/
├── notebooks/
│   ├── module1/
│   └── module2/
├── src/
│   ├── common/              # 公共工具、日志、参数解析
│   ├── module2/
│   ├── module3/
│   ├── module4/
│   └── projects/
│       ├── mini_ffmpeg/
│       └── mini_player/
├── assets/
│   ├── MANIFEST.md
│   ├── scripts/
│   ├── images/
│   ├── audio/
│   └── video/
├── output/
│   ├── yuv/
│   ├── pcm/
│   ├── encoded/
│   └── reports/
├── docker/
│   ├── Dockerfile.ubuntu
│   └── docker-compose.yml
└── CMakeLists.txt
```

---

## 15. 课程开发检查清单

### 15.1 每节课开发前

- [ ] 确认本节属于主线还是扩展
- [ ] 从 `PROGRESS.md` 确认当前节次和实际进度
- [ ] 检查 `README.md`、`PROGRESS.md` 是否存在进度冲突
- [ ] 明确本节能力目标和工程场景
- [ ] 准备测试素材并验证可运行
- [ ] 明确本节最常见的 3 个坑

### 15.2 每节课开发中

- [ ] 编写 Markdown 讲义
- [ ] 创建 Notebook 或 C++ 示例工程
- [ ] 补齐运行命令、预期输出和验收标准
- [ ] 补齐常见错误与排查说明
- [ ] 验证代码和命令可以独立执行
- [ ] Python 显示代码明确处理 BGR/RGB
- [ ] C++ FFmpeg 代码检查返回值和资源释放

### 15.3 每节课开发后

- [ ] 本地运行最小验证
- [ ] 调用 Reviewer 审查
- [ ] 修复 Reviewer 的 BLOCKER 和 MAJOR 问题
- [ ] 更新 README 与 PROGRESS
- [ ] 记录素材来源和生成方式
- [ ] 准备下一节课依赖的前置内容

---

## 16. 进度管理规则

- `PROGRESS.md` 是唯一实际进度真源
- `README.md` 展示面向学习者的简化进度与入口链接
- `COURSE_BLUEPRINT.md` 只在课程结构、模板、规范、阶段划分变化时更新
- 每节课完成后默认更新 `README.md` 和 `PROGRESS.md`
- 不再在本蓝图中维护逐节完成状态，避免多文件状态漂移

---

**文档版本：** v2.1  
**维护者：** opencode
