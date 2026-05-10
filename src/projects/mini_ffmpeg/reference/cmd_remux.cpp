#include "common.h"

int cmd_remux(const Config& config) {
    AVFormatContext* ifmt_ctx = nullptr;
    AVFormatContext* ofmt_ctx = nullptr;
    AVPacket* pkt = nullptr;
    int ret = 0;

    pkt = av_packet_alloc();
    if (!pkt) return AVERROR(ENOMEM);

    ret = avformat_open_input(&ifmt_ctx, config.input_file.c_str(), nullptr, nullptr);
    CHECK_RET(ret, "Cannot open input file");

    ret = avformat_find_stream_info(ifmt_ctx, nullptr);
    CHECK_RET(ret, "Cannot find stream info");

    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, config.output_file.c_str());
    if (!ofmt_ctx) { ret = AVERROR(ENOMEM); CHECK_RET(ret, "Cannot create output context"); }

    for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* in_stream = ifmt_ctx->streams[i];
        AVStream* out_stream = avformat_new_stream(ofmt_ctx, nullptr);
        if (!out_stream) { ret = AVERROR(ENOMEM); CHECK_RET(ret, "Cannot create output stream"); }
        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        CHECK_RET(ret, "Cannot copy codec parameters");
        out_stream->codecpar->codec_tag = 0;
    }

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, config.output_file.c_str(), AVIO_FLAG_WRITE);
        CHECK_RET(ret, "Cannot open output file");
    }

    ret = avformat_write_header(ofmt_ctx, nullptr);
    CHECK_RET(ret, "Cannot write header");

    while (av_read_frame(ifmt_ctx, pkt) >= 0) {
        AVStream* in_stream = ifmt_ctx->streams[pkt->stream_index];
        AVStream* out_stream = ofmt_ctx->streams[pkt->stream_index];

        pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;

        ret = av_interleaved_write_frame(ofmt_ctx, pkt);
        av_packet_unref(pkt);
        if (ret < 0) break;
    }

    av_write_trailer(ofmt_ctx);

end:
    if (ifmt_ctx) avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&ofmt_ctx->pb);
    if (ofmt_ctx) avformat_free_context(ofmt_ctx);
    if (pkt) av_packet_free(&pkt);
    return ret;
}
