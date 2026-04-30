# 《动手学音视频开发》

## 课程简介

本课程通过“理论 + 可运行代码 + 实验项目”的方式系统学习音视频技术，目标是帮助学习者从数据直觉、工具排障、FFmpeg API 到播放器与基础流媒体链路逐步建立工程能力。

课程默认方向为 `通用媒体工程`，主线聚焦媒体处理、播放器、转码与流媒体基础，扩展专题覆盖 WebRTC、实时采集和硬件编解码。

## 课程特色

- 🧪 **实验驱动**：每节课包含理论讲解、可运行实验和验收标准
- 📊 **可视化反馈**：前期使用 Python + Jupyter 建立图像、音频、视频数据直觉
- 🔧 **工程落地**：后期过渡到 C++17 + FFmpeg API + SDL2 实现工程代码
- 🧭 **排障导向**：贯穿 ffprobe、时间戳、像素格式、采样格式、资源释放和同步问题
- 📈 **渐进式学习**：从单帧/单音频处理到媒体工具、播放器，再到网络流媒体

## 目录结构

```text
./
├── README.md              # 项目总览
├── COURSE_BLUEPRINT.md    # 课程建设蓝图
├── PROGRESS.md            # 唯一开发进度真源
├── requirements.txt       # Python 依赖
├── docs/                  # 课程讲义
│   └── module1_basics/
├── notebooks/             # Jupyter 实验笔记本
│   └── module1/
├── src/                   # C++ 工程代码（后续模块）
├── assets/                # 素材清单与生成脚本；二进制素材默认不提交
└── output/                # 实验输出文件
```

## 快速开始

### 1. 安装依赖

```bash
pip install -r requirements.txt
```

### 2. 启动 Jupyter

```bash
jupyter notebook notebooks/
```

### 3. 开始学习

建议按顺序学习：

1. 打开 `docs/module1_basics/lesson01_pixel.md` 阅读第 1 节讲义
2. 运行 `notebooks/module1/01_pixel_exploration.ipynb`
3. 完成每节课的验收标准和课后挑战

## 课程大纲

课程结构为 `26 节主线课程 + 2 个扩展专题`。实际开发进度以 `PROGRESS.md` 为准。

### 模块一：音视频基础概念与数据可视化

- 第1节：数字图像基础 ✅ 已完成
- 第2节：视频的色彩空间 ✅ 已完成
- 第3节：数字音频基础 ✅ 已完成
- 第4节：音视频容器格式与编解码 ⏳ 待开发
- 第5节：FFmpeg CLI 与媒体排查基础 ⏳ 待开发
- 第6节：帧率、码率与时间戳 ⏳ 待开发
- 模块 checkpoint：媒体文件体检报告 ⏳ 待开发

### 模块二：音视频编解码核心原理

- 第7节：视频编码原理（DCT + 量化） ⏳ 待开发
- 第8节：H.264/H.265 关键概念 ⏳ 待开发
- 第9节：音频编码原理（AAC/MP3） ⏳ 待开发
- 第10节：FFmpeg 解码器 API 入门 ⏳ 待开发
- 第11节：FFmpeg 内存模型与引用计数 ⏳ 待开发
- 第12节：FFmpeg 编码器 API 入门 ⏳ 待开发

### 模块三：音视频解封装、封装与处理管道

- 第13节：解封装与封装 API 详解 ⏳ 待开发
- 第14节：解码管道构建 ⏳ 待开发
- 第15节：图像滤镜与转码 ⏳ 待开发
- 第16节：音频重采样与格式转换 ⏳ 待开发
- 第17节：转封装、流拷贝与精确剪辑 ⏳ 待开发
- 第18节：阶段项目一：mini_ffmpeg ⏳ 待开发

### 模块四：音视频播放与同步

- 第19节：SDL2 渲染基础 ⏳ 待开发
- 第20节：音频播放与回调机制 ⏳ 待开发
- 第21节：视频播放循环 ⏳ 待开发
- 第22节：线程模型与队列设计 ⏳ 待开发
- 第23节：音视频同步原理 ⏳ 待开发
- 第24节：播放控制与播放器排障 ⏳ 待开发

### 模块五：流媒体基础与结课项目

- 第25节：RTMP 推流与拉流 ⏳ 待开发
- 第26节：HLS 切片与基础直播链路 ⏳ 待开发

### 扩展专题

- 专题 A：WebRTC 基础架构 ⏳ 待开发
- 专题 B：实时音视频采集与直播实验 ⏳ 待开发

**📖 完整课程蓝图：** 查看 [COURSE_BLUEPRINT.md](./COURSE_BLUEPRINT.md) 获取详细课程设计、技术栈选型、阶段项目和实现规范。

## 学习建议

1. 按顺序学习，每节课完成后再进入下一节
2. 动手运行所有代码实验，观察输出结果
3. 优先理解工具输出和现象，再回到概念和原理
4. 完成课后小挑战，巩固所学知识
5. 遇到问题先查看讲义中的“常见错误与排查”部分

## 📚 课程开发指南

### 文档职责

- `COURSE_BLUEPRINT.md`：课程结构、能力目标、技术规范和阶段项目蓝图
- `PROGRESS.md`：唯一实际开发进度真源
- `README.md`：面向学习者的项目入口和简化进度
- `.opencode/skills/**`：AI Agent 的课程开发 SOP
- `.opencode/agents/reviewer.md`：课程内容和代码审查标准

### 对于课程开发者

开发新课程时请遵循以下流程：

1. **确认进度与蓝图**
   - 阅读 `COURSE_BLUEPRINT.md` 了解课程结构和实现规范
   - 阅读 `PROGRESS.md` 确认当前应该开发的节次
   - 检查 `README.md` 与 `PROGRESS.md` 是否存在状态冲突

2. **使用教学模板**
   - 参考 `COURSE_BLUEPRINT.md` 中的单节课教学模板
   - 确保包含：能力目标、工程场景、引言、理论讲解、环境准备、核心实验、验收标准、排障说明

3. **遵循代码规范**
   - Python 代码：注意 BGR/RGB 转换、可视化输出和轻量运行
   - C++ 代码：使用 C++17，检查 FFmpeg API 返回值，严格释放资源
   - 所有代码必须经过最小运行验证

4. **使用轻量化素材**
   - 视频素材：5-10 秒，360p/720p
   - 音频素材：5-10 秒，44.1kHz/48kHz
   - 实验目标运行时间小于 3 秒
   - 素材需记录来源、生成命令或许可证

5. **强制审查**
   - 新增或重写讲义、Notebook、C++ 示例工程后，必须经过 Reviewer 审查
   - BLOCKER 和 MAJOR 问题必须修复后再交付

6. **更新进度**
   - 每节课完成后更新 `README.md` 和 `PROGRESS.md`
   - 只有课程结构、模板、规范、阶段划分变化时才更新 `COURSE_BLUEPRINT.md`

7. **Git 提交规范**
   - 遵循 `CONTRIBUTING.md` 中的提交信息规范
   - 格式：`<type>(<scope>): <subject>`，例如 `feat(lesson): 添加第X节 - 课程标题`
   - 未经用户明确授权，不得执行 `git commit` 或 `git push`

### 关键注意事项

**内存管理（C++ 篇）：**

- FFmpeg API 必须正确释放资源，例如 `av_frame_free`、`av_packet_free`
- 循环复用对象时使用 `av_frame_unref`、`av_packet_unref`
- Debug 模式启用 ASAN 检测内存泄漏

**颜色空间转换：**

- OpenCV 默认 BGR，Matplotlib 默认 RGB
- YUV420P、NV12、RGB24、BGR24 要明确区分
- 图像显示前必须确认通道顺序和数据布局

**时间戳处理：**

- 所有 PTS/DTS 计算必须带 `time_base`
- C++ 阶段优先使用 `av_rescale_q` 和 `av_packet_rescale_ts`
- seek、重封装、同步逻辑必须记录关键时间戳日志

**错误处理：**

- FFmpeg API 调用必须检查返回值
- 使用 `av_strerror` 输出清晰错误信息
- 命令行工具和播放器应提供可观测日志

## 技术栈

- **基础实验篇**：Python + PyAV + OpenCV + NumPy + Matplotlib + Jupyter
- **工程实战篇**：C++17 + FFmpeg API + SDL2 + CMake + ASAN

## 📄 项目文档

### 核心文档

- [COURSE_BLUEPRINT.md](./COURSE_BLUEPRINT.md)：完整课程建设蓝图
- [PROGRESS.md](./PROGRESS.md)：课程开发进度追踪
- [CONTRIBUTING.md](./CONTRIBUTING.md)：Git 提交规范和开发指南
- [GIT_HOOKS.md](./GIT_HOOKS.md)：Git hooks 配置和使用说明

### Git 配置

- [.gitmessage](./.gitmessage)：提交信息模板文件

### 课程讲义

- [docs/module1_basics/lesson01_pixel.md](./docs/module1_basics/lesson01_pixel.md)：第1节课讲义
- [docs/module1_basics/lesson02_colorspace.md](./docs/module1_basics/lesson02_colorspace.md)：第2节课讲义
- [docs/module1_basics/lesson03_audio.md](./docs/module1_basics/lesson03_audio.md)：第3节课讲义

### 实验代码

- [notebooks/module1/01_pixel_exploration.ipynb](./notebooks/module1/01_pixel_exploration.ipynb)：第1节课实验笔记本
- [notebooks/module1/02_yuv_manipulation.ipynb](./notebooks/module1/02_yuv_manipulation.ipynb)：第2节课实验笔记本
- [notebooks/module1/03_audio_analysis.ipynb](./notebooks/module1/03_audio_analysis.ipynb)：第3节课实验笔记本

## 许可证

MIT License
