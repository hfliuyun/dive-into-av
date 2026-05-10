#ifndef MINI_FFMPEG_COMMON_H
#define MINI_FFMPEG_COMMON_H

#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswresample/swresample.h>
}

// 统一错误处理宏
#define CHECK_RET(ret, msg) \
    if (ret < 0) { \
        char err_buf[AV_ERROR_MAX_STRING_SIZE]; \
        av_strerror(ret, err_buf, sizeof(err_buf)); \
        std::cerr << msg << ": " << err_buf << " (code: " << ret << ")\n"; \
        goto end; \
    }

// 命令行参数结构体
struct Config {
    std::string input_file;
    std::string output_file;
    std::string scale; // e.g. "1280x720"
    int sample_rate = 0;
    int channels = 0;
};

// 子命令处理函数声明
int cmd_probe(const Config& config);
int cmd_remux(const Config& config);
int cmd_transcode(const Config& config);
int cmd_resample(const Config& config);

#endif // MINI_FFMPEG_COMMON_H
