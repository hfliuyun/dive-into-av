#include <iostream>
#include <thread>
#include <SDL3/SDL.h>
#include <csignal>
#include "PlayerContext.h"
#include "DemuxThread.h"
#include "VideoThread.h"
#include "AudioThread.h"

// 全局指针用于信号处理
static PlayerContext* g_ctx_ptr = nullptr;
void signal_handler(int sig) {
    if (g_ctx_ptr) {
        g_ctx_ptr->quit = true;
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&event);
    }
}

uint32_t sdl_refresh_timer_cb(void* userdata, SDL_TimerID timerID, uint32_t interval) {
    PlayerContext* ctx = (PlayerContext*)userdata;
    if (ctx->quit) return 0;
    SDL_Event event;
    SDL_zero(event);
    event.type = ctx->refresh_event;
    SDL_PushEvent(&event);
    return 0; 
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return -1;
    }

    PlayerContext ctx;
    g_ctx_ptr = &ctx;
    std::signal(SIGINT, signal_handler);
    
    // TODO: 1. FFmpeg 初始化 (avformat_open_input, avformat_find_stream_info, av_find_best_stream)
    // 提示：设置 fmt_ctx->interrupt_callback 以支持快速退出

    // TODO: 2. 初始化音视频解码器 (avcodec_alloc_context3, avcodec_parameters_to_context, avcodec_open2)

    // TODO: 3. SDL 初始化 (SDL_Init, SDL_CreateWindowAndRenderer, SDL_CreateTextureWithProperties)

    // TODO: 4. 初始化音频播放 (SDL_OpenAudioDeviceStream, SDL_ResumeAudioStreamDevice)

    // 5. 启动线程
    std::thread demux_thr(demux_thread_proc, &ctx);
    std::thread video_thr(video_thread_proc, &ctx);
    std::thread audio_thr(audio_thread_proc, &ctx);

    // 6. 注册事件并开启定时器
    ctx.refresh_event = SDL_RegisterEvents(1);
    SDL_AddTimer(40, sdl_refresh_timer_cb, &ctx);

    // 7. SDL 事件循环
    SDL_Event event;
    while (!ctx.quit) {
        if (SDL_WaitEvent(&event)) {
            // TODO: 处理刷新事件 (REFRESH_EVENT)
            // 核心思路：pop 帧 -> 换算 pts -> 计算 diff -> 渲染 -> av_frame_free -> 开启下一次定时器

            // TODO: 处理按键事件 (SDL_EVENT_KEY_DOWN)
            // 提示：Space 切换暂停，Left/Right 增减 ctx->seek_pos

            if (event.type == SDL_EVENT_QUIT) {
                ctx.quit = true;
            }
        }
    }

    // 8. 优雅退出流程
    ctx.quit = true;
    ctx.video_pkt_queue.abort();
    ctx.audio_pkt_queue.abort();
    ctx.video_frame_queue.abort();
    
    if (demux_thr.joinable()) demux_thr.join();
    if (video_thr.joinable()) video_thr.join();
    if (audio_thr.joinable()) audio_thr.join();

    // TODO: 释放队列中残留的 Packet 和 Frame (非常重要，否则会泄露)

    // TODO: 释放 FFmpeg 和 SDL 资源
    
    return 0;
}
