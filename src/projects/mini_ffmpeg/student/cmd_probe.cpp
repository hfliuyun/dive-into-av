#include "common.h"

int cmd_probe(const Config& config) {
    AVFormatContext* fmt_ctx = nullptr;
    int ret = 0;

    // TODO: 1. 打开输入文件并初始化解封装上下文
    // 提示: 使用 avformat_open_input
    
    // TODO: 2. 读取流信息
    // 提示: 使用 avformat_find_stream_info

    std::cout << "File: " << config.input_file << "\n";
    std::cout << "Duration: " << fmt_ctx->duration / AV_TIME_BASE << "s\n";
    std::cout << "Number of streams: " << fmt_ctx->nb_streams << "\n";

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        AVCodecParameters* params = stream->codecpar;

        std::cout << "Stream #" << i << ": ";
        // TODO: 3. 判断流类型（视频/音频）并打印相关信息
        // 视频：宽、高、编码器名称、帧率
        // 音频：采样率、声道数、编码器名称
    }

end:
    // TODO: 4. 关闭输入并释放资源
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    return ret;
}
