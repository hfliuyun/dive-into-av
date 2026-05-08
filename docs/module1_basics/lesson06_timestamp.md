# 第6节：帧率、码率与时间戳

| 字段 | 内容 |
|------|------|
| lesson_id | 06 |
| module | module1_basics |
| type | python_visualization |
| prerequisites | 第1-5节 |
| outputs | 理解帧率、码率、时间戳的概念，能分析 PTS/DTS 和 GOP 结构 |
| estimated_time | 2-3 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.mp4, test_bframes.mp4 |
| notebook/source | notebooks/module1/06_timestamp_analysis.ipynb |

## 本节能力目标

- 理解帧率（FPS）的概念和常见帧率标准
- 理解码率（Bitrate）的概念，区分 CBR 和 VBR
- 理解时间戳（PTS/DTS）的含义和作用
- 理解 time_base 的作用和换算方法
- 理解 GOP（Group of Pictures）的结构和 B 帧的影响

## 真实工程对应场景

- 视频播放：播放器根据 PTS 进行音视频同步
- 视频剪辑：精确剪辑需要正确处理时间戳
- 直播推流：码率控制影响画质和带宽
- 视频转码：需要正确设置帧率和码率
- 问题排查：播放卡顿、音画不同步往往与时间戳有关

## 引言

在前几节课中，我们学习了容器、码流、编解码器的概念，以及 FFmpeg 工具的使用。但你是否想过：播放器是如何知道什么时候显示哪一帧？为什么有时候视频会卡顿或音画不同步？

本节课我们将深入学习**帧率、码率和时间戳**这三个核心概念。时间戳是音视频同步的基础，理解它将帮助你排查播放问题、理解视频剪辑、掌握直播推流。

---

## 理论讲解

### 核心概念1：帧率（FPS）

**帧率（Frames Per Second）** 是每秒显示的帧数，单位是 fps。

#### 1.1 常见帧率标准

| 帧率 | 应用场景 | 特点 |
|------|----------|------|
| 24fps | 电影 | 电影感，适合大屏幕 |
| 25fps | PAL 制式（中国、欧洲） | 广播电视标准 |
| 30fps | NTSC 制式（美国、日本） | 广播电视标准 |
| 60fps | 高帧率视频、游戏 | 流畅，适合快速运动 |
| 120fps | 慢动作视频 | 可放慢到 1/4 速度（30fps 播放） |

#### 1.2 帧率对文件大小的影响

帧率越高，每秒的帧数越多，文件越大：

```
文件大小 ≈ 码率 × 时长
码率 ≈ 单帧大小 × 帧率
```

**示例**：同样 5 秒的视频
- 30fps：150 帧
- 60fps：300 帧（文件大约是 30fps 的 2 倍）

---

### 核心概念2：码率（Bitrate）

**码率** 是每秒传输的数据量，单位是 kbps（千比特/秒）或 Mbps（兆比特/秒）。

#### 2.1 码率与画质的关系

码率越高，画质越好，但文件也越大：

| 码率 | 画质 | 应用场景 |
|------|------|----------|
| 500 kbps | 低清 | 移动网络、低带宽 |
| 2 Mbps | 标清 | 网络视频 |
| 5 Mbps | 高清 | 在线视频 |
| 20 Mbps | 蓝光 | 蓝光光盘 |
| 50 Mbps | 4K | 4K 流媒体 |

#### 2.2 CBR vs VBR

| 模式 | 全称 | 特点 | 应用场景 |
|------|------|------|----------|
| CBR | Constant Bitrate | 恒定码率，每秒数据量相同 | 直播推流、实时通信 |
| VBR | Variable Bitrate | 可变码率，根据画面复杂度调整 | 点播视频、存储 |

**CBR 的优缺点**：
- 优点：带宽可预测，适合实时传输
- 缺点：复杂画面可能画质差，简单画面浪费带宽

**VBR 的优缺点**：
- 优点：画质更均匀，文件更小
- 缺点：带宽不可预测，不适合实时传输

---

### 核心概念3：时间戳（PTS/DTS）

时间戳是音视频同步的核心。

#### 3.1 PTS vs DTS

| 时间戳 | 全称 | 含义 | 作用 |
|--------|------|------|------|
| PTS | Presentation Time Stamp | 显示时间戳 | 决定什么时候显示这帧 |
| DTS | Decoding Time Stamp | 解码时间戳 | 决定什么时候解码这帧 |

#### 3.2 为什么 PTS 和 DTS 可能不同？

当视频包含 **B 帧** 时，PTS 和 DTS 会不同：

```
解码顺序：I P B B P B B I
显示顺序：I B B P B B P I
```

**示例**：
- 帧类型：I P B B P
- DTS：0 1 2 3 4（按解码顺序）
- PTS：0 3 1 2 4（按显示顺序）

B 帧需要参考后面的 P 帧，所以必须先解码 P 帧，才能解码 B 帧。

#### 3.3 time_base

**time_base** 是时间基准，将整数时间戳转换为秒：

```
时间(秒) = PTS × time_base
```

**常见 time_base**：
- 视频：1/fps（如 1/30）
- 音频：1/sample_rate（如 1/44100）

**示例**：
- PTS = 90，time_base = 1/30
- 时间 = 90 × (1/30) = 3.0 秒

---

### 核心概念4：GOP（Group of Pictures）

**GOP** 是两个关键帧（I帧）之间的帧序列。

#### 4.1 GOP 结构

```
I B B P B B P B B I B B P B B P B B I
|_______ GOP 1 _______|_______ GOP 2 _______|
```

- **I帧**：关键帧，完整的图像，可以独立解码
- **P帧**：预测帧，参考前面的帧，只存储差异
- **B帧**：双向预测帧，参考前后两个方向，压缩率最高

#### 4.2 GOP 大小的影响

| GOP 大小 | 优点 | 缺点 |
|----------|------|------|
| 小 GOP | seek 精度高，随机访问快 | 文件更大 |
| 大 GOP | 文件更小，压缩率高 | seek 精度低 |

**常见 GOP 设置**：
- 直播：小 GOP（1-2 秒），方便快速切换
- 点播：大 GOP（2-5 秒），节省带宽

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **帧率决定流畅度** | 帧率越高越流畅，但文件也越大 |
| **码率决定画质** | 码率越高画质越好，但带宽需求也越大 |
| **PTS 决定显示时间** | PTS 告诉播放器什么时候显示这帧 |
| **DTS 决定解码时间** | DTS 告诉播放器什么时候解码这帧 |
| **B 帧导致 PTS ≠ DTS** | 有 B 帧时，解码顺序和显示顺序不同 |
| **time_base 是换算桥梁** | 时间(秒) = PTS × time_base |

---

## 环境与素材准备

### 环境要求

```bash
# 确保 ffmpeg、ffprobe 已安装
ffmpeg -version
ffprobe -version

# Python 依赖
pip install jupyter notebook
```

### 测试素材

```bash
# 生成带 B 帧的测试视频
ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
       -f lavfi -i sine=frequency=440:duration=5 \
       -c:v libx264 -bf 2 -g 30 -c:a aac \
       -shortest -y test_bframes.mp4
```

---

## 核心代码实验

### 实验1：提取并分析时间戳

**目标**：理解 PTS 和 DTS 的含义

**代码实现：**
```python
import subprocess
import json

def get_frame_timestamps(file_path):
    """获取每一帧的时间戳信息"""
    cmd = [
        'ffprobe',
        '-v', 'quiet',
        '-select_streams', 'v:0',
        '-show_entries', 'frame=pts,pts_time,dts,dts_time,pict_type',
        '-of', 'json',
        file_path
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode != 0:
            print(f"ffprobe 失败: {result.stderr[:200]}")
            return None
        return json.loads(result.stdout)
    except subprocess.TimeoutExpired:
        print("ffprobe 超时")
        return None

# 分析带 B 帧的视频
print("=" * 50)
print("时间戳分析")
print("=" * 50)

info = get_frame_timestamps('test_bframes.mp4')
if info and 'frames' in info:
    frames = info['frames']
    
    print(f"\n总帧数: {len(frames)}")
    print(f"\n前10帧的时间戳:")
    print(f"{'帧类型':<8} {'PTS':<10} {'PTS时间':<12} {'DTS':<10} {'DTS时间':<12}")
    print("-" * 52)
    
    for i, frame in enumerate(frames[:10]):
        pict_type = frame.get('pict_type', 'N/A')
        pts = frame.get('pts', 'N/A')
        pts_time = frame.get('pts_time', 'N/A')
        dts = frame.get('dts', 'N/A')
        dts_time = frame.get('dts_time', 'N/A')
        
        # 格式化时间
        if pts_time != 'N/A':
            pts_time = f"{float(pts_time):.6f}"
        if dts_time != 'N/A':
            dts_time = f"{float(dts_time):.6f}"
        
        print(f"{pict_type:<8} {pts:<10} {pts_time:<12} {dts:<10} {dts_time:<12}")
```

**代码解析：**
- `pts`：显示时间戳（整数）
- `pts_time`：显示时间戳（秒）
- `dts`：解码时间戳（整数）
- `dts_time`：解码时间戳（秒）
- `pict_type`：帧类型（I/P/B）

---

### 实验2：分析 GOP 结构

**目标**：理解 GOP 的组成

**代码实现：**
```python
def analyze_gop(file_path):
    """分析 GOP 结构"""
    info = get_frame_timestamps(file_path)
    if not info or 'frames' not in info:
        return None
    
    frames = info['frames']
    gops = []
    current_gop = []
    
    for frame in frames:
        current_gop.append(frame)
        if frame.get('pict_type') == 'I' and len(current_gop) > 1:
            # 遇到新的 I 帧，保存当前 GOP
            gops.append(current_gop[:-1])
            current_gop = [frame]
    
    # 保存最后一个 GOP
    if current_gop:
        gops.append(current_gop)
    
    return gops

# 分析 GOP 结构
print("=" * 50)
print("GOP 结构分析")
print("=" * 50)

gops = analyze_gop('test_bframes.mp4')
if gops:
    print(f"\nGOP 数量: {len(gops)}")
    
    for i, gop in enumerate(gops[:3]):  # 只显示前3个 GOP
        print(f"\nGOP {i + 1}:")
        print(f"  帧数: {len(gop)}")
        print(f"  帧类型: ", end="")
        for frame in gop:
            print(frame.get('pict_type', '?'), end=" ")
        print()
        
        # 显示时间范围
        if gop:
            start_time = gop[0].get('pts_time', 'N/A')
            end_time = gop[-1].get('pts_time', 'N/A')
            print(f"  时间范围: {start_time}s - {end_time}s")
```

**代码解析：**
- 通过遍历帧，识别 I 帧作为 GOP 的分界点
- 统计每个 GOP 的帧数和帧类型
- 显示 GOP 的时间范围

---

### 实验3：码率分析

**目标**：理解码率与画质的关系

**代码实现：**
```python
def get_bitrate_info(file_path):
    """获取码率信息"""
    cmd = [
        'ffprobe',
        '-v', 'quiet',
        '-print_format', 'json',
        '-show_format',
        '-show_streams',
        file_path
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        if result.returncode != 0:
            print(f"ffprobe 失败: {result.stderr[:200]}")
            return None
        return json.loads(result.stdout)
    except subprocess.TimeoutExpired:
        print("ffprobe 超时")
        return None

# 分析码率信息
print("=" * 50)
print("码率分析")
print("=" * 50)

info = get_bitrate_info('test_bframes.mp4')
if info:
    # 格式信息
    fmt = info['format']
    print(f"\n容器格式: {fmt['format_name']}")
    print(f"时长: {float(fmt['duration']):.2f} 秒")
    print(f"总码率: {int(fmt['bit_rate']) / 1000:.0f} kbps")
    
    # 流信息
    for stream in info['streams']:
        if stream['codec_type'] == 'video':
            print(f"\n视频流:")
            print(f"  编码: {stream['codec_name']}")
            print(f"  分辨率: {stream['width']}x{stream['height']}")
            print(f"  帧率: {stream['r_frame_rate']}")
            print(f"  码率: {int(stream.get('bit_rate', 0)) / 1000:.0f} kbps")
        elif stream['codec_type'] == 'audio':
            print(f"\n音频流:")
            print(f"  编码: {stream['codec_name']}")
            print(f"  采样率: {stream['sample_rate']} Hz")
            print(f"  码率: {int(stream.get('bit_rate', 0)) / 1000:.0f} kbps")
```

**代码解析：**
- 获取容器级别的码率信息
- 获取每个流的码率信息
- 区分视频码率和音频码率

---

## 预期结果与验收标准

### 预期输出

```text
注：以下为示意输出，实际结果可能因视频内容和编码参数略有差异。

==================================================
时间戳分析
==================================================

总帧数: 150

前10帧的时间戳:
帧类型   PTS        PTS时间      DTS        DTS时间    
----------------------------------------------------
I        0          0.000000     0          0.000000   
P        3          0.100000     1          0.033333   
B        1          0.033333     2          0.066667   
B        2          0.066667     3          0.100000   
P        6          0.200000     4          0.133333   
B        4          0.133333     5          0.166667   
B        5          0.166667     6          0.200000   
P        9          0.300000     7          0.233333   
B        7          0.233333     8          0.266667   
B        8          0.266667     9          0.300000   

==================================================
GOP 结构分析
==================================================

GOP 数量: 6

GOP 1:
  帧数: 30
  帧类型: I P B B P B B P B B P B B P B B P B B P B B P B B P B B P B B P 
  时间范围: 0.0s - 0.966667s

==================================================
码率分析
==================================================

容器格式: mov,mp4,m4a,3gp,3g2,mj2
时长: 5.00 秒
总码率: 320 kbps

视频流:
  编码: h264
  分辨率: 1280x720
  帧率: 30/1
  码率: 280 kbps

音频流:
  编码: aac
  采样率: 44100 Hz
  码率: 40 kbps
```

### 验收标准

- [ ] 能区分 PTS 和 DTS 的含义
- [ ] 能解释 time_base 的作用
- [ ] 能分析 GOP 结构和帧类型
- [ ] 能理解 B 帧对解码顺序和显示顺序的影响
- [ ] 能解释码率与画质的关系

---

## 常见错误与排查

### 错误1：PTS 和 DTS 完全相同

**原因**：视频没有 B 帧，解码顺序和显示顺序一致。

**排查**：
- 用 `ffprobe` 检查帧类型，确认是否有 B 帧
- 生成带 B 帧的测试视频：`-bf 2`

### 错误2：时间戳不连续

**原因**：视频有丢帧或编辑过。

**排查**：
- 用 `ffprobe` 检查时间戳是否有跳跃
- 检查视频是否经过剪辑

### 错误3：码率信息缺失

**原因**：某些容器格式不存储码率信息。

**排查**：
- 使用 MP4 或 MKV 容器
- 手动计算码率：码率 = 文件大小 / 时长

---

## 课后小挑战

### 基础题：计算 GOP 大小

编写 Python 脚本，计算视频的平均 GOP 大小（两个 I 帧之间的平均帧数）。

### 进阶题：分析 PTS 和 DTS 的差异

编写 Python 脚本，统计 PTS 和 DTS 不一致的帧数，分析 B 帧的比例。

### 思考题1：为什么直播推流使用小 GOP？

提示：考虑观众切换频道时的首帧等待时间。

### 思考题2：为什么 B 帧会导致延迟？

提示：考虑 B 帧需要参考后面的帧，必须缓存后续帧才能解码。

---

## 面试延伸题

### Q1：PTS 和 DTS 的区别是什么？

**答题要点**：
1. PTS 是显示时间戳，决定什么时候显示这帧
2. DTS 是解码时间戳，决定什么时候解码这帧
3. 没有 B 帧时，PTS 和 DTS 相同
4. 有 B 帧时，PTS 和 DTS 可能不同，因为解码顺序和显示顺序不同

### Q2：什么是 GOP？为什么需要 GOP？

**答题要点**：
1. GOP 是两个关键帧之间的帧序列
2. GOP 的存在是因为 P 帧和 B 帧需要参考其他帧
3. GOP 越大，压缩率越高，但 seek 精度越低
4. 直播使用小 GOP，点播使用大 GOP

### Q3：如何选择合适的码率？

**答题要点**：
1. 码率取决于分辨率、帧率、内容复杂度
2. 直播使用 CBR，点播使用 VBR
3. 码率太高浪费带宽，太低影响画质
4. 参考行业标准：720p 约 2-4 Mbps，1080p 约 4-8 Mbps

---

## 延伸阅读

1. [FFmpeg Wiki - Rate Control](https://trac.ffmpeg.org/wiki/Encode/H.264#RateControl)
2. [FFmpeg Wiki - GOP](https://trac.ffmpeg.org/wiki/StreamingGuide#GOP)
3. [PTS/DTS 详解](https://www.framebyframe.info/pts-dts-explained)

---

### 下节课预告

第7节：视频编码原理（DCT + 量化）
- 学习 DCT 变换和量化的原理
- 理解视频压缩的基本方法
- 手写简化版 DCT，观察压缩前后的图像变化
