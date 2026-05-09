#include <iostream>
#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <input.h264> <output.mp4>" << std::endl;
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    int ret = 0;

    // 1. 打开输入（H.264 裸流）
    AVFormatContext *ifmt_ctx = NULL;
    ret = avformat_open_input(&ifmt_ctx, input_file, NULL, NULL);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "打开输入文件失败: " << err_buf << std::endl;
        return 1;
    }

    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "查找流信息失败: " << err_buf << std::endl;
        avformat_close_input(&ifmt_ctx);
        return 1;
    }

    std::cout << "输入信息:" << std::endl;
    std::cout << "  格式: " << ifmt_ctx->iformat->name << std::endl;
    std::cout << "  流数量: " << ifmt_ctx->nb_streams << std::endl;

    for (unsigned i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream = ifmt_ctx->streams[i];
        std::cout << "  流 " << i << ": "
                  << av_get_media_type_string(stream->codecpar->codec_type)
                  << " (" << avcodec_get_name(stream->codecpar->codec_id) << ")"
                  << std::endl;
    }
    std::cout << std::endl;

    // 2. 创建输出上下文
    AVFormatContext *ofmt_ctx = NULL;
    ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output_file);
    if (ret < 0) {
        std::cerr << "创建输出上下文失败" << std::endl;
        avformat_close_input(&ifmt_ctx);
        return 1;
    }

    // 3. 添加输出流
    for (unsigned i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            std::cerr << "添加输出流失败" << std::endl;
            avformat_close_input(&ifmt_ctx);
            avformat_free_context(ofmt_ctx);
            return 1;
        }

        // 复制编解码参数
        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        if (ret < 0) {
            std::cerr << "复制编解码参数失败" << std::endl;
            avformat_close_input(&ifmt_ctx);
            avformat_free_context(ofmt_ctx);
            return 1;
        }
        out_stream->codecpar->codec_tag = 0;
    }

    // 4. 打开输出文件
    ret = avio_open(&ofmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "打开输出文件失败: " << err_buf << std::endl;
        avformat_close_input(&ifmt_ctx);
        avformat_free_context(ofmt_ctx);
        return 1;
    }

    // 5. 写入文件头
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "写入文件头失败: " << err_buf << std::endl;
        avio_closep(&ofmt_ctx->pb);
        avformat_close_input(&ifmt_ctx);
        avformat_free_context(ofmt_ctx);
        return 1;
    }

    // 6. 读取包并写入输出
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "分配 AVPacket 失败" << std::endl;
        avio_closep(&ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
        avformat_close_input(&ifmt_ctx);
        return 1;
    }
    int packet_count = 0;

    std::cout << "开始封装..." << std::endl;
    std::cout << "  输入: " << input_file << std::endl;
    std::cout << "  输出: " << output_file << std::endl;
    std::cout << std::endl;

    while (av_read_frame(ifmt_ctx, packet) >= 0) {
        AVStream *in_stream = ifmt_ctx->streams[packet->stream_index];
        AVStream *out_stream = ofmt_ctx->streams[packet->stream_index];

        // 转换时间基
        packet->pts = av_rescale_q(packet->pts, in_stream->time_base, out_stream->time_base);
        packet->dts = av_rescale_q(packet->dts, in_stream->time_base, out_stream->time_base);
        packet->duration = av_rescale_q(packet->duration, in_stream->time_base, out_stream->time_base);
        packet->pos = -1;

        // 写入包
        ret = av_interleaved_write_frame(ofmt_ctx, packet);
        if (ret < 0) {
            char err_buf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, err_buf, sizeof(err_buf));
            std::cerr << "写入包失败: " << err_buf << std::endl;
            break;
        }

        packet_count++;
        av_packet_unref(packet);
    }

    // 7. 写入文件尾
    av_write_trailer(ofmt_ctx);

    std::cout << std::endl;
    std::cout << "封装完成: " << packet_count << " 个包" << std::endl;
    std::cout << "输出文件: " << output_file << std::endl;
    std::cout << "验证命令: ffprobe " << output_file << std::endl;

    // 8. 释放资源
    av_packet_free(&packet);
    avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    avformat_close_input(&ifmt_ctx);

    return 0;
}
