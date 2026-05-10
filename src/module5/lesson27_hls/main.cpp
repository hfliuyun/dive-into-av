#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/mathematics.h>
}

/**
 * 第27节：HLS 切片与基础直播链路
 * 
 * 目标：将本地 MP4 文件切片为 HLS (M3U8 + TS)。
 * 核心点：AVDictionary 参数传递、HLS 封装逻辑、流映射优化。
 */

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input_file> <output_m3u8>" << std::endl;
        std::cout << "Example: " << argv[0] << " test.mp4 output/playlist.m3u8" << std::endl;
        return -1;
    }

    const char* in_filename = argv[1];
    const char* out_filename = argv[2];

    AVFormatContext* ifmt_ctx = nullptr;
    AVFormatContext* ofmt_ctx = nullptr;
    AVPacket* pkt = nullptr;
    int ret = 0;

    pkt = av_packet_alloc();
    if (!pkt) return -1;

    // 1. 打开输入文件
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, nullptr, nullptr)) < 0) {
        std::cerr << "Could not open input file" << std::endl;
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, nullptr)) < 0) goto end;

    // 2. 初始化输出上下文 (指定为 hls 格式)
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, "hls", out_filename);
    if (!ofmt_ctx) {
        std::cerr << "Could not create output context" << std::endl;
        goto end;
    }

    // 3. 复制流信息并建立映射表
    {
        std::vector<int> stream_mapping(ifmt_ctx->nb_streams, -1);
        int out_stream_idx = 0;

        for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++) {
            AVStream* in_stream = ifmt_ctx->streams[i];
            if (in_stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO && 
                in_stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
                continue;
            }

            AVStream* out_stream = avformat_new_stream(ofmt_ctx, nullptr);
            if (!out_stream) goto end;
            avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            out_stream->codecpar->codec_tag = 0;
            stream_mapping[i] = out_stream_idx++;
        }

        // 4. 打开输出 IO
        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
            if (ret < 0) {
                std::cerr << "Could not open output URL" << std::endl;
                goto end;
            }
        }

        // 5. 设置 HLS 参数并写入头信息
        AVDictionary* options = nullptr;
        av_dict_set(&options, "hls_time", "5", 0);
        av_dict_set(&options, "hls_list_size", "0", 0);
        // 重要：切片名不应包含 output/ 目录前缀，因为 M3U8 文件已经在 output 目录下
        // 否则 M3U8 内部会记录为 output/seg_xxx.ts，导致播放器在 output/output/ 查找
        av_dict_set(&options, "hls_segment_filename", "seg_%03d.ts", 0);

        ret = avformat_write_header(ofmt_ctx, &options);
        av_dict_free(&options);
        if (ret < 0) {
            std::cerr << "Error occurred when writing header" << std::endl;
            goto end;
        }

        // 6. 转换并写入数据包
        while (true) {
            ret = av_read_frame(ifmt_ctx, pkt);
            if (ret < 0) break;

            int target_idx = stream_mapping[pkt->stream_index];
            if (target_idx < 0) {
                av_packet_unref(pkt);
                continue;
            }

            AVStream* in_stream  = ifmt_ctx->streams[pkt->stream_index];
            AVStream* out_stream = ofmt_ctx->streams[target_idx];

            pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
            pkt->pos = -1;
            pkt->stream_index = target_idx;

            ret = av_interleaved_write_frame(ofmt_ctx, pkt);
            av_packet_unref(pkt);
            if (ret < 0) break;
        }
    }

    av_write_trailer(ofmt_ctx);
    std::cout << "HLS Segmenting finished successfully." << std::endl;

end:
    if (ifmt_ctx) avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&ofmt_ctx->pb);
    if (ofmt_ctx) avformat_free_context(ofmt_ctx);
    if (pkt) av_packet_free(&pkt);

    return (ret < 0 && ret != AVERROR_EOF) ? 1 : 0;
}
