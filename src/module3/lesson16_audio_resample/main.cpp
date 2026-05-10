#include <iostream>
#include <cstdio>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

static void print_usage(const char *prog) {
    std::cerr << "Usage: " << prog << " <input.mp4> <output.mp4>" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Audio resampling options:" << std::endl;
    std::cerr << "  -ar <rate>   Set output sample rate (default: 44100)" << std::endl;
    std::cerr << "  -ac <ch>     Set output channels (default: 1)" << std::endl;
    std::cerr << "  -af <fmt>    Set output sample format (default: s16)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Supported sample formats:" << std::endl;
    std::cerr << "  s16  - Signed 16-bit" << std::endl;
    std::cerr << "  s32  - Signed 32-bit" << std::endl;
    std::cerr << "  flt  - Float" << std::endl;
    std::cerr << "  dbl  - Double" << std::endl;
    std::cerr << "  s16p - Signed 16-bit planar" << std::endl;
    std::cerr << "  fltp - Float planar" << std::endl;
}

static AVSampleFormat parse_sample_fmt(const char *name) {
    if (strcmp(name, "s16") == 0) return AV_SAMPLE_FMT_S16;
    if (strcmp(name, "s32") == 0) return AV_SAMPLE_FMT_S32;
    if (strcmp(name, "flt") == 0) return AV_SAMPLE_FMT_FLT;
    if (strcmp(name, "dbl") == 0) return AV_SAMPLE_FMT_DBL;
    if (strcmp(name, "s16p") == 0) return AV_SAMPLE_FMT_S16P;
    if (strcmp(name, "s32p") == 0) return AV_SAMPLE_FMT_S32P;
    if (strcmp(name, "fltp") == 0) return AV_SAMPLE_FMT_FLTP;
    if (strcmp(name, "dblp") == 0) return AV_SAMPLE_FMT_DBLP;
    return AV_SAMPLE_FMT_S16;
}

static const char *sample_fmt_name(AVSampleFormat fmt) {
    switch (fmt) {
        case AV_SAMPLE_FMT_NONE: return "none";
        case AV_SAMPLE_FMT_U8:   return "u8";
        case AV_SAMPLE_FMT_S16:  return "s16";
        case AV_SAMPLE_FMT_S32:  return "s32";
        case AV_SAMPLE_FMT_FLT:  return "flt";
        case AV_SAMPLE_FMT_DBL:  return "dbl";
        case AV_SAMPLE_FMT_U8P:  return "u8p";
        case AV_SAMPLE_FMT_S16P: return "s16p";
        case AV_SAMPLE_FMT_S32P: return "s32p";
        case AV_SAMPLE_FMT_FLTP: return "fltp";
        case AV_SAMPLE_FMT_DBLP: return "dblp";
        default: return "unknown";
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    int out_sample_rate = 44100;
    int out_channels = 1;
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLTP;  // AAC requires FLTP

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-ar") == 0 && i + 1 < argc) {
            out_sample_rate = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ac") == 0 && i + 1 < argc) {
            out_channels = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-af") == 0 && i + 1 < argc) {
            out_sample_fmt = parse_sample_fmt(argv[++i]);
        }
    }

    int ret = 0;

    AVFormatContext *fmt_ctx = nullptr;
    ret = avformat_open_input(&fmt_ctx, input_file, nullptr, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "Failed to open input: " << err_buf << std::endl;
        return 1;
    }

    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "Failed to find stream info: " << err_buf << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    int audio_idx = -1;
    AVCodecContext *dec_ctx = nullptr;

    for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_idx = i;
            const AVCodec *codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
            if (!codec) {
                std::cerr << "Audio decoder not found" << std::endl;
                avformat_close_input(&fmt_ctx);
                return 1;
            }
            dec_ctx = avcodec_alloc_context3(codec);
            if (!dec_ctx) {
                avformat_close_input(&fmt_ctx);
                return 1;
            }
            ret = avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[i]->codecpar);
            if (ret < 0) {
                avcodec_free_context(&dec_ctx);
                avformat_close_input(&fmt_ctx);
                return 1;
            }
            ret = avcodec_open2(dec_ctx, codec, nullptr);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, err_buf, sizeof(err_buf));
                std::cerr << "Failed to open decoder: " << err_buf << std::endl;
                avcodec_free_context(&dec_ctx);
                avformat_close_input(&fmt_ctx);
                return 1;
            }
            break;
        }
    }

    if (audio_idx == -1) {
        std::cerr << "No audio stream found" << std::endl;
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Get input parameters from decoder
    AVStream *audio_stream = fmt_ctx->streams[audio_idx];
    int in_sample_rate = dec_ctx->sample_rate;
    AVSampleFormat in_sample_fmt = dec_ctx->sample_fmt;
    AVChannelLayout in_ch_layout = audio_stream->codecpar->ch_layout.nb_channels > 0 ? 
        audio_stream->codecpar->ch_layout : dec_ctx->ch_layout;
    
    // Fallback to stereo if not set
    if (in_ch_layout.nb_channels == 0) {
        av_channel_layout_from_mask(&in_ch_layout, AV_CH_LAYOUT_STEREO);
    }
    
    // Configure output
    AVChannelLayout out_ch_layout;
    av_channel_layout_from_mask(&out_ch_layout, out_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO);

    // Create SwrContext using the newer API (FFmpeg 6+)
    SwrContext *swr_ctx = nullptr;
    ret = swr_alloc_set_opts2(&swr_ctx,
        &out_ch_layout, out_sample_fmt, out_sample_rate,
        &in_ch_layout, in_sample_fmt, in_sample_rate,
        0, nullptr);
    
    if (ret < 0 || !swr_ctx) {
        std::cerr << "Failed to allocate resampler" << std::endl;
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    ret = swr_init(swr_ctx);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "Failed to initialize resampler: " << err_buf << std::endl;
        swr_free(&swr_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    AVFormatContext *ofmt_ctx = nullptr;
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, output_file);
    if (!ofmt_ctx) {
        std::cerr << "Failed to create output context" << std::endl;
        swr_free(&swr_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!encoder) {
        encoder = avcodec_find_encoder_by_name("libfdk_aac");
    }
    if (!encoder) {
        encoder = avcodec_find_encoder(AV_CODEC_ID_MP3);
    }
    if (!encoder) {
        std::cerr << "No audio encoder available" << std::endl;
        swr_free(&swr_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    AVCodecContext *enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
        std::cerr << "Failed to allocate encoder context" << std::endl;
        swr_free(&swr_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    enc_ctx->ch_layout = out_ch_layout;
    enc_ctx->sample_rate = out_sample_rate;
    enc_ctx->sample_fmt = out_sample_fmt;
    enc_ctx->bit_rate = 128000;

    ret = avcodec_open2(enc_ctx, encoder, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "Failed to open encoder: " << err_buf << std::endl;
        avcodec_free_context(&enc_ctx);
        swr_free(&swr_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    AVStream *out_stream = avformat_new_stream(ofmt_ctx, nullptr);
    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (ret < 0) {
        std::cerr << "Failed to copy encoder parameters" << std::endl;
        avcodec_free_context(&enc_ctx);
        swr_free(&swr_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }
    out_stream->time_base = AVRational{1, out_sample_rate};

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            char err_buf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, err_buf, sizeof(err_buf));
            std::cerr << "Failed to open output: " << err_buf << std::endl;
            avcodec_free_context(&enc_ctx);
            swr_free(&swr_ctx);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&ofmt_ctx);
            avformat_close_input(&fmt_ctx);
            return 1;
        }
    }

    ret = avformat_write_header(ofmt_ctx, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "Failed to write header: " << err_buf << std::endl;
        avcodec_free_context(&enc_ctx);
        swr_free(&swr_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    std::cout << "Audio Resampling Pipeline:" << std::endl;
    std::cout << "  Input:  " << dec_ctx->sample_rate << "Hz, "
              << dec_ctx->ch_layout.nb_channels << "ch, "
              << sample_fmt_name(dec_ctx->sample_fmt) << std::endl;
    std::cout << "  Output: " << out_sample_rate << "Hz, "
              << out_channels << "ch, "
              << sample_fmt_name(out_sample_fmt) << std::endl;
    std::cout << "  Encoder: " << encoder->name << std::endl;
    std::cout << std::endl;

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *resampled_frame = av_frame_alloc();

    if (!packet || !frame || !resampled_frame) {
        std::cerr << "Failed to allocate packet/frame" << std::endl;
        av_frame_free(&resampled_frame);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&enc_ctx);
        swr_free(&swr_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    int max_out_samples = 1024;
    int out_linesize = 0;
    ret = av_samples_alloc(resampled_frame->data, &out_linesize, out_channels,
                     max_out_samples, out_sample_fmt, 0);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << "Failed to allocate sample buffer: " << err_buf << std::endl;
        av_frame_free(&resampled_frame);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&enc_ctx);
        swr_free(&swr_ctx);
        avcodec_free_context(&dec_ctx);
        av_channel_layout_uninit(&in_ch_layout);
        av_channel_layout_uninit(&out_ch_layout);
        avformat_close_input(&ofmt_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }
    resampled_frame->nb_samples = 0;
    resampled_frame->format = out_sample_fmt;
    resampled_frame->ch_layout = out_ch_layout;

    int frame_count = 0;

    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index != audio_idx) {
            av_packet_unref(packet);
            continue;
        }

        ret = avcodec_send_packet(dec_ctx, packet);
        if (ret < 0) {
            av_packet_unref(packet);
            continue;
        }

        while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
            int out_samples = swr_convert(swr_ctx, 
                resampled_frame->data, max_out_samples,
                (const uint8_t **)frame->data, frame->nb_samples);

            if (out_samples < 0) {
                av_frame_unref(frame);
                continue;
            }

            resampled_frame->nb_samples = out_samples;
            resampled_frame->pts = frame_count;

            ret = avcodec_send_frame(enc_ctx, resampled_frame);
            if (ret < 0) {
                av_frame_unref(frame);
                continue;
            }

            AVPacket *enc_pkt = av_packet_alloc();
            while (avcodec_receive_packet(enc_ctx, enc_pkt) >= 0) {
                enc_pkt->stream_index = out_stream->index;
                av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, out_stream->time_base);
                av_interleaved_write_frame(ofmt_ctx, enc_pkt);
                av_packet_unref(enc_pkt);
            }
            av_packet_free(&enc_pkt);

            frame_count++;
            av_frame_unref(frame);
        }
        av_packet_unref(packet);
    }

    swr_close(swr_ctx);

    while (swr_get_out_samples(swr_ctx, 0) > 0) {
        int out_samples = swr_convert(swr_ctx, 
            resampled_frame->data, max_out_samples,
            nullptr, 0);
        
        if (out_samples <= 0) break;

        resampled_frame->nb_samples = out_samples;
        resampled_frame->pts = frame_count;

        ret = avcodec_send_frame(enc_ctx, resampled_frame);
        if (ret < 0) break;

        AVPacket *enc_pkt = av_packet_alloc();
        while (avcodec_receive_packet(enc_ctx, enc_pkt) >= 0) {
            enc_pkt->stream_index = out_stream->index;
            av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, out_stream->time_base);
            av_interleaved_write_frame(ofmt_ctx, enc_pkt);
            av_packet_unref(enc_pkt);
        }
        av_packet_free(&enc_pkt);
        frame_count++;
    }

    avcodec_send_frame(enc_ctx, nullptr);
    AVPacket *enc_pkt = av_packet_alloc();
    while (avcodec_receive_packet(enc_ctx, enc_pkt) >= 0) {
        enc_pkt->stream_index = out_stream->index;
        av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, out_stream->time_base);
        av_interleaved_write_frame(ofmt_ctx, enc_pkt);
        av_packet_unref(enc_pkt);
    }
    av_packet_free(&enc_pkt);

    av_write_trailer(ofmt_ctx);

    std::cout << "Resampling done:" << std::endl;
    std::cout << "  Output frames: " << frame_count << std::endl;
    std::cout << "  Output file: " << output_file << std::endl;
    std::cout << "  Verify: ffplay " << output_file << std::endl;

    av_freep(&resampled_frame->data[0]);
    av_frame_free(&resampled_frame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&enc_ctx);
    swr_free(&swr_ctx);
    avcodec_free_context(&dec_ctx);
    av_channel_layout_uninit(&in_ch_layout);
    av_channel_layout_uninit(&out_ch_layout);
    avformat_close_input(&ofmt_ctx);
    avformat_close_input(&fmt_ctx);

    return 0;
}
