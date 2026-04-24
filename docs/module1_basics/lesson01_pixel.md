# 第1节：数字图像基础

## 📚 引言

欢迎来到《动手学音视频开发》的第一节课！在这一节中，我们将从最基础的**像素**开始，理解数字图像的构成原理。

为什么从像素开始？因为无论是视频处理、图像滤镜，还是计算机视觉，所有操作本质上都是对像素数据的处理。理解了像素，你就理解了音视频开发的基石。

本节课你将学到：
- 什么是像素，以及如何访问和修改像素
- 分辨率、位深度等图像属性的含义
- RGB色彩模型的工作原理
- 如何用Python和OpenCV操作图像数据

---

## 🔬 理论讲解

### 1.1 像素的概念

**像素（Pixel）**是数字图像的最小单位。一幅数字图像可以看作是一个二维的像素矩阵，每个像素包含颜色信息。

对于彩色图像，每个像素通常由3个通道组成：
- **R（Red）**：红色分量
- **G（Green）**：绿色分量
- **B（Blue）**：蓝色分量

每个通道的值通常用0-255的整数表示（8位深度），其中：
- 0 表示该颜色通道完全关闭
- 255 表示该颜色通道完全开启

例如：
- 纯红色像素：(255, 0, 0)
- 纯绿色像素：(0, 255, 0)
- 纯蓝色像素：(0, 0, 255)
- 白色像素：(255, 255, 255)
- 黑色像素：(0, 0, 0)

### 1.2 分辨率与图像尺寸

**分辨率**指图像的宽度和高度，通常表示为 `宽度×高度`。

常见分辨率：
- 720p：1280×720（约92万像素）
- 1080p：1920×1080（约207万像素）
- 4K：3840×2160（约829万像素）

**总像素数** = 宽度 × 高度

例如，1280×720的图像总共有 921,600 个像素。

### 1.3 位深度与颜色范围

**位深度（Bit Depth）**决定每个颜色通道可以表示的颜色数量。

- **8位深度**：每个通道 2^8 = 256 个值（0-255）
- **16位深度**：每个通道 2^16 = 65,536 个值（0-65535）

对于RGB三通道图像：
- 8位深度：256 × 256 × 256 = 16,777,216 种颜色（约1677万色）
- 16位深度：65,536³ = 约2.8万亿种颜色

大多数图像和视频使用8位深度，因为人眼无法区分如此细微的颜色差异。

### 1.4 RGB色彩模型

**RGB（Red-Green-Blue）**是一种加色色彩模型，通过红、绿、蓝三种光的叠加来产生各种颜色。

**颜色混合原理：**
- 红 + 绿 = 黄色
- 红 + 蓝 = 品红
- 绿 + 蓝 = 青色
- 红 + 绿 + 蓝 = 白色

在计算机中，RGB通常以数组形式存储：`[R, G, B]`

**注意：OpenCV使用BGR顺序**
- OpenCV历史原因使用BGR而非RGB
- matplotlib使用RGB顺序
- 在两者之间转换时需要使用 `cv2.cvtColor(img, cv2.COLOR_BGR2RGB)`

### 1.5 NumPy数组表示图像

在Python中，图像用NumPy数组表示：

```python
import numpy as np

# 创建一个720p的RGB图像
height, width = 720, 1280
img = np.zeros((height, width, 3), dtype=np.uint8)

# 数组形状：(高度, 宽度, 通道数)
# 数据类型：uint8（无符号8位整数）
```

**重要：数组索引顺序是 `[row, col]` 而非 `[x, y]`**
- `img[row, col]` 访问第row行、第col列的像素
- `img[y, x]` 等同于 `img[row, col]`

---

## 🛠️ 环境与素材准备

### 安装依赖

确保已安装以下Python库：

```bash
pip install -r requirements.txt
```

或者手动安装：

```bash
pip install opencv-python numpy matplotlib pillow jupyter
```

### 测试素材

本节课使用OpenCV自动生成测试图像，无需下载外部文件。我们将生成一个720p（1280×720）的彩色测试卡，包含红、绿、蓝、白四个区域。

---

## 💻 核心代码实验

### 实验1：生成并显示测试图像

**目标：** 创建一个720p的彩色测试图像，并使用matplotlib显示。

**代码实现：**

```python
import cv2
import numpy as np
import matplotlib.pyplot as plt

def generate_test_image(width=1280, height=720):
    """
    生成一个四色测试图像
    - 左上：红色
    - 右上：绿色
    - 左下：蓝色
    - 右下：白色
    """
    img = np.zeros((height, width, 3), dtype=np.uint8)
    
    # 左上：红色
    img[0:height//2, 0:width//2] = [0, 0, 255]  # OpenCV使用BGR
    
    # 右上：绿色
    img[0:height//2, width//2:width] = [0, 255, 0]
    
    # 左下：蓝色
    img[height//2:height, 0:width//2] = [255, 0, 0]
    
    # 右下：白色
    img[height//2:height, width//2:width] = [255, 255, 255]
    
    return img

# 生成测试图像
img = generate_test_image()

# 转换为RGB格式供matplotlib显示
img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

# 显示图像
plt.figure(figsize=(12, 7))
plt.imshow(img_rgb)
plt.title("Test Image (1280x720)")
plt.axis('off')
plt.show()
```

**代码解析：**
- `np.zeros()` 创建全0数组（黑色）
- 数组切片 `img[0:height//2, 0:width//2]` 选择左上区域
- OpenCV使用BGR顺序，所以红色是 `[0, 0, 255]`
- `cv2.cvtColor()` 转换颜色空间

---

### 实验2：探索图像属性

**目标：** 理解图像的形状、数据类型和大小。

**代码实现：**

```python
print("=== 图像属性 ===")
print(f"图像形状: {img.shape}")
print(f"  - 高度: {img.shape[0]} 像素")
print(f"  - 宽度: {img.shape[1]} 像素")
print(f"  - 通道数: {img.shape[2]}")
print(f"\n数据类型: {img.dtype}")
print(f"每个像素大小: {img.itemsize} 字节")
print(f"\n总元素数: {img.size}")
print(f"总字节数: {img.nbytes}")
```

**预期输出：**
```
=== 图像属性 ===
图像形状: (720, 1280, 3)
  - 高度: 720 像素
  - 宽度: 1280 像素
  - 通道数: 3

数据类型: uint8
每个像素大小: 1 字节

总元素数: 2764800
总字节数: 2764800
```

**代码解析：**
- `img.shape` 返回 `(高度, 宽度, 通道数)`
- `img.dtype` 返回数据类型（uint8）
- `img.size` 返回总元素数（720×1280×3）
- `img.nbytes` 返回总字节数

---

### 实验3：访问单个像素

**目标：** 学习如何读取和修改单个像素的值。

**代码实现：**

```python
# 访问位置(100, 100)的像素
row, col = 100, 100
pixel_bgr = img[row, col]

print(f"=== 像素访问 ===")
print(f"位置({row}, {col})的像素值(BGR): {pixel_bgr}")
print(f"  - B(蓝色): {pixel_bgr[0]}")
print(f"  - G(绿色): {pixel_bgr[1]}")
print(f"  - R(红色): {pixel_bgr[2]}")

# 修改单个像素
img_modified = img.copy()
img_modified[row, col] = [255, 255, 0]  # 改为青色（BGR）

print(f"\n修改后的像素值: {img_modified[row, col]}")
```

**预期输出：**
```
=== 像素访问 ===
位置(100, 100)的像素值(BGR): [0 0 255]
  - B(蓝色): 0
  - G(绿色): 0
  - R(红色): 255

修改后的像素值: [255 255 0]
```

**代码解析：**
- `img[row, col]` 返回一个包含3个值的数组
- 注意索引顺序：先行后列
- 修改像素直接赋值新值

---

### 实验4：区域颜色修改

**目标：** 修改图像的某个区域，观察变化。

**代码实现：**

```python
# 创建副本用于修改
img_region = img.copy()

# 将左上角200x200区域改为黄色
region_size = 200
img_region[0:region_size, 0:region_size] = [0, 255, 255]  # 黄色（BGR）

# 将右下角300x300区域改为紫色
img_region[-300:, -300:] = [255, 0, 255]  # 紫色（BGR）

# 对比显示
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))

ax1.imshow(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
ax1.set_title("Original Image")
ax1.axis('off')

ax2.imshow(cv2.cvtColor(img_region, cv2.COLOR_BGR2RGB))
ax2.set_title("Modified Image (Regions)")
ax2.axis('off')

plt.tight_layout()
plt.show()
```

**代码解析：**
- `img[0:200, 0:200]` 选择左上角200×200区域
- `img[-300:, -300:]` 选择右下角300×300区域（负索引表示从末尾开始）
- `plt.subplots()` 创建并排显示的子图

---

### 实验5：RGB通道分离与可视化

**目标：** 分离RGB三个通道，单独显示每个通道。

**代码实现：**

```python
# 分离RGB通道
b_channel = img[:, :, 0]  # 蓝色通道
g_channel = img[:, :, 1]  # 绿色通道
r_channel = img[:, :, 2]  # 红色通道

print("=== 通道信息 ===")
print(f"蓝色通道形状: {b_channel.shape}")
print(f"绿色通道形状: {g_channel.shape}")
print(f"红色通道形状: {r_channel.shape}")

# 可视化各通道
fig, axes = plt.subplots(2, 2, figsize=(12, 12))

# 原始图像
axes[0, 0].imshow(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
axes[0, 0].set_title("Original Image")
axes[0, 0].axis('off')

# 红色通道（用红色调色板显示）
axes[0, 1].imshow(r_channel, cmap='Reds')
axes[0, 1].set_title("Red Channel")
axes[0, 1].axis('off')

# 绿色通道
axes[1, 0].imshow(g_channel, cmap='Greens')
axes[1, 0].set_title("Green Channel")
axes[1, 0].axis('off')

# 蓝色通道
axes[1, 1].imshow(b_channel, cmap='Blues')
axes[1, 1].set_title("Blue Channel")
axes[1, 1].axis('off')

plt.tight_layout()
plt.show()
```

**代码解析：**
- `img[:, :, 0]` 选择所有行、所有列的第0个通道（蓝色）
- `cmap='Reds'` 使用红色调色板显示单通道图像
- 单通道图像显示为灰度图，使用colormap可以增强可视化效果

---

### 实验6：保存修改后的图像

**目标：** 将修改后的图像保存到文件。

**代码实现：**

```python
import os

# 确保输出目录存在
output_dir = '../output'
os.makedirs(output_dir, exist_ok=True)

# 保存图像
output_path = os.path.join(output_dir, 'modified_image.jpg')
success = cv2.imwrite(output_path, img_region)

if success:
    print(f"✅ 图像已成功保存到: {output_path}")
    
    # 读取并验证
    saved_img = cv2.imread(output_path)
    print(f"✅ 保存的图像尺寸: {saved_img.shape}")
else:
    print(f"❌ 图像保存失败")
```

**预期输出：**
```
✅ 图像已成功保存到: ../output/modified_image.jpg
✅ 保存的图像尺寸: (720, 1280, 3)
```

**代码解析：**
- `os.makedirs()` 创建目录（如果不存在）
- `cv2.imwrite()` 保存图像到文件
- `cv2.imread()` 读取图像文件

---

## 📊 实验结果预期

完成所有实验后，你应该看到：

1. **测试图像显示**：一个四色（红绿蓝白）的720p图像
2. **图像属性输出**：显示尺寸、数据类型等信息
3. **像素值打印**：显示特定位置的RGB值
4. **区域修改对比**：原始图像和修改后的图像并排显示
5. **RGB通道分离**：四个子图显示原始图像和三个通道
6. **文件保存成功**：output目录下生成modified_image.jpg

---

## 🎯 课后小挑战

### 挑战1：创建渐变图像
创建一个720p图像，实现从左到右的红色渐变（从黑色到纯红色）。

**提示：** 使用双重循环或NumPy的广播功能。

### 挑战2：计算图像平均亮度
计算测试图像的平均亮度值（RGB三个通道的平均值）。

**提示：** 使用 `np.mean()` 函数。

### 挑战3：实现图像水平翻转
不使用OpenCV的翻转函数，手动实现图像的水平翻转。

**提示：** 使用数组切片 `img[:, ::-1]`。

### 挑战4：创建棋盘格图像
创建一个720p的棋盘格图像，每个格子100×100像素，黑白相间。

**提示：** 使用双重循环和取模运算。

---

## 📚 延伸阅读

### 推荐资源
- [OpenCV Python官方教程](https://docs.opencv.org/4.x/d6/d00/tutorial_py_root.html)
- [NumPy数组操作指南](https://numpy.org/doc/stable/user/quickstart.html)
- [数字图像处理基础](https://en.wikipedia.org/wiki/Digital_image_processing)

### 下节课预告
第2节：视频的色彩空间
- 学习YUV色彩空间
- 理解为什么视频使用YUV而非RGB
- 实现RGB与YUV的转换

---

## 🎓 总结

本节课我们学习了：
- ✅ 像素是数字图像的最小单位
- ✅ 分辨率、位深度等图像属性
- ✅ RGB色彩模型和颜色混合原理
- ✅ 使用NumPy数组表示和操作图像
- ✅ OpenCV与matplotlib的颜色空间差异
- ✅ 像素访问、修改和区域操作

掌握这些基础知识后，我们就可以进入更复杂的视频处理和编解码学习了！

**继续加油！下一节我们将探索视频的色彩空间！** 🚀
