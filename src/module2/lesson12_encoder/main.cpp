#include <iostream>
#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <input.yuv> <output.h264>" << std::endl;
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    int ret = 0;

    // 1. 查找编码器
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "未找到 H.264 编码器" << std::endl;
        return 1;
    }

    // 2. 创建编码器上下文
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "创建编码器上下文失败" << std::endl;
        return 1;
    }

    // 3. 设置编码参数
    codec_ctx->width = 640;
    codec_ctx->height = 360;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->time_base = (AVRational){1, 25};
    codec_ctx->framerate = (AVRational){25, 1};
    codec_ctx->bit_rate = 500000;
    codec_ctx->gop_size = 10;
    codec_ctx->max_b_frames = 2;

    // 设置 preset
    av_opt_set(codec_ctx->priv_data, "preset", "medium", 0);

    // 4. 打开编码器
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "打开编码器失败: " << err_buf << std::endl;
        avcodec_free_context(&codec_ctx);
        return 1;
    }

    // 5. 打开输入文件
    FILE *yuv_file = fopen(input_file, "rb");
    if (!yuv_file) {
        std::cerr << "打开输入文件失败: " << input_file << std::endl;
        avcodec_free_context(&codec_ctx);
        return 1;
    }

    // 6. 打开输出文件
    FILE *h264_file = fopen(output_file, "wb");
    if (!h264_file) {
        std::cerr << "打开输出文件失败: " << output_file << std::endl;
        fclose(yuv_file);
        avcodec_free_context(&codec_ctx);
        return 1;
    }

    // 7. 分配 frame 和 packet
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    if (!frame || !packet) {
        std::cerr << "分配 frame/packet 失败" << std::endl;
        av_frame_free(&frame);
        av_packet_free(&packet);
        fclose(yuv_file);
        fclose(h264_file);
        avcodec_free_context(&codec_ctx);
        return 1;
    }

    // 设置 frame 参数
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    frame->format = codec_ctx->pix_fmt;

    // 8. 编码循环
    int frame_count = 0;
    int y_size = codec_ctx->width * codec_ctx->height;
    int uv_size = y_size / 4;

    std::cout << "编码参数:" << std::endl;
    std::cout << "  分辨率: " << codec_ctx->width << "x" << codec_ctx->height << std::endl;
    std::cout << "  帧率: " << codec_ctx->framerate.num << "/" << codec_ctx->framerate.den << std::endl;
    std::cout << "  码率: " << codec_ctx->bit_rate << " bps" << std::endl;
    std::cout << "  GOP: " << codec_ctx->gop_size << std::endl;
    std::cout << std::endl;

    while (true) {
        // 读取 YUV 数据
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            std::cerr << "分配 frame 缓冲区失败" << std::endl;
            break;
        }

        // 读取 Y 分量
        if (fread(frame->data[0], 1, y_size, yuv_file) != (size_t)y_size) break;
        // 读取 U 分量
        if (fread(frame->data[1], 1, uv_size, yuv_file) != (size_t)uv_size) break;
        // 读取 V 分量
        if (fread(frame->data[2], 1, uv_size, yuv_file) != (size_t)uv_size) break;

        frame->pts = frame_count++;

        // 发送帧到编码器
        ret = avcodec_send_frame(codec_ctx, frame);
        if (ret < 0) {
            char err_buf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, err_buf, sizeof(err_buf));
            std::cerr << "发送帧失败: " << err_buf << std::endl;
            break;
        }

        // 接收编码后的包
        while (avcodec_receive_packet(codec_ctx, packet) >= 0) {
            fwrite(packet->data, 1, packet->size, h264_file);
            av_packet_unref(packet);
        }

        av_frame_unref(frame);
    }

    // 9. flush 编码器
    std::cout << "\n--- flush 编码器 ---" << std::endl;
    ret = avcodec_send_frame(codec_ctx, NULL);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "flush 编码器失败: " << err_buf << std::endl;
    } else {
        while (avcodec_receive_packet(codec_ctx, packet) >= 0) {
            fwrite(packet->data, 1, packet->size, h264_file);
            av_packet_unref(packet);
        }
    }

    std::cout << "\n编码完成: " << frame_count << " 帧" << std::endl;
    std::cout << "输出文件: " << output_file << std::endl;
    std::cout << "验证命令: ffplay " << output_file << std::endl;

    // 10. 释放资源
    av_frame_free(&frame);
    av_packet_free(&packet);
    fclose(yuv_file);
    fclose(h264_file);
    avcodec_free_context(&codec_ctx);

    return 0;
}
