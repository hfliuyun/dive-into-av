# 第23节：音视频同步原理 示例工程

本工程实现了播放器的终极目标：**音画同步**。

## 核心原理：视频同步到音频 (Sync to Audio)
1. **音频时钟**：以声卡实际消耗数据的进度作为全局参考时间。
2. **动态延时**：
   - 视频渲染前，比较 `Video PTS` 与 `Audio Clock`。
   - 若视频超前，则增加下一次定时器的等待时间。
   - 若视频滞后，则减小等待时间，实现追赶。

## 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## 准备测试素材
生成一个带有秒表计时和同步嘀嗒声的测试视频：
```bash
ffmpeg -f lavfi -i "testsrc=size=640x360:rate=25" -f lavfi -i "sine=frequency=1000:beep_factor=4" -c:v libx264 -c:a aac -shortest -y sync_test.mp4
```

## 运行
```bash
./lesson23_av_sync sync_test.mp4
```

## 观察重点
- 运行过程中，观察控制台输出的 `A-V Diff`。正常的同步状态下，该值应在 $0.01 \sim 0.04$ 秒之间小幅波动。
- 注意声音和画面中秒表跳动的合拍程度。
