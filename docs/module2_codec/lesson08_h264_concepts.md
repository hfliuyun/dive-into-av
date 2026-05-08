# 第8节：H.264/H.265 关键概念

| 字段 | 内容 |
|------|------|
| lesson_id | 08 |
| module | module2_codec |
| type | python_visualization |
| prerequisites | 第1-7节 |
| outputs | 理解 H.264 的 I/P/B 帧、GOP、NALU、SPS、PPS、IDR 概念 |
| estimated_time | 3-4 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.mp4, test_bframes.mp4 |
| notebook/source | notebooks/module2/08_h264_analysis.ipynb |

## 本节能力目标

- 理解 I/P/B 帧的特点和用途
- 理解 GOP（Group of Pictures）的结构和作用
- 理解 NALU（Network Abstraction Layer Unit）的类型和头部结构
- 理解 SPS/PPS/IDR 的作用
- 能用 ffprobe 分析帧类型和 GOP 结构

## 真实工程对应场景

- 视频播放器：根据帧类型决定解码策略
- 视频剪辑：精确剪辑需要理解 GOP 和关键帧
- 直播推流：GOP 大小影响首帧等待时间
- 视频转码：需要正确设置帧类型和 GOP 参数
- 问题排查：播放卡顿、seek 不准往往与 GOP 和帧类型有关

## 引言

在第7节中，我们学习了 DCT 变换和量化，这是视频编码的核心原理。但你是否想过：H.264 码流是如何组织的？为什么有时候 seek 不准？为什么直播需要小 GOP？

本节课我们将深入学习 **H.264 的关键概念**：I/P/B 帧、GOP、NALU、SPS、PPS、IDR。这是理解 H.264 码流结构的基础，也是排查播放问题的关键。

---

## 理论讲解

### 核心概念1：I/P/B 帧

H.264 使用三种帧类型来压缩视频：

#### 1.1 I帧（关键帧）

**I帧** 是完整的图像，可以独立解码：

- **特点**：不依赖其他帧，可以独立解码
- **用途**：seek 的目标点，随机访问的入口
- **体积**：最大，因为存储了完整的图像信息

#### 1.2 P帧（预测帧）

**P帧** 参考前面的帧，只存储差异：

- **特点**：依赖前面的 I 帧或 P 帧
- **用途**：压缩率比 I 帧高
- **体积**：中等，只存储运动矢量和残差

#### 1.3 B帧（双向预测帧）

**B帧** 参考前后两个方向，压缩率最高：

- **特点**：依赖前面和后面的帧
- **用途**：压缩率最高，但解码复杂度也最高
- **体积**：最小，因为可以参考两个方向

#### 1.4 帧类型对比

| 帧类型 | 依赖关系 | 压缩率 | 解码复杂度 | 体积 |
|--------|----------|--------|------------|------|
| I帧 | 无 | 低 | 低 | 最大 |
| P帧 | 前面的帧 | 中 | 中 | 中等 |
| B帧 | 前后的帧 | 高 | 高 | 最小 |

---

### 核心概念2：GOP（Group of Pictures）

**GOP** 是从一个 I 帧开始，到下一个 I 帧**之前**的所有帧序列（包含起始 I 帧）。

#### 2.1 GOP 结构

```
I P B B P B B P B B | I P B B P B B P B B | I ...
|______ GOP 1 ______| |______ GOP 2 ______|
```

- **GOP 大小**：一个 GOP 中的帧数
- **GOP 结构**：I-P-B 帧的排列方式

#### 2.2 GOP 大小的影响

| GOP 大小 | 优点 | 缺点 | 应用场景 |
|----------|------|------|----------|
| 小 GOP | seek 精度高，首帧快 | 文件更大 | 直播、实时通信 |
| 大 GOP | 文件更小，压缩率高 | seek 精度低 | 点播、存储 |

#### 2.3 Open GOP vs Closed GOP

- **Closed GOP**：GOP 内的帧只参考本 GOP 内的帧
- **Open GOP**：GOP 内的帧可以参考前一个 GOP 的帧

---

### 核心概念3：NALU（Network Abstraction Layer Unit）

**NALU** 是 H.264 码流的基本单位。

#### 3.1 NALU 结构

```
[NALU 头部] [NALU 数据]
```

- **NALU 头部**：1 字节，包含类型和优先级信息
- **NALU 数据**：实际的编码数据

#### 3.2 NALU 头部结构

```
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
```

- **F（1 bit）**：禁止位，通常为 0
- **NRI（2 bits）**：重要性参考，值越大越重要
- **Type（5 bits）**：NALU 类型

#### 3.3 常见 NALU 类型

| Type | 含义 | 说明 |
|------|------|------|
| 1 | 非 IDR 图像 | 普通帧 |
| 5 | IDR 图像 | 关键帧，清空参考帧缓冲区 |
| 6 | SEI | 补充增强信息 |
| 7 | SPS | 序列参数集 |
| 8 | PPS | 图像参数集 |

---

### 核心概念4：SPS/PPS/IDR

#### 4.1 SPS（序列参数集）

**SPS** 包含视频序列的全局参数：

- 分辨率、帧率、Profile、Level
- 参考帧数量、熵编码模式
- 解码器必须先解析 SPS 才能解码视频

#### 4.2 PPS（图像参数集）

**PPS** 包含图像的编码参数：

- 量化参数、去块滤波参数
- 熵编码模式（CABAC/CAVLC）
- PPS 依赖 SPS

#### 4.3 IDR（即时解码刷新）

**IDR** 是特殊的 I 帧：

- 清空参考帧缓冲区
- 后面的帧不能参考 IDR 之前的帧
- seek 必须从 IDR 开始

#### 4.4 SPS/PPS/IDR 的关系

```
SPS → PPS → IDR → P → B → B → P → ...
```

- 解码器必须先收到 SPS 和 PPS 才能解码
- IDR 是随机访问的入口
- SPS/PPS 通常在码流开头或 IDR 前发送

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **I帧是关键帧** | I 帧可以独立解码，是 seek 的目标点 |
| **P帧参考前面** | P 帧依赖前面的帧，压缩率中等 |
| **B帧参考前后** | B 帧压缩率最高，但解码复杂度也最高 |
| **GOP 是帧序列** | GOP 是两个 I 帧之间的帧序列 |
| **NALU 是码流单位** | H.264 码流由 NALU 组成 |
| **SPS/PPS 是参数** | SPS/PPS 是解码器初始化的必要数据 |
| **IDR 清空缓冲区** | IDR 是特殊的 I 帧，清空参考帧缓冲区 |

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

### 实验1：提取关键帧并分析帧类型

**目标**：理解 I/P/B 帧的区别

**代码实现：**
```python
import subprocess
import json

def get_frame_types(file_path, timeout=5):
    """获取每一帧的类型信息"""
    cmd = [
        'ffprobe',
        '-v', 'quiet',
        '-select_streams', 'v:0',
        '-show_entries', 'frame=pict_type,pts_time,key_frame',
        '-read_intervals', '%+5',
        '-of', 'json',
        file_path
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        if result.returncode != 0:
            print(f"ffprobe 失败: {result.stderr[:200]}")
            return None
        return json.loads(result.stdout)
    except FileNotFoundError:
        print("ffprobe 未安装")
        return None
    except subprocess.TimeoutExpired:
        print("ffprobe 超时")
        return None

# 分析帧类型
print("=" * 50)
print("帧类型分析")
print("=" * 50)

info = get_frame_types('test_bframes.mp4')
if info and 'frames' in info:
    frames = info['frames']
    
    # 统计帧类型
    i_count = sum(1 for f in frames if f.get('pict_type') == 'I')
    p_count = sum(1 for f in frames if f.get('pict_type') == 'P')
    b_count = sum(1 for f in frames if f.get('pict_type') == 'B')
    
    print(f"\n总帧数 (前5秒): {len(frames)}")
    print(f"I帧数量: {i_count}")
    print(f"P帧数量: {p_count}")
    print(f"B帧数量: {b_count}")
    
    print(f"\n前10帧的类型:")
    for i, frame in enumerate(frames[:10]):
        pict_type = frame.get('pict_type', 'N/A')
        pts_time = frame.get('pts_time', 'N/A')
        key_frame = frame.get('key_frame', 0)
        
        if pts_time != 'N/A':
            pts_time = f"{float(pts_time):.3f}"
        
        print(f"  帧 {i}: 类型={pict_type}, 时间={pts_time}s, 关键帧={'是' if key_frame else '否'}")
```

**代码解析：**
- `pict_type`：帧类型（I/P/B）
- `key_frame`：是否是关键帧
- `pts_time`：显示时间戳

---

### 实验2：分析 GOP 结构

**目标**：理解 GOP 的组成

**代码实现：**
```python
def analyze_gop(file_path, timeout=30):
    """分析 GOP 结构"""
    info = get_frame_types(file_path, timeout)
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
            if start_time != 'N/A' and end_time != 'N/A':
                print(f"  时间范围: {float(start_time):.3f}s - {float(end_time):.3f}s")
```

**代码解析：**
- 通过遍历帧，识别 I 帧作为 GOP 的分界点
- 统计每个 GOP 的帧数和帧类型
- 显示 GOP 的时间范围

---

### 实验3：提取单帧并对比体积

**目标**：观察不同帧类型的体积差异

**代码实现：**
```python
import os

def extract_single_frame(file_path, frame_index, output_file, timeout=5):
    """提取单帧"""
    cmd = [
        'ffmpeg',
        '-i', file_path,
        '-vf', f'select=eq(n\\,{frame_index})',
        '-vframes', '1',
        '-y', output_file
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        if result.returncode != 0:
            print(f"提取帧失败: {result.stderr[:200]}")
            return False
        return True
    except subprocess.TimeoutExpired:
        print(f"提取帧超时")
        return False

# 提取不同类型的帧并对比体积
print("=" * 50)
print("帧体积对比")
print("=" * 50)

info = get_frame_types('test_bframes.mp4')
if info and 'frames' in info:
    frames = info['frames']
    
    # 找到第一个 I/P/B 帧
    i_idx = next((i for i, f in enumerate(frames) if f.get('pict_type') == 'I'), None)
    p_idx = next((i for i, f in enumerate(frames) if f.get('pict_type') == 'P'), None)
    b_idx = next((i for i, f in enumerate(frames) if f.get('pict_type') == 'B'), None)
    
    print(f"\n提取帧索引: I={i_idx}, P={p_idx}, B={b_idx}")
    
    for frame_type, idx in [('I', i_idx), ('P', p_idx), ('B', b_idx)]:
        if idx is not None:
            output_file = f'frame_{frame_type}.png'
            if extract_single_frame('test_bframes.mp4', idx, output_file):
                size = os.path.getsize(output_file)
                print(f"{frame_type}帧 (索引{idx}): {size} 字节")
```

**代码解析：**
- 提取单帧并保存为 PNG 文件
- 对比不同帧类型的体积
- I 帧通常体积最大，B 帧通常体积最小

---

### 实验4：分析包信息

**目标**：理解 H.264 码流结构

**代码实现：**
```python
def analyze_packets(file_path, timeout=5):
    """分析包信息"""
    cmd = [
        'ffprobe',
        '-v', 'quiet',
        '-select_streams', 'v:0',
        '-show_entries', 'packet=pts_time,dts_time,flags',
        '-read_intervals', '%+5',
        '-of', 'json',
        file_path
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        if result.returncode != 0:
            print(f"ffprobe 失败: {result.stderr[:200]}")
            return None
        return json.loads(result.stdout)
    except FileNotFoundError:
        print("ffprobe 未安装")
        return None
    except subprocess.TimeoutExpired:
        print("ffprobe 超时")
        return None

# 分析包信息
print("=" * 50)
print("包信息分析")
print("=" * 50)

info = analyze_packets('test_bframes.mp4')
if info and 'packets' in info:
    packets = info['packets']
    
    print(f"\n总包数 (前5秒): {len(packets)}")
    
    # 统计关键帧
    key_frames = sum(1 for p in packets if 'K' in p.get('flags', ''))
    print(f"关键帧数量: {key_frames}")
    
    print(f"\n前5个包的信息:")
    for i, packet in enumerate(packets[:5]):
        pts_time = packet.get('pts_time', 'N/A')
        dts_time = packet.get('dts_time', 'N/A')
        flags = packet.get('flags', '')
        
        if pts_time != 'N/A':
            pts_time = f"{float(pts_time):.3f}"
        if dts_time != 'N/A':
            dts_time = f"{float(dts_time):.3f}"
        
        print(f"  包 {i}: PTS={pts_time}s, DTS={dts_time}s, 标志={flags}")
```

**代码解析：**
- 使用 ffprobe 提取包信息
- 通过 flags 判断是否是关键帧
- 分析 PTS 和 DTS 的关系

---

## 预期结果与验收标准

### 预期输出

```text
==================================================
帧类型分析
==================================================

总帧数 (前5秒): 150
I帧数量: 6
P帧数量: 44
B帧数量: 100

前10帧的类型:
  帧 0: 类型=I, 时间=0.000s, 关键帧=是
  帧 1: 类型=P, 时间=0.100s, 关键帧=否
  帧 2: 类型=B, 时间=0.033s, 关键帧=否
  帧 3: 类型=B, 时间=0.067s, 关键帧=否
  帧 4: 类型=P, 时间=0.200s, 关键帧=否
  帧 5: 类型=B, 时间=0.133s, 关键帧=否
  帧 6: 类型=B, 时间=0.167s, 关键帧=否
  帧 7: 类型=P, 时间=0.300s, 关键帧=否
  帧 8: 类型=B, 时间=0.233s, 关键帧=否
  帧 9: 类型=B, 时间=0.267s, 关键帧=否

注：有 B 帧时，PTS 顺序可能与帧序号不一致。B 帧的 PTS 通常小于其后面的 P 帧。

==================================================
GOP 结构分析
==================================================

GOP 数量: 6

GOP 1:
  帧数: 30
  帧类型: I P B B P B B P B B P B B P B B P B B P B B P B B P B B P B B P
  时间范围: 0.000s - 0.967s

==================================================
NALU 分析
==================================================

总包数 (前5秒): 150
关键帧数量: 6

前5个包的信息:
  包 0: PTS=0.000s, DTS=0.000s, 标志=K_
  包 1: PTS=0.100s, DTS=0.033s, 标志=__
  包 2: PTS=0.033s, DTS=0.067s, 标志=__
  包 3: PTS=0.067s, DTS=0.100s, 标志=__
  包 4: PTS=0.200s, DTS=0.133s, 标志=__
```

### 验收标准

- [ ] 能区分 I/P/B 帧的特点和用途
- [ ] 能解释 GOP 的结构和作用
- [ ] 能理解 NALU 的类型和头部结构
- [ ] 能解释 SPS/PPS/IDR 的作用
- [ ] 能用 ffprobe 分析帧类型和 GOP 结构

---

## 常见错误与排查

### 错误1：没有 B 帧

**原因**：编码时未启用 B 帧。

**排查**：
- 用 `ffprobe` 检查帧类型
- 生成带 B 帧的测试视频：`-bf 2`

### 错误2：GOP 大小不一致

**原因**：视频经过编辑或转码。

**排查**：
- 用 `ffprobe` 分析 GOP 结构
- 检查视频是否经过剪辑

### 错误3：PTS 和 DTS 相同

**原因**：没有 B 帧，解码顺序和显示顺序一致。

**排查**：
- 用 `ffprobe` 检查帧类型
- 确认是否有 B 帧

---

## 课后小挑战

### 基础题：统计 GOP 大小分布

编写 Python 脚本，统计视频中 GOP 大小的分布（每个 GOP 的帧数）。

### 进阶题：分析 IDR 和普通 I 帧的区别

编写 Python 脚本，区分 IDR 和普通 I 帧，分析它们对 seek 的影响。

### 思考题1：为什么直播推流使用小 GOP？

提示：考虑观众切换频道时的首帧等待时间。

### 思考题2：为什么 B 帧会导致延迟？

提示：考虑 B 帧需要参考后面的帧，必须缓存后续帧才能解码。

---

## 面试延伸题

### Q1：I/P/B 帧的区别是什么？

**答题要点**：
1. I帧可以独立解码，P帧参考前面的帧，B帧参考前后两个方向
2. I帧体积最大，B帧体积最小
3. I帧是 seek 的目标点，B帧会导致 PTS 和 DTS 不同

### Q2：什么是 GOP？为什么需要 GOP？

**答题要点**：
1. GOP 是两个关键帧之间的帧序列
2. GOP 的存在是因为 P 帧和 B 帧需要参考其他帧
3. GOP 越大，压缩率越高，但 seek 精度越低
4. 直播使用小 GOP，点播使用大 GOP

### Q3：SPS/PPS 的作用是什么？

**答题要点**：
1. SPS 包含视频序列的全局参数（分辨率、帧率、Profile）
2. PPS 包含图像的编码参数（量化参数、熵编码模式）
3. 解码器必须先解析 SPS/PPS 才能解码
4. SPS/PPS 通常在码流开头或 IDR 前发送

---

## 延伸阅读

1. [H.264 标准文档](https://www.itu.int/rec/T-REC-H.264)
2. [H.264 NALU 详解](https://www.cnblogs.com/blogernice/articles/9896543.html)
3. [GOP 结构详解](https://en.wikipedia.org/wiki/Group_of_pictures)

---

### 下节课预告

第9节：音频编码原理（AAC/MP3）
- 学习 MDCT、心理声学、码率和感知质量
- 比较不同 AAC 码率的波形、频谱和听感差异
- 理解音频编码的基本原理
