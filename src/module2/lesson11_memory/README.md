# 第11节：FFmpeg 内存模型与引用计数

## 简介

本项目演示了 FFmpeg 的内存模型与引用计数机制，并使用 ASAN 检测内存泄漏。

## 项目结构

```
lesson11_memory/
├── CMakeLists.txt       # CMake 配置
├── main.cpp             # 引用计数演示程序
├── leak_test.cpp        # 内存泄漏测试程序
└── README.md            # 本文件
```

## 环境配置

### macOS

```bash
brew install ffmpeg cmake
```

### Fedora

```bash
sudo dnf install ffmpeg-devel cmake gcc-c++
```

### Ubuntu/Debian

```bash
sudo apt install libavcodec-dev libavformat-dev libavutil-dev cmake g++
```

### 验证 ASAN 支持

```bash
g++ -fsanitize=address -x c++ -c /dev/null -o /dev/null && echo "ASAN 支持"
```

## 编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置（Debug 模式，启用 ASAN）
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 编译
make
```

## 运行

### 引用计数演示

```bash
./refcount_demo
```

预期输出：

```text
=== 引用计数演示 ===
创建 ref1: refcount=1
创建 ref2: refcount=2
创建 ref3: refcount=3
释放 ref3: refcount=2
释放 ref2: refcount=1
释放 ref1: 内存已释放

=== AVFrame 引用计数演示 ===
创建 frame1: refcount=1
克隆 frame2: refcount=2
清空 frame2: refcount=1
释放 frame1: 内存已释放

=== 演示完成 ===
```

### 内存泄漏测试

```bash
# 测试内存泄漏（ASAN 会报错）
./leak_test leak

# 测试 packet 泄漏（ASAN 会报错）
./leak_test leak_packet

# 测试正确释放（ASAN 不会报错）
./leak_test no_leak

# 测试重复释放（ASAN 会报错）
./leak_test double_free

# 测试使用已释放内存（ASAN 会报错）
./leak_test use_after_free
```

## ASAN 输出示例

### 内存泄漏

```text
=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 1024 byte(s) in 1 object(s) allocated from:
    #0 0x... in malloc
    #1 0x... in av_frame_alloc
    #2 0x... in leak_memory()
    ...

SUMMARY: AddressSanitizer: 1024 byte(s) leaked in 1 allocation(s).
```

### 重复释放

```text
=================================================================
==12345==ERROR: AddressSanitizer: attempting double-free on 0x...
    #0 0x... in free
    #1 0x... in av_frame_free
    #2 0x... in double_free()
    ...
```

### 使用已释放内存

```text
=================================================================
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x...
    #0 0x... in use_after_free()
    ...
```

## 关键概念

### AVBuffer 和 AVBufferRef

- `AVBuffer`：底层内存缓冲区，包含数据指针、大小和引用计数
- `AVBufferRef`：对 AVBuffer 的引用，可以有多个引用

### 引用计数

- `av_buffer_ref()`：增加引用计数
- `av_buffer_unref()`：减少引用计数，为 0 时释放内存

### unref vs free

| 操作 | 作用 | 使用场景 |
|------|------|----------|
| `av_packet_unref()` | 清空数据，减少引用计数 | 循环中复用 packet |
| `av_packet_free()` | 释放 packet 结构体本身 | 程序结束时 |
| `av_frame_unref()` | 清空数据，减少引用计数 | 循环中复用 frame |
| `av_frame_free()` | 释放 frame 结构体本身 | 程序结束时 |

## 常见问题

### ASAN 报错 "use-after-free"

原因：释放后继续使用指针。

检查是否在 `av_frame_free()` 后继续使用 frame。

### ASAN 报错 "double-free"

原因：重复释放同一资源。

检查是否对同一 frame 调用两次 `av_frame_free()`。

### ASAN 报错 "memory leak"

原因：忘记释放资源。

检查所有 `av_xxx_alloc()` 是否有对应的 `av_xxx_free()`。
