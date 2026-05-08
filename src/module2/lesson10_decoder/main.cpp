#include <iostream>
#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

// 保存 YUV 帧到文件
static void save_yuv_frame(AVFrame *frame, FILE *output_file) {
    // 保存 Y 分量
    for (int i = 0; i < frame->height; i++) {
        fwrite(frame->data[0] + i * frame->linesize[0], 1, frame->width, output_file);
    }
    // 保存 U 分量
    for (int i = 0; i < frame->height / 2; i++) {
        fwrite(frame->data[1] + i * frame->linesize[1], 1, frame->width / 2, output_file);
    }
    // 保存 V 分量
    for (int i = 0; i < frame->height / 2; i++) {
        fwrite(frame->data[2] + i * frame->linesize[2], 1, frame->width / 2, output_file);
    }
}

// 打印帧信息
static void print_frame_info(int frame_count, AVFrame *frame) {
    std::cout << "帧 " << frame_count
              << ": PTS=" << frame->pts
              << ", 尺寸=" << frame->width << "x" << frame->height
              << ", YUV大小=" << av_image_get_buffer_size(
                     (AVPixelFormat)frame->format, frame->width, frame->height, 1)
              << " 字节" << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <input.mp4> [output.yuv]" << std::endl;
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = (argc >= 3) ? argv[2] : nullptr;
    int ret = 0;

    // 1. 打开输入文件
    AVFormatContext *fmt_ctx = nullptr;
    ret = avformat_open_input(&fmt_ctx, input_file, nullptr, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "打开输入文件失败: " << err_buf << std::endl;
        return 1;
    }

    // 2. 查找流信息
    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        std::cerr << "查找流信息失败" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 3. 查找视频流
    int video_stream_idx = -1;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    if (video_stream_idx == -1) {
        std::cerr << "未找到视频流" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 4. 查找解码器
    AVCodecParameters *codecpar = fmt_ctx->streams[video_stream_idx]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "未找到解码器" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 5. 创建解码器上下文
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "创建解码器上下文失败" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 6. 复制参数到上下文
    ret = avcodec_parameters_to_context(codec_ctx, codecpar);
    if (ret < 0) {
        std::cerr << "复制参数失败" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 7. 打开解码器
    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        std::cerr << "打开解码器失败" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 8. 打开输出文件（如果指定）
    FILE *yuv_file = nullptr;
    if (output_file) {
        yuv_file = fopen(output_file, "wb");
        if (!yuv_file) {
            std::cerr << "打开输出文件失败: " << output_file << std::endl;
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&fmt_ctx);
            return 1;
        }
    }

    // 9. 分配 packet 和 frame
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    if (!packet || !frame) {
        std::cerr << "分配 packet/frame 失败" << std::endl;
        av_frame_free(&frame);
        av_packet_free(&packet);
        if (yuv_file) fclose(yuv_file);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 10. 打印视频信息
    std::cout << "视频信息:" << std::endl;
    std::cout << "  分辨率: " << codec_ctx->width << "x" << codec_ctx->height << std::endl;
    std::cout << "  像素格式: " << av_get_pix_fmt_name(codec_ctx->pix_fmt) << std::endl;
    std::cout << "  解码器: " << codec->name << std::endl;
    std::cout << std::endl;

    // 11. 解码循环
    int frame_count = 0;
    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_idx) {
            ret = avcodec_send_packet(codec_ctx, packet);
            if (ret < 0) {
                if (ret == AVERROR(EAGAIN)) {
                    // 解码器缓冲区满，先接收已缓冲的帧
                    while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                        frame_count++;
                        print_frame_info(frame_count, frame);
                        if (yuv_file) save_yuv_frame(frame, yuv_file);
                        av_frame_unref(frame);
                    }
                    // 重试发送
                    ret = avcodec_send_packet(codec_ctx, packet);
                }
                if (ret < 0 && ret != AVERROR_EOF) {
                    char err_buf[AV_ERROR_MAX_STRING_SIZE];
                    av_strerror(ret, err_buf, sizeof(err_buf));
                    std::cerr << "发送包失败: " << err_buf << std::endl;
                    av_packet_unref(packet);
                    continue;
                }
            }

            while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                frame_count++;
                print_frame_info(frame_count, frame);
                if (yuv_file) save_yuv_frame(frame, yuv_file);
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    // 12. flush 解码器
    std::cout << "\n--- flush 解码器 ---" << std::endl;
    avcodec_send_packet(codec_ctx, nullptr);
    while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
        frame_count++;
        print_frame_info(frame_count, frame);
        if (yuv_file) save_yuv_frame(frame, yuv_file);
        av_frame_unref(frame);
    }

    std::cout << "\n总共解码 " << frame_count << " 帧" << std::endl;

    if (yuv_file) {
        std::cout << "YUV 数据已保存到: " << output_file << std::endl;
        std::cout << "验证命令: ffplay -video_size " << codec_ctx->width << "x" << codec_ctx->height
                  << " -pixel_format yuv420p " << output_file << std::endl;
    }

    // 13. 释放资源
    av_frame_free(&frame);
    av_packet_free(&packet);
    if (yuv_file) {
        fclose(yuv_file);
    }
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);

    return 0;
}
