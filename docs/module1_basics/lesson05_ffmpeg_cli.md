# 第5节：FFmpeg CLI 与媒体排查基础

| 字段            | 内容                                                        |
| --------------- | ----------------------------------------------------------- |
| lesson_id       | 05                                                          |
| module          | module1_basics                                              |
| type            | python_visualization                                        |
| prerequisites   | 第1-4节                                                     |
| outputs         | 掌握 ffmpeg/ffprobe/ffplay 常用命令，能独立排查媒体文件问题 |
| estimated_time  | 2-3 小时                                                    |
| runtime_limit   | 实验运行 < 3 秒                                             |
| assets          | test.mp4, test_keyframes.mp4                                |
| notebook/source | notebooks/module1/05_ffmpeg_cli.ipynb                       |

## 本节能力目标

- 理解 `ffmpeg`、`ffprobe`、`ffplay` 三个工具的职责划分
- 掌握 `ffmpeg` 的常用操作模式：转码、转封装、抽帧、抽音频
- 掌握 `ffprobe` 的高级用法：流过滤、格式化输出、关键帧分析
- 能用 `ffplay` 快速验证媒体文件和调试滤镜
- 理解关键帧（I帧）对 seek 行为的影响

## 真实工程对应场景

- 媒体文件排查：收到一个视频文件，快速了解编码、分辨率、帧率、时长等信息
- 问题定位：视频播放卡顿，需要分析关键帧分布和 GOP 结构
- 素材提取：从视频中提取特定时间点的截图或音频
- 格式验证：确认转码后的文件是否符合预期
- 快速验证：用 ffplay 播放文件，确认音视频是否正常

## 引言

在第4节中，我们学习了容器、码流、编解码器的概念，并用 `ffprobe` 分析了媒体文件的基本信息。但实际工作中，我们不仅需要"看"文件信息，还需要"操作"媒体文件——提取帧、抽取音频、测试 seek 行为、验证转码结果。

本节课我们将深入学习 FFmpeg 工具家族的三个核心命令：`ffmpeg`（处理）、`ffprobe`（探测）、`ffplay`（播放）。这些是音视频工程师的"瑞士军刀"，掌握它们将让你在排查问题时游刃有余。

---

## 理论讲解

### 核心概念1：FFmpeg 工具家族

FFmpeg 不只是一个命令，而是一个**工具家族**：

| 工具        | 职责                                        | 典型场景                        |
| ----------- | ------------------------------------------- | ------------------------------- |
| `ffmpeg`  | 音视频处理（转码、转封装、滤镜、抽帧...）   | 格式转换、提取帧/音频、添加水印 |
| `ffprobe` | 媒体文件探测（容器信息、流信息、时间戳...） | 分析文件结构、排查编码问题      |
| `ffplay`  | 轻量播放器（快速验证、调试滤镜）            | 播放文件、测试滤镜效果          |

#### 1.1 ffmpeg：音视频处理瑞士军刀

`ffmpeg` 的基本语法：

```bash
ffmpeg [全局选项] [输入选项] -i input.mp4 [输出选项] output.mp4
```

**核心参数分类：**

| 参数类型 | 示例                    | 作用                         |
| -------- | ----------------------- | ---------------------------- |
| 输入选项 | `-ss 10`              | 在输入文件上 seek 到 10 秒   |
| 输出选项 | `-t 5`                | 只输出 5 秒                  |
| 编码控制 | `-c:v copy`           | 视频流直接复制（不重新编码） |
| 编码控制 | `-c:v libx264`        | 使用 H.264 编码器            |
| 流选择   | `-map 0:v:0`          | 选择第一个输入的第一个视频流 |
| 滤镜     | `-vf "scale=640:360"` | 视频缩放滤镜                 |

#### 1.2 ffprobe：媒体文件探测器

`ffprobe` 的核心能力：

```bash
# 基本信息
ffprobe input.mp4

# JSON 格式输出
ffprobe -v quiet -print_format json -show_format -show_streams input.mp4

# 只显示特定信息
ffprobe -v quiet -show_entries format=duration,size input.mp4

# 显示帧信息
ffprobe -v quiet -show_frames input.mp4
```

#### 1.3 ffplay：轻量播放器

`ffplay` 的核心能力：

```bash
# 播放文件
ffplay input.mp4

# 播放裸流（需要指定参数）
ffplay -video_size 1280x720 -pixel_format yuv420p input.yuv

# 播放音频
ffplay input.pcm -f s16le -channels 2 -sample_rate 44100

# 测试滤镜
ffplay -vf "scale=640:360" input.mp4
```

---

### 核心概念2：ffmpeg 常用操作模式

#### 2.1 转封装（Remux）

只更换容器，不改变码流：

```bash
# MKV → MP4
ffmpeg -i input.mkv -c copy output.mp4

# 提取音频流
ffmpeg -i input.mp4 -c:a copy -vn output.aac

# 提取视频流
ffmpeg -i input.mp4 -c:v copy -an output.h264
```

#### 2.2 转码（Transcode）

重新编码码流：

```bash
# 转换编码
ffmpeg -i input.mp4 -c:v libx264 -c:a aac output.mp4

# 改变分辨率
ffmpeg -i input.mp4 -vf "scale=1280:720" output.mp4

# 改变帧率
ffmpeg -i input.mp4 -r 25 output.mp4
```

#### 2.3 抽帧（Extract Frames）

从视频中提取帧：

```bash
# 提取所有帧
ffmpeg -i input.mp4 frames/%04d.png

# 提取关键帧（I帧）
ffmpeg -i input.mp4 -vf "select=eq(pict_type\,I)" -vsync vfr iframe_%03d.png

# 提取指定时间点的帧
ffmpeg -i input.mp4 -ss 00:00:02 -vframes 1 frame.png
```

#### 2.4 抽音频（Extract Audio）

从视频中提取音频：

```bash
# 提取音频为 WAV
ffmpeg -i input.mp4 -vn -c:a pcm_s16le output.wav

# 提取音频为 MP3
ffmpeg -i input.mp4 -vn -c:a libmp3lame output.mp3
```

---

### 核心概念3：媒体排查核心技能

#### 3.1 关键帧与 GOP

**GOP（Group of Pictures）** 是视频中两个关键帧之间的帧序列：

```
I P B B P B B   I P B B P B B   I ...
|___ GOP 1 ___| |___ GOP 2 ___| |__ GOP 3 ...
```

- **I帧（关键帧）**：完整的图像，可以独立解码
- **P帧**：参考前面的帧，只存储差异
- **B帧**：参考前后两个方向的帧，压缩率最高

**关键帧对 seek 的影响：**

- 输入 seek（`-ss` 放在 `-i` 前）：**快速但不精确**，直接跳到最近关键帧，适合快速预览
- 输出 seek（`-ss` 放在 `-i` 后）：**慢速但精确**，从头解码到目标帧，适合精确截取

#### 3.2 像素格式

常见像素格式：

| 格式    | 说明               | 应用             |
| ------- | ------------------ | ---------------- |
| yuv420p | YUV 4:2:0 平面存储 | 视频编码标准输入 |
| yuv422p | YUV 4:2:2 平面存储 | 专业视频         |
| rgb24   | RGB 24位           | 图像处理         |
| bgr24   | BGR 24位           | OpenCV 默认      |

#### 3.3 时间戳与 time_base

时间戳是音视频同步的基础：

- **PTS（Presentation Time Stamp）**：显示时间戳
- **DTS（Decoding Time Stamp）**：解码时间戳
- **time_base**：时间基准，将整数时间戳转换为秒

```bash
# 查看时间戳信息
ffprobe -v quiet -show_entries frame=pts,pts_time input.mp4
```

---

## 关键点总结

| 要点                           | 一句话总结                                  |
| ------------------------------ | ------------------------------------------- |
| **ffmpeg 是处理工具**    | 转码、转封装、抽帧、抽音频都用它            |
| **ffprobe 是探测工具**   | 分析文件结构、查看流信息、排查问题          |
| **ffplay 是播放工具**    | 快速验证文件、测试滤镜效果                  |
| **关键帧决定 seek 精度** | seek 只能精确到关键帧，GOP 越大 seek 越不准 |
| **-c copy 是最快的操作** | 转封装比转码快 10-100 倍                    |

---

## 环境与素材准备

### 环境要求

```bash
# 确保 ffmpeg、ffprobe、ffplay 已安装
ffmpeg -version
ffprobe -version
ffplay -version

# Python 依赖
pip install jupyter notebook
```

### 测试素材

```bash
# 复用第4节的测试素材
cp ../module1_basics/test.mp4 .

# 生成带明显 GOP 结构的测试视频（每秒1个关键帧）
ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
       -f lavfi -i sine=frequency=440:duration=5 \
       -c:v libx264 -g 30 -keyint_min 30 -c:a aac \
       -shortest -y test_keyframes.mp4
```

---

## 核心代码实验

### 实验1：ffprobe 深度解析

**目标**：掌握 ffprobe 的各种输出格式和过滤器

**代码实现：**

```python
import subprocess
import json

def probe_detailed(file_path):
    """使用 ffprobe 获取详细的媒体信息"""
    cmd = [
        'ffprobe',
        '-v', 'quiet',
        '-print_format', 'json',
        '-show_format',
        '-show_streams',
        '-show_frames',
        '-select_streams', 'v:0',  # 只选择第一个视频流
        file_path
    ]
  
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"ffprobe 失败: {result.stderr[:200]}")
        return None
  
    return json.loads(result.stdout)

# 分析测试文件
print("=== ffprobe 深度解析 ===\n")

info = probe_detailed('test_keyframes.mp4')
if info:
    # 显示格式信息
    fmt = info['format']
    print(f"容器格式: {fmt['format_name']}")
    print(f"时长: {float(fmt['duration']):.2f} 秒")
    print(f"总码率: {int(fmt['bit_rate']) / 1000:.0f} kbps")
  
    # 显示视频流信息
    for stream in info['streams']:
        if stream['codec_type'] == 'video':
            print(f"\n视频流:")
            print(f"  编码: {stream['codec_name']}")
            print(f"  分辨率: {stream['width']}x{stream['height']}")
            print(f"  帧率: {stream['r_frame_rate']}")
            print(f"  像素格式: {stream.get('pix_fmt', 'N/A')}")
            print(f"  关键帧间隔: {stream.get('gop_size', 'N/A')}")
  
    # 分析帧信息
    if 'frames' in info:
        frames = info['frames']
        i_frames = [f for f in frames if f.get('pict_type') == 'I']
        print(f"\n帧统计:")
        print(f"  总帧数: {len(frames)}")
        print(f"  关键帧数: {len(i_frames)}")
        print(f"  GOP 数量: {len(i_frames)}")
```

**代码解析：**

- `-show_frames`：显示每一帧的详细信息
- `-select_streams v:0`：只选择第一个视频流
- `pict_type`：帧类型（I/P/B）

---

### 实验2：ffmpeg 抽帧与抽音频

**目标**：掌握从视频中提取帧和音频的方法

**代码实现：**

```python
import os
import time

def extract_frames(input_file, output_dir, frame_type='all'):
    """从视频中提取帧"""
    os.makedirs(output_dir, exist_ok=True)
  
    if frame_type == 'keyframe':
        # 只提取关键帧
        cmd = [
            'ffmpeg', '-i', input_file,
            '-vf', 'select=eq(pict_type\\,I)',
            '-vsync', 'vfr',
            '-y', f'{output_dir}/frame_%03d.png'
        ]
    else:
        # 提取所有帧
        cmd = [
            'ffmpeg', '-i', input_file,
            '-y', f'{output_dir}/frame_%04d.png'
        ]
  
    start = time.time()
    result = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.time() - start
  
    if result.returncode != 0:
        print(f"提取帧失败: {result.stderr[:200]}")
        return 0, elapsed
  
    # 统计提取的帧数
    frame_count = len([f for f in os.listdir(output_dir) if f.endswith('.png')])
    return frame_count, elapsed

def extract_audio(input_file, output_file):
    """从视频中提取音频"""
    cmd = [
        'ffmpeg', '-i', input_file,
        '-vn',  # 不包含视频
        '-c:a', 'pcm_s16le',  # PCM 16位
        '-y', output_file
    ]
  
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"提取音频失败: {result.stderr[:200]}")
        return False
    return True

# 实验：提取关键帧
print("=== 实验：提取关键帧 ===\n")

frame_count, elapsed = extract_frames('test_keyframes.mp4', 'keyframes', 'keyframe')
print(f"提取关键帧: {frame_count} 帧，耗时 {elapsed:.3f} 秒")

# 实验：提取所有帧
print("\n=== 实验：提取所有帧 ===\n")

frame_count, elapsed = extract_frames('test_keyframes.mp4', 'all_frames', 'all')
print(f"提取所有帧: {frame_count} 帧，耗时 {elapsed:.3f} 秒")

# 实验：提取音频
print("\n=== 实验：提取音频 ===\n")

if extract_audio('test_keyframes.mp4', 'output.wav'):
    size = os.path.getsize('output.wav') / 1024
    print(f"提取音频: {size:.1f} KB")
```

**代码解析：**

- `select=eq(pict_type\\,I)`：只选择 I 帧（关键帧）
- `-vsync vfr`：可变帧率输出，避免重复帧
- `-vn`：不包含视频流
- `-c:a pcm_s16le`：PCM 16位小端编码

---

### 实验3：关键帧与 seek 行为分析

**目标**：理解关键帧对 seek 的影响

**代码实现：**

```python
def test_seek(input_file, seek_time, mode='input'):
    """测试 seek 行为"""
    output_file = f'seek_{mode}_{seek_time}s.png'
  
    if mode == 'input':
        # 精确 seek（-ss 放在 -i 前）
        cmd = [
            'ffmpeg',
            '-ss', str(seek_time),
            '-i', input_file,
            '-vframes', '1',
            '-y', output_file
        ]
    else:
        # 近似 seek（-ss 放在 -i 后）
        cmd = [
            'ffmpeg',
            '-i', input_file,
            '-ss', str(seek_time),
            '-vframes', '1',
            '-y', output_file
        ]
  
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Seek 失败: {result.stderr[:200]}")
        return None
  
    return output_file

def get_frame_pts(file_path):
    """获取帧的时间戳"""
    cmd = [
        'ffprobe',
        '-v', 'quiet',
        '-select_streams', 'v:0',
        '-show_entries', 'frame=pts_time,pict_type',
        '-of', 'json',
        file_path
    ]
  
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return None
  
    return json.loads(result.stdout)

# 实验：分析关键帧位置
print("=== 实验：分析关键帧位置 ===\n")

frames_info = get_frame_pts('test_keyframes.mp4')
if frames_info and 'frames' in frames_info:
    frames = frames_info['frames']
    i_frames = [f for f in frames if f.get('pict_type') == 'I']
  
    print(f"总帧数: {len(frames)}")
    print(f"关键帧时间点:")
    for f in i_frames[:5]:  # 只显示前5个
        print(f"  I帧: {float(f['pts_time']):.3f} 秒")

# 实验：测试 seek 行为
print("\n=== 实验：测试 seek 行为 ===\n")

# 测试 seek 到 2.5 秒（非关键帧时间点）
seek_time = 2.5

# 精确 seek
output1 = test_seek('test_keyframes.mp4', seek_time, 'input')
print(f"精确 seek 到 {seek_time} 秒: {output1}")

# 近似 seek
output2 = test_seek('test_keyframes.mp4', seek_time, 'output')
print(f"近似 seek 到 {seek_time} 秒: {output2}")

# 比较两种 seek 的结果
if output1 and output2:
    size1 = os.path.getsize(output1) / 1024
    size2 = os.path.getsize(output2) / 1024
    print(f"\n文件大小对比:")
    print(f"  精确 seek: {size1:.1f} KB")
    print(f"  近似 seek: {size2:.1f} KB")
```

**代码解析：**

- `-ss` 放在 `-i` 前：输入 seek，更精确但可能慢
- `-ss` 放在 `-i` 后：输出 seek，更快但可能不精确
- `pict_type`：帧类型，I 表示关键帧

---

## 预期结果与验收标准

### 预期输出

```text
=== ffprobe 深度解析 ===

容器格式: mov,mp4,m4a,3gp,3g2,mj2
时长: 5.00 秒
总码率: 320 kbps

视频流:
  编码: h264
  分辨率: 1280x720
  帧率: 30/1
  像素格式: yuv420p
  关键帧间隔: 30

帧统计:
  总帧数: 150
  关键帧数: 6
  GOP 数量: 6

=== 实验：提取关键帧 ===

提取关键帧: 6 帧，耗时 0.856 秒

=== 实验：提取所有帧 ===

提取所有帧: 150 帧，耗时 2.345 秒

=== 实验：提取音频 ===

提取音频: 441.2 KB

=== 实验：分析关键帧位置 ===

总帧数: 150
关键帧时间点:
  I帧: 0.000 秒
  I帧: 1.000 秒
  I帧: 2.000 秒
  I帧: 3.000 秒
  I帧: 4.000 秒

=== 实验：测试 seek 行为 ===

精确 seek 到 2.5 秒: seek_input_2.5s.png
近似 seek 到 2.5 秒: seek_output_2.5s.png

文件大小对比:
  精确 seek: 123.4 KB
  近似 seek: 125.6 KB
```

### 验收标准

- [ ] 能区分 ffmpeg、ffprobe、ffplay 的使用场景
- [ ] 能用 ffmpeg 从视频中提取关键帧和所有帧
- [ ] 能用 ffmpeg 从视频中提取音频
- [ ] 能用 ffprobe 分析关键帧位置和 GOP 结构
- [ ] 能解释精确 seek 和近似 seek 的区别

---

## 常见错误与排查

### 错误1：ffmpeg 报错 "No such file or directory"

**原因**：输入文件路径错误或文件不存在。

**排查**：

- 检查文件路径是否正确
- 使用绝对路径或确保相对路径正确
- 用 `ls -la` 确认文件存在

### 错误2：提取的帧数为 0

**原因**：视频文件损坏或编码格式不支持。

**排查**：

- 用 `ffprobe` 确认文件格式正确
- 检查视频编码是否支持
- 尝试用其他视频文件测试

### 错误3：seek 结果不准确

**原因**：seek 只能精确到关键帧，如果目标时间点没有关键帧，会 seek 到最近的关键帧。

**排查**：

- 用 `ffprobe` 分析关键帧位置
- 确认 GOP 大小（`-g` 参数）
- 理解精确 seek 和近似 seek 的区别

### 错误4：ffplay 无法播放裸流

**原因**：裸流没有容器头信息，需要手动指定参数。

**排查**：

- 指定分辨率：`-video_size 1280x720`
- 指定像素格式：`-pixel_format yuv420p`
- 指定帧率：`-framerate 30`

---

## 课后小挑战

### 基础题：提取视频缩略图

从视频中每隔 1 秒提取一帧，生成缩略图序列：

```bash
ffmpeg -i input.mp4 -vf "fps=1" thumb_%02d.png
```

### 进阶题：分析 GOP 结构

编写 Python 脚本，分析视频的 GOP 结构，输出每个 GOP 的帧数和时长。

### 思考题1：为什么 GOP 越大，seek 越不准？

提示：考虑 seek 的实现原理——seek 只能到关键帧，然后解码到目标帧。

### 思考题2：精确 seek 和近似 seek 的速度差异？

提示：精确 seek 需要解码从关键帧到目标帧的所有帧，近似 seek 直接跳到关键帧。

---

## 面试延伸题

### Q1：ffmpeg 的 -c copy 和 -c:v libx264 有什么区别？

**答题要点**：

1. `-c copy`：转封装模式，直接复制码流，速度极快（毫秒级）
2. `-c:v libx264`：转码模式，重新编码，速度慢（秒到分钟级）
3. 应用场景：格式兼容用转封装，压缩/改变参数用转码

### Q2：如何从视频中提取关键帧？

**答题要点**：

1. 使用 `-vf "select=eq(pict_type\\,I)"` 过滤器
2. 使用 `-vsync vfr` 避免重复帧
3. 关键帧是完整的图像，可以独立解码

### Q3：为什么 ffplay 播放 YUV 文件需要指定参数？

**答题要点**：

1. YUV 是裸数据，没有容器头信息
2. 需要指定分辨率（`-video_size`）、像素格式（`-pixel_format`）、帧率（`-framerate`）
3. 参数错误会导致画面错乱或无法播放

---

## 延伸阅读

1. [FFmpeg 官方文档](https://ffmpeg.org/documentation.html)
2. [FFmpeg Wiki](https://trac.ffmpeg.org/)
3. [FFmpeg 命令行教程](https://trac.ffmpeg.org/wiki/How%20to%20use%20the%20FFmpeg%20documentation)

---

### 下节课预告

第6节：帧率、码率与时间戳

- 学习 FPS、GOP、PTS、DTS、CBR、VBR、time_base 的概念
- 提取帧序列并分析时间戳与显示顺序
- 理解 B 帧对解码顺序和显示顺序的影响
