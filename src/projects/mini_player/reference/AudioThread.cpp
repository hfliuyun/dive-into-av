#include "AudioThread.h"
#include <SDL3/SDL.h>

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
}

void audio_thread_proc(PlayerContext* ctx) {
    SDL_Log("Audio thread started.");
    PacketWrapper pw = {nullptr, 0};
    AVFrame* frame = av_frame_alloc();
    AVFrame* res_frame = av_frame_alloc();
    SwrContext* swr_ctx = nullptr;
    int last_serial = -1;

    const int out_freq = 44100;
    const int out_channels = 2;
    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, out_channels);
    const int bytes_per_sec = out_freq * out_channels * 2;

    swr_ctx = swr_alloc();
    swr_alloc_set_opts2(&swr_ctx, &out_layout, AV_SAMPLE_FMT_S16, out_freq,
                        &ctx->a_dec_ctx->ch_layout, ctx->a_dec_ctx->sample_fmt, ctx->a_dec_ctx->sample_rate, 0, nullptr);
    swr_init(swr_ctx);

    while (!ctx->quit) {
        if (ctx->paused) {
            SDL_Delay(10);
            continue;
        }

        if (!ctx->audio_pkt_queue.pop(pw)) break;

        if (pw.serial != ctx->seek_serial) {
            av_packet_free(&pw.pkt);
            continue;
        }

        if (last_serial != pw.serial) {
            avcodec_flush_buffers(ctx->a_dec_ctx);
            last_serial = pw.serial;
        }

        if (avcodec_send_packet(ctx->a_dec_ctx, pw.pkt) >= 0) {
            while (avcodec_receive_frame(ctx->a_dec_ctx, frame) == 0) {
                int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
                res_frame->nb_samples = out_samples;
                res_frame->format = AV_SAMPLE_FMT_S16;
                av_channel_layout_copy(&res_frame->ch_layout, &out_layout);
                av_frame_get_buffer(res_frame, 0);

                swr_convert(swr_ctx, res_frame->data, out_samples, (const uint8_t**)frame->data, frame->nb_samples);

                // 推送到 SDL
                while (!ctx->quit && SDL_GetAudioStreamAvailable(ctx->a_stream) > bytes_per_sec * 0.5) {
                    SDL_Delay(10);
                    if (pw.serial != ctx->seek_serial) break;
                }
                
                if (!ctx->quit && pw.serial == ctx->seek_serial) {
                    SDL_PutAudioStreamData(ctx->a_stream, res_frame->data[0], res_frame->nb_samples * out_channels * 2);
                    
                    double pts = frame->best_effort_timestamp * av_q2d(ctx->fmt_ctx->streams[ctx->audio_idx]->time_base);
                    double buffered_duration = (double)SDL_GetAudioStreamAvailable(ctx->a_stream) / bytes_per_sec;
                    ctx->audio_clock = pts - buffered_duration;
                }

                av_frame_unref(res_frame);
                av_frame_unref(frame);
            }
        }
        av_packet_free(&pw.pkt);
    }

    swr_free(&swr_ctx);
    av_frame_free(&frame);
    av_frame_free(&res_frame);
    av_channel_layout_uninit(&out_layout);
    SDL_Log("Audio thread finished.");
}
