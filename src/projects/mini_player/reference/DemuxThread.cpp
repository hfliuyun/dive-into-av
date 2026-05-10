#include "DemuxThread.h"
#include <SDL3/SDL.h>

extern "C" {
#include <libavutil/time.h>
}

static void flush_pkt_queue(SafeQueue<PacketWrapper>& q) {
    auto items = q.flush();
    for (auto pw : items) av_packet_free(&pw.pkt);
}
static void flush_frame_queue(SafeQueue<FrameWrapper>& q) {
    auto items = q.flush();
    for (auto fw : items) av_frame_free(&fw.frame);
}

void demux_thread_proc(PlayerContext* ctx) {
    SDL_Log("Demux thread started.");
    
    while (!ctx->quit) {
        // 处理 Seek
        if (ctx->seek_pos >= 0) {
            int64_t target = (int64_t)(ctx->seek_pos * AV_TIME_BASE);
            if (avformat_seek_file(ctx->fmt_ctx, -1, INT64_MIN, target, target, AVSEEK_FLAG_BACKWARD) >= 0) {
                ctx->seek_serial++;
                
                flush_pkt_queue(ctx->video_pkt_queue);
                flush_pkt_queue(ctx->audio_pkt_queue);
                flush_frame_queue(ctx->video_frame_queue);
                
                if (ctx->a_stream) SDL_ClearAudioStream(ctx->a_stream);
                
                ctx->audio_clock = (double)ctx->seek_pos;
                ctx->video_pts = (double)ctx->seek_pos;

                // Seek 成功后主动推送一个刷新事件，让主线程尽快渲染新位置的画面
                SDL_Event event;
                SDL_zero(event);
                event.type = ctx->refresh_event;
                SDL_PushEvent(&event);
            }
            ctx->seek_pos = -1.0;
        }

        // 流量控制
        if (ctx->video_pkt_queue.size() > 50 || ctx->audio_pkt_queue.size() > 50) {
            SDL_Delay(10);
            continue;
        }

        AVPacket* pkt = av_packet_alloc();
        int ret = av_read_frame(ctx->fmt_ctx, pkt);
        if (ret >= 0) {
            if (pkt->stream_index == ctx->video_idx) {
                if (!ctx->video_pkt_queue.push({pkt, ctx->seek_serial.load()})) av_packet_free(&pkt);
            } else if (pkt->stream_index == ctx->audio_idx) {
                if (!ctx->audio_pkt_queue.push({pkt, ctx->seek_serial.load()})) av_packet_free(&pkt);
            } else {
                av_packet_free(&pkt);
            }
        } else {
            av_packet_free(&pkt);
            if (ret == AVERROR_EOF) {
                // 等待 Seek 或 退出信号，但不建议死等，应周期性检查 quit 标志
                SDL_Delay(10); 
            } else {
                SDL_Log("Read error: %d", ret);
                break;
            }
        }
    }
    
    ctx->video_pkt_queue.abort();
    ctx->audio_pkt_queue.abort();
    SDL_Log("Demux thread finished.");
}
