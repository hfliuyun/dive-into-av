#include <iostream>
#include <string>
#include <vector>
#include <chrono>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
}

/**
 * 第26节：RTMP 推流与拉流
 * 
 * 目标：将本地 MP4 文件推送到 RTMP 流媒体服务器。
 * 核心点：节奏控制（Pacing）、起始偏移修正、流过滤。
 */

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input_file> <rtmp_url>" << std::endl;
        return -1;
    }

    const char* in_filename = argv[1];
    const char* out_filename = argv[2];

    AVFormatContext* ifmt_ctx = nullptr;
    AVFormatContext* ofmt_ctx = nullptr;
    AVPacket* pkt = nullptr;
    int ret = 0;
    int out_stream_idx = 0;
    std::vector<int> stream_mapping;

    pkt = av_packet_alloc();
    if (!pkt) {
        std::cerr << "Could not allocate packet" << std::endl;
        return AVERROR(ENOMEM);
    }

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, nullptr, nullptr)) < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "Could not open input: " << err_buf << std::endl;
        av_packet_free(&pkt);
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, nullptr)) < 0) goto end;

    avformat_alloc_output_context2(&ofmt_ctx, nullptr, "flv", out_filename);
    if (!ofmt_ctx) {
        std::cerr << "Could not create output context" << std::endl;
        goto end;
    }

    stream_mapping.resize(ifmt_ctx->nb_streams, -1);

    for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* in_stream = ifmt_ctx->streams[i];
        AVCodecParameters* in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO && 
            in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
            continue;
        }

        AVStream* out_stream = avformat_new_stream(ofmt_ctx, nullptr);
        if (!out_stream) goto end;

        avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        out_stream->codecpar->codec_tag = 0;
        stream_mapping[i] = out_stream_idx++;
    }

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0) goto end;
    }

    if ((ret = avformat_write_header(ofmt_ctx, nullptr)) < 0) goto end;

    {
        int64_t start_time = av_gettime();
        int64_t first_dts = AV_NOPTS_VALUE;
        int64_t total_bytes = 0;
        auto start_wall_time = std::chrono::steady_clock::now();

        while (true) {
            ret = av_read_frame(ifmt_ctx, pkt);
            if (ret < 0) break;

            int out_idx = stream_mapping[pkt->stream_index];
            if (out_idx < 0) { 
                av_packet_unref(pkt);
                continue;
            }

            AVStream* in_stream  = ifmt_ctx->streams[pkt->stream_index];
            AVStream* out_stream = ofmt_ctx->streams[out_idx];

            if (first_dts == AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE) {
                first_dts = pkt->dts;
            }

            int64_t dts_offset = (first_dts != AV_NOPTS_VALUE) ? pkt->dts - first_dts : 0;
            int64_t pts_time = av_rescale_q(dts_offset, in_stream->time_base, {1, 1000000});
            int64_t now_time = av_gettime() - start_time;
            if (pts_time > now_time) {
                av_usleep(static_cast<unsigned int>(pts_time - now_time));
            }

            pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
            pkt->pos = -1;
            pkt->stream_index = out_idx;

            total_bytes += pkt->size;
            if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_wall_time).count();
                double kbps = elapsed > 0 ? (total_bytes * 8.0 / 1024.0 / elapsed) : 0;
                printf("Pushing... Video PTS: %8.2f s | Rate: %6.1f kbps\r", (double)pkt->pts * av_q2d(out_stream->time_base), kbps);
                fflush(stdout);
            }

            ret = av_interleaved_write_frame(ofmt_ctx, pkt);
            av_packet_unref(pkt);
            if (ret < 0) break;
        }
    }

    av_write_trailer(ofmt_ctx);
    std::cout << "\nPushing finished." << std::endl;

end:
    if (ifmt_ctx) avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&ofmt_ctx->pb);
    if (ofmt_ctx) avformat_free_context(ofmt_ctx);
    if (pkt) av_packet_free(&pkt);
    return (ret < 0 && ret != AVERROR_EOF) ? 1 : 0;
}
