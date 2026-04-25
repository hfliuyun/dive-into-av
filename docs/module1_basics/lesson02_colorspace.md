# 第2节：视频的色彩空间

## 本节能力目标
- 理解YUV色彩模型与RGB的本质区别
- 掌握YUV420P采样原理和平面存储结构
- 能独立操作YUV分量并观察色彩变化
- 理解视频编码优先选择YUV而非RGB的技术原因

## 真实工程对应场景
- 视频播放器像素格式转换链路
- 视频编码器输入预处理
- 色彩空间适配（不同设备/标准间转换）
- 视频画质分析与调优
- YUV裸流分析与排查

## 引言

在第1节我们学习了RGB色彩模型，这是计算机显示图像的标准格式。但你知道吗？几乎所有的视频编码（H.264/HEVC/AV1）、视频文件、流媒体都使用YUV色彩模型而非RGB。

为什么视频行业选择YUV？这背后是**人眼视觉特性**和**压缩效率**的双重考量。本节课我们将深入学习YUV色彩空间，通过动手实验直观理解Y/U/V三个分量的作用，以及色度下采样带来的压缩优势。

---

## 🔬 理论讲解

### 核心概念1：YUV色彩模型

YUV是一种将亮度信息和色度信息分离的色彩模型：
- **Y（Luma，亮度）**：表示像素的明暗程度，0表示全黑，255表示全亮
- **U（Cb，蓝色色度）**：表示蓝色分量与亮度的差值
- **V（Cr，红色色度）**：表示红色分量与亮度的差值

#### 为什么分离亮度和色度？
人眼视网膜上感知亮度的视杆细胞数量远多于感知色彩的视锥细胞，这导致：
- ✅ 人眼对亮度变化非常敏感
- ❌ 人眼对色度变化相对不敏感

利用这个特性，我们可以对色度信息进行压缩（下采样）而人眼几乎察觉不到画质损失，这就是YUV色彩模型的核心优势。

### 核心概念2：YUV采样格式

视频中最常用的是**YUV420P**采样格式，采样比为4:2:0：
- **水平方向**：每4个Y像素对应1个U和1个V像素
- **垂直方向**：每4个Y像素对应1个U和1个V像素
- **总体**：每4个Y像素共享1组U/V像素，色度分辨率是亮度的1/4

#### 存储空间计算
对于`width × height`的图像：
- RGB格式：`width × height × 3` 字节
- YUV420P格式：`width × height × 1.5` 字节（Y占1倍，U/V各占0.25倍）
- **节省50%的存储空间！** 这是视频压缩的重要基础。

#### YUV420P平面存储结构
YUV420P采用**平面（Planar）**存储方式：
1. 先存储完整的Y平面（所有Y像素连续存放）
2. 再存储完整的U平面（U像素连续存放，宽高各为Y的1/2）
3. 最后存储完整的V平面（V像素连续存放，宽高各为Y的1/2）

```
Y平面：[Y00, Y01, Y02, Y03, ...]  # 尺寸: w × h
U平面：[U00, U01, ...]            # 尺寸: w/2 × h/2
V平面：[V00, V01, ...]            # 尺寸: w/2 × h/2
```

#### 其他采样格式对比
| 采样格式 | 色度分辨率 | 存储空间比例 | 应用场景 |
|----------|------------|--------------|----------|
| YUV444P | 1:1（和Y相同） | 3× | 专业视频制作、高质量母版 |
| YUV422P | 水平1/2，垂直1:1 | 2× | 广电行业、专业摄像机 |
| YUV420P | 水平1/2，垂直1/2 | 1.5× | 互联网视频、直播、蓝光光盘 |

### 核心概念3：RGB ↔ YUV转换公式

视频行业有两个主要的转换标准：

#### 1. BT.601（标清标准，用于480p/576p以下分辨率）
```
Y  =  0.299 * R + 0.587 * G + 0.114 * B
U  = -0.147 * R - 0.289 * G + 0.436 * B + 128
V  =  0.615 * R - 0.515 * G - 0.100 * B + 128

R = Y + 1.13983 * (V - 128)
G = Y - 0.39465 * (U - 128) - 0.58060 * (V - 128)
B = Y + 2.03211 * (U - 128)
```

#### 2. BT.709（高清标准，用于720p/1080p及以上分辨率）
```
Y  = 0.2126 * R + 0.7152 * G + 0.0722 * B
U  = -0.1146 * R -   0.3854 * G + 0.5000 * B + 128
V  =  0.5000 * R -   0.4542 * G - 0.0458 * B + 128

R = Y + 1.5748 * (V - 128)
G = Y - 0.1873 * (U - 128) - 0.4681 * (V - 128)
B = Y + 1.8556 * (U - 128)
```

#### 2. BT.709（高清标准，用于720p/1080p及以上分辨率）
```
Y  =  0.2126 * R + 0.7152 * G + 0.0722 * B
U  = -0.1146 * R - 0.3854 * G + 0.5000 * B + 128
V  =  0.5000 * R - 0.4542 * G - 0.0458 * B + 128
```

#### 关键注意点
- U/V分量的取值范围通常是16-240（TV Range），而非0-255（Full Range）
- 转换时需要注意匹配正确的标准，否则会出现颜色偏色
- 实际工程中建议使用FFmpeg等成熟库进行转换，避免手动计算错误

---

## 📌 关键点总结
1. **YUV优势**：分离亮度和色度，利用人眼视觉特性实现高效压缩
2. **YUV420P**：互联网视频的主流格式，比RGB节省50%存储空间
3. **平面存储**：Y、U、V三个平面分开连续存放，U/V尺寸是Y的1/2
4. **转换标准**：标清用BT.601，高清用BT.709，转换错误会导致颜色失真
5. **工程实践**：优先使用成熟库进行色彩空间转换，避免手动实现

---

## 🛠️ 环境与素材准备

### 环境要求
```bash
# 安装新增依赖PyAV
pip install av>=10.0.0

# 验证安装
python -c "import av; print(f'PyAV version: {av.__version__}')"
```

### 测试素材
本节课使用5秒360p的轻量化测试视频，确保实验在3秒内完成：
```bash
# 创建素材目录
mkdir -p assets/video

# 下载5秒360p测试视频（约1MB）
wget https://sample-videos.com/video123/mp4/360/big_buck_bunny_360p_1mb.mp4 \
     -O assets/video/test_360p_5s.mp4

# 验证视频信息
ffprobe -v error -show_entries format=duration -show_entries stream=width,height,pix_fmt \
        -of default=noprint_wrappers=1 assets/video/test_360p_5s.mp4
```

预期输出：
```
duration=5.312000
width=640
height=360
pix_fmt=yuv420p
```

---

## 💻 核心代码实验

### 实验1：读取视频并提取YUV帧
**目标：** 使用PyAV打开视频文件，读取第一帧YUV数据，理解帧结构。

**代码实现：**
```python
import av
import numpy as np
import matplotlib.pyplot as plt
import cv2

# 打开视频文件
container = av.open('../assets/video/test_360p_5s.mp4')

# 获取视频流
video_stream = container.streams.video[0]
print(f"视频信息：")
print(f"  分辨率: {video_stream.width}x{video_stream.height}")
print(f"  帧率: {float(video_stream.average_rate):.2f} fps")
print(f"  像素格式: {video_stream.pix_fmt}")
print(f"  总帧数: {video_stream.frames}")

# 读取第一帧
frame = next(container.decode(video=0))
print(f"\n帧信息：")
print(f"  格式: {frame.format.name}")
print(f"  平面数: {len(frame.planes)}")
print(f"  Y平面步长: {frame.planes[0].line_size}")
print(f"  U平面步长: {frame.planes[1].line_size}")
print(f"  V平面步长: {frame.planes[2].line_size}")

# 关闭容器
container.close()
```

**代码解析：**
- `av.open()` 打开视频文件，返回容器对象
- `container.decode(video=0)` 解码第0个视频流
- `frame.planes` 包含YUV三个平面的数据，索引0=Y, 1=U, 2=V
- `line_size` 是每行的字节数，可能大于宽度（对齐需要）

---

### 实验2：分离Y/U/V分量并可视化
**目标：** 提取Y、U、V三个分量，分别可视化，直观理解各分量的作用。

**代码实现：**
```python
# 将平面数据转换为NumPy数组
def read_plane(plane, width, height):
    """读取平面数据为NumPy数组"""
    return np.frombuffer(plane, np.uint8).reshape(height, width)

# 获取YUV分量
y_width, y_height = frame.width, frame.height
uv_width, uv_height = y_width // 2, y_height // 2

y = read_plane(frame.planes[0], y_width, y_height)
u = read_plane(frame.planes[1], uv_width, uv_height)
v = read_plane(frame.planes[2], uv_width, uv_height)

print(f"分量尺寸：")
print(f"  Y: {y.shape}")
print(f"  U: {u.shape}")
print(f"  V: {v.shape}")

# 可视化三个分量
fig, axes = plt.subplots(1, 3, figsize=(18, 6))

# Y分量（灰度图）
axes[0].imshow(y, cmap='gray')
axes[0].set_title('Y Component (Luma - Brightness)', fontsize=14)
axes[0].axis('off')

# U分量（蓝色色度，使用冷色调调色板）
axes[1].imshow(u, cmap='Blues')
axes[1].set_title('U Component (Cb - Blue Chroma)', fontsize=14)
axes[1].axis('off')

# V分量（红色色度，使用暖色调调色板）
axes[2].imshow(v, cmap='Reds')
axes[2].set_title('V Component (Cr - Red Chroma)', fontsize=14)
axes[2].axis('off')

plt.tight_layout()
plt.show()

# 对比存储空间
rgb_size = y_width * y_height * 3
yuv_size = y.size + u.size + v.size
print(f"\n存储空间对比：")
print(f"  RGB格式: {rgb_size:,} 字节")
print(f"  YUV420P格式: {yuv_size:,} 字节")
print(f"  节省空间: {100 - (yuv_size / rgb_size) * 100:.1f}%")
```

**代码解析：**
- U/V分量的宽高是Y分量的1/2，所以需要用`// 2`计算
- Y分量是灰度图，直接反映画面的明暗
- U分量反映蓝色信息，V分量反映红色信息
- YUV420P确实比RGB节省约50%的存储空间

---

### 实验3：修改U/V分量观察色彩变化
**目标：** 通过修改U/V分量的值，观察画面色彩变化，直观理解色度分量的作用。

**代码实现：**
```python
# 复制原始帧数据用于修改
y_modified = y.copy()
u_modified = u.copy()
v_modified = v.copy()

# 创建三种修改版本
# 版本1：U分量全部置128（无色度）
u_128 = np.full_like(u, 128)
v_original = v.copy()

# 版本2：V分量全部置128（无色度）
u_original = u.copy()
v_128 = np.full_like(v, 128)

# 版本3：U/V分量都置128（完全灰度图）
u_all_128 = np.full_like(u, 128)
v_all_128 = np.full_like(v, 128)

# 将修改后的YUV转换为RGB用于显示
def yuv420p_to_rgb(y, u, v):
    """简化的YUV420P转RGB，使用BT.601标准"""
    # 上采样U/V到Y的尺寸
    u_upsampled = cv2.resize(u, (y.shape[1], y.shape[0]), interpolation=cv2.INTER_NEAREST)
    v_upsampled = cv2.resize(v, (y.shape[1], y.shape[0]), interpolation=cv2.INTER_NEAREST)
    
    # 转换公式（BT.601）
    y = y.astype(np.float32)
    u = u_upsampled.astype(np.float32) - 128
    v = v_upsampled.astype(np.float32) - 128
    
    r = y + 1.13983 * v
    g = y - 0.39465 * u - 0.58060 * v
    b = y + 2.03211 * u
    
    # 裁剪到0-255范围
    rgb = np.stack([r, g, b], axis=-1)
    rgb = np.clip(rgb, 0, 255).astype(np.uint8)
    return rgb

# 生成不同版本的RGB图像
rgb_original = yuv420p_to_rgb(y, u, v)
rgb_u128 = yuv420p_to_rgb(y, u_128, v_original)
rgb_v128 = yuv420p_to_rgb(y, u_original, v_128)
rgb_gray = yuv420p_to_rgb(y, u_all_128, v_all_128)

# 可视化对比
fig, axes = plt.subplots(2, 2, figsize=(16, 12))

axes[0, 0].imshow(rgb_original)
axes[0, 0].set_title('1. Original Image', fontsize=14)
axes[0, 0].axis('off')

axes[0, 1].imshow(rgb_u128)
axes[0, 1].set_title('2. U Component = 128 (Missing Blue)', fontsize=14)
axes[0, 1].axis('off')

axes[1, 0].imshow(rgb_v128)
axes[1, 0].set_title('3. V Component = 128 (Missing Red)', fontsize=14)
axes[1, 0].axis('off')

axes[1, 1].imshow(rgb_gray)
axes[1, 1].set_title('4. U/V = 128 (Grayscale Image)', fontsize=14)
axes[1, 1].axis('off')

plt.tight_layout()
plt.show()
```

**代码解析：**
- U分量置128：丢失蓝色色度信息，画面整体偏绿色调
- V分量置128：丢失红色色度信息，画面整体偏青色调
- U/V都置128：完全丢失彩色信息，得到纯灰度图
- 实验清晰展示了U/V分量分别负责蓝色和红色的色度信息

---

### 实验4：调整Y分量亮度
**目标：** 修改Y分量的值，观察亮度变化，理解Y分量的作用。

**代码实现：**
```python
# 调整亮度（Y分量整体加减值）
def adjust_brightness(y, offset):
    y_new = y.astype(np.int16) + offset
    return np.clip(y_new, 0, 255).astype(np.uint8)

# 生成不同亮度版本
y_dark = adjust_brightness(y, -80)   # 变暗
y_bright = adjust_brightness(y, 80)  # 变亮

# 转换为RGB
rgb_dark = yuv420p_to_rgb(y_dark, u, v)
rgb_bright = yuv420p_to_rgb(y_bright, u, v)

# 可视化对比
fig, axes = plt.subplots(1, 3, figsize=(18, 6))

axes[0].imshow(rgb_dark)
axes[0].set_title('Brightness -80', fontsize=14)
axes[0].axis('off')

axes[1].imshow(rgb_original)
axes[1].set_title('Original Brightness', fontsize=14)
axes[1].axis('off')

axes[2].imshow(rgb_bright)
axes[2].set_title('Brightness +80', fontsize=14)
axes[2].axis('off')

plt.tight_layout()
plt.show()
```

**代码解析：**
- Y分量完全决定画面的亮度
- 增加Y值整体变亮，减少Y值整体变暗
- 色度分量不影响亮度信息

---

### 实验5：保存修改后的YUV帧
**目标：** 将修改后的YUV帧保存为图片，验证效果。

**代码实现：**
```python
import os

# 确保输出目录存在
output_dir = '../output'
os.makedirs(output_dir, exist_ok=True)

# 保存不同版本的图片
cv2.imwrite(os.path.join(output_dir, 'yuv_original.jpg'), cv2.cvtColor(rgb_original, cv2.COLOR_RGB2BGR))
cv2.imwrite(os.path.join(output_dir, 'yuv_u128.jpg'), cv2.cvtColor(rgb_u128, cv2.COLOR_RGB2BGR))
cv2.imwrite(os.path.join(output_dir, 'yuv_v128.jpg'), cv2.cvtColor(rgb_v128, cv2.COLOR_RGB2BGR))
cv2.imwrite(os.path.join(output_dir, 'yuv_gray.jpg'), cv2.cvtColor(rgb_gray, cv2.COLOR_RGB2BGR))

print(f"✅ 修改后的图片已保存到 {output_dir} 目录")
```

---

## 📊 预期结果与验收标准

### 预期输出
1. **分量可视化：**
   - Y分量是清晰的灰度图像，能完整呈现画面内容
   - U分量（蓝色）呈现蓝色调，蓝色区域（如天空）值较高
   - V分量（红色）呈现红色调，红色/肤色区域值较高

2. **分量修改效果：**
   - U=128：画面缺少蓝色，整体偏绿色，天空变成绿色
   - V=128：画面缺少红色，整体偏青色，红色物体变成青色
   - U/V=128：纯灰度图像，和原图的亮度完全一致

3. **亮度调整：**
   - 亮度-80：画面整体变暗，暗部细节丢失
   - 亮度+80：画面整体变亮，亮部细节过曝

### 验收标准
- [ ] 成功读取视频文件并解码第一帧
- [ ] 正确分离Y/U/V三个分量，尺寸符合预期
- [ ] 存储空间计算正确，YUV比RGB节省约50%空间
- [ ] 修改U/V分量后产生预期的色彩变化
- [ ] 理解Y分量控制亮度，U/V分量控制色度的原理

---

## ❌ 常见错误与排查

### 错误1：PyAV解码失败，找不到视频流
**原因：** 视频文件损坏或格式不支持
**排查：**
- 用ffplay验证视频文件能否正常播放
- 检查文件路径是否正确
- 确保PyAV版本支持视频的编码格式

### 错误2：U/V分量读取出来全是0或者乱码
**原因：** 像素格式不是YUV420P，或者平面索引错误
**排查：**
- 打印 `frame.format.name` 确认是yuv420p格式
- 确认平面索引：0=Y, 1=U, 2=V
- 检查步长line_size是否和宽度一致（有些视频有对齐填充）

### 错误3：转换后的RGB图像颜色偏色
**原因：** 转换标准不匹配（BT.601/BT.709混用）或U/V偏移错误
**排查：**
- 确认视频使用的色彩标准
- 确保U/V分量减去128后再参与计算
- 转换后使用`np.clip`裁剪到0-255范围

### 错误4：U/V分量尺寸计算错误
**原因：** 错误地认为U/V和Y的尺寸相同
**排查：**
- YUV420P的U/V宽高是Y的1/2
- 转换时需要对U/V进行上采样到Y的尺寸

---

## 🎯 课后小挑战

### 基础题：修改指定区域的色度
将画面中心200x200区域的U/V分量都置为128，让中心区域变成灰度，周围保持彩色。观察效果。

### 进阶题：手动实现YUV转RGB
不使用OpenCV的resize函数，手动实现U/V分量的上采样和YUV到RGB的转换。

### 思考题1：YUV422P的存储空间
YUV422P的采样比是4:2:2，水平方向色度下采样2倍，垂直方向不下采样。请问对于1920x1080的图像，YUV422P需要多少存储空间？比YUV420P多多少？

### 思考题2：半平面格式NV12
NV12是另一种常见的YUV格式，Y平面单独存储，U/V交错存储在同一个平面。请查找NV12的存储结构，思考它和YUV420P的区别和优缺点。

---

## 🎤 面试延伸题

### Q1：为什么视频编码都使用YUV而不是RGB？
**答题要点：**
1. YUV分离亮度和色度，利用人眼对色度不敏感的特性，可以对色度下采样
2. YUV420P比RGB节省50%的存储空间，大幅提升压缩效率
3. 历史兼容性：电视行业从模拟时代就使用YUV格式，延续至今

### Q2：YUV420P、YUV422P、YUV444P分别有什么特点，应用在什么场景？
**答题要点：**
- YUV444P：无下采样，画质最好，体积最大，用于专业视频制作
- YUV422P：水平下采样，垂直不采样，体积是RGB的2/3，用于广电行业
- YUV420P：水平垂直都下采样，体积最小，是互联网视频的主流格式

### Q3：什么是TV Range和Full Range？
**答题要点：**
- Full Range：Y/U/V的取值范围都是0-255，多用于计算机图形
- TV Range：Y范围16-235，U/V范围16-240，多用于广播电视
- 两者之间转换错误会导致画面发白或发黑

---

## 📚 延伸阅读
1. [ITU-R BT.601标准文档](https://www.itu.int/rec/R-REC-BT.601)
2. [ITU-R BT.709标准文档](https://www.itu.int/rec/R-REC-BT.709)
3. [PyAV官方文档 - Frame API](https://pyav.org/docs/stable/api/frame.html)
4. [FFmpeg像素格式文档](https://ffmpeg.org/doxygen/trunk/pixfmt_8h.html)

---

### 下节课预告
第3节：数字音频基础
- 学习PCM音频格式、采样率、位深度、声道等概念
- 绘制音频波形图和频谱图
- 理解音频数字化的原理和过程
