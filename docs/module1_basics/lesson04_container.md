# 第4节：音视频容器格式与编解码

| 字段 | 内容 |
|------|------|
| lesson_id | 04 |
| module | module1_basics |
| type | python_visualization |
| prerequisites | 第1-3节 |
| outputs | 理解容器/码流/编解码器概念，能用 ffprobe 分析媒体文件 |
| estimated_time | 2-3 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.mp4, test.mkv, test.h264, test.aac |
| notebook/source | notebooks/module1/04_container_analysis.ipynb |

## 本节能力目标

- 理解容器（Container）、码流（Bitstream）、编解码器（Codec）的本质区别
- 理解封装（Mux）与解封装（Demux）的概念和作用
- 能用 `ffprobe` 分析 MP4、MKV、AAC、H.264 文件并解释输出
- 理解"转封装"与"转码"的本质区别
- 理解裸流与容器文件的差异，以及 extradata 的作用

## 真实工程对应场景

- 媒体文件探测：播放器打开文件前需要识别容器格式和编码类型
- 格式转换工具：将 MKV 转为 MP4，或将 FLV 转为 TS
- 流媒体推流：RTMP 推流需要将 MP4 解封装后重新封装为 FLV
- 视频剪辑：精确剪辑需要理解容器中的时间戳和关键帧
- 播放器开发：解封装是播放器的第一步，分离音频流和视频流

## 引言

在前三节课中，我们学习了数字图像（RGB/YUV）、数字视频（色彩空间）和数字音频（PCM/采样率）。但你是否想过：一个 MP4 文件是如何同时包含视频和音频的？为什么同一个 H.264 视频流可以放在 MP4、MKV、FLV 等不同容器中？

本节课我们将从**容器格式**出发，理解音视频数据的"打包"方式。你将学习如何用 `ffprobe` 这个强大的工具来"解剖"媒体文件，看清容器内部的结构——有哪些流、什么编码、什么参数。这是你成为音视频工程师的第一步工具技能。

---

## 理论讲解

### 核心概念1：容器 vs 码流 vs 编解码器

这三个概念是音视频领域最基础也最容易混淆的：

#### 1.1 编解码器（Codec）

编解码器是**算法**，用于将原始音视频数据压缩（编码）或解压（解码）。

| 类型 | 常见编解码器 | 作用 |
|------|-------------|------|
| 视频编码器 | H.264 (AVC), H.265 (HEVC), AV1 | 将原始视频数据（如 YUV）压缩为码流 |
| 音频编码器 | AAC, MP3, Opus, FLAC | 将原始音频数据（如 PCM）压缩为码流 |

**关键理解**：编解码器是"菜谱"，定义了如何压缩和解压数据。

#### 1.2 码流（Bitstream）

码流是编码后的**数据**，是编解码器的输出结果。

- H.264 码流：由 NALU（Network Abstraction Layer Unit）组成
- AAC 码流：由 ADTS 帧组成
- 码流本身是"裸数据"，不包含文件名、时长等元信息

**关键理解**：码流是"菜"，是按照菜谱做出来的成品。

#### 1.3 容器（Container）

容器是**包装盒**，用于将多路码流（视频、音频、字幕）打包成一个文件，并存储元信息（时长、分辨率、标题等）。

| 容器格式 | 文件扩展名 | 特点 |
|----------|-----------|------|
| MP4 | .mp4 | 最通用，支持 H.264/AAC，适合网络传输 |
| MKV | .mkv | 开放格式，支持多音轨/字幕，适合本地存储 |
| FLV | .flv | 流媒体时代产物，RTMP 推流常用 |
| TS | .ts | 广播电视，支持边下边播，HLS 切片基础 |
| AVI | .avi | 老旧格式，兼容性好但功能有限 |

**关键理解**：容器是"外卖盒"，可以同时装视频（主菜）、音频（配菜）和字幕（餐具）。

#### 三者关系图

```
┌─────────────────────────────────────────────────────────┐
│                      容器 (MP4/MKV)                      │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────┐  │
│  │   视频码流       │  │   音频码流       │  │  字幕   │  │
│  │  (H.264/HEVC)   │  │   (AAC/MP3)     │  │ (SRT)   │  │
│  └────────┬────────┘  └────────┬────────┘  └─────────┘  │
│           │                    │                         │
│  ┌────────▼────────┐  ┌────────▼────────┐               │
│  │   视频解码器     │  │   音频解码器     │               │
│  │   (H.264 解码)   │  │   (AAC 解码)     │               │
│  └────────┬────────┘  └────────┬────────┘               │
│           │                    │                         │
│  ┌────────▼────────┐  ┌────────▼────────┐               │
│  │   YUV 原始帧     │  │   PCM 原始音频   │               │
│  └─────────────────┘  └─────────────────┘               │
└─────────────────────────────────────────────────────────┘
```

---

### 核心概念2：封装与解封装

#### 2.1 解封装（Demux）

解封装是从容器中**分离**出各路码流的过程。

```
MP4 文件 → 解封装 → H.264 视频码流 + AAC 音频码流 + 字幕
```

解封装时会读取：
- 容器头信息（时长、分辨率、采样率等）
- 各路流的参数（编码类型、码率、time_base）
- extradata（SPS/PPS、AudioSpecificConfig）

#### 2.2 封装（Mux）

封装是将多路码流**打包**进容器的过程。

```
H.264 视频码流 + AAC 音频码流 → 封装 → MP4 文件
```

封装时需要：
- 写入容器头信息
- 为每个包写入时间戳（PTS/DTS）
- 处理流之间的同步关系

#### 2.3 转封装（Remux）

转封装是**不改变码流数据**，只更换容器格式的过程。

```
MKV (H.264 + AAC) → 转封装 → MP4 (H.264 + AAC)
```

**关键理解**：转封装就像把外卖盒里的食物换到另一个盒子里，食物本身不变。

---

### 核心概念3：裸流与容器文件

#### 3.1 H.264 裸流 vs 容器中的 H.264

| 特征 | H.264 裸流 (.h264) | MP4 中的 H.264 |
|------|-------------------|----------------|
| 封装格式 | Annex B | AVCC (长度前缀) |
| SPS/PPS | 内嵌在码流中 | 存储在 extradata 中 |
| 文件头 | 无 | 有 moov box |
| 可直接播放 | 需要外部提供参数 | 可以 |

#### 3.2 AAC 裸流 vs 容器中的 AAC

| 特征 | AAC 裸流 (.aac) | MP4 中的 AAC |
|------|----------------|--------------|
| 封装格式 | ADTS | raw AAC |
| 音频配置 | 每帧 ADTS 头 | 存储在 extradata 中 |
| 文件头 | 无 | 有 moov box |

#### 3.3 Extradata 的作用

Extradata 是存储在容器中的**编解码器初始化数据**：

- **H.264**：SPS（序列参数集）和 PPS（图像参数集），包含分辨率、帧率、profile 等信息
- **AAC**：AudioSpecificConfig，包含采样率、声道数、编码 profile 等信息

```
播放器打开 MP4 → 读取 extradata → 初始化解码器 → 开始解码码流
```

---

### 核心概念4：容器格式详细对比

| 特性 | MP4 | MKV | FLV | TS |
|------|-----|-----|-----|-----|
| 开发者 | MPEG | 开源社区 | Adobe | MPEG |
| 开放标准 | 是 | 是 | 否 | 是 |
| 视频编码 | H.264/HEVC/AV1 | 几乎所有 | H.264/HEVC | H.264/HEVC |
| 音频编码 | AAC/MP3/AC3 | 几乎所有 | AAC/MP3 | AAC/MP3/AC3 |
| 字幕支持 | 有限 | 丰富 | 无 | 有限 |
| 多音轨 | 支持 | 支持 | 不支持 | 支持 |
| 流媒体 | 支持 | 不适合 | 适合 | 非常适合 |
| 典型场景 | 网络视频 | 本地收藏 | 直播推流 | 直播/HLS |

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **容器是包装盒** | MP4/MKV/FLV 是容器，负责打包多路码流和元信息 |
| **码流是内容** | H.264/AAC 是码流，是编码后的音视频数据 |
| **编解码器是算法** | 编码器压缩、解码器解压，算法决定压缩效率和质量 |
| **封装/解封装是操作** | 封装打包、解封装分离，转封装只换盒子不换内容 |
| **Extradata 是钥匙** | SPS/PPS 和 AudioSpecificConfig 是解码器初始化的必要数据 |

---

## 环境与素材准备

### 环境要求

```bash
# 确保 ffmpeg 和 ffprobe 已安装
ffmpeg -version
ffprobe -version

# Python 依赖
pip install jupyter notebook
```

### 测试素材

```bash
# 使用脚本生成测试素材
chmod +x assets/scripts/generate_test_media.sh
./assets/scripts/generate_test_media.sh

# 或手动执行以下命令：
# 生成 MP4 测试文件（5秒，720p，H.264+AAC）
ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
       -f lavfi -i sine=frequency=440:duration=5 \
       -c:v libx264 -c:a aac -shortest -y test.mp4

# 生成 MKV 测试文件（相同内容）
ffmpeg -i test.mp4 -c copy -y test.mkv

# 提取 H.264 裸流
ffmpeg -i test.mp4 -c:v copy -an -y test.h264

# 提取 AAC 裸流
ffmpeg -i test.mp4 -c:a copy -vn -y test.aac
```

---

## 核心代码实验

### 实验1：用 ffprobe 分析不同格式文件

**目标**：理解 ffprobe 输出结构，识别容器、编码、流信息

**代码实现：**
```python
import subprocess
import json

def probe_file(file_path):
    """使用 ffprobe 分析媒体文件"""
    cmd = [
        'ffprobe',
        '-v', 'quiet',
        '-print_format', 'json',
        '-show_format',
        '-show_streams',
        file_path
    ]
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    return json.loads(result.stdout)

# 分析 MP4 文件
print("=== MP4 文件分析 ===")
mp4_info = probe_file('test.mp4')
print(f"容器格式: {mp4_info['format']['format_name']}")
print(f"时长: {float(mp4_info['format']['duration']):.2f} 秒")
print(f"流数量: {mp4_info['format']['nb_streams']}")

for stream in mp4_info['streams']:
    if stream['codec_type'] == 'video':
        print(f"\n视频流:")
        print(f"  编码: {stream['codec_name']}")
        print(f"  分辨率: {stream['width']}x{stream['height']}")
        print(f"  帧率: {stream['r_frame_rate']}")
    elif stream['codec_type'] == 'audio':
        print(f"\n音频流:")
        print(f"  编码: {stream['codec_name']}")
        print(f"  采样率: {stream['sample_rate']} Hz")
        print(f"  声道数: {stream['channels']}")
```

**代码解析：**
- `ffprobe` 的 `-print_format json` 参数输出 JSON 格式，便于程序解析
- `-show_format` 显示容器级别的信息（时长、码率、格式名）
- `-show_streams` 显示每个流的详细信息（编码、分辨率、采样率等）

---

### 实验2：提取并对比裸流

**目标**：理解容器封装与裸流的区别

**代码实现：**
```python
import os

def extract_streams(input_file, output_prefix):
    """从容器中提取裸流"""
    # 提取视频流
    cmd_video = [
        'ffmpeg', '-i', input_file,
        '-c:v', 'copy', '-an',
        '-y', f'{output_prefix}_video.h264'
    ]
    
    # 提取音频流
    cmd_audio = [
        'ffmpeg', '-i', input_file,
        '-c:a', 'copy', '-vn',
        '-y', f'{output_prefix}_audio.aac'
    ]
    
    subprocess.run(cmd_video, capture_output=True)
    subprocess.run(cmd_audio, capture_output=True)
    
    # 对比文件大小
    original_size = os.path.getsize(input_file)
    video_size = os.path.getsize(f'{output_prefix}_video.h264')
    audio_size = os.path.getsize(f'{output_prefix}_audio.aac')
    
    print(f"原始文件: {original_size / 1024:.1f} KB")
    print(f"视频裸流: {video_size / 1024:.1f} KB")
    print(f"音频裸流: {audio_size / 1024:.1f} KB")
    print(f"裸流总和: {(video_size + audio_size) / 1024:.1f} KB")
    print(f"容器开销: {(original_size - video_size - audio_size) / 1024:.1f} KB")

# 从 MP4 提取裸流
print("=== 从 MP4 提取裸流 ===")
extract_streams('test.mp4', 'extracted')
```

**代码解析：**
- `-c:v copy -an`：只复制视频流，丢弃音频流
- `-c:a copy -vn`：只复制音频流，丢弃视频流
- 裸流大小之和通常小于容器文件，差值是容器头和索引的开销

---

### 实验3：封装格式转换实验

**目标**：理解"转封装"与"转码"的区别

**代码实现：**
```python
import time

def remux_file(input_file, output_file):
    """转封装：只更换容器，不改变码流"""
    cmd = [
        'ffmpeg', '-i', input_file,
        '-c', 'copy',  # 关键：copy 表示不重新编码
        '-y', output_file
    ]
    
    start = time.time()
    subprocess.run(cmd, capture_output=True)
    elapsed = time.time() - start
    
    return elapsed

def transcode_file(input_file, output_file):
    """转码：重新编码码流"""
    cmd = [
        'ffmpeg', '-i', input_file,
        '-c:v', 'libx264',  # 重新编码视频
        '-c:a', 'aac',      # 重新编码音频
        '-y', output_file
    ]
    
    start = time.time()
    subprocess.run(cmd, capture_output=True)
    elapsed = time.time() - start
    
    return elapsed

# 对比转封装和转码的速度
print("=== 转封装 vs 转码对比 ===")

remux_time = remux_file('test.mkv', 'remuxed.mp4')
print(f"转封装 (MKV → MP4): {remux_time:.3f} 秒")

transcode_time = transcode_file('test.mkv', 'transcoded.mp4')
print(f"转码 (MKV → MP4): {transcode_time:.3f} 秒")

print(f"\n速度比: 转封装比转码快 {transcode_time / remux_time:.1f} 倍")
```

**代码解析：**
- `-c copy`：转封装模式，直接复制码流，速度极快
- `-c:v libx264 -c:a aac`：转码模式，重新编码，速度慢但可改变编码参数
- 转封装通常比转码快 10-100 倍，因为不需要编解码计算

---

## 预期结果与验收标准

### 预期输出

```text
=== MP4 文件分析 ===
容器格式: mov,mp4,m4a,3gp,3g2,mj2
时长: 5.00 秒
流数量: 2

视频流:
  编码: h264
  分辨率: 1280x720
  帧率: 30/1

音频流:
  编码: aac
  采样率: 44100 Hz
  声道数: 2

=== 从 MP4 提取裸流 ===
原始文件: 312.5 KB
视频裸流: 298.3 KB
音频裸流: 12.8 KB
裸流总和: 311.1 KB
容器开销: 1.4 KB

=== 转封装 vs 转码对比 ===
转封装 (MKV → MP4): 0.023 秒
转码 (MKV → MP4): 1.247 秒

速度比: 转封装比转码快 54.2 倍
```

### 验收标准

- [ ] 能用 ffprobe 分析 MP4/MKV 文件并正确识别容器格式
- [ ] 能解释 ffprobe 输出中的 codec_type、codec_name、width、height、sample_rate 等字段
- [ ] 能用 ffmpeg 从容器中提取 H.264 裸流和 AAC 裸流
- [ ] 能说明转封装与转码的区别（速度、输出大小、码流是否变化）
- [ ] 能解释 extradata（SPS/PPS）的作用

---

## 常见错误与排查

### 错误1：ffprobe 输出 "Invalid data found when processing input"

**原因**：文件损坏或格式不支持，可能是下载不完整或编码格式特殊。

**排查**：
- 用 `file` 命令检查文件类型
- 尝试用 `ffmpeg -i file.mp4` 查看是否有错误信息
- 重新生成测试文件

### 错误2：提取的裸流无法播放

**原因**：裸流缺少容器头信息，播放器不知道分辨率、采样率等参数。

**排查**：
- H.264 裸流需要指定分辨率：`ffplay -video_size 1280x720 test.h264`
- AAC 裸流通常可以直接播放：`ffplay test.aac`
- 使用容器文件（MP4/MKV）进行播放测试

### 错误3：转封装后文件大小变化较大

**原因**：不同容器的索引结构和元数据存储方式不同。

**排查**：
- 正常现象，MP4 的 moov box 大小与 MKV 的 segment header 不同
- 如果变化过大（>50%），检查是否意外触发了重编码
- 用 `ffprobe` 对比输入输出的码流编码信息

### 错误4：ffprobe 报错 "moov atom not found"

**原因**：MP4 文件的 moov atom（索引信息）在文件末尾，下载未完成或文件损坏。

**排查**：
- 使用 `-movflags faststart` 重新封装，将 moov 移到文件开头：
  ```bash
  ffmpeg -i broken.mp4 -c copy -movflags faststart fixed.mp4
  ```
- 这是网络视频的常见优化，确保边下边播

---

## 课后小挑战

### 基础题：对比不同容器的元数据

用 ffprobe 分析 test.mp4 和 test.mkv，对比两者的 format 信息有何差异（时长、码率、格式名、metadata）。

### 进阶题：提取关键帧

用 ffmpeg 提取 test.mp4 的所有关键帧（I帧）并保存为图片：
```bash
ffmpeg -i test.mp4 -vf "select=eq(pict_type\,I)" -vsync vfr iframe_%03d.png
```
分析关键帧数量与 GOP 长度的关系。

### 思考题1：为什么 MP4 适合网络传输而 MKV 不适合？

提示：考虑 moov atom 的位置、索引结构、流媒体协议的要求。

### 思考题2：RTMP 推流为什么使用 FLV 容器？

提示：考虑 FLV 的结构特点、流媒体推拉流的实时性需求、以及与 H.264/AAC 的兼容性。

---

## 面试延伸题

### Q1：容器和编解码器的区别是什么？

**答题要点**：
1. 容器是"包装盒"，定义文件格式（MP4/MKV/FLV），负责打包多路码流和元信息
2. 编解码器是"算法"，定义压缩方式（H.264/AAC），负责将原始数据压缩为码流
3. 同一个码流可以放在不同容器中（H.264 可以放在 MP4、MKV、FLV 中）
4. 容器格式决定了文件的兼容性和功能特性

### Q2：什么是 extradata？为什么需要它？

**答题要点**：
1. Extradata 是存储在容器中的编解码器初始化数据
2. H.264 的 extradata 包含 SPS（序列参数集）和 PPS（图像参数集）
3. 解码器需要 extradata 来初始化：分辨率、帧率、profile、level 等信息
4. 没有 extradata，解码器无法正确解码码流

### Q3：转封装和转码的区别？各自的应用场景？

**答题要点**：
- **转封装**：只更换容器，不改变码流，速度快（毫秒级），用于格式兼容性问题
- **转码**：重新编码码流，可改变编码参数，速度慢（秒到分钟级），用于压缩、分辨率转换
- 应用场景：MKV→MP4 用转封装；4K→1080p 用转码

---

## 延伸阅读

1. [FFmpeg 官方文档 - Formats](https://ffmpeg.org/ffmpeg-formats.html)
2. [MP4 文件格式详解](https://wiki.multimedia.cx/index.php/MP4)
3. [MKV 文件格式规范](https://www.matroska.org/technical/elements.html)
4. [FFmpeg Wiki - Muxing](https://trac.ffmpeg.org/wiki/How%20to%20use%20the%20FFmpeg%20documentation)

---

### 下节课预告

第5节：FFmpeg CLI 与媒体排查基础
- 学习 `ffmpeg`、`ffprobe`、`ffplay` 的职责划分
- 掌握抽帧、抽音频、查看关键帧、查看像素格式等常用操作
- 理解 seek 行为和关键帧约束
