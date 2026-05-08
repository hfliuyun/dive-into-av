# 第11节：FFmpeg 内存模型与引用计数

| 字段 | 内容 |
|------|------|
| lesson_id | 11 |
| module | module2_codec |
| type | cpp_ffmpeg |
| prerequisites | 第1-10节 |
| outputs | 理解 FFmpeg 内存模型，能用 ASAN 验证资源释放 |
| estimated_time | 3-4 小时 |
| runtime_limit | 实验运行 < 3 秒 |
| assets | test.mp4 |
| source | src/module2/lesson11_memory/ |

## 本节能力目标

- 理解 FFmpeg 内存模型：AVBuffer、AVBufferRef
- 理解引用计数的工作原理
- 能区分 `av_packet_unref()` 和 `av_packet_free()` 的使用场景
- 能用 ASAN 检测内存泄漏
- 能正确管理 FFmpeg 内存资源

## 真实工程对应场景

- 视频播放器：需要正确管理帧和包的内存
- 视频转码：大量帧和包的内存管理
- 视频编辑：帧的引用和共享
- 问题排查：内存泄漏是常见问题

## 引言

在第10节中，我们学习了 FFmpeg 解码器 API，并用 C++ 编写了一个最简视频解码器。但你是否想过：`av_frame_unref()` 和 `av_frame_free()` 有什么区别？为什么有时候程序会内存泄漏？

本节课我们将深入学习 **FFmpeg 的内存模型与引用计数**。这是 FFmpeg 内存管理的核心，也是编写健壮 C++ 程序的基础。

---

## 理论讲解

### 核心概念1：FFmpeg 内存模型概览

#### 1.1 AVBuffer

`AVBuffer` 是 FFmpeg 的底层内存缓冲区：

```c
typedef struct AVBuffer {
    uint8_t *data;        // 数据指针
    size_t size;          // 数据大小
    int refcount;         // 引用计数
    void (*free)(void *opaque, uint8_t *data);  // 释放回调
    void *opaque;         // 回调参数
} AVBuffer;
```

#### 1.2 AVBufferRef

`AVBufferRef` 是对 `AVBuffer` 的引用：

```c
typedef struct AVBufferRef {
    AVBuffer *buffer;     // 指向 AVBuffer
    uint8_t *data;        // 数据指针（与 buffer->data 相同）
    size_t size;          // 数据大小
} AVBufferRef;
```

#### 1.3 引用计数机制

```
AVBuffer (refcount=1)
    ↑
    │
AVBufferRef (ref1)
    │
    ├── AVBufferRef (ref2)  ← av_buffer_ref()
    │
    └── AVBufferRef (ref3)  ← av_buffer_ref()
```

- `av_buffer_ref()`：增加引用计数，返回新的 AVBufferRef
- `av_buffer_unref()`：减少引用计数，当计数为 0 时释放内存

---

### 核心概念2：引用计数的工作原理

#### 2.1 增加引用计数

```c
AVBufferRef *ref1 = av_buffer_alloc(1024);  // refcount = 1
AVBufferRef *ref2 = av_buffer_ref(ref1);    // refcount = 2
AVBufferRef *ref3 = av_buffer_ref(ref1);    // refcount = 3
```

#### 2.2 减少引用计数

```c
av_buffer_unref(&ref3);  // refcount = 2
av_buffer_unref(&ref2);  // refcount = 1
av_buffer_unref(&ref1);  // refcount = 0，释放内存
```

#### 2.3 引用计数为 0 时的行为

当引用计数为 0 时：
1. 调用 `free` 回调函数
2. 释放 `data` 指向的内存
3. 释放 `AVBuffer` 结构体本身

---

### 核心概念3：AVPacket 和 AVFrame 的内存管理

#### 3.1 AVPacket 的内存管理

```c
// 分配
AVPacket *packet = av_packet_alloc();

// 读取数据
av_read_frame(fmt_ctx, packet);

// 使用数据
process_packet(packet);

// 清空数据（减少引用计数）
av_packet_unref(packet);

// 释放
av_packet_free(&packet);
```

#### 3.2 AVFrame 的内存管理

```c
// 分配
AVFrame *frame = av_frame_alloc();

// 解码
avcodec_receive_frame(codec_ctx, frame);

// 使用数据
process_frame(frame);

// 清空数据（减少引用计数）
av_frame_unref(frame);

// 释放
av_frame_free(&frame);
```

#### 3.3 AVFrame 的引用计数

AVFrame 支持引用计数，可以通过 `av_frame_clone()` 增加引用：

```c
// 创建 frame1
AVFrame *frame1 = av_frame_alloc();
frame1->width = 640;
frame1->height = 360;
frame1->format = AV_PIX_FMT_YUV420P;
av_frame_get_buffer(frame1, 0);  // 分配数据缓冲区，refcount = 1

// 克隆 frame1（增加引用计数）
AVFrame *frame2 = av_frame_clone(frame1);  // refcount = 2

// 清空 frame2（减少引用计数）
av_frame_unref(frame2);  // refcount = 1

// 释放 frame1（释放内存）
av_frame_free(&frame1);  // refcount = 0，释放内存
```

**注意**：`av_frame_clone()` 会增加引用计数，而 `av_frame_copy()` 会复制数据。

#### 3.4 unref vs free

| 操作 | 作用 | 使用场景 |
|------|------|----------|
| `av_packet_unref()` | 清空数据，减少引用计数 | 循环中复用 packet |
| `av_packet_free()` | 释放 packet 结构体本身 | 程序结束时 |
| `av_frame_unref()` | 清空数据，减少引用计数 | 循环中复用 frame |
| `av_frame_free()` | 释放 frame 结构体本身 | 程序结束时 |
| `av_frame_clone()` | 增加引用计数 | 共享帧数据 |

**关键区别**：
- `unref`：清空数据，但保留结构体，可以复用
- `free`：释放结构体本身，不能再使用
- `clone`：增加引用计数，共享数据

---

## 关键点总结

| 要点 | 一句话总结 |
|------|-----------|
| **AVBuffer 是底层缓冲区** | 包含数据指针、大小和引用计数 |
| **AVBufferRef 是引用** | 指向 AVBuffer，可以有多个引用 |
| **引用计数管理内存** | 增加引用时 +1，减少引用时 -1，为 0 时释放 |
| **unref 清空数据** | 减少引用计数，但保留结构体 |
| **free 释放结构体** | 释放结构体本身，不能再使用 |
| **ASAN 检测泄漏** | 编译时启用 ASAN，运行时检测内存泄漏 |

---

## 环境与素材准备

### 环境配置

**macOS：**
```bash
brew install ffmpeg cmake
```

**Fedora：**
```bash
sudo dnf install ffmpeg-devel cmake gcc-c++
```

**Ubuntu/Debian：**
```bash
sudo apt install libavcodec-dev libavformat-dev libavutil-dev cmake g++
```

### 验证 ASAN 支持

```bash
# 检查编译器是否支持 ASAN
g++ -fsanitize=address -x c++ -c /dev/null -o /dev/null && echo "ASAN 支持"
```

### 测试素材

```bash
# 生成测试视频
ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=25 \
       -c:v libx264 -y test.mp4
```

---

## 核心代码实验

### 实验1：引用计数演示

**目标**：演示引用计数的工作原理

**代码实现：**
```cpp
// src/module2/lesson11_memory/main.cpp
#include <iostream>
#include <cstdio>

extern "C" {
#include <libavutil/buffer.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

int main() {
    std::cout << "=== 引用计数演示 ===" << std::endl;

    // 1. 创建 AVBufferRef
    AVBufferRef *ref1 = av_buffer_alloc(1024);
    if (!ref1) {
        std::cerr << "分配 AVBufferRef 失败" << std::endl;
        return 1;
    }
    std::cout << "创建 ref1: refcount=" << ref1->buffer->refcount << std::endl;

    // 2. 增加引用
    AVBufferRef *ref2 = av_buffer_ref(ref1);
    std::cout << "创建 ref2: refcount=" << ref1->buffer->refcount << std::endl;

    AVBufferRef *ref3 = av_buffer_ref(ref1);
    std::cout << "创建 ref3: refcount=" << ref1->buffer->refcount << std::endl;

    // 3. 减少引用
    av_buffer_unref(&ref3);
    std::cout << "释放 ref3: refcount=" << ref1->buffer->refcount << std::endl;

    av_buffer_unref(&ref2);
    std::cout << "释放 ref2: refcount=" << ref1->buffer->refcount << std::endl;

    av_buffer_unref(&ref1);
    std::cout << "释放 ref1: 内存已释放" << std::endl;

    return 0;
}
```

**代码解析：**
- `av_buffer_alloc()`：分配 AVBufferRef，refcount = 1
- `av_buffer_ref()`：增加引用计数，返回新的 AVBufferRef
- `av_buffer_unref()`：减少引用计数，为 0 时释放内存

---

### 实验2：内存泄漏检测

**目标**：使用 ASAN 检测内存泄漏

**代码实现：**
```cpp
// src/module2/lesson11_memory/leak_test.cpp
#include <iostream>
#include <cstdio>

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
}

void leak_memory() {
    std::cout << "故意泄漏内存..." << std::endl;

    // 故意不释放 frame
    AVFrame *frame = av_frame_alloc();
    if (frame) {
        std::cout << "分配了 AVFrame，但没有释放" << std::endl;
        // 故意不调用 av_frame_free(&frame)
    }
}

void no_leak() {
    std::cout << "正确释放内存..." << std::endl;

    AVFrame *frame = av_frame_alloc();
    if (frame) {
        std::cout << "分配了 AVFrame" << std::endl;
        av_frame_free(&frame);
        std::cout << "释放了 AVFrame" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <leak|no_leak>" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "leak") {
        leak_memory();
    } else if (mode == "no_leak") {
        no_leak();
    } else {
        std::cerr << "未知模式: " << mode << std::endl;
        return 1;
    }

    std::cout << "程序结束" << std::endl;
    return 0;
}
```

**代码解析：**
- `leak_memory()`：故意不释放 frame，ASAN 会报错
- `no_leak()`：正确释放 frame，ASAN 不会报错

---

### 实验3：正确的资源释放

**目标**：演示正确的资源释放方式

**代码实现：**
```cpp
// 正确的解码器资源释放
void correct_cleanup() {
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr;

    // 分配资源
    packet = av_packet_alloc();
    frame = av_frame_alloc();

    // ... 使用资源 ...

    // 正确释放资源（顺序很重要）
    av_frame_free(&frame);      // 先释放 frame
    av_packet_free(&packet);    // 再释放 packet
    avcodec_free_context(&codec_ctx);  // 释放解码器上下文
    avformat_close_input(&fmt_ctx);    // 关闭输入
}
```

**代码解析：**
- 释放顺序：frame → packet → codec_ctx → fmt_ctx
- 使用 `av_xxx_free()` 释放，而不是 `delete`
- 使用 `&` 传递指针的指针

---

## 预期结果与验收标准

### 预期输出

```text
=== 引用计数演示 ===
创建 ref1: refcount=1
创建 ref2: refcount=2
创建 ref3: refcount=3
释放 ref3: refcount=2
释放 ref2: refcount=1
释放 ref1: 内存已释放
```

### ASAN 检测结果

```text
# 运行泄漏测试
./leak_test leak

=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 1024 byte(s) in 1 object(s) allocated from:
    #0 0x... in malloc
    #1 0x... in av_frame_alloc
    #2 0x... in leak_memory()
    ...

SUMMARY: AddressSanitizer: 1024 byte(s) leaked in 1 allocation(s).
```

### 验收标准

- [ ] 能理解 AVBuffer 和 AVBufferRef 的作用
- [ ] 能理解引用计数的工作原理
- [ ] 能区分 unref 和 free 的使用场景
- [ ] 能用 ASAN 检测内存泄漏
- [ ] 能正确管理 FFmpeg 内存资源

---

## 常见错误与排查

### 错误1：ASAN 报错 "use-after-free"

**原因**：释放后继续使用指针。

**排查**：
- 检查是否在 `av_frame_free()` 后继续使用 frame
- 检查是否在 `av_packet_free()` 后继续使用 packet

### 错误2：ASAN 报错 "double-free"

**原因**：重复释放同一资源。

**排查**：
- 检查是否对同一 frame 调用两次 `av_frame_free()`
- 检查是否对同一 packet 调用两次 `av_packet_free()`

### 错误3：ASAN 报错 "memory leak"

**原因**：忘记释放资源。

**排查**：
- 检查所有 `av_xxx_alloc()` 是否有对应的 `av_xxx_free()`
- 检查循环中是否正确使用 `av_xxx_unref()`

### 错误4：程序崩溃 "segmentation fault"

**原因**：访问已释放的内存。

**排查**：
- 检查是否在 `av_frame_free()` 后访问 frame
- 检查指针是否初始化为 nullptr

---

## 课后小挑战

### 基础题：修改解码器，正确释放资源

修改第10节的解码器代码，确保所有资源正确释放，ASAN 不报错。

### 进阶题：实现引用计数跟踪

编写一个工具，跟踪 FFmpeg 对象的引用计数变化，输出日志。

### 思考题1：为什么 FFmpeg 使用引用计数而不是拷贝？

提示：考虑视频帧的大小和拷贝的开销。

### 思考题2：什么时候应该使用 unref，什么时候应该使用 free？

提示：考虑对象的生命周期和复用需求。

---

## 面试延伸题

### Q1：FFmpeg 的引用计数机制是什么？

**答题要点**：
1. AVBuffer 包含数据和引用计数
2. AVBufferRef 是对 AVBuffer 的引用
3. `av_buffer_ref()` 增加引用计数
4. `av_buffer_unref()` 减少引用计数，为 0 时释放内存

### Q2：unref 和 free 有什么区别？

**答题要点**：
1. `unref`：清空数据，减少引用计数，但保留结构体
2. `free`：释放结构体本身，不能再使用
3. 循环中使用 `unref` 复用对象
4. 程序结束时使用 `free` 释放对象

### Q3：如何用 ASAN 检测内存泄漏？

**答题要点**：
1. 编译时启用 ASAN：`-fsanitize=address`
2. 运行程序，ASAN 会自动检测内存泄漏
3. 泄漏时输出分配堆栈和大小
4. 修复后重新编译运行

---

## 延伸阅读

1. [FFmpeg 内存管理](https://ffmpeg.org/doxygen/trunk/group__lavu__buffer.html)
2. [AddressSanitizer 文档](https://github.com/google/sanitizers/wiki/AddressSanitizer)
3. [FFmpeg 内存泄漏检测](https://ffmpeg.org/doxygen/trunk/group__lavu__mem.html)

---

### 下节课预告

第12节：FFmpeg 编码器 API 入门
- 学习编码参数、码流输出、flush 流程
- 将 YUV 序列编码为 H.264 文件
- 进阶挑战：尝试开启硬件编码器
