#include "common.h"

int cmd_remux(const Config& config) {
    AVFormatContext* ifmt_ctx = nullptr;
    AVFormatContext* ofmt_ctx = nullptr;
    AVPacket* pkt = nullptr;
    int ret = 0;

    // TODO: 1. 分配 Packet 并打开输入文件
    
    // TODO: 2. 创建输出封装上下文 (avformat_alloc_output_context2)

    // TODO: 3. 遍历输入流，为输出上下文创建对应的流 (avformat_new_stream)
    // 提示: 记得使用 avcodec_parameters_copy 拷贝参数，并将 codec_tag 置 0

    // TODO: 4. 打开输出文件 (avio_open) 并写入头信息 (avformat_write_header)

    // TODO: 5. 循环读取 Packet (av_read_frame)
    // 核心思路：
    // a. 换算 PTS/DTS/Duration (av_rescale_q_rnd)
    // b. 写入输出文件 (av_interleaved_write_frame)
    // c. 引用计数减一 (av_packet_unref)

    // TODO: 6. 写入文件尾 (av_write_trailer)

end:
    // TODO: 7. 释放所有资源
    return ret;
}
