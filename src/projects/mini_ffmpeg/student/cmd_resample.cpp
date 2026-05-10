#include "common.h"

int cmd_resample(const Config& config) {
    AVFormatContext* ifmt_ctx = nullptr;
    AVFormatContext* ofmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;
    AVCodecContext* enc_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;
    AVPacket* ipkt = av_packet_alloc();
    AVPacket* opkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* res_frame = av_frame_alloc();
    int ret = 0;
    int audio_stream_idx = -1;

    // TODO: 1. 初始化解码器 (参考第10, 14节)

    // TODO: 2. 初始化编码器 (AAC)
    // 提示: 如果 config.sample_rate/channels 不为 0，设置新参数

    // TODO: 3. 初始化重采样上下文 SwrContext (参考第16节)

    // TODO: 4. 初始化输出封装上下文

    // TODO: 5. 处理循环
    // 核心思路：
    // a. 解码音频帧
    // b. 计算输出样本数 (swr_get_out_samples) 并分配缓冲
    // c. 转换音频 (swr_convert)
    // d. 编码重采样后的帧并写入输出文件

    // TODO: 6. 刷新编码器 (Flush Encoder)

    // TODO: 7. 写入文件尾

end:
    // TODO: 7. 释放资源 (SwrContext, Encoder, Decoder, etc.)
    return ret;
}
