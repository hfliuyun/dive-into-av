#include "common.h"

int cmd_transcode(const Config& config) {
    AVFormatContext* ifmt_ctx = nullptr;
    AVFormatContext* ofmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;
    AVCodecContext* enc_ctx = nullptr;
    AVFilterGraph* filter_graph = nullptr;
    AVFilterContext* buffersrc_ctx = nullptr;
    AVFilterContext* buffersink_ctx = nullptr;
    AVPacket* ipkt = av_packet_alloc();
    AVPacket* opkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* filt_frame = av_frame_alloc();
    int ret = 0;
    int video_stream_idx = -1;

    // TODO: 1. 打开输入并初始化解码器 (参考第10, 14节)

    // TODO: 2. 初始化编码器 (H.264)
    // 提示: 如果 config.scale 不为空，需要解析新的宽高

    // TODO: 3. 初始化输出封装上下文并写入头 (参考第13节)

    // TODO: 4. 初始化滤镜图 (参考第15节)
    // 核心思路：
    // a. 创建 buffer source 和 buffersink
    // b. 设置 scale 滤镜描述符
    // c. 解析并配置滤镜图 (avfilter_graph_parse_ptr)

    // TODO: 5. 转码循环
    // 核心思路：
    // a. 读取包 (av_read_frame)
    // b. 发送包到解码器 (avcodec_send_packet)
    // c. 接收解码后的帧 (avcodec_receive_frame)
    // d. 将帧送入滤镜 (av_buffersrc_add_frame_flags)
    // e. 从滤镜拉取处理后的帧 (av_buffersink_get_frame)
    // f. 将滤镜帧送入编码器 (avcodec_send_frame)
    // g. 接收编码后的包 (avcodec_receive_packet)
    // h. 写入输出文件 (av_interleaved_write_frame)

    // TODO: 6. 刷新滤镜和编码器 (Flush Pipeline)
    // 提示: 发送 NULL 帧到滤镜和编码器，确保所有缓存数据都被写出

    // TODO: 7. 写入文件尾

end:
    // TODO: 7. 彻底释放资源 (Context, Graph, Frame, Packet)
    return ret;
}
