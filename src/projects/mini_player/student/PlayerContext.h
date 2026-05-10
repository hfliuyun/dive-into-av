#ifndef PLAYER_CONTEXT_H
#define PLAYER_CONTEXT_H

#include <atomic>
#include <SDL3/SDL.h>
#include "SafeQueue.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

struct PacketWrapper {
    AVPacket* pkt;
    int serial;
};

struct FrameWrapper {
    AVFrame* frame;
    int serial;
};

struct PlayerContext {
    // 资源句柄
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* v_dec_ctx = nullptr;
    AVCodecContext* a_dec_ctx = nullptr;
    
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_AudioStream* a_stream = nullptr;

    // 流索引
    int video_idx = -1;
    int audio_idx = -1;

    // 队列
    SafeQueue<PacketWrapper> video_pkt_queue{100};
    SafeQueue<PacketWrapper> audio_pkt_queue{100};
    SafeQueue<FrameWrapper> video_frame_queue{3};

    // 时钟与同步
    std::atomic<double> audio_clock{0.0};
    std::atomic<double> video_pts{0.0};
    double frame_delay = 0.04; // 默认 25fps

    // 控制标志
    std::atomic<bool> quit{false};
    std::atomic<bool> paused{false};
    std::atomic<double> seek_pos{-1.0};
    std::atomic<int> seek_serial{0};

    // 事件
    uint32_t refresh_event = 0;

    // FFmpeg 中断回调处理
    static int interrupt_cb(void* ctx) {
        PlayerContext* p = (PlayerContext*)ctx;
        return p->quit ? 1 : 0;
    }

    PlayerContext() = default;
};

#endif // PLAYER_CONTEXT_H
