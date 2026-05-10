#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

/**
 * 第17节：转封装、流拷贝与精确剪辑
 * 
 * 演示两种剪辑模式：
 * 1. Fast Mode (Stream Copy): 直接拷贝 AVPacket，速度极快但受限于关键帧。
 * 2. Precise Mode (Decode & Seek): 解码到精确时间点，确保首帧准确。
 */

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " <input> <output> <start_ms> <mode>\n"
              << "Modes: \n"
              << "  fast: Stream copy starting from the nearest IDR before start_ms\n"
              << "  precise: Decode until start_ms and save the exact frame as YUV\n";
}

int save_yuv_frame(AVFrame* frame, const std::string& filename) {
    if (frame->format != AV_PIX_FMT_YUV420P) {
        std::cerr << "Error: Only YUV420P is supported for saving. Frame format is " 
                  << av_get_pix_fmt_name((AVPixelFormat)frame->format) << "\n";
        return -1;
    }

    FILE* f = fopen(filename.c_str(), "wb");
    if (!f) return -1;

    for (int i = 0; i < frame->height; i++)
        fwrite(frame->data[0] + i * frame->linesize[0], 1, frame->width, f);
    for (int i = 0; i < frame->height / 2; i++)
        fwrite(frame->data[1] + i * frame->linesize[1], 1, frame->width / 2, f);
    for (int i = 0; i < frame->height / 2; i++)
        fwrite(frame->data[2] + i * frame->linesize[2], 1, frame->width / 2, f);

    fclose(f);
    return 0;
}

int run_fast_mode(const char* in_filename, const char* out_filename, int64_t start_ms) {
    AVFormatContext* ifmt_ctx = nullptr;
    AVFormatContext* ofmt_ctx = nullptr;
    AVPacket* pkt = nullptr;
    int ret = 0;
    int video_stream_idx = -1;

    pkt = av_packet_alloc();
    if (!pkt) return AVERROR(ENOMEM);

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, nullptr, nullptr)) < 0) goto end;
    if ((ret = avformat_find_stream_info(ifmt_ctx, nullptr)) < 0) goto end;

    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, out_filename);
    if (!ofmt_ctx) { ret = AVERROR(ENOMEM); goto end; }

    for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* in_stream = ifmt_ctx->streams[i];
        if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
        }
        AVStream* out_stream = avformat_new_stream(ofmt_ctx, nullptr);
        if (!out_stream) { ret = AVERROR(ENOMEM); goto end; }
        if ((ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar)) < 0) goto end;
        out_stream->codecpar->codec_tag = 0;
    }

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0) goto end;
    }

    if ((ret = avformat_write_header(ofmt_ctx, nullptr)) < 0) goto end;

    if (video_stream_idx < 0) {
        std::cerr << "No video stream found\n";
        ret = -1;
        goto end;
    }

    // Seek to start_ms
    {
        AVStream* v_stream = ifmt_ctx->streams[video_stream_idx];
        int64_t seek_ts = av_rescale_q(start_ms, {1, 1000}, v_stream->time_base);
        std::cout << "Seeking to " << start_ms << "ms (TS: " << seek_ts << ") using BACKWARD flag...\n";
        ret = avformat_seek_file(ifmt_ctx, video_stream_idx, INT64_MIN, seek_ts, seek_ts, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            std::cerr << "Seek failed\n";
            goto end;
        }
    }

    while (av_read_frame(ifmt_ctx, pkt) >= 0) {
        AVStream *in_stream = ifmt_ctx->streams[pkt->stream_index];
        AVStream *out_stream = ofmt_ctx->streams[pkt->stream_index];

        pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;

        if ((ret = av_interleaved_write_frame(ofmt_ctx, pkt)) < 0) {
            av_packet_unref(pkt);
            break;
        }
        av_packet_unref(pkt);
    }

    av_write_trailer(ofmt_ctx);

end:
    if (ifmt_ctx) avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&ofmt_ctx->pb);
    if (ofmt_ctx) avformat_free_context(ofmt_ctx);
    if (pkt) av_packet_free(&pkt);
    return ret;
}

int run_precise_mode(const char* in_filename, const char* out_filename, int64_t start_ms) {
    AVFormatContext* ifmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;
    const AVCodec* decoder = nullptr;
    AVPacket* pkt = nullptr;
    AVFrame* frame = nullptr;
    int ret = 0;
    int video_stream_idx = -1;
    bool found = false;

    pkt = av_packet_alloc();
    frame = av_frame_alloc();
    if (!pkt || !frame) { ret = AVERROR(ENOMEM); goto end; }

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, nullptr, nullptr)) < 0) goto end;
    if ((ret = avformat_find_stream_info(ifmt_ctx, nullptr)) < 0) goto end;

    video_stream_idx = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (video_stream_idx < 0) { ret = video_stream_idx; goto end; }

    dec_ctx = avcodec_alloc_context3(decoder);
    if (!dec_ctx) { ret = AVERROR(ENOMEM); goto end; }
    if ((ret = avcodec_parameters_to_context(dec_ctx, ifmt_ctx->streams[video_stream_idx]->codecpar)) < 0) goto end;
    if ((ret = avcodec_open2(dec_ctx, decoder, nullptr)) < 0) goto end;

    {
        AVStream* v_stream = ifmt_ctx->streams[video_stream_idx];
        int64_t target_pts = av_rescale_q(start_ms, {1, 1000}, v_stream->time_base);

        std::cout << "Precise Mode: Seeking to keyframe before " << start_ms << "ms...\n";
        if ((ret = avformat_seek_file(ifmt_ctx, video_stream_idx, INT64_MIN, target_pts, target_pts, AVSEEK_FLAG_BACKWARD)) < 0) goto end;

        while (av_read_frame(ifmt_ctx, pkt) >= 0) {
            if (pkt->stream_index == video_stream_idx) {
                ret = avcodec_send_packet(dec_ctx, pkt);
                if (ret < 0) { av_packet_unref(pkt); goto end; }
                while (ret >= 0) {
                    ret = avcodec_receive_frame(dec_ctx, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        ret = 0;
                        break;
                    } else if (ret < 0) {
                        goto end;
                    }
                    
                    double current_ms = frame->pts * av_q2d(v_stream->time_base) * 1000;
                    if (!found) {
                        std::cout << "Decoding frame at pts: " << frame->pts << " (" << current_ms << " ms)\n";
                    }

                    if (frame->pts >= target_pts) {
                        std::cout << "Bingo! Found precise frame at " << current_ms << "ms (Target: " << start_ms << "ms)\n";
                        if ((ret = save_yuv_frame(frame, out_filename)) < 0) goto end;
                        std::cout << "Saved precise frame to " << out_filename << "\n";
                        found = true;
                        break;
                    }
                }
            }
            av_packet_unref(pkt);
            if (found) break;
        }
    }

end:
    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (ifmt_ctx) avformat_close_input(&ifmt_ctx);
    if (pkt) av_packet_free(&pkt);
    if (frame) av_frame_free(&frame);
    return (found) ? 0 : (ret < 0 ? ret : -1);
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        print_usage(argv[0]);
        return 1;
    }

    const char* input = argv[1];
    const char* output = argv[2];
    int64_t start_ms = 0;
    try {
        start_ms = std::stoll(argv[3]);
    } catch (...) {
        std::cerr << "Invalid start_ms\n";
        return 1;
    }
    std::string mode = argv[4];

    int ret = 0;
    if (mode == "fast") {
        ret = run_fast_mode(input, output, start_ms);
    } else if (mode == "precise") {
        ret = run_precise_mode(input, output, start_ms);
    } else {
        print_usage(argv[0]);
        return 1;
    }

    if (ret < 0) {
        char err_buf[128];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "Error occurred: " << err_buf << "\n";
        return 1;
    }

    return 0;
}
