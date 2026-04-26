# 第3节：数字音频基础

## 本节能力目标
- 理解数字音频的核心概念：采样率、位深度、声道数、PCM
- 能读取和录制音频文件，理解音频数据的存储格式
- 能绘制音频波形图和频谱图，分析音频的时域和频域特征
- 能解释不同采样率和位深度对音质的影响
- 能识别常见的音频格式和编码特性

## 真实工程对应场景
- 音频播放器开发：理解音频数据格式和播放流程
- 音频处理工具：音量调整、格式转换、频谱分析
- 语音识别和音频分析：预处理、特征提取
- 音视频同步：理解音频时间戳和播放时序
- 音频质量评估：采样率、位深度对音质的影响

## 引言

在前两节课中，我们学习了数字图像和视频的色彩空间。本节课我们将转向音频领域，探索数字音频的基本原理。

音频是我们日常交流的重要媒介，从语音通话到音乐播放，都离不开数字音频技术。但你是否想过，声音是如何被计算机记录和处理的？为什么CD音质是44.1kHz采样率？什么是16位深度？这些概念对音频质量有什么影响？

本节课我们将通过动手实验，直观理解数字音频的核心概念。你将学习如何读取音频文件、分析音频数据、绘制波形图和频谱图，并理解不同采样率和位深度对音质的影响。

---

## 理论讲解

### 核心概念1：数字音频的三大要素

#### 1. 采样率（Sampling Rate）
- **定义**：每秒钟对声音信号采样的次数，单位为赫兹（Hz）
- **原理**：根据奈奎斯特-香农采样定理，采样率必须至少是信号最高频率的两倍
- **常见标准**：
  - 8kHz：电话语音质量
  - 22.05kHz：广播音质
  - 44.1kHz：CD音质（人类听觉上限约20kHz，44.1kHz满足2倍要求）
  - 48kHz：DVD/专业音频
  - 96kHz/192kHz：高保真音频

#### 2. 位深度（Bit Depth）
- **定义**：每个采样点的量化精度，决定动态范围和信噪比
- **原理**：将模拟信号的幅度离散化为数字值
- **常见标准**：
  - 8位：256个量化级别，动态范围约48dB（电话语音）
  - 16位：65,536个量化级别，动态范围约96dB（CD音质）
  - 24位：16,777,216个量化级别，动态范围约144dB（专业音频）
  - 32位浮点：专业音频处理

#### 3. 声道数（Channels）
- **定义**：音频信号的独立声道数量
- **常见格式**：
  - 单声道（Mono）：1个声道，所有声音混合在一起
  - 立体声（Stereo）：2个声道（左/右），提供空间感
  - 环绕声（Surround）：5.1（5个全频+1个低频）、7.1等

### 核心概念2：PCM音频格式

PCM（Pulse Code Modulation，脉冲编码调制）是最基础的数字音频格式：

#### PCM编码过程
```
模拟信号 → 采样（时间离散化） → 量化（幅度离散化） → 编码（二进制表示）
```

#### PCM数据存储
- **无压缩**：原始采样数据直接存储
- **存储计算**：音频文件大小 = 采样率 × 位深度/8 × 声道数 × 时长
- **示例**：44.1kHz、16位、立体声、3分钟音频
  - 每秒数据：44,100 × 2 × 2 = 176,400 字节
  - 总大小：176,400 × 180 = 31,752,000 字节 ≈ 30.3 MB

#### PCM文件格式
- WAV：Windows标准，包含PCM数据头
- AIFF：苹果标准
- RAW PCM：纯数据，无文件头

### 核心概念3：音频波形与频谱

#### 时域分析（波形图）
- **横轴**：时间（秒）
- **纵轴**：振幅（量化值）
- **特征**：显示音频信号的幅度随时间变化
- **用途**：观察音量、节奏、静音段

#### 频域分析（频谱图）
- **横轴**：频率（Hz）
- **纵轴**：幅度（dB）
- **特征**：显示音频信号在不同频率上的能量分布
- **用途**：分析音色、识别乐器、检测噪声

#### 傅里叶变换
将时域信号转换为频域信号的关键数学工具：
```
时域信号 → 傅里叶变换 → 频域信号
```

---

## 关键点总结
1. **采样率决定频率上限**：44.1kHz采样率能记录最高22.05kHz的声音
2. **位深度决定动态范围**：16位提供96dB动态范围，足够覆盖人耳听觉
3. **声道数影响空间感**：立体声比单声道有更好的空间定位
4. **PCM是最基础的音频格式**：所有压缩格式都基于PCM
5. **时域和频域分析互补**：波形图看时间变化，频谱图看频率分布

---

## 环境与素材准备

### 环境要求
```bash
# 安装音频处理库
pip install soundfile matplotlib numpy scipy

# 可选：用于音频录制
pip install pyaudio
```

### 测试素材
```bash
# 创建音频素材目录
mkdir -p ../assets/audio

# 下载测试音频（5秒，44.1kHz，16位，立体声）
wget https://filesamples.com/samples/audio/wav/sample1.wav -O ../assets/audio/test_stereo.wav

# 下载不同采样率的测试音频
wget https://filesamples.com/samples/audio/wav/sample2.wav -O ../assets/audio/test_mono.wav

# 验证音频信息
ffprobe -v error -show_entries stream=sample_rate,channels,bits_per_sample -of default=noprint_wrappers=1 ../assets/audio/test_stereo.wav
```

---

## 核心代码实验

### 实验1：读取音频文件并分析基本信息
**目标：** 使用soundfile库读取音频文件，获取采样率、位深度、声道数等信息

**代码实现：**
```python
import soundfile as sf
import numpy as np

# 读取音频文件
audio_path = '../assets/audio/test_stereo.wav'
data, sample_rate = sf.read(audio_path)

print(f"=== 音频基本信息 ===")
print(f"音频文件: {audio_path}")
print(f"采样率: {sample_rate} Hz")
print(f"声道数: {data.shape[1] if len(data.shape) > 1 else 1}")
print(f"总采样点数: {len(data)}")
print(f"音频时长: {len(data) / sample_rate:.2f} 秒")
print(f"数据类型: {data.dtype}")
print(f"数据范围: [{data.min():.4f}, {data.max():.4f}]")

    # 计算文件大小
    file_size_bytes = data.nbytes
    print(f"原始PCM数据大小: {file_size_bytes:,} 字节 ({file_size_bytes/1024/1024:.2f} MB)")
```

**代码解析：**
- `sf.read()` 返回音频数据和采样率
- 立体声音频的shape为`(采样点数, 声道数)`，单声道为`(采样点数,)`
- 数据类型通常是`float32`或`int16`，表示位深度
- 通过采样点数和采样率计算音频时长

---

### 实验2：绘制音频波形图
**目标：** 可视化音频的时域波形，观察振幅随时间的变化

**代码实现：**
```python
import matplotlib.pyplot as plt

# 创建时间轴
duration = len(data) / sample_rate
time = np.linspace(0, duration, len(data))

# 绘制波形图
plt.figure(figsize=(12, 6))

if len(data.shape) > 1:  # 立体声
    plt.subplot(2, 1, 1)
    plt.plot(time, data[:, 0], alpha=0.7)
    plt.title('左声道波形图', fontsize=14)
    plt.xlabel('时间 (秒)')
    plt.ylabel('振幅')
    plt.grid(True, alpha=0.3)
    
    plt.subplot(2, 1, 2)
    plt.plot(time, data[:, 1], alpha=0.7, color='orange')
    plt.title('右声道波形图', fontsize=14)
    plt.xlabel('时间 (秒)')
    plt.ylabel('振幅')
    plt.grid(True, alpha=0.3)
else:  # 单声道
    plt.plot(time, data, alpha=0.7)
    plt.title('单声道波形图', fontsize=14)
    plt.xlabel('时间 (秒)')
    plt.ylabel('振幅')
    plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.show()

# 计算并显示统计信息
print(f"=== 波形统计信息 ===")
if len(data.shape) > 1:
    print(f"左声道 - 最大值: {data[:, 0].max():.4f}, 最小值: {data[:, 0].min():.4f}, 平均值: {data[:, 0].mean():.4f}")
    print(f"右声道 - 最大值: {data[:, 1].max():.4f}, 最小值: {data[:, 1].min():.4f}, 平均值: {data[:, 1].mean():.4f}")
else:
    print(f"最大值: {data.max():.4f}, 最小值: {data.min():.4f}, 平均值: {data.mean():.4f}")
```

**代码解析：**
- 时间轴通过`np.linspace()`创建，从0到音频时长
- 立体声音频需要分别绘制左右声道
- 波形图显示振幅随时间的变化，可以观察音量、节奏等特征
- 统计信息帮助理解音频的动态范围

---

### 实验3：绘制音频频谱图
**目标：** 分析音频的频域特征，观察不同频率的能量分布

**代码实现：**
```python
from scipy import signal
from scipy.fft import fft, fftfreq

# 计算频谱
def compute_spectrum(audio_data, sample_rate):
    """计算音频信号的频谱"""
    # 使用汉明窗减少频谱泄漏
    window = signal.windows.hamming(len(audio_data))
    windowed_data = audio_data * window
    
    # 计算FFT
    n = len(windowed_data)
    yf = fft(windowed_data)
    xf = fftfreq(n, 1 / sample_rate)
    
    # 只取正频率部分
    half_n = n // 2
    frequencies = xf[:half_n]
    magnitude = np.abs(yf[:half_n]) / n * 2  # 归一化
    
    return frequencies, magnitude

# 绘制频谱图
plt.figure(figsize=(12, 8))

if len(data.shape) > 1:  # 立体声
    # 左声道频谱
    freqs_left, mag_left = compute_spectrum(data[:, 0], sample_rate)
    
    plt.subplot(2, 1, 1)
    plt.semilogx(freqs_left[1:], 20 * np.log10(mag_left[1:] + 1e-10), alpha=0.7)
    plt.title('左声道频谱图', fontsize=14)
    plt.xlabel('频率 (Hz)')
    plt.ylabel('幅度 (dB)')
    plt.grid(True, alpha=0.3)
    plt.xlim(20, sample_rate/2)  # 人耳可听范围
    
    # 右声道频谱
    freqs_right, mag_right = compute_spectrum(data[:, 1], sample_rate)
    
    plt.subplot(2, 1, 2)
    plt.semilogx(freqs_right[1:], 20 * np.log10(mag_right[1:] + 1e-10), alpha=0.7, color='orange')
    plt.title('右声道频谱图', fontsize=14)
    plt.xlabel('频率 (Hz)')
    plt.ylabel('幅度 (dB)')
    plt.grid(True, alpha=0.3)
    plt.xlim(20, sample_rate/2)
    
else:  # 单声道
    freqs, mag = compute_spectrum(data, sample_rate)
    
    plt.semilogx(freqs[1:], 20 * np.log10(mag[1:] + 1e-10), alpha=0.7)
    plt.title('单声道频谱图', fontsize=14)
    plt.xlabel('频率 (Hz)')
    plt.ylabel('幅度 (dB)')
    plt.grid(True, alpha=0.3)
    plt.xlim(20, sample_rate/2)

plt.tight_layout()
plt.show()

# 显示频谱统计信息
print(f"=== 频谱统计信息 ===")
if len(data.shape) > 1:
    peak_freq_left = freqs_left[np.argmax(mag_left)]
    peak_freq_right = freqs_right[np.argmax(mag_right)]
    print(f"左声道 - 峰值频率: {peak_freq_left:.1f} Hz, 峰值幅度: {20*np.log10(mag_left.max()):.1f} dB")
    print(f"右声道 - 峰值频率: {peak_freq_right:.1f} Hz, 峰值幅度: {20*np.log10(mag_right.max()):.1f} dB")
else:
    peak_freq = freqs[np.argmax(mag)]
    print(f"峰值频率: {peak_freq:.1f} Hz, 峰值幅度: {20*np.log10(mag.max()):.1f} dB")
```

**代码解析：**
- 使用FFT（快速傅里叶变换）将时域信号转换为频域
- 汉明窗减少频谱泄漏，提高频谱分析精度
- 半对数坐标（semilogx）更适合显示音频频谱
- dB（分贝）单位表示相对幅度，更符合人耳感知
- 频谱图显示不同频率的能量分布，可以分析音色特征

---

### 实验4：对比不同采样率的音频
**目标：** 理解采样率对音频质量的影响

**代码实现：**
```python
def resample_audio(audio_data, original_rate, target_rate):
    """重采样音频到目标采样率"""
    from scipy import signal
    
    # 计算重采样比例
    ratio = target_rate / original_rate
    
    # 重采样
    resampled = signal.resample(audio_data, int(len(audio_data) * ratio))
    
    return resampled, target_rate

# 读取原始音频
original_data, original_rate = sf.read('../assets/audio/test_stereo.wav')

# 创建不同采样率的版本
rates_to_test = [8000, 16000, 22050, 44100]

plt.figure(figsize=(15, 10))

for i, target_rate in enumerate(rates_to_test, 1):
    # 重采样
    resampled_data, new_rate = resample_audio(original_data, original_rate, target_rate)
    
    # 计算频谱（只取左声道）
    if len(resampled_data.shape) > 1:
        channel_data = resampled_data[:, 0]
    else:
        channel_data = resampled_data
    
    freqs, mag = compute_spectrum(channel_data, new_rate)
    
    # 绘制频谱对比
    plt.subplot(2, 2, i)
    plt.semilogx(freqs[1:], 20 * np.log10(mag[1:] + 1e-10), alpha=0.7)
    plt.title(f'{target_rate} Hz 采样率', fontsize=12)
    plt.xlabel('频率 (Hz)')
    plt.ylabel('幅度 (dB)')
    plt.grid(True, alpha=0.3)
    plt.xlim(20, target_rate/2)
    plt.axvline(x=target_rate/2, color='red', linestyle='--', alpha=0.5, label='奈奎斯特频率')
    plt.legend()

plt.tight_layout()
plt.show()

print(f"=== 采样率对比分析 ===")
for target_rate in rates_to_test:
    nyquist = target_rate / 2
    print(f"{target_rate}Hz采样率: 最高可记录频率 = {nyquist:.1f}Hz, {'(电话质量)' if target_rate == 8000 else '(CD质量)' if target_rate == 44100 else ''}")
```

**代码解析：**
- 重采样改变音频的采样率，影响可记录的频率范围
- 奈奎斯特频率（采样率/2）是理论最高可记录频率
- 低采样率（8kHz）只能记录最高4kHz的声音，适合语音
- 高采样率（44.1kHz）能记录完整的人耳可听范围（20Hz-20kHz）

---

### 实验5：录制音频并分析
**目标：** 使用麦克风录制音频，并进行实时分析

**代码实现：**
```python
import pyaudio
import wave
import time

def record_audio(duration=3, sample_rate=44100, channels=1):
    """录制音频"""
    CHUNK = 1024
    FORMAT = pyaudio.paInt16
    CHANNELS = channels
    RATE = sample_rate
    
    p = pyaudio.PyAudio()
    
    print(f"开始录制 {duration} 秒音频...")
    print(f"参数: {sample_rate}Hz, {channels}声道, 16位深度")
    
    stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK)
    
    frames = []
    
    for i in range(0, int(RATE / CHUNK * duration)):
        data = stream.read(CHUNK)
        frames.append(data)
    
    print("录制完成!")
    
    stream.stop_stream()
    stream.close()
    p.terminate()
    
    # 转换为numpy数组
    audio_data = np.frombuffer(b''.join(frames), dtype=np.int16)
    
    # 归一化到[-1, 1]
    audio_data = audio_data.astype(np.float32) / np.iinfo(np.int16).max
    
    return audio_data, RATE

# 录制音频（需要麦克风）
try:
    recorded_data, record_rate = record_audio(duration=3)
    
    # 绘制录制的音频波形
    plt.figure(figsize=(12, 4))
    time_axis = np.linspace(0, 3, len(recorded_data))
    plt.plot(time_axis, recorded_data, alpha=0.7)
    plt.title('录制的音频波形', fontsize=14)
    plt.xlabel('时间 (秒)')
    plt.ylabel('振幅')
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.show()
    
    print(f"=== 录制音频分析 ===")
    print(f"采样点数: {len(recorded_data)}")
    print(f"最大值: {recorded_data.max():.4f}, 最小值: {recorded_data.min():.4f}")
    print(f"平均音量: {np.mean(np.abs(recorded_data)):.4f}")
    
except Exception as e:
    print(f"麦克风录制失败: {e}")
    print("请确保已安装pyaudio并连接麦克风")
```

**代码解析：**
- 使用pyaudio库访问音频输入设备
- 实时录制音频并转换为numpy数组
- 16位PCM数据需要归一化到[-1, 1]范围
- 录制功能需要实际硬件支持，失败时提供友好提示

---

## 预期结果与验收标准

### 预期输出
1. **音频基本信息输出**：
```
=== 音频基本信息 ===
音频文件: ../assets/audio/test_stereo.wav
采样率: 44100 Hz
声道数: 2
总采样点数: 220500
音频时长: 5.00 秒
数据类型: float32
数据范围: [-0.1234, 0.5678]
原始PCM数据大小: 1,764,000 字节 (1.68 MB)
```

2. **波形图显示**：清晰的时域波形，能观察振幅变化
3. **频谱图显示**：频域能量分布，峰值频率可识别
4. **采样率对比**：不同采样率的频谱有明显差异，高频截止点不同

### 验收标准
- [ ] 成功读取音频文件并正确显示基本信息
- [ ] 波形图正确显示音频的时域特征
- [ ] 频谱图正确显示音频的频域特征
- [ ] 理解采样率、位深度、声道数的含义
- [ ] 能解释不同采样率对音频质量的影响

---

## 常见错误与排查

### 错误1：音频文件读取失败
**原因**：文件路径错误或格式不支持
**排查**：
- 检查文件路径是否正确
- 使用`ffprobe`验证音频文件格式
- 确保已安装soundfile库：`pip install soundfile`

### 错误2：频谱图显示异常
**原因**：FFT参数设置不当或数据预处理问题
**排查**：
- 确保使用窗函数（如汉明窗）减少频谱泄漏
- 检查采样率参数是否正确传递
- 验证音频数据没有NaN或无穷大值

### 错误3：波形图幅度异常
**原因**：数据归一化问题或数据类型错误
**排查**：
- 检查音频数据的范围，确保在[-1, 1]之间
- 验证数据类型，PCM数据可能需要转换
- 检查声道分离是否正确

### 错误4：录制功能失败
**原因**：麦克风权限或驱动问题
**排查**：
- 检查系统麦克风权限
- 确保pyaudio正确安装：`pip install pyaudio`
- 测试系统录音功能是否正常

---

## 课后小挑战

### 基础题：音频音量分析
修改实验2的代码，计算音频的平均音量（RMS）和峰值音量，并显示音量随时间的变化曲线。

### 进阶题：频谱特征提取
实现一个函数，从音频频谱中提取以下特征：
1. 频谱质心（spectral centroid）
2. 频谱带宽（spectral bandwidth）
3. 频谱滚降点（spectral rolloff）

### 思考题：音频压缩的影响
下载同一段音频的MP3版本（128kbps）和原始WAV版本，对比两者的频谱差异，分析压缩对音频质量的影响。

---

## 面试延伸题

### Q1：为什么CD采用44.1kHz采样率，而不是40kHz或50kHz？
**答题要点**：
1. 奈奎斯特定理要求采样率至少是信号最高频率的2倍
2. 人耳可听范围约20Hz-20kHz，需要至少40kHz采样率
3. 44.1kHz考虑了抗混叠滤波器的过渡带
4. 历史原因：与数字视频设备兼容

### Q2：16位深度和24位深度在实际应用中有何区别？
**答题要点**：
1. 动态范围：16位约96dB，24位约144dB
2. 量化噪声：24位的量化噪声更低
3. 实际应用：16位足够用于消费级音频，24位用于专业录音和母带处理
4. 文件大小：24位比16位大50%

### Q3：如何从频谱图中判断音频质量？
**答题要点**：
1. 高频截止点：接近奈奎斯特频率表示高频信息完整
2. 频谱平滑度：过度锯齿可能表示量化噪声或压缩失真
3. 谐波结构：音乐应有清晰的谐波分布
4. 噪声基底：高质量音频的噪声基底应较低

---

## 延伸阅读
1. [Digital Audio Fundamentals - Sound on Sound](https://www.soundonsound.com/techniques/digital-audio-fundamentals)
2. [PCM Audio Technology - Audio Engineering Society](https://www.aes.org/)
3. [FFT and Spectral Analysis - SciPy Documentation](https://docs.scipy.org/doc/scipy/tutorial/fft.html)
4. [Audio Signal Processing with Python - Real Python](https://realpython.com/playing-and-recording-sound-python/)

---

### 下节课预告
第4节：音视频容器格式与编解码
- 学习容器格式（MP4/MKV/FLV）与编解码器（H.264/AAC）的区别
- 使用ffprobe分析媒体文件
- 理解封装与解封装的基本原理