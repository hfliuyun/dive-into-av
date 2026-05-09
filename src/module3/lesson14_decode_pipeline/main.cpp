#include <iostream>
#include <cstdio>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <input.mp4> [-y output.yuv] [-p output.pcm]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "示例:" << std::endl;
        std::cerr << "  " << argv[0] << " test.mp4 -y output.yuv          # 只解码视频" << std::endl;
        std::cerr << "  " << argv[0] << " test.mp4 -p output.pcm          # 只解码音频" << std::endl;
        std::cerr << "  " << argv[0] << " test.mp4 -y out.yuv -p out.pcm  # 同时解码音频和视频" << std::endl;
        return 1;
    }

    const char *input_file = argv[1];
    const char *yuv_file = nullptr;
    const char *pcm_file = nullptr;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-y") == 0 && i + 1 < argc) {
            yuv_file = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            pcm_file = argv[++i];
        }
    }

    if (!yuv_file && !pcm_file) {
        std::cerr << "错误: 至少需要指定一个输出文件 (-y 或 -p)" << std::endl;
        return 1;
    }

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

    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "查找流信息失败: " << err_buf << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 2. 查找视频流和音频流
    int video_idx = -1;
    int audio_idx = -1;
    AVCodecContext *video_ctx = nullptr;
    AVCodecContext *audio_ctx = nullptr;

    for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream *stream = fmt_ctx->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;

        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_idx == -1) {
            video_idx = i;
            const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
            if (!codec) {
                std::cerr << "未找到视频解码器" << std::endl;
                continue;
            }
            video_ctx = avcodec_alloc_context3(codec);
            if (!video_ctx) {
                std::cerr << "创建视频解码器上下文失败" << std::endl;
                continue;
            }
            ret = avcodec_parameters_to_context(video_ctx, codecpar);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, err_buf, sizeof(err_buf));
                std::cerr << "复制视频参数失败: " << err_buf << std::endl;
                avcodec_free_context(&video_ctx);
                video_ctx = nullptr;
                continue;
            }
            ret = avcodec_open2(video_ctx, codec, nullptr);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, err_buf, sizeof(err_buf));
                std::cerr << "打开视频解码器失败: " << err_buf << std::endl;
                avcodec_free_context(&video_ctx);
                video_ctx = nullptr;
            }
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_idx == -1) {
            audio_idx = i;
            const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
            if (!codec) {
                std::cerr << "未找到音频解码器" << std::endl;
                continue;
            }
            audio_ctx = avcodec_alloc_context3(codec);
            if (!audio_ctx) {
                std::cerr << "创建音频解码器上下文失败" << std::endl;
                continue;
            }
            ret = avcodec_parameters_to_context(audio_ctx, codecpar);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, err_buf, sizeof(err_buf));
                std::cerr << "复制音频参数失败: " << err_buf << std::endl;
                avcodec_free_context(&audio_ctx);
                audio_ctx = nullptr;
                continue;
            }
            ret = avcodec_open2(audio_ctx, codec, nullptr);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, err_buf, sizeof(err_buf));
                std::cerr << "打开音频解码器失败: " << err_buf << std::endl;
                avcodec_free_context(&audio_ctx);
                audio_ctx = nullptr;
            }
        }
    }

    // 3. 打开输出文件
    FILE *yuv_fp = nullptr;
    FILE *pcm_fp = nullptr;

    if (yuv_file && video_ctx) {
        yuv_fp = fopen(yuv_file, "wb");
        if (!yuv_fp) {
            std::cerr << "打开 YUV 输出文件失败: " << yuv_file << std::endl;
        }
    }

    if (pcm_file && audio_ctx) {
        pcm_fp = fopen(pcm_file, "wb");
        if (!pcm_fp) {
            std::cerr << "打开 PCM 输出文件失败: " << pcm_file << std::endl;
        }
    }

    // 4. 解码循环
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    if (!packet || !frame) {
        std::cerr << "分配 packet/frame 失败" << std::endl;
        av_frame_free(&frame);
        av_packet_free(&packet);
        if (yuv_fp) fclose(yuv_fp);
        if (pcm_fp) fclose(pcm_fp);
        if (video_ctx) avcodec_free_context(&video_ctx);
        if (audio_ctx) avcodec_free_context(&audio_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    int video_frames = 0;
    int audio_frames = 0;

    std::cout << "解码管道信息:" << std::endl;
    if (video_ctx) {
        std::cout << "  视频: " << video_ctx->width << "x" << video_ctx->height
                  << " (" << avcodec_get_name(video_ctx->codec_id) << ")" << std::endl;
    }
    if (audio_ctx) {
        std::cout << "  音频: " << audio_ctx->sample_rate << "Hz, "
                  << audio_ctx->ch_layout.nb_channels << "ch"
                  << " (" << avcodec_get_name(audio_ctx->codec_id) << ")" << std::endl;
    }
    std::cout << std::endl;

    // 检查像素格式
    if (video_ctx && video_ctx->pix_fmt != AV_PIX_FMT_YUV420P) {
        std::cout << "警告: 视频像素格式不是 yuv420p (实际: "
                  << av_get_pix_fmt_name(video_ctx->pix_fmt)
                  << ")，YUV 输出可能不正确" << std::endl;
    }

    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index == video_idx && video_ctx && yuv_fp) {
            ret = avcodec_send_packet(video_ctx, packet);
            if (ret < 0) {
                av_packet_unref(packet);
                continue;
            }

            while (avcodec_receive_frame(video_ctx, frame) >= 0) {
                // 保存 YUV 数据
                int y_size = frame->width * frame->height;
                int uv_size = y_size / 4;

                fwrite(frame->data[0], 1, y_size, yuv_fp);  // Y
                fwrite(frame->data[1], 1, uv_size, yuv_fp); // U
                fwrite(frame->data[2], 1, uv_size, yuv_fp); // V

                video_frames++;
                av_frame_unref(frame);
            }
        } else if (packet->stream_index == audio_idx && audio_ctx && pcm_fp) {
            ret = avcodec_send_packet(audio_ctx, packet);
            if (ret < 0) {
                av_packet_unref(packet);
                continue;
            }

            while (avcodec_receive_frame(audio_ctx, frame) >= 0) {
                // 保存 PCM 数据
                int data_size = av_get_bytes_per_sample(audio_ctx->sample_fmt);
                int is_planar = av_sample_fmt_is_planar(audio_ctx->sample_fmt);

                if (is_planar) {
                    // 平面格式：每个通道独立存储
                    for (int i = 0; i < frame->nb_samples; i++) {
                        for (int ch = 0; ch < audio_ctx->ch_layout.nb_channels; ch++) {
                            fwrite(frame->data[ch] + data_size * i, 1, data_size, pcm_fp);
                        }
                    }
                } else {
                    // 交错格式：所有通道数据交织在 data[0] 中
                    fwrite(frame->data[0], 1, frame->nb_samples * data_size * audio_ctx->ch_layout.nb_channels, pcm_fp);
                }

                audio_frames++;
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    // 5. flush 解码器
    std::cout << "\n--- flush 解码器 ---" << std::endl;

    if (video_ctx) {
        ret = avcodec_send_packet(video_ctx, nullptr);
        if (ret < 0 && ret != AVERROR_EOF) {
            char err_buf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, err_buf, sizeof(err_buf));
            std::cerr << "flush 视频解码器失败: " << err_buf << std::endl;
        }
        while ((ret = avcodec_receive_frame(video_ctx, frame)) >= 0) {
            if (yuv_fp) {
                int y_size = frame->width * frame->height;
                int uv_size = y_size / 4;
                fwrite(frame->data[0], 1, y_size, yuv_fp);
                fwrite(frame->data[1], 1, uv_size, yuv_fp);
                fwrite(frame->data[2], 1, uv_size, yuv_fp);
            }
            video_frames++;
            av_frame_unref(frame);
        }
    }

    if (audio_ctx) {
        ret = avcodec_send_packet(audio_ctx, nullptr);
        if (ret < 0 && ret != AVERROR_EOF) {
            char err_buf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, err_buf, sizeof(err_buf));
            std::cerr << "flush 音频解码器失败: " << err_buf << std::endl;
        }
        while ((ret = avcodec_receive_frame(audio_ctx, frame)) >= 0) {
            if (pcm_fp) {
                int data_size = av_get_bytes_per_sample(audio_ctx->sample_fmt);
                int is_planar = av_sample_fmt_is_planar(audio_ctx->sample_fmt);

                if (is_planar) {
                    for (int i = 0; i < frame->nb_samples; i++) {
                        for (int ch = 0; ch < audio_ctx->ch_layout.nb_channels; ch++) {
                            fwrite(frame->data[ch] + data_size * i, 1, data_size, pcm_fp);
                        }
                    }
                } else {
                    fwrite(frame->data[0], 1, frame->nb_samples * data_size * audio_ctx->ch_layout.nb_channels, pcm_fp);
                }
            }
            audio_frames++;
            av_frame_unref(frame);
        }
    }

    std::cout << "\n解码完成:" << std::endl;
    std::cout << "  视频帧: " << video_frames << std::endl;
    std::cout << "  音频帧: " << audio_frames << std::endl;

    if (yuv_file && video_ctx) {
        std::cout << "  YUV 文件: " << yuv_file << std::endl;
        std::cout << "  验证命令: ffplay -video_size " << video_ctx->width << "x" << video_ctx->height
                  << " -pixel_format yuv420p " << yuv_file << std::endl;
    }

    if (pcm_file && audio_ctx) {
        std::cout << "  PCM 文件: " << pcm_file << std::endl;
        std::cout << "  验证命令: ffplay -f " << av_get_sample_fmt_name(audio_ctx->sample_fmt)
                  << " -channels " << audio_ctx->ch_layout.nb_channels
                  << " -sample_rate " << audio_ctx->sample_rate
                  << " " << pcm_file << std::endl;
    }

    // 6. 释放资源
    av_frame_free(&frame);
    av_packet_free(&packet);
    if (yuv_fp) fclose(yuv_fp);
    if (pcm_fp) fclose(pcm_fp);
    if (video_ctx) avcodec_free_context(&video_ctx);
    if (audio_ctx) avcodec_free_context(&audio_ctx);
    avformat_close_input(&fmt_ctx);

    return 0;
}
