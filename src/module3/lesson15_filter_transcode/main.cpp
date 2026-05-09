#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
}

static int init_filter_graph(AVFilterGraph **graph,
                              AVFilterContext **src_ctx,
                              AVFilterContext **sink_ctx,
                              AVCodecContext *dec_ctx,
                              const char *filter_desc) {
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    *graph = avfilter_graph_alloc();
    if (!*graph) {
        std::cerr << "failed to allocate filter graph" << std::endl;
        return -1;
    }

    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
             dec_ctx->time_base.num > 0 ? dec_ctx->time_base.num : 1,
             dec_ctx->time_base.den > 0 ? dec_ctx->time_base.den : 25,
             dec_ctx->sample_aspect_ratio.num > 0
                 ? dec_ctx->sample_aspect_ratio.num : 1,
             dec_ctx->sample_aspect_ratio.den > 0
                 ? dec_ctx->sample_aspect_ratio.den : 1);

    int ret = avfilter_graph_create_filter(src_ctx, buffersrc, "in",
                                           args, nullptr, *graph);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "create buffer src failed: " << err_buf << std::endl;
        return ret;
    }

    ret = avfilter_graph_create_filter(sink_ctx, buffersink, "out",
                                       nullptr, nullptr, *graph);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "create buffer sink failed: " << err_buf << std::endl;
        return ret;
    }

    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    if (!outputs || !inputs) {
        std::cerr << "failed to allocate filter I/O" << std::endl;
        avfilter_inout_free(&outputs);
        avfilter_inout_free(&inputs);
        return -1;
    }

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = *src_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = nullptr;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = *sink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = nullptr;

    ret = avfilter_graph_parse_ptr(*graph, filter_desc,
                                    &inputs, &outputs, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "parse filter graph failed: " << err_buf << std::endl;
        std::cerr << "  filter: " << filter_desc << std::endl;
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return ret;
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    ret = avfilter_graph_config(*graph, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "configure filter graph failed: " << err_buf << std::endl;
        return ret;
    }

    return 0;
}

static void print_usage(const char *prog) {
    std::cerr << "usage: " << prog << " <input.mp4> <output.mp4> [filter] [param]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "built-in shortcuts:" << std::endl;
    std::cerr << "  " << prog << " in.mp4 out.mp4 scale 1280:720         # resize" << std::endl;
    std::cerr << "  " << prog << " in.mp4 out.mp4 text AV:24:10:10       # text watermark" << std::endl;
    std::cerr << "  " << prog << " in.mp4 out.mp4 hflip                  # horizontal flip" << std::endl;
    std::cerr << "  " << prog << " in.mp4 out.mp4 vflip                  # vertical flip" << std::endl;
    std::cerr << std::endl;
    std::cerr << "custom filter graph:" << std::endl;
    std::cerr << "  " << prog << " in.mp4 out.mp4 \"scale=640:360,drawtext=text='AV':fontsize=32:x=10:y=10\"" << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char *input_file   = argv[1];
    const char *output_file  = argv[2];
    const char *filter_cmd   = nullptr;
    const char *filter_param = nullptr;

    if (argc >= 4) {
        filter_cmd = argv[3];
        if (argc >= 5) filter_param = argv[4];
    }

    std::string filter_desc = "null";
    if (filter_cmd) {
        if (strcmp(filter_cmd, "scale") == 0 && filter_param) {
            filter_desc = std::string("scale=") + filter_param;
        } else if (strcmp(filter_cmd, "text") == 0 && filter_param) {
            char t[128] = "AV";
            int fs = 24, px = 10, py = 10;
            sscanf(filter_param, "%127[^:]:%d:%d:%d", t, &fs, &px, &py);
            char buf[512];
            snprintf(buf, sizeof(buf),
                     "drawtext=text='%s':fontsize=%d:fontcolor=white:"
                     "box=1:boxcolor=black@0.5:x=%d:y=%d",
                     t, fs, px, py);
            filter_desc = buf;
        } else if (strcmp(filter_cmd, "hflip") == 0) {
            filter_desc = "hflip";
        } else if (strcmp(filter_cmd, "vflip") == 0) {
            filter_desc = "vflip";
        } else {
            // pass through as raw filter string
            filter_desc = filter_cmd;
        }
    }

    int ret = 0;

    // 1. open input
    AVFormatContext *fmt_ctx = nullptr;
    ret = avformat_open_input(&fmt_ctx, input_file, nullptr, nullptr);
    if (ret < 0) {
        char eb[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, eb, sizeof(eb));
        std::cerr << "open input failed: " << eb << std::endl;
        return 1;
    }

    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        char eb[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, eb, sizeof(eb));
        std::cerr << "find stream info failed: " << eb << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 2. find video stream and create decoder
    int video_idx = -1;
    AVCodecContext *dec_ctx = nullptr;

    for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_idx = i;
            const AVCodec *codec = avcodec_find_decoder(
                fmt_ctx->streams[i]->codecpar->codec_id);
            if (!codec) {
                std::cerr << "decoder not found" << std::endl;
                avformat_close_input(&fmt_ctx);
                return 1;
            }
            dec_ctx = avcodec_alloc_context3(codec);
            if (!dec_ctx) {
                std::cerr << "alloc decoder context failed" << std::endl;
                avformat_close_input(&fmt_ctx);
                return 1;
            }
            ret = avcodec_parameters_to_context(dec_ctx,
                fmt_ctx->streams[i]->codecpar);
            if (ret < 0) {
                avcodec_free_context(&dec_ctx);
                avformat_close_input(&fmt_ctx);
                return 1;
            }
            ret = avcodec_open2(dec_ctx, codec, nullptr);
            if (ret < 0) {
                char eb[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, eb, sizeof(eb));
                std::cerr << "open decoder failed: " << eb << std::endl;
                avcodec_free_context(&dec_ctx);
                avformat_close_input(&fmt_ctx);
                return 1;
            }
            break;
        }
    }

    if (video_idx == -1) {
        std::cerr << "no video stream found" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 3. init filter graph
    AVFilterGraph *filter_graph   = nullptr;
    AVFilterContext *buffersrc_ctx  = nullptr;
    AVFilterContext *buffersink_ctx = nullptr;

    ret = init_filter_graph(&filter_graph, &buffersrc_ctx, &buffersink_ctx,
                            dec_ctx, filter_desc.c_str());
    if (ret < 0) {
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 4. create encoder
    const AVCodec *encoder = avcodec_find_encoder_by_name("libx264");
    if (!encoder) {
        std::cerr << "libx264 encoder not found" << std::endl;
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    AVCodecContext *enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
        std::cerr << "alloc encoder context failed" << std::endl;
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    enc_ctx->width   = av_buffersink_get_w(buffersink_ctx);
    enc_ctx->height  = av_buffersink_get_h(buffersink_ctx);
    enc_ctx->pix_fmt = (AVPixelFormat)av_buffersink_get_format(buffersink_ctx);
    enc_ctx->bit_rate = 1000000;
    enc_ctx->gop_size = 25;
    enc_ctx->max_b_frames = 2;

    AVRational fps = fmt_ctx->streams[video_idx]->avg_frame_rate;
    if (fps.num > 0 && fps.den > 0) {
        enc_ctx->time_base = av_inv_q(fps);
    } else {
        enc_ctx->time_base = AVRational{1, 25};
    }
    enc_ctx->framerate = fps;

    ret = avcodec_open2(enc_ctx, encoder, nullptr);
    if (ret < 0) {
        char eb[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, eb, sizeof(eb));
        std::cerr << "open encoder failed: " << eb << std::endl;
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&enc_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 5. create output
    AVFormatContext *ofmt_ctx = nullptr;
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, output_file);
    if (!ofmt_ctx) {
        std::cerr << "create output context failed" << std::endl;
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&enc_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    AVStream *out_stream = avformat_new_stream(ofmt_ctx, nullptr);
    if (!out_stream) {
        std::cerr << "create output stream failed" << std::endl;
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&enc_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (ret < 0) {
        std::cerr << "copy encoder params failed" << std::endl;
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&enc_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }
    out_stream->time_base = enc_ctx->time_base;

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            char eb[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, eb, sizeof(eb));
            std::cerr << "open output file failed: " << eb << std::endl;
            avfilter_graph_free(&filter_graph);
            avcodec_free_context(&enc_ctx);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&ofmt_ctx);
            avformat_close_input(&fmt_ctx);
            return 1;
        }
    }

    ret = avformat_write_header(ofmt_ctx, nullptr);
    if (ret < 0) {
        char eb[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, eb, sizeof(eb));
        std::cerr << "write header failed: " << eb << std::endl;
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&enc_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // 6. transcode loop
    AVPacket *packet     = av_packet_alloc();
    AVFrame  *frame      = av_frame_alloc();
    AVFrame  *filt_frame = av_frame_alloc();
    AVPacket *enc_pkt    = av_packet_alloc();

    if (!packet || !frame || !filt_frame || !enc_pkt) {
        std::cerr << "alloc packet/frame failed" << std::endl;
        av_frame_free(&filt_frame);
        av_frame_free(&frame);
        av_packet_free(&packet);
        av_packet_free(&enc_pkt);
        avfilter_graph_free(&filter_graph);
        avcodec_free_context(&enc_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    std::cout << "filter pipeline:" << std::endl;
    std::cout << "  filter: " << filter_desc << std::endl;
    std::cout << "  input:  " << dec_ctx->width << "x" << dec_ctx->height
              << " (" << av_get_pix_fmt_name(dec_ctx->pix_fmt) << ")" << std::endl;
    std::cout << "  output: " << enc_ctx->width << "x" << enc_ctx->height
              << " (" << av_get_pix_fmt_name(enc_ctx->pix_fmt) << ")" << std::endl;
    std::cout << std::endl;

    int frame_count = 0;

    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index != video_idx) {
            av_packet_unref(packet);
            continue;
        }

        ret = avcodec_send_packet(dec_ctx, packet);
        if (ret < 0) {
            av_packet_unref(packet);
            continue;
        }

        while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
            frame->pts = frame->best_effort_timestamp;

            ret = av_buffersrc_add_frame_flags(buffersrc_ctx, frame,
                                               AV_BUFFERSRC_FLAG_KEEP_REF);
            if (ret < 0) {
                av_frame_unref(frame);
                continue;
            }

            while ((ret = av_buffersink_get_frame(buffersink_ctx, filt_frame)) >= 0) {
            filt_frame->pts = frame->pts;

                ret = avcodec_send_frame(enc_ctx, filt_frame);
                if (ret < 0) {
                    av_frame_unref(filt_frame);
                    continue;
                }

                while ((ret = avcodec_receive_packet(enc_ctx, enc_pkt)) >= 0) {
                    enc_pkt->stream_index = out_stream->index;
                    av_packet_rescale_ts(enc_pkt, enc_ctx->time_base,
                                         out_stream->time_base);
                    av_interleaved_write_frame(ofmt_ctx, enc_pkt);
                    av_packet_unref(enc_pkt);
                }
                frame_count++;
                av_frame_unref(filt_frame);
            }
            av_frame_unref(frame);
        }
        av_packet_unref(packet);
    }

    // 7. flush decoder
    avcodec_send_packet(dec_ctx, nullptr);
    while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
        frame->pts = frame->best_effort_timestamp;

        ret = av_buffersrc_add_frame_flags(buffersrc_ctx, frame,
                                           AV_BUFFERSRC_FLAG_KEEP_REF);
        if (ret < 0) {
            av_frame_unref(frame);
            continue;
        }

        while ((ret = av_buffersink_get_frame(buffersink_ctx, filt_frame)) >= 0) {
            filt_frame->pts = frame->pts;

            ret = avcodec_send_frame(enc_ctx, filt_frame);
            if (ret < 0) {
                av_frame_unref(filt_frame);
                continue;
            }

            while ((ret = avcodec_receive_packet(enc_ctx, enc_pkt)) >= 0) {
                enc_pkt->stream_index = out_stream->index;
                av_packet_rescale_ts(enc_pkt, enc_ctx->time_base,
                                     out_stream->time_base);
                av_interleaved_write_frame(ofmt_ctx, enc_pkt);
                av_packet_unref(enc_pkt);
            }
            frame_count++;
            av_frame_unref(filt_frame);
        }
        av_frame_unref(frame);
    }

    // 8. flush filter graph
    av_buffersrc_add_frame_flags(buffersrc_ctx, nullptr, AV_BUFFERSRC_FLAG_KEEP_REF);
    while ((ret = av_buffersink_get_frame(buffersink_ctx, filt_frame)) >= 0) {
        filt_frame->pts = frame_count;

        ret = avcodec_send_frame(enc_ctx, filt_frame);
        if (ret < 0) {
            av_frame_unref(filt_frame);
            continue;
        }

        while ((ret = avcodec_receive_packet(enc_ctx, enc_pkt)) >= 0) {
            enc_pkt->stream_index = out_stream->index;
            av_packet_rescale_ts(enc_pkt, enc_ctx->time_base,
                                 out_stream->time_base);
            av_interleaved_write_frame(ofmt_ctx, enc_pkt);
            av_packet_unref(enc_pkt);
        }
        frame_count++;
        av_frame_unref(filt_frame);
    }

    // 9. flush encoder
    avcodec_send_frame(enc_ctx, nullptr);
    while ((ret = avcodec_receive_packet(enc_ctx, enc_pkt)) >= 0) {
        enc_pkt->stream_index = out_stream->index;
        av_packet_rescale_ts(enc_pkt, enc_ctx->time_base,
                             out_stream->time_base);
        av_interleaved_write_frame(ofmt_ctx, enc_pkt);
        av_packet_unref(enc_pkt);
    }

    av_write_trailer(ofmt_ctx);

    std::cout << "transcode done:" << std::endl;
    std::cout << "  frames: " << frame_count << std::endl;
    std::cout << "  output: " << output_file << std::endl;
    std::cout << "  verify: ffplay " << output_file << std::endl;

    // 9. cleanup
    av_frame_free(&filt_frame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    av_packet_free(&enc_pkt);
    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&enc_ctx);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&ofmt_ctx);
    avformat_close_input(&fmt_ctx);

    return 0;
}
