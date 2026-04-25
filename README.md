# 《动手学音视频开发》

## 课程简介
本课程通过"理论+可运行代码+实验"的方式系统学习音视频技术，打破传统学习曲线，从基础概念到工程实战。

课程默认方向为 `通用媒体工程`，主线聚焦媒体处理、播放器、转码与流媒体基础，扩展专题再覆盖 WebRTC、实时采集和硬件编解码。

## 课程特色
- 🧪 **实验驱动**：每节课包含理论讲解和动手实验
- 📊 **可视化反馈**：前期使用Python+Jupyter进行数据可视化
- 🔧 **工程落地**：后期过渡到C+++FFmpeg API实现工业级代码
- 📈 **渐进式学习**：从单帧处理到完整播放器，再到网络流媒体

## 目录结构
```
dive_into_media/
├── README.md              # 项目总览
├── requirements.txt       # Python依赖
├── docs/                  # 课程讲义
│   └── module1_basics/   # 模块一：基础概念
├── notebooks/             # Jupyter实验笔记本
│   └── module1/
├── src/                   # C++工程代码（后续模块）
├── assets/                # 测试素材（自动生成）
└── output/                # 实验输出文件
```

## 快速开始

### 1. 安装依赖
```bash
pip install -r requirements.txt
```

### 2. 启动Jupyter
```bash
jupyter notebook notebooks/
```

### 3. 开始学习
打开 `notebooks/module1/01_pixel_exploration.ipynb` 开始第一节课

## 课程大纲

课程结构分为 `26节主线课程 + 2个扩展专题`。

### 模块一：音视频基础概念与数据可视化
- 第1节：数字图像基础 ✅ 已完成
- 第2节：视频的色彩空间 ✅ 已完成
- 第3节：数字音频基础 ⏳ 待开发
- 第4节：音视频容器格式与编解码 ⏳ 待开发
- 第5节：FFmpeg CLI 与媒体排查基础 ⏳ 待开发
- 第6节：帧率、码率与时间戳 ⏳ 待开发

### 模块二：音视频编解码核心原理
- 第7节：视频编码原理（DCT + 量化） ⏳ 待开发
- 第8节：H.264/H.265 关键概念 ⏳ 待开发
- 第9节：音频编码原理（AAC/MP3） ⏳ 待开发
- 第10节：FFmpeg 解码器API入门 ⏳ 待开发
- 第11节：FFmpeg 内存模型与引用计数 ⏳ 待开发
- 第12节：FFmpeg 编码器API入门 ⏳ 待开发

### 模块三：音视频解封装、封装与处理管道
- 第13节：解封装与封装API详解 ⏳ 待开发
- 第14节：解码管道构建 ⏳ 待开发
- 第15节：图像滤镜与转码 ⏳ 待开发
- 第16节：音频重采样与格式转换 ⏳ 待开发
- 第17节：转封装、流拷贝与精确剪辑 ⏳ 待开发
- 第18节：阶段项目一：mini_ffmpeg ⏳ 待开发

### 模块四：音视频播放与同步
- 第19节：SDL2渲染基础 ⏳ 待开发
- 第20节：音频播放与回调机制 ⏳ 待开发
- 第21节：视频播放循环 ⏳ 待开发
- 第22节：线程模型与队列设计 ⏳ 待开发
- 第23节：音视频同步原理 ⏳ 待开发
- 第24节：播放控制与播放器排障 ⏳ 待开发

### 模块五：流媒体基础与结课项目
- 第25节：RTMP推流与拉流 ⏳ 待开发
- 第26节：HLS切片与基础直播链路 ⏳ 待开发

### 扩展专题
- 专题A：WebRTC基础架构 ⏳ 待开发
- 专题B：实时音视频采集与直播实验 ⏳ 待开发

**📖 完整课程蓝图：** 查看 [COURSE_BLUEPRINT.md](./COURSE_BLUEPRINT.md) 获取详细的课程设计、技术栈选型和实现规范。

## 学习建议
1. 按顺序学习，每节课完成后再进入下一节
2. 动手运行所有代码实验，观察输出结果
3. 完成课后小挑战，巩固所学知识
4. 遇到问题先查看代码注释和讲义说明

## 📚 课程开发指南

### 对于课程开发者

**开发新课程时请遵循以下流程：**

1. **查阅课程蓝图**
   - 阅读 `COURSE_BLUEPRINT.md` 了解完整课程设计
   - 确认当前课程的核心概念和实验内容

2. **使用教学模板**
   - 参考 `COURSE_BLUEPRINT.md` 中的单节课教学模板
   - 确保包含：能力目标、工程场景、引言、理论讲解、环境准备、核心实验、验收标准、排障说明

3. **遵循代码规范**
   - Python代码：使用PEP 8风格，添加详细注释
   - C++代码：使用现代C++17特性，注意内存管理
   - 所有代码必须经过测试验证

4. **使用轻量化素材**
   - 视频素材：5-10秒，360p/720p
   - 音频素材：5-10秒，44.1kHz/48kHz
   - 确保实验能在3秒内完成

5. **Git提交规范**
   - 遵循 `CONTRIBUTING.md` 中的提交信息规范
   - 格式：`<type>(<scope>): <subject>`（如 `feat(lesson): 添加第X节 - 课程标题`）
   - 每节课开发完成后提交一次

6. **更新进度**
   - 在README.md中更新课程状态
   - 在COURSE_BLUEPRINT.md中更新进度追踪表
   - 在PROGRESS.md中记录开发日志

### 关键注意事项

**内存管理（C++篇）：**
- FFmpeg API必须正确释放资源（av_frame_free, av_packet_free）
- 启用ASAN检测内存泄漏
- 参考COURSE_BLUEPRINT.md中的内存管理最佳实践

**颜色空间转换：**
- OpenCV使用BGR，matplotlib使用RGB
- YUV格式注意平面和半平面区别
- 时间戳处理注意time_base转换

**Git提交规范：**
- 遵循 `CONTRIBUTING.md` 中的提交信息规范
- 使用规范的提交类型和范围
- 提供详细的提交信息正文

**错误处理：**
- FFmpeg API调用必须检查返回值
- 使用av_strerror获取错误信息
- 提供清晰的错误提示

## 技术栈
- **基础实验篇**：Python + OpenCV + NumPy + Matplotlib + Jupyter
- **工程实战篇**：C++17 + FFmpeg API + SDL2 + CMake

## 📄 项目文档

### 核心文档
- [COURSE_BLUEPRINT.md](./COURSE_BLUEPRINT.md)：完整课程建设蓝图
- [PROGRESS.md](./PROGRESS.md)：课程开发进度追踪
- [CONTRIBUTING.md](./CONTRIBUTING.md)：Git提交规范和开发指南
- [GIT_HOOKS.md](./GIT_HOOKS.md)：Git hooks配置和使用说明

### Git配置
- [.gitmessage](./.gitmessage)：提交信息模板文件
- [.git/hooks/](./.git/hooks/)：Git hooks脚本目录

### 课程讲义
- [docs/module1_basics/lesson01_pixel.md](./docs/module1_basics/lesson01_pixel.md)：第1节课讲义
- [docs/module1_basics/lesson02_colorspace.md](./docs/module1_basics/lesson02_colorspace.md)：第2节课讲义

### 实验代码
- [notebooks/module1/01_pixel_exploration.ipynb](./notebooks/module1/01_pixel_exploration.ipynb)：第1节课实验笔记本
- [notebooks/module1/02_yuv_manipulation.ipynb](./notebooks/module1/02_yuv_manipulation.ipynb)：第2节课实验笔记本

## 许可证
MIT License
