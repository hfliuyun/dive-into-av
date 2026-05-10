#include "VideoThread.h"
#include <SDL3/SDL.h>

void video_thread_proc(PlayerContext* ctx) {
    SDL_Log("Video thread started.");
    PacketWrapper pw = {nullptr, 0};
    int last_serial = -1;

    while (!ctx->quit) {
        if (ctx->paused) {
            SDL_Delay(10);
            continue;
        }

        if (!ctx->video_pkt_queue.pop(pw)) break;

        if (pw.serial != ctx->seek_serial) {
            av_packet_free(&pw.pkt);
            continue;
        }

        if (last_serial != pw.serial) {
            avcodec_flush_buffers(ctx->v_dec_ctx);
            last_serial = pw.serial;
        }

        if (avcodec_send_packet(ctx->v_dec_ctx, pw.pkt) >= 0) {
            while (true) {
                AVFrame* frame = av_frame_alloc();
                int ret = avcodec_receive_frame(ctx->v_dec_ctx, frame);
                if (ret == 0) {
                    if (!ctx->video_frame_queue.push({frame, pw.serial})) {
                        av_frame_free(&frame);
                    }
                } else {
                    av_frame_free(&frame);
                    break;
                }
            }
        }
        av_packet_free(&pw.pkt);
    }
    
    ctx->video_frame_queue.abort();
    SDL_Log("Video thread finished.");
}
