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
    AVStream* out_stream = nullptr;
    const AVCodec* decoder = nullptr;
    const AVCodec* encoder = nullptr;
    AVFilterInOut *outputs = nullptr;
    AVFilterInOut *inputs  = nullptr;
    int ret = 0;
    int video_stream_idx = -1;
    char args[512];
    char filter_descr[128];

    ret = avformat_open_input(&ifmt_ctx, config.input_file.c_str(), nullptr, nullptr);
    CHECK_RET(ret, "Cannot open input");
    ret = avformat_find_stream_info(ifmt_ctx, nullptr);
    CHECK_RET(ret, "Cannot find stream info");

    // 1. Setup Decoder
    video_stream_idx = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (video_stream_idx < 0) { ret = video_stream_idx; CHECK_RET(ret, "No video stream"); }

    dec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(dec_ctx, ifmt_ctx->streams[video_stream_idx]->codecpar);
    ret = avcodec_open2(dec_ctx, decoder, nullptr);
    CHECK_RET(ret, "Cannot open decoder");

    // 2. Setup Encoder (H.264)
    encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    enc_ctx = avcodec_alloc_context3(encoder);
    enc_ctx->height = dec_ctx->height;
    enc_ctx->width = dec_ctx->width;
    if (!config.scale.empty()) {
        sscanf(config.scale.c_str(), "%dx%d", &enc_ctx->width, &enc_ctx->height);
    }
    enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
    enc_ctx->pix_fmt = encoder->pix_fmts ? encoder->pix_fmts[0] : AV_PIX_FMT_YUV420P;
    enc_ctx->time_base = ifmt_ctx->streams[video_stream_idx]->avg_frame_rate.num ? 
                         av_inv_q(ifmt_ctx->streams[video_stream_idx]->avg_frame_rate) : 
                         (AVRational){1, 25};
    ret = avcodec_open2(enc_ctx, encoder, nullptr);
    CHECK_RET(ret, "Cannot open encoder");

    // 3. Setup Muxer
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, config.output_file.c_str());
    out_stream = avformat_new_stream(ofmt_ctx, nullptr);
    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    out_stream->time_base = enc_ctx->time_base;
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, config.output_file.c_str(), AVIO_FLAG_WRITE);
        CHECK_RET(ret, "Cannot open output file");
    }
    ret = avformat_write_header(ofmt_ctx, nullptr);
    CHECK_RET(ret, "Cannot write header");

    // 4. Setup Filter (Scale)
    filter_graph = avfilter_graph_alloc();
    snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
             dec_ctx->time_base.num, dec_ctx->time_base.den,
             dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, avfilter_get_by_name("buffer"), "in", args, nullptr, filter_graph);
    CHECK_RET(ret, "Cannot create buffer source");
    ret = avfilter_graph_create_filter(&buffersink_ctx, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, filter_graph);
    CHECK_RET(ret, "Cannot create buffer sink");

    snprintf(filter_descr, sizeof(filter_descr), "scale=%d:%d", enc_ctx->width, enc_ctx->height);
    outputs = avfilter_inout_alloc();
    inputs  = avfilter_inout_alloc();
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = nullptr;
    inputs->name        = av_strdup("out");
    inputs->filter_ctx  = buffersink_ctx;
    inputs->pad_idx     = 0;
    inputs->next        = nullptr;

    ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, nullptr);
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        CHECK_RET(ret, "Cannot parse filter graph");
    }
    ret = avfilter_graph_config(filter_graph, nullptr);
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    CHECK_RET(ret, "Cannot config filter graph");

    // 5. Transcode Loop
    while (av_read_frame(ifmt_ctx, ipkt) >= 0) {
        if (ipkt->stream_index == video_stream_idx) {
            ret = avcodec_send_packet(dec_ctx, ipkt);
            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                
                frame->pts = frame->best_effort_timestamp;
                ret = av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
                while (ret >= 0) {
                    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

                    ret = avcodec_send_frame(enc_ctx, filt_frame);
                    while (ret >= 0) {
                        ret = avcodec_receive_packet(enc_ctx, opkt);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                        opkt->stream_index = 0;
                        av_packet_rescale_ts(opkt, enc_ctx->time_base, out_stream->time_base);
                        av_interleaved_write_frame(ofmt_ctx, opkt);
                        av_packet_unref(opkt);
                    }
                    av_frame_unref(filt_frame);
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(ipkt);
    }

    // Flush Filter & Encoder
    av_buffersrc_add_frame_flags(buffersrc_ctx, nullptr, 0);
    while (av_buffersink_get_frame(buffersink_ctx, filt_frame) >= 0) {
        avcodec_send_frame(enc_ctx, filt_frame);
        av_frame_unref(filt_frame);
    }
    
    // Flush Encoder
    avcodec_send_frame(enc_ctx, nullptr);
    while (true) {
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
    if (filter_graph) avfilter_graph_free(&filter_graph);
    av_packet_free(&ipkt); av_packet_free(&opkt);
    av_frame_free(&frame); av_frame_free(&filt_frame);
    return ret;
}
