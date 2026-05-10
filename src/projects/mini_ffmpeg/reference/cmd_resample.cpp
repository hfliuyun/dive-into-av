#include "common.h"

int cmd_resample(const Config& config) {
    AVFormatContext* ifmt_ctx = nullptr;
    AVFormatContext* ofmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;
    AVCodecContext* enc_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;
    AVPacket* ipkt = av_packet_alloc();
    AVPacket* opkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* res_frame = av_frame_alloc();
    AVStream* out_stream = nullptr;
    const AVCodec* decoder = nullptr;
    const AVCodec* encoder = nullptr;
    int ret = 0;
    int audio_stream_idx = -1;
    int out_samples = 0;

    ret = avformat_open_input(&ifmt_ctx, config.input_file.c_str(), nullptr, nullptr);
    CHECK_RET(ret, "Cannot open input");
    ret = avformat_find_stream_info(ifmt_ctx, nullptr);
    CHECK_RET(ret, "Cannot find stream info");

    audio_stream_idx = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
    if (audio_stream_idx < 0) { ret = audio_stream_idx; CHECK_RET(ret, "No audio stream"); }

    dec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(dec_ctx, ifmt_ctx->streams[audio_stream_idx]->codecpar);
    ret = avcodec_open2(dec_ctx, decoder, nullptr);
    CHECK_RET(ret, "Cannot open decoder");

    encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    enc_ctx = avcodec_alloc_context3(encoder);
    enc_ctx->sample_rate = config.sample_rate > 0 ? config.sample_rate : dec_ctx->sample_rate;
    if (config.channels > 0) {
        av_channel_layout_default(&enc_ctx->ch_layout, config.channels);
    } else {
        av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout);
    }
    enc_ctx->sample_fmt = encoder->sample_fmts ? encoder->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};
    ret = avcodec_open2(enc_ctx, encoder, nullptr);
    CHECK_RET(ret, "Cannot open encoder");

    swr_ctx = swr_alloc();
    swr_alloc_set_opts2(&swr_ctx, &enc_ctx->ch_layout, enc_ctx->sample_fmt, enc_ctx->sample_rate,
                        &dec_ctx->ch_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate, 0, nullptr);
    ret = swr_init(swr_ctx);
    CHECK_RET(ret, "Cannot init swr");

    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, config.output_file.c_str());
    out_stream = avformat_new_stream(ofmt_ctx, nullptr);
    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, config.output_file.c_str(), AVIO_FLAG_WRITE);
        CHECK_RET(ret, "Cannot open output file");
    }
    ret = avformat_write_header(ofmt_ctx, nullptr);
    CHECK_RET(ret, "Cannot write header");

    while (av_read_frame(ifmt_ctx, ipkt) >= 0) {
        if (ipkt->stream_index == audio_stream_idx) {
            ret = avcodec_send_packet(dec_ctx, ipkt);
            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

                out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
                res_frame->nb_samples = out_samples;
                res_frame->format = enc_ctx->sample_fmt;
                av_channel_layout_copy(&res_frame->ch_layout, &enc_ctx->ch_layout);
                av_frame_get_buffer(res_frame, 0);

                ret = swr_convert(swr_ctx, res_frame->data, out_samples,
                                  (const uint8_t**)frame->data, frame->nb_samples);
                
                res_frame->pts = av_rescale_q(frame->pts, dec_ctx->time_base, enc_ctx->time_base);
                
                ret = avcodec_send_frame(enc_ctx, res_frame);
                while (ret >= 0) {
                    ret = avcodec_receive_packet(enc_ctx, opkt);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                    opkt->stream_index = 0;
                    av_packet_rescale_ts(opkt, enc_ctx->time_base, out_stream->time_base);
                    av_interleaved_write_frame(ofmt_ctx, opkt);
                    av_packet_unref(opkt);
                }
                av_frame_unref(res_frame);
                av_frame_unref(frame);
            }
        }
        av_packet_unref(ipkt);
    }

    // Flush Encoder
    ret = avcodec_send_frame(enc_ctx, nullptr);
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, opkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        opkt->stream_index = 0;
        av_packet_rescale_ts(opkt, enc_ctx->time_base, out_stream->time_base);
        av_interleaved_write_frame(ofmt_ctx, opkt);
        av_packet_unref(opkt);
    }

    av_write_trailer(ofmt_ctx);

end:
    if (ifmt_ctx) avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&ofmt_ctx->pb);
    if (ofmt_ctx) avformat_free_context(ofmt_ctx);
    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (enc_ctx) avcodec_free_context(&enc_ctx);
    if (swr_ctx) swr_free(&swr_ctx);
    av_packet_free(&ipkt); av_packet_free(&opkt);
    av_frame_free(&frame); av_frame_free(&res_frame);
    return ret;
}
