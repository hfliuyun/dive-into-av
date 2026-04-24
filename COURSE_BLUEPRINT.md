# 《动手学音视频开发》课程建设蓝图

本文档包含完整的课程设计蓝图，包括技术栈选型、课程大纲、实现规范等，供后续课程开发参考。

---

## 📦 技术栈选型与环境依赖

### 基础实验篇（Python + 可视化）

**语言与工具：**
- Python 3.8+
- Jupyter Notebook/Lab（交互式实验环境）

**核心库：**
- **PyAV**（FFmpeg的Python绑定，用于音视频处理）
- **OpenCV**（cv2，图像处理与显示）
- **NumPy**（数组运算与数据处理）
- **Matplotlib**（数据可视化，绘制波形、频谱）
- **PyAudio/PySoundFile**（音频I/O）
- **Pillow**（图像处理）

**环境配置：**
```bash
# 推荐使用 Conda 管理环境
conda create -n av-learn python=3.9
conda activate av-learn
conda install -c conda-forge av opencv numpy matplotlib pyaudio
pip install pysoundfile pillow jupyter
```

### 工程实战篇（C/C++ + FFmpeg API）

**语言与工具：**
- C++17（现代C++特性）
- CMake 3.15+（跨平台构建）
- GDB/LLDB（调试工具）
- **AddressSanitizer (ASAN)** - 内存泄漏检测工具

**核心库：**
- FFmpeg 开发库（libavformat, libavcodec, libavutil, libswscale, libswresample, libavfilter）
- SDL2（跨平台音视频播放）
- OpenGL（可选，高级渲染）

**CMake编译配置（启用ASAN）：**
```cmake
# 开发模式启用ASAN
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()
```

**环境配置：**
```bash
# 方案1：Docker（推荐，零配置）
docker pull jrottenberg/ffmpeg:4.4-ubuntu20.04

# 方案2：手动安装（Ubuntu）
sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev \
    libswscale-dev libswresample-dev libavfilter-dev libsdl2-dev \
    cmake build-essential
```

---

## 📚 完整课程大纲 (Syllabus)

### 模块一：音视频基础概念与数据可视化
**目标：建立对音视频数据的直观认识，掌握色彩空间、采样率等核心概念**

- **第1节：数字图像基础** ✅ 已完成
  - [核心概念] 像素、分辨率、位深度、RGB色彩模型
  - [核心实验] 用Python读取图片，打印像素值，修改特定区域颜色并保存

- **第2节：视频的色彩空间**
  - [核心概念] RGB与YUV420P的原理、转换公式、人眼视觉特性
  - [核心实验] 剥离MP4文件的YUV数据，将U/V分量涂黑，观察画面变化

- **第3节：数字音频基础**
  - [核心概念] 采样率、位深度、声道数、PCM数据格式
  - [核心实验] 录制一段音频，用Matplotlib绘制波形图和频谱图

- **第4节：音视频容器格式与编解码**
  - [核心概念] 封装格式（MP4/MKV/FLV）与编解码器（H.264/AAC）的区别
  - [核心实验] 用ffprobe分析媒体文件，打印流信息、编码参数

- **第5节：帧率、码率与时间戳**
  - [核心概念] FPS、PTS/DTS、码率计算、VBR/CBR
  - [核心实验] 提取视频帧序列，分析帧间隔与时间戳关系

### 模块二：音视频编解码核心原理
**目标：深入理解编解码流程，掌握FFmpeg API的基本使用**

- **第6节：视频编码原理（DCT + 量化）**
  - [核心概念] 空域到频域变换、量化、熵编码
  - [核心实验] 手动实现简化的DCT变换，观察图像压缩效果

- **第7节：H.264关键帧与参考帧**
  - [核心概念] I/P/B帧、GOP结构、运动估计
  - [核心实验] 提取I帧和P帧，对比文件大小与画质差异

- **第8节：音频编码原理（AAC/MP3）**
  - [核心概念] MDCT变换、心理声学模型、比特池
  - [核心实验] 对比不同码率下AAC编码的频谱差异

- **第9节：FFmpeg解码器API入门**
  - [核心概念] AVCodecContext、AVPacket、AVFrame数据结构
  - [核心实验] 用C++编写最简视频解码器，输出YUV帧
  - **🚀 进阶挑战：** 探索NVENC/VideoToolbox/VAAPI硬件解码器开启方式

- **第10节：FFmpeg内存模型与引用计数**
  - [核心概念] **AVBufferRef、引用计数机制、av_frame_unref、av_packet_unref**
  - [核心实验] 编写内存泄漏检测程序，用ASAN验证资源释放正确性

- **第11节：FFmpeg编码器API入门**
  - [核心概念] 编码参数配置、码流封装
  - [核心实验] 将YUV序列编码为H.264文件
  - **🚀 进阶挑战：** 尝试开启NVENC/VideoToolbox硬件编码器

### 模块三：音视频解封装、封装与处理管道
**目标：掌握完整的媒体文件处理流程**

- **第12节：解封装与封装API详解**
  - [核心概念] **AVFormatContext、流索引、时间基转换、Muxer/Demuxer对称性**
  - [核心实验] **将纯H.264裸流和AAC裸流混合打包（Mux）成一个完整的MP4文件**

- **第13节：解码管道构建**
  - [核心概念] 解封装→解码→后处理完整流程
  - [核心实验] 将MP4解码为原始YUV/PCM文件

- **第14节：图像滤镜与转码（libavfilter + libswscale）**
  - [核心概念] **libavfilter滤镜图、滤镜链、水印叠加、分辨率缩放**
  - [核心实验] **用C++ API给视频画面添加自定义水印（文字或图片），并实现格式转换**

- **第15节：音频重采样**
  - [核心概念] 采样率转换、声道映射、音量控制
  - [核心实验] 将48kHz立体声转为44.1kHz单声道

- **第16节：音视频剪辑与拼接**
  - [核心概念] 精确剪辑（seek）、流拷贝模式
  - [核心实验] 实现视频转码工具（分辨率+格式转换）

### 模块四：音视频播放与同步
**目标：实现一个功能完整的播放器**

- **第17节：SDL2渲染基础**
  - [核心概念] SDL窗口、纹理、渲染循环
  - [核心实验] 用SDL显示一张静态YUV图像

- **第18节：音频播放与回调机制**
  - [核心概念] SDL音频队列、回调函数、缓冲区管理
  - [核心实验] 播放PCM音频文件

- **第19节：视频播放循环**
  - [核心概念] 帧率控制、刷新率同步
  - [核心实验] 实现无音频的视频播放器

- **第20节：音视频同步原理**
  - [核心概念] 音频时钟、视频追赶、同步阈值
  - [核心实验] 实现音视频同步播放器（基于音频时钟）

- **第21节：播放控制功能**
  - [核心概念] 暂停/继续、快进/快退、进度条
  - [核心实验] 为播放器添加播放控制功能

### 模块五：网络流媒体与实时通信
**目标：掌握流媒体传输与实时通信技术**

- **第22节：RTMP推流与拉流**
  - [核心概念] RTMP协议、握手过程、分片传输
  - [核心实验] 将本地文件推流到RTMP服务器

- **第23节：HLS流媒体生成**
  - [核心概念] M3U8播放列表、TS分片、码率自适应
  - [核心实验] 将视频切片为HLS流

- **第24节：WebRTC基础架构**
  - [核心概念] SDP协商、ICE打洞、STUN/TURN
  - [核心实验] 搭建WebRTC信令服务器

- **第25节：实时音视频采集**
  - [核心概念] 摄像头/麦克风采集、编码参数优化
  - [核心实验] 实现实时摄像头采集与推流

- **第26节：项目实战：简易直播系统**
  - [核心概念] 端到端延迟优化、丢包恢复
  - [核心实验] 完整实现"采集→编码→推流→播放"直播链路

---

## 📁 项目目录结构规划

```
dive_into_media/
├── README.md                           # 项目总览
├── COURSE_BLUEPRINT.md                 # 课程建设蓝图（本文件）
├── requirements.txt                    # Python依赖
├── docs/                              # 课程讲义
│   ├── module1_basics/               # 模块一：基础概念
│   │   ├── lesson01_pixel.md         ✅ 已完成
│   │   ├── lesson02_colorspace.md
│   │   ├── lesson03_audio.md
│   │   ├── lesson04_container.md
│   │   └── lesson05_timestamp.md
│   ├── module2_codec/                # 模块二：编解码
│   ├── module3_pipeline/             # 模块三：处理管道
│   ├── module4_player/               # 模块四：播放器
│   └── module5_streaming/            # 模块五：流媒体
├── notebooks/                        # Jupyter实验笔记本
│   ├── module1/
│   │   ├── 01_pixel_exploration.ipynb  ✅ 已完成
│   │   ├── 02_yuv_manipulation.ipynb
│   │   └── ...
│   └── ...
├── src/                              # C++工程代码
│   ├── module2/
│   │   ├── lesson09_decoder/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── main.cpp
│   │   │   └── README.md
│   │   └── ...
│   ├── module3/
│   ├── module4/
│   └── module5/
├── assets/                           # 测试媒体素材
│   ├── images/
│   │   └── test_720p.jpg
│   ├── audio/
│   │   └── test_48k_stereo.wav
│   └── video/
│       └── test_720p_5s.mp4
├── output/                           # 实验输出目录
│   ├── yuv/
│   ├── pcm/
│   └── encoded/
├── docker/                           # Docker配置
│   ├── Dockerfile.ubuntu
│   └── docker-compose.yml
└── CMakeLists.txt                    # 根CMake配置
```

---

## 📝 单节课教学模板

```markdown
# 第X节：[课程标题]

## 📚 引言
[简短介绍本节课要解决的问题和学习目标，1-2段话]

## 🔬 理论讲解

### 核心概念1：[概念名称]
[详细讲解概念原理，配合公式/图示说明]

### 核心概念2：[概念名称]
[详细讲解概念原理，配合公式/图示说明]

### 关键点总结
- 要点1
- 要点2
- 要点3

## 🛠️ 环境与素材准备

### 环境要求
```bash
# 列出本节课需要的依赖
pip install xxx
```

### 测试素材
```bash
# 下载或准备测试文件
wget https://example.com/test.mp4 -O assets/video/test.mp4
```

## 💻 核心代码实验

### 实验1：[实验标题]
**目标：** [说明实验目的]

**代码实现：**
```python
# 分段1：初始化
# [代码]

# 分段2：核心处理逻辑
# [代码]

# 分段3：结果输出
# [代码]
```

**代码解析：**
- [关键点1解释]
- [关键点2解释]

### 实验2：[实验标题]
[重复上述结构]

## 📊 实验结果预期

### 预期输出
```
[展示预期的终端输出或可视化结果]
```

### 结果分析
[解释为什么会出现这样的结果，验证了什么理论]

## 🎯 课后小挑战

1. [基础题] 修改代码参数，观察变化
2. [进阶题] 实现额外功能
3. [思考题] 探索边界情况

## 📚 延伸阅读
- [推荐资料1]
- [推荐资料2]
```

---

## 🎬 轻量化测试素材规范

### 素材规格要求
```bash
# 视频素材规格（确保3秒内完成处理）
- 时长：5-10秒
- 分辨率：360p (640x360) 或 720p (1280x720)
- 编码：H.264
- 容器：MP4
- 码率：500-1500 kbps

# 音频素材规格
- 时长：5-10秒
- 采样率：44.1kHz 或 48kHz
- 声道：单声道或立体声
- 格式：WAV (PCM) 或 AAC
```

### 环境与素材准备模板（每节课统一）
```markdown
## 🛠️ 环境与素材准备

### 环境要求
```bash
# Python环境（基础实验篇）
pip install av opencv-python numpy matplotlib pillow

# C++环境（工程实战篇）
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

### 测试素材下载
```bash
# 创建素材目录
mkdir -p assets/video assets/audio assets/images

# 下载轻量化测试视频（5秒，360p，约500KB）
wget https://sample-videos.com/video123/mp4/360/big_buck_bunny_360p_5mb.mp4 \
     -O assets/video/test_360p_5s.mp4

# 下载测试音频（5秒，44.1kHz，单声道）
wget https://sample-videos.com/audio/mp3/crowd-cheering.mp3 \
     -O assets/audio/test_5s.mp3

# 下载测试图片
wget https://sample-videos.com/img/Sample-jpg-image-10mb.jpg \
     -O assets/images/test_720p.jpg
```

### 验证素材
```bash
# 确认视频时长和分辨率
ffprobe -v error -show_entries format=duration -show_entries stream=width,height \
        -of default=noprint_wrappers=1 assets/video/test_360p_5s.mp4
```
```

---

## ⚠️ 实现注意事项与最佳实践

### 1. 内存管理（C++篇）

**FFmpeg内存管理黄金法则：**
- 所有 `av_frame_alloc()` 分配的帧必须用 `av_frame_free()` 释放
- 所有 `av_packet_alloc()` 分配的包必须用 `av_packet_free()` 释放
- 使用 `av_frame_unref()` 和 `av_packet_unref()` 清空数据
- 启用ASAN进行内存泄漏检测

**示例代码：**
```cpp
AVFrame *frame = av_frame_alloc();
// ... 使用frame
av_frame_free(&frame);  // 必须释放

AVPacket *packet = av_packet_alloc();
// ... 使用packet
av_packet_unref(packet);  // 清空数据
av_packet_free(&packet);  // 释放结构
```

### 2. 颜色空间转换

**OpenCV vs matplotlib：**
- OpenCV使用BGR顺序
- matplotlib使用RGB顺序
- 转换：`cv2.cvtColor(img, cv2.COLOR_BGR2RGB)`

**YUV格式说明：**
- YUV420P：平面格式，Y、U、V分开存储
- YUV420SP：半平面格式，Y单独，UV交错
- 转换时注意对齐和填充

### 3. 时间戳处理

**关键概念：**
- PTS（Presentation Timestamp）：显示时间戳
- DTS（Decoding Timestamp）：解码时间戳
- 时间基（time_base）：时间单位，如 1/90000

**时间戳转换：**
```cpp
// 将PTS转换为秒
double timestamp_seconds = packet->pts * av_q2d(stream->time_base);

// 将秒转换为PTS
int64_t pts = (int64_t)(timestamp_seconds / av_q2d(stream->time_base));
```

### 4. 错误处理

**FFmpeg错误码检查：**
```cpp
int ret = avcodec_send_packet(codec_ctx, packet);
if (ret < 0) {
    char err_buf[128];
    av_strerror(ret, err_buf, sizeof(err_buf));
    fprintf(stderr, "Error sending packet: %s\n", err_buf);
    return -1;
}
```

### 5. 性能优化

**关键优化点：**
- 使用零拷贝API（如 `av_frame_ref()`）
- 避免频繁的内存分配/释放
- 使用硬件加速（NVENC/VideoToolbox/VAAPI）
- 批量处理而非逐帧处理

---

## 🔄 课程开发检查清单

### 每节课开发前
- [ ] 确认课程大纲中的核心概念和实验
- [ ] 准备测试素材（符合轻量化规范）
- [ ] 确定所需依赖库
- [ ] 设计实验代码结构

### 每节课开发中
- [ ] 编写Markdown讲义（遵循教学模板）
- [ ] 创建Jupyter Notebook或C++代码
- [ ] 添加详细代码注释
- [ ] 测试所有代码单元
- [ ] 验证实验结果

### 每节课开发后
- [ ] 更新README.md进度
- [ ] 提交Git commit（使用规范的commit message）
- [ ] 更新课程进度追踪表
- [ ] 准备下一节课的素材

---

## 📊 课程进度追踪

| 模块 | 章节 | 标题 | 状态 | 完成日期 |
|------|------|------|------|----------|
| 模块一 | 第1节 | 数字图像基础 | ✅ 已完成 | 2026-04-24 |
| 模块一 | 第2节 | 视频的色彩空间 | ⏳ 待开发 | - |
| 模块一 | 第3节 | 数字音频基础 | ⏳ 待开发 | - |
| 模块一 | 第4节 | 音视频容器格式与编解码 | ⏳ 待开发 | - |
| 模块一 | 第5节 | 帧率、码率与时间戳 | ⏳ 待开发 | - |
| 模块二 | 第6节 | 视频编码原理（DCT + 量化） | ⏳ 待开发 | - |
| ... | ... | ... | ... | ... |

---

## 🎯 下一步行动

1. ✅ 完成第1节课开发
2. ⏳ 开发第2节：视频的色彩空间
3. ⏳ 开发第3节：数字音频基础
4. ⏳ ...

---

**文档版本：** v1.0
**最后更新：** 2026-04-24
**维护者：** opencode
