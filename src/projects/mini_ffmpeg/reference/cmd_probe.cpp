#include "common.h"

int cmd_probe(const Config& config) {
    AVFormatContext* fmt_ctx = nullptr;
    int ret = 0;

    ret = avformat_open_input(&fmt_ctx, config.input_file.c_str(), nullptr, nullptr);
    CHECK_RET(ret, "Cannot open input file");

    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    CHECK_RET(ret, "Cannot find stream info");

    std::cout << "File: " << config.input_file << "\n";
    std::cout << "Duration: " << fmt_ctx->duration / AV_TIME_BASE << "s\n";
    std::cout << "Number of streams: " << fmt_ctx->nb_streams << "\n";

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        AVCodecParameters* params = stream->codecpar;

        std::cout << "Stream #" << i << ": ";
        if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
            std::cout << "Video (" << avcodec_get_name(params->codec_id) << "), "
                      << params->width << "x" << params->height << ", "
                      << av_q2d(stream->avg_frame_rate) << " fps\n";
        } else if (params->codec_type == AVMEDIA_TYPE_AUDIO) {
            std::cout << "Audio (" << avcodec_get_name(params->codec_id) << "), "
                      << params->sample_rate << " Hz, "
                      << params->ch_layout.nb_channels << " channels\n";
        } else {
            std::cout << "Other\n";
        }
    }

end:
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    return ret;
}
