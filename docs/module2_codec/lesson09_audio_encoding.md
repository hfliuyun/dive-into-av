# 第9节：音频编码原理（AAC/MP3）

| 字段 | 内容 |
|------|------|
| lesson_id | 09 |
| module | module2_codec |
| type | python_visualization |
| prerequisites | 第1-8节 |
| outputs | 理解音频编码的基本原理，能比较不同码率的音频质量差异 |
| estimated_time | 3-4 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.wav, test_128k.aac, test_256k.aac |
| notebook/source | notebooks/module2/09_audio_encoding.ipynb |

## 本节能力目标

- 理解音频编码的基本原理：为什么音频可以压缩？
- 理解 MDCT（改进离散余弦变换）的作用
- 理解心理声学模型的作用
- 能比较不同码率的音频质量差异
- 能用 Python 绘制波形和频谱图

## 真实工程对应场景

- 音频播放器：理解音频解码和播放流程
- 音频编辑：理解音频压缩对编辑的影响
- 直播推流：选择合适的音频码率
- 音频转码：理解码率和音质的关系
- 问题排查：音频卡顿、音质差往往与编码参数有关

## 引言

在第7-8节中，我们学习了视频编码的原理（DCT、量化、H.264 码流结构）。但你是否想过：音频是如何压缩的？为什么 128kbps 的 MP3 听起来还不错？为什么专业录音使用无损编码？

本节课我们将深入学习**音频编码的核心原理**：MDCT、心理声学模型和码率控制。这是理解 AAC/MP3 的基础，也是音频工程师的必备知识。

---

## 理论讲解

### 核心概念1：音频编码的基本原理

#### 1.1 为什么音频可以压缩？

音频压缩基于两个关键原理：

**频域冗余**：音频信号的能量通常集中在低频区域，高频区域能量较低。

**人耳掩蔽效应**：人耳对某些声音不敏感，可以被"掩蔽"。

#### 1.2 时域冗余和频域冗余

| 冗余类型 | 描述 | 压缩方法 |
|----------|------|----------|
| 时域冗余 | 相邻采样点相似 | 预测编码 |
| 频域冗余 | 能量集中在低频 | MDCT + 量化 |

#### 1.3 人耳的听觉掩蔽效应

**频率掩蔽**：一个强信号会掩蔽其频率附近的弱信号。

**时间掩蔽**：一个强信号会掩蔽其前后的弱信号。

```
频率掩蔽示例：
强信号 (1kHz) → 掩蔽 0.9kHz-1.1kHz 的弱信号

时间掩蔽示例：
强信号 → 掩蔽前 5ms 和后 20ms 的弱信号
```

---

### 核心概念2：MDCT（改进离散余弦变换）

#### 2.1 MDCT 与 DCT 的区别

| 特性 | DCT | MDCT |
|------|-----|------|
| 输入长度 | N | 2N |
| 输出长度 | N | N |
| 重叠 | 无 | 50% 重叠 |
| 边界效应 | 有 | 无 |

#### 2.2 MDCT 在音频编码中的作用

MDCT 将时域信号转换为频域信号：

```
时域信号 → MDCT → 频域信号 → 量化 → 熵编码 → 码流
```

**MDCT 的优势**：
- 无边界效应：通过 50% 重叠避免块效应
- 能量集中：频域系数能量集中，便于压缩
- 心理声学友好：频域系数与人耳感知对应

#### 2.3 窗函数和重叠保留

MDCT 使用窗函数来减少频谱泄漏：

```
输入信号 → 加窗 → MDCT → 量化 → 熵编码
```

**常见窗函数**：
| 编码器 | 变换方式 |
|--------|----------|
| AAC | 纯 MDCT（支持长窗 2048 / 短窗 256 切换） |
| MP3 | 多相滤波器组 + MDCT（混合方案） |

---

### 核心概念3：心理声学模型

#### 3.1 人耳的听觉特性

**频率范围**：20Hz - 20kHz

**频率分辨率**：约 3Hz（低频）到 300Hz（高频）

**时间分辨率**：约 2ms

#### 3.2 频率掩蔽

**频率掩蔽**：一个强信号会掩蔽其频率附近的弱信号。

**掩蔽曲线**：
```
掩蔽阈值 = 强信号电平 - 掩蔽衰减
```

**掩蔽衰减**：
- 低频：约 -20dB/octave
- 高频：约 -10dB/octave

#### 3.3 时间掩蔽

**时间掩蔽**：一个强信号会掩蔽其前后的弱信号。

**前掩蔽**：约 5ms
**后掩蔽**：约 20ms

#### 3.4 量化步长的确定

心理声学模型确定每个频带的量化步长：

```
量化步长 = 掩蔽阈值 - 信噪比要求
```

**码率控制**：
- 码率高 → 量化步长小 → 音质好
- 码率低 → 量化步长大 → 音质差

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **音频压缩基于掩蔽效应** | 人耳对某些声音不敏感，可以被掩蔽 |
| **MDCT 是音频编码的核心** | MDCT 将时域转换为频域，无边界效应 |
| **心理声学模型确定量化步长** | 掩蔽阈值决定哪些频带可以压缩 |
| **码率控制音质** | 码率越高，音质越好，但文件也越大 |
| **AAC 优于 MP3** | AAC 使用更先进的编码工具，相同码率音质更好 |

---

## 环境与素材准备

### 环境要求

```bash
# 确保 ffmpeg 已安装
ffmpeg -version

# Python 依赖
pip install numpy matplotlib scipy jupyter
```

### 测试素材

```bash
# 生成测试音频（正弦波）
ffmpeg -f lavfi -i sine=frequency=440:duration=5 \
       -c:a pcm_s16le -ar 44100 -ac 2 -y test.wav

# 转码为不同码率的 AAC
ffmpeg -i test.wav -c:a aac -b:a 128k -y test_128k.aac
ffmpeg -i test.wav -c:a aac -b:a 256k -y test_256k.aac
```

---

## 核心代码实验

### 实验1：比较不同 AAC 码率的波形

**目标**：理解码率对音频质量的影响

**代码实现：**
```python
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile

def extract_audio(input_file, output_file, bitrate='128k', timeout=5):
    """提取音频"""
    cmd = [
        'ffmpeg', '-i', input_file,
        '-c:a', 'aac', '-b:a', bitrate,
        '-y', output_file
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        if result.returncode != 0:
            print(f"提取音频失败: {result.stderr[:200]}")
            return False
        return True
    except subprocess.TimeoutExpired:
        print(f"提取音频超时")
        return False

def decode_audio(input_file, output_file, timeout=5):
    """解码音频为 WAV"""
    cmd = [
        'ffmpeg', '-i', input_file,
        '-c:a', 'pcm_s16le',
        '-y', output_file
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        if result.returncode != 0:
            print(f"解码音频失败: {result.stderr[:200]}")
            return False
        return True
    except subprocess.TimeoutExpired:
        print(f"解码音频超时")
        return False

# 生成测试音频
print("=" * 50)
print("生成测试音频")
print("=" * 50)

# 生成原始 WAV
cmd = [
    'ffmpeg', '-f', 'lavfi', '-i', 'sine=frequency=440:duration=5',
    '-c:a', 'pcm_s16le', '-ar', '44100', '-ac', '2', '-y', 'test.wav'
]
run_ffmpeg_cmd(cmd, "生成原始 WAV")

# 转码为不同码率
for bitrate in ['64k', '128k', '256k']:
    extract_audio('test.wav', f'test_{bitrate}.aac', bitrate)

# 解码为 WAV 以便比较
for bitrate in ['64k', '128k', '256k']:
    decode_audio(f'test_{bitrate}.aac', f'test_{bitrate}.wav')

# 读取并绘制波形
print("\n" + "=" * 50)
print("波形比较")
print("=" * 50)

fig, axes = plt.subplots(2, 2, figsize=(12, 8))

# 原始音频
rate, data = wavfile.read('test.wav')
axes[0, 0].plot(data[:1000, 0])
axes[0, 0].set_title('Original (WAV)')
axes[0, 0].set_xlabel('Sample')
axes[0, 0].set_ylabel('Amplitude')

# 不同码率
for idx, bitrate in enumerate(['64k', '128k', '256k']):
    row = (idx + 1) // 2
    col = (idx + 1) % 2
    
    rate, data = wavfile.read(f'test_{bitrate}.wav')
    axes[row, col].plot(data[:1000, 0])
    axes[row, col].set_title(f'AAC {bitrate}')
    axes[row, col].set_xlabel('Sample')
    axes[row, col].set_ylabel('Amplitude')

plt.tight_layout()
plt.savefig('waveform_comparison.png', dpi=150)
plt.show()

print("波形比较图已保存到 waveform_comparison.png")
```

**代码解析：**
- 使用 ffmpeg 生成不同码率的音频
- 使用 scipy.io.wavfile 读取 WAV 文件
- 使用 Matplotlib 绘制波形图

---

### 实验2：比较不同 AAC 码率的频谱

**目标**：理解码率对频谱的影响

**代码实现：**
```python
from scipy.fft import fft, fftfreq

def plot_spectrum(audio_data, sample_rate, title, ax):
    """绘制频谱图"""
    # 计算 FFT
    N = len(audio_data)
    yf = fft(audio_data)
    xf = fftfreq(N, 1 / sample_rate)
    
    # 只取正频率部分
    positive_freq = xf[:N//2]
    magnitude = np.abs(yf[:N//2]) / N
    
    # 转换为 dB
    magnitude_db = 20 * np.log10(magnitude + 1e-10)
    
    ax.plot(positive_freq, magnitude_db)
    ax.set_title(title)
    ax.set_xlabel('Frequency (Hz)')
    ax.set_ylabel('Magnitude (dB)')
    ax.set_xlim(0, 20000)
    ax.set_ylim(-80, 0)

# 绘制频谱图
print("=" * 50)
print("频谱比较")
print("=" * 50)

fig, axes = plt.subplots(2, 2, figsize=(12, 8))

# 原始音频
rate, data = wavfile.read('test.wav')
plot_spectrum(data[:4096, 0], rate, 'Original (WAV)', axes[0, 0])

# 不同码率
for idx, bitrate in enumerate(['64k', '128k', '256k']):
    row = (idx + 1) // 2
    col = (idx + 1) % 2
    
    rate, data = wavfile.read(f'test_{bitrate}.wav')
    plot_spectrum(data[:4096, 0], rate, f'AAC {bitrate}', axes[row, col])

plt.tight_layout()
plt.savefig('spectrum_comparison.png', dpi=150)
plt.show()

print("频谱比较图已保存到 spectrum_comparison.png")
```

**代码解析：**
- 使用 FFT 计算频谱
- 将幅度转换为 dB
- 绘制频谱图

---

### 实验3：比较不同 AAC 码率的文件大小

**目标**：理解码率与文件大小的关系

**代码实现：**
```python
import os

# 比较文件大小
print("=" * 50)
print("文件大小比较")
print("=" * 50)

files = ['test.wav', 'test_64k.aac', 'test_128k.aac', 'test_256k.aac']

for f in files:
    if os.path.exists(f):
        size = os.path.getsize(f) / 1024
        print(f"{f}: {size:.1f} KB")

# 计算压缩比
original_size = os.path.getsize('test.wav')
for bitrate in ['64k', '128k', '256k']:
    compressed_size = os.path.getsize(f'test_{bitrate}.aac')
    ratio = original_size / compressed_size
    print(f"\n{bitrate} 压缩比: {ratio:.1f}x")
```

**代码解析：**
- 使用 os.path.getsize 获取文件大小
- 计算压缩比

---

### 实验4：比较不同 AAC 码率的听感差异

**目标**：理解码率对听感的影响

**代码实现：**
```python
from IPython.display import Audio, display

# 播放不同码率的音频
print("=" * 50)
print("听感比较")
print("=" * 50)

print("\n请播放以下音频，比较听感差异：")

# 原始音频
print("\n1. 原始音频 (WAV):")
display(Audio('test.wav'))

# 不同码率
for bitrate in ['64k', '128k', '256k']:
    print(f"\n2. AAC {bitrate}:")
    display(Audio(f'test_{bitrate}.wav'))

print("\n听感对比要点：")
print("- 64kbps：可能有明显的高频丢失和压缩伪影")
print("- 128kbps：大部分音乐听起来不错，但仔细听可能有细微差异")
print("- 256kbps：与原始音频几乎无法区分")
```

**代码解析：**
- 使用 IPython.display.Audio 播放音频
- 用户可以实际感受不同码率的听感差异

---

## 预期结果与验收标准

### 预期输出

```text
==================================================
生成测试音频
==================================================

==================================================
波形比较
==================================================

波形比较图已保存到 waveform_comparison.png

==================================================
频谱比较
==================================================

频谱比较图已保存到 spectrum_comparison.png

==================================================
文件大小比较
==================================================

test.wav: 862.5 KB
test_64k.aac: 40.2 KB
test_128k.aac: 78.5 KB
test_256k.aac: 155.3 KB

64k 压缩比: 21.5x
128k 压缩比: 11.0x
256k 压缩比: 5.6x

注：以上为示例数值，实际结果可能因系统和 ffmpeg 版本略有差异。
```

### 验收标准

- [ ] 能解释音频编码的基本原理（频域冗余、人耳掩蔽效应）
- [ ] 能理解 MDCT 的作用（时域 → 频域，无边界效应）
- [ ] 能理解心理声学模型的作用（确定量化步长）
- [ ] 能比较不同码率的音频质量差异
- [ ] 能用 Python 绘制波形和频谱图
- [ ] 能通过听感描述不同码率音频的质量差异

---

## 常见错误与排查

### 错误1：无法读取 AAC 文件

**原因**：scipy.io.wavfile 只能读取 WAV 文件。

**排查**：
- 先将 AAC 解码为 WAV：`ffmpeg -i test.aac -c:a pcm_s16le test.wav`
- 然后读取 WAV 文件

### 错误2：频谱图显示异常

**原因**：FFT 参数设置不当。

**排查**：
- 确认 FFT 长度是 2 的幂
- 确认采样率参数正确

### 错误3：波形图显示不明显

**原因**：音频是正弦波，波形变化不明显。

**排查**：
- 使用真实音频文件
- 增加音频时长

---

## 课后小挑战

### 基础题：比较 MP3 和 AAC 的音质

使用 ffmpeg 将同一音频分别编码为 MP3 和 AAC（相同码率），比较它们的频谱差异。

### 进阶题：分析心理声学掩蔽效应

生成两个频率相近的正弦波（如 440Hz 和 450Hz），观察强信号对弱信号的掩蔽效果。

### 思考题1：为什么 128kbps 的 AAC 听起来还不错？

提示：考虑人耳的频率范围和掩蔽效应。

### 思考题2：为什么专业录音使用无损编码？

提示：考虑多次编辑和转码对音质的影响。

---

## 面试延伸题

### Q1：AAC 和 MP3 的区别是什么？

**答题要点**：
1. AAC 使用更先进的编码工具（MDCT、心理声学模型）
2. 相同码率下，AAC 音质优于 MP3
3. AAC 是 H.264 的最佳搭档

### Q2：什么是心理声学模型？

**答题要点**：
1. 心理声学模型描述人耳的听觉特性
2. 包括频率掩蔽和时间掩蔽
3. 用于确定每个频带的量化步长

### Q3：如何选择合适的音频码率？

**答题要点**：
1. 语音：64kbps AAC 足够
2. 音乐：128-256kbps AAC
3. 高保真：无损编码（FLAC）

---

## 延伸阅读

1. [AAC 编码原理](https://en.wikipedia.org/wiki/Advanced_Audio_Coding)
2. [MP3 编码原理](https://en.wikipedia.org/wiki/MP3)
3. [心理声学模型](https://en.wikipedia.org/wiki/Psychoacoustics)

---

### 下节课预告

第10节：FFmpeg 解码器 API 入门
- 学习 AVCodecContext、AVPacket、AVFrame 的概念
- 用 C++ 编写最简视频解码器，输出 YUV 帧
- 进入 C++ 工程实战阶段
